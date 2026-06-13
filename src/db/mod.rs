pub mod repo;
pub mod cache;

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
    let guard = DB.lock().unwrap();
    match guard.as_ref() {
        Some(conn) => f(conn),
        None => Err(rusqlite::Error::InvalidParameterName(
            "Database not initialized".into()
        )),
    }
}

pub fn init_connection(conn: Connection) {
    *DB.lock().unwrap() = Some(conn);
}

pub fn take_connection() -> Option<Connection> {
    DB.lock().unwrap().take()
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

    fn run_migrations(conn: &Connection) -> SqlResult<()> {
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
        assert_eq!(version, 3);
    }
}
