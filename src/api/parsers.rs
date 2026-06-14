use super::models::{Album, Artist, HomeItem, HomeSection, Playlist, SearchResults, Track};
use serde_json::Value;
use std::collections::HashSet;

pub(crate) struct ParsedPage<T> {
    pub items: T,
    pub continuation: Option<String>,
}

fn schema_error(endpoint: &str, expected: &str, json: &Value) -> String {
    let observed = match json {
        Value::Object(map) if map.is_empty() => "empty object".to_string(),
        Value::Object(map) => format!(
            "root keys [{}]",
            map.keys()
                .map(String::as_str)
                .collect::<Vec<_>>()
                .join(", ")
        ),
        Value::Array(items) => format!("array with {} items", items.len()),
        Value::Null => "null".to_string(),
        other => format!(
            "{} value",
            match other {
                Value::Bool(_) => "boolean",
                Value::Number(_) => "number",
                Value::String(_) => "string",
                _ => "unknown",
            }
        ),
    };
    format!("Innertube {endpoint} schema changed: expected {expected}; observed {observed}")
}

fn runs(value: &Value) -> String {
    value["runs"]
        .as_array()
        .map(|runs| {
            runs.iter()
                .filter_map(|run| run["text"].as_str())
                .collect::<Vec<_>>()
                .join(" ")
        })
        .unwrap_or_default()
}

fn text(value: &Value) -> String {
    value["simpleText"]
        .as_str()
        .map(str::to_string)
        .unwrap_or_else(|| runs(value))
}

fn thumbnail(renderer: &Value) -> String {
    renderer
        .pointer("/thumbnail/musicThumbnailRenderer/thumbnail/thumbnails")
        .or_else(|| renderer.pointer("/thumbnail/thumbnails"))
        .and_then(Value::as_array)
        .and_then(|items| items.last())
        .and_then(|item| item["url"].as_str())
        .unwrap_or_default()
        .to_string()
}

fn video_id(renderer: &Value) -> String {
    renderer
        .pointer("/playlistItemData/videoId")
        .or_else(|| renderer.pointer("/navigationEndpoint/watchEndpoint/videoId"))
        .and_then(Value::as_str)
        .unwrap_or_default()
        .to_string()
}

fn duration_ms(value: &str) -> i64 {
    value
        .split(':')
        .try_fold(0_i64, |total, part| {
            part.trim()
                .parse::<i64>()
                .ok()
                .map(|number| total.saturating_mul(60).saturating_add(number))
        })
        .unwrap_or(0)
        .saturating_mul(1000)
}

fn search_category(title: &str) -> &'static str {
    let title = title.to_lowercase();
    if title.contains("canción") || title.contains("cancion") || title.contains("song") {
        "songs"
    } else if title.contains("video") {
        "videos"
    } else if title.contains("álbum") || title.contains("album") {
        "albums"
    } else if title.contains("artista") || title.contains("artist") {
        "artists"
    } else if title.contains("playlist") || title.contains("lista") {
        "playlists"
    } else {
        "songs"
    }
}

fn song_metadata(subtitle: &str) -> (String, String) {
    let parts = subtitle
        .split('•')
        .map(str::trim)
        .filter(|part| {
            !matches!(
                part.to_lowercase().as_str(),
                "canción"
                    | "cancion"
                    | "song"
                    | "video"
                    | "sencillo"
                    | "single"
                    | "artista"
                    | "artist"
            )
        })
        .collect::<Vec<_>>();
    let artist = parts.first().copied().unwrap_or_default().to_string();
    let album = parts
        .get(1)
        .filter(|part| !part.contains(':'))
        .copied()
        .unwrap_or_default()
        .to_string();
    (artist, album)
}

fn continuation_token(value: &Value) -> Option<String> {
    match value {
        Value::Object(map) => {
            if let Some(token) = map
                .get("continuationCommand")
                .and_then(|command| command.get("token"))
                .and_then(Value::as_str)
            {
                return Some(token.to_string());
            }
            if let Some(token) = map
                .get("nextContinuationData")
                .or_else(|| map.get("reloadContinuationData"))
                .and_then(|data| data.get("continuation"))
                .and_then(Value::as_str)
            {
                return Some(token.to_string());
            }
            map.values().find_map(continuation_token)
        }
        Value::Array(items) => items.iter().find_map(continuation_token),
        _ => None,
    }
}

fn search_sections(json: &Value) -> Option<&Vec<Value>> {
    json.pointer("/contents/tabbedSearchResultsRenderer/tabs/0/tabRenderer/content/sectionListRenderer/contents")
        .or_else(|| json.pointer("/continuationContents/musicShelfContinuation/contents"))
        .and_then(Value::as_array)
}

pub(crate) fn parse_search_page(
    json: &Value,
    query: &str,
    fallback_category: &str,
) -> Result<ParsedPage<SearchResults>, String> {
    let sections = search_sections(json).ok_or_else(|| {
        schema_error(
            "search",
            "sectionListRenderer.contents or musicShelfContinuation.contents",
            json,
        )
    })?;
    let mut result = SearchResults {
        query: query.to_string(),
        songs: Vec::new(),
        videos: Vec::new(),
        albums: Vec::new(),
        artists: Vec::new(),
        playlists: Vec::new(),
    };
    for section in sections {
        let (category, items) = if section.get("musicResponsiveListItemRenderer").is_some() {
            (fallback_category, vec![section])
        } else {
            let Some(shelf) = section
                .get("musicShelfRenderer")
                .or_else(|| section.get("musicCardShelfRenderer"))
            else {
                continue;
            };
            let section_title = runs(&shelf["title"]);
            let category = if section_title.is_empty() {
                fallback_category
            } else {
                search_category(&section_title)
            };
            let Some(items) = shelf["contents"].as_array() else {
                continue;
            };
            (category, items.iter().collect())
        };
        for item in items {
            let renderer = &item["musicResponsiveListItemRenderer"];
            if renderer.is_null() {
                continue;
            }
            let title = runs(
                &renderer["flexColumns"][0]["musicResponsiveListItemFlexColumnRenderer"]["text"],
            );
            if title.is_empty() {
                continue;
            }
            let subtitle = renderer["flexColumns"]
                .as_array()
                .and_then(|columns| columns.get(1))
                .map(|column| runs(&column["musicResponsiveListItemFlexColumnRenderer"]["text"]))
                .unwrap_or_default();
            let image = thumbnail(renderer);
            let browse_id = renderer
                .pointer("/navigationEndpoint/browseEndpoint/browseId")
                .and_then(Value::as_str)
                .unwrap_or_default()
                .to_string();
            match category {
                "songs" => {
                    let id = video_id(renderer);
                    if id.is_empty() {
                        continue;
                    }
                    let (artist, album) = song_metadata(&subtitle);
                    result.songs.push(Track {
                        id,
                        title,
                        artists: vec![artist],
                        album: Some(album),
                        album_id: None,
                        duration_ms: 0,
                        thumbnail: image,
                        stream_url: None,
                    });
                }
                "videos" => {
                    let id = video_id(renderer);
                    if id.is_empty() {
                        continue;
                    }
                    result.videos.push(Track {
                        id,
                        title,
                        artists: vec![subtitle],
                        album: None,
                        album_id: None,
                        duration_ms: 0,
                        thumbnail: image,
                        stream_url: None,
                    });
                }
                "albums" if !browse_id.is_empty() => result.albums.push(Album {
                    id: browse_id,
                    title,
                    artists: vec![subtitle],
                    year: None,
                    thumbnail: image,
                    track_count: None,
                }),
                "artists" if !browse_id.is_empty() => result.artists.push(Artist {
                    id: browse_id,
                    name: title,
                    thumbnail: image,
                    subscriber_count: None,
                }),
                "playlists" if !browse_id.is_empty() => result.playlists.push(Playlist {
                    id: browse_id,
                    title,
                    description: None,
                    owner: Some(subtitle),
                    thumbnail: image,
                    track_count: None,
                }),
                _ => {}
            }
        }
    }
    Ok(ParsedPage {
        items: result,
        continuation: continuation_token(json),
    })
}

pub fn parse_search(json: &Value, query: &str) -> Result<SearchResults, String> {
    parse_search_page(json, query, "songs").map(|page| page.items)
}

fn collect_suggestions(value: &Value, suggestions: &mut Vec<String>, seen: &mut HashSet<String>) {
    match value {
        Value::Object(map) => {
            if let Some(renderer) = map.get("searchSuggestionRenderer") {
                let suggestion = renderer.get("suggestion").map(runs).unwrap_or_default();
                if !suggestion.is_empty() && seen.insert(suggestion.to_lowercase()) {
                    suggestions.push(suggestion);
                }
            }
            for child in map.values() {
                collect_suggestions(child, suggestions, seen);
            }
        }
        Value::Array(items) => {
            for child in items {
                collect_suggestions(child, suggestions, seen);
            }
        }
        _ => {}
    }
}

pub fn parse_search_suggestions(json: &Value) -> Result<Vec<String>, String> {
    if !json.is_object() {
        return Err(schema_error(
            "music/get_search_suggestions",
            "an object containing searchSuggestionRenderer entries",
            json,
        ));
    }
    let mut suggestions = Vec::new();
    let mut seen = HashSet::new();
    collect_suggestions(json, &mut suggestions, &mut seen);
    Ok(suggestions)
}

pub(crate) fn parse_home_page(json: &Value) -> Result<ParsedPage<Vec<HomeSection>>, String> {
    let contents = json.pointer("/contents/singleColumnBrowseResultsRenderer/tabs/0/tabRenderer/content/sectionListRenderer/contents")
        .or_else(|| json.pointer("/continuationContents/sectionListContinuation/contents"))
        .and_then(Value::as_array)
        .ok_or_else(|| {
            schema_error(
                "browse/home",
                "sectionListRenderer.contents or sectionListContinuation.contents",
                json,
            )
        })?;
    let mut sections = Vec::new();
    for item in contents {
        let Some(shelf) = item
            .get("musicCarouselShelfRenderer")
            .or_else(|| item.get("musicDescriptionShelfRenderer"))
        else {
            continue;
        };
        let title = shelf
            .pointer("/header/musicCarouselShelfBasicHeaderRenderer/title")
            .or_else(|| shelf.pointer("/header/musicDescriptionShelfRenderer/title"))
            .map(runs)
            .unwrap_or_default();
        let mut items = Vec::new();
        for child in shelf["contents"].as_array().into_iter().flatten() {
            let Some(renderer) = child
                .get("musicResponsiveListItemRenderer")
                .or_else(|| child.get("musicTwoRowItemRenderer"))
            else {
                continue;
            };
            let item_title = renderer
                .pointer("/flexColumns/0/musicResponsiveListItemFlexColumnRenderer/text")
                .map(runs)
                .filter(|value| !value.is_empty())
                .unwrap_or_else(|| text(&renderer["title"]));
            if item_title.is_empty() {
                continue;
            }
            let subtitle = renderer
                .pointer("/flexColumns/1/musicResponsiveListItemFlexColumnRenderer/text")
                .map(runs)
                .filter(|value| !value.is_empty())
                .unwrap_or_else(|| runs(&renderer["subtitle"]));
            let browse_id = renderer
                .pointer("/navigationEndpoint/browseEndpoint/browseId")
                .and_then(Value::as_str)
                .map(str::to_string);
            let playlist_id = renderer
                .pointer("/navigationEndpoint/watchPlaylistEndpoint/playlistId")
                .and_then(Value::as_str)
                .map(str::to_string);
            let item_type = if renderer.get("playlistItemData").is_some() {
                "song"
            } else if browse_id
                .as_deref()
                .map(|id| id.starts_with("MPRE"))
                .unwrap_or(false)
            {
                "album"
            } else {
                "playlist"
            };
            items.push(HomeItem {
                title: item_title,
                subtitle,
                thumbnail: thumbnail(renderer),
                item_type: item_type.to_string(),
                browse_id,
                playlist_id,
            });
        }
        if !title.is_empty() || !items.is_empty() {
            sections.push(HomeSection {
                title,
                items,
                browse_id: None,
            });
        }
    }
    Ok(ParsedPage {
        items: sections,
        continuation: continuation_token(json),
    })
}

pub fn parse_home(json: &Value) -> Result<Vec<HomeSection>, String> {
    parse_home_page(json).map(|page| page.items)
}

fn collect_related(value: &Value, tracks: &mut Vec<Track>, seen: &mut HashSet<String>) {
    match value {
        Value::Object(map) => {
            if let Some(renderer) = map.get("playlistPanelVideoRenderer") {
                let id = renderer["videoId"].as_str().unwrap_or_default();
                let title = text(&renderer["title"]);
                if !id.is_empty() && !title.is_empty() && seen.insert(id.to_string()) {
                    let byline = renderer
                        .get("longBylineText")
                        .or_else(|| renderer.get("shortBylineText"))
                        .map(text)
                        .unwrap_or_default();
                    let artist = byline
                        .split('•')
                        .next()
                        .unwrap_or_default()
                        .trim()
                        .to_string();
                    tracks.push(Track {
                        id: id.to_string(),
                        title,
                        artists: if artist.is_empty() {
                            Vec::new()
                        } else {
                            vec![artist]
                        },
                        album: None,
                        album_id: None,
                        duration_ms: duration_ms(&text(&renderer["lengthText"])),
                        thumbnail: thumbnail(renderer),
                        stream_url: None,
                    });
                }
            }
            for child in map.values() {
                collect_related(child, tracks, seen);
            }
        }
        Value::Array(items) => {
            for child in items {
                collect_related(child, tracks, seen);
            }
        }
        _ => {}
    }
}

pub fn parse_related(json: &Value, seed_video_id: &str) -> Result<Vec<Track>, String> {
    if !json.is_object() {
        return Err(schema_error(
            "next/related",
            "an object containing playlistPanelVideoRenderer entries",
            json,
        ));
    }
    let mut tracks = Vec::new();
    let mut seen = HashSet::from([seed_video_id.to_string()]);
    collect_related(json, &mut tracks, &mut seen);
    Ok(tracks)
}

pub(crate) fn related_continuation(json: &Value) -> Option<String> {
    continuation_token(json)
}

#[cfg(test)]
mod tests {
    use super::*;

    fn fixture(contents: &str) -> Value {
        serde_json::from_str(contents).unwrap()
    }

    #[test]
    fn related_filters_seed_invalid_and_duplicates() {
        let fixture = fixture(include_str!("fixtures/related_tracks.json"));
        let tracks = parse_related(&fixture, "video-anonymous-seed").unwrap();
        assert_eq!(tracks.len(), 1);
        assert_eq!(tracks[0].id, "video-anonymous-next");
        assert_eq!(tracks[0].duration_ms, 201_000);
    }

    #[test]
    fn parsers_report_missing_top_level_schema() {
        let search = parse_search(&serde_json::json!({"unexpected": {}}), "query").unwrap_err();
        assert!(search.contains("Innertube search schema changed"));
        assert!(search.contains("sectionListRenderer.contents"));
        assert!(search.contains("unexpected"));

        let home = parse_home(&serde_json::json!({})).unwrap_err();
        assert!(home.contains("Innertube browse/home schema changed"));
        assert!(home.contains("empty object"));

        let related = parse_related(&serde_json::json!([]), "seed").unwrap_err();
        assert!(related.contains("Innertube next/related schema changed"));
        assert!(related.contains("array with 0 items"));
    }

    #[test]
    fn parses_search_continuation_and_its_next_token() {
        let fixture = fixture(include_str!("fixtures/search_continuation.json"));
        let page = parse_search_page(&fixture, "query", "songs").unwrap();
        assert_eq!(page.items.songs[0].id, "video-anonymous-2");
        assert_eq!(page.continuation.as_deref(), Some("token-anonymous-next"));
    }

    #[test]
    fn parses_home_continuation_sections() {
        let fixture = fixture(include_str!("fixtures/home_continuation.json"));
        let page = parse_home_page(&fixture).unwrap();
        assert_eq!(page.items[0].title, "Anonymous recommendations");
    }

    #[test]
    fn parses_and_deduplicates_search_suggestions() {
        let fixture = fixture(include_str!("fixtures/search_suggestions.json"));
        let suggestions = parse_search_suggestions(&fixture).unwrap();
        assert_eq!(
            suggestions,
            vec!["anonymous suggestion one", "anonymous suggestion two"]
        );
    }
}
