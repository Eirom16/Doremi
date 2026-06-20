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
pub struct FavoriteShow {
    pub id: String,
    pub title: String,
    pub author: String,
    pub description: String,
    pub thumbnail: String,
    pub episode_count: i32,
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
    pub privacy: i32,
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
    pub play_count: i32,
    pub progress_ms: i64,
    pub skipped: i32,
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
    pub status: String,
    pub progress: f64,
    pub error: String,
    pub cancelled: bool,
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

    pub fn is_favorite_album(album_id: &str) -> SqlResult<bool> {
        with_db(|conn| {
            conn.query_row(
                "SELECT COUNT(*) FROM favorite_albums WHERE id = ?1",
                params![album_id],
                |r| r.get::<_, i64>(0),
            )
            .map(|c| c > 0)
        })
    }

    pub fn is_favorite_artist(artist_id: &str) -> SqlResult<bool> {
        with_db(|conn| {
            conn.query_row(
                "SELECT COUNT(*) FROM favorite_artists WHERE id = ?1",
                params![artist_id],
                |r| r.get::<_, i64>(0),
            )
            .map(|count| count > 0)
        })
    }

    pub fn is_favorite_show(show_id: &str) -> SqlResult<bool> {
        with_db(|conn| {
            conn.query_row(
                "SELECT COUNT(*) FROM favorite_shows WHERE id = ?1",
                params![show_id],
                |r| r.get::<_, i64>(0),
            )
            .map(|count| count > 0)
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

    pub fn add_show(show: &FavoriteShow) -> SqlResult<()> {
        with_db(|conn| {
            conn.execute(
                "INSERT OR IGNORE INTO favorite_shows (id, title, author, description, thumbnail, episode_count)
                 VALUES (?1, ?2, ?3, ?4, ?5, ?6)",
                params![show.id, show.title, show.author, show.description, show.thumbnail, show.episode_count],
            )?;
            Ok(())
        })
    }

    pub fn remove_show(show_id: &str) -> SqlResult<()> {
        with_db(|conn| {
            conn.execute("DELETE FROM favorite_shows WHERE id = ?1", params![show_id])?;
            Ok(())
        })
    }

    pub fn all_shows() -> SqlResult<Vec<FavoriteShow>> {
        with_db(|conn| {
            let mut stmt = conn.prepare(
                "SELECT id, title, author, description, thumbnail, episode_count, added_at
                 FROM favorite_shows ORDER BY added_at DESC",
            )?;
            let rows = stmt.query_map([], |r| {
                Ok(FavoriteShow {
                    id: r.get(0)?,
                    title: r.get(1)?,
                    author: r.get(2)?,
                    description: r.get(3)?,
                    thumbnail: r.get(4)?,
                    episode_count: r.get(5)?,
                    added_at: r.get(6)?,
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
                "INSERT INTO playlists (id, name, description, privacy) VALUES (?1, ?2, ?3, 1)",
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
                "SELECT id, name, description, artwork, created_at, updated_at, privacy FROM playlists ORDER BY updated_at DESC"
            )?;
            let rows = stmt.query_map([], |r| {
                Ok(Playlist {
                    id: r.get(0)?,
                    name: r.get(1)?,
                    description: r.get(2)?,
                    artwork: r.get(3)?,
                    created_at: r.get(4)?,
                    updated_at: r.get(5)?,
                    privacy: r.get(6)?,
                })
            })?;
            rows.collect()
        })
    }

    pub fn get(playlist_id: &str) -> SqlResult<Playlist> {
        with_db(|conn| {
            conn.query_row(
                "SELECT id, name, description, artwork, created_at, updated_at, privacy
                 FROM playlists WHERE id = ?1",
                params![playlist_id],
                |r| {
                    Ok(Playlist {
                        id: r.get(0)?,
                        name: r.get(1)?,
                        description: r.get(2)?,
                        artwork: r.get(3)?,
                        created_at: r.get(4)?,
                        updated_at: r.get(5)?,
                        privacy: r.get(6)?,
                    })
                },
            )
        })
    }

    pub fn update(playlist: &Playlist) -> SqlResult<()> {
        with_db(|conn| {
            conn.execute(
                "UPDATE playlists SET name = ?1, description = ?2, privacy = ?3, updated_at = datetime('now')
                 WHERE id = ?4",
                params![playlist.name, playlist.description, playlist.privacy, playlist.id],
            )
            .map(|_| ())
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

pub struct PlayHistoryRepo;

impl PlayHistoryRepo {
    pub fn record(
        track_id: &str,
        title: &str,
        artist: &str,
        album: &str,
        duration_ms: i64,
        thumbnail: &str,
        progress_ms: i64,
        skipped: bool,
    ) -> SqlResult<()> {
        with_db(|conn| {
            conn.execute(
                "INSERT INTO recently_played (track_id, title, artist, album, duration_ms, thumbnail, play_count, progress_ms, skipped)
                 VALUES (?1, ?2, ?3, ?4, ?5, ?6, 1, ?7, ?8)
                 ON CONFLICT(track_id) DO UPDATE SET
                     play_count = play_count + 1,
                     played_at = datetime('now'),
                     progress_ms = ?7,
                     skipped = ?8,
                     title = ?2,
                     artist = ?3,
                     album = ?4,
                     duration_ms = ?5,
                     thumbnail = ?6",
                params![track_id, title, artist, album, duration_ms, thumbnail, progress_ms, skipped as i32],
            ).map(|_| ())
        })
    }

    pub fn recent(limit: i64) -> SqlResult<Vec<RecentTrack>> {
        with_db(|conn| {
            let mut stmt = conn.prepare(
                "SELECT id, track_id, title, artist, album, duration_ms, thumbnail, played_at,
                        play_count, progress_ms, skipped
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
                    play_count: r.get(8)?,
                    progress_ms: r.get(9)?,
                    skipped: r.get(10)?,
                })
            })?;
            rows.collect()
        })
    }

    pub fn clear() -> SqlResult<()> {
        with_db(|conn| conn.execute("DELETE FROM recently_played", []).map(|_| ()))
    }

    pub fn total_play_time(days: i64) -> SqlResult<i64> {
        with_db(|conn| {
            conn.query_row(
                "SELECT COALESCE(SUM(duration_ms), 0) FROM recently_played
                 WHERE played_at >= datetime('now', ?1)",
                params![format!("-{} days", days)],
                |r| r.get(0),
            )
        })
    }

    pub fn unique_artists(days: i64) -> SqlResult<i64> {
        with_db(|conn| {
            conn.query_row(
                "SELECT COUNT(DISTINCT artist) FROM recently_played
                 WHERE played_at >= datetime('now', ?1)",
                params![format!("-{} days", days)],
                |r| r.get(0),
            )
        })
    }

    pub fn top_tracks(limit: i64, days: i64) -> SqlResult<Vec<RecentTrack>> {
        with_db(|conn| {
            let mut stmt = conn.prepare(
                "SELECT id, track_id, title, artist, album, duration_ms, thumbnail, played_at,
                        play_count, progress_ms, skipped
                 FROM recently_played
                 WHERE played_at >= datetime('now', ?1)
                 ORDER BY play_count DESC LIMIT ?2",
            )?;
            let rows = stmt.query_map(params![format!("-{} days", days), limit], |r| {
                Ok(RecentTrack {
                    id: r.get(0)?,
                    track_id: r.get(1)?,
                    title: r.get(2)?,
                    artist: r.get(3)?,
                    album: r.get(4)?,
                    duration_ms: r.get(5)?,
                    thumbnail: r.get(6)?,
                    played_at: r.get(7)?,
                    play_count: r.get(8)?,
                    progress_ms: r.get(9)?,
                    skipped: r.get(10)?,
                })
            })?;
            rows.collect()
        })
    }

    pub fn weekly_activity() -> SqlResult<Vec<(String, i64)>> {
        with_db(|conn| {
            let mut stmt = conn.prepare(
                "SELECT DATE(played_at) as day, COUNT(*) as plays
                 FROM recently_played
                 WHERE played_at >= datetime('now', '-7 days')
                 GROUP BY day ORDER BY day",
            )?;
            let rows = stmt.query_map([], |r| Ok((r.get::<_, String>(0)?, r.get::<_, i64>(1)?)))?;
            rows.collect()
        })
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
                "SELECT video_id, title, artist, album, file_path, thumbnail_url, duration_ms,
                        downloaded_at, parent_playlist_id, parent_playlist_title,
                        parent_playlist_thumbnail_url, status, progress, error, cancelled
                 FROM downloads WHERE video_id = ?1 LIMIT 1",
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
                    status: r.get(11)?,
                    progress: r.get(12)?,
                    error: r.get(13)?,
                    cancelled: r.get::<_, i32>(14)? != 0,
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
                    parent_playlist_id, parent_playlist_title, parent_playlist_thumbnail_url,
                    status, progress, error, cancelled
                 ) VALUES (?1, ?2, ?3, ?4, ?5, ?6, ?7, ?8, ?9, ?10, ?11, ?12, ?13, ?14)",
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
                    track.parent_playlist_thumbnail_url,
                    track.status,
                    track.progress,
                    track.error,
                    if track.cancelled { 1 } else { 0 },
                ],
            )
            .map(|_| ())
        })
    }

    pub fn update_status(
        video_id: &str,
        status: &str,
        progress: f64,
        error: &str,
    ) -> SqlResult<()> {
        with_db(|conn| {
            conn.execute(
                "UPDATE downloads SET status = ?1, progress = ?2, error = ?3 WHERE video_id = ?4",
                params![status, progress, error, video_id],
            )
            .map(|_| ())
        })
    }

    pub fn mark_cancelled(video_id: &str) -> SqlResult<()> {
        with_db(|conn| {
            conn.execute(
                "UPDATE downloads SET status = 'cancelled', cancelled = 1 WHERE video_id = ?1",
                params![video_id],
            )
            .map(|_| ())
        })
    }

    pub fn batch_progress(parent_playlist_id: &str) -> SqlResult<(i64, i64, f64)> {
        with_db(|conn| {
            let total: i64 = conn.query_row(
                "SELECT COUNT(*) FROM downloads WHERE parent_playlist_id = ?1",
                params![parent_playlist_id],
                |r| r.get(0),
            )?;
            let completed: i64 = conn.query_row(
                "SELECT COUNT(*) FROM downloads WHERE parent_playlist_id = ?1 AND status = 'completed'",
                params![parent_playlist_id],
                |r| r.get(0),
            )?;
            let avg_progress: f64 = conn.query_row(
                "SELECT COALESCE(AVG(progress), 0.0) FROM downloads WHERE parent_playlist_id = ?1",
                params![parent_playlist_id],
                |r| r.get(0),
            )?;
            Ok((total, completed, avg_progress))
        })
    }

    pub fn find_queued() -> SqlResult<Vec<DownloadTrack>> {
        with_db(|conn| {
            let mut stmt = conn.prepare(
                "SELECT video_id, title, artist, album, file_path, thumbnail_url, duration_ms,
                        downloaded_at, parent_playlist_id, parent_playlist_title,
                        parent_playlist_thumbnail_url, status, progress, error, cancelled
                 FROM downloads WHERE status = 'queued' ORDER BY downloaded_at ASC",
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
                    status: r.get(11)?,
                    progress: r.get(12)?,
                    error: r.get(13)?,
                    cancelled: r.get::<_, i32>(14)? != 0,
                })
            })?;
            rows.collect()
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
                "SELECT video_id, title, artist, album, file_path, thumbnail_url, duration_ms,
                        downloaded_at, parent_playlist_id, parent_playlist_title,
                        parent_playlist_thumbnail_url, status, progress, error, cancelled
                 FROM downloads ORDER BY downloaded_at DESC",
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
                    status: r.get(11)?,
                    progress: r.get(12)?,
                    error: r.get(13)?,
                    cancelled: r.get::<_, i32>(14)? != 0,
                })
            })?;
            rows.collect()
        })
    }
}

pub struct ShowCacheRepo;

impl ShowCacheRepo {
    pub fn save(
        show: &crate::api::models::Show,
        episodes: &[crate::api::models::Episode],
    ) -> SqlResult<()> {
        with_db(|conn| {
            conn.execute(
                "INSERT OR REPLACE INTO cached_shows (id, title, author, description, thumbnail, episode_count)
                 VALUES (?1, ?2, ?3, ?4, ?5, ?6)",
                params![
                    show.id,
                    show.title,
                    show.author,
                    show.description,
                    show.thumbnail,
                    show.episode_count.unwrap_or(episodes.len() as i32)
                ],
            )?;

            conn.execute(
                "DELETE FROM cached_show_episodes WHERE show_id = ?1",
                params![show.id],
            )?;

            for ep in episodes {
                conn.execute(
                    "INSERT OR REPLACE INTO cached_show_episodes (id, show_id, title, show_title, description, thumbnail, duration_ms, published_at, position)
                     VALUES (?1, ?2, ?3, ?4, ?5, ?6, ?7, ?8, ?9)",
                    params![
                        ep.id,
                        show.id,
                        ep.title,
                        ep.show,
                        ep.description,
                        ep.thumbnail,
                        ep.duration_ms,
                        ep.published_at,
                        ep.position
                    ],
                )?;
            }
            Ok(())
        })
    }

    pub fn get(
        show_id: &str,
    ) -> SqlResult<Option<(crate::api::models::Show, Vec<crate::api::models::Episode>)>> {
        let guard = match crate::db::global().lock() {
            Ok(g) => g,
            Err(e) => e.into_inner(),
        };
        let conn = match guard.as_ref() {
            Some(c) => c,
            None => return Ok(None),
        };

        let show_row: Result<crate::api::models::Show, _> = conn.query_row(
            "SELECT id, title, author, description, thumbnail, episode_count FROM cached_shows WHERE id = ?1",
            params![show_id],
            |r| {
                Ok(crate::api::models::Show {
                    id: r.get(0)?,
                    title: r.get(1)?,
                    author: r.get(2)?,
                    description: r.get(3)?,
                    thumbnail: r.get(4)?,
                    episode_count: r.get(5)?,
                    subscriber_count: None,
                })
            },
        );

        let show = match show_row {
            Ok(s) => s,
            Err(rusqlite::Error::QueryReturnedNoRows) => return Ok(None),
            Err(e) => return Err(e),
        };

        let mut stmt = conn.prepare(
            "SELECT id, title, show_title, description, thumbnail, duration_ms, published_at, position
             FROM cached_show_episodes WHERE show_id = ?1 ORDER BY position ASC, added_at DESC",
        )?;
        let episode_rows = stmt.query_map(params![show_id], |r| {
            Ok(crate::api::models::Episode {
                id: r.get(0)?,
                title: r.get(1)?,
                show: r.get(2)?,
                show_id: show_id.to_string(),
                description: r.get(3)?,
                thumbnail: r.get(4)?,
                duration_ms: r.get(5)?,
                published_at: r.get(6)?,
                position: r.get(7)?,
            })
        })?;

        let mut episodes = Vec::new();
        for ep in episode_rows {
            episodes.push(ep?);
        }

        Ok(Some((show, episodes)))
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

    #[test]
    fn test_show_cache_sql() {
        with_test_conn(|| {
            use crate::api::models::{Episode, Show};
            let show = Show {
                id: "show-1".to_string(),
                title: "Test Show".to_string(),
                author: "Author A".to_string(),
                description: "Description of Show".to_string(),
                thumbnail: "thumb_base64".to_string(),
                episode_count: Some(2),
                subscriber_count: None,
            };
            let episodes = vec![
                Episode {
                    id: "ep-1".to_string(),
                    title: "Episode 1".to_string(),
                    show: "Test Show".to_string(),
                    show_id: "show-1".to_string(),
                    description: "First episode".to_string(),
                    thumbnail: "ep_thumb_1".to_string(),
                    duration_ms: 300000,
                    published_at: "2026-06-19".to_string(),
                    position: Some(1),
                },
                Episode {
                    id: "ep-2".to_string(),
                    title: "Episode 2".to_string(),
                    show: "Test Show".to_string(),
                    show_id: "show-1".to_string(),
                    description: "Second episode".to_string(),
                    thumbnail: "ep_thumb_2".to_string(),
                    duration_ms: 400000,
                    published_at: "2026-06-20".to_string(),
                    position: Some(2),
                },
            ];

            ShowCacheRepo::save(&show, &episodes).unwrap();

            let cached = ShowCacheRepo::get("show-1").unwrap();
            assert!(cached.is_some());
            let (cached_show, cached_eps) = cached.unwrap();
            assert_eq!(cached_show.title, "Test Show");
            assert_eq!(cached_show.episode_count, Some(2));
            assert_eq!(cached_eps.len(), 2);
            assert_eq!(cached_eps[0].title, "Episode 1");
            assert_eq!(cached_eps[1].title, "Episode 2");
        });
    }
}
