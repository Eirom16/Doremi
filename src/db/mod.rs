pub mod cache;
pub mod lyrics_cache;
pub mod repo;

use crate::config::paths::AppDirs;
use rusqlite::{Connection, Result as SqlResult};
use std::sync::Mutex;

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
            "Database not initialized".into(),
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
            "CREATE TABLE IF NOT EXISTS schema_version (version INTEGER PRIMARY KEY);",
        )?;
        let version: i32 = conn
            .query_row(
                "SELECT COALESCE(MAX(version), 0) FROM schema_version",
                [],
                |r| r.get(0),
            )
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
                INSERT INTO schema_version (version) VALUES (1);",
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
                INSERT INTO schema_version (version) VALUES (2);",
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
                INSERT INTO schema_version (version) VALUES (3);",
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
                INSERT INTO schema_version (version) VALUES (4);",
            )?;
        }

        if version < 5 {
            Self::add_column_if_missing(
                conn,
                "playlists",
                "privacy",
                "INTEGER NOT NULL DEFAULT 1",
            )?;
            conn.execute(
                "INSERT OR IGNORE INTO schema_version (version) VALUES (5)",
                [],
            )?;
        }

        if version < 6 {
            Self::add_column_if_missing(
                conn,
                "recently_played",
                "play_count",
                "INTEGER NOT NULL DEFAULT 1",
            )?;
            Self::add_column_if_missing(
                conn,
                "recently_played",
                "progress_ms",
                "INTEGER NOT NULL DEFAULT 0",
            )?;
            Self::add_column_if_missing(
                conn,
                "recently_played",
                "skipped",
                "INTEGER NOT NULL DEFAULT 0",
            )?;
            Self::dedupe_recently_played(conn)?;
            conn.execute_batch(
                "CREATE UNIQUE INDEX IF NOT EXISTS idx_recently_played_track_id ON recently_played(track_id);"
            )?;
            conn.execute(
                "INSERT OR IGNORE INTO schema_version (version) VALUES (6)",
                [],
            )?;
        }

        if version < 7 {
            Self::add_column_if_missing(
                conn,
                "downloads",
                "status",
                "TEXT NOT NULL DEFAULT 'completed'",
            )?;
            Self::add_column_if_missing(
                conn,
                "downloads",
                "progress",
                "REAL NOT NULL DEFAULT 100.0",
            )?;
            Self::add_column_if_missing(conn, "downloads", "error", "TEXT NOT NULL DEFAULT ''")?;
            Self::add_column_if_missing(
                conn,
                "downloads",
                "cancelled",
                "INTEGER NOT NULL DEFAULT 0",
            )?;
            conn.execute(
                "INSERT OR IGNORE INTO schema_version (version) VALUES (7)",
                [],
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
                 INSERT INTO schema_version (version) VALUES (8);",
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
                 INSERT INTO schema_version (version) VALUES (9);",
            )?;
        }

        if version < 10 {
            let has_song_id = Self::column_exists(conn, "downloads", "song_id")?;
            if has_song_id {
                log::info!("Migrating downloads table schema (renaming song_id to video_id and adjusting columns)...");
                conn.execute_batch(
                    "BEGIN TRANSACTION;
                     CREATE TABLE IF NOT EXISTS downloads_new (
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
                        parent_playlist_thumbnail_url TEXT,
                        status TEXT NOT NULL DEFAULT 'completed',
                        progress REAL NOT NULL DEFAULT 100.0,
                        error TEXT NOT NULL DEFAULT '',
                        cancelled INTEGER NOT NULL DEFAULT 0
                     );
                     
                     INSERT INTO downloads_new (
                        video_id, title, artist, album, file_path, thumbnail_url,
                        duration_ms, downloaded_at, status, progress, error, cancelled
                     )
                     SELECT 
                        song_id, title, artist, COALESCE(album, ''), file_path, COALESCE(thumbnail_url, ''),
                        duration_ms, COALESCE(datetime(downloaded_at, 'unixepoch'), datetime('now')), status, progress, COALESCE(error, ''), cancelled
                     FROM downloads;
                     
                     DROP TABLE downloads;
                     ALTER TABLE downloads_new RENAME TO downloads;
                     CREATE INDEX IF NOT EXISTS idx_downloads_status ON downloads(status);
                     COMMIT;"
                )?;
            } else {
                log::info!("Ensuring downloads table columns are correct...");
                Self::add_column_if_missing(conn, "downloads", "parent_playlist_id", "TEXT")?;
                Self::add_column_if_missing(conn, "downloads", "parent_playlist_title", "TEXT")?;
                Self::add_column_if_missing(conn, "downloads", "parent_playlist_thumbnail_url", "TEXT")?;
                Self::add_column_if_missing(conn, "downloads", "status", "TEXT NOT NULL DEFAULT 'completed'")?;
                Self::add_column_if_missing(conn, "downloads", "progress", "REAL NOT NULL DEFAULT 100.0")?;
                Self::add_column_if_missing(conn, "downloads", "error", "TEXT NOT NULL DEFAULT ''")?;
                Self::add_column_if_missing(conn, "downloads", "cancelled", "INTEGER NOT NULL DEFAULT 0")?;
            }
            conn.execute(
                "INSERT OR IGNORE INTO schema_version (version) VALUES (10)",
                [],
            )?;
        }

        if version < 11 {
            conn.execute_batch(
                "CREATE INDEX IF NOT EXISTS idx_recently_played_played_at
                    ON recently_played(played_at DESC);
                 CREATE INDEX IF NOT EXISTS idx_recently_played_play_count
                    ON recently_played(play_count DESC, played_at DESC);
                 CREATE INDEX IF NOT EXISTS idx_recently_played_played_at_artist
                    ON recently_played(played_at DESC, artist);
                 CREATE INDEX IF NOT EXISTS idx_search_history_searched_at
                    ON search_history(searched_at DESC);
                 INSERT OR IGNORE INTO schema_version (version) VALUES (11);",
            )?;
        }

        Ok(())
    }

    fn add_column_if_missing(
        conn: &Connection,
        table: &str,
        column: &str,
        definition: &str,
    ) -> SqlResult<()> {
        if Self::column_exists(conn, table, column)? {
            return Ok(());
        }
        conn.execute(
            &format!("ALTER TABLE {table} ADD COLUMN {column} {definition}"),
            [],
        )?;
        Ok(())
    }

    fn column_exists(conn: &Connection, table: &str, column: &str) -> SqlResult<bool> {
        let mut stmt = conn.prepare(&format!("PRAGMA table_info({table})"))?;
        let mut rows = stmt.query([])?;
        while let Some(row) = rows.next()? {
            let name: String = row.get(1)?;
            if name == column {
                return Ok(true);
            }
        }
        Ok(false)
    }

    fn dedupe_recently_played(conn: &Connection) -> SqlResult<()> {
        conn.execute_batch(
            "UPDATE recently_played
             SET play_count = (
                SELECT SUM(COALESCE(r2.play_count, 1))
                FROM recently_played r2
                WHERE r2.track_id = recently_played.track_id
             ),
             progress_ms = (
                SELECT MAX(COALESCE(r2.progress_ms, 0))
                FROM recently_played r2
                WHERE r2.track_id = recently_played.track_id
             ),
             skipped = (
                SELECT MAX(COALESCE(r2.skipped, 0))
                FROM recently_played r2
                WHERE r2.track_id = recently_played.track_id
             )
             WHERE rowid IN (
                SELECT MAX(rowid)
                FROM recently_played
                GROUP BY track_id
             );

             DELETE FROM recently_played
             WHERE rowid NOT IN (
                SELECT MAX(rowid)
                FROM recently_played
                GROUP BY track_id
             );",
        )?;
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
            .query_row(
                "SELECT COALESCE(MAX(version), 0) FROM schema_version",
                [],
                |r| r.get(0),
            )
            .unwrap();
        assert!(version >= 2, "Expected schema version >= 2, got {version}");
    }

    #[test]
    fn test_db_reinit() {
        let conn = Connection::open_in_memory().unwrap();
        Database::run_migrations(&conn).unwrap();
        Database::run_migrations(&conn).unwrap();
        let version: i32 = conn
            .query_row(
                "SELECT COALESCE(MAX(version), 0) FROM schema_version",
                [],
                |r| r.get(0),
            )
            .unwrap();
        assert_eq!(version, 11);
    }

    #[test]
    fn test_partial_column_migration_recovers() {
        let conn = Connection::open_in_memory().unwrap();
        conn.execute_batch(
            "CREATE TABLE schema_version (version INTEGER PRIMARY KEY);
             INSERT INTO schema_version (version) VALUES (5);
             CREATE TABLE recently_played (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                track_id TEXT NOT NULL,
                title TEXT NOT NULL,
                artist TEXT NOT NULL,
                album TEXT NOT NULL DEFAULT '',
                duration_ms INTEGER NOT NULL DEFAULT 0,
                thumbnail TEXT NOT NULL DEFAULT '',
                played_at TEXT NOT NULL DEFAULT (datetime('now')),
                play_count INTEGER NOT NULL DEFAULT 1
             );
             CREATE TABLE downloads (
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
             CREATE TABLE search_history (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                query TEXT NOT NULL,
                filter TEXT NOT NULL DEFAULT 'all',
                searched_at TEXT NOT NULL DEFAULT (datetime('now'))
             );
             INSERT INTO recently_played
                (track_id, title, artist, play_count)
             VALUES
                ('dup-track', 'Song', 'Artist', 1),
                ('dup-track', 'Song', 'Artist', 3);",
        )
        .unwrap();

        Database::run_migrations(&conn).unwrap();

        assert!(Database::column_exists(&conn, "recently_played", "play_count").unwrap());
        assert!(Database::column_exists(&conn, "recently_played", "progress_ms").unwrap());
        assert!(Database::column_exists(&conn, "recently_played", "skipped").unwrap());
        assert!(Database::column_exists(&conn, "downloads", "status").unwrap());

        let duplicate_count: i32 = conn
            .query_row(
                "SELECT COUNT(*) FROM recently_played WHERE track_id = 'dup-track'",
                [],
                |r| r.get(0),
            )
            .unwrap();
        let merged_play_count: i32 = conn
            .query_row(
                "SELECT play_count FROM recently_played WHERE track_id = 'dup-track'",
                [],
                |r| r.get(0),
            )
            .unwrap();
        assert_eq!(duplicate_count, 1);
        assert_eq!(merged_play_count, 4);

        let version: i32 = conn
            .query_row(
                "SELECT COALESCE(MAX(version), 0) FROM schema_version",
                [],
                |r| r.get(0),
            )
            .unwrap();
        assert_eq!(version, 11);
    }

    #[test]
    fn test_downloads_song_id_migration() {
        let conn = Connection::open_in_memory().unwrap();
        // First run migrations on clean DB to create all tables
        Database::run_migrations(&conn).unwrap();

        // Now drop downloads table and replace it with the legacy song_id-based one
        conn.execute_batch(
            "DROP TABLE downloads;
             CREATE TABLE downloads (
                song_id TEXT PRIMARY KEY,
                title TEXT NOT NULL,
                artist TEXT NOT NULL,
                album TEXT,
                file_path TEXT NOT NULL,
                duration_ms INTEGER NOT NULL,
                status TEXT NOT NULL,
                progress REAL NOT NULL,
                downloaded_at INTEGER NOT NULL,
                thumbnail_url TEXT,
                error TEXT NOT NULL DEFAULT '',
                cancelled INTEGER NOT NULL DEFAULT 0
             );
             INSERT INTO downloads (
                song_id, title, artist, file_path, duration_ms, status, progress, downloaded_at
             ) VALUES (
                'test-id', 'Test Title', 'Test Artist', '/path/to/file.mp3', 180000, 'completed', 100.0, 1780621169
             );
              DELETE FROM schema_version WHERE version >= 10;"
        ).unwrap();

        // Run migrations again - it will run version 10 migration!
        Database::run_migrations(&conn).unwrap();

        assert!(Database::column_exists(&conn, "downloads", "video_id").unwrap());
        assert!(!Database::column_exists(&conn, "downloads", "song_id").unwrap());

        let (title, artist, path, downloaded_at): (String, String, String, String) = conn.query_row(
            "SELECT title, artist, file_path, downloaded_at FROM downloads WHERE video_id = 'test-id'",
            [],
            |r| Ok((r.get(0)?, r.get(1)?, r.get(2)?, r.get(3)?))
        ).unwrap();

        assert_eq!(title, "Test Title");
        assert_eq!(artist, "Test Artist");
        assert_eq!(path, "/path/to/file.mp3");
        assert!(downloaded_at.starts_with("2026-"));
    }
}
