pub mod repo;
pub mod cache;
pub mod lyrics_cache;

use std::sync::Mutex;
use rusqlite::{Connection, Result as SqlResult};
use crate::config::paths::AppDirs;

static DB: Mutex<Option<Connection>> = Mutex::new(None);

pub fn global() -> &'static Mutex<Option<Connection>> {
    &DB
}

pub fn with_db<F, R>(f: F) -> SqlResult<R>
where
    F: FnOnce(&Connection) -> SqlResult<R>,
{
    let guard = match DB.lock() {
        Ok(g) => g,
        Err(e) => e.into_inner(),
    };
    match guard.as_ref() {
        Some(conn) => f(conn),
        None => Err(rusqlite::Error::InvalidParameterName(
            "Database not initialized".into()
        )),
    }
}

pub fn init_connection(conn: Connection) {
    let mut guard = match DB.lock() {
        Ok(g) => g,
        Err(e) => e.into_inner(),
    };
    *guard = Some(conn);
}

pub fn take_connection() -> Option<Connection> {
    let mut guard = match DB.lock() {
        Ok(g) => g,
        Err(e) => e.into_inner(),
    };
    guard.take()
}

pub struct Database;

impl Database {
    pub fn init() -> SqlResult<()> {
        let dirs = AppDirs::global();
        let path = dirs.database_path();
        if let Some(parent) = path.parent() {
            std::fs::create_dir_all(parent).ok();
        }
        let conn = Connection::open(&path)?;
        conn.execute_batch("PRAGMA journal_mode=WAL; PRAGMA foreign_keys=ON;")?;
        Self::run_migrations(&conn)?;
        init_connection(conn);
        Ok(())
    }

    pub(crate) fn run_migrations(conn: &Connection) -> SqlResult<()> {
        conn.execute_batch(
            "CREATE TABLE IF NOT EXISTS schema_version (version INTEGER PRIMARY KEY);"
        )?;
        let version: i32 = conn
            .query_row("SELECT COALESCE(MAX(version), 0) FROM schema_version", [], |r| r.get(0))
            .unwrap_or(0);

        if version < 1 {
            conn.execute_batch(
                "CREATE TABLE IF NOT EXISTS favorite_tracks (
                    id TEXT PRIMARY KEY,
                    title TEXT NOT NULL,
                    artist TEXT NOT NULL,
                    album TEXT NOT NULL DEFAULT '',
                    album_id TEXT NOT NULL DEFAULT '',
                    duration_ms INTEGER NOT NULL DEFAULT 0,
                    thumbnail TEXT NOT NULL DEFAULT '',
                    added_at TEXT NOT NULL DEFAULT (datetime('now'))
                );
                CREATE TABLE IF NOT EXISTS favorite_albums (
                    id TEXT PRIMARY KEY,
                    title TEXT NOT NULL,
                    artist TEXT NOT NULL,
                    year INTEGER,
                    thumbnail TEXT NOT NULL DEFAULT '',
                    added_at TEXT NOT NULL DEFAULT (datetime('now'))
                );
                CREATE TABLE IF NOT EXISTS favorite_artists (
                    id TEXT PRIMARY KEY,
                    name TEXT NOT NULL,
                    thumbnail TEXT NOT NULL DEFAULT '',
                    added_at TEXT NOT NULL DEFAULT (datetime('now'))
                );
                CREATE TABLE IF NOT EXISTS playlists (
                    id TEXT PRIMARY KEY,
                    name TEXT NOT NULL,
                    description TEXT NOT NULL DEFAULT '',
                    artwork TEXT NOT NULL DEFAULT '',
                    created_at TEXT NOT NULL DEFAULT (datetime('now')),
                    updated_at TEXT NOT NULL DEFAULT (datetime('now'))
                );
                CREATE TABLE IF NOT EXISTS playlist_tracks (
                    playlist_id TEXT NOT NULL,
                    track_id TEXT NOT NULL,
                    position INTEGER NOT NULL,
                    title TEXT NOT NULL,
                    artist TEXT NOT NULL,
                    album TEXT NOT NULL DEFAULT '',
                    duration_ms INTEGER NOT NULL DEFAULT 0,
                    thumbnail TEXT NOT NULL DEFAULT '',
                    added_at TEXT NOT NULL DEFAULT (datetime('now')),
                    PRIMARY KEY (playlist_id, track_id),
                    FOREIGN KEY (playlist_id) REFERENCES playlists(id) ON DELETE CASCADE
                );
                CREATE TABLE IF NOT EXISTS recently_played (
                    id INTEGER PRIMARY KEY AUTOINCREMENT,
                    track_id TEXT NOT NULL,
                    title TEXT NOT NULL,
                    artist TEXT NOT NULL,
                    album TEXT NOT NULL DEFAULT '',
                    duration_ms INTEGER NOT NULL DEFAULT 0,
                    thumbnail TEXT NOT NULL DEFAULT '',
                    played_at TEXT NOT NULL DEFAULT (datetime('now'))
                );
                CREATE TABLE IF NOT EXISTS search_history (
                    id INTEGER PRIMARY KEY AUTOINCREMENT,
                    query TEXT NOT NULL,
                    filter TEXT NOT NULL DEFAULT 'all',
                    searched_at TEXT NOT NULL DEFAULT (datetime('now'))
                );
                INSERT INTO schema_version (version) VALUES (1);"
            )?;
        }

        if version < 2 {
            conn.execute_batch(
                "CREATE TABLE IF NOT EXISTS response_cache (
                    cache_key TEXT PRIMARY KEY,
                    response TEXT NOT NULL,
                    content_type TEXT NOT NULL DEFAULT 'json',
                    cached_at TEXT NOT NULL DEFAULT (datetime('now')),
                    expires_at TEXT
                );
                INSERT INTO schema_version (version) VALUES (2);"
            )?;
        }

        if version < 3 {
            conn.execute_batch(
                "CREATE TABLE IF NOT EXISTS downloads (
                    video_id TEXT PRIMARY KEY,
                    title TEXT NOT NULL,
                    artist TEXT NOT NULL,
                    album TEXT NOT NULL DEFAULT '',
                    file_path TEXT NOT NULL,
                    thumbnail_url TEXT NOT NULL DEFAULT '',
                    duration_ms INTEGER NOT NULL DEFAULT 0,
                    downloaded_at TEXT NOT NULL DEFAULT (datetime('now')),
                    parent_playlist_id TEXT,
                    parent_playlist_title TEXT,
                    parent_playlist_thumbnail_url TEXT
                );
                INSERT INTO schema_version (version) VALUES (3);"
            )?;
        }

        if version < 4 {
            conn.execute_batch(
                "CREATE TABLE IF NOT EXISTS lyrics_cache (
                    id INTEGER PRIMARY KEY AUTOINCREMENT,
                    track_artist TEXT NOT NULL,
                    track_title TEXT NOT NULL,
                    plain_lyrics TEXT NOT NULL DEFAULT '',
                    synced_lyrics TEXT NOT NULL DEFAULT '',
                    source_version TEXT NOT NULL DEFAULT '1',
                    cached_at TEXT NOT NULL DEFAULT (datetime('now')),
                    expires_at TEXT,
                    UNIQUE(track_artist, track_title)
                );
                INSERT INTO schema_version (version) VALUES (4);"
            )?;
        }

        if version < 5 {
            conn.execute_batch(
                "ALTER TABLE playlists ADD COLUMN privacy INTEGER NOT NULL DEFAULT 1;
                 INSERT INTO schema_version (version) VALUES (5);"
            )?;
        }

        if version < 6 {
            conn.execute_batch(
                "ALTER TABLE recently_played ADD COLUMN play_count INTEGER NOT NULL DEFAULT 1;
                 ALTER TABLE recently_played ADD COLUMN progress_ms INTEGER NOT NULL DEFAULT 0;
                 ALTER TABLE recently_played ADD COLUMN skipped INTEGER NOT NULL DEFAULT 0;
                 CREATE UNIQUE INDEX IF NOT EXISTS idx_recently_played_track_id ON recently_played(track_id);
                 INSERT INTO schema_version (version) VALUES (6);"
            )?;
        }

        if version < 7 {
            conn.execute_batch(
                "ALTER TABLE downloads ADD COLUMN status TEXT NOT NULL DEFAULT 'completed';
                 ALTER TABLE downloads ADD COLUMN progress REAL NOT NULL DEFAULT 100.0;
                 ALTER TABLE downloads ADD COLUMN error TEXT NOT NULL DEFAULT '';
                 ALTER TABLE downloads ADD COLUMN cancelled INTEGER NOT NULL DEFAULT 0;
                 INSERT INTO schema_version (version) VALUES (7);"
            )?;
        }

        if version < 8 {
            conn.execute_batch(
                "CREATE TABLE IF NOT EXISTS favorite_shows (
                    id TEXT PRIMARY KEY,
                    title TEXT NOT NULL,
                    author TEXT NOT NULL DEFAULT '',
                    description TEXT NOT NULL DEFAULT '',
                    thumbnail TEXT NOT NULL DEFAULT '',
                    episode_count INTEGER NOT NULL DEFAULT 0,
                    added_at TEXT NOT NULL DEFAULT (datetime('now'))
                 );
                 INSERT INTO schema_version (version) VALUES (8);"
            )?;
        }

        if version < 9 {
            conn.execute_batch(
                "CREATE TABLE IF NOT EXISTS cached_shows (
                    id TEXT PRIMARY KEY,
                    title TEXT NOT NULL,
                    author TEXT NOT NULL DEFAULT '',
                    description TEXT NOT NULL DEFAULT '',
                    thumbnail TEXT NOT NULL DEFAULT '',
                    episode_count INTEGER NOT NULL DEFAULT 0,
                    added_at TEXT NOT NULL DEFAULT (datetime('now'))
                 );
                 CREATE TABLE IF NOT EXISTS cached_show_episodes (
                    id TEXT PRIMARY KEY,
                    show_id TEXT NOT NULL,
                    title TEXT NOT NULL,
                    show_title TEXT NOT NULL DEFAULT '',
                    description TEXT NOT NULL DEFAULT '',
                    thumbnail TEXT NOT NULL DEFAULT '',
                    duration_ms INTEGER NOT NULL DEFAULT 0,
                    published_at TEXT NOT NULL DEFAULT '',
                    position INTEGER,
                    added_at TEXT NOT NULL DEFAULT (datetime('now')),
                    FOREIGN KEY (show_id) REFERENCES cached_shows(id) ON DELETE CASCADE
                 );
                 INSERT INTO schema_version (version) VALUES (9);"
            )?;
        }

        Ok(())
    }

    pub fn reset() -> SqlResult<()> {
        let path = AppDirs::global().database_path();
        take_connection();
        std::fs::remove_file(&path).ok();
        std::fs::remove_file(path.with_extension("db-wal")).ok();
        std::fs::remove_file(path.with_extension("db-shm")).ok();
        Self::init()
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_db_init_and_migration() {
        let conn = Connection::open_in_memory().unwrap();
        Database::run_migrations(&conn).unwrap();
        let version: i32 = conn
            .query_row("SELECT COALESCE(MAX(version), 0) FROM schema_version", [], |r| r.get(0))
            .unwrap();
        assert!(version >= 2, "Expected schema version >= 2, got {version}");
    }

    #[test]
    fn test_db_reinit() {
        let conn = Connection::open_in_memory().unwrap();
        Database::run_migrations(&conn).unwrap();
        Database::run_migrations(&conn).unwrap();
        let version: i32 = conn
            .query_row("SELECT COALESCE(MAX(version), 0) FROM schema_version", [], |r| r.get(0))
            .unwrap();
        assert_eq!(version, 9);
    }
}
