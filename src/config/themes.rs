use std::collections::HashMap;

#[derive(Debug, Clone)]
pub struct ColorScheme {
    pub bg_base: &'static str,
    pub bg_surface: &'static str,
    pub bg_elevated: &'static str,
    pub bg_high: &'static str,
    pub bg_overlay: &'static str,
    pub accent: &'static str,
    pub accent_bright: &'static str,
    pub accent_dim: &'static str,
    pub secondary: &'static str,
    pub secondary_dim: &'static str,
    pub text_primary: &'static str,
    pub text_secondary: &'static str,
    pub text_disabled: &'static str,
    pub text_on_accent: &'static str,
    pub border: &'static str,
    pub border_focus: &'static str,
    pub success: &'static str,
    pub warning: &'static str,
    pub error: &'static str,
    pub info: &'static str,
    pub like_color: &'static str,
}

pub static DARK: ColorScheme = ColorScheme {
    bg_base: "#0A0A14",
    bg_surface: "#10101E",
    bg_elevated: "#16162A",
    bg_high: "#1E1E38",
    bg_overlay: "rgba(10,10,20,0.85)",
    accent: "#A78BFA",
    accent_bright: "#8B5CF6",
    accent_dim: "rgba(167,139,250,0.15)",
    secondary: "#22D3EE",
    secondary_dim: "rgba(34,211,238,0.15)",
    text_primary: "#F1F0FF",
    text_secondary: "#9B9BC0",
    text_disabled: "#4A4A6A",
    text_on_accent: "#0A0A14",
    border: "rgba(167,139,250,0.12)",
    border_focus: "rgba(167,139,250,0.50)",
    success: "#34D399",
    warning: "#FBBF24",
    error: "#F87171",
    info: "#60A5FA",
    like_color: "#F472B6",
};

pub static LIGHT: ColorScheme = ColorScheme {
    bg_base: "#F3F3F9",
    bg_surface: "#FFFFFF",
    bg_elevated: "#E8E8F0",
    bg_high: "#DFDFE8",
    bg_overlay: "rgba(243,243,249,0.85)",
    accent: "#A78BFA",
    accent_bright: "#8B5CF6",
    accent_dim: "rgba(167,139,250,0.15)",
    secondary: "#06B6D4",
    secondary_dim: "rgba(6,182,212,0.15)",
    text_primary: "#121224",
    text_secondary: "#5C5C8A",
    text_disabled: "#9E9EBF",
    text_on_accent: "#FFFFFF",
    border: "rgba(167,139,250,0.12)",
    border_focus: "rgba(167,139,250,0.50)",
    success: "#10B981",
    warning: "#F59E0B",
    error: "#EF4444",
    info: "#3B82F6",
    like_color: "#EC4899",
};

use std::sync::atomic::{AtomicBool, AtomicUsize, Ordering};

pub static CURRENT_THEME_MODE: AtomicUsize = AtomicUsize::new(0); // 0 = dark, 1 = light
pub static THEME_APPLYING: AtomicBool = AtomicBool::new(false);

pub fn current() -> &'static ColorScheme {
    if CURRENT_THEME_MODE.load(Ordering::Relaxed) == 0 {
        &DARK
    } else {
        &LIGHT
    }
}

pub fn set_theme_mode(dark: bool) {
    CURRENT_THEME_MODE.store(if dark { 0 } else { 1 }, Ordering::Relaxed);
}

/// EQ band labels in Hz
pub const EQ_BAND_LABELS: [&str; 10] = [
    "60Hz", "170Hz", "310Hz", "600Hz", "1kHz", "3kHz", "6kHz", "12kHz", "14kHz", "16kHz",
];

/// Equalizer presets: (preamp, [10 bands])
pub fn eq_presets() -> HashMap<&'static str, (f64, [f64; 10])> {
    let mut m = HashMap::new();
    m.insert("Flat", (0.0, [0.0; 10]));
    m.insert(
        "Bass Boost",
        (2.0, [6.0, 5.0, 4.0, 2.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0]),
    );
    m.insert(
        "Treble Boost",
        (0.0, [0.0, 0.0, 0.0, 0.0, 0.0, 2.0, 4.0, 5.0, 6.0, 6.0]),
    );
    m.insert(
        "Vocal",
        (0.0, [-2.0, -1.0, 0.0, 2.0, 4.0, 4.0, 3.0, 2.0, 1.0, 0.0]),
    );
    m.insert(
        "Classical",
        (0.0, [4.0, 3.0, 2.0, 0.0, 0.0, 0.0, 0.0, 2.0, 3.0, 4.0]),
    );
    m.insert(
        "Electronic",
        (2.0, [4.0, 3.0, 0.0, 2.0, 0.0, 0.0, 2.0, 3.0, 4.0, 4.0]),
    );
    m.insert(
        "Hip-Hop",
        (2.0, [5.0, 4.0, 2.0, 3.0, 0.0, 0.0, 1.0, 2.0, 3.0, 4.0]),
    );
    m.insert(
        "Rock",
        (1.0, [4.0, 3.0, 2.0, 0.0, -1.0, -1.0, 0.0, 2.0, 3.0, 4.0]),
    );
    m.insert(
        "Jazz",
        (0.0, [3.0, 2.0, 1.0, 2.0, 0.0, 0.0, 1.0, 2.0, 3.0, 3.0]),
    );
    m.insert(
        "Pop",
        (0.0, [-1.0, 0.0, 2.0, 3.0, 4.0, 3.0, 2.0, 0.0, -1.0, -1.0]),
    );
    m
}

/// Extract dominant color from an image URL (async)
pub async fn extract_dominant_color(image_url: &str) -> Option<String> {
    let client = reqwest::Client::builder()
        .timeout(std::time::Duration::from_secs(5))
        .build()
        .ok()?;
    let resp = client.get(image_url).send().await.ok()?;
    let bytes = resp.bytes().await.ok()?;
    extract_dominant_from_bytes(&bytes)
}

fn extract_dominant_from_bytes(bytes: &[u8]) -> Option<String> {
    let img = image::load_from_memory(bytes).ok()?;
    let rgb = img.to_rgb8();
    let (w, h) = rgb.dimensions();
    if w == 0 || h == 0 {
        return None;
    }

    // Sample center zone (25%-75%)
    let x_start = (w as f64 * 0.25) as u32;
    let x_end = (w as f64 * 0.75) as u32;
    let y_start = (h as f64 * 0.25) as u32;
    let y_end = (h as f64 * 0.75) as u32;

    let mut r_sum: u64 = 0;
    let mut g_sum: u64 = 0;
    let mut b_sum: u64 = 0;
    let mut count: u64 = 0;

    for y in y_start..y_end {
        for x in x_start..x_end {
            let pixel = rgb.get_pixel(x, y);
            r_sum += pixel[0] as u64;
            g_sum += pixel[1] as u64;
            b_sum += pixel[2] as u64;
            count += 1;
        }
    }

    if count == 0 {
        return None;
    }

    let r_avg = (r_sum / count) as f64 / 255.0;
    let g_avg = (g_sum / count) as f64 / 255.0;
    let b_avg = (b_sum / count) as f64 / 255.0;

    // Convert RGB -> HSV, adjust min saturation/value, then back
    let (hue, sat, val) = rgb_to_hsv(r_avg, g_avg, b_avg);
    let sat = sat.max(0.5);
    let val = val.max(0.6);
    let (r, g, b) = hsv_to_rgb(hue, sat, val);

    Some(format!(
        "#{:02X}{:02X}{:02X}",
        (r * 255.0) as u8,
        (g * 255.0) as u8,
        (b * 255.0) as u8
    ))
}

fn rgb_to_hsv(r: f64, g: f64, b: f64) -> (f64, f64, f64) {
    let max = r.max(g).max(b);
    let min = r.min(g).min(b);
    let delta = max - min;

    let hue = if delta < 1e-10 {
        0.0
    } else if (max - r).abs() < 1e-10 {
        60.0 * (((g - b) / delta) % 6.0)
    } else if (max - g).abs() < 1e-10 {
        60.0 * (((b - r) / delta) + 2.0)
    } else {
        60.0 * (((r - g) / delta) + 4.0)
    };
    let hue = if hue < 0.0 { hue + 360.0 } else { hue };

    let sat = if max < 1e-10 { 0.0 } else { delta / max };
    let val = max;

    (hue, sat, val)
}

fn hsv_to_rgb(h: f64, s: f64, v: f64) -> (f64, f64, f64) {
    let c = v * s;
    let x = c * (1.0 - ((h / 60.0) % 2.0 - 1.0).abs());
    let m = v - c;

    let (r1, g1, b1) = match h as i32 {
        0..=59 => (c, x, 0.0),
        60..=119 => (x, c, 0.0),
        120..=179 => (0.0, c, x),
        180..=239 => (0.0, x, c),
        240..=299 => (x, 0.0, c),
        _ => (c, 0.0, x),
    };

    (r1 + m, g1 + m, b1 + m)
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_color_conversion() {
        let (h, s, v) = rgb_to_hsv(1.0, 0.5, 0.0);
        assert!((h - 30.0).abs() < 0.1);
        let (r, _g, _b) = hsv_to_rgb(h, s.max(0.5), v.max(0.6));
        assert!(r > 0.0);
    }

    #[test]
    fn test_eq_presets_count() {
        let presets = eq_presets();
        assert_eq!(presets.len(), 10);
    }
}
