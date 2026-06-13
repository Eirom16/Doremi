use serde::{Deserialize, Serialize};

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct LibraryItem {
    pub id: String,
    pub title: String,
    pub subtitle: String,
    pub item_type: String,
}

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct PlaylistSummary {
    pub id: String,
    pub name: String,
    pub song_count: u32,
}

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct LibraryData {
    pub songs: Vec<LibraryItem>,
    pub playlists: Vec<PlaylistSummary>,
}

impl LibraryData {
    pub fn mock() -> Self {
        Self {
            songs: vec![
                LibraryItem { id: "s1".into(), title: "Bohemian Rhapsody".into(), subtitle: "Queen".into(), item_type: "song".into() },
                LibraryItem { id: "s2".into(), title: "Stairway to Heaven".into(), subtitle: "Led Zeppelin".into(), item_type: "song".into() },
                LibraryItem { id: "s3".into(), title: "Hotel California".into(), subtitle: "Eagles".into(), item_type: "song".into() },
                LibraryItem { id: "s4".into(), title: "Imagine".into(), subtitle: "John Lennon".into(), item_type: "song".into() },
                LibraryItem { id: "s5".into(), title: "Smells Like Teen Spirit".into(), subtitle: "Nirvana".into(), item_type: "song".into() },
            ],
            playlists: vec![
                PlaylistSummary { id: "p1".into(), name: "Favoritos".into(), song_count: 24 },
                PlaylistSummary { id: "p2".into(), name: "Para el gym".into(), song_count: 18 },
                PlaylistSummary { id: "p3".into(), name: "Relax".into(), song_count: 12 },
                PlaylistSummary { id: "p4".into(), name: "Descubrimientos".into(), song_count: 8 },
            ],
        }
    }
}

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct Track {
    pub id: String,
    pub title: String,
    pub artists: Vec<String>,
    pub album: Option<String>,
    pub album_id: Option<String>,
    pub duration_ms: i64,
    pub thumbnail: String,
    pub stream_url: Option<String>,
}

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct Artist {
    pub id: String,
    pub name: String,
    pub thumbnail: String,
    pub subscriber_count: Option<String>,
}

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct Album {
    pub id: String,
    pub title: String,
    pub artists: Vec<String>,
    pub year: Option<i32>,
    pub thumbnail: String,
    pub track_count: Option<i32>,
}

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct Playlist {
    pub id: String,
    pub title: String,
    pub description: Option<String>,
    pub owner: Option<String>,
    pub thumbnail: String,
    pub track_count: Option<i32>,
}

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct SearchResults {
    pub query: String,
    pub songs: Vec<Track>,
    pub videos: Vec<Track>,
    pub albums: Vec<Album>,
    pub artists: Vec<Artist>,
    pub playlists: Vec<Playlist>,
}

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct HomeSection {
    pub title: String,
    pub items: Vec<HomeItem>,
    pub browse_id: Option<String>,
}

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct HomeItem {
    pub title: String,
    pub subtitle: String,
    pub thumbnail: String,
    pub item_type: String,
    pub browse_id: Option<String>,
    pub playlist_id: Option<String>,
}
