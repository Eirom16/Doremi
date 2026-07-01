use once_cell::sync::Lazy;
use std::collections::HashMap;
use std::sync::{Arc, Mutex, RwLock};
use std::time::{Duration, Instant};

pub static GLOBAL_LIBRARY_CACHE: Lazy<RwLock<LibraryCache>> =
    Lazy::new(|| RwLock::new(LibraryCache::new(false)));

/// Origen de la biblioteca
#[derive(Clone, Copy, Debug, PartialEq, Eq, Hash)]
pub enum LibrarySource {
    /// Canciones y playlists remotas de YouTube Music
    Remote,
    /// Descargas locales
    Downloaded,
    /// Datos locales (favoritos no sincronizados)
    Local,
}

impl LibrarySource {
    pub fn as_str(&self) -> &'static str {
        match self {
            Self::Remote => "remote",
            Self::Downloaded => "downloaded",
            Self::Local => "local",
        }
    }
}

/// Entrada en caché con validez temporal
#[derive(Clone)]
pub struct CacheEntry<T> {
    pub data: T,
    pub loaded_at: Instant,
    pub ttl: Duration,
}

impl<T> CacheEntry<T> {
    pub fn new(data: T, ttl: Duration) -> Self {
        Self {
            data,
            loaded_at: Instant::now(),
            ttl,
        }
    }

    pub fn is_valid(&self) -> bool {
        self.loaded_at.elapsed() < self.ttl
    }
}

/// Cache de biblioteca por tab
pub struct LibraryCache {
    // Cache por tab (songs, albums, artists, playlists)
    cache: Arc<Mutex<HashMap<String, CacheEntry<CacheData>>>>,
    // TTL para cada tab (default 5 minutos)
    default_ttl: Duration,
    // Estado de autenticación
    pub authenticated: bool,
}

#[derive(Clone)]
pub struct CacheData {
    pub songs: Vec<crate::bridge::bridge::Track>,
    pub albums: Vec<crate::bridge::bridge::Album>,
    pub artists: Vec<crate::bridge::bridge::Artist>,
    pub playlists: Vec<crate::bridge::bridge::Playlist>,
    pub source: LibrarySource,
}

impl CacheData {
    /// Create CacheData containing only songs.
    pub fn songs_only(songs: Vec<crate::bridge::bridge::Track>, source: LibrarySource) -> Self {
        Self { songs, albums: Vec::new(), artists: Vec::new(), playlists: Vec::new(), source }
    }

    /// Create CacheData containing only albums.
    pub fn albums_only(albums: Vec<crate::bridge::bridge::Album>, source: LibrarySource) -> Self {
        Self { songs: Vec::new(), albums, artists: Vec::new(), playlists: Vec::new(), source }
    }

    /// Create CacheData containing only artists.
    pub fn artists_only(artists: Vec<crate::bridge::bridge::Artist>, source: LibrarySource) -> Self {
        Self { songs: Vec::new(), albums: Vec::new(), artists, playlists: Vec::new(), source }
    }

    /// Create CacheData containing only playlists.
    pub fn playlists_only(playlists: Vec<crate::bridge::bridge::Playlist>, source: LibrarySource) -> Self {
        Self { songs: Vec::new(), albums: Vec::new(), artists: Vec::new(), playlists, source }
    }
}

/// Helper to update the global library cache for a given tab, encapsulating
/// the RwLock acquire + set pattern that was previously repeated 20+ times.
pub fn update_cache(tab: &str, data: CacheData) {
    if let Ok(cache) = GLOBAL_LIBRARY_CACHE.write() {
        cache.set(tab, data);
    }
}

/// Helper to invalidate a specific tab in the global cache.
pub fn invalidate_cache_tab(tab: &str) {
    if let Ok(cache) = GLOBAL_LIBRARY_CACHE.write() {
        cache.invalidate(tab);
    }
}

impl LibraryCache {
    pub fn new(authenticated: bool) -> Self {
        Self {
            cache: Arc::new(Mutex::new(HashMap::new())),
            default_ttl: Duration::from_secs(300), // 5 minutos
            authenticated,
        }
    }

    /// Obtener datos cacheados para un tab
    pub fn get(&self, tab: &str) -> Option<CacheData> {
        let cache = self.cache.lock().unwrap();
        cache.get(tab).and_then(|entry| {
            if entry.is_valid() {
                Some(entry.data.clone())
            } else {
                None
            }
        })
    }

    /// Guardar datos en caché
    pub fn set(&self, tab: &str, data: CacheData) {
        let mut cache = self.cache.lock().unwrap();
        cache.insert(tab.to_string(), CacheEntry::new(data, self.default_ttl));
    }

    /// Invalidar un tab específico
    pub fn invalidate(&self, tab: &str) {
        let mut cache = self.cache.lock().unwrap();
        cache.remove(tab);
    }

    /// Invalidar todos los tabs
    pub fn invalidate_all(&self) {
        let mut cache = self.cache.lock().unwrap();
        cache.clear();
    }

    /// Buscar dentro de un conjunto de canciones
    pub fn search_tracks(
        tracks: &[crate::bridge::bridge::Track],
        query: &str,
    ) -> Vec<crate::bridge::bridge::Track> {
        if query.is_empty() {
            return tracks.to_vec();
        }

        let query_lower = query.to_lowercase();
        tracks
            .iter()
            .filter(|t| {
                let title_lower = t.title.to_lowercase();
                let artist_lower = t.artist.to_lowercase();
                let album_lower = t.album.to_lowercase();

                title_lower.contains(&query_lower)
                    || artist_lower.contains(&query_lower)
                    || album_lower.contains(&query_lower)
            })
            .cloned()
            .collect()
    }

    /// Buscar dentro de un conjunto de álbumes
    pub fn search_albums(
        albums: &[crate::bridge::bridge::Album],
        query: &str,
    ) -> Vec<crate::bridge::bridge::Album> {
        if query.is_empty() {
            return albums.to_vec();
        }

        let query_lower = query.to_lowercase();
        albums
            .iter()
            .filter(|a| {
                let title_lower = a.title.to_lowercase();
                let artist_lower = a.artist.to_lowercase();

                title_lower.contains(&query_lower) || artist_lower.contains(&query_lower)
            })
            .cloned()
            .collect()
    }

    /// Buscar dentro de un conjunto de artistas
    pub fn search_artists(
        artists: &[crate::bridge::bridge::Artist],
        query: &str,
    ) -> Vec<crate::bridge::bridge::Artist> {
        if query.is_empty() {
            return artists.to_vec();
        }

        let query_lower = query.to_lowercase();
        artists
            .iter()
            .filter(|a| {
                let name_lower = a.name.to_lowercase();
                name_lower.contains(&query_lower)
            })
            .cloned()
            .collect()
    }

    /// Buscar dentro de un conjunto de playlists
    pub fn search_playlists(
        playlists: &[crate::bridge::bridge::Playlist],
        query: &str,
    ) -> Vec<crate::bridge::bridge::Playlist> {
        if query.is_empty() {
            return playlists.to_vec();
        }

        let query_lower = query.to_lowercase();
        playlists
            .iter()
            .filter(|p| {
                let name_lower = p.name.to_lowercase();
                let desc_lower = p.description.to_lowercase();

                name_lower.contains(&query_lower) || desc_lower.contains(&query_lower)
            })
            .cloned()
            .collect()
    }

    /// Ordenar canciones
    pub fn sort_tracks(
        mut tracks: Vec<crate::bridge::bridge::Track>,
        sort_by: &str,
    ) -> Vec<crate::bridge::bridge::Track> {
        match sort_by {
            "title" => {
                tracks.sort_by(|a, b| a.title.cmp(&b.title));
            }
            "artist" => {
                tracks.sort_by(|a, b| a.artist.cmp(&b.artist));
            }
            "album" => {
                tracks.sort_by(|a, b| a.album.cmp(&b.album));
            }
            "duration" => {
                tracks.sort_by(|a, b| a.duration_ms.cmp(&b.duration_ms));
            }
            _ => {}
        }
        tracks
    }

    /// Ordenar álbumes
    pub fn sort_albums(
        mut albums: Vec<crate::bridge::bridge::Album>,
        sort_by: &str,
    ) -> Vec<crate::bridge::bridge::Album> {
        match sort_by {
            "title" => {
                albums.sort_by(|a, b| a.title.cmp(&b.title));
            }
            "artist" => {
                albums.sort_by(|a, b| a.artist.cmp(&b.artist));
            }
            "year" => {
                albums.sort_by(|a, b| a.year.cmp(&b.year));
            }
            _ => {}
        }
        albums
    }

    /// Ordenar artistas
    pub fn sort_artists(
        mut artists: Vec<crate::bridge::bridge::Artist>,
        sort_by: &str,
    ) -> Vec<crate::bridge::bridge::Artist> {
        match sort_by {
            "name" => {
                artists.sort_by(|a, b| a.name.cmp(&b.name));
            }
            _ => {}
        }
        artists
    }

    /// Ordenar playlists
    pub fn sort_playlists(
        mut playlists: Vec<crate::bridge::bridge::Playlist>,
        sort_by: &str,
    ) -> Vec<crate::bridge::bridge::Playlist> {
        match sort_by {
            "name" => {
                playlists.sort_by(|a, b| a.name.cmp(&b.name));
            }
            "tracks" => {
                playlists.sort_by(|a, b| b.track_count.cmp(&a.track_count));
            }
            _ => {}
        }
        playlists
    }
}
