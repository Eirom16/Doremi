use super::models::*;

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum ApiErrorKind {
    AuthExpired,
    InvalidRequest,
    SchemaChanged,
    Network,
    Empty,
    Unknown,
}

impl ApiErrorKind {
    pub fn as_str(self) -> &'static str {
        match self {
            Self::AuthExpired => "not_authenticated",
            Self::InvalidRequest => "invalid_request",
            Self::SchemaChanged => "schema_changed",
            Self::Network => "network",
            Self::Empty => "empty",
            Self::Unknown => "request_failed",
        }
    }
}

pub fn classify_error(error: &str) -> ApiErrorKind {
    if error.contains("401 Unauthorized") || error.contains("403 Forbidden") {
        ApiErrorKind::AuthExpired
    } else if error.contains("400 Bad Request") || error.contains("INVALID_ARGUMENT") {
        ApiErrorKind::InvalidRequest
    } else if error.contains("schema changed") {
        ApiErrorKind::SchemaChanged
    } else if error.contains("transport error") || error.contains("timed out") {
        ApiErrorKind::Network
    } else if error.contains("empty") || error.contains("no header") {
        ApiErrorKind::Empty
    } else {
        ApiErrorKind::Unknown
    }
}

pub fn friendly_error(context: &str, error: &str) -> String {
    match classify_error(error) {
        ApiErrorKind::AuthExpired => "Tu sesión de YouTube Music expiró.".to_string(),
        ApiErrorKind::InvalidRequest if context == "library_albums" => {
            "No se pudieron cargar los álbumes de tu biblioteca. YouTube Music rechazó la solicitud.".to_string()
        }
        ApiErrorKind::InvalidRequest => {
            "No se pudo cargar este contenido. YouTube Music rechazó la solicitud.".to_string()
        }
        ApiErrorKind::SchemaChanged if context == "playlist" => {
            "No se pudo abrir la playlist. El formato de YouTube Music cambió.".to_string()
        }
        ApiErrorKind::SchemaChanged => {
            "No se pudo cargar este contenido. El formato de YouTube Music cambió.".to_string()
        }
        ApiErrorKind::Network => "No hay conexión estable con YouTube Music.".to_string(),
        ApiErrorKind::Empty => "No hay contenido disponible para mostrar.".to_string(),
        ApiErrorKind::Unknown => "No se pudo completar la solicitud a YouTube Music.".to_string(),
    }
}

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
                top_result: None,
                songs: vec![],
                videos: vec![],
                albums: vec![],
                artists: vec![],
                playlists: vec![],
                shows: vec![],
                episodes: vec![],
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
                    top_result: None,
                    songs: vec![],
                    videos: vec![],
                    albums: vec![],
                    artists: vec![],
                    playlists: vec![],
                    shows: vec![],
                    episodes: vec![],
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

    pub async fn home_sections(&self) -> Result<Vec<HomeSection>, String> {
        let sections = super::innertube::home_sections().await?;
        log::info!("Home feed (real): {} sections", sections.len());
        Ok(sections)
    }

    pub async fn home_sections_page(
        &self,
        continuation: Option<&str>,
    ) -> Result<(Vec<HomeSection>, Option<String>), String> {
        super::innertube::home_sections_page(continuation).await
    }

    pub async fn charts(&self) -> Result<Vec<HomeSection>, String> {
        super::innertube::charts().await
    }

    pub async fn library_playlists(&self) -> Vec<Playlist> {
        match super::innertube::library_playlists().await {
            Ok(playlists) => playlists,
            Err(e) => {
                log::error!(
                    "Library playlists API failed ({}): {e}",
                    classify_error(&e).as_str()
                );
                crate::bridge::bridge::show_notification(
                    &friendly_error("library_playlists", &e),
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
                log::error!(
                    "Library songs API failed ({}): {e}",
                    classify_error(&e).as_str()
                );
                crate::bridge::bridge::show_notification(
                    &friendly_error("library_songs", &e),
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
                log::error!(
                    "Library albums API failed ({}): {e}",
                    classify_error(&e).as_str()
                );
                crate::bridge::bridge::show_notification(
                    &friendly_error("library_albums", &e),
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
                log::error!(
                    "Library artists API failed ({}): {e}",
                    classify_error(&e).as_str()
                );
                crate::bridge::bridge::show_notification(
                    &friendly_error("library_artists", &e),
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

    #[test]
    fn classifies_library_request_errors() {
        let invalid = "Innertube endpoint browse returned 400 Bad Request: INVALID_ARGUMENT";
        assert_eq!(classify_error(invalid), ApiErrorKind::InvalidRequest);
        assert_eq!(
            friendly_error("library_albums", invalid),
            "No se pudieron cargar los álbumes de tu biblioteca. YouTube Music rechazó la solicitud."
        );
        let schema = "Innertube browse/playlist schema changed: expected header";
        assert_eq!(classify_error(schema), ApiErrorKind::SchemaChanged);
        assert_eq!(
            friendly_error("playlist", schema),
            "No se pudo abrir la playlist. El formato de YouTube Music cambió."
        );
    }

    /// Integration test — requires valid YouTube Music credentials and internet.
    /// Run explicitly with: cargo test -- --ignored test_real_search --nocapture
    #[tokio::test]
    #[ignore = "requires valid YouTube Music credentials and internet access"]
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

    /// Integration test — requires authenticated YouTube Music session.
    /// Run explicitly with: cargo test -- --ignored test_real_library_albums --nocapture
    #[tokio::test]
    #[ignore = "requires valid YouTube Music credentials and internet access"]
    async fn test_real_library_albums() {
        let _ = env_logger::builder().is_test(true).try_init();
        let body = serde_json::json!({
            "browseId": "FEmusic_liked_albums",
            "context": {
                "client": {
                    "clientName": "WEB_REMIX",
                    "clientVersion": "1.20230522.01.00",
                    "hl": "es"
                }
            }
        });
        let results = crate::api::transport::post("browse", body).await;
        if let Ok(json) = results {
            println!("Real library albums RAW JSON: {}", serde_json::to_string_pretty(&json).unwrap_or_default());
        }
        let parsed = crate::api::endpoints::library_albums().await;
        println!("Real library albums parsed: {:?}", parsed);
    }

    #[tokio::test]
    #[ignore = "requires valid YouTube Music credentials and internet access"]
    async fn test_real_mix_detail() {
        let _ = env_logger::builder().is_test(true).try_init();
        let client = ApiClient::new();
        let home = client.home_sections().await.unwrap();
        
        let mut mix_id = None;
        for section in &home {
            for item in &section.items {
                if item.playlist_id.is_some() && item.title.to_lowercase().contains("mix") {
                    mix_id = item.playlist_id.clone();
                    println!("Found mix: {} with id {:?}", item.title, mix_id);
                    break;
                }
            }
            if mix_id.is_some() { break; }
        }
        
        if let Some(id) = mix_id {
            let detail = crate::api::endpoints::playlist_detail(&id).await;
            println!("Mix detail result: {:?}", detail.is_ok());
            if let Err(e) = detail {
                println!("Error was: {}", e);
            }
        } else {
            println!("No mix found in home feed");
        }
    }
}
