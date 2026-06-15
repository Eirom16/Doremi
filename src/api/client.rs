use super::models::*;

#[derive(Clone)]
pub struct ApiClient;

impl ApiClient {
    pub fn new() -> Self {
        Self
    }

    pub async fn search(&self, query: &str, filter: &str) -> SearchResults {
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
        match super::innertube::search(query, filter).await {
            Ok(results) => {
                log::info!(
                    "Search (real): '{query}' — {} songs, {} albums, {} artists",
                    results.songs.len(),
                    results.albums.len(),
                    results.artists.len()
                );
                results
            }
            Err(e) => {
                log::error!("Search API failed: {e}");
                crate::bridge::bridge::show_notification(
                    &format!("Error de red al buscar: {e}"),
                    "error",
                );
                SearchResults {
                    query: query.to_string(),
                    songs: vec![],
                    videos: vec![],
                    albums: vec![],
                    artists: vec![],
                    playlists: vec![],
                }
            }
        }
    }

    pub async fn search_suggestions(&self, query: &str) -> Vec<String> {
        match super::innertube::search_suggestions(query).await {
            Ok(suggestions) => suggestions,
            Err(error) => {
                log::debug!("Search suggestions failed: {error}");
                Vec::new()
            }
        }
    }

    pub async fn home_sections(&self) -> Vec<HomeSection> {
        match super::innertube::home_sections().await {
            Ok(sections) => {
                log::info!("Home feed (real): {} sections", sections.len());
                sections
            }
            Err(e) => {
                log::error!("Home feed API failed: {e}");
                crate::bridge::bridge::show_notification(
                    &format!("Error al cargar inicio: {e}"),
                    "error",
                );
                vec![]
            }
        }
    }

    pub async fn charts(&self) -> Vec<HomeSection> {
        match super::innertube::charts().await {
            Ok(sections) => sections,
            Err(error) => {
                log::error!("Charts API failed: {error}");
                crate::bridge::bridge::show_notification(
                    &format!("Error al cargar tendencias: {error}"),
                    "error",
                );
                Vec::new()
            }
        }
    }

    pub async fn library_playlists(&self) -> Vec<Playlist> {
        match super::innertube::library_playlists().await {
            Ok(playlists) => playlists,
            Err(e) => {
                log::error!("Library playlists API failed: {e}");
                crate::bridge::bridge::show_notification(
                    &format!("Error al cargar playlists de biblioteca: {e}"),
                    "error",
                );
                Vec::new()
            }
        }
    }

    pub async fn library_songs(&self) -> Vec<Track> {
        match super::innertube::library_songs(None).await {
            Ok(songs) => songs,
            Err(e) => {
                log::error!("Library songs API failed: {e}");
                crate::bridge::bridge::show_notification(
                    &format!("Error al cargar canciones de biblioteca: {e}"),
                    "error",
                );
                Vec::new()
            }
        }
    }

    pub async fn library_albums(&self) -> Vec<Album> {
        match super::innertube::library_albums().await {
            Ok(albums) => albums,
            Err(e) => {
                log::error!("Library albums API failed: {e}");
                crate::bridge::bridge::show_notification(
                    &format!("Error al cargar álbumes de biblioteca: {e}"),
                    "error",
                );
                Vec::new()
            }
        }
    }

    pub async fn library_artists(&self) -> Vec<Artist> {
        match super::innertube::library_artists().await {
            Ok(artists) => artists,
            Err(e) => {
                log::error!("Library artists API failed: {e}");
                crate::bridge::bridge::show_notification(
                    &format!("Error al cargar artistas de biblioteca: {e}"),
                    "error",
                );
                Vec::new()
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
}

impl Default for ApiClient {
    fn default() -> Self {
        Self
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[tokio::test]
    async fn test_real_search() {
        let _ = env_logger::builder().is_test(true).try_init();
        let client = ApiClient::new();
        let results = client.search("Michael Jackson", "all").await;
        assert!(
            !results.songs.is_empty(),
            "Should return real search results"
        );
        assert!(
            results.songs.iter().any(|s| {
                s.title.to_lowercase().contains("michael")
                    || s.artists
                        .iter()
                        .any(|a| a.to_lowercase().contains("michael"))
            }),
            "Expected Michael Jackson in search results, got: {:?}",
            results.songs
        );

        let song_results = client.search("Thriller", "songs").await;
        assert!(
            !song_results.songs.is_empty(),
            "Should return songs when filtering by songs"
        );
        assert!(
            song_results.albums.is_empty(),
            "Should not return albums when filtering by songs"
        );
        assert!(
            song_results.artists.is_empty(),
            "Should not return artists when filtering by songs"
        );
    }
}
