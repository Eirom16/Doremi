use reqwest::header::{HeaderMap, HeaderName, HeaderValue, USER_AGENT};
use std::sync::atomic::{AtomicBool, Ordering};

const ANONYMOUS_USER_AGENT: &str = "Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 Doremi/2";

/// Guards against multiple in-flight requests all degrading the session at once.
static SESSION_REVOKED: AtomicBool = AtomicBool::new(false);

fn stored_headers() -> Option<String> {
    crate::utils::secure_storage::load_youtube_headers()
        .map_err(|error| log::debug!("Could not load YouTube credentials: {error}"))
        .ok()
        .flatten()
}

/// Re-arm revocation detection after a fresh, deliberate login.
pub fn reset_session_revoked() {
    SESSION_REVOKED.store(false, Ordering::SeqCst);
}

/// Called by the transport layer when an authenticated Innertube request is
/// rejected with 401/403. Clears stored credentials exactly once and degrades
/// cleanly to anonymous mode, notifying the UI.
pub fn handle_session_revoked() {
    if !is_authenticated() {
        return;
    }
    // Only the first concurrent request to observe the revocation acts on it.
    if SESSION_REVOKED.swap(true, Ordering::SeqCst) {
        return;
    }
    log::warn!("YouTube Music session was revoked; degrading to anonymous mode");
    if let Err(error) = crate::utils::secure_storage::delete_youtube_headers() {
        log::warn!("Failed to delete revoked YouTube credentials: {error}");
    }
    crate::api::endpoints::invalidate_cache();
    crate::bridge::on_session_revoked();
}

pub fn cache_scope() -> String {
    stored_headers()
        .map(|content| format!("user:{:x}", md5::compute(content.as_bytes())))
        .unwrap_or_else(|| "anonymous".to_string())
}

pub fn is_authenticated() -> bool {
    stored_headers().as_deref().is_some_and(session_has_sapisid)
}

fn session_has_sapisid(content: &str) -> bool {
    serde_json::from_str::<serde_json::Map<String, serde_json::Value>>(content)
        .ok()
        .and_then(|headers| {
            headers
                .iter()
                .find(|(name, _)| name.eq_ignore_ascii_case("cookie"))
                .and_then(|(_, value)| value.as_str())
                .and_then(extract_sapisid_from_cookie)
        })
        .is_some()
}

fn extract_sapisid_from_cookie(cookie_val: &str) -> Option<String> {
    let mut sapisid = None;
    let mut secure_1 = None;
    let mut secure_3 = None;

    for part in cookie_val.split(';') {
        let part = part.trim();
        if let Some(eq_idx) = part.find('=') {
            let name = part[..eq_idx].trim();
            let val = part[eq_idx + 1..].trim().replace('"', "");
            if name == "__Secure-3PAPISID" {
                secure_3 = Some(val);
            } else if name == "__Secure-1PAPISID" {
                secure_1 = Some(val);
            } else if name == "SAPISID" {
                sapisid = Some(val);
            }
        }
    }

    secure_3.or(secure_1).or(sapisid)
}

pub fn request_headers() -> HeaderMap {
    match stored_headers() {
        Some(content) => headers_from_session(&content),
        None => headers_from_session(""),
    }
}

/// Build the outgoing Innertube header map from a stored session blob (a JSON
/// object of header name -> value). Pure and side-effect free so it can be
/// tested without touching the system keyring. An empty/invalid blob yields a
/// valid anonymous header set.
fn headers_from_session(content: &str) -> HeaderMap {
    let mut headers = HeaderMap::new();

    if let Ok(values) = serde_json::from_str::<serde_json::Map<String, serde_json::Value>>(content)
    {
        for (name, value) in values {
            let (Ok(name), Some(value)) = (
                HeaderName::from_bytes(name.as_bytes()),
                value
                    .as_str()
                    .and_then(|value| HeaderValue::from_str(value).ok()),
            ) else {
                continue;
            };
            headers.insert(name, value);
        }
    }

    // Extract sapisid cookie if present and construct dynamic SAPISIDHASH
    if let Some(cookie_val) = headers.get("cookie").and_then(|h| h.to_str().ok()) {
        if let Some(sapisid) = extract_sapisid_from_cookie(cookie_val) {
            let timestamp = std::time::SystemTime::now()
                .duration_since(std::time::UNIX_EPOCH)
                .map(|d| d.as_secs())
                .unwrap_or(0);
            if let Ok(auth_val) = HeaderValue::from_str(&sapisidhash(timestamp, &sapisid)) {
                headers.insert("authorization", auth_val);
            }
        }
    }

    if !headers.contains_key(USER_AGENT) {
        headers.insert(USER_AGENT, HeaderValue::from_static(ANONYMOUS_USER_AGENT));
    }
    headers
}

/// Compute the `SAPISIDHASH <ts>_<sha1>` authorization value for a given
/// timestamp and SAPISID, against the music.youtube.com origin.
fn sapisidhash(timestamp: u64, sapisid: &str) -> String {
    use sha1::{Digest, Sha1};
    let origin = "https://music.youtube.com";
    let message = format!("{} {} {}", timestamp, sapisid, origin);
    let mut hasher = Sha1::new();
    hasher.update(message.as_bytes());
    format!("SAPISIDHASH {}_{:x}", timestamp, hasher.finalize())
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn anonymous_headers_always_have_a_user_agent() {
        assert!(request_headers().contains_key(USER_AGENT));
    }

    #[test]
    fn test_extract_sapisid() {
        assert_eq!(
            extract_sapisid_from_cookie("__Secure-3PAPISID=\"hello\""),
            Some("hello".to_string())
        );
        assert_eq!(
            extract_sapisid_from_cookie("foo=bar; SAPISID=world; baz=qux"),
            Some("world".to_string())
        );
        assert_eq!(extract_sapisid_from_cookie("foo=bar; baz=qux"), None);
    }

    #[test]
    fn empty_session_blob_yields_anonymous_headers() {
        for blob in ["", "   ", "not json", "[]", "null"] {
            let headers = headers_from_session(blob);
            assert!(headers.contains_key(USER_AGENT), "blob: {blob:?}");
            assert!(!headers.contains_key("authorization"), "blob: {blob:?}");
            assert!(!headers.contains_key("cookie"), "blob: {blob:?}");
        }
    }

    #[test]
    fn session_blob_round_trips_headers_and_signs_requests() {
        let blob = serde_json::json!({
            "cookie": "SAPISID=secret-sapisid; SID=abc",
            "x-goog-authuser": "0"
        })
        .to_string();
        let headers = headers_from_session(&blob);
        assert_eq!(
            headers.get("x-goog-authuser").unwrap().to_str().unwrap(),
            "0"
        );
        let auth = headers
            .get("authorization")
            .expect("authenticated session must carry an authorization header")
            .to_str()
            .unwrap()
            .to_string();
        assert!(auth.starts_with("SAPISIDHASH "));
        // The raw SAPISID must never leak into the signed header.
        assert!(!auth.contains("secret-sapisid"));
    }

    #[test]
    fn sapisidhash_is_deterministic_and_hides_the_secret() {
        let value = sapisidhash(1_700_000_000, "secret-sapisid");
        // Format is "SAPISIDHASH <ts>_<40-hex sha1>".
        assert!(value.starts_with("SAPISIDHASH 1700000000_"));
        let hash = &value["SAPISIDHASH 1700000000_".len()..];
        assert_eq!(hash.len(), 40);
        assert!(hash.chars().all(|c| c.is_ascii_hexdigit()));
        // The raw secret must never appear in the signed value.
        assert!(!value.contains("secret-sapisid"));
        // Same inputs -> same hash; different timestamp -> different hash.
        assert_eq!(value, sapisidhash(1_700_000_000, "secret-sapisid"));
        assert_ne!(value, sapisidhash(1_700_000_001, "secret-sapisid"));
    }

    #[test]
    fn malformed_header_values_are_skipped_not_panicked() {
        // Newline in a header value is illegal; it must be dropped silently.
        let blob = serde_json::json!({
            "cookie": "SAPISID=ok",
            "x-bad": "line1\nline2",
            "x-good": "fine"
        })
        .to_string();
        let headers = headers_from_session(&blob);
        assert!(!headers.contains_key("x-bad"));
        assert_eq!(headers.get("x-good").unwrap().to_str().unwrap(), "fine");
    }

    #[test]
    fn authentication_requires_a_valid_sapisid_cookie() {
        assert!(session_has_sapisid(
            &serde_json::json!({"cookie": "SID=abc; SAPISID=renewable"}).to_string()
        ));
        assert!(!session_has_sapisid(
            &serde_json::json!({"cookie": "SID=abc"}).to_string()
        ));
        assert!(!session_has_sapisid("not json"));
    }
}
