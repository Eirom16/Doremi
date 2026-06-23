use image::GenericImageView;
use std::path::Path;
use std::time::UNIX_EPOCH;

/// Simple conversion from RGB to HSV
fn rgb_to_hsv(r: u8, g: u8, b: u8) -> (f32, f32, f32) {
    let r = r as f32 / 255.0;
    let g = g as f32 / 255.0;
    let b = b as f32 / 255.0;
    let max = r.max(g).max(b);
    let min = r.min(g).min(b);
    let delta = max - min;
    let h = if delta == 0.0 {
        0.0
    } else if max == r {
        60.0 * (((g - b) / delta) % 6.0)
    } else if max == g {
        60.0 * (((b - r) / delta) + 2.0)
    } else {
        60.0 * (((r - g) / delta) + 4.0)
    };
    let h = if h < 0.0 { h + 360.0 } else { h };
    let s = if max == 0.0 { 0.0 } else { delta / max };
    let v = max;
    (h, s, v)
}

/// Simple conversion from HSV to RGB
fn hsv_to_rgb(h: f32, s: f32, v: f32) -> (u8, u8, u8) {
    let c = v * s;
    let x = c * (1.0 - ((h / 60.0) % 2.0 - 1.0).abs());
    let m = v - c;
    let (r1, g1, b1) = if h < 60.0 {
        (c, x, 0.0)
    } else if h < 120.0 {
        (x, c, 0.0)
    } else if h < 180.0 {
        (0.0, c, x)
    } else if h < 240.0 {
        (0.0, x, c)
    } else if h < 300.0 {
        (x, 0.0, c)
    } else {
        (c, 0.0, x)
    };
    (
        ((r1 + m) * 255.0).round() as u8,
        ((g1 + m) * 255.0).round() as u8,
        ((b1 + m) * 255.0).round() as u8,
    )
}

/// Simple squared Euclidean distance between two RGB pixels
fn rgb_dist_sq(c1: &(f32, f32, f32), c2: &(f32, f32, f32)) -> f32 {
    let dr = c1.0 - c2.0;
    let dg = c1.1 - c2.1;
    let db = c1.2 - c2.2;
    dr * dr + dg * dg + db * db
}

/// Extract 3 dominant colors from an image, adjusted for dark mode ambient gradients.
/// Returns a list of hex strings like ["#1a2b3c", "#2d3e4f", "#3e4f5a"].
pub fn extract_dominant_colors<P: AsRef<Path>>(path: P) -> Vec<String> {
    let path = path.as_ref();
    let cache_key = dominant_colors_cache_key(path);
    if let Some(key) = &cache_key {
        if let Some(entry) = crate::db::cache::ResponseCache::get::<Vec<String>>(key) {
            return entry.data;
        }
    }

    let colors = extract_dominant_colors_uncached(path);
    if let Some(key) = &cache_key {
        let _ = crate::db::cache::ResponseCache::set(key, &colors, None);
    }
    colors
}

fn extract_dominant_colors_uncached(path: &Path) -> Vec<String> {
    let default_colors = vec![
        "#8B5CF6".to_string(), // Purple
        "#3B82F6".to_string(), // Blue
        "#EC4899".to_string(), // Pink
    ];

    let img = match image::open(path) {
        Ok(img) => img,
        Err(_) => return default_colors,
    };

    // Resize to 32x32 for fast processing
    let resized = img.thumbnail(32, 32);
    let pixels: Vec<(f32, f32, f32)> = resized
        .pixels()
        .map(|(_, _, p)| (p[0] as f32, p[1] as f32, p[2] as f32))
        .collect();

    if pixels.is_empty() {
        return default_colors;
    }

    // Initialize K-Means (k=3) centroids. We choose spread-out points.
    let n = pixels.len();
    let mut centroids = vec![
        pixels[n / 4].clone(),
        pixels[n / 2].clone(),
        pixels[(3 * n) / 4].clone(),
    ];

    // Iterative K-Means
    for _ in 0..8 {
        let mut groups = vec![Vec::new(); 3];
        for p in &pixels {
            let mut min_dist = f32::MAX;
            let mut best_idx = 0;
            for (idx, c) in centroids.iter().enumerate() {
                let dist = rgb_dist_sq(p, c);
                if dist < min_dist {
                    min_dist = dist;
                    best_idx = idx;
                }
            }
            groups[best_idx].push(p.clone());
        }

        // Update centroids
        for idx in 0..3 {
            if !groups[idx].is_empty() {
                let count = groups[idx].len() as f32;
                let mut sum_r = 0.0;
                let mut sum_g = 0.0;
                let mut sum_b = 0.0;
                for p in &groups[idx] {
                    sum_r += p.0;
                    sum_g += p.1;
                    sum_b += p.2;
                }
                centroids[idx] = (sum_r / count, sum_g / count, sum_b / count);
            }
        }
    }

    // Format & adjust centroids
    let mut result = Vec::new();
    for c in centroids {
        let r_u8 = c.0.clamp(0.0, 255.0) as u8;
        let g_u8 = c.1.clamp(0.0, 255.0) as u8;
        let b_u8 = c.2.clamp(0.0, 255.0) as u8;

        // Convert to HSV to perform dark mode styling adjustments
        let (h, mut s, mut v) = rgb_to_hsv(r_u8, g_u8, b_u8);

        // Roadmap: saturación mínima 40%, valor 25-50% (dark mode)
        s = s.max(0.40);
        v = v.clamp(0.25, 0.50);

        let (adj_r, adj_g, adj_b) = hsv_to_rgb(h, s, v);
        result.push(format!("#{:02X}{:02X}{:02X}", adj_r, adj_g, adj_b));
    }

    // Ensure we have exactly 3 colors
    while result.len() < 3 {
        result.push(default_colors[result.len()].clone());
    }

    result
}

fn dominant_colors_cache_key(path: &Path) -> Option<String> {
    let meta = std::fs::metadata(path).ok()?;
    let modified = meta
        .modified()
        .ok()
        .and_then(|value| value.duration_since(UNIX_EPOCH).ok())
        .map(|value| value.as_secs())
        .unwrap_or_default();
    let key_src = format!("{}:{}:{modified}", path.to_string_lossy(), meta.len());
    Some(format!(
        "color:dominant:{:x}",
        md5::compute(key_src.as_bytes())
    ))
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::db::{init_connection, take_connection, Database};

    fn setup_test_db() {
        let _ = take_connection();
        let conn = rusqlite::Connection::open_in_memory().unwrap();
        Database::run_migrations(&conn).unwrap();
        init_connection(conn);
    }

    #[test]
    fn dominant_colors_are_cached_by_file_identity() {
        let _guard = crate::db::TEST_MUTEX.lock().unwrap();
        setup_test_db();
        let path = std::env::temp_dir().join(format!(
            "doremi-color-cache-test-{}.png",
            std::process::id()
        ));
        let image = image::RgbImage::from_pixel(8, 8, image::Rgb([200, 40, 80]));
        image.save(&path).unwrap();

        let key = dominant_colors_cache_key(&path).unwrap();
        let colors = extract_dominant_colors(&path);
        let cached = crate::db::cache::ResponseCache::get::<Vec<String>>(&key).unwrap();

        assert_eq!(cached.data, colors);
        assert_eq!(colors.len(), 3);

        let _ = std::fs::remove_file(path);
        let _ = take_connection();
    }
}
