use super::models::*;

#[derive(Clone)]
pub struct ApiClient;

impl ApiClient {
    pub fn new() -> Self { Self }

    pub fn search(&self, query: &str, filter: &str) -> SearchResults {
        if query.trim().is_empty() {
            return SearchResults {
                query: query.to_string(),
                songs: vec![],
                videos: vec![],
                albums: vec![],
                artists: vec![],
                playlists: vec![],
            };
        }
        match super::innertube::search(query, filter) {
            Ok(results) => {
                log::info!("Search (real): '{query}' — {} songs, {} albums, {} artists",
                    results.songs.len(), results.albums.len(), results.artists.len());
                results
            }
            Err(e) => {
                log::warn!("Search API failed, using mock: {e}");
                self.mock_search(query)
            }
        }
    }

    pub fn home_sections(&self) -> Vec<HomeSection> {
        match super::innertube::home_sections() {
            Ok(sections) => {
                log::info!("Home feed (real): {} sections", sections.len());
                sections
            }
            Err(e) => {
                log::warn!("Home feed API failed, using mock: {e}");
                self.mock_home()
            }
        }
    }

    pub fn get_stream_url(&self, video_id: &str) -> Option<String> {
        let url = super::innertube::get_stream_url(video_id);
        if url.is_some() {
            log::info!("Stream URL resolved for {video_id}");
        } else {
            log::warn!("Failed to resolve stream URL for {video_id}");
        }
        url
    }

    pub async fn get_stream_url_async(&self, video_id: &str) -> Option<String> {
        let url = super::innertube::get_stream_url_async(video_id).await;
        if url.is_some() {
            log::info!("Stream URL resolved async for {video_id}");
        } else {
            log::warn!("Failed to resolve stream URL async for {video_id}");
        }
        url
    }

    fn mock_search(&self, query: &str) -> SearchResults {
        log::info!("Search (mock fallback): '{query}'");
        SearchResults {
            query: query.to_string(),
            songs: vec![
                Track { id: "s1".into(), title: format!("{query} — Remix"), artists: vec!["Artista A".into()], album: Some("Álbum 1".into()), album_id: None, duration_ms: 234000, thumbnail: "".into(), stream_url: None },
                Track { id: "s2".into(), title: format!("{query} (En Vivo)"), artists: vec!["Artista B".into()], album: Some("Álbum 2".into()), album_id: None, duration_ms: 312000, thumbnail: "".into(), stream_url: None },
                Track { id: "s3".into(), title: format!("{query} — Acústico"), artists: vec!["Artista C".into()], album: None, album_id: None, duration_ms: 198000, thumbnail: "".into(), stream_url: None },
            ],
            videos: vec![],
            albums: vec![
                Album { id: "a1".into(), title: format!("{query} (Álbum)"), artists: vec!["Artista A".into()], year: Some(2024), thumbnail: "".into(), track_count: Some(12) },
            ],
            artists: vec![
                Artist { id: "ar1".into(), name: format!("{query} (Artista)"), thumbnail: "".into(), subscriber_count: Some("1.2M".into()) },
            ],
            playlists: vec![
                Playlist { id: "p1".into(), title: format!("Lo mejor de {query}"), description: None, owner: Some("Usuario".into()), thumbnail: "".into(), track_count: Some(50) },
            ],
        }
    }

    fn mock_home(&self) -> Vec<HomeSection> {
        log::info!("Home feed (mock fallback)");
        vec![
            HomeSection {
                title: "Seguir escuchando".into(),
                browse_id: None,
                items: vec![
                    HomeItem { title: "Canción A".into(), subtitle: "Artista 1".into(), thumbnail: "".into(), item_type: "song".into(), browse_id: None, playlist_id: None },
                    HomeItem { title: "Canción B".into(), subtitle: "Artista 2".into(), thumbnail: "".into(), item_type: "song".into(), browse_id: None, playlist_id: None },
                ],
            },
            HomeSection {
                title: "Recomendados para ti".into(),
                browse_id: None,
                items: vec![
                    HomeItem { title: "Álbum X".into(), subtitle: "Artista X".into(), thumbnail: "".into(), item_type: "album".into(), browse_id: Some("b1".into()), playlist_id: None },
                    HomeItem { title: "Mix del Día".into(), subtitle: "Selección personalizada".into(), thumbnail: "".into(), item_type: "playlist".into(), browse_id: None, playlist_id: Some("pl1".into()) },
                ],
            },
            HomeSection {
                title: "Novedades".into(),
                browse_id: None,
                items: vec![
                    HomeItem { title: "Nuevo lanzamiento 1".into(), subtitle: "Artista Y".into(), thumbnail: "".into(), item_type: "album".into(), browse_id: Some("b2".into()), playlist_id: None },
                    HomeItem { title: "Nuevo lanzamiento 2".into(), subtitle: "Artista Z".into(), thumbnail: "".into(), item_type: "album".into(), browse_id: Some("b3".into()), playlist_id: None },
                ],
            },
        ]
    }
}

impl Default for ApiClient {
    fn default() -> Self { Self }
}
