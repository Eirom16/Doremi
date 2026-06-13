use std::fs::File;
use std::io::{Read, Write};
use std::path::Path;
use crate::config::paths::AppDirs;

pub fn export_backup(zip_path: &Path) -> bool {
    let dirs = AppDirs::global();
    let db_path = dirs.database_path();
    let settings_path = dirs.settings_path();

    log::info!("Exporting backup to {:?}", zip_path);

    let file = match File::create(zip_path) {
        Ok(f) => f,
        Err(e) => {
            log::error!("Failed to create backup file: {e}");
            return false;
        }
    };

    let mut zip = zip::ZipWriter::new(file);
    let options = zip::write::FileOptions::default()
        .compression_method(zip::CompressionMethod::Deflated);

    // 1. Add database
    if db_path.exists() {
        if let Err(e) = zip.start_file("doremi.db", options) {
            log::error!("Failed to add doremi.db to zip: {e}");
            return false;
        }
        let mut db_file = match File::open(&db_path) {
            Ok(f) => f,
            Err(e) => {
                log::error!("Failed to open doremi.db: {e}");
                return false;
            }
        };
        let mut buffer = Vec::new();
        if let Err(e) = db_file.read_to_end(&mut buffer) {
            log::error!("Failed to read doremi.db: {e}");
            return false;
        }
        if let Err(e) = zip.write_all(&buffer) {
            log::error!("Failed to write doremi.db to zip: {e}");
            return false;
        }
    }

    // 2. Add settings
    if settings_path.exists() {
        if let Err(e) = zip.start_file("settings.toml", options) {
            log::error!("Failed to add settings.toml to zip: {e}");
            return false;
        }
        let mut settings_file = match File::open(&settings_path) {
            Ok(f) => f,
            Err(e) => {
                log::error!("Failed to open settings.toml: {e}");
                return false;
            }
        };
        let mut buffer = Vec::new();
        if let Err(e) = settings_file.read_to_end(&mut buffer) {
            log::error!("Failed to read settings.toml: {e}");
            return false;
        }
        if let Err(e) = zip.write_all(&buffer) {
            log::error!("Failed to write settings.toml to zip: {e}");
            return false;
        }
    }

    if let Err(e) = zip.finish() {
        log::error!("Failed to finish zip file: {e}");
        return false;
    }

    log::info!("Backup exported successfully");
    true
}

pub fn import_backup(zip_path: &Path) -> bool {
    log::info!("Importing backup from {:?}", zip_path);

    let file = match File::open(zip_path) {
        Ok(f) => f,
        Err(e) => {
            log::error!("Failed to open backup zip: {e}");
            return false;
        }
    };

    let mut archive = match zip::ZipArchive::new(file) {
        Ok(a) => a,
        Err(e) => {
            log::error!("Failed to parse zip archive: {e}");
            return false;
        }
    };

    // 1. Close current DB connection to allow file overwriting
    let _conn = crate::db::take_connection();

    let dirs = AppDirs::global();
    let db_path = dirs.database_path();
    let settings_path = dirs.settings_path();

    // Remove old WAL/SHM files to prevent corruption when replacing doremi.db
    std::fs::remove_file(db_path.with_extension("db-wal")).ok();
    std::fs::remove_file(db_path.with_extension("db-shm")).ok();

    let mut db_extracted = false;
    let mut settings_extracted = false;

    for i in 0..archive.len() {
        let mut file = match archive.by_index(i) {
            Ok(f) => f,
            Err(_) => continue,
        };

        let outpath = match file.enclosed_name() {
            Some(path) => path.to_owned(),
            None => continue,
        };

        if outpath.to_str() == Some("doremi.db") {
            let mut outfile = match File::create(&db_path) {
                Ok(f) => f,
                Err(e) => {
                    log::error!("Failed to create doremi.db for restore: {e}");
                    continue;
                }
            };
            if let Err(e) = std::io::copy(&mut file, &mut outfile) {
                log::error!("Failed to extract doremi.db: {e}");
            } else {
                db_extracted = true;
            }
        } else if outpath.to_str() == Some("settings.toml") {
            let mut outfile = match File::create(&settings_path) {
                Ok(f) => f,
                Err(e) => {
                    log::error!("Failed to create settings.toml for restore: {e}");
                    continue;
                }
            };
            if let Err(e) = std::io::copy(&mut file, &mut outfile) {
                log::error!("Failed to extract settings.toml: {e}");
            } else {
                settings_extracted = true;
            }
        }
    }

    // 2. Re-initialize database
    if let Err(e) = crate::db::Database::init() {
        log::error!("Failed to re-initialize database after restore: {e}");
    }

    // 3. Reload settings in memory and apply to UI
    if settings_extracted {
        crate::bridge::apply_settings_impl();
    }

    log::info!(
        "Backup imported successfully. DB: {}, Settings: {}",
        db_extracted,
        settings_extracted
    );

    db_extracted || settings_extracted
}
