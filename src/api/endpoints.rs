use once_cell::sync::Lazy;
use serde_json::Value;
use std::collections::HashSet;
use std::sync::RwLock;

const MAX_CONTINUATION_PAGES: usize = 3;
const SEARCH_CACHE_TTL_SECS: u64 = 300;
const HOME_CACHE_TTL_SECS: u64 = 900;
const RELATED_CACHE_TTL_SECS: u64 = 600;

#[derive(Clone)]
struct ClientContext {
    language: String,
    region: String,
}

static CLIENT_CONTEXT: Lazy<RwLock<ClientContext>> = Lazy::new(|| {
    RwLock::new(ClientContext {
        language: "es".to_string(),
        region: "US".to_string(),
    })
});

pub fn configure(language: &str, region: &str) {
    let language = normalize_language(language);
    let region = normalize_region(region);
    if let Ok(mut context) = CLIENT_CONTEXT.write() {
        if context.language != language || context.region != region {
            let _ = crate::db::cache::ResponseCache::invalidate_prefix("ytm:");
        }
        *context = ClientContext { language, region };
    }
}

pub fn invalidate_cache() {
    if let Err(error) = crate::db::cache::ResponseCache::invalidate_prefix("ytm:") {
        log::debug!("Could not invalidate YouTube Music response cache: {error}");
    }
}

fn normalize_language(language: &str) -> String {
    let value = language.trim().replace('_', "-").to_lowercase();
    value
        .split('-')
        .next()
        .filter(|value| value.len() == 2)
        .unwrap_or("es")
        .to_string()
}

fn normalize_region(region: &str) -> String {
    let value = region.trim().to_uppercase();
    if value.len() == 2
        && value
            .chars()
            .all(|character| character.is_ascii_alphabetic())
    {
        value
    } else {
        "US".to_string()
    }
}

fn context() -> Value {
    let current = current_client_context();
    serde_json::json!({
        "context": {"client": {
            "clientName": "WEB_REMIX",
            "clientVersion": "1.20250331.00.00",
            "hl": current.language,
            "gl": current.region
        }, "user": {"lockedSafetyMode": false}}
    })
}

fn current_client_context() -> ClientContext {
    CLIENT_CONTEXT
        .read()
        .map(|value| value.clone())
        .unwrap_or(ClientContext {
            language: "es".to_string(),
            region: "US".to_string(),
        })
}

fn cache_key(resource: &str) -> String {
    let current = CLIENT_CONTEXT
        .read()
        .map(|value| value.clone())
        .unwrap_or(ClientContext {
            language: "es".to_string(),
            region: "US".to_string(),
        });
    format!(
        "ytm:{}:{}-{}:{resource}",
        super::auth::cache_scope(),
        current.language,
        current.region
    )
}

fn cached<T: serde::de::DeserializeOwned>(key: &str) -> Option<T> {
    crate::db::cache::ResponseCache::get(key).map(|entry| entry.data)
}

fn cache<T: serde::Serialize>(key: &str, value: &T, ttl_secs: u64) {
    if let Err(error) = crate::db::cache::ResponseCache::set(key, value, Some(ttl_secs)) {
        log::debug!("Could not cache YouTube Music response {key}: {error}");
    }
}

pub async fn search(query: &str, filter: &str) -> Result<super::models::SearchResults, String> {
    let key = cache_key(&format!(
        "search:{filter}:{:x}",
        md5::compute(query.as_bytes())
    ));
    if let Some(results) = cached(&key) {
        return Ok(results);
    }
    let mut body = context();
    body["query"] = serde_json::json!(query);
    let params = match filter {
        "songs" => Some("EgWKAQIIAWoFCAQQgAE="),
        "videos" => Some("EgWKAQIIARoFCAQQgAE="),
        "albums" => Some("EgWKAQIIRRoFCAQQgAE="),
        "artists" => Some("EgWKAQIIBRoFCAQQgAE="),
        "playlists" => Some("EgWKAQIIBxoFCAQQgAE="),
        _ => None,
    };
    if let Some(params) = params {
        body["params"] = serde_json::json!(params);
    }
    let response = super::transport::post("search", body).await?;
    let fallback_category = if filter == "all" || filter.is_empty() {
        "songs"
    } else {
        filter
    };
    let mut page = super::parsers::parse_search_page(&response, query, fallback_category)?;
    let mut results = page.items;
    let mut seen_tokens = HashSet::new();
    for _ in 0..MAX_CONTINUATION_PAGES {
        let Some(token) = page.continuation.take() else {
            break;
        };
        if !seen_tokens.insert(token.clone()) {
            break;
        }
        let mut continuation_body = context();
        continuation_body["continuation"] = serde_json::json!(token);
        let response = super::transport::post("search", continuation_body).await?;
        page = super::parsers::parse_search_page(&response, query, fallback_category)?;
        merge_search_results(&mut results, page.items);
    }
    cache(&key, &results, SEARCH_CACHE_TTL_SECS);
    Ok(results)
}

pub async fn search_suggestions(query: &str) -> Result<Vec<String>, String> {
    let query = query.trim();
    if query.len() < 2 {
        return Ok(Vec::new());
    }
    let key = cache_key(&format!("suggestions:{:x}", md5::compute(query.as_bytes())));
    if let Some(suggestions) = cached(&key) {
        return Ok(suggestions);
    }
    let mut body = context();
    body["input"] = serde_json::json!(query);
    let response = super::transport::post("music/get_search_suggestions", body).await?;
    let suggestions = super::parsers::parse_search_suggestions(&response)?;
    cache(&key, &suggestions, SEARCH_CACHE_TTL_SECS);
    Ok(suggestions)
}

pub async fn home_sections() -> Result<Vec<super::models::HomeSection>, String> {
    let key = cache_key("home");
    if let Some(sections) = cached(&key) {
        return Ok(sections);
    }
    let mut body = context();
    body["browseId"] = serde_json::json!("FEmusic_home");
    let response = super::transport::post("browse", body).await?;
    let mut page = super::parsers::parse_home_page(&response)?;
    let mut sections = page.items;
    let mut seen_tokens = HashSet::new();
    for _ in 0..MAX_CONTINUATION_PAGES {
        let Some(token) = page.continuation.take() else {
            break;
        };
        if !seen_tokens.insert(token.clone()) {
            break;
        }
        let mut continuation_body = context();
        continuation_body["continuation"] = serde_json::json!(token);
        let response = super::transport::post("browse", continuation_body).await?;
        page = super::parsers::parse_home_page(&response)?;
        sections.append(&mut page.items);
    }
    cache(&key, &sections, HOME_CACHE_TTL_SECS);
    Ok(sections)
}

pub async fn home_sections_page(
    continuation: Option<&str>,
) -> Result<(Vec<super::models::HomeSection>, Option<String>), String> {
    let mut body = context();
    if let Some(token) = continuation.filter(|token| !token.trim().is_empty()) {
        body["continuation"] = serde_json::json!(token);
    } else {
        body["browseId"] = serde_json::json!("FEmusic_home");
    }
    let response = super::transport::post("browse", body).await?;
    let page = super::parsers::parse_home_page(&response)?;
    Ok((page.items, page.continuation))
}

fn charts_body() -> Value {
    let region = current_client_context().region;
    let mut body = context();
    body["browseId"] = serde_json::json!("FEmusic_charts");
    body["formData"] = serde_json::json!({"selectedValues": [region]});
    body
}

pub async fn charts() -> Result<Vec<super::models::HomeSection>, String> {
    let key = cache_key("charts");
    if let Some(sections) = cached(&key) {
        return Ok(sections);
    }
    let response = super::transport::post("browse", charts_body()).await?;
    let sections = super::parsers::parse_home(&response)?;
    cache(&key, &sections, HOME_CACHE_TTL_SECS);
    Ok(sections)
}

pub async fn album_detail(
    browse_id: &str,
) -> Result<(super::models::Album, Vec<super::models::Track>), String> {
    let browse_id = browse_id.trim();
    if !browse_id.starts_with("MPRE") {
        return Err("Album browse ID must start with MPRE".to_string());
    }
    let key = cache_key(&format!("album:{browse_id}"));
    if let Some(detail) = cached(&key) {
        return Ok(detail);
    }
    let mut body = context();
    body["browseId"] = serde_json::json!(browse_id);
    let response = super::transport::post("browse", body).await?;
    let detail = super::parsers::parse_album_detail(&response, browse_id)?;
    cache(&key, &detail, HOME_CACHE_TTL_SECS);
    Ok(detail)
}

pub async fn artist_detail(browse_id: &str) -> Result<super::models::ArtistDetail, String> {
    let browse_id = browse_id
        .trim()
        .strip_prefix("MPLA")
        .unwrap_or(browse_id.trim());
    if browse_id.is_empty() {
        return Err("Artist browse ID cannot be empty".to_string());
    }
    let key = cache_key(&format!("artist:{browse_id}"));
    if let Some(detail) = cached(&key) {
        return Ok(detail);
    }
    let mut body = context();
    body["browseId"] = serde_json::json!(browse_id);
    let response = super::transport::post("browse", body).await?;
    let detail = super::parsers::parse_artist_detail(&response, browse_id)?;
    cache(&key, &detail, HOME_CACHE_TTL_SECS);
    Ok(detail)
}

pub async fn show_detail(browse_id: &str) -> Result<super::models::ShowDetail, String> {
    let browse_id = browse_id.trim();
    if browse_id.is_empty() {
        return Err("Show browse ID cannot be empty".to_string());
    }
    let key = cache_key(&format!("show:{browse_id}"));
    if let Some(detail) = cached(&key) {
        return Ok(detail);
    }
    let mut body = context();
    body["browseId"] = serde_json::json!(browse_id);
    let response = super::transport::post("browse", body).await?;
    let detail = super::parsers::parse_show_detail(&response, browse_id)?;
    cache(&key, &detail, HOME_CACHE_TTL_SECS);
    Ok(detail)
}

pub async fn playlist_detail(playlist_id: &str) -> Result<super::models::PlaylistDetail, String> {
    let playlist_id = playlist_id.trim();
    if playlist_id.is_empty() {
        return Err("Playlist ID cannot be empty".to_string());
    }
    let browse_id = if playlist_id.starts_with("VL") {
        playlist_id.to_string()
    } else {
        format!("VL{playlist_id}")
    };
    let key = cache_key(&format!("playlist:{browse_id}"));
    if let Some(detail) = cached(&key) {
        return Ok(detail);
    }
    let mut body = context();
    body["browseId"] = serde_json::json!(browse_id);
    let response = super::transport::post("browse", body).await?;
    let (detail, _, _, mut continuation) =
        super::parsers::parse_playlist_page(&response, &browse_id)?;
    let mut detail = detail.ok_or_else(|| "Playlist initial response had no header".to_string())?;
    let mut seen_tokens = HashSet::new();
    let mut seen_tracks = detail
        .tracks
        .iter()
        .map(|track| track.id.clone())
        .collect::<HashSet<_>>();
    for _ in 0..MAX_CONTINUATION_PAGES {
        let Some(token) = continuation.take() else {
            break;
        };
        if !seen_tokens.insert(token.clone()) {
            break;
        }
        let mut continuation_body = context();
        continuation_body["continuation"] = serde_json::json!(token);
        let response = super::transport::post("browse", continuation_body).await?;
        let (_, tracks, unavailable, next) =
            super::parsers::parse_playlist_page(&response, &browse_id)?;
        detail.unavailable_count += unavailable;
        for track in tracks {
            if seen_tracks.insert(track.id.clone()) {
                detail.tracks.push(track);
            }
        }
        continuation = next;
    }
    cache(&key, &detail, HOME_CACHE_TTL_SECS);
    Ok(detail)
}

pub async fn related_tracks(video_id: &str) -> Result<Vec<super::models::Track>, String> {
    if video_id.trim().is_empty() {
        return Ok(Vec::new());
    }
    let key = cache_key(&format!("related:{video_id}"));
    if let Some(tracks) = cached(&key) {
        return Ok(tracks);
    }
    let mut body = context();
    body["videoId"] = serde_json::json!(video_id);
    body["playlistId"] = serde_json::json!(format!("RDAMVM{video_id}"));
    body["isAudioOnly"] = serde_json::json!(true);
    body["params"] = serde_json::json!("wAEB");
    let response = super::transport::post("next", body).await?;
    let mut tracks = super::parsers::parse_related(&response, video_id)?;
    let mut continuation = super::parsers::related_continuation(&response);
    let mut seen_tokens = HashSet::new();
    let mut seen_tracks = tracks
        .iter()
        .map(|track| track.id.clone())
        .collect::<HashSet<_>>();
    for _ in 0..MAX_CONTINUATION_PAGES {
        let Some(token) = continuation.take() else {
            break;
        };
        if !seen_tokens.insert(token.clone()) {
            break;
        }
        let mut continuation_body = context();
        continuation_body["continuation"] = serde_json::json!(token);
        let response = super::transport::post("next", continuation_body).await?;
        for track in super::parsers::parse_related(&response, video_id)? {
            if seen_tracks.insert(track.id.clone()) {
                tracks.push(track);
            }
        }
        continuation = super::parsers::related_continuation(&response);
    }
    cache(&key, &tracks, RELATED_CACHE_TTL_SECS);
    Ok(tracks)
}

fn rating_endpoint(status: super::models::LikeStatus) -> &'static str {
    match status {
        super::models::LikeStatus::Like => "like/like",
        super::models::LikeStatus::Dislike => "like/dislike",
        super::models::LikeStatus::Indifferent => "like/removelike",
    }
}

pub async fn rate_song(video_id: &str, status: super::models::LikeStatus) -> Result<(), String> {
    let video_id = video_id.trim();
    if video_id.is_empty() {
        return Err("Video ID cannot be empty".to_string());
    }
    if !super::auth::is_authenticated() {
        return Err("YouTube Music authentication is required to rate songs".to_string());
    }
    let mut body = context();
    body["target"] = serde_json::json!({"videoId": video_id});
    super::transport::post(rating_endpoint(status), body).await?;
    invalidate_cache();
    Ok(())
}

pub async fn song_like_status(video_id: &str) -> Result<super::models::LikeStatus, String> {
    let video_id = video_id.trim();
    if video_id.is_empty() {
        return Err("Video ID cannot be empty".to_string());
    }
    if !super::auth::is_authenticated() {
        return Ok(super::models::LikeStatus::Indifferent);
    }
    let mut body = context();
    body["videoId"] = serde_json::json!(video_id);
    body["isAudioOnly"] = serde_json::json!(true);
    let response = super::transport::post("next", body).await?;
    super::parsers::parse_like_status(&response)
}

fn require_authenticated(operation: &str) -> Result<(), String> {
    if super::auth::is_authenticated() {
        Ok(())
    } else {
        Err(format!(
            "YouTube Music authentication is required to {operation}"
        ))
    }
}

fn normalize_playlist_id(playlist_id: &str) -> Result<&str, String> {
    let playlist_id = playlist_id
        .trim()
        .strip_prefix("VL")
        .unwrap_or(playlist_id.trim());
    if playlist_id.is_empty() {
        Err("Playlist ID cannot be empty".to_string())
    } else {
        Ok(playlist_id)
    }
}

fn validate_playlist_title(title: &str) -> Result<&str, String> {
    let title = title.trim();
    if title.is_empty() {
        Err("Playlist title cannot be empty".to_string())
    } else if title.contains(['<', '>']) {
        Err("Playlist title cannot contain '<' or '>'".to_string())
    } else {
        Ok(title)
    }
}

fn validate_privacy(privacy: &str) -> Result<&str, String> {
    match privacy.trim().to_uppercase().as_str() {
        "PUBLIC" => Ok("PUBLIC"),
        "PRIVATE" => Ok("PRIVATE"),
        "UNLISTED" => Ok("UNLISTED"),
        _ => Err("Playlist privacy must be PUBLIC, PRIVATE, or UNLISTED".to_string()),
    }
}

pub async fn create_playlist(
    title: &str,
    description: &str,
    privacy: &str,
) -> Result<String, String> {
    require_authenticated("create playlists")?;
    let title = validate_playlist_title(title)?;
    let privacy = validate_privacy(privacy)?;
    let mut body = context();
    body["title"] = serde_json::json!(title);
    body["description"] = serde_json::json!(description.trim());
    body["privacyStatus"] = serde_json::json!(privacy);
    let response = super::transport::post("playlist/create", body).await?;
    let playlist_id = response["playlistId"]
        .as_str()
        .filter(|value| !value.is_empty())
        .ok_or_else(|| "Playlist creation response did not include playlistId".to_string())?;
    invalidate_cache();
    Ok(playlist_id.to_string())
}

pub async fn edit_playlist(
    playlist_id: &str,
    title: Option<&str>,
    description: Option<&str>,
    privacy: Option<&str>,
) -> Result<(), String> {
    require_authenticated("edit playlists")?;
    let playlist_id = normalize_playlist_id(playlist_id)?;
    let mut actions = Vec::new();
    if let Some(title) = title {
        actions.push(serde_json::json!({
            "action": "ACTION_SET_PLAYLIST_NAME",
            "playlistName": validate_playlist_title(title)?
        }));
    }
    if let Some(description) = description {
        actions.push(serde_json::json!({
            "action": "ACTION_SET_PLAYLIST_DESCRIPTION",
            "playlistDescription": description.trim()
        }));
    }
    if let Some(privacy) = privacy {
        actions.push(serde_json::json!({
            "action": "ACTION_SET_PLAYLIST_PRIVACY",
            "playlistPrivacy": validate_privacy(privacy)?
        }));
    }
    if actions.is_empty() {
        return Err("Playlist edit requires at least one change".to_string());
    }
    let mut body = context();
    body["playlistId"] = serde_json::json!(playlist_id);
    body["actions"] = serde_json::json!(actions);
    let response = super::transport::post("browse/edit_playlist", body).await?;
    require_succeeded("edit playlist", &response)?;
    invalidate_cache();
    Ok(())
}

pub async fn delete_playlist(playlist_id: &str) -> Result<(), String> {
    require_authenticated("delete playlists")?;
    let mut body = context();
    body["playlistId"] = serde_json::json!(normalize_playlist_id(playlist_id)?);
    let response = super::transport::post("playlist/delete", body).await?;
    require_succeeded("delete playlist", &response)?;
    invalidate_cache();
    Ok(())
}

fn add_video_actions(video_ids: &[String], skip_duplicates: bool) -> Result<Vec<Value>, String> {
    let mut actions = Vec::new();
    for video_id in video_ids {
        let video_id = video_id.trim();
        if video_id.is_empty() {
            return Err("Video IDs cannot be empty".to_string());
        }
        let mut action = serde_json::json!({
            "action": "ACTION_ADD_VIDEO",
            "addedVideoId": video_id
        });
        if skip_duplicates {
            action["dedupeOption"] = serde_json::json!("DEDUPE_OPTION_SKIP");
        }
        actions.push(action);
    }
    if actions.is_empty() {
        Err("At least one video ID is required".to_string())
    } else {
        Ok(actions)
    }
}

pub async fn add_playlist_items(
    playlist_id: &str,
    video_ids: &[String],
    skip_duplicates: bool,
) -> Result<Vec<super::models::PlaylistItemRef>, String> {
    require_authenticated("add songs to playlists")?;
    let mut body = context();
    body["playlistId"] = serde_json::json!(normalize_playlist_id(playlist_id)?);
    body["actions"] = serde_json::json!(add_video_actions(video_ids, skip_duplicates)?);
    let response = super::transport::post("browse/edit_playlist", body).await?;
    require_succeeded("add playlist items", &response)?;
    let added = response["playlistEditResults"]
        .as_array()
        .into_iter()
        .flatten()
        .filter_map(|result| result.get("playlistEditVideoAddedResultData"))
        .filter_map(|result| {
            Some(super::models::PlaylistItemRef {
                video_id: result["videoId"].as_str()?.to_string(),
                set_video_id: result["setVideoId"].as_str()?.to_string(),
            })
        })
        .collect();
    invalidate_cache();
    Ok(added)
}

fn remove_video_actions(items: &[super::models::PlaylistItemRef]) -> Result<Vec<Value>, String> {
    if items.is_empty() {
        return Err("At least one playlist item is required".to_string());
    }
    items
        .iter()
        .map(|item| {
            if item.video_id.trim().is_empty() || item.set_video_id.trim().is_empty() {
                return Err("Playlist items require both videoId and setVideoId".to_string());
            }
            Ok(serde_json::json!({
                "action": "ACTION_REMOVE_VIDEO",
                "removedVideoId": item.video_id.trim(),
                "setVideoId": item.set_video_id.trim()
            }))
        })
        .collect()
}

pub async fn remove_playlist_items(
    playlist_id: &str,
    items: &[super::models::PlaylistItemRef],
) -> Result<(), String> {
    require_authenticated("remove songs from playlists")?;
    let actions = remove_video_actions(items)?;
    let mut body = context();
    body["playlistId"] = serde_json::json!(normalize_playlist_id(playlist_id)?);
    body["actions"] = serde_json::json!(actions);
    let response = super::transport::post("browse/edit_playlist", body).await?;
    require_succeeded("remove playlist items", &response)?;
    invalidate_cache();
    Ok(())
}

pub async fn remote_history() -> Result<Vec<super::models::RemoteHistoryItem>, String> {
    require_authenticated("read remote history")?;
    let key = cache_key("history");
    if let Some(history) = cached(&key) {
        return Ok(history);
    }
    let mut body = context();
    body["browseId"] = serde_json::json!("FEmusic_history");
    let response = super::transport::post("browse", body).await?;
    let history = super::parsers::parse_remote_history(&response)?;
    cache(&key, &history, 60);
    Ok(history)
}

pub async fn remove_remote_history_items(feedback_tokens: &[String]) -> Result<(), String> {
    require_authenticated("remove remote history items")?;
    let feedback_tokens = feedback_tokens
        .iter()
        .map(|token| token.trim())
        .filter(|token| !token.is_empty())
        .collect::<Vec<_>>();
    if feedback_tokens.is_empty() {
        return Err("At least one history feedback token is required".to_string());
    }
    let mut body = context();
    body["feedbackTokens"] = serde_json::json!(feedback_tokens);
    super::transport::post("feedback", body).await?;
    invalidate_cache();
    Ok(())
}

fn require_succeeded(operation: &str, response: &Value) -> Result<(), String> {
    match response["status"].as_str() {
        Some("STATUS_SUCCEEDED") => Ok(()),
        Some(status) => Err(format!("Innertube failed to {operation}: {status}")),
        None => Err(format!(
            "Innertube {operation} response did not include status"
        )),
    }
}

pub async fn library_playlists() -> Result<Vec<super::models::Playlist>, String> {
    let key = cache_key("library:playlists");
    if let Some(playlists) = cached(&key) {
        return Ok(playlists);
    }
    let mut body = context();
    body["browseId"] = serde_json::json!("FEmusic_liked_playlists");
    let response = super::transport::post("browse", body).await?;
    let mut page = super::parsers::parse_library_playlists(&response)?;
    let mut playlists = page.items;
    let mut seen_tokens = HashSet::new();
    for _ in 0..MAX_CONTINUATION_PAGES {
        let Some(token) = page.continuation.take() else {
            break;
        };
        if !seen_tokens.insert(token.clone()) {
            break;
        }
        let mut continuation_body = context();
        continuation_body["continuation"] = serde_json::json!(token);
        let response = super::transport::post("browse", continuation_body).await?;
        page = super::parsers::parse_library_playlists(&response)?;
        playlists.append(&mut page.items);
    }
    cache(&key, &playlists, RELATED_CACHE_TTL_SECS);
    Ok(playlists)
}

pub async fn library_songs(limit: Option<usize>) -> Result<Vec<super::models::Track>, String> {
    let limit_val = limit.unwrap_or(100);
    let key = cache_key(&format!("library:songs:{}", limit_val));
    if let Some(songs) = cached(&key) {
        return Ok(songs);
    }
    let mut body = context();
    body["browseId"] = serde_json::json!("FEmusic_liked_videos");
    let response = super::transport::post("browse", body).await?;
    let mut page = super::parsers::parse_library_songs_page(&response)?;
    let mut songs = page.items;
    let mut seen_tokens = HashSet::new();
    for _ in 0..MAX_CONTINUATION_PAGES {
        if songs.len() >= limit_val {
            break;
        }
        let Some(token) = page.continuation.take() else {
            break;
        };
        if !seen_tokens.insert(token.clone()) {
            break;
        }
        let mut continuation_body = context();
        continuation_body["continuation"] = serde_json::json!(token);
        let response = super::transport::post("browse", continuation_body).await?;
        page = super::parsers::parse_library_songs_page(&response)?;
        songs.append(&mut page.items);
    }
    if limit.is_some() {
        songs.truncate(limit_val);
    }
    cache(&key, &songs, RELATED_CACHE_TTL_SECS);
    Ok(songs)
}

pub async fn library_albums() -> Result<Vec<super::models::Album>, String> {
    let key = cache_key("library:albums");
    if let Some(albums) = cached(&key) {
        return Ok(albums);
    }
    let mut body = context();
    body["browseId"] = serde_json::json!("FEmusic_liked_albums");
    let response = super::transport::post("browse", body).await?;
    let mut page = super::parsers::parse_library_albums(&response)?;
    let mut albums = page.items;
    let mut seen_tokens = HashSet::new();
    for _ in 0..MAX_CONTINUATION_PAGES {
        let Some(token) = page.continuation.take() else {
            break;
        };
        if !seen_tokens.insert(token.clone()) {
            break;
        }
        let mut continuation_body = context();
        continuation_body["continuation"] = serde_json::json!(token);
        let response = super::transport::post("browse", continuation_body).await?;
        page = super::parsers::parse_library_albums(&response)?;
        albums.append(&mut page.items);
    }
    cache(&key, &albums, RELATED_CACHE_TTL_SECS);
    Ok(albums)
}

pub async fn library_artists() -> Result<Vec<super::models::Artist>, String> {
    let key = cache_key("library:artists");
    if let Some(artists) = cached(&key) {
        return Ok(artists);
    }

    let fetch_artists = |browse_id: &'static str| async move {
        let mut body = context();
        body["browseId"] = serde_json::json!(browse_id);
        let response = super::transport::post("browse", body).await?;
        let mut page = super::parsers::parse_library_artists(&response)?;
        let mut list = page.items;
        let mut seen_tokens = HashSet::new();
        for _ in 0..MAX_CONTINUATION_PAGES {
            let Some(token) = page.continuation.take() else {
                break;
            };
            if !seen_tokens.insert(token.clone()) {
                break;
            }
            let mut continuation_body = context();
            continuation_body["continuation"] = serde_json::json!(token);
            let response = super::transport::post("browse", continuation_body).await?;
            page = super::parsers::parse_library_artists(&response)?;
            list.append(&mut page.items);
        }
        Result::<Vec<super::models::Artist>, String>::Ok(list)
    };

    let track_artists_fut = fetch_artists("FEmusic_library_corpus_track_artists");
    let corpus_artists_fut = fetch_artists("FEmusic_library_corpus_artists");

    let (track_artists_res, corpus_artists_res) =
        tokio::join!(track_artists_fut, corpus_artists_fut);

    let mut merged = Vec::new();
    let mut seen_ids = HashSet::new();

    if let Ok(ref list) = track_artists_res {
        for artist in list {
            if seen_ids.insert(artist.id.clone()) {
                merged.push(artist.clone());
            }
        }
    }
    if let Ok(ref list) = corpus_artists_res {
        for artist in list {
            if seen_ids.insert(artist.id.clone()) {
                merged.push(artist.clone());
            }
        }
    }

    if track_artists_res.is_err() && corpus_artists_res.is_err() {
        return Err("Failed to retrieve library artists".to_string());
    }

    cache(&key, &merged, RELATED_CACHE_TTL_SECS);
    Ok(merged)
}

fn retain_new<T, F>(items: &mut Vec<T>, incoming: Vec<T>, id: F)
where
    F: Fn(&T) -> &str,
{
    let mut seen = items
        .iter()
        .map(|item| id(item).to_string())
        .collect::<HashSet<_>>();
    items.extend(
        incoming
            .into_iter()
            .filter(|item| seen.insert(id(item).to_string())),
    );
}

fn merge_search_results(
    target: &mut super::models::SearchResults,
    incoming: super::models::SearchResults,
) {
    retain_new(&mut target.songs, incoming.songs, |item| &item.id);
    retain_new(&mut target.videos, incoming.videos, |item| &item.id);
    retain_new(&mut target.albums, incoming.albums, |item| &item.id);
    retain_new(&mut target.artists, incoming.artists, |item| &item.id);
    retain_new(&mut target.playlists, incoming.playlists, |item| &item.id);
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn locale_and_region_are_normalized_in_request_context() {
        configure("es_DO", "do");
        let value = context();
        assert_eq!(
            value.pointer("/context/client/hl").and_then(Value::as_str),
            Some("es")
        );
        assert_eq!(
            value.pointer("/context/client/gl").and_then(Value::as_str),
            Some("DO")
        );

        configure("invalid", "123");
        let value = context();
        assert_eq!(
            value.pointer("/context/client/hl").and_then(Value::as_str),
            Some("es")
        );
        assert_eq!(
            value.pointer("/context/client/gl").and_then(Value::as_str),
            Some("US")
        );
    }

    #[test]
    fn search_page_merge_deduplicates_each_result_kind() {
        use super::super::models::{SearchResults, Track};
        let track = |id: &str| Track {
            id: id.to_string(),
            title: id.to_string(),
            artists: Vec::new(),
            album: None,
            album_id: None,
            duration_ms: 0,
            thumbnail: String::new(),
            stream_url: None,
        };
        let mut target = SearchResults {
            query: "q".into(),
            top_result: None,
            songs: vec![track("one")],
            videos: Vec::new(),
            albums: Vec::new(),
            artists: Vec::new(),
            playlists: Vec::new(),
        };
        let incoming = SearchResults {
            query: "q".into(),
            top_result: None,
            songs: vec![track("one"), track("two")],
            videos: Vec::new(),
            albums: Vec::new(),
            artists: Vec::new(),
            playlists: Vec::new(),
        };
        merge_search_results(&mut target, incoming);
        assert_eq!(
            target
                .songs
                .iter()
                .map(|track| track.id.as_str())
                .collect::<Vec<_>>(),
            vec!["one", "two"]
        );
    }

    #[test]
    fn charts_request_uses_configured_region() {
        configure("es", "DO");
        let body = charts_body();
        assert_eq!(body["browseId"], "FEmusic_charts");
        assert_eq!(
            body.pointer("/formData/selectedValues/0")
                .and_then(Value::as_str),
            Some("DO")
        );
    }

    #[test]
    fn like_status_maps_to_innertube_endpoints() {
        use super::super::models::LikeStatus;
        assert_eq!(rating_endpoint(LikeStatus::Like), "like/like");
        assert_eq!(rating_endpoint(LikeStatus::Dislike), "like/dislike");
        assert_eq!(rating_endpoint(LikeStatus::Indifferent), "like/removelike");
    }

    #[test]
    fn playlist_mutation_inputs_are_validated_and_normalized() {
        assert_eq!(normalize_playlist_id("VLPL123").unwrap(), "PL123");
        assert_eq!(normalize_playlist_id(" PL123 ").unwrap(), "PL123");
        assert!(normalize_playlist_id("VL").is_err());
        assert!(validate_playlist_title("<invalid>").is_err());
        assert_eq!(validate_privacy("private").unwrap(), "PRIVATE");
        assert!(validate_privacy("friends").is_err());
        assert!(require_succeeded(
            "edit playlist",
            &serde_json::json!({"status": "STATUS_SUCCEEDED"})
        )
        .is_ok());
    }

    #[test]
    fn playlist_item_actions_require_stable_remote_ids() {
        let actions = add_video_actions(&["video-1".to_string()], true).unwrap();
        assert_eq!(actions[0]["action"], "ACTION_ADD_VIDEO");
        assert_eq!(actions[0]["dedupeOption"], "DEDUPE_OPTION_SKIP");
        assert!(add_video_actions(&[], false).is_err());

        let invalid = super::super::models::PlaylistItemRef {
            video_id: "video-1".to_string(),
            set_video_id: String::new(),
        };
        assert!(remove_video_actions(&[invalid]).is_err());
        assert!(remove_video_actions(&[]).is_err());
    }

    #[test]
    fn remote_history_requires_feedback_tokens_for_removal() {
        let tokens = [String::new(), "  ".to_string()];
        let filtered = tokens
            .iter()
            .map(|token| token.trim())
            .filter(|token| !token.is_empty())
            .collect::<Vec<_>>();
        assert!(filtered.is_empty());
    }
}
