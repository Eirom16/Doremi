use crate::api::client::ApiClient;
use crate::api::models::SearchResults;

pub struct SearchService {
    api: ApiClient,
}

impl SearchService {
    pub fn new() -> Self {
        Self { api: ApiClient::new() }
    }

    pub fn search(&self, query: &str) -> SearchResults {
        self.api.search(query, "songs")
    }

    pub fn push_to_ui(&self, results: &SearchResults) {
        let songs: Vec<String> = results.songs.iter()
            .map(|t| format!("{} — {}\x1f{}", t.title, t.artists.join(", "), t.id))
            .collect();
        let artists: Vec<String> = results.artists.iter()
            .map(|a| a.name.clone())
            .collect();
        let albums: Vec<String> = results.albums.iter()
            .map(|a| a.title.clone())
            .collect();

        crate::bridge::bridge::set_search_results(songs, artists, albums);
    }
}

impl Default for SearchService {
    fn default() -> Self {
        Self::new()
    }
}
