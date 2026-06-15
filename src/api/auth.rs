use reqwest::header::{HeaderMap, HeaderName, HeaderValue, USER_AGENT};

const ANONYMOUS_USER_AGENT: &str = "Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 Doremi/2";

fn stored_headers() -> Option<String> {
    crate::utils::secure_storage::load_youtube_headers()
        .map_err(|error| log::debug!("Could not load YouTube credentials: {error}"))
        .ok()
        .flatten()
}

pub fn cache_scope() -> String {
    stored_headers()
        .map(|content| format!("user:{:x}", md5::compute(content.as_bytes())))
        .unwrap_or_else(|| "anonymous".to_string())
}

pub fn is_authenticated() -> bool {
    stored_headers().is_some()
}

fn extract_sapisid_from_cookie(cookie_val: &str) -> Option<String> {
    for part in cookie_val.split(';') {
        let part = part.trim();
        if let Some(eq_idx) = part.find('=') {
            let name = part[..eq_idx].trim();
            let val = part[eq_idx + 1..].trim().replace('"', "");
            if name == "__Secure-3PAPISID" || name == "__Secure-1PAPISID" || name == "SAPISID" {
                return Some(val);
            }
        }
    }
    None
}

pub fn request_headers() -> HeaderMap {
    let mut headers = HeaderMap::new();
    let stored = stored_headers();

    if let Some(content) = stored {
        if let Ok(values) =
            serde_json::from_str::<serde_json::Map<String, serde_json::Value>>(&content)
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
    }

    // Extract sapisid cookie if present and construct dynamic SAPISIDHASH
    if let Some(cookie_val) = headers.get("cookie").and_then(|h| h.to_str().ok()) {
        if let Some(sapisid) = extract_sapisid_from_cookie(cookie_val) {
            let timestamp = std::time::SystemTime::now()
                .duration_since(std::time::UNIX_EPOCH)
                .map(|d| d.as_secs())
                .unwrap_or(0);
            let origin = "https://music.youtube.com";
            let message = format!("{} {} {}", timestamp, sapisid, origin);

            use sha1::{Digest, Sha1};
            let mut hasher = Sha1::new();
            hasher.update(message.as_bytes());
            let result = hasher.finalize();
            let hash_hex = format!("{:x}", result);

            if let Ok(auth_val) = HeaderValue::from_str(&format!("SAPISIDHASH {}_{}", timestamp, hash_hex)) {
                headers.insert("authorization", auth_val);
            }
        }
    }

    if !headers.contains_key(USER_AGENT) {
        headers.insert(USER_AGENT, HeaderValue::from_static(ANONYMOUS_USER_AGENT));
    }
    headers
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
        assert_eq!(
            extract_sapisid_from_cookie("foo=bar; baz=qux"),
            None
        );
    }
}
