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
}
