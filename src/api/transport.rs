use once_cell::sync::Lazy;
use reqwest::{header::RETRY_AFTER, StatusCode};
use serde_json::Value;
use std::time::Duration;
use tokio::sync::{Mutex, Semaphore};
use tokio::time::Instant;

const API_KEY: &str = "AIzaSyC9XL3ZjWddXya6X74dJoCTL-WEYFDNX30";
const BASE_URL: &str = "https://music.youtube.com/youtubei/v1";
const MAX_ATTEMPTS: usize = 4;
const MAX_CONCURRENT_REQUESTS: usize = 4;
const MIN_REQUEST_INTERVAL: Duration = Duration::from_millis(100);

static CLIENT: Lazy<reqwest::Client> = Lazy::new(|| {
    // BF1.4: use expect() so a misconfigured builder is caught at startup, not silently
    // dropped, which would remove the timeout.
    reqwest::Client::builder()
        .timeout(Duration::from_secs(10))
        .build()
        .expect("HTTP client is misconfigured")
});
static REQUEST_LIMIT: Lazy<Semaphore> = Lazy::new(|| Semaphore::new(MAX_CONCURRENT_REQUESTS));
static LAST_REQUEST: Lazy<Mutex<Option<Instant>>> = Lazy::new(|| Mutex::new(None));

async fn wait_for_rate_limit() {
    let mut last_request = LAST_REQUEST.lock().await;
    if let Some(previous) = *last_request {
        let elapsed = previous.elapsed();
        if elapsed < MIN_REQUEST_INTERVAL {
            tokio::time::sleep(MIN_REQUEST_INTERVAL - elapsed).await;
        }
    }
    *last_request = Some(Instant::now());
}

fn retryable(status: StatusCode) -> bool {
    status == StatusCode::TOO_MANY_REQUESTS || status.is_server_error()
}

/// A 401/403 on an authenticated request means the session was revoked or
/// expired server-side; retrying with the same credentials cannot succeed.
fn revokes_session(status: StatusCode) -> bool {
    status == StatusCode::UNAUTHORIZED
        || status == StatusCode::FORBIDDEN
        || status == StatusCode::BAD_REQUEST
}

fn retry_delay(attempt: usize, retry_after: Option<&str>) -> Duration {
    retry_after
        .and_then(|value| value.trim().parse::<u64>().ok())
        .map(Duration::from_secs)
        .unwrap_or_else(|| Duration::from_millis(250 * (1_u64 << attempt.min(3))))
        .min(Duration::from_secs(10))
}

pub async fn post(endpoint: &str, body: Value) -> Result<Value, String> {
    let _permit = REQUEST_LIMIT
        .acquire()
        .await
        .map_err(|_| "Innertube request limiter is unavailable".to_string())?;
    let url = format!("{BASE_URL}/{endpoint}?key={API_KEY}");
    let was_authenticated = super::auth::is_authenticated();
    let mut last_error = None;

    for attempt in 0..MAX_ATTEMPTS {
        wait_for_rate_limit().await;
        let headers = super::auth::request_headers();
        // BF0.2: log only header *names* at trace level; never log values (they contain
        // session cookies and authorization tokens).
        if log::log_enabled!(log::Level::Trace) {
            let names: Vec<&str> = headers.keys().map(|k| k.as_str()).collect();
            log::trace!("Innertube request headers present: {:?}", names);
        }
        let response = CLIENT.post(&url).headers(headers).json(&body).send().await;

        let response = match response {
            Ok(response) => response,
            Err(error) => {
                last_error = Some(format!("Innertube transport error: {error}"));
                if attempt + 1 < MAX_ATTEMPTS {
                    tokio::time::sleep(retry_delay(attempt, None)).await;
                    continue;
                }
                break;
            }
        };
        let status = response.status();
        let retry_after = response
            .headers()
            .get(RETRY_AFTER)
            .and_then(|value| value.to_str().ok())
            .map(str::to_string);
        let text = response
            .text()
            .await
            .map_err(|error| format!("Innertube response read error: {error}"))?;
        if status.is_success() {
            return serde_json::from_str(&text).map_err(|error| {
                format!("Innertube endpoint {endpoint} returned invalid JSON: {error}")
            });
        }

        last_error = Some(format!(
            "Innertube endpoint {endpoint} returned {status}: {text}"
        ));
        if revokes_session(status) && was_authenticated {
            super::auth::handle_session_revoked();
            break;
        }
        if retryable(status) && attempt + 1 < MAX_ATTEMPTS {
            tokio::time::sleep(retry_delay(attempt, retry_after.as_deref())).await;
            continue;
        }
        break;
    }

    Err(last_error.unwrap_or_else(|| "Innertube request failed without a response".to_string()))
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn retries_rate_limits_and_server_errors_only() {
        assert!(retryable(StatusCode::TOO_MANY_REQUESTS));
        assert!(retryable(StatusCode::SERVICE_UNAVAILABLE));
        assert!(!retryable(StatusCode::BAD_REQUEST));
        assert!(!retryable(StatusCode::UNAUTHORIZED));
    }

    #[test]
    fn retry_delay_uses_header_and_caps_backoff() {
        assert_eq!(retry_delay(0, None), Duration::from_millis(250));
        assert_eq!(retry_delay(2, None), Duration::from_secs(1));
        assert_eq!(retry_delay(0, Some("3")), Duration::from_secs(3));
        assert_eq!(retry_delay(0, Some("60")), Duration::from_secs(10));
    }
}
