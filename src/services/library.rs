use crate::api::client::ApiClient;
use crate::services::library_cache::{LibraryCache, LibrarySource, CacheData};
use std::sync::Arc;
use tokio::sync::RwLock;

pub struct LibraryService {
    api: ApiClient,
    cache: Arc<RwLock<LibraryCache>>,
}

impl LibraryService {
    pub fn new(authenticated: bool) -> Self {
        Self {
            api: ApiClient::new(),
            cache: Arc::new(RwLock::new(LibraryCache::new(authenticated))),
        }
    }

    /// Cargar canciones de la biblioteca remota
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
        
        // Guardar en caché
        let cache = self.cache.write().await;
        cache.set("songs", CacheData {
            songs: songs.clone(),
            albums: Vec::new(),
            artists: Vec::new(),
            playlists: Vec::new(),
            source: LibrarySource::Remote,
        });
        
        crate::bridge::bridge::set_library_songs(songs);
    }

    /// Cargar playlists de la biblioteca remota
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
        
        // Guardar en caché
        let cache = self.cache.write().await;
        cache.set("playlists", CacheData {
            songs: Vec::new(),
            albums: Vec::new(),
            artists: Vec::new(),
            playlists: playlists.clone(),
            source: LibrarySource::Remote,
        });

        crate::bridge::bridge::set_context_playlists(
            playlists.iter().map(|p| crate::bridge::bridge::Playlist {
                id: p.id.clone(),
                name: p.name.clone(),
                description: p.description.clone(),
                thumbnail: p.thumbnail.clone(),
                track_count: p.track_count,
            }).collect()
        );
        crate::bridge::bridge::set_library_playlists(playlists);
    }

    /// Cargar álbumes de la biblioteca remota
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
                artist_id: a.artist_id.unwrap_or_default(),
            })
            .collect();
        
        // Guardar en caché
        let cache = self.cache.write().await;
        cache.set("albums", CacheData {
            songs: Vec::new(),
            albums: albums.clone(),
            artists: Vec::new(),
            playlists: Vec::new(),
            source: LibrarySource::Remote,
        });

        crate::bridge::bridge::set_library_albums(albums);
    }

    /// Cargar artistas de la biblioteca remota
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
        
        // Guardar en caché
        let cache = self.cache.write().await;
        cache.set("artists", CacheData {
            songs: Vec::new(),
            albums: Vec::new(),
            artists: artists.clone(),
            playlists: Vec::new(),
            source: LibrarySource::Remote,
        });

        crate::bridge::bridge::set_library_artists(artists);
    }

    /// Buscar y filtrar canciones
    pub async fn search_songs(&self, query: &str) -> Vec<crate::bridge::bridge::Track> {
        let cache = self.cache.read().await;
        if let Some(data) = cache.get("songs") {
            LibraryCache::search_tracks(&data.songs, query)
        } else {
            Vec::new()
        }
    }

    /// Buscar y filtrar álbumes
    pub async fn search_albums(&self, query: &str) -> Vec<crate::bridge::bridge::Album> {
        let cache = self.cache.read().await;
        if let Some(data) = cache.get("albums") {
            LibraryCache::search_albums(&data.albums, query)
        } else {
            Vec::new()
        }
    }

    /// Buscar y filtrar artistas
    pub async fn search_artists(&self, query: &str) -> Vec<crate::bridge::bridge::Artist> {
        let cache = self.cache.read().await;
        if let Some(data) = cache.get("artists") {
            LibraryCache::search_artists(&data.artists, query)
        } else {
            Vec::new()
        }
    }

    /// Buscar y filtrar playlists
    pub async fn search_playlists(&self, query: &str) -> Vec<crate::bridge::bridge::Playlist> {
        let cache = self.cache.read().await;
        if let Some(data) = cache.get("playlists") {
            LibraryCache::search_playlists(&data.playlists, query)
        } else {
            Vec::new()
        }
    }

    /// Ordenar canciones
    pub async fn sort_songs(&self, sort_by: &str) -> Vec<crate::bridge::bridge::Track> {
        let cache = self.cache.read().await;
        if let Some(data) = cache.get("songs") {
            LibraryCache::sort_tracks(data.songs, sort_by)
        } else {
            Vec::new()
        }
    }

    /// Invalidar caché de un tab
    pub async fn invalidate_cache(&self, tab: &str) {
        let cache = self.cache.write().await;
        cache.invalidate(tab);
    }

    /// Invalidar todo el caché
    pub async fn invalidate_all_cache(&self) {
        let cache = self.cache.write().await;
        cache.invalidate_all();
    }
}

impl Default for LibraryService {
    fn default() -> Self {
        Self::new(false)
    }
}
