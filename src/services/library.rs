use crate::api::client::ApiClient;
use crate::services::library_cache::{
    CacheData, LibraryCache, LibrarySource, GLOBAL_LIBRARY_CACHE,
};

pub struct LibraryService {
    api: ApiClient,
}

impl LibraryService {
    pub fn new(authenticated: bool) -> Self {
        if let Ok(mut cache) = GLOBAL_LIBRARY_CACHE.write() {
            cache.authenticated = authenticated;
        }
        Self {
            api: ApiClient::new(),
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
        if let Ok(cache) = GLOBAL_LIBRARY_CACHE.read() {
            cache.set(
                "songs",
                CacheData {
                    songs: songs.clone(),
                    albums: Vec::new(),
                    artists: Vec::new(),
                    playlists: Vec::new(),
                    source: LibrarySource::Remote,
                },
            );
        }

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
                owner: p.owner.unwrap_or_default(),
                privacy: String::new(),
            })
            .collect();

        // Guardar en caché
        if let Ok(cache) = GLOBAL_LIBRARY_CACHE.read() {
            cache.set(
                "playlists",
                CacheData {
                    songs: Vec::new(),
                    albums: Vec::new(),
                    artists: Vec::new(),
                    playlists: playlists.clone(),
                    source: LibrarySource::Remote,
                },
            );
        }

        crate::bridge::bridge::set_context_playlists(
            playlists
                .iter()
                .map(|p| crate::bridge::bridge::Playlist {
                    id: p.id.clone(),
                    name: p.name.clone(),
                    description: p.description.clone(),
                    thumbnail: p.thumbnail.clone(),
                    track_count: p.track_count,
                    owner: p.owner.clone(),
                    privacy: p.privacy.clone(),
                })
                .collect(),
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
        if let Ok(cache) = GLOBAL_LIBRARY_CACHE.read() {
            cache.set(
                "albums",
                CacheData {
                    songs: Vec::new(),
                    albums: albums.clone(),
                    artists: Vec::new(),
                    playlists: Vec::new(),
                    source: LibrarySource::Remote,
                },
            );
        }

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
        if let Ok(cache) = GLOBAL_LIBRARY_CACHE.read() {
            cache.set(
                "artists",
                CacheData {
                    songs: Vec::new(),
                    albums: Vec::new(),
                    artists: artists.clone(),
                    playlists: Vec::new(),
                    source: LibrarySource::Remote,
                },
            );
        }

        crate::bridge::bridge::set_library_artists(artists);
    }

    /// Buscar dentro de un tab
    pub fn search_songs(&self, tab: &str, query: &str) -> Vec<crate::bridge::bridge::Track> {
        if let Ok(cache) = GLOBAL_LIBRARY_CACHE.read() {
            if let Some(data) = cache.get(tab) {
                return LibraryCache::search_tracks(&data.songs, query);
            }
        }
        Vec::new()
    }

    pub fn search_albums(&self, tab: &str, query: &str) -> Vec<crate::bridge::bridge::Album> {
        if let Ok(cache) = GLOBAL_LIBRARY_CACHE.read() {
            if let Some(data) = cache.get(tab) {
                return LibraryCache::search_albums(&data.albums, query);
            }
        }
        Vec::new()
    }

    pub fn search_artists(&self, tab: &str, query: &str) -> Vec<crate::bridge::bridge::Artist> {
        if let Ok(cache) = GLOBAL_LIBRARY_CACHE.read() {
            if let Some(data) = cache.get(tab) {
                return LibraryCache::search_artists(&data.artists, query);
            }
        }
        Vec::new()
    }

    pub fn search_playlists(&self, tab: &str, query: &str) -> Vec<crate::bridge::bridge::Playlist> {
        if let Ok(cache) = GLOBAL_LIBRARY_CACHE.read() {
            if let Some(data) = cache.get(tab) {
                return LibraryCache::search_playlists(&data.playlists, query);
            }
        }
        Vec::new()
    }

    /// Ordenar canciones
    pub fn sort_songs(
        &self,
        _tab: &str,
        songs: Vec<crate::bridge::bridge::Track>,
        sort_by: &str,
    ) -> Vec<crate::bridge::bridge::Track> {
        // If sort_by is empty, return as is
        if sort_by.is_empty() {
            return songs;
        }
        // Normalize sort key
        let key = match sort_by {
            "name_asc" | "title" => "title",
            "artist" => "artist",
            "album" => "album",
            "duration" => "duration",
            _ => "title",
        };
        LibraryCache::sort_tracks(songs, key)
    }

    /// Ordenar álbumes
    pub fn sort_albums(
        &self,
        _tab: &str,
        albums: Vec<crate::bridge::bridge::Album>,
        sort_by: &str,
    ) -> Vec<crate::bridge::bridge::Album> {
        if sort_by.is_empty() {
            return albums;
        }
        let key = match sort_by {
            "name_asc" | "title" => "title",
            "artist" => "artist",
            "year" => "year",
            _ => "title",
        };
        LibraryCache::sort_albums(albums, key)
    }

    /// Ordenar artistas
    pub fn sort_artists(
        &self,
        _tab: &str,
        artists: Vec<crate::bridge::bridge::Artist>,
        sort_by: &str,
    ) -> Vec<crate::bridge::bridge::Artist> {
        if sort_by.is_empty() {
            return artists;
        }
        let key = match sort_by {
            "name_asc" | "name" => "name",
            _ => "name",
        };
        LibraryCache::sort_artists(artists, key)
    }

    /// Ordenar playlists
    pub fn sort_playlists(
        &self,
        _tab: &str,
        playlists: Vec<crate::bridge::bridge::Playlist>,
        sort_by: &str,
    ) -> Vec<crate::bridge::bridge::Playlist> {
        if sort_by.is_empty() {
            return playlists;
        }
        let key = match sort_by {
            "name_asc" | "name" => "name",
            "tracks" => "tracks",
            _ => "name",
        };
        LibraryCache::sort_playlists(playlists, key)
    }

    /// Invalidar caché de un tab
    pub async fn invalidate_cache(&self, tab: &str) {
        if let Ok(cache) = GLOBAL_LIBRARY_CACHE.write() {
            cache.invalidate(tab);
        }
    }

    /// Invalidar todo el caché
    pub async fn invalidate_all_cache(&self) {
        if let Ok(cache) = GLOBAL_LIBRARY_CACHE.write() {
            cache.invalidate_all();
        }
    }
}

impl Default for LibraryService {
    fn default() -> Self {
        Self::new(false)
    }
}
