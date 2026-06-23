use crate::config::paths::AppDirs;
use crate::config::settings::AppSettings;
use crate::utils::backup::export_backup;
use crate::utils::secure_storage::{self, LastFmCredentials};
use std::fs;
use std::path::{Path, PathBuf};
use std::process::Command;

#[derive(Debug, Clone, Default)]
pub struct MigrationSummary {
    pub tracks_migrated: usize,
    pub recently_played_migrated: usize,
    pub downloads_migrated: usize,
    pub secrets_migrated: usize,
    pub files_moved: usize,
    pub errors: Vec<String>,
}

fn lookup_legacy(account: &str) -> Option<String> {
    let mut command = Command::new("secret-tool");
    let output = command
        .args(["lookup", "service", "pyrolist", "account", account])
        .output()
        .ok()?;

    if output.status.success() {
        let value = String::from_utf8(output.stdout).ok()?;
        let value = value.trim_end_matches(['\r', '\n']).to_string();
        if !value.is_empty() {
            return Some(value);
        }
    }
    None
}

fn clear_legacy(account: &str) -> bool {
    let mut command = Command::new("secret-tool");
    let output = command
        .args(["clear", "service", "pyrolist", "account", account])
        .output();
    output.map(|o| o.status.success()).unwrap_or(false)
}

pub fn run_migration() -> Result<Option<MigrationSummary>, String> {
    let home = std::env::var("HOME").unwrap_or_else(|_| {
        directories::BaseDirs::new()
            .map(|d| d.home_dir().to_string_lossy().to_string())
            .unwrap_or_else(|| "/tmp".to_string())
    });

    let xdg_config = std::env::var("XDG_CONFIG_HOME")
        .map(PathBuf::from)
        .unwrap_or_else(|_| PathBuf::from(&home).join(".config"));
    let xdg_data = std::env::var("XDG_DATA_HOME")
        .map(PathBuf::from)
        .unwrap_or_else(|_| PathBuf::from(&home).join(".local/share"));
    let xdg_cache = std::env::var("XDG_CACHE_HOME")
        .map(PathBuf::from)
        .unwrap_or_else(|_| PathBuf::from(&home).join(".cache"));

    let legacy_config = xdg_config.join("pyrolist");
    let legacy_data = xdg_data.join("pyrolist");
    let legacy_cache = xdg_cache.join("pyrolist");

    // Only run if legacy database exists or legacy settings exist
    let legacy_db_path = legacy_data.join("pyrolist.db");
    let legacy_settings_path = legacy_config.join("settings.toml");

    if !legacy_db_path.exists() && !legacy_settings_path.exists() {
        return Ok(None);
    }

    let dirs = AppDirs::global();
    // Create a backup of Doremi's current database and settings before migrating
    let backup_path = dirs.data_dir().join("pre_migration_backup.zip");
    log::info!("Creating backup before migration at {:?}", backup_path);
    export_backup(&backup_path);

    let summary = run_migration_with_paths(
        &legacy_config,
        &legacy_data,
        &legacy_cache,
        &dirs.config_dir(),
        &dirs.data_dir(),
        &dirs.cache_dir(),
    )?;
    Ok(Some(summary))
}

pub fn run_migration_with_paths(
    legacy_config_dir: &Path,
    legacy_data_dir: &Path,
    legacy_cache_dir: &Path,
    doremi_config_dir: &Path,
    doremi_data_dir: &Path,
    doremi_cache_dir: &Path,
) -> Result<MigrationSummary, String> {
    let mut summary = MigrationSummary::default();

    // 1. Settings Migration
    let legacy_settings_path = legacy_config_dir.join("settings.toml");
    let doremi_settings_path = doremi_config_dir.join("settings.toml");
    if legacy_settings_path.exists() && !doremi_settings_path.exists() {
        log::info!("Migrating settings from Pyrolist...");
        if let Ok(raw) = fs::read_to_string(&legacy_settings_path) {
            if let Ok(settings) = toml::from_str::<AppSettings>(&raw) {
                if let Err(e) = settings.save(&doremi_settings_path) {
                    summary.errors.push(format!("Failed to save settings: {e}"));
                }
            } else {
                summary
                    .errors
                    .push("Failed to parse legacy settings.toml".to_string());
            }
        }
    }

    // 2. Database Migration
    let legacy_db_path = legacy_data_dir.join("pyrolist.db");
    if legacy_db_path.exists() {
        log::info!("Migrating database records from Pyrolist...");
        let db_res = crate::db::with_db(|conn| {
            let attach_query = format!(
                "ATTACH DATABASE '{}' AS legacy",
                legacy_db_path.to_string_lossy().replace('\'', "''")
            );
            conn.execute(&attach_query, [])?;

            // Begin transaction
            conn.execute("BEGIN TRANSACTION", [])?;

            // Migrate favorite tracks
            let tracks_res = conn.execute(
                "INSERT OR IGNORE INTO favorite_tracks (id, title, artist, album, album_id, duration_ms, thumbnail, added_at)
                 SELECT video_id, title, artist, COALESCE(album, ''), '', COALESCE(duration_ms, 0), COALESCE(thumbnail_url, ''), datetime('now')
                 FROM legacy.songs
                 WHERE is_liked = 1",
                [],
            );
            match tracks_res {
                Ok(count) => summary.tracks_migrated = count,
                Err(e) => {
                    conn.execute("ROLLBACK", []).ok();
                    return Err(rusqlite::Error::InvalidParameterName(format!(
                        "Tracks migration failed: {e}"
                    )));
                }
            }

            // Migrate recently played
            let recent_res = conn.execute(
                "INSERT OR IGNORE INTO recently_played (track_id, title, artist, album, duration_ms, thumbnail, played_at)
                 SELECT video_id, title, artist, '', COALESCE(duration_ms, 0), '', datetime(played_at)
                 FROM legacy.play_history",
                [],
            );
            match recent_res {
                Ok(count) => summary.recently_played_migrated = count,
                Err(e) => {
                    conn.execute("ROLLBACK", []).ok();
                    return Err(rusqlite::Error::InvalidParameterName(format!(
                        "History migration failed: {e}"
                    )));
                }
            }

            // Migrate downloads
            let downloads_res = conn.execute(
                "INSERT OR IGNORE INTO downloads (video_id, title, artist, album, file_path, thumbnail_url, duration_ms, downloaded_at, parent_playlist_id, parent_playlist_title, parent_playlist_thumbnail_url)
                 SELECT video_id, title, artist, COALESCE(album, ''), file_path, COALESCE(thumbnail_url, ''), COALESCE(duration_ms, 0), datetime(downloaded_at), parent_playlist_id, parent_playlist_title, parent_playlist_thumbnail_url
                 FROM legacy.downloads",
                [],
            );
            match downloads_res {
                Ok(count) => {
                    summary.downloads_migrated = count;
                    // Update downloads path from pyrolist to Doremi
                    conn.execute(
                        "UPDATE downloads SET file_path = REPLACE(file_path, '/pyrolist/downloads/', '/Doremi/downloads/')",
                        [],
                    ).ok();
                }
                Err(e) => {
                    conn.execute("ROLLBACK", []).ok();
                    return Err(rusqlite::Error::InvalidParameterName(format!(
                        "Downloads migration failed: {e}"
                    )));
                }
            }

            conn.execute("COMMIT", [])?;
            conn.execute("DETACH DATABASE legacy", [])?;
            Ok(())
        });

        if let Err(e) = db_res {
            summary.errors.push(e.to_string());
        } else {
            // Rename database to mark as migrated
            let migrated_db_path = legacy_data_dir.join("pyrolist.db.migrated");
            if let Err(e) = fs::rename(&legacy_db_path, &migrated_db_path) {
                summary
                    .errors
                    .push(format!("Failed to rename legacy database: {e}"));
            }
        }
    }

    // 3. Cache & Downloads File Migration
    let legacy_downloads = legacy_data_dir.join("downloads");
    let new_downloads = doremi_data_dir.join("downloads");
    if legacy_downloads.exists() {
        log::info!("Migrating downloads files...");
        if !new_downloads.exists() {
            if fs::rename(&legacy_downloads, &new_downloads).is_ok() {
                summary.files_moved += 1;
            } else {
                let _ = fs::create_dir_all(&new_downloads);
                if let Ok(entries) = fs::read_dir(&legacy_downloads) {
                    for entry in entries.flatten() {
                        let path = entry.path();
                        let dest = new_downloads.join(path.file_name().unwrap());
                        if fs::rename(&path, &dest).is_ok()
                            || fs::copy(&path, &dest).map(|_| ()).is_ok()
                        {
                            summary.files_moved += 1;
                        }
                    }
                }
            }
        } else if let Ok(entries) = fs::read_dir(&legacy_downloads) {
            for entry in entries.flatten() {
                let path = entry.path();
                let dest = new_downloads.join(path.file_name().unwrap());
                if !dest.exists() {
                    if fs::rename(&path, &dest).is_ok()
                        || fs::copy(&path, &dest).map(|_| ()).is_ok()
                    {
                        summary.files_moved += 1;
                    }
                }
            }
        }
    }

    // Migrate cache artwork
    let legacy_artwork = legacy_cache_dir.join("artwork");
    let new_artwork = doremi_cache_dir.join("artwork");
    if legacy_artwork.exists() {
        let _ = fs::create_dir_all(&new_artwork);
        if let Ok(entries) = fs::read_dir(&legacy_artwork) {
            for entry in entries.flatten() {
                let path = entry.path();
                let dest = new_artwork.join(path.file_name().unwrap());
                if !dest.exists() {
                    let _ =
                        fs::rename(&path, &dest).or_else(|_| fs::copy(&path, &dest).map(|_| ()));
                }
            }
        }
    }

    // Migrate cache lyrics
    let legacy_lyrics = legacy_cache_dir.join("lyrics");
    let new_lyrics = doremi_cache_dir.join("lyrics");
    if legacy_lyrics.exists() {
        let _ = fs::create_dir_all(&new_lyrics);
        if let Ok(entries) = fs::read_dir(&legacy_lyrics) {
            for entry in entries.flatten() {
                let path = entry.path();
                let dest = new_lyrics.join(path.file_name().unwrap());
                if !dest.exists() {
                    let _ =
                        fs::rename(&path, &dest).or_else(|_| fs::copy(&path, &dest).map(|_| ()));
                }
            }
        }
    }

    // 4. Secrets Migration from Keyring
    log::info!("Migrating credentials from Keyring...");
    if let Some(yt_headers) = lookup_legacy("youtube_music_auth") {
        if secure_storage::save_youtube_headers(&yt_headers).is_ok() {
            clear_legacy("youtube_music_auth");
            summary.secrets_migrated += 1;
        }
    }

    let fm_key = lookup_legacy("lastfm_api_key");
    let fm_secret = lookup_legacy("lastfm_api_secret");
    let fm_session = lookup_legacy("lastfm_session_key");
    if fm_key.is_some() || fm_secret.is_some() || fm_session.is_some() {
        let credentials = LastFmCredentials {
            api_key: fm_key.clone().unwrap_or_default(),
            api_secret: fm_secret.clone().unwrap_or_default(),
            session_key: fm_session.clone().unwrap_or_default(),
        };
        if secure_storage::save_lastfm_credentials(&credentials).is_ok() {
            if fm_key.is_some() {
                clear_legacy("lastfm_api_key");
            }
            if fm_secret.is_some() {
                clear_legacy("lastfm_api_secret");
            }
            if fm_session.is_some() {
                clear_legacy("lastfm_session_key");
            }
            summary.secrets_migrated += 1;
        }
    }

    Ok(summary)
}

#[cfg(test)]
mod tests {
    use super::*;
    use rusqlite::{params, Connection};

    fn setup_legacy_db(path: &Path) {
        let conn = Connection::open(path).unwrap();
        conn.execute_batch(
            "CREATE TABLE songs (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                video_id TEXT UNIQUE NOT NULL,
                title TEXT NOT NULL,
                artist TEXT NOT NULL,
                album TEXT,
                duration_ms INTEGER,
                thumbnail_url TEXT,
                is_liked INTEGER DEFAULT 0,
                last_played TEXT,
                play_count INTEGER DEFAULT 0
            );
            CREATE TABLE play_history (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                video_id TEXT NOT NULL,
                title TEXT NOT NULL,
                artist TEXT NOT NULL,
                played_at TEXT,
                duration_ms INTEGER
            );
            CREATE TABLE downloads (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                video_id TEXT UNIQUE NOT NULL,
                title TEXT NOT NULL,
                artist TEXT NOT NULL,
                album TEXT,
                file_path TEXT NOT NULL,
                thumbnail_url TEXT,
                duration_ms INTEGER,
                downloaded_at TEXT,
                parent_playlist_id TEXT,
                parent_playlist_title TEXT,
                parent_playlist_thumbnail_url TEXT
            );",
        )
        .unwrap();

        conn.execute(
            "INSERT INTO songs (video_id, title, artist, album, duration_ms, thumbnail_url, is_liked)
             VALUES (?1, ?2, ?3, ?4, ?5, ?6, ?7)",
            params!["track-1", "Song A", "Artist X", "Album 1", 180000, "thumb-1", 1],
        )
        .unwrap();

        conn.execute(
            "INSERT INTO play_history (video_id, title, artist, played_at, duration_ms)
             VALUES (?1, ?2, ?3, ?4, ?5)",
            params![
                "track-1",
                "Song A",
                "Artist X",
                "2026-06-12 12:00:00",
                180000
            ],
        )
        .unwrap();

        conn.execute(
            "INSERT INTO downloads (video_id, title, artist, album, file_path, thumbnail_url, duration_ms, downloaded_at)
             VALUES (?1, ?2, ?3, ?4, ?5, ?6, ?7, ?8)",
            params![
                "track-1",
                "Song A",
                "Artist X",
                "Album 1",
                "/home/user/.local/share/pyrolist/downloads/track-1.mp3",
                "thumb-1",
                180000,
                "2026-06-12 12:00:00"
            ],
        )
        .unwrap();
    }

    #[test]
    fn test_legacy_migration() {
        let _guard = crate::db::TEST_MUTEX.lock().unwrap();
        let temp_dir = std::env::temp_dir().join(format!(
            "doremi-migration-test-{}-{}",
            std::process::id(),
            std::thread::current().name().unwrap_or("main")
        ));
        let legacy_config = temp_dir.join("config_legacy");
        let legacy_data = temp_dir.join("data_legacy");
        let legacy_cache = temp_dir.join("cache_legacy");

        fs::create_dir_all(&legacy_config).unwrap();
        fs::create_dir_all(&legacy_data).unwrap();
        fs::create_dir_all(&legacy_cache).unwrap();

        // 1. Setup settings
        let settings_raw = r#"
google_client_id = "test-client-id"
language = "es"
[appearance]
theme_mode = "light"
font_size = 14
"#;
        fs::write(legacy_config.join("settings.toml"), settings_raw).unwrap();

        // 2. Setup database
        setup_legacy_db(&legacy_data.join("pyrolist.db"));

        // 3. Create dummy downloads and cache folders
        let legacy_downloads = legacy_data.join("downloads");
        fs::create_dir_all(&legacy_downloads).unwrap();
        fs::write(legacy_downloads.join("track-1.mp3"), "audio content").unwrap();

        let legacy_artwork = legacy_cache.join("artwork");
        fs::create_dir_all(&legacy_artwork).unwrap();
        fs::write(legacy_artwork.join("art-1.png"), "artwork content").unwrap();

        // 4. Run database initialization in memory for isolation
        let _ = AppDirs::setup(); // Ensure directories initialized
        let conn = rusqlite::Connection::open_in_memory().unwrap();
        crate::db::Database::run_migrations(&conn).unwrap();
        crate::db::init_connection(conn);

        // Create target Doremi directories for test
        let doremi_config = temp_dir.join("config_doremi");
        let doremi_data = temp_dir.join("data_doremi");
        let doremi_cache = temp_dir.join("cache_doremi");
        fs::create_dir_all(&doremi_config).unwrap();
        fs::create_dir_all(&doremi_data).unwrap();
        fs::create_dir_all(&doremi_cache).unwrap();

        // 5. Run migration
        let summary = run_migration_with_paths(
            &legacy_config,
            &legacy_data,
            &legacy_cache,
            &doremi_config,
            &doremi_data,
            &doremi_cache,
        )
        .unwrap();

        assert_eq!(summary.tracks_migrated, 1);
        assert_eq!(summary.recently_played_migrated, 1);
        assert_eq!(summary.downloads_migrated, 1);
        assert_eq!(summary.files_moved, 1); // Only downloads files_moved count

        // 6. Verify migrated settings
        let doremi_settings_path = doremi_config.join("settings.toml");
        assert!(doremi_settings_path.exists());
        let doremi_settings = AppSettings::load(&doremi_settings_path);
        assert_eq!(doremi_settings.language, "es");
        assert_eq!(doremi_settings.appearance.theme_mode, "light");
        assert_eq!(doremi_settings.appearance.font_size, 14);

        // 7. Verify migrated database contents
        crate::db::with_db(|conn| {
            let count_tracks: i32 = conn
                .query_row(
                    "SELECT COUNT(*) FROM favorite_tracks WHERE id = 'track-1'",
                    [],
                    |r| r.get(0),
                )
                .unwrap();
            assert_eq!(count_tracks, 1);

            let count_history: i32 = conn
                .query_row(
                    "SELECT COUNT(*) FROM recently_played WHERE track_id = 'track-1'",
                    [],
                    |r| r.get(0),
                )
                .unwrap();
            assert_eq!(count_history, 1);

            let file_path: String = conn
                .query_row(
                    "SELECT file_path FROM downloads WHERE video_id = 'track-1'",
                    [],
                    |r| r.get(0),
                )
                .unwrap();
            assert!(file_path.contains("/Doremi/downloads/"));
            Ok(())
        })
        .unwrap();

        let _ = crate::db::take_connection();

        // Clean up temp dir
        let _ = fs::remove_dir_all(temp_dir);
    }
}
