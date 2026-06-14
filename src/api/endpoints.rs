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

    let (track_artists_res, corpus_artists_res) = tokio::join!(track_artists_fut, corpus_artists_fut);

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
            songs: vec![track("one")],
            videos: Vec::new(),
            albums: Vec::new(),
            artists: Vec::new(),
            playlists: Vec::new(),
        };
        let incoming = SearchResults {
            query: "q".into(),
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
}
