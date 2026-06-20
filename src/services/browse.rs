pub async fn load_album(browse_id: &str) {
    match crate::api::innertube::album_detail(browse_id).await {
        Ok((album, tracks)) => {
            let album = crate::bridge::bridge::Album {
                id: album.id,
                title: album.title,
                artist: album.artists.join(", "),
                year: album.year.map(|year| year.to_string()).unwrap_or_default(),
                thumbnail: album.thumbnail,
                track_count: album.track_count.unwrap_or(tracks.len() as i32),
                artist_id: album.artist_id.unwrap_or_default(),
            };
            let tracks = tracks
                .into_iter()
                .map(|track| crate::bridge::bridge::Track {
                    id: track.id,
                    title: track.title,
                    artist: track.artists.join(", "),
                    album: track.album.unwrap_or_default(),
                    duration_ms: track.duration_ms,
                    thumbnail: track.thumbnail,
                })
                .collect();
            crate::bridge::bridge::set_album_detail(album, tracks);
        }
        Err(error) => crate::bridge::bridge::show_notification(
            &format!("No se pudo cargar el álbum: {error}"),
            "error",
        ),
    }
}

pub async fn load_artist(browse_id: &str) {
    match crate::api::innertube::artist_detail(browse_id).await {
        Ok(mut detail) => {
            let artist = crate::bridge::bridge::Artist {
                id: detail.artist.id,
                name: detail.artist.name,
                thumbnail: detail.artist.thumbnail,
                description: detail.description.unwrap_or_default(),
                subscribers: detail.artist.subscriber_count.unwrap_or_default(),
            };
            let tracks = detail
                .top_songs
                .into_iter()
                .map(|track| crate::bridge::bridge::Track {
                    id: track.id,
                    title: track.title,
                    artist: track.artists.join(", "),
                    album: track.album.unwrap_or_default(),
                    duration_ms: track.duration_ms,
                    thumbnail: track.thumbnail,
                })
                .collect();
            detail.albums.append(&mut detail.singles);
            let albums = detail
                .albums
                .into_iter()
                .map(|album| crate::bridge::bridge::Album {
                    id: album.id,
                    title: album.title,
                    artist: album.artists.join(", "),
                    year: album.year.map(|year| year.to_string()).unwrap_or_default(),
                    thumbnail: album.thumbnail,
                    track_count: album.track_count.unwrap_or_default(),
                    artist_id: album.artist_id.unwrap_or_default(),
                })
                .collect();
            crate::bridge::bridge::set_artist_detail(artist, tracks, albums);
        }
        Err(error) => crate::bridge::bridge::show_notification(
            &format!("No se pudo cargar el artista: {error}"),
            "error",
        ),
    }
}

pub async fn load_playlist(playlist_id: &str) {
    match crate::api::innertube::playlist_detail(playlist_id).await {
        Ok(detail) => {
            let playlist = crate::bridge::bridge::Playlist {
                id: detail.playlist.id,
                name: detail.playlist.title,
                description: detail.playlist.description.unwrap_or_default(),
                thumbnail: detail.playlist.thumbnail,
                track_count: detail
                    .playlist
                    .track_count
                    .unwrap_or(detail.tracks.len() as i32),
                owner: detail.playlist.owner.clone().unwrap_or_default(),
                privacy: detail.privacy.clone(),
            };
            let tracks = detail
                .tracks
                .into_iter()
                .map(|track| crate::bridge::bridge::Track {
                    id: track.id,
                    title: track.title,
                    artist: track.artists.join(", "),
                    album: track.album.unwrap_or_default(),
                    duration_ms: track.duration_ms,
                    thumbnail: track.thumbnail,
                })
                .collect();
            crate::bridge::bridge::set_playlist_detail(playlist, tracks);
            if detail.unavailable_count > 0 {
                crate::bridge::bridge::show_notification(
                    &format!(
                        "{} canciones no están disponibles",
                        detail.unavailable_count
                    ),
                    "info",
                );
            }
        }
        Err(error) => crate::bridge::bridge::show_notification(
            &format!("No se pudo cargar la playlist: {error}"),
            "error",
        ),
    }
}

pub async fn load_show(browse_id: &str) {
    match crate::api::innertube::show_detail(browse_id).await {
        Ok(detail) => {
            if let Err(e) = crate::db::repo::ShowCacheRepo::save(&detail.show, &detail.episodes) {
                log::error!("Failed to cache show detail in database: {e}");
            }

            let show = crate::bridge::bridge::Show {
                id: detail.show.id,
                title: detail.show.title,
                author: detail.show.author,
                description: detail.show.description,
                thumbnail: detail.show.thumbnail,
                episode_count: detail.show.episode_count.unwrap_or(detail.episodes.len() as i32),
            };
            let episodes = detail
                .episodes
                .into_iter()
                .map(|ep| crate::bridge::bridge::Episode {
                    id: ep.id,
                    title: ep.title,
                    show: ep.show,
                    show_id: ep.show_id,
                    description: ep.description,
                    thumbnail: ep.thumbnail,
                    duration_ms: ep.duration_ms,
                })
                .collect();
            crate::bridge::bridge::set_show_detail(show, episodes);
        }
        Err(error) => {
            log::warn!("Failed to fetch show detail from network: {error}");
            match crate::db::repo::ShowCacheRepo::get(browse_id) {
                Ok(Some((show_model, episodes_model))) => {
                    let show = crate::bridge::bridge::Show {
                        id: show_model.id,
                        title: show_model.title,
                        author: show_model.author,
                        description: show_model.description,
                        thumbnail: show_model.thumbnail,
                        episode_count: show_model.episode_count.unwrap_or(episodes_model.len() as i32),
                    };
                    let episodes = episodes_model
                        .into_iter()
                        .map(|ep| crate::bridge::bridge::Episode {
                            id: ep.id,
                            title: ep.title,
                            show: ep.show,
                            show_id: ep.show_id,
                            description: ep.description,
                            thumbnail: ep.thumbnail,
                            duration_ms: ep.duration_ms,
                        })
                        .collect();
                    crate::bridge::bridge::set_show_detail(show, episodes);
                    crate::bridge::bridge::show_notification(
                        "Cargada copia local sin conexión",
                        "info",
                    );
                }
                _ => {
                    crate::bridge::bridge::show_notification(
                        &format!("No se pudo cargar el podcast: {error}"),
                        "error",
                    );
                }
            }
        }
    }
}
