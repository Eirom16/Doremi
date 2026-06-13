use std::io::Write;
use std::path::Path;
use std::process::{Command, Stdio};

const SERVICE: &str = "doremi";
const YOUTUBE_AUTH: &str = "youtube_music_auth";
const LASTFM_CREDENTIALS: &str = "lastfm_credentials";
const LASTFM_API_KEY: &str = "lastfm_api_key";
const LASTFM_API_SECRET: &str = "lastfm_api_secret";
const LASTFM_SESSION_KEY: &str = "lastfm_session_key";
const GOOGLE_CLIENT_SECRET: &str = "google_client_secret";

#[derive(Debug, Clone, Default, PartialEq, Eq, serde::Deserialize, serde::Serialize)]
pub struct LastFmCredentials {
    pub api_key: String,
    pub api_secret: String,
    pub session_key: String,
}

fn secret_tool() -> Command {
    let mut command = Command::new("secret-tool");
    command
        .stdin(Stdio::piped())
        .stdout(Stdio::piped())
        .stderr(Stdio::piped());
    command
}

fn store(account: &str, value: &str) -> Result<(), String> {
    if value.is_empty() {
        return clear(account);
    }

    let mut command = secret_tool();
    command.args([
        "store",
        "--label=Doremi credentials",
        "service",
        SERVICE,
        "account",
        account,
    ]);
    let mut child = command
        .spawn()
        .map_err(|e| format!("Secret Service is unavailable: {e}"))?;

    child
        .stdin
        .as_mut()
        .ok_or_else(|| "Could not open Secret Service input".to_string())?
        .write_all(value.as_bytes())
        .map_err(|e| format!("Could not write secret: {e}"))?;

    let output = child
        .wait_with_output()
        .map_err(|e| format!("Secret Service failed: {e}"))?;
    if output.status.success() {
        Ok(())
    } else {
        Err(secret_tool_error(&output.stderr))
    }
}

fn lookup(account: &str) -> Result<Option<String>, String> {
    let mut command = secret_tool();
    let output = command
        .args(["lookup", "service", SERVICE, "account", account])
        .output()
        .map_err(|e| format!("Secret Service is unavailable: {e}"))?;

    if output.status.success() {
        let value = String::from_utf8(output.stdout)
            .map_err(|_| "Secret Service returned invalid UTF-8".to_string())?;
        let value = value.trim_end_matches(['\r', '\n']).to_string();
        return Ok((!value.is_empty()).then_some(value));
    }

    // secret-tool returns a non-zero status when no matching item exists.
    if output.stderr.is_empty() {
        Ok(None)
    } else {
        Err(secret_tool_error(&output.stderr))
    }
}

fn clear(account: &str) -> Result<(), String> {
    let mut command = secret_tool();
    let output = command
        .args(["clear", "service", SERVICE, "account", account])
        .output()
        .map_err(|e| format!("Secret Service is unavailable: {e}"))?;
    if output.status.success() || output.stderr.is_empty() {
        Ok(())
    } else {
        Err(secret_tool_error(&output.stderr))
    }
}

fn secret_tool_error(stderr: &[u8]) -> String {
    let detail = String::from_utf8_lossy(stderr);
    let detail = detail.trim();
    if detail.is_empty() {
        "Secret Service operation failed".to_string()
    } else {
        format!("Secret Service operation failed: {detail}")
    }
}

pub fn save_youtube_headers(headers_json: &str) -> Result<(), String> {
    let value: serde_json::Value = serde_json::from_str(headers_json)
        .map_err(|e| format!("Invalid YouTube authentication data: {e}"))?;
    if !value.is_object() {
        return Err("Invalid YouTube authentication data".to_string());
    }
    store(YOUTUBE_AUTH, headers_json)
}

pub fn load_youtube_headers() -> Result<Option<String>, String> {
    lookup(YOUTUBE_AUTH)
}

pub fn delete_youtube_headers() -> Result<(), String> {
    clear(YOUTUBE_AUTH)
}

pub fn migrate_legacy_youtube_headers(config_dir: &Path) -> Result<bool, String> {
    let path = config_dir.join("headers_auth.json");
    if !path.exists() {
        return Ok(false);
    }

    let headers = std::fs::read_to_string(&path)
        .map_err(|e| format!("Could not read legacy YouTube credentials: {e}"))?;
    save_youtube_headers(&headers)?;

    let stored = load_youtube_headers()?;
    if stored.as_deref() != Some(headers.as_str()) {
        return Err("Could not verify migrated YouTube credentials".to_string());
    }

    std::fs::remove_file(&path)
        .map_err(|e| format!("Could not remove legacy YouTube credentials: {e}"))?;
    Ok(true)
}

pub fn save_lastfm_credentials(credentials: &LastFmCredentials) -> Result<(), String> {
    let serialized = serde_json::to_string(credentials)
        .map_err(|e| format!("Could not serialize Last.fm credentials: {e}"))?;
    store(LASTFM_CREDENTIALS, &serialized)
}

pub fn load_lastfm_credentials() -> Result<LastFmCredentials, String> {
    let Some(serialized) = lookup(LASTFM_CREDENTIALS)? else {
        return Ok(LastFmCredentials::default());
    };
    serde_json::from_str(&serialized)
        .map_err(|e| format!("Stored Last.fm credentials are invalid: {e}"))
}

pub fn delete_lastfm_credentials() -> Result<(), String> {
    let mut errors = Vec::new();
    for account in [
        LASTFM_CREDENTIALS,
        LASTFM_API_KEY,
        LASTFM_API_SECRET,
        LASTFM_SESSION_KEY,
    ] {
        if let Err(e) = clear(account) {
            errors.push(e);
        }
    }
    if errors.is_empty() {
        Ok(())
    } else {
        Err(errors.join("; "))
    }
}

pub fn save_google_client_secret(secret: &str) -> Result<(), String> {
    store(GOOGLE_CLIENT_SECRET, secret)
}

pub fn load_google_client_secret() -> Result<Option<String>, String> {
    lookup(GOOGLE_CLIENT_SECRET)
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn youtube_headers_must_be_a_json_object() {
        assert!(save_youtube_headers("[]").is_err());
        assert!(save_youtube_headers("not-json").is_err());
    }

    #[test]
    fn secret_error_never_contains_a_secret_value() {
        let error = secret_tool_error(b"backend unavailable");
        assert!(error.contains("backend unavailable"));
        assert!(!error.contains("super-secret"));
    }

    #[test]
    fn lastfm_credentials_round_trip_as_one_value() {
        let credentials = LastFmCredentials {
            api_key: "key".to_string(),
            api_secret: "secret".to_string(),
            session_key: "session".to_string(),
        };
        let serialized = serde_json::to_string(&credentials).unwrap();
        let restored: LastFmCredentials = serde_json::from_str(&serialized).unwrap();
        assert_eq!(restored, credentials);
    }
}
