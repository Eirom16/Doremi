use super::models::{
    Album, Artist, ArtistDetail, Episode, HomeItem, HomeSection, LikeStatus, Playlist,
    PlaylistDetail, RemoteHistoryItem, SearchResults, Show, ShowDetail, TopResultItem, Track,
};
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

fn thumbnail_from_array(value: &Value) -> Option<String> {
    value
        .as_array()
        .and_then(|items| items.last())
        .and_then(|item| item["url"].as_str())
        .map(str::to_string)
}

fn find_thumbnail(value: &Value) -> Option<String> {
    match value {
        Value::Object(map) => {
            if let Some(thumbnail) = map.get("thumbnails").and_then(thumbnail_from_array) {
                return Some(thumbnail);
            }
            map.values().find_map(find_thumbnail)
        }
        Value::Array(items) => items.iter().find_map(find_thumbnail),
        _ => None,
    }
}

fn thumbnail(renderer: &Value) -> String {
    renderer
        .pointer("/thumbnail/musicThumbnailRenderer/thumbnail/thumbnails")
        .or_else(|| renderer.pointer("/thumbnail/thumbnails"))
        .or_else(|| {
            renderer.pointer("/thumbnailRenderer/musicThumbnailRenderer/thumbnail/thumbnails")
        })
        .or_else(|| renderer.pointer("/thumbnailRenderer/playlistVideoThumbnailRenderer/thumbnail/thumbnails"))
        .and_then(thumbnail_from_array)
        .or_else(|| find_thumbnail(renderer))
        .unwrap_or_default()
}

fn video_id(renderer: &Value) -> String {
    renderer
        .pointer("/playlistItemData/videoId")
        .or_else(|| renderer.pointer("/navigationEndpoint/watchEndpoint/videoId"))
        .or_else(|| renderer.pointer("/videoId"))
        .or_else(|| {
            renderer.pointer("/title/runs/0/navigationEndpoint/watchEndpoint/videoId")
        })
        .and_then(Value::as_str)
        .map(str::to_string)
        .or_else(|| find_string_field(renderer, "videoId"))
        .unwrap_or_default()
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

fn first_year(value: &Value) -> Option<i32> {
    value["runs"].as_array().and_then(|items| {
        items.iter().find_map(|item| {
            item["text"]
                .as_str()
                .filter(|text| {
                    text.len() == 4 && text.chars().all(|character| character.is_ascii_digit())
                })
                .and_then(|text| text.parse().ok())
        })
    })
}

fn first_number(value: &Value) -> Option<i32> {
    text(value)
        .split_whitespace()
        .find_map(|part| part.replace(',', "").parse().ok())
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
    } else if title.contains("episodio") || title.contains("episode") {
        "episodes"
    } else if title.contains("podcast") || title.contains("show") {
        "shows"
    } else {
        "songs"
    }
}

fn is_metadata_noise(value: &str) -> bool {
    let value = value.trim().to_lowercase();
    if value.is_empty() {
        return true;
    }
    matches!(
        value.as_str(),
        "canción"
            | "cancion"
            | "song"
            | "video"
            | "sencillo"
            | "single"
            | "artista"
            | "artist"
            | "álbum"
            | "album"
            | "playlist"
            | "podcast"
            | "show"
    ) || value.contains("visualizaci")
        || value.contains("views")
        || value.contains("reproducciones")
        || value.contains("subscribers")
        || value.contains("suscriptores")
}

fn clean_metadata_parts(subtitle: &str) -> Vec<&str> {
    subtitle
        .split('•')
        .map(str::trim)
        .filter(|part| !is_metadata_noise(part))
        .collect()
}

fn contains_home_token(value: &str, tokens: &[&str]) -> bool {
    let value = value.to_lowercase();
    tokens.iter().any(|token| value.contains(token))
}

fn normalize_playlist_id(value: &str) -> String {
    value
        .trim()
        .strip_prefix("VL")
        .unwrap_or(value.trim())
        .to_string()
}

fn home_item_type(
    title: &str,
    subtitle: &str,
    browse_id: Option<&str>,
    playlist_id: Option<&str>,
    video_id: Option<&str>,
) -> &'static str {
    if contains_home_token(subtitle, &["episodio", "episode"]) {
        return "episode";
    }
    if video_id.is_some() {
        return "song";
    }
    if contains_home_token(&format!("{title} {subtitle}"), &["podcast", "show"]) {
        return "show";
    }

    if let Some(id) = browse_id {
        if id.starts_with("MPRE") {
            return "album";
        }
        if id.starts_with("UC") {
            return "artist";
        }
        if id.starts_with("MPSP") {
            return "show";
        }
        if id.starts_with("VL") {
            if contains_home_token(&format!("{title} {subtitle}"), &["mix", "radio"]) {
                return "mix";
            }
            return "playlist";
        }
    }

    if let Some(id) = playlist_id {
        if id.starts_with("RD")
            || contains_home_token(&format!("{title} {subtitle}"), &["mix", "radio"])
        {
            return "mix";
        }
        return "playlist";
    }

    "playlist"
}

fn is_probable_video_id(id: &str) -> bool {
    !id.is_empty()
        && !id.starts_with("UC")
        && id
            .chars()
            .all(|ch| ch.is_ascii_alphanumeric() || ch == '_' || ch == '-')
}

fn song_metadata(subtitle: &str) -> (String, String) {
    let parts = clean_metadata_parts(subtitle);
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

fn find_renderer_with_items(value: &Value) -> Option<&Value> {
    match value {
        Value::Object(map) => {
            for key in [
                "gridRenderer",
                "musicShelfRenderer",
                "musicPlaylistShelfRenderer",
                "gridContinuation",
                "musicShelfContinuation",
                "musicPlaylistShelfContinuation",
            ] {
                if let Some(candidate) = map.get(key) {
                    if candidate
                        .get("items")
                        .or_else(|| candidate.get("contents"))
                        .and_then(Value::as_array)
                        .is_some()
                    {
                        return Some(candidate);
                    }
                }
            }
            map.values().find_map(find_renderer_with_items)
        }
        Value::Array(items) => items.iter().find_map(find_renderer_with_items),
        _ => None,
    }
}

fn has_item_array(value: &Value) -> bool {
    value
        .get("items")
        .or_else(|| value.get("contents"))
        .or_else(|| value.get("continuationItems"))
        .and_then(Value::as_array)
        .is_some()
}

fn find_library_contents(json: &Value) -> Option<&Value> {
    if let Some(grid) = json.get("gridContinuation") {
        return Some(grid);
    }
    if let Some(shelf) = json.get("musicShelfContinuation") {
        return Some(shelf);
    }

    // Support continuationContents wrapping
    if let Some(cont) = json.get("continuationContents") {
        if let Some(grid) = cont.get("gridContinuation") {
            return Some(grid);
        }
        if let Some(shelf) = cont.get("musicShelfContinuation") {
            return Some(shelf);
        }
        if let Some(shelf) = cont.get("musicPlaylistShelfContinuation") {
            return Some(shelf);
        }
        if let Some(shelf) = cont.get("sectionListContinuation") {
            return Some(shelf);
        }
    }

    if let Some(contents) = json
        .pointer("/continuationContents/musicShelfContinuation")
        .or_else(|| json.pointer("/continuationContents/sectionListContinuation"))
        .or_else(|| json.pointer("/continuationContents/gridContinuation"))
        .or_else(|| json.pointer("/onResponseReceivedActions/0/appendContinuationItemsAction"))
    {
        return Some(contents);
    }

    for col_type in &[
        "singleColumnBrowseResultsRenderer",
        "twoColumnBrowseResultsRenderer",
    ] {
        for tab_idx in 0..4 {
            let path = format!(
                "/contents/{}/tabs/{}/tabRenderer/content/sectionListRenderer/contents",
                col_type, tab_idx
            );
            if let Some(contents) = json.pointer(&path).and_then(Value::as_array) {
                for item in contents {
                    if let Some(grid) = item.get("gridRenderer") {
                        if has_item_array(grid) {
                            return Some(grid);
                        }
                    }
                    if let Some(shelf) = item.get("musicShelfRenderer") {
                        if has_item_array(shelf) {
                            return Some(shelf);
                        }
                    }
                    if let Some(item_contents) = item.pointer("/itemSectionRenderer/contents")
                        .and_then(Value::as_array) {
                        for sub_item in item_contents {
                            if let Some(grid) = sub_item.get("gridRenderer") {
                                if has_item_array(grid) {
                                    return Some(grid);
                                }
                            }
                            if let Some(shelf) = sub_item.get("musicShelfRenderer") {
                                if has_item_array(shelf) {
                                    return Some(shelf);
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    if let Some(contents) = json.pointer("/contents/sectionListRenderer/contents").and_then(Value::as_array) {
        for item in contents {
            if let Some(grid) = item.get("gridRenderer") {
                if has_item_array(grid) {
                    return Some(grid);
                }
            }
            if let Some(shelf) = item.get("musicShelfRenderer") {
                if has_item_array(shelf) {
                    return Some(shelf);
                }
            }
        }
    }
    find_renderer_with_items(json)
}

fn collect_renderer_items<'a>(value: &'a Value, renderer_key: &str, items: &mut Vec<&'a Value>) {
    match value {
        Value::Object(map) => {
            if let Some(renderer) = map.get(renderer_key) {
                items.push(renderer);
            }
            for child in map.values() {
                collect_renderer_items(child, renderer_key, items);
            }
        }
        Value::Array(children) => {
            for child in children {
                collect_renderer_items(child, renderer_key, items);
            }
        }
        _ => {}
    }
}

fn shelf_items<'a>(json: &'a Value, endpoint: &str) -> Result<Vec<&'a Value>, String> {
    let shelf = find_library_contents(json).ok_or_else(|| {
        let keys = json.get("continuationContents")
            .and_then(|c| c.as_object())
            .map(|obj| {
                let inner_keys = obj.values().next()
                    .and_then(|v| v.as_object())
                    .map(|v_obj| v_obj.keys().map(|s| s.as_str()).collect::<Vec<_>>().join(", "))
                    .unwrap_or_default();
                format!("{} (inner keys: [{}])", obj.keys().map(|s| s.as_str()).collect::<Vec<_>>().join(", "), inner_keys)
            })
            .unwrap_or_default();
        schema_error(
            endpoint,
            &format!("gridRenderer, musicShelfRenderer, or nested item renderers. continuationContents keys: [{}]", keys),
            json,
        )
    })?;
    
    if shelf.get("items").is_none() && shelf.get("contents").is_none() && shelf.get("continuationItems").is_none() {
        return Ok(Vec::new());
    }

    shelf
        .get("items")
        .or_else(|| shelf.get("contents"))
        .or_else(|| shelf.get("continuationItems"))
        .and_then(Value::as_array)
        .map(|items| items.iter().collect())
        .ok_or_else(|| schema_error(endpoint, "items, contents, or continuationItems", json))
}

fn parse_duration(s: &str) -> Option<i64> {
    let parts: Vec<&str> = s.trim().split(':').collect();
    match parts.len() {
        2 => {
            let mins = parts[0].parse::<i64>().ok()?;
            let secs = parts[1].parse::<i64>().ok()?;
            Some((mins * 60 + secs) * 1000)
        }
        3 => {
            let hrs = parts[0].parse::<i64>().ok()?;
            let mins = parts[1].parse::<i64>().ok()?;
            let secs = parts[2].parse::<i64>().ok()?;
            Some((hrs * 3600 + mins * 60 + secs) * 1000)
        }
        _ => None,
    }
}

fn parse_item_common(
    renderer: &Value,
) -> Option<(String, Option<String>, Option<String>, Option<String>)> {
    let title = renderer
        .pointer("/flexColumns/0/musicResponsiveListItemFlexColumnRenderer/text")
        .map(runs)
        .filter(|value| !value.is_empty())
        .or_else(|| renderer.pointer("/title").map(runs))
        .filter(|value| !value.is_empty())?;

    let subtitle = renderer
        .pointer("/flexColumns/1/musicResponsiveListItemFlexColumnRenderer/text")
        .map(runs)
        .filter(|value| !value.is_empty())
        .or_else(|| renderer.pointer("/subtitle").map(runs))
        .filter(|value| !value.is_empty());

    let browse_id = renderer
        .pointer("/navigationEndpoint/browseEndpoint/browseId")
        .or_else(|| renderer.pointer("/title/runs/0/navigationEndpoint/browseEndpoint/browseId"))
        .and_then(Value::as_str)
        .map(str::to_string);

    let playlist_id = renderer
        .pointer("/navigationEndpoint/watchPlaylistEndpoint/playlistId")
        .or_else(|| renderer.pointer("/navigationEndpoint/watchEndpoint/playlistId"))
        .and_then(Value::as_str)
        .map(str::to_string);

    Some((title, subtitle, browse_id, playlist_id))
}

fn parse_library_playlist(renderer: &Value) -> Option<Playlist> {
    let (title, subtitle, browse_id, playlist_id) = parse_item_common(renderer)?;
    let id = playlist_id
        .or_else(|| browse_id.map(|id| id.strip_prefix("VL").unwrap_or(&id).to_string()))?;
    let track_count = subtitle.as_ref().and_then(|sub| {
        sub.split_whitespace()
            .find_map(|part| part.replace(',', "").parse::<i32>().ok())
    });
    Some(Playlist {
        id,
        title,
        description: None,
        owner: subtitle,
        thumbnail: thumbnail(renderer),
        track_count,
    })
}

fn parse_library_album(renderer: &Value) -> Option<Album> {
    let (title, subtitle, browse_id, _) = parse_item_common(renderer)?;
    let id = browse_id?;
    if !id.starts_with("MPRE") {
        return None;
    }

    let mut artists = Vec::new();
    let mut artist_id = None;
    let mut year = None;
    if let Some(runs) = renderer.pointer("/subtitle/runs").and_then(Value::as_array) {
        for run in runs {
            if let Some(text) = run["text"].as_str() {
                let text = text.trim();
                if text == "•" || text.is_empty() {
                    continue;
                }
                if text.len() == 4 && text.chars().all(|c| c.is_ascii_digit()) {
                    year = text.parse::<i32>().ok();
                } else if text != "Álbum"
                    && text != "Album"
                    && text != "Sencillo"
                    && text != "Single"
                    && text != "EP"
                {
                    artists.push(text.to_string());
                    if artist_id.is_none() {
                        artist_id = run
                            .pointer("/navigationEndpoint/browseEndpoint/browseId")
                            .and_then(Value::as_str)
                            .map(String::from);
                    }
                }
            }
        }
    }

    if artists.is_empty() {
        if let Some(sub) = subtitle {
            artists.push(sub);
        }
    }

    Some(Album {
        id,
        title,
        artists,
        year,
        thumbnail: thumbnail(renderer),
        track_count: None,
        artist_id,
    })
}

fn parse_library_artist(renderer: &Value) -> Option<Artist> {
    let (name, subtitle, browse_id, _) = parse_item_common(renderer)?;
    let id = browse_id?;
    let subscriber_count = subtitle.and_then(|sub| {
        let parts = sub.split_whitespace().collect::<Vec<_>>();
        parts.first().map(|s| s.to_string())
    });
    Some(Artist {
        id,
        name,
        thumbnail: thumbnail(renderer),
        subscriber_count,
    })
}

pub(crate) fn parse_library_playlists(json: &Value) -> Result<ParsedPage<Vec<Playlist>>, String> {
    let mut fallback_renderers = Vec::new();
    let items = match shelf_items(json, "library/playlists") {
        Ok(items) => items,
        Err(e) => {
            collect_renderer_items(json, "musicTwoRowItemRenderer", &mut fallback_renderers);
            collect_renderer_items(
                json,
                "musicResponsiveListItemRenderer",
                &mut fallback_renderers,
            );
            let mut message_renderers = Vec::new();
            collect_renderer_items(json, "messageRenderer", &mut message_renderers);
            
            if fallback_renderers.is_empty() && message_renderers.is_empty() {
                return Err(e);
            }
            Vec::new()
        }
    };

    let mut playlists = Vec::new();
    let start_idx = if items
        .first()
        .and_then(|item| {
            item.pointer("/musicTwoRowItemRenderer/navigationEndpoint/createPlaylistEndpoint")
                .or_else(|| {
                    item.pointer(
                    "/musicResponsiveListItemRenderer/navigationEndpoint/createPlaylistEndpoint",
                )
                })
        })
        .is_some()
    {
        1
    } else {
        0
    };

    for item in items[start_idx..].iter() {
        let renderer = item
            .get("musicTwoRowItemRenderer")
            .or_else(|| item.get("musicResponsiveListItemRenderer"))
            .unwrap_or(item);
        if let Some(playlist) = parse_library_playlist(renderer) {
            playlists.push(playlist);
        }
    }
    for renderer in fallback_renderers {
        if let Some(playlist) = parse_library_playlist(renderer) {
            playlists.push(playlist);
        }
    }

    let empty = serde_json::json!({});
    Ok(ParsedPage {
        items: playlists,
        continuation: continuation_token(find_library_contents(json).unwrap_or(&empty)),
    })
}

pub(crate) fn parse_library_songs_page(json: &Value) -> Result<ParsedPage<Vec<Track>>, String> {
    let tracks = match shelf_items(json, "library/songs") {
        Ok(items) => {
            let owned_items = items.into_iter().cloned().collect::<Vec<_>>();
            parse_playlist_tracks(&owned_items).0
        }
        Err(_) => {
            let mut renderers = Vec::new();
            collect_renderer_items(json, "musicResponsiveListItemRenderer", &mut renderers);
            let tracks = renderers
                .into_iter()
                .filter(|renderer| {
                    renderer["musicItemRendererDisplayPolicy"]
                        != "MUSIC_ITEM_RENDERER_DISPLAY_POLICY_GREY_OUT"
                })
                .filter_map(parse_track_renderer)
                .collect::<Vec<_>>();
            tracks
        }
    };

    Ok(ParsedPage {
        items: tracks,
        continuation: continuation_token(json),
    })
}

pub(crate) fn parse_library_albums(json: &Value) -> Result<ParsedPage<Vec<Album>>, String> {
    let mut fallback_renderers = Vec::new();
    let items = match shelf_items(json, "library/albums") {
        Ok(items) => items,
        Err(e) => {
            collect_renderer_items(json, "musicTwoRowItemRenderer", &mut fallback_renderers);
            collect_renderer_items(
                json,
                "musicResponsiveListItemRenderer",
                &mut fallback_renderers,
            );
            let mut message_renderers = Vec::new();
            collect_renderer_items(json, "messageRenderer", &mut message_renderers);
            
            if fallback_renderers.is_empty() && message_renderers.is_empty() {
                return Err(e);
            }
            Vec::new()
        }
    };

    let mut albums = Vec::new();
    for item in items {
        let renderer = item
            .get("musicTwoRowItemRenderer")
            .or_else(|| item.get("musicResponsiveListItemRenderer"))
            .unwrap_or(item);
        if let Some(album) = parse_library_album(renderer) {
            albums.push(album);
        }
    }
    for renderer in fallback_renderers {
        if let Some(album) = parse_library_album(renderer) {
            albums.push(album);
        }
    }

    let empty = serde_json::json!({});
    Ok(ParsedPage {
        items: albums,
        continuation: continuation_token(find_library_contents(json).unwrap_or(&empty)),
    })
}

pub(crate) fn parse_library_artists(json: &Value) -> Result<ParsedPage<Vec<Artist>>, String> {
    let shelf = match find_library_contents(json) {
        Some(shelf) => shelf,
        None => {
            let mut message_renderers = Vec::new();
            collect_renderer_items(json, "messageRenderer", &mut message_renderers);
            if !message_renderers.is_empty() {
                return Ok(ParsedPage {
                    items: Vec::new(),
                    continuation: None,
                });
            }
            return Err(schema_error(
                "library/artists",
                "gridRenderer or musicShelfRenderer",
                json,
            ));
        }
    };
    let items = shelf
        .get("items")
        .or_else(|| shelf.get("contents"))
        .and_then(Value::as_array)
        .ok_or_else(|| schema_error("library/artists", "items or contents", json))?;

    let mut artists = Vec::new();
    for item in items {
        let renderer = item
            .get("musicTwoRowItemRenderer")
            .or_else(|| item.get("musicResponsiveListItemRenderer"))
            .unwrap_or(item);
        if let Some(artist) = parse_library_artist(renderer) {
            artists.push(artist);
        }
    }

    Ok(ParsedPage {
        items: artists,
        continuation: continuation_token(shelf),
    })
}

fn search_sections(json: &Value) -> Option<&Vec<Value>> {
    json.pointer("/contents/tabbedSearchResultsRenderer/tabs/0/tabRenderer/content/sectionListRenderer/contents")
        .or_else(|| json.pointer("/continuationContents/musicShelfContinuation/contents"))
        .or_else(|| json.pointer("/continuationContents/sectionListContinuation/contents"))
        .and_then(Value::as_array)
}

fn parse_top_result(card: &Value) -> Option<TopResultItem> {
    let subtitle_runs = card.pointer("/subtitle/runs").and_then(Value::as_array)?;
    let type_str = subtitle_runs.first()?.get("text")?.as_str()?;
    let type_lower = type_str.to_lowercase();
    let subtitle_text = subtitle_runs
        .iter()
        .filter_map(|run| run["text"].as_str())
        .collect::<Vec<_>>()
        .join(" ");
    let metadata = clean_metadata_parts(&subtitle_text);

    if type_lower.contains("artist") || type_lower.contains("artista") {
        let id = card
            .pointer("/onTap/browseEndpoint/browseId")
            .and_then(Value::as_str)?
            .to_string();
        let name = runs(&card["title"]);
        let thumbnail = thumbnail(card);
        let subscriber_count = metadata.first().map(|value| value.to_string());
        Some(TopResultItem::Artist(Artist {
            id,
            name,
            thumbnail,
            subscriber_count,
        }))
    } else if type_lower.contains("album") || type_lower.contains("álbum") {
        let id = card
            .pointer("/onTap/browseEndpoint/browseId")
            .and_then(Value::as_str)?
            .to_string();
        let title = runs(&card["title"]);
        let artist = metadata.first().copied().unwrap_or_default().to_string();
        let artist_id = if subtitle_runs.len() >= 3 {
            subtitle_runs[2]
                .pointer("/navigationEndpoint/browseEndpoint/browseId")
                .and_then(Value::as_str)
                .map(String::from)
        } else {
            None
        };
        let year = if subtitle_runs.len() >= 5 {
            subtitle_runs[4]["text"]
                .as_str()
                .and_then(|s| s.parse::<i32>().ok())
        } else {
            None
        };
        let thumbnail = thumbnail(card);
        Some(TopResultItem::Album(Album {
            id,
            title,
            artists: vec![artist],
            year,
            thumbnail,
            track_count: None,
            artist_id,
        }))
    } else if type_lower.contains("song") || type_lower.contains("canci") {
        let id = card
            .pointer("/onTap/watchEndpoint/videoId")
            .and_then(Value::as_str)?
            .to_string();
        if !is_probable_video_id(&id) {
            return None;
        }
        let title = runs(&card["title"]);
        let artist = metadata.first().copied().unwrap_or_default().to_string();
        let album = metadata
            .get(1)
            .filter(|part| !part.contains(':'))
            .map(|value| value.to_string());
        let thumbnail = thumbnail(card);
        Some(TopResultItem::Track(Track {
            id,
            title,
            artists: vec![artist],
            album,
            album_id: None,
            duration_ms: 0,
            thumbnail,
            stream_url: None,
        }))
    } else if type_lower.contains("video") {
        let id = card
            .pointer("/onTap/watchEndpoint/videoId")
            .and_then(Value::as_str)?
            .to_string();
        if !is_probable_video_id(&id) {
            return None;
        }
        let title = runs(&card["title"]);
        let artist = metadata.first().copied().unwrap_or_default().to_string();
        let thumbnail = thumbnail(card);
        Some(TopResultItem::Track(Track {
            id,
            title,
            artists: vec![artist],
            album: None,
            album_id: None,
            duration_ms: 0,
            thumbnail,
            stream_url: None,
        }))
    } else if type_lower.contains("playlist") {
        let id = card
            .pointer("/onTap/browseEndpoint/browseId")
            .and_then(Value::as_str)?
            .to_string();
        let title = runs(&card["title"]);
        let owner = metadata.first().map(|value| value.to_string());
        let thumbnail = thumbnail(card);
        Some(TopResultItem::Playlist(Playlist {
            id,
            title,
            description: None,
            owner,
            thumbnail,
            track_count: None,
        }))
    } else {
        None
    }
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
        top_result: None,
        songs: Vec::new(),
        videos: Vec::new(),
        albums: Vec::new(),
        artists: Vec::new(),
        playlists: Vec::new(),
        shows: Vec::new(),
        episodes: Vec::new(),
    };
    for section in sections {
        if let Some(card) = section.get("musicCardShelfRenderer") {
            result.top_result = parse_top_result(card);
        }
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
                    if !is_probable_video_id(&id) {
                        if browse_id.starts_with("UC") {
                            result.artists.push(Artist {
                                id: browse_id.clone(),
                                name: title,
                                thumbnail: image,
                                subscriber_count: clean_metadata_parts(&subtitle)
                                    .first()
                                    .map(|value| value.to_string()),
                            });
                        }
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
                    if !is_probable_video_id(&id) {
                        if browse_id.starts_with("UC") {
                            result.artists.push(Artist {
                                id: browse_id.clone(),
                                name: title,
                                thumbnail: image,
                                subscriber_count: clean_metadata_parts(&subtitle)
                                    .first()
                                    .map(|value| value.to_string()),
                            });
                        }
                        continue;
                    }
                    let artist = clean_metadata_parts(&subtitle)
                        .first()
                        .copied()
                        .unwrap_or_default()
                        .to_string();
                    result.videos.push(Track {
                        id,
                        title,
                        artists: vec![artist],
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
                    artist_id: None,
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
                "shows" if !browse_id.is_empty() => {
                    let author = subtitle
                        .split('•')
                        .next()
                        .map(|s| s.trim())
                        .unwrap_or("")
                        .to_string();
                    let ep_count = subtitle.split('•').nth(1).and_then(|s| {
                        let s = s
                            .trim()
                            .trim_end_matches("episodios")
                            .trim_end_matches("episodes")
                            .trim();
                        s.parse::<i32>().ok()
                    });
                    result.shows.push(Show {
                        id: browse_id,
                        title,
                        author,
                        description: String::new(),
                        thumbnail: image,
                        episode_count: ep_count,
                        subscriber_count: None,
                    });
                }
                "episodes" => {
                    let id = video_id(renderer);
                    if !is_probable_video_id(&id) {
                        continue;
                    }
                    let show_name = subtitle
                        .split('•')
                        .next()
                        .map(|s| s.trim())
                        .unwrap_or("")
                        .to_string();
                    let duration = renderer
                        .pointer(
                            "/fixedColumns/0/musicResponsiveListItemFixedColumnRenderer/text/runs",
                        )
                        .and_then(Value::as_array)
                        .and_then(|runs| {
                            runs.get(0)
                                .and_then(|r| r.get("text"))
                                .and_then(Value::as_str)
                                .and_then(parse_duration)
                        })
                        .unwrap_or(0);
                    result.episodes.push(Episode {
                        id,
                        title,
                        show: show_name,
                        show_id: browse_id,
                        description: String::new(),
                        thumbnail: image,
                        duration_ms: duration,
                        published_at: String::new(),
                        position: None,
                    });
                }
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

pub fn parse_album_detail(json: &Value, browse_id: &str) -> Result<(Album, Vec<Track>), String> {
    let header = json
        .pointer("/contents/twoColumnBrowseResultsRenderer/tabs/0/tabRenderer/content/sectionListRenderer/contents/0/musicResponsiveHeaderRenderer")
        .or_else(|| json.pointer("/contents/twoColumnBrowseResultsRenderer/tabs/0/tabRenderer/content/sectionListRenderer/contents/0/musicDetailHeaderRenderer"))
        .ok_or_else(|| {
            schema_error(
                "browse/album",
                "a responsive or detail album header",
                json,
            )
        })?;
    let title = text(&header["title"]);
    if title.is_empty() {
        return Err(schema_error(
            "browse/album",
            "a non-empty album header title",
            json,
        ));
    }
    let artist_text = text(&header["straplineTextOne"]);
    let artist_id = header["straplineTextOne"]["runs"][0]["navigationEndpoint"]["browseEndpoint"]
        ["browseId"]
        .as_str()
        .map(|s| s.to_string());
    let artists = if artist_text.is_empty() {
        Vec::new()
    } else {
        vec![artist_text]
    };
    let album_thumbnail = thumbnail(header);
    let track_count = first_number(&header["secondSubtitle"]);
    let album = Album {
        id: browse_id.to_string(),
        title: title.clone(),
        artists: artists.clone(),
        year: first_year(&header["subtitle"]),
        thumbnail: album_thumbnail.clone(),
        track_count,
        artist_id,
    };

    let contents = json
        .pointer("/contents/twoColumnBrowseResultsRenderer/secondaryContents/sectionListRenderer/contents/0/musicShelfRenderer/contents")
        .and_then(Value::as_array)
        .ok_or_else(|| {
            schema_error(
                "browse/album",
                "secondaryContents musicShelfRenderer.contents",
                json,
            )
        })?;
    let mut tracks = Vec::new();
    for item in contents {
        let renderer = &item["musicResponsiveListItemRenderer"];
        if renderer.is_null()
            || renderer["musicItemRendererDisplayPolicy"]
                == "MUSIC_ITEM_RENDERER_DISPLAY_POLICY_GREY_OUT"
        {
            continue;
        }
        let id = video_id(renderer);
        let track_title =
            runs(&renderer["flexColumns"][0]["musicResponsiveListItemFlexColumnRenderer"]["text"]);
        if id.is_empty() || track_title.is_empty() {
            continue;
        }
        let track_artist =
            runs(&renderer["flexColumns"][1]["musicResponsiveListItemFlexColumnRenderer"]["text"]);
        let duration = text(
            &renderer["fixedColumns"][0]["musicResponsiveListItemFixedColumnRenderer"]["text"],
        );
        tracks.push(Track {
            id,
            title: track_title,
            artists: if track_artist.is_empty() {
                artists.clone()
            } else {
                vec![track_artist]
            },
            album: Some(title.clone()),
            album_id: Some(browse_id.to_string()),
            duration_ms: duration_ms(&duration),
            thumbnail: album_thumbnail.clone(),
            stream_url: None,
        });
    }
    Ok((album, tracks))
}

pub fn parse_show_detail(json: &Value, browse_id: &str) -> Result<ShowDetail, String> {
    let header = json
        .pointer("/contents/twoColumnBrowseResultsRenderer/tabs/0/tabRenderer/content/sectionListRenderer/contents/0/musicImmersiveHeaderRenderer")
        .ok_or_else(|| {
            schema_error("browse/show", "musicImmersiveHeaderRenderer", json)
        })?;
    let title = text(&header["title"]);
    let author = text(&header["subtitle"]);
    let description = text(&header["description"]);
    let thumbnail = thumbnail(header);

    let show = Show {
        id: browse_id.to_string(),
        title,
        author,
        description: description.clone(),
        thumbnail: thumbnail.clone(),
        episode_count: None,
        subscriber_count: None,
    };

    let mut episodes = Vec::new();
    if let Some(contents) = json
        .pointer("/contents/twoColumnBrowseResultsRenderer/tabs/0/tabRenderer/content/sectionListRenderer/contents/1/musicShelfRenderer/contents")
        .or_else(|| json.pointer("/contents/twoColumnBrowseResultsRenderer/tabs/0/tabRenderer/content/sectionListRenderer/contents/0/musicShelfRenderer/contents"))
        .and_then(Value::as_array)
    {
        for item in contents {
            let renderer = &item["musicResponsiveListItemRenderer"];
            if renderer.is_null() {
                continue;
            }
            let ep_title = runs(&renderer["flexColumns"][0]["musicResponsiveListItemFlexColumnRenderer"]["text"]);
            let id = video_id(renderer);
            if id.is_empty() || ep_title.is_empty() {
                continue;
            }
            let subtitle_text = runs(&renderer["flexColumns"][1]["musicResponsiveListItemFlexColumnRenderer"]["text"]);
            let duration = text(&renderer["fixedColumns"][0]["musicResponsiveListItemFixedColumnRenderer"]["text"]);
            episodes.push(Episode {
                id,
                title: ep_title,
                show: show.title.clone(),
                show_id: browse_id.to_string(),
                description: subtitle_text,
                thumbnail: thumbnail.clone(),
                duration_ms: duration_ms(&duration),
                published_at: String::new(),
                position: None,
            });
        }
    }

    Ok(ShowDetail {
        show,
        episodes,
        description: if description.is_empty() {
            None
        } else {
            Some(description)
        },
    })
}

fn artist_section_title(section: &Value) -> String {
    section
        .pointer("/musicShelfRenderer/title")
        .or_else(|| {
            section.pointer(
                "/musicCarouselShelfRenderer/header/musicCarouselShelfBasicHeaderRenderer/title",
            )
        })
        .map(runs)
        .unwrap_or_default()
        .to_lowercase()
}

fn parse_artist_release(renderer: &Value, artist_name: &str, artist_id: &str) -> Option<Album> {
    let id = renderer
        .pointer("/navigationEndpoint/browseEndpoint/browseId")
        .and_then(Value::as_str)?;
    if !id.starts_with("MPRE") {
        return None;
    }
    let title = text(&renderer["title"]);
    if title.is_empty() {
        return None;
    }
    Some(Album {
        id: id.to_string(),
        title,
        artists: vec![artist_name.to_string()],
        year: first_year(&renderer["subtitle"]),
        thumbnail: thumbnail(renderer),
        track_count: None,
        artist_id: Some(artist_id.to_string()),
    })
}

pub fn parse_artist_detail(json: &Value, browse_id: &str) -> Result<ArtistDetail, String> {
    let header = json
        .pointer("/header/musicImmersiveHeaderRenderer")
        .ok_or_else(|| schema_error("browse/artist", "musicImmersiveHeaderRenderer", json))?;
    let name = text(&header["title"]);
    if name.is_empty() {
        return Err(schema_error(
            "browse/artist",
            "a non-empty artist name",
            json,
        ));
    }
    let subscriber_count = header
        .pointer("/subscriptionButton/subscribeButtonRenderer/subscriberCountText")
        .map(text)
        .filter(|value| !value.is_empty());
    let sections = json
        .pointer("/contents/singleColumnBrowseResultsRenderer/tabs/0/tabRenderer/content/sectionListRenderer/contents")
        .and_then(Value::as_array)
        .ok_or_else(|| schema_error("browse/artist", "sectionListRenderer.contents", json))?;

    let mut detail = ArtistDetail {
        artist: Artist {
            id: browse_id.to_string(),
            name: name.clone(),
            thumbnail: thumbnail(header),
            subscriber_count,
        },
        description: None,
        top_songs: Vec::new(),
        albums: Vec::new(),
        singles: Vec::new(),
        related: Vec::new(),
    };
    for section in sections {
        if let Some(description) = section
            .pointer("/musicDescriptionShelfRenderer/description")
            .map(text)
            .filter(|value| !value.is_empty())
        {
            detail.description = Some(description);
            continue;
        }
        let title = artist_section_title(section);
        if let Some(contents) = section
            .pointer("/musicShelfRenderer/contents")
            .and_then(Value::as_array)
        {
            for item in contents {
                let renderer = &item["musicResponsiveListItemRenderer"];
                let id = video_id(renderer);
                let track_title = runs(
                    &renderer["flexColumns"][0]["musicResponsiveListItemFlexColumnRenderer"]
                        ["text"],
                );
                if id.is_empty() || track_title.is_empty() {
                    continue;
                }
                let artist = runs(
                    &renderer["flexColumns"][1]["musicResponsiveListItemFlexColumnRenderer"]
                        ["text"],
                );
                let album = runs(
                    &renderer["flexColumns"][2]["musicResponsiveListItemFlexColumnRenderer"]
                        ["text"],
                );
                let duration = text(
                    &renderer["fixedColumns"][0]["musicResponsiveListItemFixedColumnRenderer"]
                        ["text"],
                );
                detail.top_songs.push(Track {
                    id,
                    title: track_title,
                    artists: vec![if artist.is_empty() {
                        name.clone()
                    } else {
                        artist
                    }],
                    album: if album.is_empty() { None } else { Some(album) },
                    album_id: None,
                    duration_ms: duration_ms(&duration),
                    thumbnail: thumbnail(renderer),
                    stream_url: None,
                });
            }
        }
        for item in section
            .pointer("/musicCarouselShelfRenderer/contents")
            .and_then(Value::as_array)
            .into_iter()
            .flatten()
        {
            let renderer = &item["musicTwoRowItemRenderer"];
            let id = renderer
                .pointer("/navigationEndpoint/browseEndpoint/browseId")
                .and_then(Value::as_str)
                .unwrap_or_default();
            if title.contains("album") || title.contains("álbum") {
                if let Some(album) = parse_artist_release(renderer, &name, browse_id) {
                    detail.albums.push(album);
                }
            } else if title.contains("single") || title.contains("sencillo") {
                if let Some(single) = parse_artist_release(renderer, &name, browse_id) {
                    detail.singles.push(single);
                }
            } else if !id.is_empty() {
                let related_name = text(&renderer["title"]);
                if !related_name.is_empty() {
                    detail.related.push(Artist {
                        id: id.to_string(),
                        name: related_name,
                        thumbnail: thumbnail(renderer),
                        subscriber_count: None,
                    });
                }
            }
        }
    }
    Ok(detail)
}

fn parse_track_renderer(renderer: &Value) -> Option<Track> {
    let id = video_id(renderer);
    let title =
        runs(&renderer["flexColumns"][0]["musicResponsiveListItemFlexColumnRenderer"]["text"]);
    if id.is_empty() || title.is_empty() {
        return None;
    }
    let artist =
        runs(&renderer["flexColumns"][1]["musicResponsiveListItemFlexColumnRenderer"]["text"]);
    let album =
        runs(&renderer["flexColumns"][2]["musicResponsiveListItemFlexColumnRenderer"]["text"]);
    let duration =
        text(&renderer["fixedColumns"][0]["musicResponsiveListItemFixedColumnRenderer"]["text"]);
    Some(Track {
        id,
        title,
        artists: if artist.is_empty() {
            Vec::new()
        } else {
            vec![artist]
        },
        album: if album.is_empty() { None } else { Some(album) },
        album_id: None,
        duration_ms: duration_ms(&duration),
        thumbnail: thumbnail(renderer),
        stream_url: None,
    })
}

fn parse_playlist_tracks(items: &[Value]) -> (Vec<Track>, usize) {
    let mut tracks = Vec::new();
    let mut unavailable = 0;
    for item in items {
        let renderer = &item["musicResponsiveListItemRenderer"];
        if renderer.is_null() {
            continue;
        }
        if renderer["musicItemRendererDisplayPolicy"]
            == "MUSIC_ITEM_RENDERER_DISPLAY_POLICY_GREY_OUT"
        {
            unavailable += 1;
            continue;
        }
        if let Some(track) = parse_track_renderer(renderer) {
            tracks.push(track);
        }
    }
    (tracks, unavailable)
}

fn find_string_field(value: &Value, field: &str) -> Option<String> {
    match value {
        Value::Object(map) => map
            .get(field)
            .and_then(Value::as_str)
            .map(str::to_string)
            .or_else(|| {
                map.values()
                    .find_map(|child| find_string_field(child, field))
            }),
        Value::Array(items) => items
            .iter()
            .find_map(|child| find_string_field(child, field)),
        _ => None,
    }
}

pub(crate) fn parse_remote_history(json: &Value) -> Result<Vec<RemoteHistoryItem>, String> {
    let sections = json
        .pointer("/contents/singleColumnBrowseResultsRenderer/tabs/0/tabRenderer/content/sectionListRenderer/contents")
        .and_then(Value::as_array)
        .ok_or_else(|| {
            schema_error(
                "browse/history",
                "sectionListRenderer.contents",
                json,
            )
        })?;
    let mut history = Vec::new();
    for section in sections {
        if let Some(message) = section
            .pointer("/musicNotifierShelfRenderer/title")
            .map(text)
            .filter(|value| !value.is_empty())
        {
            return Err(format!("YouTube Music history is unavailable: {message}"));
        }
        let Some(shelf) = section.get("musicShelfRenderer") else {
            continue;
        };
        let played = text(&shelf["title"]);
        for item in shelf["contents"].as_array().into_iter().flatten() {
            let (mut tracks, _) = parse_playlist_tracks(std::slice::from_ref(item));
            let Some(track) = tracks.pop() else {
                continue;
            };
            history.push(RemoteHistoryItem {
                track,
                played: played.clone(),
                feedback_token: find_string_field(item, "feedbackToken"),
            });
        }
    }
    Ok(history)
}

pub(crate) fn parse_playlist_page(
    json: &Value,
    playlist_id: &str,
) -> Result<(Option<PlaylistDetail>, Vec<Track>, usize, Option<String>), String> {
    if let Some(items) = json
        .pointer("/continuationContents/musicPlaylistShelfContinuation/contents")
        .or_else(|| json.pointer("/continuationContents/musicShelfContinuation/contents"))
        .or_else(|| json.pointer("/onResponseReceivedActions/0/appendContinuationItemsAction/continuationItems"))
        .and_then(Value::as_array)
    {
        let (mut tracks, unavailable) = parse_playlist_tracks(items);
        for track in &mut tracks {
            if track.thumbnail.is_empty() {
                track.thumbnail = thumbnail(json);
            }
        }
        return Ok((None, tracks, unavailable, continuation_token(json)));
    }

    let mut shelf_renderers = Vec::new();
    collect_renderer_items(json, "musicPlaylistShelfRenderer", &mut shelf_renderers);
    collect_renderer_items(json, "musicShelfRenderer", &mut shelf_renderers);
    let shelf = shelf_renderers
        .into_iter()
        .find(|renderer| renderer.get("contents").and_then(Value::as_array).is_some())
        .ok_or_else(|| {
            schema_error(
                "browse/playlist",
                "musicPlaylistShelfRenderer.contents",
                json,
            )
        })?;
    let items = shelf
        .get("contents")
        .and_then(Value::as_array)
        .ok_or_else(|| {
            schema_error(
                "browse/playlist",
                "musicPlaylistShelfRenderer.contents",
                json,
            )
        })?;

    let mut header_renderers = Vec::new();
    collect_renderer_items(json, "musicResponsiveHeaderRenderer", &mut header_renderers);
    collect_renderer_items(json, "musicDetailHeaderRenderer", &mut header_renderers);
    collect_renderer_items(
        json,
        "musicEditablePlaylistDetailHeaderRenderer",
        &mut header_renderers,
    );
    let header = header_renderers.first().copied();
    let meta_header = header
        .and_then(|value| value.pointer("/header/musicDetailHeaderRenderer"))
        .or(header);

    let title = meta_header
        .map(|value| text(&value["title"]))
        .filter(|value| !value.is_empty())
        .or_else(|| {
            shelf
                .pointer("/header/musicShelfBasicHeaderRenderer/title")
                .map(text)
                .filter(|value| !value.is_empty())
        })
        .or_else(|| {
            shelf
                .get("title")
                .map(text)
                .filter(|value| !value.is_empty())
        })
        .unwrap_or_else(|| "Playlist".to_string());
    let description = meta_header
        .or(header)
        .and_then(|value| {
            value
                .pointer("/description/musicDescriptionShelfRenderer/description")
                .or_else(|| value.pointer("/description"))
        })
        .map(text)
        .filter(|value| !value.is_empty());
    let owner = meta_header
        .map(|value| text(&value["straplineTextOne"]))
        .filter(|value| !value.is_empty())
        .or_else(|| {
            meta_header
                .map(|value| text(&value["subtitle"]))
                .filter(|value| !value.is_empty())
        })
        .unwrap_or_default();
    let track_count = meta_header
        .and_then(|value| first_number(&value["secondSubtitle"]))
        .or_else(|| first_number(&shelf["subtitle"]));
    let thumbnail = meta_header
        .map(thumbnail)
        .filter(|value| !value.is_empty())
        .unwrap_or_else(|| thumbnail(shelf));
    let (mut tracks, unavailable) = parse_playlist_tracks(items);
    for track in &mut tracks {
        if track.thumbnail.is_empty() {
            track.thumbnail = thumbnail.clone();
        }
    }
    let detail = PlaylistDetail {
        playlist: Playlist {
            id: playlist_id.trim_start_matches("VL").to_string(),
            title,
            description,
            owner: if owner.is_empty() { None } else { Some(owner) },
            thumbnail,
            track_count,
        },
        privacy: "PUBLIC".to_string(),
        tracks: tracks.clone(),
        unavailable_count: unavailable,
    };
    Ok((Some(detail), tracks, unavailable, continuation_token(json)))
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
                .or_else(|| {
                    renderer.pointer("/title/runs/0/navigationEndpoint/browseEndpoint/browseId")
                })
                .and_then(Value::as_str)
                .map(str::to_string);
            let playlist_id = renderer
                .pointer("/navigationEndpoint/watchPlaylistEndpoint/playlistId")
                .or_else(|| renderer.pointer("/navigationEndpoint/watchEndpoint/playlistId"))
                .or_else(|| {
                    renderer.pointer(
                        "/title/runs/0/navigationEndpoint/watchPlaylistEndpoint/playlistId",
                    )
                })
                .and_then(Value::as_str)
                .map(str::to_string)
                .or_else(|| {
                    browse_id
                        .as_deref()
                        .filter(|id| id.starts_with("VL"))
                        .map(normalize_playlist_id)
                });
            let item_video_id = renderer
                .pointer("/playlistItemData/videoId")
                .or_else(|| renderer.pointer("/navigationEndpoint/watchEndpoint/videoId"))
                .and_then(Value::as_str)
                .map(str::to_string);
            let item_type = home_item_type(
                &item_title,
                &subtitle,
                browse_id.as_deref(),
                playlist_id.as_deref(),
                item_video_id.as_deref(),
            );
            items.push(HomeItem {
                title: item_title,
                subtitle,
                thumbnail: thumbnail(renderer),
                item_type: item_type.to_string(),
                browse_id,
                playlist_id,
                video_id: item_video_id,
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

/// Parse a mix/radio playlist from a `next` endpoint response.
/// Mixes (RD* IDs) use `playlistPanelVideoRenderer` items inside the `next` response.
pub(crate) fn parse_mix_playlist(json: &Value, playlist_id: &str) -> Result<PlaylistDetail, String> {
    if !json.is_object() {
        return Err(schema_error(
            "next/mix",
            "an object containing playlistPanelVideoRenderer entries",
            json,
        ));
    }

    // Extract mix title from the response
    let title = json
        .pointer("/contents/singleColumnMusicWatchNextResultsRenderer/tabbedRenderer/watchNextTabbedResultsRenderer/tabs/0/tabRenderer/content/musicQueueRenderer/header/musicQueueHeaderRenderer/title")
        .or_else(|| json.pointer("/header/musicHeaderRenderer/title"))
        .map(text)
        .filter(|v| !v.is_empty())
        .or_else(|| {
            // Try finding the playlist title from the panel
            fn find_playlist_title(value: &Value) -> Option<String> {
                match value {
                    Value::Object(map) => {
                        if let Some(renderer) = map.get("playlistPanelRenderer") {
                            return renderer.get("title")
                                .and_then(Value::as_str)
                                .map(str::to_string)
                                .or_else(|| renderer.pointer("/title").map(text))
                                .filter(|v| !v.is_empty());
                        }
                        map.values().find_map(find_playlist_title)
                    }
                    Value::Array(items) => items.iter().find_map(find_playlist_title),
                    _ => None,
                }
            }
            find_playlist_title(json)
        })
        .unwrap_or_else(|| "Mix".to_string());

    // Extract thumbnail from the response
    let mix_thumbnail = json
        .pointer("/contents/singleColumnMusicWatchNextResultsRenderer/tabbedRenderer/watchNextTabbedResultsRenderer/tabs/0/tabRenderer/content/musicQueueRenderer/header/musicQueueHeaderRenderer/thumbnail")
        .or_else(|| json.pointer("/background"))
        .map(thumbnail)
        .filter(|v| !v.is_empty())
        .unwrap_or_default();

    let mut tracks = Vec::new();
    let mut seen = HashSet::new();
    collect_related(json, &mut tracks, &mut seen);

    let track_count = tracks.len() as i32;
    Ok(PlaylistDetail {
        playlist: Playlist {
            id: playlist_id.to_string(),
            title,
            description: None,
            owner: Some("YouTube Music".to_string()),
            thumbnail: mix_thumbnail,
            track_count: Some(track_count),
        },
        privacy: "PUBLIC".to_string(),
        tracks,
        unavailable_count: 0,
    })
}


pub fn parse_like_status(json: &Value) -> Result<LikeStatus, String> {
    fn find(value: &Value) -> Option<&str> {
        match value {
            Value::Object(map) => map
                .get("likeStatus")
                .and_then(Value::as_str)
                .or_else(|| map.values().find_map(find)),
            Value::Array(items) => items.iter().find_map(find),
            _ => None,
        }
    }
    match find(json) {
        Some("LIKE") => Ok(LikeStatus::Like),
        Some("DISLIKE") => Ok(LikeStatus::Dislike),
        Some("INDIFFERENT") | None if json.is_object() => Ok(LikeStatus::Indifferent),
        Some(other) => Err(format!("Innertube returned unknown like status: {other}")),
        None => Err(schema_error("next/like-status", "an object response", json)),
    }
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
    fn search_does_not_treat_channel_ids_as_tracks() {
        let json = serde_json::json!({
            "contents": {"tabbedSearchResultsRenderer": {"tabs": [{"tabRenderer": {
                "content": {"sectionListRenderer": {"contents": [{"musicShelfRenderer": {
                    "title": {"runs": [{"text": "Canciones"}]},
                    "contents": [{"musicResponsiveListItemRenderer": {
                        "navigationEndpoint": {"browseEndpoint": {"browseId": "UC8NHWaksFiVRmy2wluQN9Xw"}},
                        "flexColumns": [
                            {"musicResponsiveListItemFlexColumnRenderer": {"text": {"runs": [{"text": "Rutshelle Guillaume"}]}}},
                            {"musicResponsiveListItemFlexColumnRenderer": {"text": {"runs": [{"text": "Artista"}]}}}
                        ]
                    }}]
                }}]}}
            }}]}}
        });
        let page = parse_search_page(&json, "rutshelle", "songs").unwrap();
        assert!(page.items.songs.is_empty());
        assert_eq!(page.items.artists[0].id, "UC8NHWaksFiVRmy2wluQN9Xw");
    }

    #[test]
    fn search_track_metadata_drops_view_counts() {
        let json = serde_json::json!({
            "contents": {"tabbedSearchResultsRenderer": {"tabs": [{"tabRenderer": {
                "content": {"sectionListRenderer": {"contents": [{"musicShelfRenderer": {
                    "title": {"runs": [{"text": "Videos"}]},
                    "contents": [{"musicResponsiveListItemRenderer": {
                        "navigationEndpoint": {"watchEndpoint": {"videoId": "IGgMjgcqNsc"}},
                        "flexColumns": [
                            {"musicResponsiveListItemFlexColumnRenderer": {"text": {"runs": [{"text": "Smooth Criminal - Edición radio"}]}}},
                            {"musicResponsiveListItemFlexColumnRenderer": {"text": {"runs": [{"text": "Michael Jackson"}, {"text": " • "}, {"text": "4 M de visualizaciones"}]}}}
                        ]
                    }}]
                }}]}}
            }}]}}
        });
        let page = parse_search_page(&json, "michael jackson", "videos").unwrap();
        assert_eq!(page.items.videos[0].artists, vec!["Michael Jackson"]);
    }

    #[test]
    fn library_parsers_find_nested_shelf_renderers() {
        let songs_json = serde_json::json!({
            "contents": {"sectionListRenderer": {"contents": [{"itemSectionRenderer": {"contents": [{"musicShelfRenderer": {
                "contents": [{"musicResponsiveListItemRenderer": {
                    "playlistItemData": {"videoId": "song-video-1"},
                    "flexColumns": [
                        {"musicResponsiveListItemFlexColumnRenderer": {"text": {"runs": [{"text": "Nested song"}]}}},
                        {"musicResponsiveListItemFlexColumnRenderer": {"text": {"runs": [{"text": "Nested artist"}]}}}
                    ]
                }}]
            }}]}}]}}
        });
        let songs = parse_library_songs_page(&songs_json).unwrap();
        assert_eq!(songs.items[0].id, "song-video-1");

        let albums_json = serde_json::json!({
            "contents": {"sectionListRenderer": {"contents": [{"itemSectionRenderer": {"contents": [{"gridRenderer": {
                "items": [{"musicTwoRowItemRenderer": {
                    "title": {"runs": [{"text": "Nested album"}]},
                    "subtitle": {"runs": [{"text": "Album"}, {"text": " • "}, {"text": "Nested artist"}, {"text": " • "}, {"text": "2026"}]},
                    "navigationEndpoint": {"browseEndpoint": {"browseId": "MPREnested"}}
                }}]
            }}]}}]}}
        });
        let albums = parse_library_albums(&albums_json).unwrap();
        assert_eq!(albums.items[0].id, "MPREnested");

        let playlists_json = serde_json::json!({
            "continuationContents": {"sectionListContinuation": {"contents": [{"gridRenderer": {
                "items": [{"musicTwoRowItemRenderer": {
                    "title": {"runs": [{"text": "Nested playlist"}]},
                    "subtitle": {"runs": [{"text": "12 canciones"}]},
                    "navigationEndpoint": {"browseEndpoint": {"browseId": "VLPLnested"}}
                }}]
            }}]}}
        });
        let playlists = parse_library_playlists(&playlists_json).unwrap();
        assert_eq!(playlists.items[0].id, "PLnested");
    }

    #[test]
    fn parses_home_continuation_sections() {
        let fixture = fixture(include_str!("fixtures/home_continuation.json"));
        let page = parse_home_page(&fixture).unwrap();
        assert_eq!(page.items[0].title, "Anonymous recommendations");
    }

    #[test]
    fn home_items_keep_typed_navigation_ids() {
        let json = serde_json::json!({
            "contents": {"singleColumnBrowseResultsRenderer": {"tabs": [{"tabRenderer": {
                "content": {"sectionListRenderer": {"contents": [{"musicCarouselShelfRenderer": {
                    "header": {"musicCarouselShelfBasicHeaderRenderer": {"title": {"runs": [{"text": "Typed"}]}}},
                    "contents": [
                        {"musicResponsiveListItemRenderer": {
                            "playlistItemData": {"videoId": "video-1"},
                            "flexColumns": [{"musicResponsiveListItemFlexColumnRenderer": {"text": {"runs": [{"text": "Song"}]}}}]
                        }},
                        {"musicTwoRowItemRenderer": {
                            "title": {"runs": [{"text": "Album"}]},
                            "navigationEndpoint": {"browseEndpoint": {"browseId": "MPREalbum"}}
                        }},
                        {"musicTwoRowItemRenderer": {
                            "title": {"runs": [{"text": "Artist"}]},
                            "navigationEndpoint": {"browseEndpoint": {"browseId": "UCartist"}}
                        }}
                    ]
                }}]}}
            }}]}}
        });
        let sections = parse_home(&json).unwrap();
        assert_eq!(sections[0].items[0].item_type, "song");
        assert_eq!(sections[0].items[0].video_id.as_deref(), Some("video-1"));
        assert_eq!(sections[0].items[1].item_type, "album");
        assert_eq!(sections[0].items[2].item_type, "artist");
        assert_eq!(sections[0].items[2].browse_id.as_deref(), Some("UCartist"));
    }

    #[test]
    fn home_items_normalize_playlist_mix_show_and_episode_ids() {
        let json = serde_json::json!({
            "contents": {"singleColumnBrowseResultsRenderer": {"tabs": [{"tabRenderer": {
                "content": {"sectionListRenderer": {"contents": [{"musicCarouselShelfRenderer": {
                    "header": {"musicCarouselShelfBasicHeaderRenderer": {"title": {"runs": [{"text": "Mixed"}]}}},
                    "contents": [
                        {"musicTwoRowItemRenderer": {
                            "title": {"runs": [{"text": "Playlist by endpoint"}]},
                            "subtitle": {"runs": [{"text": "Playlist"}]},
                            "navigationEndpoint": {"watchPlaylistEndpoint": {"playlistId": "PLendpoint"}}
                        }},
                        {"musicTwoRowItemRenderer": {
                            "title": {"runs": [{"text": "Playlist by browse"}]},
                            "subtitle": {"runs": [{"text": "12 canciones"}]},
                            "navigationEndpoint": {"browseEndpoint": {"browseId": "VLPLbrowse"}}
                        }},
                        {"musicTwoRowItemRenderer": {
                            "title": {"runs": [{"text": "Daily Mix 1"}]},
                            "subtitle": {"runs": [{"text": "Mix"}]},
                            "navigationEndpoint": {"watchEndpoint": {"playlistId": "RDCLAKmix"}}
                        }},
                        {"musicTwoRowItemRenderer": {
                            "title": {"runs": [{"text": "Smart Talks"}]},
                            "subtitle": {"runs": [{"text": "Podcast"}]},
                            "navigationEndpoint": {"browseEndpoint": {"browseId": "MPSPshow"}}
                        }},
                        {"musicTwoRowItemRenderer": {
                            "title": {"runs": [{"text": "Episode one"}]},
                            "subtitle": {"runs": [{"text": "Episode"}]},
                            "navigationEndpoint": {"watchEndpoint": {"videoId": "episode-video"}}
                        }}
                    ]
                }}]}}
            }}]}}
        });
        let sections = parse_home(&json).unwrap();
        let items = &sections[0].items;
        assert_eq!(items[0].item_type, "playlist");
        assert_eq!(items[0].playlist_id.as_deref(), Some("PLendpoint"));
        assert_eq!(items[1].item_type, "playlist");
        assert_eq!(items[1].playlist_id.as_deref(), Some("PLbrowse"));
        assert_eq!(items[2].item_type, "mix");
        assert_eq!(items[2].playlist_id.as_deref(), Some("RDCLAKmix"));
        assert_eq!(items[3].item_type, "show");
        assert_eq!(items[3].browse_id.as_deref(), Some("MPSPshow"));
        assert_eq!(items[4].item_type, "episode");
        assert_eq!(items[4].video_id.as_deref(), Some("episode-video"));
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

    #[test]
    fn parses_album_header_tracks_and_availability() {
        let fixture = fixture(include_str!("fixtures/album_detail.json"));
        let (album, tracks) = parse_album_detail(&fixture, "MPREanonymous").unwrap();
        assert_eq!(album.title, "Anonymous album");
        assert_eq!(album.artists, vec!["Anonymous artist"]);
        assert_eq!(album.year, Some(2026));
        assert_eq!(album.track_count, Some(2));
        assert_eq!(tracks.len(), 1);
        assert_eq!(tracks[0].id, "video-anonymous-album-1");
        assert_eq!(tracks[0].duration_ms, 201_000);
    }

    #[test]
    fn parses_artist_sections_and_related_content() {
        let fixture = fixture(include_str!("fixtures/artist_detail.json"));
        let detail = parse_artist_detail(&fixture, "UCanonymous").unwrap();
        assert_eq!(detail.artist.name, "Anonymous artist");
        assert_eq!(
            detail.description.as_deref(),
            Some("Anonymous artist biography.")
        );
        assert_eq!(detail.top_songs[0].duration_ms, 242_000);
        assert_eq!(detail.albums[0].id, "MPREartistalbum");
        assert_eq!(detail.singles[0].id, "MPREartistsingle");
        assert_eq!(detail.related[0].id, "UCanonymousrelated");
    }

    #[test]
    fn parses_playlist_detail_availability_and_continuation() {
        let detail_fixture = fixture(include_str!("fixtures/playlist_detail.json"));
        let (detail, tracks, unavailable, token) =
            parse_playlist_page(&detail_fixture, "VLPLanonymous").unwrap();
        let detail = detail.unwrap();
        assert_eq!(detail.playlist.title, "Anonymous playlist");
        assert_eq!(detail.playlist.owner.as_deref(), Some("Anonymous owner"));
        assert_eq!(tracks.len(), 1);
        assert_eq!(unavailable, 1);
        assert_eq!(token.as_deref(), Some("playlist-page-2"));

        let continuation = fixture(include_str!("fixtures/playlist_continuation.json"));
        let (detail, tracks, unavailable, token) =
            parse_playlist_page(&continuation, "VLPLanonymous").unwrap();
        assert!(detail.is_none());
        assert_eq!(tracks[0].id, "playlist-track-2");
        assert_eq!(unavailable, 0);
        assert!(token.is_none());
    }

    #[test]
    fn parses_playlist_with_detail_header_renderer() {
        let json = serde_json::json!({
            "contents": {"twoColumnBrowseResultsRenderer": {
                "tabs": [{"tabRenderer": {"content": {"sectionListRenderer": {"contents": [{
                    "musicDetailHeaderRenderer": {
                        "title": {"runs": [{"text": "Alternate playlist"}]},
                        "subtitle": {"runs": [{"text": "Alternate owner"}]},
                        "secondSubtitle": {"runs": [{"text": "1 song"}]},
                        "thumbnail": {"musicThumbnailRenderer": {"thumbnail": {"thumbnails": [{"url": "https://example.invalid/alt.jpg"}]}}}
                    }
                }]}}}}],
                "secondaryContents": {"sectionListRenderer": {"contents": [{
                    "musicPlaylistShelfRenderer": {"contents": [{
                        "musicResponsiveListItemRenderer": {
                            "playlistItemData": {"videoId": "alt-track"},
                            "flexColumns": [
                                {"musicResponsiveListItemFlexColumnRenderer": {"text": {"runs": [{"text": "Alt track"}]}}},
                                {"musicResponsiveListItemFlexColumnRenderer": {"text": {"runs": [{"text": "Alt artist"}]}}}
                            ]
                        }
                    }]}
                }]}}
            }}
        });
        let (detail, tracks, unavailable, _) = parse_playlist_page(&json, "VLPLalt").unwrap();
        let detail = detail.unwrap();
        assert_eq!(detail.playlist.title, "Alternate playlist");
        assert_eq!(detail.playlist.owner.as_deref(), Some("Alternate owner"));
        assert_eq!(detail.playlist.track_count, Some(1));
        assert_eq!(tracks[0].id, "alt-track");
        assert_eq!(unavailable, 0);
    }

    #[test]
    fn parses_playlist_shelf_without_header() {
        let json = serde_json::json!({
            "contents": {"twoColumnBrowseResultsRenderer": {
                "secondaryContents": {"sectionListRenderer": {"contents": [{
                    "musicPlaylistShelfRenderer": {
                        "header": {"musicShelfBasicHeaderRenderer": {"title": {"runs": [{"text": "Shelf playlist"}]}}},
                        "contents": [{
                            "musicResponsiveListItemRenderer": {
                                "playlistItemData": {"videoId": "shelf-track"},
                                "flexColumns": [
                                    {"musicResponsiveListItemFlexColumnRenderer": {"text": {"runs": [{"text": "Shelf track"}]}}},
                                    {"musicResponsiveListItemFlexColumnRenderer": {"text": {"runs": [{"text": "Shelf artist"}]}}}
                                ]
                            }
                        }]
                    }
                }]}}
            }}
        });
        let (detail, tracks, unavailable, _) = parse_playlist_page(&json, "VLPLshelf").unwrap();
        let detail = detail.unwrap();
        assert_eq!(detail.playlist.title, "Shelf playlist");
        assert_eq!(tracks[0].id, "shelf-track");
        assert_eq!(unavailable, 0);
    }

    #[test]
    fn parses_playlist_from_music_shelf_renderer() {
        let json = serde_json::json!({
            "contents": {"singleColumnBrowseResultsRenderer": {"tabs": [{"tabRenderer": {
                "content": {"sectionListRenderer": {"contents": [{
                    "musicShelfRenderer": {
                        "title": {"runs": [{"text": "Shelf renderer playlist"}]},
                        "contents": [{
                            "musicResponsiveListItemRenderer": {
                                "playlistItemData": {"videoId": "music-shelf-track"},
                                "flexColumns": [
                                    {"musicResponsiveListItemFlexColumnRenderer": {"text": {"runs": [{"text": "Music shelf track"}]}}},
                                    {"musicResponsiveListItemFlexColumnRenderer": {"text": {"runs": [{"text": "Music shelf artist"}]}}}
                                ]
                            }
                        }]
                    }
                }]}}
            }}]}}
        });
        let (detail, tracks, unavailable, _) = parse_playlist_page(&json, "VLPLshelf").unwrap();
        let detail = detail.unwrap();
        assert_eq!(detail.playlist.title, "Shelf renderer playlist");
        assert_eq!(tracks[0].id, "music-shelf-track");
        assert_eq!(unavailable, 0);
    }

    #[test]
    fn parses_playlist_music_shelf_continuation() {
        let json = serde_json::json!({
            "continuationContents": {"musicShelfContinuation": {"contents": [{
                "musicResponsiveListItemRenderer": {
                    "playlistItemData": {"videoId": "music-shelf-page-2"},
                    "flexColumns": [
                        {"musicResponsiveListItemFlexColumnRenderer": {"text": {"runs": [{"text": "Page 2 track"}]}}},
                        {"musicResponsiveListItemFlexColumnRenderer": {"text": {"runs": [{"text": "Page 2 artist"}]}}}
                    ]
                }
            }]}}
        });
        let (detail, tracks, unavailable, _) = parse_playlist_page(&json, "VLPLshelf").unwrap();
        assert!(detail.is_none());
        assert_eq!(tracks[0].id, "music-shelf-page-2");
        assert_eq!(unavailable, 0);
    }

    #[test]
    fn parses_like_status_recursively_and_rejects_unknown_values() {
        let liked = serde_json::json!({"menu": {"toggleButtonRenderer": {"likeStatus": "LIKE"}}});
        assert_eq!(parse_like_status(&liked).unwrap(), LikeStatus::Like);
        assert_eq!(
            parse_like_status(&serde_json::json!({})).unwrap(),
            LikeStatus::Indifferent
        );
        assert!(parse_like_status(&serde_json::json!({"likeStatus": "NEW_STATE"})).is_err());
    }

    #[test]
    fn parses_remote_history_groups_and_feedback_tokens() {
        let fixture = fixture(include_str!("fixtures/history.json"));
        let history = parse_remote_history(&fixture).unwrap();
        assert_eq!(history.len(), 2);
        assert_eq!(history[0].track.id, "history-video-1");
        assert_eq!(history[0].played, "Hoy");
        assert_eq!(history[0].feedback_token.as_deref(), Some("feedback-1"));
        assert_eq!(history[1].played, "Esta semana");
        assert!(history[1].feedback_token.is_none());
    }
}
