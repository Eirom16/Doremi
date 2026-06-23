use crate::bridge::bridge::*;

pub fn load_mock_data_for_view(view: &str) {
    match view {
        "home" => {
            // Show loading skeleton first for a brief moment before data arrives
            set_home_state("loading", "");
            clear_home_sections();

            // Short title song
            let song_short = HomeCard {
                id: "song_short".to_string(),
                title: "Doremi".to_string(),
                subtitle: "Short Artist".to_string(),
                thumbnail: "https://lh3.googleusercontent.com/notfound-short".to_string(),
                item_type: "song".to_string(),
            };

            // Extremely long title song
            let song_long = HomeCard {
                id: "song_long".to_string(),
                title: "This is a super extremely long song title that is designed to test how the UI handles overflow and wrapping in the player bar and song card".to_string(),
                subtitle: "Extremely Long Artist Name Studio Group Orchestra".to_string(),
                thumbnail: "https://lh3.googleusercontent.com/notfound-long".to_string(),
                item_type: "song".to_string(),
            };

            // Artist without image
            let artist_no_img = HomeCard {
                id: "artist_no_img".to_string(),
                title: "Artist Without Image".to_string(),
                subtitle: "No Image".to_string(),
                thumbnail: "".to_string(),
                item_type: "artist".to_string(),
            };

            // Album with missing image
            let album_no_img = HomeCard {
                id: "album_no_img".to_string(),
                title: "Album Without Image".to_string(),
                subtitle: "Missing Image".to_string(),
                thumbnail: "".to_string(),
                item_type: "album".to_string(),
            };

            add_home_section(
                "Casos Especiales de UI",
                vec![song_short, song_long, artist_no_img, album_no_img],
            );

            // Add standard mock sections
            let mut rec_items = Vec::new();
            for i in 1..=8 {
                rec_items.push(HomeCard {
                    id: format!("rec_{i}"),
                    title: format!("Canción Recomendada {i}"),
                    subtitle: format!("Artista {i}"),
                    thumbnail: "https://lh3.googleusercontent.com/placeholder".to_string(),
                    item_type: "song".to_string(),
                });
            }
            add_home_section("Recomendados para ti", rec_items);

            // Empty playlist
            let playlist_empty = HomeCard {
                id: "playlist_empty".to_string(),
                title: "My Empty Playlist".to_string(),
                subtitle: "0 canciones • Test User".to_string(),
                thumbnail: "".to_string(),
                item_type: "playlist".to_string(),
            };
            add_home_section("Listas de reproducción", vec![playlist_empty]);

            // Set success state
            set_home_state("success", "");
        }
        "search" => {
            // Mixed search results
            let top_res = TopResult {
                id: "top_artist".to_string(),
                title: "The Ultimate Artist".to_string(),
                subtitle: "Artista • 10M suscriptores".to_string(),
                thumbnail: "".to_string(), // Artist without image
                item_type: "artist".to_string(),
            };

            let mut songs = Vec::new();
            songs.push(Track {
                id: "song_short".to_string(),
                title: "Doremi".to_string(),
                artist: "Short Artist".to_string(),
                album: "Doremi EP".to_string(),
                duration_ms: 180000,
                thumbnail: "https://lh3.googleusercontent.com/notfound-short".to_string(),
            });
            songs.push(Track {
                id: "song_long".to_string(),
                title: "This is a super extremely long song title that is designed to test how the UI handles overflow and wrapping in the player bar and song card".to_string(),
                artist: "Extremely Long Artist Name Studio Group Orchestra".to_string(),
                album: "The Longest Album in the History of Recorded Music Volume 1".to_string(),
                duration_ms: 600000,
                thumbnail: "https://lh3.googleusercontent.com/notfound-long".to_string(),
            });

            let mut artists = Vec::new();
            artists.push(Artist {
                id: "artist_no_img".to_string(),
                name: "Artist Without Image".to_string(),
                thumbnail: "".to_string(),
                description: "Mock description for no image artist".to_string(),
                subscribers: "123K".to_string(),
            });

            let mut albums = Vec::new();
            albums.push(Album {
                id: "album_no_img".to_string(),
                title: "Album Without Image".to_string(),
                artist: "Missing Image Artist".to_string(),
                year: "2026".to_string(),
                thumbnail: "".to_string(),
                track_count: 10,
                artist_id: "artist_123".to_string(),
            });

            let mut playlists = Vec::new();
            // Empty playlist
            playlists.push(Playlist {
                id: "playlist_empty".to_string(),
                name: "My Empty Playlist".to_string(),
                description: "Esta es una playlist vacía para pruebas visuales".to_string(),
                thumbnail: "".to_string(),
                track_count: 0,
                owner: "Test User".to_string(),
                privacy: "PRIVATE".to_string(),
            });

            set_search_results(
                top_res,
                true,
                songs,
                Vec::new(), // videos
                artists,
                albums,
                playlists,
                Vec::new(), // shows
                Vec::new(), // episodes
            );
        }
        "library" => {
            // Library with many elements
            let mut songs = Vec::new();
            for i in 1..=25 {
                songs.push(Track {
                    id: format!("fav_{i}"),
                    title: format!("Canción Favorita {i}"),
                    artist: format!("Artista {i}"),
                    album: format!("Álbum {i}"),
                    duration_ms: 200000,
                    thumbnail: "".to_string(),
                });
            }
            set_library_songs(songs);

            // Empty playlist in library
            let empty_playlist = Playlist {
                id: "playlist_empty".to_string(),
                name: "My Empty Playlist".to_string(),
                description: "Esta es una playlist vacía para pruebas visuales".to_string(),
                thumbnail: "".to_string(),
                track_count: 0,
                owner: "Test User".to_string(),
                privacy: "PRIVATE".to_string(),
            };
            set_library_playlists(vec![empty_playlist]);
        }
        "now-playing" => {
            // Song with extremely long title
            let track = Track {
                id: "song_long".to_string(),
                title: "This is a super extremely long song title that is designed to test how the UI handles overflow and wrapping in the player bar and song card".to_string(),
                artist: "Extremely Long Artist Name Studio Group Orchestra".to_string(),
                album: "The Longest Album in the History of Recorded Music Volume 1".to_string(),
                duration_ms: 600000,
                thumbnail: "".to_string(),
            };
            set_current_track(track);

            // Set playback queue with some items
            let queue = vec![
                Track {
                    id: "song_short".to_string(),
                    title: "Doremi".to_string(),
                    artist: "Short Artist".to_string(),
                    album: "Doremi EP".to_string(),
                    duration_ms: 180000,
                    thumbnail: "".to_string(),
                },
                Track {
                    id: "song_long".to_string(),
                    title: "This is a super extremely long song title that is designed to test how the UI handles overflow and wrapping in the player bar and song card".to_string(),
                    artist: "Extremely Long Artist Name Studio Group Orchestra".to_string(),
                    album: "The Longest Album in the History of Recorded Music Volume 1".to_string(),
                    duration_ms: 600000,
                    thumbnail: "".to_string(),
                }
            ];
            set_playback_queue(queue, 1);
            set_playing(true);
        }
        "downloads" => {
            // Download at 37%
            let items = vec![
                crate::bridge::bridge::DownloadItem {
                    video_id: "vid_dl_1".to_string(),
                    title: "Canción Descargando".to_string(),
                    artist: "Artista Descarga".to_string(),
                    album: String::new(),
                    thumbnail_url: String::new(),
                    parent_playlist_id: String::new(),
                    parent_playlist_title: String::new(),
                    parent_playlist_thumbnail_url: String::new(),
                    status: "downloading".to_string(),
                    progress: 37.0,
                },
                crate::bridge::bridge::DownloadItem {
                    video_id: "vid_dl_2".to_string(),
                    title: "Canción Completada".to_string(),
                    artist: "Artista Completado".to_string(),
                    album: String::new(),
                    thumbnail_url: String::new(),
                    parent_playlist_id: String::new(),
                    parent_playlist_title: String::new(),
                    parent_playlist_thumbnail_url: String::new(),
                    status: "completed".to_string(),
                    progress: 100.0,
                },
            ];
            set_downloads_list(items);
        }
        "loading" => {
            // Loading state
            set_home_state("loading", "Cargando recomendados...");
        }
        "error" => {
            // Error state
            set_home_state("error", "Error de conexión de red. Inténtalo de nuevo.");
        }
        "history" => {
            let mut tracks = Vec::new();
            let mut played_at = Vec::new();

            // Item 1: Hoy
            tracks.push(Track {
                id: "song_short".to_string(),
                title: "Doremi".to_string(),
                artist: "Short Artist".to_string(),
                album: "Doremi EP".to_string(),
                duration_ms: 180000,
                thumbnail: "".to_string(),
            });
            played_at.push(chrono::Local::now().format("%Y-%m-%d %H:%M:%S").to_string());

            // Item 2: Ayer
            tracks.push(Track {
                id: "song_long".to_string(),
                title: "This is a super extremely long song title that is designed to test how the UI handles overflow and wrapping in the player bar and song card".to_string(),
                artist: "Extremely Long Artist Name Studio Group Orchestra".to_string(),
                album: "The Longest Album in the History of Recorded Music Volume 1".to_string(),
                duration_ms: 600000,
                thumbnail: "".to_string(),
            });
            played_at.push(
                (chrono::Local::now() - chrono::Duration::days(1))
                    .format("%Y-%m-%d %H:%M:%S")
                    .to_string(),
            );

            // Item 3: Esta semana
            tracks.push(Track {
                id: "song_fav_1".to_string(),
                title: "Canción de la semana".to_string(),
                artist: "Artista Semanal".to_string(),
                album: "Álbum Semanal".to_string(),
                duration_ms: 240000,
                thumbnail: "".to_string(),
            });
            played_at.push(
                (chrono::Local::now() - chrono::Duration::days(3))
                    .format("%Y-%m-%d %H:%M:%S")
                    .to_string(),
            );

            // Item 4: Anterior
            tracks.push(Track {
                id: "song_fav_2".to_string(),
                title: "Canción Antigua".to_string(),
                artist: "Artista Clásico".to_string(),
                album: "Álbum Antiguo".to_string(),
                duration_ms: 320000,
                thumbnail: "".to_string(),
            });
            played_at.push(
                (chrono::Local::now() - chrono::Duration::days(10))
                    .format("%Y-%m-%d %H:%M:%S")
                    .to_string(),
            );

            let tokens = vec![String::new(); tracks.len()];
            set_history_data(tracks, played_at, tokens);
        }
        "stats" => {
            let mut top_tracks = Vec::new();
            top_tracks.push(Track {
                id: "song_short".to_string(),
                title: "Doremi".to_string(),
                artist: "Short Artist".to_string(),
                album: "Doremi EP".to_string(),
                duration_ms: 180000,
                thumbnail: "".to_string(),
            });
            top_tracks.push(Track {
                id: "song_fav_1".to_string(),
                title: "Canción de la semana".to_string(),
                artist: "Artista Semanal".to_string(),
                album: "Álbum Semanal".to_string(),
                duration_ms: 240000,
                thumbnail: "".to_string(),
            });

            let stats = StatsData {
                total_play_time: "15h 24m".to_string(),
                total_plays: 284,
                unique_artists: 42,
                weekly_activity: vec![10, 25, 18, 30, 45, 12, 5],
                top_tracks,
                top_tracks_plays: vec![84, 45],
            };
            set_stats_data(stats);
        }
        "trending" => {
            set_trending_state("loading", "");
            let mut items = Vec::new();
            let genres = [
                "Pop",
                "Rock",
                "Hip-Hop",
                "Latin",
                "R&B",
                "Electronic",
                "Jazz",
                "Classical",
            ];
            let artists = [
                "Bad Bunny",
                "Taylor Swift",
                "The Weeknd",
                "Rauw Alejandro",
                "SZA",
                "Daft Punk",
                "Miles Davis",
                "Ludwig van Beethoven",
            ];
            for i in 1..=12usize {
                items.push(HomeCard {
                    id: format!("trend_{i}"),
                    title: format!("#{i} — Canción Trending {i}"),
                    subtitle: format!(
                        "{} · {}",
                        artists[(i - 1) % artists.len()],
                        genres[(i - 1) % genres.len()]
                    ),
                    thumbnail: "".to_string(),
                    item_type: "song".to_string(),
                });
            }
            set_trending_items(items);
            set_trending_state("content", "");
        }
        "settings" => {
            // Settings view initializes itself from design tokens, no mock data needed.
            // This case just exists so --ui-test settings navigates correctly.
        }
        "album" | "album_detail" => {
            let album = Album {
                id: "mock_album_1".to_string(),
                title: "Mock Album de Prueba".to_string(),
                artist: "Artista de Prueba".to_string(),
                year: "2024".to_string(),
                thumbnail: "".to_string(),
                track_count: 10,
                artist_id: "mock_artist_1".to_string(),
            };
            let tracks: Vec<Track> = (1..=10)
                .map(|i| Track {
                    id: format!("album_track_{i}"),
                    title: format!("Pista {i}: Canción de Ejemplo"),
                    artist: "Artista de Prueba".to_string(),
                    album: "Mock Album de Prueba".to_string(),
                    duration_ms: 180000 + i * 10000,
                    thumbnail: "".to_string(),
                })
                .collect();
            set_album_detail(album, tracks);
        }
        _ => {}
    }
}
