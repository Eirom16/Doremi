// Library / Favorites / Playlists — extracted from bridge.rs
use super::bridge;
use std::sync::atomic::Ordering;

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub(super) enum LibraryTab {
    Songs,
    Albums,
    Artists,
    Playlists,
    Shows,
}

impl LibraryTab {
    pub(super) fn from_key(key: &str) -> Option<Self> {
        match key {
            "songs" => Some(Self::Songs),
            "albums" => Some(Self::Albums),
            "artists" => Some(Self::Artists),
            "playlists" => Some(Self::Playlists),
            "shows" => Some(Self::Shows),
            _ => None,
        }
    }

    pub(super) fn to_key(&self) -> &'static str {
        match self {
            Self::Songs => "songs",
            Self::Albums => "albums",
            Self::Artists => "artists",
            Self::Playlists => "playlists",
            Self::Shows => "shows",
        }
    }
}

fn load_local_library_tab(tab: LibraryTab) {
    use crate::db::repo::*;
    match tab {
        LibraryTab::Songs => {
            if let Ok(tracks) = FavoritesRepo::all_tracks() {
                let songs: Vec<bridge::Track> = tracks
                    .iter()
                    .map(|t| bridge::Track {
                        id: t.id.clone(),
                        title: t.title.clone(),
                        artist: t.artist.clone(),
                        album: t.album.clone(),
                        duration_ms: t.duration_ms,
                        thumbnail: t.thumbnail.clone(),
                    })
                    .collect();

                if let Ok(cache) = crate::services::library_cache::GLOBAL_LIBRARY_CACHE.write() {
                    cache.set(
                        "songs",
                        crate::services::library_cache::CacheData {
                            songs: songs.clone(),
                            albums: Vec::new(),
                            artists: Vec::new(),
                            playlists: Vec::new(),
                            source: crate::services::library_cache::LibrarySource::Local,
                        },
                    );
                }

                bridge::set_library_songs(songs);
            }
        }
        LibraryTab::Albums => {
            if let Ok(albums) = FavoritesRepo::all_albums() {
                let a_list: Vec<bridge::Album> = albums
                    .iter()
                    .map(|a| bridge::Album {
                        id: a.id.clone(),
                        title: a.title.clone(),
                        artist: a.artist.clone(),
                        year: a.year.map(|y| y.to_string()).unwrap_or_default(),
                        thumbnail: a.thumbnail.clone(),
                        track_count: 0,
                        artist_id: String::new(),
                    })
                    .collect();

                if let Ok(cache) = crate::services::library_cache::GLOBAL_LIBRARY_CACHE.write() {
                    cache.set(
                        "albums",
                        crate::services::library_cache::CacheData {
                            songs: Vec::new(),
                            albums: a_list.clone(),
                            artists: Vec::new(),
                            playlists: Vec::new(),
                            source: crate::services::library_cache::LibrarySource::Local,
                        },
                    );
                }

                bridge::set_library_albums(a_list);
            }
        }
        LibraryTab::Artists => {
            if let Ok(artists) = FavoritesRepo::all_artists() {
                let art_list: Vec<bridge::Artist> = artists
                    .iter()
                    .map(|a| bridge::Artist {
                        id: a.id.clone(),
                        name: a.name.clone(),
                        thumbnail: a.thumbnail.clone(),
                        description: String::new(),
                        subscribers: String::new(),
                    })
                    .collect();

                if let Ok(cache) = crate::services::library_cache::GLOBAL_LIBRARY_CACHE.write() {
                    cache.set(
                        "artists",
                        crate::services::library_cache::CacheData {
                            songs: Vec::new(),
                            albums: Vec::new(),
                            artists: art_list.clone(),
                            playlists: Vec::new(),
                            source: crate::services::library_cache::LibrarySource::Local,
                        },
                    );
                }

                bridge::set_library_artists(art_list);
            }
        }
        LibraryTab::Playlists => {
            if let Ok(playlists) = PlaylistRepo::all() {
                let p_list: Vec<bridge::Playlist> = playlists
                    .iter()
                    .map(|p| {
                        let count = PlaylistRepo::tracks(&p.id)
                            .ok()
                            .map(|t| t.len() as i32)
                            .unwrap_or(0);
                        bridge::Playlist {
                            id: p.id.clone(),
                            name: p.name.clone(),
                            description: p.description.clone(),
                            thumbnail: p.artwork.clone(),
                            track_count: count,
                            owner: String::new(),
                            privacy: String::new(),
                            editable: true,
                        }
                    })
                    .collect();

                if let Ok(cache) = crate::services::library_cache::GLOBAL_LIBRARY_CACHE.write() {
                    cache.set(
                        "playlists",
                        crate::services::library_cache::CacheData {
                            songs: Vec::new(),
                            albums: Vec::new(),
                            artists: Vec::new(),
                            playlists: p_list.clone(),
                            source: crate::services::library_cache::LibrarySource::Local,
                        },
                    );
                }

                bridge::set_library_playlists(p_list);
            }
        }
        LibraryTab::Shows => refresh_library_shows_ui(),
    }
}

pub fn on_library_tab_changed(tab_key: &str) {
    if std::env::var_os("DOREMI_UI_TEST").is_some() {
        log::info!("Skipping library tab load during UI test");
        return;
    }
    log::info!("Library tab changed: {tab_key}");
    let tab_key = tab_key.to_string();
    tokio::spawn(async move {
        let Some(tab) = LibraryTab::from_key(&tab_key) else {
            log::warn!("Ignoring unknown library tab key: {tab_key}");
            return;
        };

        let source_filter = super::FILTER_SOURCE.load(Ordering::SeqCst);

        if source_filter == 2 {
            load_downloaded_library_tab(tab);
        } else if source_filter == 3 || !super::is_online() || !super::is_youtube_authenticated() {
            tokio::task::spawn_blocking(move || load_local_library_tab(tab))
                .await
                .ok();
        } else {
            let service = crate::services::library::LibraryService::new(true);
            match tab {
                LibraryTab::Songs => service.load_songs().await,
                LibraryTab::Albums => service.load_albums().await,
                LibraryTab::Artists => service.load_artists().await,
                LibraryTab::Playlists => service.load_playlists().await,
                LibraryTab::Shows => refresh_library_shows_ui(),
            }
            if source_filter == 0 {
                on_library_search(tab.to_key(), "", "");
            }
        }
    });
}

pub fn on_remove_favorite_impl(track_id: &str) {
    log::info!("Remove favorite: {track_id}");
    if let Err(e) = crate::db::repo::FavoritesRepo::remove_track(track_id) {
        log::error!("Failed to remove favorite: {e}");
    } else {
        if let Ok(cache) = crate::services::library_cache::GLOBAL_LIBRARY_CACHE.write() {
            cache.invalidate("songs");
        }
    }
}

pub fn on_remove_favorite(track_id: &str) {
    let track_id = track_id.to_string();
    tokio::spawn(async move {
        let local_id = track_id.clone();
        tokio::task::spawn_blocking(move || {
            on_remove_favorite_impl(&local_id);
        })
        .await
        .ok();
        if let Err(error) =
            crate::api::innertube::rate_song(&track_id, crate::api::models::LikeStatus::Indifferent)
                .await
        {
            log::debug!("Remote unlike was not applied for {track_id}: {error}");
        }
    });
}

pub fn on_add_favorite_impl(track: bridge::Track) {
    if track.id.trim().is_empty() || track.title.trim().is_empty() {
        log::warn!("Rejected adding favorite track: empty id or title");
        return;
    }
    log::info!("Add favorite: {} — {}", track.title, track.artist);
    use crate::db::repo::FavoriteTrack;
    let fav_track = FavoriteTrack {
        id: track.id.clone(),
        title: track.title,
        artist: track.artist,
        album: track.album,
        album_id: String::new(),
        duration_ms: track.duration_ms,
        thumbnail: track.thumbnail,
        added_at: String::new(),
    };
    if let Err(e) = crate::db::repo::FavoritesRepo::add_track(&fav_track) {
        log::error!("Failed to add favorite: {e}");
    } else {
        log::info!("Added favorite: {} — {}", fav_track.title, fav_track.artist);
        if let Ok(cache) = crate::services::library_cache::GLOBAL_LIBRARY_CACHE.write() {
            cache.invalidate("songs");
        }
    }
}

pub fn on_add_favorite(track: bridge::Track) {
    let track_id = track.id.clone();
    tokio::spawn(async move {
        tokio::task::spawn_blocking(move || {
            on_add_favorite_impl(track);
        })
        .await
        .ok();
        if let Err(error) =
            crate::api::innertube::rate_song(&track_id, crate::api::models::LikeStatus::Like).await
        {
            log::debug!("Remote like was not applied for {track_id}: {error}");
        }
    });
}

pub fn on_add_favorite_album(album: bridge::Album) {
    let fav = crate::db::repo::FavoriteAlbum {
        id: album.id,
        title: album.title,
        artist: album.artist,
        year: album.year.parse::<i32>().ok(),
        thumbnail: album.thumbnail,
        added_at: String::new(),
    };
    if let Err(e) = crate::db::repo::FavoritesRepo::add_album(&fav) {
        log::error!("Failed to add favorite album: {e}");
    } else {
        log::info!("Added favorite album: {}", fav.title);
        if let Ok(cache) = crate::services::library_cache::GLOBAL_LIBRARY_CACHE.write() {
            cache.invalidate("albums");
        }
    }
}

pub fn on_remove_favorite_album(album_id: &str) {
    if let Err(e) = crate::db::repo::FavoritesRepo::remove_album(album_id) {
        log::error!("Failed to remove favorite album: {e}");
    } else {
        if let Ok(cache) = crate::services::library_cache::GLOBAL_LIBRARY_CACHE.write() {
            cache.invalidate("albums");
        }
    }
}

pub fn on_add_favorite_artist(artist: bridge::Artist) {
    let fav = crate::db::repo::FavoriteArtist {
        id: artist.id,
        name: artist.name,
        thumbnail: artist.thumbnail,
        added_at: String::new(),
    };
    if let Err(e) = crate::db::repo::FavoritesRepo::add_artist(&fav) {
        log::error!("Failed to add favorite artist: {e}");
    } else {
        log::info!("Added favorite artist: {}", fav.name);
        if let Ok(cache) = crate::services::library_cache::GLOBAL_LIBRARY_CACHE.write() {
            cache.invalidate("artists");
        }
    }
}

pub fn on_remove_favorite_artist(artist_id: &str) {
    if let Err(e) = crate::db::repo::FavoritesRepo::remove_artist(artist_id) {
        log::error!("Failed to remove favorite artist: {e}");
    } else {
        if let Ok(cache) = crate::services::library_cache::GLOBAL_LIBRARY_CACHE.write() {
            cache.invalidate("artists");
        }
    }
}

pub fn on_add_favorite_show(show: bridge::Show) {
    let fav = crate::db::repo::FavoriteShow {
        id: show.id,
        title: show.title,
        author: show.author,
        description: show.description,
        thumbnail: show.thumbnail,
        episode_count: show.episode_count,
        added_at: String::new(),
    };
    if let Err(e) = crate::db::repo::FavoritesRepo::add_show(&fav) {
        log::error!("Failed to add favorite show: {e}");
    } else {
        log::info!("Added favorite show: {}", fav.title);
        refresh_library_shows_ui();
    }
}

pub fn on_remove_favorite_show(show_id: &str) {
    if let Err(e) = crate::db::repo::FavoritesRepo::remove_show(show_id) {
        log::error!("Failed to remove favorite show: {e}");
    } else {
        refresh_library_shows_ui();
    }
}

pub fn get_show_favorite_state(show_id: &str) -> bool {
    crate::db::repo::FavoritesRepo::is_favorite_show(show_id).unwrap_or(false)
}

pub fn get_track_favorite_state(track_id: &str) -> bool {
    crate::db::repo::FavoritesRepo::is_favorite_track(track_id).unwrap_or(false)
}

fn refresh_library_shows_ui() {
    let shows = crate::db::repo::FavoritesRepo::all_shows().unwrap_or_default();
    let bridge_shows: Vec<bridge::Show> = shows
        .into_iter()
        .map(|s| bridge::Show {
            id: s.id,
            title: s.title,
            author: s.author,
            description: s.description,
            thumbnail: s.thumbnail,
            episode_count: s.episode_count,
        })
        .collect();
    bridge::set_library_shows(bridge_shows);
}

pub fn on_add_to_playlist(track: bridge::Track, playlist_id: &str) {
    log::info!("Adding track {} to playlist {}", track.title, playlist_id);
    let tracks = crate::db::repo::PlaylistRepo::tracks(playlist_id).unwrap_or_default();
    let next_pos = tracks.len() as i32;
    let pt = crate::db::repo::PlaylistTrack {
        playlist_id: playlist_id.to_string(),
        track_id: track.id.clone(),
        position: next_pos,
        title: track.title,
        artist: track.artist,
        album: track.album,
        duration_ms: track.duration_ms,
        thumbnail: track.thumbnail,
        added_at: String::new(),
    };
    if let Err(e) = crate::db::repo::PlaylistRepo::add_track(playlist_id, &pt) {
        log::error!("Failed to add track to playlist: {e}");
    } else {
        push_context_and_library_playlists();
    }
}

pub fn on_create_playlist(name: &str, description: &str, privacy: &str) {
    let name = name.to_string();
    let description = description.to_string();
    let privacy = privacy.to_string();
    log::info!("Creating playlist: {name}");
    tokio::spawn(async move {
        let created_id = tokio::task::spawn_blocking({
            let name = name.clone();
            let description = description.clone();
            move || crate::db::repo::PlaylistRepo::create(&name, &description)
        })
        .await
        .ok()
        .and_then(|r| r.ok());
        if created_id.is_some() && crate::api::auth::is_authenticated() {
            if let Err(e) = crate::api::endpoints::create_playlist(&name, &description, &privacy).await {
                log::warn!("Failed to create playlist on remote server: {e}");
            }
        }

        push_context_and_library_playlists();
    });
}

pub fn on_rename_playlist(playlist_id: &str, name: &str) {
    let playlist_id = playlist_id.to_string();
    let name = name.to_string();
    log::info!("Renaming playlist {playlist_id} to {name}");
    let _ = crate::db::repo::PlaylistRepo::rename(&playlist_id, &name);
    push_context_and_library_playlists();
}

pub fn on_delete_playlist(playlist_id: &str) {
    let playlist_id = playlist_id.to_string();
    log::info!("Deleting playlist {playlist_id}");
    let _ = crate::db::repo::PlaylistRepo::delete(&playlist_id);
    push_context_and_library_playlists();
}

pub fn on_remove_playlist_track(playlist_id: &str, track_id: &str) {
    let playlist_id = playlist_id.to_string();
    let track_id = track_id.to_string();
    log::info!("Removing track {track_id} from playlist {playlist_id}");
    let _ = crate::db::repo::PlaylistRepo::remove_track(&playlist_id, &track_id);
    if let Some(detail) = load_local_playlist_detail(&playlist_id) {
        bridge::set_playlist_detail(detail.0, detail.1);
    }
    push_context_and_library_playlists();
}

pub fn on_move_playlist_track(playlist_id: &str, from: i32, to: i32) {
    let playlist_id = playlist_id.to_string();
    log::info!("Moving playlist track in {playlist_id}: {from} -> {to}");
    tokio::spawn(async move {
        let playlist_for_task = playlist_id.clone();
        let result = tokio::task::spawn_blocking(move || {
            crate::db::repo::PlaylistRepo::move_track(&playlist_for_task, from, to)
        })
        .await;

        match result {
            Ok(Ok(())) => {
                if let Some(detail) = load_local_playlist_detail(&playlist_id) {
                    bridge::set_playlist_detail(detail.0, detail.1);
                }
            }
            Ok(Err(e)) => log::error!("Failed to move playlist track: {e}"),
            Err(e) => log::error!("Playlist track move task failed: {e}"),
        }
    });
}

fn load_local_playlist_detail(
    playlist_id: &str,
) -> Option<(
    bridge::Playlist,
    Vec<bridge::Track>,
)> {
    let all = crate::db::repo::PlaylistRepo::all().ok()?;
    let p = all.into_iter().find(|pl| pl.id == playlist_id)?;
    let tracks = crate::db::repo::PlaylistRepo::tracks(playlist_id).ok()?;
    let count = tracks.len() as i32;
    let bp = bridge::Playlist {
        id: p.id.clone(),
        name: p.name.clone(),
        description: p.description.clone(),
        thumbnail: p.artwork.clone(),
        track_count: count,
        owner: String::new(),
        privacy: String::new(),
        editable: true,
    };
    let btracks: Vec<bridge::Track> = tracks
        .into_iter()
        .map(|t| bridge::Track {
            id: t.track_id,
            title: t.title,
            artist: t.artist,
            album: t.album,
            duration_ms: t.duration_ms,
            thumbnail: t.thumbnail,
        })
        .collect();
    Some((bp, btracks))
}

fn push_context_and_library_playlists() {
    if let Ok(playlists) = crate::db::repo::PlaylistRepo::all() {
        let p_list: Vec<bridge::Playlist> = playlists
            .iter()
            .map(|p| {
                let count = crate::db::repo::PlaylistRepo::tracks(&p.id)
                    .ok()
                    .map(|t| t.len() as i32)
                    .unwrap_or(0);
                bridge::Playlist {
                    id: p.id.clone(),
                    name: p.name.clone(),
                    description: p.description.clone(),
                    thumbnail: p.artwork.clone(),
                    track_count: count,
                    owner: String::new(),
                    privacy: String::new(),
                    editable: true,
                }
            })
            .collect();
        bridge::set_context_playlists(
            p_list
                .iter()
                .map(|p| bridge::Playlist {
                    id: p.id.clone(),
                    name: p.name.clone(),
                    description: p.description.clone(),
                    thumbnail: p.thumbnail.clone(),
                    track_count: p.track_count,
                    owner: String::new(),
                    privacy: String::new(),
                    editable: p.editable,
                })
                .collect(),
        );
        bridge::set_library_playlists(p_list);
        if let Ok(cache) = crate::services::library_cache::GLOBAL_LIBRARY_CACHE.write() {
            cache.invalidate("playlists");
        }
    }
}

fn load_downloaded_library_tab(tab: LibraryTab) {
    match tab {
        LibraryTab::Songs => {
            let tracks = if let Ok(dl_tracks) = crate::db::repo::DownloadsRepo::all() {
                dl_tracks
                    .into_iter()
                    .filter(|d| d.status == "completed")
                    .map(|d| bridge::Track {
                        id: d.video_id,
                        title: d.title,
                        artist: d.artist,
                        album: d.album,
                        duration_ms: d.duration_ms,
                        thumbnail: d.thumbnail_url,
                    })
                    .collect::<Vec<_>>()
            } else {
                Vec::new()
            };

            if let Ok(cache) = crate::services::library_cache::GLOBAL_LIBRARY_CACHE.write() {
                cache.set(
                    "songs",
                    crate::services::library_cache::CacheData {
                        songs: tracks.clone(),
                        albums: Vec::new(),
                        artists: Vec::new(),
                        playlists: Vec::new(),
                        source: crate::services::library_cache::LibrarySource::Downloaded,
                    },
                );
            }

            bridge::set_library_songs(tracks);
        }
        LibraryTab::Albums => {
            let mut seen_albums = std::collections::HashSet::new();
            let mut albums = Vec::new();
            if let Ok(dl_tracks) = crate::db::repo::DownloadsRepo::all() {
                for d in dl_tracks {
                    if d.status == "completed" && !d.album.is_empty() {
                        let album_id = d.parent_playlist_id.clone().unwrap_or_default();
                        if seen_albums.insert(d.album.clone()) {
                            albums.push(bridge::Album {
                                id: album_id,
                                title: d.album,
                                artist: d.artist,
                                year: String::new(),
                                thumbnail: d
                                    .parent_playlist_thumbnail_url
                                    .clone()
                                    .unwrap_or_default(),
                                track_count: 0,
                                artist_id: String::new(),
                            });
                        }
                    }
                }
            }

            if let Ok(cache) = crate::services::library_cache::GLOBAL_LIBRARY_CACHE.write() {
                cache.set(
                    "albums",
                    crate::services::library_cache::CacheData {
                        songs: Vec::new(),
                        albums: albums.clone(),
                        artists: Vec::new(),
                        playlists: Vec::new(),
                        source: crate::services::library_cache::LibrarySource::Downloaded,
                    },
                );
            }

            bridge::set_library_albums(albums);
        }
        LibraryTab::Artists => {
            let mut seen_artists = std::collections::HashSet::new();
            let mut artists = Vec::new();
            if let Ok(dl_tracks) = crate::db::repo::DownloadsRepo::all() {
                for d in dl_tracks {
                    if d.status == "completed" && !d.artist.is_empty() {
                        if seen_artists.insert(d.artist.clone()) {
                            artists.push(bridge::Artist {
                                id: String::new(),
                                name: d.artist,
                                thumbnail: String::new(),
                                description: String::new(),
                                subscribers: String::new(),
                            });
                        }
                    }
                }
            }

            if let Ok(cache) = crate::services::library_cache::GLOBAL_LIBRARY_CACHE.write() {
                cache.set(
                    "artists",
                    crate::services::library_cache::CacheData {
                        songs: Vec::new(),
                        albums: Vec::new(),
                        artists: artists.clone(),
                        playlists: Vec::new(),
                        source: crate::services::library_cache::LibrarySource::Downloaded,
                    },
                );
            }

            bridge::set_library_artists(artists);
        }
        LibraryTab::Playlists => {
            let mut seen_playlists = std::collections::HashSet::new();
            let mut playlists = Vec::new();
            if let Ok(dl_tracks) = crate::db::repo::DownloadsRepo::all() {
                for d in dl_tracks {
                    if d.status == "completed" {
                        if let Some(ref playlist_id) = d.parent_playlist_id {
                            if !playlist_id.is_empty() && playlist_id.starts_with("VL") {
                                if seen_playlists.insert(playlist_id.clone()) {
                                    playlists.push(bridge::Playlist {
                                        id: playlist_id.clone(),
                                        name: d.parent_playlist_title.clone().unwrap_or_default(),
                                        description: String::new(),
                                        thumbnail: d
                                            .parent_playlist_thumbnail_url
                                            .clone()
                                            .unwrap_or_default(),
                                        track_count: 0,
                                        owner: String::new(),
                                        privacy: String::new(),
                                        editable: false,
                                    });
                                }
                            }
                        }
                    }
                }
            }

            if let Ok(cache) = crate::services::library_cache::GLOBAL_LIBRARY_CACHE.write() {
                cache.set(
                    "playlists",
                    crate::services::library_cache::CacheData {
                        songs: Vec::new(),
                        albums: Vec::new(),
                        artists: Vec::new(),
                        playlists: playlists.clone(),
                        source: crate::services::library_cache::LibrarySource::Downloaded,
                    },
                );
            }

            bridge::set_library_playlists(playlists);
        }
        LibraryTab::Shows => refresh_library_shows_ui(),
    }
}

pub fn on_library_search(tab: &str, query: &str, sort_by: &str) {
    let tab = tab.to_string();
    let query = query.to_string();
    let sort_by = sort_by.to_string();

    tokio::spawn(async move {
        log::info!(
            "Library search: tab={}, query={}, sort_by={}",
            tab,
            query,
            sort_by
        );

        let source_filter = super::FILTER_SOURCE.load(Ordering::SeqCst);
        let lib_service = crate::services::library::LibraryService::new(true);

        match tab.as_str() {
            "songs" => {
                let mut songs = Vec::new();

                if source_filter == 0 || source_filter == 1 {
                    if let Ok(cache) = crate::services::library_cache::GLOBAL_LIBRARY_CACHE.read() {
                        if let Some(data) = cache.get("songs") {
                            songs.extend(data.songs);
                        }
                    }
                }

                if source_filter == 0 || source_filter == 3 {
                    if let Ok(local_tracks) = crate::db::repo::FavoritesRepo::all_tracks() {
                        for t in local_tracks {
                            if !songs.iter().any(|s| s.id == t.id) {
                                songs.push(bridge::Track {
                                    id: t.id,
                                    title: t.title,
                                    artist: t.artist,
                                    album: t.album,
                                    duration_ms: t.duration_ms,
                                    thumbnail: t.thumbnail,
                                });
                            }
                        }
                    }
                }

                if source_filter == 0 || source_filter == 2 {
                    if let Ok(dl_tracks) = crate::db::repo::DownloadsRepo::all() {
                        for d in dl_tracks {
                            if d.status == "completed" && !songs.iter().any(|s| s.id == d.video_id)
                            {
                                songs.push(bridge::Track {
                                    id: d.video_id,
                                    title: d.title,
                                    artist: d.artist,
                                    album: d.album,
                                    duration_ms: d.duration_ms,
                                    thumbnail: d.thumbnail_url,
                                });
                            }
                        }
                    }
                }

                let filtered =
                    crate::services::library_cache::LibraryCache::search_tracks(&songs, &query);
                let sorted = lib_service.sort_songs("songs", filtered, &sort_by);
                bridge::set_library_songs(sorted);
            }
            "albums" => {
                let mut albums = Vec::new();

                if source_filter == 0 || source_filter == 1 {
                    if let Ok(cache) = crate::services::library_cache::GLOBAL_LIBRARY_CACHE.read() {
                        if let Some(data) = cache.get("albums") {
                            albums.extend(data.albums);
                        }
                    }
                }

                if source_filter == 0 || source_filter == 3 {
                    if let Ok(local_albums) = crate::db::repo::FavoritesRepo::all_albums() {
                        for a in local_albums {
                            if !albums.iter().any(|x| x.id == a.id) {
                                albums.push(bridge::Album {
                                    id: a.id,
                                    title: a.title,
                                    artist: a.artist,
                                    year: a.year.map(|y| y.to_string()).unwrap_or_default(),
                                    thumbnail: a.thumbnail,
                                    track_count: 0,
                                    artist_id: String::new(),
                                });
                            }
                        }
                    }
                }

                if source_filter == 0 || source_filter == 2 {
                    let mut seen_albums = std::collections::HashSet::new();
                    if let Ok(dl_tracks) = crate::db::repo::DownloadsRepo::all() {
                        for d in dl_tracks {
                            if d.status == "completed" && !d.album.is_empty() {
                                let album_id = d.parent_playlist_id.clone().unwrap_or_default();
                                if seen_albums.insert(d.album.clone())
                                    && !albums.iter().any(|x| x.title == d.album)
                                {
                                    albums.push(bridge::Album {
                                        id: album_id,
                                        title: d.album,
                                        artist: d.artist,
                                        year: String::new(),
                                        thumbnail: d
                                            .parent_playlist_thumbnail_url
                                            .clone()
                                            .unwrap_or_default(),
                                        track_count: 0,
                                        artist_id: String::new(),
                                    });
                                }
                            }
                        }
                    }
                }

                let filtered =
                    crate::services::library_cache::LibraryCache::search_albums(&albums, &query);
                let sorted = lib_service.sort_albums("albums", filtered, &sort_by);
                bridge::set_library_albums(sorted);
            }
            "artists" => {
                let mut artists = Vec::new();

                if source_filter == 0 || source_filter == 1 {
                    if let Ok(cache) = crate::services::library_cache::GLOBAL_LIBRARY_CACHE.read() {
                        if let Some(data) = cache.get("artists") {
                            artists.extend(data.artists);
                        }
                    }
                }

                if source_filter == 0 || source_filter == 3 {
                    if let Ok(local_artists) = crate::db::repo::FavoritesRepo::all_artists() {
                        for a in local_artists {
                            if !artists.iter().any(|x| x.id == a.id) {
                                artists.push(bridge::Artist {
                                    id: a.id,
                                    name: a.name,
                                    thumbnail: a.thumbnail,
                                    description: String::new(),
                                    subscribers: String::new(),
                                });
                            }
                        }
                    }
                }

                if source_filter == 0 || source_filter == 2 {
                    let mut seen_artists = std::collections::HashSet::new();
                    if let Ok(dl_tracks) = crate::db::repo::DownloadsRepo::all() {
                        for d in dl_tracks {
                            if d.status == "completed" && !d.artist.is_empty() {
                                if seen_artists.insert(d.artist.clone())
                                    && !artists.iter().any(|x| x.name == d.artist)
                                {
                                    artists.push(bridge::Artist {
                                        id: String::new(),
                                        name: d.artist,
                                        thumbnail: String::new(),
                                        description: String::new(),
                                        subscribers: String::new(),
                                    });
                                }
                            }
                        }
                    }
                }

                let filtered =
                    crate::services::library_cache::LibraryCache::search_artists(&artists, &query);
                let sorted = lib_service.sort_artists("artists", filtered, &sort_by);
                bridge::set_library_artists(sorted);
            }
            "playlists" => {
                let mut playlists = Vec::new();

                if source_filter == 0 || source_filter == 1 {
                    if let Ok(cache) = crate::services::library_cache::GLOBAL_LIBRARY_CACHE.read() {
                        if let Some(data) = cache.get("playlists") {
                            playlists.extend(data.playlists);
                        }
                    }
                }

                if source_filter == 0 || source_filter == 3 {
                    if let Ok(local_playlists) = crate::db::repo::PlaylistRepo::all() {
                        for p in local_playlists {
                            if !playlists.iter().any(|x| x.id == p.id) {
                                let count = crate::db::repo::PlaylistRepo::tracks(&p.id)
                                    .ok()
                                    .map(|t| t.len() as i32)
                                    .unwrap_or(0);
                                playlists.push(bridge::Playlist {
                                    id: p.id,
                                    name: p.name,
                                    description: p.description,
                                    thumbnail: p.artwork,
                                    track_count: count,
                                    owner: String::new(),
                                    privacy: String::new(),
                                    editable: true,
                                });
                            }
                        }
                    }
                }

                if source_filter == 0 || source_filter == 2 {
                    let mut seen_playlists = std::collections::HashSet::new();
                    if let Ok(dl_tracks) = crate::db::repo::DownloadsRepo::all() {
                        for d in dl_tracks {
                            if d.status == "completed" {
                                if let Some(ref playlist_id) = d.parent_playlist_id {
                                    if !playlist_id.is_empty() && playlist_id.starts_with("VL") {
                                        if seen_playlists.insert(playlist_id.clone())
                                            && !playlists.iter().any(|x| x.id == *playlist_id)
                                        {
                                            playlists.push(bridge::Playlist {
                                                id: playlist_id.clone(),
                                                name: d
                                                    .parent_playlist_title
                                                    .clone()
                                                    .unwrap_or_default(),
                                                description: String::new(),
                                                thumbnail: d
                                                    .parent_playlist_thumbnail_url
                                                    .clone()
                                                    .unwrap_or_default(),
                                                track_count: 0,
                                                owner: String::new(),
                                                privacy: String::new(),
                                                editable: false,
                                            });
                                        }
                                    }
                                }
                            }
                        }
                    }
                }

                let filtered = crate::services::library_cache::LibraryCache::search_playlists(
                    &playlists, &query,
                );
                let sorted = lib_service.sort_playlists("playlists", filtered, &sort_by);
                bridge::set_library_playlists(sorted);
            }
            "shows" => {
                if let Ok(local_shows) = crate::db::repo::FavoritesRepo::all_shows() {
                    let mut shows: Vec<bridge::Show> = local_shows
                        .iter()
                        .map(|s| bridge::Show {
                            id: s.id.clone(),
                            title: s.title.clone(),
                            author: s.author.clone(),
                            thumbnail: s.thumbnail.clone(),
                            episode_count: s.episode_count as i32,
                            description: String::new(),
                        })
                        .collect();

                    if !query.is_empty() {
                        let query_lower = query.to_lowercase();
                        shows.retain(|s| {
                            s.title.to_lowercase().contains(&query_lower)
                                || s.author.to_lowercase().contains(&query_lower)
                        });
                    }

                    if sort_by == "title" || sort_by == "name_asc" {
                        shows.sort_by(|a, b| a.title.cmp(&b.title));
                    }

                    bridge::set_library_shows(shows);
                }
            }
            _ => {}
        }
    });
}

pub fn on_library_invalidate_cache(tab: &str) {
    let tab = tab.to_string();
    tokio::spawn(async move {
        use crate::services::library::LibraryService;
        let lib = LibraryService::new(true);
        lib.invalidate_cache(&tab).await;
    });
}

pub fn on_library_set_filter_source(source: i32) {
    super::FILTER_SOURCE.store(source, Ordering::SeqCst);
    log::info!("Filter library by source: {}", source);
}

pub fn get_album_favorite_state(album_id: &str) -> bool {
    use crate::db::repo::FavoritesRepo;
    FavoritesRepo::is_favorite_album(album_id).unwrap_or(false)
}

pub fn get_artist_favorite_state(artist_id: &str) -> bool {
    use crate::db::repo::FavoritesRepo;
    FavoritesRepo::is_favorite_artist(artist_id).unwrap_or(false)
}

pub fn on_update_playlist_privacy(playlist_id: &str, privacy: &str) {
    let playlist_id = playlist_id.to_string();
    let privacy = privacy.to_string();

    tokio::spawn(async move {
        use crate::db::repo::PlaylistRepo;
        if let Ok(mut playlist) = PlaylistRepo::get(&playlist_id) {
            playlist.privacy = if privacy == "public" { 0 } else { 1 };
            if PlaylistRepo::update(&playlist).is_ok() {
                log::info!("Playlist {} privacy updated to {}", playlist_id, privacy);
            }
        }
    });
}

pub fn on_playlist_load_continuations(playlist_id: &str) {
    let playlist_id = playlist_id.to_string();

    tokio::spawn(async move {
        use crate::api::client::ApiClient;
        let _api = ApiClient::new();
        log::info!("Loading continuations for playlist: {}", playlist_id);
    });
}
