// Stats / History — extracted from bridge.rs
use super::bridge;

pub fn on_stats_requested(days: i32) {
    log::info!("Stats requested for {days} days");
    tokio::spawn(async move {
        let stats_res = tokio::task::spawn_blocking(move || {
            let days_i64 = days as i64;
            let total_time_ms =
                crate::db::repo::PlayHistoryRepo::total_play_time(days_i64).unwrap_or(0);
            let total_plays =
                crate::db::repo::PlayHistoryRepo::total_plays(days_i64).unwrap_or(0) as i32;
            let unique_artists =
                crate::db::repo::PlayHistoryRepo::unique_artists(days_i64).unwrap_or(0) as i32;

            // Weekly activity: last 7 days daily counts
            let mut weekly_activity = vec![0; 7];
            if let Ok(results) = crate::db::repo::PlayHistoryRepo::weekly_activity() {
                let now = chrono::Local::now();
                for i in 0..7 {
                    let d = now - chrono::Duration::days(6 - i as i64);
                    let d_str = d.format("%Y-%m-%d").to_string();
                    if let Some((_, cnt)) = results.iter().find(|(day, _)| day == &d_str) {
                        weekly_activity[i] = *cnt as i32;
                    }
                }
            }

            // Top 5 Tracks
            let mut top_tracks = Vec::new();
            let mut top_tracks_plays = Vec::new();

            if let Ok(results) = crate::db::repo::PlayHistoryRepo::top_tracks(5, days_i64) {
                for recent in results {
                    let mut thumb = recent.thumbnail.clone();
                    if thumb.is_empty() {
                        thumb = bridge::get_or_create_thumbnail(&recent.title, 0);
                    }
                    top_tracks.push(bridge::Track {
                        id: recent.track_id,
                        title: recent.title,
                        artist: recent.artist,
                        album: recent.album,
                        duration_ms: recent.duration_ms,
                        thumbnail: thumb,
                    });
                    top_tracks_plays.push(recent.play_count as i32);
                }
            }

            let total_secs = total_time_ms / 1000;
            let total_mins = total_secs / 60;
            let total_hours = total_mins / 60;
            let time_str = if total_hours > 0 {
                format!("{}h {}m", total_hours, total_mins % 60)
            } else {
                format!("{}m", total_mins)
            };

            bridge::StatsData {
                total_play_time: time_str,
                total_plays,
                unique_artists,
                weekly_activity,
                top_tracks,
                top_tracks_plays,
            }
        })
        .await;

        if let Ok(stats) = stats_res {
            bridge::set_stats_data(stats);
        }
    });
}

fn load_local_history() -> (Vec<bridge::Track>, Vec<String>) {
    let mut history = Vec::new();
    let mut played_at = Vec::new();
    if let Ok(results) = crate::db::with_db(|conn| {
        let mut stmt = conn.prepare(
            "SELECT track_id, title, artist, duration_ms, thumbnail, played_at
             FROM recently_played
             ORDER BY played_at DESC
             LIMIT 50",
        )?;
        let rows = stmt.query_map([], |row| {
            Ok((
                row.get::<_, String>(0)?,
                row.get::<_, String>(1)?,
                row.get::<_, String>(2)?,
                row.get::<_, i64>(3)?,
                row.get::<_, String>(4)?,
                row.get::<_, String>(5)?,
            ))
        })?;
        rows.collect::<Result<Vec<_>, rusqlite::Error>>()
    }) {
        for (id, title, artist, duration_ms, thumbnail, played) in results {
            history.push(bridge::Track {
                id,
                title,
                artist,
                album: String::new(),
                duration_ms,
                thumbnail,
            });
            played_at.push(played);
        }
    }
    (history, played_at)
}

fn remote_played_at(label: &str, index: usize) -> String {
    let label = label.to_lowercase();
    let days = if label.contains("today") || label.contains("hoy") {
        0
    } else if label.contains("yesterday") || label.contains("ayer") {
        1
    } else if label.contains("week") || label.contains("semana") {
        3
    } else if label.contains("month") || label.contains("mes") {
        14
    } else {
        30
    };
    (chrono::Local::now() - chrono::Duration::days(days) - chrono::Duration::seconds(index as i64))
        .format("%Y-%m-%d %H:%M:%S")
        .to_string()
}

pub fn on_history_requested() {
    if std::env::var_os("DOREMI_UI_TEST").is_some() {
        log::info!("Skipping history load during UI test");
        return;
    }
    log::info!("History requested");
    tokio::spawn(async move {
        if super::is_online() && crate::api::auth::is_authenticated() {
            match crate::api::innertube::remote_history().await {
                Ok(items) => {
                    let mut history = Vec::with_capacity(items.len());
                    let mut played_at = Vec::with_capacity(items.len());
                    let mut feedback_tokens = Vec::with_capacity(items.len());
                    for (index, item) in items.into_iter().enumerate() {
                        played_at.push(remote_played_at(&item.played, index));
                        feedback_tokens.push(item.feedback_token.clone().unwrap_or_default());
                        history.push(bridge::Track {
                            id: item.track.id,
                            title: item.track.title,
                            artist: item.track.artists.join(", "),
                            album: item.track.album.unwrap_or_default(),
                            duration_ms: item.track.duration_ms,
                            thumbnail: item.track.thumbnail,
                        });
                    }
                    bridge::set_history_data(history, played_at, feedback_tokens);
                    return;
                }
                Err(error) => {
                    log::warn!("Could not load remote YouTube Music history: {error}");
                }
            }
        }

        let local = tokio::task::spawn_blocking(load_local_history).await;
        if let Ok((history, played_at)) = local {
            let tokens = vec![String::new(); history.len()];
            bridge::set_history_data(history, played_at, tokens);
        }
    });
}

pub fn on_clear_history() {
    log::info!("Clear history requested");
    tokio::task::spawn_blocking(|| {
        crate::db::repo::PlayHistoryRepo::clear().ok();
    });
}

pub fn on_delete_history_item(track_id: &str, feedback_token: &str) {
    log::info!("Delete history item requested for track_id: {track_id}");
    let track_id_owned = track_id.to_string();
    let token_owned = feedback_token.to_string();
    tokio::spawn(async move {
        let local_res = tokio::task::spawn_blocking(move || {
            crate::db::repo::PlayHistoryRepo::remove(&track_id_owned)
        })
        .await;

        if let Ok(Err(e)) = local_res {
            log::error!("Failed to remove local history item: {e}");
        }

        if crate::api::auth::is_authenticated() && !token_owned.is_empty() {
            if let Err(e) = crate::api::endpoints::remove_remote_history_items(&[token_owned]).await
            {
                log::error!("Failed to remove remote history item: {e}");
            }
        }

        on_history_requested();
    });
}
