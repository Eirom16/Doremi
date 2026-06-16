use rusqlite::{params, Result as SqlResult};
use serde::{Deserialize, Serialize};

fn with_db<F, R>(f: F) -> SqlResult<R>
where
    F: FnOnce(&rusqlite::Connection) -> SqlResult<R>,
{
    crate::db::with_db(f)
}

// ── Data types ──

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct FavoriteTrack {
    pub id: String,
    pub title: String,
    pub artist: String,
    pub album: String,
    pub album_id: String,
    pub duration_ms: i64,
    pub thumbnail: String,
    pub added_at: String,
}

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct FavoriteAlbum {
    pub id: String,
    pub title: String,
    pub artist: String,
    pub year: Option<i32>,
    pub thumbnail: String,
    pub added_at: String,
}

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct FavoriteArtist {
    pub id: String,
    pub name: String,
    pub thumbnail: String,
    pub added_at: String,
}

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct Playlist {
    pub id: String,
    pub name: String,
    pub description: String,
    pub artwork: String,
    pub created_at: String,
    pub updated_at: String,
}

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct PlaylistTrack {
    pub playlist_id: String,
    pub track_id: String,
    pub position: i32,
    pub title: String,
    pub artist: String,
    pub album: String,
    pub duration_ms: i64,
    pub thumbnail: String,
    pub added_at: String,
}

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct RecentTrack {
    pub id: i64,
    pub track_id: String,
    pub title: String,
    pub artist: String,
    pub album: String,
    pub duration_ms: i64,
    pub thumbnail: String,
    pub played_at: String,
}

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct SearchEntry {
    pub id: i64,
    pub query: String,
    pub filter: String,
    pub searched_at: String,
}

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct DownloadTrack {
    pub video_id: String,
    pub title: String,
    pub artist: String,
    pub album: String,
    pub file_path: String,
    pub thumbnail_url: String,
    pub duration_ms: i64,
    pub downloaded_at: String,
    pub parent_playlist_id: Option<String>,
    pub parent_playlist_title: Option<String>,
    pub parent_playlist_thumbnail_url: Option<String>,
}

// ── Repositories ──

pub struct FavoritesRepo;

impl FavoritesRepo {
    pub fn is_favorite_track(track_id: &str) -> SqlResult<bool> {
        with_db(|conn| {
            conn.query_row(
                "SELECT COUNT(*) FROM favorite_tracks WHERE id = ?1",
                params![track_id],
                |r| r.get::<_, i64>(0),
            )
            .map(|c| c > 0)
        })
    }

    pub fn add_track(track: &FavoriteTrack) -> SqlResult<()> {
        with_db(|conn| {
            conn.execute(
                "INSERT OR IGNORE INTO favorite_tracks (id, title, artist, album, album_id, duration_ms, thumbnail)
                 VALUES (?1, ?2, ?3, ?4, ?5, ?6, ?7)",
                params![track.id, track.title, track.artist, track.album, track.album_id,
                         track.duration_ms, track.thumbnail],
            ).map(|_| ())
        })
    }

    pub fn remove_track(track_id: &str) -> SqlResult<()> {
        with_db(|conn| {
            conn.execute(
                "DELETE FROM favorite_tracks WHERE id = ?1",
                params![track_id],
            )
            .map(|_| ())
        })
    }

    pub fn all_tracks() -> SqlResult<Vec<FavoriteTrack>> {
        with_db(|conn| {
            let mut stmt = conn.prepare(
                "SELECT id, title, artist, album, album_id, duration_ms, thumbnail, added_at
                 FROM favorite_tracks ORDER BY added_at DESC",
            )?;
            let rows = stmt.query_map([], |r| {
                Ok(FavoriteTrack {
                    id: r.get(0)?,
                    title: r.get(1)?,
                    artist: r.get(2)?,
                    album: r.get(3)?,
                    album_id: r.get(4)?,
                    duration_ms: r.get(5)?,
                    thumbnail: r.get(6)?,
                    added_at: r.get(7)?,
                })
            })?;
            rows.collect()
        })
    }

    pub fn add_album(album: &FavoriteAlbum) -> SqlResult<()> {
        with_db(|conn| {
            conn.execute(
                "INSERT OR IGNORE INTO favorite_albums (id, title, artist, year, thumbnail)
                 VALUES (?1, ?2, ?3, ?4, ?5)",
                params![
                    album.id,
                    album.title,
                    album.artist,
                    album.year,
                    album.thumbnail
                ],
            )
            .map(|_| ())
        })
    }

    pub fn remove_album(album_id: &str) -> SqlResult<()> {
        with_db(|conn| {
            conn.execute(
                "DELETE FROM favorite_albums WHERE id = ?1",
                params![album_id],
            )
            .map(|_| ())
        })
    }

    pub fn all_albums() -> SqlResult<Vec<FavoriteAlbum>> {
        with_db(|conn| {
            let mut stmt = conn.prepare(
                "SELECT id, title, artist, year, thumbnail, added_at FROM favorite_albums ORDER BY added_at DESC"
            )?;
            let rows = stmt.query_map([], |r| {
                Ok(FavoriteAlbum {
                    id: r.get(0)?,
                    title: r.get(1)?,
                    artist: r.get(2)?,
                    year: r.get(3)?,
                    thumbnail: r.get(4)?,
                    added_at: r.get(5)?,
                })
            })?;
            rows.collect()
        })
    }

    pub fn add_artist(artist: &FavoriteArtist) -> SqlResult<()> {
        with_db(|conn| {
            conn.execute(
                "INSERT OR IGNORE INTO favorite_artists (id, name, thumbnail) VALUES (?1, ?2, ?3)",
                params![artist.id, artist.name, artist.thumbnail],
            )
            .map(|_| ())
        })
    }

    pub fn remove_artist(artist_id: &str) -> SqlResult<()> {
        with_db(|conn| {
            conn.execute(
                "DELETE FROM favorite_artists WHERE id = ?1",
                params![artist_id],
            )
            .map(|_| ())
        })
    }

    pub fn all_artists() -> SqlResult<Vec<FavoriteArtist>> {
        with_db(|conn| {
            let mut stmt = conn.prepare(
                "SELECT id, name, thumbnail, added_at FROM favorite_artists ORDER BY added_at DESC",
            )?;
            let rows = stmt.query_map([], |r| {
                Ok(FavoriteArtist {
                    id: r.get(0)?,
                    name: r.get(1)?,
                    thumbnail: r.get(2)?,
                    added_at: r.get(3)?,
                })
            })?;
            rows.collect()
        })
    }
}

pub struct PlaylistRepo;

impl PlaylistRepo {
    pub fn create(name: &str, description: &str) -> SqlResult<String> {
        let id = uuid_v4();
        with_db(|conn| {
            conn.execute(
                "INSERT INTO playlists (id, name, description) VALUES (?1, ?2, ?3)",
                params![id, name, description],
            )
        })?;
        Ok(id)
    }

    pub fn delete(playlist_id: &str) -> SqlResult<()> {
        with_db(|conn| {
            conn.execute("DELETE FROM playlists WHERE id = ?1", params![playlist_id])
                .map(|_| ())
        })
    }

    pub fn rename(playlist_id: &str, name: &str) -> SqlResult<()> {
        with_db(|conn| {
            conn.execute(
                "UPDATE playlists SET name = ?1, updated_at = datetime('now') WHERE id = ?2",
                params![name, playlist_id],
            )
            .map(|_| ())
        })
    }

    pub fn all() -> SqlResult<Vec<Playlist>> {
        with_db(|conn| {
            let mut stmt = conn.prepare(
                "SELECT id, name, description, artwork, created_at, updated_at FROM playlists ORDER BY updated_at DESC"
            )?;
            let rows = stmt.query_map([], |r| {
                Ok(Playlist {
                    id: r.get(0)?,
                    name: r.get(1)?,
                    description: r.get(2)?,
                    artwork: r.get(3)?,
                    created_at: r.get(4)?,
                    updated_at: r.get(5)?,
                })
            })?;
            rows.collect()
        })
    }

    pub fn add_track(playlist_id: &str, track: &PlaylistTrack) -> SqlResult<()> {
        with_db(|conn| {
            conn.execute(
                "INSERT OR IGNORE INTO playlist_tracks (playlist_id, track_id, position, title, artist, album, duration_ms, thumbnail)
                 VALUES (?1, ?2, ?3, ?4, ?5, ?6, ?7, ?8)",
                params![playlist_id, track.track_id, track.position, track.title, track.artist,
                         track.album, track.duration_ms, track.thumbnail],
            ).map(|_| ())
        })
    }

    pub fn remove_track(playlist_id: &str, track_id: &str) -> SqlResult<()> {
        with_db(|conn| {
            conn.execute(
                "DELETE FROM playlist_tracks WHERE playlist_id = ?1 AND track_id = ?2",
                params![playlist_id, track_id],
            )
            .map(|_| ())
        })
    }

    pub fn tracks(playlist_id: &str) -> SqlResult<Vec<PlaylistTrack>> {
        with_db(|conn| {
            let mut stmt = conn.prepare(
                "SELECT playlist_id, track_id, position, title, artist, album, duration_ms, thumbnail, added_at
                 FROM playlist_tracks WHERE playlist_id = ?1 ORDER BY position"
            )?;
            let rows = stmt.query_map(params![playlist_id], |r| {
                Ok(PlaylistTrack {
                    playlist_id: r.get(0)?,
                    track_id: r.get(1)?,
                    position: r.get(2)?,
                    title: r.get(3)?,
                    artist: r.get(4)?,
                    album: r.get(5)?,
                    duration_ms: r.get(6)?,
                    thumbnail: r.get(7)?,
                    added_at: r.get(8)?,
                })
            })?;
            rows.collect()
        })
    }
}

pub struct RecentlyPlayedRepo;

impl RecentlyPlayedRepo {
    pub fn record(
        track_id: &str,
        title: &str,
        artist: &str,
        album: &str,
        duration_ms: i64,
        thumbnail: &str,
    ) -> SqlResult<()> {
        with_db(|conn| {
            conn.execute(
                "INSERT INTO recently_played (track_id, title, artist, album, duration_ms, thumbnail)
                 VALUES (?1, ?2, ?3, ?4, ?5, ?6)",
                params![track_id, title, artist, album, duration_ms, thumbnail],
            ).map(|_| ())
        })
    }

    pub fn recent(limit: i64) -> SqlResult<Vec<RecentTrack>> {
        with_db(|conn| {
            let mut stmt = conn.prepare(
                "SELECT id, track_id, title, artist, album, duration_ms, thumbnail, played_at
                 FROM recently_played ORDER BY played_at DESC LIMIT ?1",
            )?;
            let rows = stmt.query_map(params![limit], |r| {
                Ok(RecentTrack {
                    id: r.get(0)?,
                    track_id: r.get(1)?,
                    title: r.get(2)?,
                    artist: r.get(3)?,
                    album: r.get(4)?,
                    duration_ms: r.get(5)?,
                    thumbnail: r.get(6)?,
                    played_at: r.get(7)?,
                })
            })?;
            rows.collect()
        })
    }

    pub fn clear() -> SqlResult<()> {
        with_db(|conn| conn.execute("DELETE FROM recently_played", []).map(|_| ()))
    }
}

pub struct SearchHistoryRepo;

impl SearchHistoryRepo {
    pub fn record(query: &str, filter: &str) -> SqlResult<()> {
        with_db(|conn| {
            conn.execute(
                "DELETE FROM search_history WHERE lower(query) = lower(?1)",
                params![query],
            )?;
            conn.execute(
                "INSERT INTO search_history (query, filter) VALUES (?1, ?2)",
                params![query, filter],
            )
            .map(|_| ())
        })
    }

    pub fn recent(limit: i64) -> SqlResult<Vec<SearchEntry>> {
        with_db(|conn| {
            let mut stmt = conn.prepare(
                "SELECT id, query, filter, searched_at FROM search_history ORDER BY searched_at DESC LIMIT ?1"
            )?;
            let rows = stmt.query_map(params![limit], |r| {
                Ok(SearchEntry {
                    id: r.get(0)?,
                    query: r.get(1)?,
                    filter: r.get(2)?,
                    searched_at: r.get(3)?,
                })
            })?;
            rows.collect()
        })
    }

    pub fn delete_entry(query: &str) -> SqlResult<()> {
        with_db(|conn| {
            conn.execute(
                "DELETE FROM search_history WHERE lower(query) = lower(?1)",
                params![query],
            )
            .map(|_| ())
        })
    }

    pub fn clear() -> SqlResult<()> {
        with_db(|conn| conn.execute("DELETE FROM search_history", []).map(|_| ()))
    }
}

pub struct DownloadsRepo;

impl DownloadsRepo {
    pub fn get(video_id: &str) -> SqlResult<Option<DownloadTrack>> {
        with_db(|conn| {
            let mut stmt = conn.prepare(
                "SELECT video_id, title, artist, album, file_path, thumbnail_url, duration_ms, downloaded_at,
                        parent_playlist_id, parent_playlist_title, parent_playlist_thumbnail_url
                 FROM downloads WHERE video_id = ?1 LIMIT 1"
            )?;
            let mut rows = stmt.query_map(params![video_id], |r| {
                Ok(DownloadTrack {
                    video_id: r.get(0)?,
                    title: r.get(1)?,
                    artist: r.get(2)?,
                    album: r.get(3)?,
                    file_path: r.get(4)?,
                    thumbnail_url: r.get(5)?,
                    duration_ms: r.get(6)?,
                    downloaded_at: r.get(7)?,
                    parent_playlist_id: r.get(8)?,
                    parent_playlist_title: r.get(9)?,
                    parent_playlist_thumbnail_url: r.get(10)?,
                })
            })?;
            if let Some(res) = rows.next() {
                res.map(Some)
            } else {
                Ok(None)
            }
        })
    }

    pub fn is_downloaded(video_id: &str) -> SqlResult<bool> {
        with_db(|conn| {
            conn.query_row(
                "SELECT COUNT(*) FROM downloads WHERE video_id = ?1",
                params![video_id],
                |r| r.get::<_, i64>(0),
            )
            .map(|c| c > 0)
        })
    }

    pub fn add(track: &DownloadTrack) -> SqlResult<()> {
        with_db(|conn| {
            conn.execute(
                "INSERT OR REPLACE INTO downloads (
                    video_id, title, artist, album, file_path, thumbnail_url, duration_ms,
                    parent_playlist_id, parent_playlist_title, parent_playlist_thumbnail_url
                 ) VALUES (?1, ?2, ?3, ?4, ?5, ?6, ?7, ?8, ?9, ?10)",
                params![
                    track.video_id,
                    track.title,
                    track.artist,
                    track.album,
                    track.file_path,
                    track.thumbnail_url,
                    track.duration_ms,
                    track.parent_playlist_id,
                    track.parent_playlist_title,
                    track.parent_playlist_thumbnail_url
                ],
            )
            .map(|_| ())
        })
    }

    pub fn remove(video_id: &str) -> SqlResult<()> {
        with_db(|conn| {
            conn.execute(
                "DELETE FROM downloads WHERE video_id = ?1",
                params![video_id],
            )
            .map(|_| ())
        })
    }

    pub fn clear_all() -> SqlResult<()> {
        with_db(|conn| conn.execute("DELETE FROM downloads", []).map(|_| ()))
    }

    pub fn all() -> SqlResult<Vec<DownloadTrack>> {
        with_db(|conn| {
            let mut stmt = conn.prepare(
                "SELECT video_id, title, artist, album, file_path, thumbnail_url, duration_ms, downloaded_at,
                        parent_playlist_id, parent_playlist_title, parent_playlist_thumbnail_url
                 FROM downloads ORDER BY downloaded_at DESC"
            )?;
            let rows = stmt.query_map([], |r| {
                Ok(DownloadTrack {
                    video_id: r.get(0)?,
                    title: r.get(1)?,
                    artist: r.get(2)?,
                    album: r.get(3)?,
                    file_path: r.get(4)?,
                    thumbnail_url: r.get(5)?,
                    duration_ms: r.get(6)?,
                    downloaded_at: r.get(7)?,
                    parent_playlist_id: r.get(8)?,
                    parent_playlist_title: r.get(9)?,
                    parent_playlist_thumbnail_url: r.get(10)?,
                })
            })?;
            rows.collect()
        })
    }
}

fn uuid_v4() -> String {
    use rand::RngExt;
    let mut rng = rand::rng();
    format!(
        "{:08x}-{:04x}-4{:03x}-{:04x}-{:012x}",
        rng.random::<u32>(),
        rng.random::<u16>(),
        rng.random::<u16>() & 0xfff,
        rng.random::<u16>() & 0x3fff | 0x8000,
        rng.random::<u64>(),
    )
}

#[cfg(test)]
#[cfg(test)]
mod tests {
    use super::*;
    use crate::db::Database;

    fn with_test_conn<F, R>(f: F) -> R
    where
        F: FnOnce() -> R,
    {
        use crate::db::{init_connection, take_connection};
        let conn = rusqlite::Connection::open_in_memory().unwrap();
        Database::run_migrations(&conn).unwrap();
        init_connection(conn);
        let res = f();
        let _ = take_connection();
        res
    }

    #[test]
    fn test_favorites_track_sql() {
        with_test_conn(|| {
            crate::db::with_db(|conn| {
                conn.execute(
                    "INSERT INTO favorite_tracks (id, title, artist, album) VALUES (?1, ?2, ?3, ?4)",
                    params!["t1", "Test Song", "Test Artist", "Test Album"],
                )
                .unwrap();
                let count: i64 = conn
                    .query_row(
                        "SELECT COUNT(*) FROM favorite_tracks WHERE id = ?1",
                        params!["t1"],
                        |r| r.get(0),
                    )
                    .unwrap();
                assert_eq!(count, 1);
                let title: String = conn
                    .query_row(
                        "SELECT title FROM favorite_tracks WHERE id = ?1",
                        params!["t1"],
                        |r| r.get(0),
                    )
                    .unwrap();
                assert_eq!(title, "Test Song");
                Ok(())
            })
            .unwrap();
        });
    }

    #[test]
    fn test_playlist_sql() {
        with_test_conn(|| {
            crate::db::with_db(|conn| {
                conn.execute(
                    "INSERT INTO playlists (id, name) VALUES ('p1', 'My Playlist')",
                    [],
                )
                .unwrap();
                let name: String = conn
                    .query_row("SELECT name FROM playlists WHERE id = 'p1'", [], |r| {
                        r.get(0)
                    })
                    .unwrap();
                assert_eq!(name, "My Playlist");

                conn.execute(
                    "INSERT INTO playlist_tracks (playlist_id, track_id, position, title, artist) VALUES ('p1', 't1', 0, 'Song', 'Artist')", []
                ).unwrap();
                let count: i64 = conn
                    .query_row(
                        "SELECT COUNT(*) FROM playlist_tracks WHERE playlist_id = 'p1'",
                        [],
                        |r| r.get(0),
                    )
                    .unwrap();
                assert_eq!(count, 1);
                Ok(())
            })
            .unwrap();
        });
    }

    #[test]
    fn test_recently_played_sql() {
        with_test_conn(|| {
            crate::db::with_db(|conn| {
                conn.execute(
                    "INSERT INTO recently_played (track_id, title, artist) VALUES ('t1', 'Song', 'Artist')", []
                ).unwrap();
                let count: i64 = conn
                    .query_row("SELECT COUNT(*) FROM recently_played", [], |r| r.get(0))
                    .unwrap();
                assert_eq!(count, 1);
                Ok(())
            })
            .unwrap();
        });
    }

    #[test]
    fn test_search_history_sql() {
        with_test_conn(|| {
            crate::db::with_db(|conn| {
                conn.execute(
                    "INSERT INTO search_history (query, filter) VALUES ('hello', 'songs')",
                    [],
                )
                .unwrap();
                conn.execute(
                    "INSERT INTO search_history (query, filter) VALUES ('world', 'all')",
                    [],
                )
                .unwrap();
                let count: i64 = conn
                    .query_row("SELECT COUNT(*) FROM search_history", [], |r| r.get(0))
                    .unwrap();
                assert_eq!(count, 2);
                Ok(())
            })
            .unwrap();

            // Test delete_entry (case-insensitive)
            SearchHistoryRepo::delete_entry("HELLO").unwrap();
            
            crate::db::with_db(|conn| {
                let count2: i64 = conn
                    .query_row("SELECT COUNT(*) FROM search_history", [], |r| r.get(0))
                    .unwrap();
                assert_eq!(count2, 1);
                Ok(())
            })
            .unwrap();

            // Test clear
            SearchHistoryRepo::clear().unwrap();

            crate::db::with_db(|conn| {
                let count3: i64 = conn
                    .query_row("SELECT COUNT(*) FROM search_history", [], |r| r.get(0))
                    .unwrap();
                assert_eq!(count3, 0);
                Ok(())
            })
            .unwrap();
        });
    }

    #[test]
    fn downloads_with_duplicate_text_are_selected_by_id() {
        with_test_conn(|| {
            crate::db::with_db(|conn| {
                conn.execute(
                    "INSERT INTO downloads (video_id, title, artist, file_path)
                     VALUES ('video-a', 'Same Song', 'Same Artist', '/tmp/a.m4a')",
                    [],
                )
                .unwrap();
                conn.execute(
                    "INSERT INTO downloads (video_id, title, artist, file_path)
                     VALUES ('video-b', 'Same Song', 'Same Artist', '/tmp/b.m4a')",
                    [],
                )
                .unwrap();

                let path: String = conn
                    .query_row(
                        "SELECT file_path FROM downloads WHERE video_id = ?1",
                        params!["video-b"],
                        |row| row.get(0),
                    )
                    .unwrap();

                assert_eq!(path, "/tmp/b.m4a");
                Ok(())
            })
            .unwrap();
        });
    }
}
