use discord_rich_presence::{activity, DiscordIpc, DiscordIpcClient};
use once_cell::sync::Lazy;
use std::sync::Mutex;

static DISCORD_CLIENT: Lazy<Mutex<Option<DiscordIpcClient>>> = Lazy::new(|| Mutex::new(None));
static ENABLED: Lazy<Mutex<bool>> = Lazy::new(|| Mutex::new(false));

pub fn set_enabled(enabled: bool) {
    let mut e = ENABLED.lock().unwrap();
    *e = enabled;
    if !enabled {
        disconnect();
    } else {
        log::info!("Discord RPC enabled");
    }
}

pub fn is_enabled() -> bool {
    *ENABLED.lock().unwrap()
}

pub fn disconnect() {
    let mut client = DISCORD_CLIENT.lock().unwrap();
    if let Some(mut c) = client.take() {
        let _ = c.close();
        log::info!("Discord RPC disconnected");
    }
}

fn update_presence_internal(
    title: &str,
    artist: &str,
    album: &str,
    is_playing: bool,
) -> Result<(), Box<dyn std::error::Error>> {
    let mut client_guard = DISCORD_CLIENT.lock().unwrap();

    // Lazy initialization & connection
    if client_guard.is_none() {
        log::info!("Connecting to Discord RPC...");
        let mut client = DiscordIpcClient::new("1395462809164263495");
        client.connect()?;
        log::info!("Discord RPC connected successfully");
        *client_guard = Some(client);
    }

    if let Some(client) = client_guard.as_mut() {
        let state = if artist.is_empty() {
            "YouTube Music".to_string()
        } else {
            format!("by {artist}")
        };

        let details = title.to_string();
        let album_name = if album.is_empty() { "Doremi" } else { album };

        let small_img = if is_playing { "play" } else { "pause" };
        let small_txt = if is_playing { "Playing" } else { "Paused" };

        let assets = activity::Assets::new()
            .large_image("music")
            .large_text(album_name)
            .small_image(small_img)
            .small_text(small_txt);

        let payload = activity::Activity::new()
            .state(&state)
            .details(&details)
            .assets(assets);

        client.set_activity(payload)?;
    }

    Ok(())
}

pub fn update_presence(title: &str, artist: &str, album: &str, is_playing: bool) {
    if !is_enabled() {
        return;
    }

    let title = title.to_string();
    let artist = artist.to_string();
    let album = album.to_string();

    tokio::spawn(async move {
        // Since connecting to Discord IPC client uses blocking sockets/named pipes,
        // we wrap it in spawn_blocking and enforce a timeout of 5 seconds.
        let join_res = tokio::time::timeout(
            tokio::time::Duration::from_secs(5),
            tokio::task::spawn_blocking(move || {
                update_presence_internal(&title, &artist, &album, is_playing)
                    .map_err(|e| e.to_string())
            }),
        )
        .await;

        match join_res {
            Ok(Ok(Ok(()))) => {
                // Success
            }
            Ok(Ok(Err(e))) => {
                log::warn!("Discord RPC error: {e}");
                // Reset connection on error so we try reconnecting next time
                if let Ok(mut client_guard) = DISCORD_CLIENT.lock() {
                    *client_guard = None;
                }
            }
            Ok(Err(join_err)) => {
                log::error!("Discord RPC spawn_blocking panicked: {join_err}");
            }
            Err(_timeout_err) => {
                log::warn!("Discord RPC connection timed out (5s limit reached)");
                // Reset connection on timeout so we try reconnecting next time
                if let Ok(mut client_guard) = DISCORD_CLIENT.lock() {
                    *client_guard = None;
                }
            }
        }
    });
}
