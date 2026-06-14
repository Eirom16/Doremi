use crate::api::client::ApiClient;

pub struct LibraryService {
    api: ApiClient,
}

impl LibraryService {
    pub fn new() -> Self {
        Self {
            api: ApiClient::new(),
        }
    }

    pub async fn load_songs(&self) {
        let remote_songs = self.api.library_songs().await;
        let songs: Vec<crate::bridge::bridge::Track> = remote_songs
            .into_iter()
            .map(|t| crate::bridge::bridge::Track {
                id: t.id,
                title: t.title,
                artist: t.artists.join(", "),
                album: t.album.unwrap_or_default(),
                duration_ms: t.duration_ms,
                thumbnail: t.thumbnail,
            })
            .collect();
        crate::bridge::bridge::set_library_songs(songs);
    }

    pub async fn load_playlists(&self) {
        let remote_playlists = self.api.library_playlists().await;
        let playlists: Vec<crate::bridge::bridge::Playlist> = remote_playlists
            .into_iter()
            .map(|p| crate::bridge::bridge::Playlist {
                id: p.id,
                name: p.title,
                description: p.description.unwrap_or_default(),
                thumbnail: p.thumbnail,
                track_count: p.track_count.unwrap_or_default(),
            })
            .collect();
        crate::bridge::bridge::set_library_playlists(playlists);
    }

    pub async fn load_albums(&self) {
        let remote_albums = self.api.library_albums().await;
        let albums: Vec<crate::bridge::bridge::Album> = remote_albums
            .into_iter()
            .map(|a| crate::bridge::bridge::Album {
                id: a.id,
                title: a.title,
                artist: a.artists.join(", "),
                year: a.year.map(|y| y.to_string()).unwrap_or_default(),
                thumbnail: a.thumbnail,
                track_count: a.track_count.unwrap_or_default(),
            })
            .collect();
        crate::bridge::bridge::set_library_albums(albums);
    }

    pub async fn load_artists(&self) {
        let remote_artists = self.api.library_artists().await;
        let artists: Vec<crate::bridge::bridge::Artist> = remote_artists
            .into_iter()
            .map(|a| crate::bridge::bridge::Artist {
                id: a.id,
                name: a.name,
                thumbnail: a.thumbnail,
                description: String::new(),
                subscribers: a.subscriber_count.unwrap_or_default(),
            })
            .collect();
        crate::bridge::bridge::set_library_artists(artists);
    }
}

impl Default for LibraryService {
    fn default() -> Self {
        Self::new()
    }
}
