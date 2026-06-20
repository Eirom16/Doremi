use once_cell::sync::OnceCell;
use reqwest::header::{
    HeaderMap, HeaderValue, ACCEPT, ACCEPT_ENCODING, ORIGIN, RANGE, REFERER, USER_AGENT,
};
use std::collections::HashMap;
use std::io;
use std::net::{SocketAddr, TcpListener as StdTcpListener};
use std::sync::atomic::{AtomicU64, Ordering};
use std::sync::{Arc, Mutex};
use std::time::{Duration, Instant};
use tokio::io::{AsyncReadExt, AsyncWriteExt};
use tokio::net::{TcpListener, TcpStream};

const YOUTUBE_ANDROID_USER_AGENT: &str =
    "com.google.android.youtube/20.10.38 (Linux; U; Android 15)";
const MAX_REQUEST_BYTES: usize = 32 * 1024;
const STREAM_RANGE_BYTES: u64 = 256 * 1024;
const TARGET_TTL: Duration = Duration::from_secs(6 * 60 * 60);

static PROXY: OnceCell<StreamProxy> = OnceCell::new();
static TOKEN_COUNTER: AtomicU64 = AtomicU64::new(1);
static CLIENT: OnceCell<reqwest::Client> = OnceCell::new();

#[derive(Clone)]
struct ProxyTarget {
    url: String,
    created_at: Instant,
}

struct StreamProxy {
    addr: SocketAddr,
    targets: Arc<Mutex<HashMap<String, ProxyTarget>>>,
}

pub fn proxied_url(url: &str) -> String {
    if !needs_proxy(url) {
        return url.to_string();
    }

    let Some(proxy) = ensure_proxy() else {
        log::warn!("YouTube stream proxy is unavailable; passing direct URL to VLC");
        return url.to_string();
    };

    let token = format!("{:x}", TOKEN_COUNTER.fetch_add(1, Ordering::Relaxed));
    if let Ok(mut targets) = proxy.targets.lock() {
        prune_targets(&mut targets);
        targets.insert(
            token.clone(),
            ProxyTarget {
                url: url.to_string(),
                created_at: Instant::now(),
            },
        );
    }

    format!("http://{}/stream/{}", proxy.addr, token)
}

fn needs_proxy(url: &str) -> bool {
    url.contains("googlevideo.com") || url.contains(".googlevideo.com/")
}

fn ensure_proxy() -> Option<&'static StreamProxy> {
    if let Some(proxy) = PROXY.get() {
        return Some(proxy);
    }

    let handle = tokio::runtime::Handle::try_current().ok()?;
    let std_listener = StdTcpListener::bind("127.0.0.1:0").ok()?;
    let addr = std_listener.local_addr().ok()?;
    std_listener.set_nonblocking(true).ok()?;
    let listener = TcpListener::from_std(std_listener).ok()?;
    let targets = Arc::new(Mutex::new(HashMap::new()));

    if PROXY
        .set(StreamProxy {
            addr,
            targets: targets.clone(),
        })
        .is_err()
    {
        return PROXY.get();
    }

    handle.spawn(run_proxy(listener, targets));
    log::info!("Started YouTube stream proxy on {addr}");
    PROXY.get()
}

fn prune_targets(targets: &mut HashMap<String, ProxyTarget>) {
    let now = Instant::now();
    targets.retain(|_, target| now.duration_since(target.created_at) <= TARGET_TTL);
}

async fn run_proxy(listener: TcpListener, targets: Arc<Mutex<HashMap<String, ProxyTarget>>>) {
    loop {
        match listener.accept().await {
            Ok((stream, _)) => {
                let targets = targets.clone();
                tokio::spawn(async move {
                    if let Err(error) = handle_connection(stream, targets).await {
                        log::debug!("YouTube stream proxy connection ended: {error}");
                    }
                });
            }
            Err(error) => {
                log::warn!("YouTube stream proxy accept failed: {error}");
                tokio::time::sleep(Duration::from_millis(250)).await;
            }
        }
    }
}

async fn handle_connection(
    mut stream: TcpStream,
    targets: Arc<Mutex<HashMap<String, ProxyTarget>>>,
) -> io::Result<()> {
    let request = read_http_request(&mut stream).await?;
    let Some((method, path, range_header)) = parse_request(&request) else {
        return write_simple_response(&mut stream, 400, "Bad Request").await;
    };

    if method != "GET" && method != "HEAD" {
        return write_simple_response(&mut stream, 405, "Method Not Allowed").await;
    }

    let Some(token) = path.strip_prefix("/stream/") else {
        return write_simple_response(&mut stream, 404, "Not Found").await;
    };

    let target = {
        let Ok(targets) = targets.lock() else {
            return write_simple_response(&mut stream, 500, "Internal Server Error").await;
        };
        targets.get(token).cloned()
    };

    let Some(target) = target else {
        return write_simple_response(&mut stream, 404, "Stream Expired").await;
    };

    forward_stream(&mut stream, method, &target.url, range_header.as_deref()).await
}

async fn read_http_request(stream: &mut TcpStream) -> io::Result<Vec<u8>> {
    let mut request = Vec::with_capacity(1024);
    let mut buffer = [0_u8; 1024];

    loop {
        let read = stream.read(&mut buffer).await?;
        if read == 0 {
            break;
        }
        request.extend_from_slice(&buffer[..read]);
        if request.windows(4).any(|window| window == b"\r\n\r\n") {
            break;
        }
        if request.len() > MAX_REQUEST_BYTES {
            break;
        }
    }

    Ok(request)
}

fn parse_request(request: &[u8]) -> Option<(&str, &str, Option<String>)> {
    let text = std::str::from_utf8(request).ok()?;
    let mut lines = text.lines();
    let first_line = lines.next()?;
    let mut parts = first_line.split_whitespace();
    let method = parts.next()?;
    let path = parts.next()?;
    let mut range_header = None;

    for line in lines {
        let Some((name, value)) = line.split_once(':') else {
            continue;
        };
        if name.trim().eq_ignore_ascii_case("range") {
            range_header = Some(value.trim().to_string());
            break;
        }
    }

    Some((method, path, range_header))
}

async fn forward_stream(
    stream: &mut TcpStream,
    method: &str,
    url: &str,
    range_header: Option<&str>,
) -> io::Result<()> {
    let client = CLIENT.get_or_init(|| {
        reqwest::Client::builder()
            .connect_timeout(Duration::from_secs(8))
            .build()
            .expect("failed to build stream proxy client")
    });

    let effective_range = bounded_range(range_header);
    let mut headers = HeaderMap::new();
    headers.insert(
        USER_AGENT,
        HeaderValue::from_static(YOUTUBE_ANDROID_USER_AGENT),
    );
    headers.insert(ORIGIN, HeaderValue::from_static("https://www.youtube.com"));
    headers.insert(
        REFERER,
        HeaderValue::from_static("https://www.youtube.com/"),
    );
    headers.insert(ACCEPT, HeaderValue::from_static("*/*"));
    headers.insert(ACCEPT_ENCODING, HeaderValue::from_static("identity"));
    if let Ok(range) = HeaderValue::from_str(&effective_range) {
        headers.insert(RANGE, range);
    }

    let request = if method == "HEAD" {
        client.head(url)
    } else {
        client.get(url)
    };
    let response = match request.headers(headers).send().await {
        Ok(response) => response,
        Err(error) => {
            log::warn!("YouTube stream proxy request failed: {error}");
            return write_simple_response(stream, 502, "Bad Gateway").await;
        }
    };
    if !response.status().is_success() {
        log::warn!(
            "YouTube stream proxy upstream returned HTTP {} for range {} (client {:?})",
            response.status(),
            effective_range,
            range_header
        );
    }

    write_response_head(stream, response.status(), response.headers()).await?;
    if method == "HEAD" {
        return Ok(());
    }

    let mut response = response;
    loop {
        match response.chunk().await {
            Ok(Some(chunk)) => stream.write_all(&chunk).await?,
            Ok(None) => break,
            Err(error) => {
                log::warn!("YouTube stream proxy body read failed: {error}");
                break;
            }
        }
    }

    Ok(())
}

fn bounded_range(client_range: Option<&str>) -> String {
    let Some(client_range) = client_range else {
        return format!("bytes=0-{}", STREAM_RANGE_BYTES - 1);
    };

    let Some(range) = client_range.trim().strip_prefix("bytes=") else {
        return format!("bytes=0-{}", STREAM_RANGE_BYTES - 1);
    };
    let range = range.split(',').next().unwrap_or(range).trim();
    let Some((start, end)) = range.split_once('-') else {
        return format!("bytes=0-{}", STREAM_RANGE_BYTES - 1);
    };
    let Ok(start) = start.trim().parse::<u64>() else {
        return format!("bytes=0-{}", STREAM_RANGE_BYTES - 1);
    };

    let capped_end = match end.trim().parse::<u64>() {
        Ok(end) if end >= start => end.min(start + STREAM_RANGE_BYTES - 1),
        _ => start + STREAM_RANGE_BYTES - 1,
    };

    format!("bytes={start}-{capped_end}")
}

async fn write_response_head(
    stream: &mut TcpStream,
    status: reqwest::StatusCode,
    headers: &HeaderMap,
) -> io::Result<()> {
    let reason = status.canonical_reason().unwrap_or("OK");
    let mut response = format!("HTTP/1.1 {} {}\r\n", status.as_u16(), reason);

    for name in [
        "content-type",
        "content-length",
        "accept-ranges",
        "content-range",
        "cache-control",
        "etag",
        "last-modified",
    ] {
        if let Some(value) = headers.get(name).and_then(|value| value.to_str().ok()) {
            response.push_str(name);
            response.push_str(": ");
            response.push_str(value);
            response.push_str("\r\n");
        }
    }

    response.push_str("connection: close\r\n\r\n");
    stream.write_all(response.as_bytes()).await
}

async fn write_simple_response(
    stream: &mut TcpStream,
    status: u16,
    message: &str,
) -> io::Result<()> {
    let body = format!("{message}\n");
    let response = format!(
        "HTTP/1.1 {status} {message}\r\ncontent-length: {}\r\ncontent-type: text/plain; charset=utf-8\r\nconnection: close\r\n\r\n{}",
        body.len(),
        body
    );
    stream.write_all(response.as_bytes()).await
}
