use std::sync::Mutex;
use once_cell::sync::Lazy;
use zeroize::Zeroize;

static ENABLED: Lazy<Mutex<bool>> = Lazy::new(|| Mutex::new(false));

pub fn set_enabled(enabled: bool) {
    let mut e = ENABLED.lock().unwrap();
    *e = enabled;
    if enabled {
        log::info!("Last.fm integration enabled");
    } else {
        log::info!("Last.fm integration disabled");
    }
}

pub fn is_enabled() -> bool {
    *ENABLED.lock().unwrap()
}

fn generate_sig(params: &[(String, String)], api_secret: &str) -> String {
    let mut sorted_params: Vec<_> = params.iter().collect();
    // Sort alphabetically by parameter name
    sorted_params.sort_by(|a, b| a.0.cmp(&b.0));
    
    // Concatenate key + value
    let mut sig_str = String::new();
    for (k, v) in sorted_params {
        sig_str.push_str(&k);
        sig_str.push_str(&v);
    }
    // Append api_secret
    sig_str.push_str(api_secret);
    
    // Compute MD5
    let digest = md5::compute(sig_str.as_bytes());
    sig_str.zeroize();
    format!("{:x}", digest)
}

pub async fn authenticate(
    api_key: &str,
    api_secret: &str,
    username: &str,
    password: &str,
) -> Result<String, String> {
    let client = reqwest::Client::new();
    let mut form_data = vec![
        ("method".to_string(), "auth.getMobileSession".to_string()),
        ("api_key".to_string(), api_key.to_string()),
        ("username".to_string(), username.to_string()),
        ("password".to_string(), password.to_string()),
    ];
    let api_sig = generate_sig(&form_data, api_secret);
    form_data.push(("api_sig".to_string(), api_sig));
    form_data.push(("format".to_string(), "json".to_string()));
    
    let response = client.post("https://ws.audioscrobbler.com/2.0/")
        .form(&form_data)
        .send()
        .await;

    for (_, value) in &mut form_data {
        value.zeroize();
    }
    let resp = response.map_err(|e| format!("HTTP request failed: {e}"))?;
        
    if !resp.status().is_success() {
        return Err(format!("HTTP error status: {}", resp.status()));
    }
    
    let json: serde_json::Value = resp.json()
        .await
        .map_err(|e| format!("JSON parsing failed: {e}"))?;
        
    if let Some(error) = json.get("error") {
        let msg = json.get("message").and_then(|m| m.as_str()).unwrap_or("Unknown error");
        return Err(format!("Last.fm error (code {}): {}", error, msg));
    }
    
    let session_key = json.pointer("/session/key")
        .and_then(|k| k.as_str())
        .ok_or_else(|| "Failed to parse session key from response".to_string())?;
        
    Ok(session_key.to_string())
}

pub fn update_now_playing(artist: &str, title: &str, album: &str) {
    if !is_enabled() {
        return;
    }
    let artist = artist.to_string();
    let title = title.to_string();
    let album = album.to_string();
    
    tokio::spawn(async move {
        if let Err(e) = update_now_playing_async(&artist, &title, &album).await {
            log::warn!("Last.fm now playing update failed: {e}");
        }
    });
}

pub fn scrobble(artist: &str, title: &str, album: &str) {
    if !is_enabled() {
        return;
    }
    let artist = artist.to_string();
    let title = title.to_string();
    let album = album.to_string();
    let timestamp = std::time::SystemTime::now()
        .duration_since(std::time::UNIX_EPOCH)
        .unwrap_or_default()
        .as_secs();
    
    tokio::spawn(async move {
        if let Err(e) = scrobble_async(&artist, &title, &album, timestamp).await {
            log::warn!("Last.fm scrobble failed: {e}");
        }
    });
}

async fn update_now_playing_async(artist: &str, title: &str, album: &str) -> Result<(), String> {
    let dirs = crate::config::paths::AppDirs::global();
    let settings = crate::config::settings::AppSettings::load(&dirs.settings_path());
    let api_key = &settings.integrations.lastfm_api_key;
    let api_secret = &settings.integrations.lastfm_api_secret;
    let session_key = &settings.integrations.lastfm_session_key;
    
    if api_key.is_empty() || api_secret.is_empty() || session_key.is_empty() {
        return Err("Last.fm credentials not configured".to_string());
    }
    
    let client = reqwest::Client::new();
    let mut params = vec![
        ("method".to_string(), "track.updateNowPlaying".to_string()),
        ("api_key".to_string(), api_key.to_string()),
        ("sk".to_string(), session_key.to_string()),
        ("artist".to_string(), artist.to_string()),
        ("track".to_string(), title.to_string()),
    ];
    if !album.is_empty() {
        params.push(("album".to_string(), album.to_string()));
    }
    
    let api_sig = generate_sig(&params, api_secret);
    params.push(("api_sig".to_string(), api_sig));
    params.push(("format".to_string(), "json".to_string()));
    
    let resp = client.post("https://ws.audioscrobbler.com/2.0/")
        .form(&params)
        .send()
        .await
        .map_err(|e| format!("HTTP request failed: {e}"))?;
        
    let json: serde_json::Value = resp.json()
        .await
        .map_err(|e| format!("JSON parsing failed: {e}"))?;
        
    if let Some(error) = json.get("error") {
        let msg = json.get("message").and_then(|m| m.as_str()).unwrap_or("Unknown error");
        return Err(format!("Last.fm error (code {}): {}", error, msg));
    }
    
    log::info!("Last.fm: Now playing updated to '{}' by '{}'", title, artist);
    Ok(())
}

async fn scrobble_async(artist: &str, title: &str, album: &str, timestamp: u64) -> Result<(), String> {
    let dirs = crate::config::paths::AppDirs::global();
    let settings = crate::config::settings::AppSettings::load(&dirs.settings_path());
    let api_key = &settings.integrations.lastfm_api_key;
    let api_secret = &settings.integrations.lastfm_api_secret;
    let session_key = &settings.integrations.lastfm_session_key;
    
    if api_key.is_empty() || api_secret.is_empty() || session_key.is_empty() {
        return Err("Last.fm credentials not configured".to_string());
    }
    
    let client = reqwest::Client::new();
    let mut params = vec![
        ("method".to_string(), "track.scrobble".to_string()),
        ("api_key".to_string(), api_key.to_string()),
        ("sk".to_string(), session_key.to_string()),
        ("artist".to_string(), artist.to_string()),
        ("track".to_string(), title.to_string()),
        ("timestamp".to_string(), timestamp.to_string()),
    ];
    if !album.is_empty() {
        params.push(("album".to_string(), album.to_string()));
    }
    
    let api_sig = generate_sig(&params, api_secret);
    params.push(("api_sig".to_string(), api_sig));
    params.push(("format".to_string(), "json".to_string()));
    
    let resp = client.post("https://ws.audioscrobbler.com/2.0/")
        .form(&params)
        .send()
        .await
        .map_err(|e| format!("HTTP request failed: {e}"))?;
        
    let json: serde_json::Value = resp.json()
        .await
        .map_err(|e| format!("JSON parsing failed: {e}"))?;
        
    if let Some(error) = json.get("error") {
        let msg = json.get("message").and_then(|m| m.as_str()).unwrap_or("Unknown error");
        return Err(format!("Last.fm error (code {}): {}", error, msg));
    }
    
    log::info!("Last.fm: Scrobbled '{}' by '{}'", title, artist);
    Ok(())
}
