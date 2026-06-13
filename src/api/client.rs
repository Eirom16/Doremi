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

    pub fn home_sections(&self) -> Vec<HomeSection> {
        match super::innertube::home_sections() {
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
    fn default() -> Self { Self }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_real_search() {
        let client = ApiClient::new();
        let results = client.search("Michael Jackson", "all");
        assert!(!results.songs.is_empty(), "Should return real search results");
        assert!(
            results.songs.iter().any(|s| {
                s.title.to_lowercase().contains("michael")
                    || s.artists.iter().any(|a| a.to_lowercase().contains("michael"))
            }),
            "Expected Michael Jackson in search results, got: {:?}",
            results.songs
        );
    }
}
