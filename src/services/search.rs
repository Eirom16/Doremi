use crate::api::client::ApiClient;
use crate::api::models::SearchResults;

pub struct SearchService {
    api: ApiClient,
}

impl SearchService {
    pub fn new() -> Self {
        Self {
            api: ApiClient::new(),
        }
    }

    pub async fn search(&self, query: &str, filter: &str) -> SearchResults {
        self.api.search(query, filter).await
    }

    pub async fn suggestions(&self, query: &str) -> Vec<String> {
        self.api.search_suggestions(query).await
    }

    pub fn push_to_ui(&self, results: &SearchResults) {
        let songs: Vec<crate::bridge::bridge::Track> = results
            .songs
            .iter()
            .map(|t| crate::bridge::bridge::Track {
                id: t.id.clone(),
                title: t.title.clone(),
                artist: t.artists.join(", "),
                album: t.album.clone().unwrap_or_default(),
                duration_ms: t.duration_ms,
                thumbnail: t.thumbnail.clone(),
            })
            .collect();
        let videos = results
            .videos
            .iter()
            .map(|track| crate::bridge::bridge::Track {
                id: track.id.clone(),
                title: track.title.clone(),
                artist: track.artists.join(", "),
                album: track.album.clone().unwrap_or_default(),
                duration_ms: track.duration_ms,
                thumbnail: track.thumbnail.clone(),
            })
            .collect();
        let artists: Vec<crate::bridge::bridge::Artist> = results
            .artists
            .iter()
            .map(|a| crate::bridge::bridge::Artist {
                id: a.id.clone(),
                name: a.name.clone(),
                thumbnail: a.thumbnail.clone(),
                description: String::new(),
                subscribers: a.subscriber_count.clone().unwrap_or_default(),
            })
            .collect();
        let albums: Vec<crate::bridge::bridge::Album> = results
            .albums
            .iter()
            .map(|a| crate::bridge::bridge::Album {
                id: a.id.clone(),
                title: a.title.clone(),
                artist: a.artists.join(", "),
                year: a.year.map(|y| y.to_string()).unwrap_or_default(),
                thumbnail: a.thumbnail.clone(),
                track_count: a.track_count.unwrap_or(0),
                artist_id: a.artist_id.clone().unwrap_or_default(),
            })
            .collect();
        let playlists: Vec<crate::bridge::bridge::Playlist> = results
            .playlists
            .iter()
            .map(|playlist| crate::bridge::bridge::Playlist {
                id: playlist.id.clone(),
                name: playlist.title.clone(),
                description: playlist.description.clone().unwrap_or_default(),
                thumbnail: playlist.thumbnail.clone(),
                track_count: playlist.track_count.unwrap_or_default(),
            })
            .collect();

        let (top_result, has_top_result) = match &results.top_result {
            Some(item) => {
                let ffi_top = match item {
                    crate::api::models::TopResultItem::Track(t) => {
                        crate::bridge::bridge::TopResult {
                            id: t.id.clone(),
                            title: t.title.clone(),
                            subtitle: t.artists.join(", "),
                            thumbnail: t.thumbnail.clone(),
                            item_type: "song".to_string(),
                        }
                    }
                    crate::api::models::TopResultItem::Artist(a) => {
                        crate::bridge::bridge::TopResult {
                            id: a.id.clone(),
                            title: a.name.clone(),
                            subtitle: a.subscriber_count.clone().unwrap_or_default(),
                            thumbnail: a.thumbnail.clone(),
                            item_type: "artist".to_string(),
                        }
                    }
                    crate::api::models::TopResultItem::Album(al) => {
                        crate::bridge::bridge::TopResult {
                            id: al.id.clone(),
                            title: al.title.clone(),
                            subtitle: al.artists.join(", "),
                            thumbnail: al.thumbnail.clone(),
                            item_type: "album".to_string(),
                        }
                    }
                    crate::api::models::TopResultItem::Playlist(p) => {
                        crate::bridge::bridge::TopResult {
                            id: p.id.clone(),
                            title: p.title.clone(),
                            subtitle: p.owner.clone().unwrap_or_default(),
                            thumbnail: p.thumbnail.clone(),
                            item_type: "playlist".to_string(),
                        }
                    }
                };
                (ffi_top, true)
            }
            None => {
                (
                    crate::bridge::bridge::TopResult {
                        id: String::new(),
                        title: String::new(),
                        subtitle: String::new(),
                        thumbnail: String::new(),
                        item_type: String::new(),
                    },
                    false,
                )
            }
        };

        crate::bridge::bridge::set_search_results(
            top_result,
            has_top_result,
            songs,
            videos,
            artists,
            albums,
            playlists,
        );
    }
}

impl Default for SearchService {
    fn default() -> Self {
        Self::new()
    }
}
