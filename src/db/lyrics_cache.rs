use crate::db::with_db;
use rusqlite::params;
use std::time::{Duration, SystemTime, UNIX_EPOCH};

pub struct LyricsCacheEntry {
    pub track_artist: String,
    pub track_title: String,
    pub plain_lyrics: String,
    pub synced_lyrics: String,
    pub source_version: String,
    pub cached_at: String,
    pub expires_at: Option<String>,
}

pub struct LyricsCache;

impl LyricsCache {
    pub fn get(artist: &str, title: &str) -> Option<LyricsCacheEntry> {
        let guard = super::DB.lock().ok()?;
        let conn = guard.as_ref()?;
        let row = conn.query_row(
            "SELECT track_artist, track_title, plain_lyrics, synced_lyrics, source_version, cached_at, expires_at \
             FROM lyrics_cache WHERE track_artist = ?1 AND track_title = ?2",
            params![artist, title],
            |r| Ok((
                r.get::<_, String>(0)?,
                r.get::<_, String>(1)?,
                r.get::<_, String>(2)?,
                r.get::<_, String>(3)?,
                r.get::<_, String>(4)?,
                r.get::<_, String>(5)?,
                r.get::<_, Option<String>>(6)?,
            )),
        );
        match row {
            Ok((track_artist, track_title, plain_lyrics, synced_lyrics, source_version, cached_at, expires_at)) => {
                if let Some(exp) = &expires_at {
                    if is_expired(exp) {
                        let _ = conn.execute(
                            "DELETE FROM lyrics_cache WHERE track_artist = ?1 AND track_title = ?2",
                            params![artist, title],
                        );
                        return None;
                    }
                }
                Some(LyricsCacheEntry {
                    track_artist,
                    track_title,
                    plain_lyrics,
                    synced_lyrics,
                    source_version,
                    cached_at,
                    expires_at,
                })
            }
            Err(_) => None,
        }
    }

    pub fn set(artist: &str, title: &str, plain_lyrics: &str, synced_lyrics: &str, ttl_secs: Option<u64>) -> rusqlite::Result<()> {
        with_db(|conn| {
            let expires_at = ttl_secs.map(|secs| {
                let expiry = SystemTime::now() + Duration::from_secs(secs);
                let since_epoch = expiry.duration_since(UNIX_EPOCH).unwrap_or_default();
                format!("{}", since_epoch.as_secs())
            });
            conn.execute(
                "INSERT OR REPLACE INTO lyrics_cache (track_artist, track_title, plain_lyrics, synced_lyrics, source_version, expires_at) \
                 VALUES (?1, ?2, ?3, ?4, '1', ?5)",
                params![artist, title, plain_lyrics, synced_lyrics, expires_at],
            ).map(|_| ())
        })
    }

    pub fn set_none(artist: &str, title: &str, ttl_secs: Option<u64>) -> rusqlite::Result<()> {
        with_db(|conn| {
            let expires_at = ttl_secs.map(|secs| {
                let expiry = SystemTime::now() + Duration::from_secs(secs);
                let since_epoch = expiry.duration_since(UNIX_EPOCH).unwrap_or_default();
                format!("{}", since_epoch.as_secs())
            });
            conn.execute(
                "INSERT OR REPLACE INTO lyrics_cache (track_artist, track_title, plain_lyrics, synced_lyrics, source_version, expires_at) \
                 VALUES (?1, ?2, '', '', '1', ?3)",
                params![artist, title, expires_at],
            ).map(|_| ())
        })
    }

    pub fn invalidate(artist: &str, title: &str) -> rusqlite::Result<()> {
        with_db(|conn| {
            conn.execute(
                "DELETE FROM lyrics_cache WHERE track_artist = ?1 AND track_title = ?2",
                params![artist, title],
            ).map(|_| ())
        })
    }

    pub fn clear_expired() -> rusqlite::Result<()> {
        with_db(|conn| {
            let now = SystemTime::now()
                .duration_since(UNIX_EPOCH)
                .unwrap_or_default()
                .as_secs();
            conn.execute(
                "DELETE FROM lyrics_cache WHERE expires_at IS NOT NULL AND CAST(expires_at AS INTEGER) < ?1",
                params![now as i64],
            ).map(|_| ())
        })
    }

    pub fn clear_all() -> rusqlite::Result<()> {
        with_db(|conn| conn.execute("DELETE FROM lyrics_cache", []).map(|_| ()))
    }
}

fn is_expired(expires_at: &str) -> bool {
    if let Ok(exp_secs) = expires_at.parse::<u64>() {
        let now = SystemTime::now()
            .duration_since(UNIX_EPOCH)
            .unwrap_or_default()
            .as_secs();
        return now > exp_secs;
    }
    false
}
