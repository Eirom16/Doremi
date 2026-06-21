use crate::db::with_db;
use rusqlite::params;
use serde::Serialize;
use std::time::{Duration, SystemTime, UNIX_EPOCH};

pub struct CacheEntry<T> {
    pub data: T,
    pub cached_at: String,
    pub expires_at: Option<String>,
}

pub struct ResponseCache;

impl ResponseCache {
    pub fn get<T: serde::de::DeserializeOwned>(key: &str) -> Option<CacheEntry<T>> {
        let guard = super::DB.lock().ok()?;
        let conn = guard.as_ref()?;
        let row: Result<(String, String, Option<String>), _> = conn.query_row(
            "SELECT response, cached_at, expires_at FROM response_cache WHERE cache_key = ?1",
            params![key],
            |r| Ok((r.get(0)?, r.get(1)?, r.get(2)?)),
        );
        match row {
            Ok((response, cached_at, expires_at)) => {
                if let Some(exp) = &expires_at {
                    if is_expired(exp) {
                        let _ = conn.execute(
                            "DELETE FROM response_cache WHERE cache_key = ?1",
                            params![key],
                        );
                        return None;
                    }
                }
                serde_json::from_str(&response).ok().map(|data| CacheEntry {
                    data,
                    cached_at,
                    expires_at,
                })
            }
            Err(_) => None,
        }
    }

    pub fn get_stale<T: serde::de::DeserializeOwned>(key: &str) -> Option<CacheEntry<T>> {
        let guard = super::DB.lock().ok()?;
        let conn = guard.as_ref()?;
        let row: Result<(String, String, Option<String>), _> = conn.query_row(
            "SELECT response, cached_at, expires_at FROM response_cache WHERE cache_key = ?1",
            params![key],
            |r| Ok((r.get(0)?, r.get(1)?, r.get(2)?)),
        );
        match row {
            Ok((response, cached_at, expires_at)) => {
                serde_json::from_str(&response).ok().map(|data| CacheEntry {
                    data,
                    cached_at,
                    expires_at,
                })
            }
            Err(_) => None,
        }
    }

    pub fn set<T: Serialize>(key: &str, data: &T, ttl_secs: Option<u64>) -> rusqlite::Result<()> {
        with_db(|conn| {
            let json = serde_json::to_string(data).unwrap_or_default();
            let expires_at = ttl_secs.map(|secs| {
                let expiry = SystemTime::now() + Duration::from_secs(secs);
                let since_epoch = expiry.duration_since(UNIX_EPOCH).unwrap_or_default();
                format!("{}", since_epoch.as_secs())
            });
            conn.execute(
                "INSERT OR REPLACE INTO response_cache (cache_key, response, expires_at) VALUES (?1, ?2, ?3)",
                params![key, json, expires_at],
            ).map(|_| ())
        })
    }

    pub fn invalidate(key: &str) -> rusqlite::Result<()> {
        with_db(|conn| {
            conn.execute(
                "DELETE FROM response_cache WHERE cache_key = ?1",
                params![key],
            )
            .map(|_| ())
        })
    }

    pub fn invalidate_prefix(prefix: &str) -> rusqlite::Result<()> {
        with_db(|conn| {
            let pattern = format!("{}%", prefix.replace('%', "\\%").replace('_', "\\_"));
            conn.execute(
                "DELETE FROM response_cache WHERE cache_key LIKE ?1 ESCAPE '\\'",
                params![pattern],
            )
            .map(|_| ())
        })
    }

    pub fn clear_all() -> rusqlite::Result<()> {
        with_db(|conn| conn.execute("DELETE FROM response_cache", []).map(|_| ()))
    }

    pub fn clear_expired() -> rusqlite::Result<()> {
        with_db(|conn| {
            let now = SystemTime::now()
                .duration_since(UNIX_EPOCH)
                .unwrap_or_default()
                .as_secs();
            conn.execute("DELETE FROM response_cache WHERE expires_at IS NOT NULL AND CAST(expires_at AS INTEGER) < ?1", params![now as i64])
                .map(|_| ())
        })
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

#[cfg(test)]
mod tests {
    use super::*;
    use crate::db::init_connection;
    use crate::db::take_connection;
    use crate::db::Database;

    fn setup_test_db() {
        take_connection();
        let conn = rusqlite::Connection::open_in_memory().unwrap();
        Database::run_migrations(&conn).unwrap();
        init_connection(conn);
    }

    #[test]
    fn test_cache_set_get() {
        setup_test_db();
        let data = vec!["a", "b", "c"];
        ResponseCache::set("test_key", &data, None).unwrap();
        let entry: Option<CacheEntry<Vec<String>>> = ResponseCache::get("test_key");
        assert!(entry.is_some());
        assert_eq!(entry.unwrap().data, vec!["a", "b", "c"]);
    }

    #[test]
    fn test_cache_invalidate() {
        setup_test_db();
        ResponseCache::set("k1", &"hello", None).unwrap();
        assert!(ResponseCache::get::<String>("k1").is_some());
        ResponseCache::invalidate("k1").unwrap();
        assert!(ResponseCache::get::<String>("k1").is_none());
    }

    #[test]
    fn test_cache_invalidate_prefix() {
        setup_test_db();
        ResponseCache::set("ytm:user:home", &"home", None).unwrap();
        ResponseCache::set("lyrics:song", &"lyrics", None).unwrap();
        ResponseCache::invalidate_prefix("ytm:").unwrap();
        assert!(ResponseCache::get::<String>("ytm:user:home").is_none());
        assert!(ResponseCache::get::<String>("lyrics:song").is_some());
    }

    #[test]
    fn test_cache_miss_returns_none() {
        setup_test_db();
        let entry: Option<CacheEntry<String>> = ResponseCache::get("nonexistent");
        assert!(entry.is_none());
    }

    #[test]
    fn test_stale_cache_ignores_expiry() {
        setup_test_db();
        ResponseCache::set("expired", &"cached value", Some(0)).unwrap();
        std::thread::sleep(std::time::Duration::from_secs(1));
        let stale: Option<CacheEntry<String>> = ResponseCache::get_stale("expired");
        assert_eq!(stale.unwrap().data, "cached value");
    }
}
