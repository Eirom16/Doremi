use crate::config::paths::AppDirs;
use std::fs::File;
use std::io::{Read, Write};
use std::path::Path;

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
    let options =
        zip::write::FileOptions::default().compression_method(zip::CompressionMethod::Deflated);

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

    // 2. Add sanitized settings. Never copy the raw file into a backup.
    if settings_path.exists() {
        if let Err(e) = zip.start_file("settings.toml", options) {
            log::error!("Failed to add settings.toml to zip: {e}");
            return false;
        }
        let buffer = match sanitized_settings_file(&settings_path) {
            Ok(data) => data.into_bytes(),
            Err(e) => {
                log::error!("Failed to sanitize settings.toml: {e}");
                return false;
            }
        };
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

fn sanitized_settings_file(path: &Path) -> Result<String, Box<dyn std::error::Error>> {
    let raw = std::fs::read_to_string(path)?;
    let settings: crate::config::settings::AppSettings = toml::from_str(&raw)?;
    Ok(settings.sanitized_toml()?)
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
                #[cfg(unix)]
                {
                    use std::os::unix::fs::PermissionsExt;
                    let _ = outfile.set_permissions(std::fs::Permissions::from_mode(0o600));
                }
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

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn backup_settings_exclude_legacy_secrets() {
        let path = std::env::temp_dir().join(format!(
            "doremi-backup-settings-{}.toml",
            std::process::id()
        ));
        let raw = r#"
google_client_secret = "google-secret"

[integrations]
lastfm_api_key = "lastfm-key"
lastfm_api_secret = "lastfm-secret"
lastfm_session_key = "lastfm-session"
"#;
        std::fs::write(&path, raw).unwrap();
        let sanitized = sanitized_settings_file(&path).unwrap();
        let _ = std::fs::remove_file(path);

        assert!(!sanitized.contains("google-secret"));
        assert!(!sanitized.contains("lastfm-key"));
        assert!(!sanitized.contains("lastfm-secret"));
        assert!(!sanitized.contains("lastfm-session"));
    }
}
