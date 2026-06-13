use serde_json::Value;

const API_KEY: &str = "AIzaSyAO_FJ2SlqU8Q4STEHLGCilw_Y9_11qcW8";
const BASE_URL: &str = "https://music.youtube.com/youtubei/v1";

fn client_context(hl: &str, gl: &str) -> Value {
    serde_json::json!({
        "context": {
            "client": {
                "clientName": "WEB_REMIX",
                "clientVersion": "1.20250331.00.00",
                "hl": hl,
                "gl": gl,
            }
        }
    })
}

fn get_auth_headers() -> Option<serde_json::Map<String, serde_json::Value>> {
    let config_dir = crate::config::paths::AppDirs::global().config_dir();
    let path = config_dir.join("headers_auth.json");
    if path.exists() {
        if let Ok(mut file) = std::fs::File::open(path) {
            let mut content = String::new();
            use std::io::Read;
            if file.read_to_string(&mut content).is_ok() {
                if let Ok(val) = serde_json::from_str::<serde_json::Value>(&content) {
                    return val.as_object().cloned();
                }
            }
        }
    }
    None
}

fn post(endpoint: &str, body: Value) -> Result<Value, String> {
    let url = format!("{BASE_URL}/{endpoint}?key={API_KEY}");
    let client = reqwest::blocking::Client::new();
    let mut request = client.post(&url)
        .header("Content-Type", "application/json");

    if let Some(headers) = get_auth_headers() {
        for (key, val) in headers {
            if let Some(val_str) = val.as_str() {
                request = request.header(key, val_str);
            }
        }
    } else {
        request = request.header("User-Agent", "Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36");
    }

    let resp = request
        .json(&body)
        .send()
        .map_err(|e| format!("HTTP error: {e}"))?;
    let status = resp.status();
    let text = resp.text().map_err(|e| format!("Read error: {e}"))?;
    if !status.is_success() {
        return Err(format!("API status {status}: {text}"));
    }
    serde_json::from_str(&text).map_err(|e| format!("JSON parse error: {e}"))
}

fn extract_runs(val: &Value) -> String {
    let mut parts = Vec::new();
    if let Some(runs) = val.get("runs").and_then(|r| r.as_array()) {
        for run in runs {
            if let Some(text) = run.get("text").and_then(|t| t.as_str()) {
                parts.push(text.to_string());
            }
        }
    }
    parts.join(" ")
}

fn extract_thumbnail(data: &Value) -> String {
    data.get("thumbnail")
        .and_then(|t| t.get("musicThumbnailRenderer"))
        .and_then(|t| t.get("thumbnail"))
        .and_then(|t| t.get("thumbnails"))
        .and_then(|t| t.as_array())
        .and_then(|arr| arr.first())
        .and_then(|t| t.get("url"))
        .and_then(|t| t.as_str())
        .map(|s| s.to_string())
        .unwrap_or_default()
}

fn extract_video_id(data: &Value) -> String {
    data.get("playlistItemData")
        .and_then(|p| p.get("videoId"))
        .or_else(|| {
            data.get("navigationEndpoint")
                .and_then(|n| n.get("watchEndpoint"))
                .and_then(|w| w.get("videoId"))
        })
        .and_then(|v| v.as_str())
        .map(|s| s.to_string())
        .unwrap_or_default()
}

pub fn search(query: &str, _filter: &str) -> Result<super::models::SearchResults, String> {
    let mut body = client_context("es", "MX");
    body["query"] = serde_json::json!(query);
    let json = post("search", body)?;

    let mut songs = Vec::new();
    let mut albums = Vec::new();
    let mut artists = Vec::new();
    let mut playlists = Vec::new();
    let mut videos = Vec::new();

    if let Some(tabs) = json["contents"]["tabbedSearchResultsRenderer"]["tabs"].as_array() {
        if let Some(tab) = tabs.first() {
            let contents = &tab["tabRenderer"]["content"]["sectionListRenderer"]["contents"];
            if let Some(sections) = contents.as_array() {
                for section in sections {
                    let shelf = section.get("musicShelfRenderer");
                    let is_card = section.get("musicCardShelfRenderer");
                    let data = shelf.or(is_card);
                    if data.is_none() { continue; }
                    let data = data.unwrap();

                    let section_title = extract_runs(&data["title"]).to_lowercase();

                    let items = data["contents"].as_array();
                    if items.is_none() { continue; }

                    let category = if section_title.contains("canción") || section_title.contains("song") {
                        "songs"
                    } else if section_title.contains("video") {
                        "videos"
                    } else if section_title.contains("álbum") || section_title.contains("album") {
                        "albums"
                    } else if section_title.contains("artista") || section_title.contains("artist") {
                        "artists"
                    } else if section_title.contains("playlist") || section_title.contains("lista") {
                        "playlists"
                    } else {
                        "songs"
                    };

                    for item in items.unwrap() {
                        let renderer = &item["musicResponsiveListItemRenderer"];
                        if renderer.is_null() { continue; }

                        let title = extract_runs(&renderer["flexColumns"][0]
                            ["musicResponsiveListItemFlexColumnRenderer"]["text"]);
                        let subtitle = renderer["flexColumns"].as_array()
                            .and_then(|cols| cols.get(1))
                            .map(|c| extract_runs(&c["musicResponsiveListItemFlexColumnRenderer"]["text"]))
                            .unwrap_or_default();
                        let thumb = extract_thumbnail(renderer);
                        let video_id = extract_video_id(renderer);
                        let browse_id = renderer["navigationEndpoint"]["browseEndpoint"]["browseId"]
                            .as_str().map(|s| s.to_string());

                        match category {
                            "songs" => {
                                let (artist, album) = if subtitle.contains("•") {
                                    let parts: Vec<&str> = subtitle.splitn(2, "•").collect();
                                    (parts.get(0).map(|s| s.trim()).unwrap_or("").to_string(),
                                     parts.get(1).map(|s| s.trim()).unwrap_or("").to_string())
                                } else {
                                    (subtitle.clone(), String::new())
                                };
                                songs.push(super::models::Track {
                                    id: video_id,
                                    title: title.clone(),
                                    artists: vec![artist],
                                    album: Some(album),
                                    album_id: None,
                                    duration_ms: 0,
                                    thumbnail: thumb,
                                    stream_url: None,
                                });
                            }
                            "videos" => {
                                videos.push(super::models::Track {
                                    id: video_id,
                                    title: title.clone(),
                                    artists: vec![subtitle.clone()],
                                    album: None,
                                    album_id: None,
                                    duration_ms: 0,
                                    thumbnail: thumb,
                                    stream_url: None,
                                });
                            }
                            "albums" => {
                                albums.push(super::models::Album {
                                    id: browse_id.unwrap_or_default(),
                                    title: title.clone(),
                                    artists: vec![subtitle.clone()],
                                    year: None,
                                    thumbnail: thumb,
                                    track_count: None,
                                });
                            }
                            "artists" => {
                                artists.push(super::models::Artist {
                                    id: browse_id.unwrap_or_default(),
                                    name: title.clone(),
                                    thumbnail: thumb,
                                    subscriber_count: None,
                                });
                            }
                            "playlists" => {
                                playlists.push(super::models::Playlist {
                                    id: browse_id.unwrap_or_default(),
                                    title: title.clone(),
                                    description: None,
                                    owner: Some(subtitle),
                                    thumbnail: thumb,
                                    track_count: None,
                                });
                            }
                            _ => {}
                        }
                    }
                }
            }
        }
    }

    Ok(super::models::SearchResults {
        query: query.to_string(),
        songs,
        videos,
        albums,
        artists,
        playlists,
    })
}

pub fn home_sections() -> Result<Vec<super::models::HomeSection>, String> {
    let mut body = client_context("es", "MX");
    body["browseId"] = serde_json::json!("FEmusic_home");
    let json = post("browse", body)?;

    let mut sections = Vec::new();

    if let Some(tabs) = json["contents"]["singleColumnBrowseResultsRenderer"]["tabs"].as_array() {
        if let Some(tab) = tabs.first() {
            let contents = &tab["tabRenderer"]["content"]["sectionListRenderer"]["contents"];
            if let Some(items) = contents.as_array() {
                for item in items {
                    let carousel = item.get("musicCarouselShelfRenderer");
                    let description = item.get("musicDescriptionShelfRenderer");
                    let data = carousel.or(description);
                    if data.is_none() { continue; }
                    let data = data.unwrap();

                    let title = data["header"]
                        .get("musicCarouselShelfBasicHeaderRenderer")
                        .or_else(|| data["header"].get("musicDescriptionShelfRenderer"))
                        .map(|h| extract_runs(&h["title"]))
                        .unwrap_or_default();

                    let mut section_items = Vec::new();
                    if let Some(contents) = data["contents"].as_array() {
                        for c in contents {
                            let renderer = c.get("musicResponsiveListItemRenderer")
                                .or_else(|| c.get("musicTwoRowItemRenderer"));
                            if renderer.is_none() { continue; }
                            let renderer = renderer.unwrap();

                            let item_title = renderer["flexColumns"].as_array()
                                .and_then(|cols| cols.first())
                                .map(|col| extract_runs(&col["musicResponsiveListItemFlexColumnRenderer"]["text"]))
                                .unwrap_or_default();
                            let item_sub = renderer["flexColumns"].as_array()
                                .and_then(|cols| cols.get(1))
                                .map(|col| extract_runs(&col["musicResponsiveListItemFlexColumnRenderer"]["text"]))
                                .unwrap_or_default();
                            let thumb = extract_thumbnail(renderer);
                            let item_type = if renderer.get("playlistItemData").is_some() {
                                "song"
                            } else if renderer["navigationEndpoint"]["browseEndpoint"]["browseId"]
                                .as_str().map(|s| s.starts_with("MPRE")).unwrap_or(false)
                            {
                                "album"
                            } else {
                                "playlist"
                            };
                            let browse_id = renderer["navigationEndpoint"]["browseEndpoint"]["browseId"]
                                .as_str().map(|s| s.to_string());
                            let playlist_id = renderer["navigationEndpoint"]["watchPlaylistEndpoint"]["playlistId"]
                                .as_str().map(|s| s.to_string());

                            section_items.push(super::models::HomeItem {
                                title: item_title,
                                subtitle: item_sub,
                                thumbnail: thumb,
                                item_type: item_type.to_string(),
                                browse_id,
                                playlist_id,
                            });
                        }
                    }

                    if !title.is_empty() || !section_items.is_empty() {
                        sections.push(super::models::HomeSection {
                            title,
                            browse_id: None,
                            items: section_items,
                        });
                    }
                }
            }
        }
    }

    Ok(sections)
}

pub fn get_stream_url(video_id: &str) -> Option<String> {
    let cache_key = format!("stream_url:{video_id}");
    if let Some(entry) = crate::db::cache::ResponseCache::get::<String>(&cache_key) {
        log::info!("Stream URL loaded from cache (sync) for {video_id}");
        return Some(entry.data);
    }

    // Use yt-dlp to get the best audio stream URL
    let output = std::process::Command::new("yt-dlp")
        .args([
            "-f", "bestaudio/best",
            "--get-url",
            "--no-playlist",
            "--no-warnings",
            &format!("https://music.youtube.com/watch?v={video_id}"),
        ])
        .output()
        .ok()?;
    if output.status.success() {
        let url = String::from_utf8_lossy(&output.stdout).trim().to_string();
        if !url.is_empty() {
            let _ = crate::db::cache::ResponseCache::set(&cache_key, &url, Some(14400));
            return Some(url);
        }
    }
    None
}

pub async fn get_stream_url_async(video_id: &str) -> Option<String> {
    let cache_key = format!("stream_url:{video_id}");
    if let Some(entry) = crate::db::cache::ResponseCache::get::<String>(&cache_key) {
        log::info!("Stream URL loaded from cache (async) for {video_id}");
        return Some(entry.data);
    }

    let output = tokio::process::Command::new("yt-dlp")
        .args([
            "-f", "bestaudio/best",
            "--get-url",
            "--no-playlist",
            "--no-warnings",
            &format!("https://music.youtube.com/watch?v={video_id}"),
        ])
        .output()
        .await
        .ok()?;
    if output.status.success() {
        let url = String::from_utf8_lossy(&output.stdout).trim().to_string();
        if !url.is_empty() {
            let _ = crate::db::cache::ResponseCache::set(&cache_key, &url, Some(14400));
            return Some(url);
        }
    }
    None
}
