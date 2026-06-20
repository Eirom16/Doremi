use crate::config::paths::AppDirs;
use std::path::Path;

fn dir_size_excluding(path: &Path, exclude: Option<&Path>) -> u64 {
    if !path.exists() {
        return 0;
    }
    if let Some(exc) = exclude {
        if path == exc {
            return 0;
        }
    }
    if path.is_file() {
        return std::fs::metadata(path).map(|m| m.len()).unwrap_or(0);
    }
    let mut size = 0;
    if let Ok(entries) = std::fs::read_dir(path) {
        for entry in entries.flatten() {
            size += dir_size_excluding(&entry.path(), exclude);
        }
    }
    size
}

pub fn get_storage_sizes() -> Vec<f64> {
    let dirs = AppDirs::global();
    let db_path = dirs.database_path();

    // 1. Calculate database size (db, wal, shm)
    let mut db_bytes = 0;
    if db_path.exists() {
        db_bytes += std::fs::metadata(&db_path).map(|m| m.len()).unwrap_or(0);
    }
    let wal_path = db_path.with_extension("db-wal");
    if wal_path.exists() {
        db_bytes += std::fs::metadata(&wal_path).map(|m| m.len()).unwrap_or(0);
    }
    let shm_path = db_path.with_extension("db-shm");
    if shm_path.exists() {
        db_bytes += std::fs::metadata(&shm_path).map(|m| m.len()).unwrap_or(0);
    }

    // 2. Calculate Cache size (excluding downloads)
    let cache_dir = dirs.cache_dir();
    let downloads_dir = cache_dir.join("downloads");
    let cache_bytes = dir_size_excluding(&cache_dir, Some(&downloads_dir));

    // 3. Calculate Downloads size
    let downloads_bytes = dir_size_excluding(&downloads_dir, None);

    // Convert bytes to Megabytes (1 MB = 1024 * 1024 bytes)
    let to_mb = |bytes: u64| (bytes as f64) / (1024.0 * 1024.0);

    vec![to_mb(db_bytes), to_mb(cache_bytes), to_mb(downloads_bytes)]
}

pub fn clear_cache() {
    let dirs = AppDirs::global();
    let cache_dir = dirs.cache_dir();
    let downloads_dir = cache_dir.join("downloads");

    if let Ok(entries) = std::fs::read_dir(&cache_dir) {
        for entry in entries.flatten() {
            let path = entry.path();
            if path == downloads_dir {
                continue;
            }
            if path.is_dir() {
                let _ = std::fs::remove_dir_all(&path);
            } else {
                let _ = std::fs::remove_file(&path);
            }
        }
    }
    std::fs::create_dir_all(dirs.artwork_cache_dir()).ok();
    std::fs::create_dir_all(dirs.lyrics_cache_dir()).ok();
    log::info!("Cache cleared successfully (downloads preserved)");
}

pub fn clear_downloads() {
    crate::services::download::DownloadManager::get_instance().clear_all();
    let dirs = AppDirs::global();
    let downloads_dir = dirs.cache_dir().join("downloads");
    let _ = std::fs::remove_dir_all(&downloads_dir);
    let _ = std::fs::create_dir_all(&downloads_dir);
    log::info!("Downloads cleared successfully");
}
