use std::sync::{Arc, Mutex};
use vlc::*;

use super::state::PlayState;

const EXPECTED_EQ_BANDS: usize = 10;
const YOUTUBE_ANDROID_USER_AGENT: &str =
    "com.google.android.youtube/20.10.38 (Linux; U; Android 15)";

fn normalized_equalizer(preamp: f64, bands: &[f64], available_bands: usize) -> (f32, Vec<f32>) {
    let mut values = bands
        .iter()
        .take(available_bands)
        .map(|value| value.clamp(-20.0, 20.0) as f32)
        .collect::<Vec<_>>();
    values.resize(available_bands, 0.0);

    // Subtractive equalization: reduce preamp by the maximum boost to avoid clipping
    let max_boost = values.iter().copied().fold(0.0f32, |a, b| a.max(b));
    let adjusted_preamp = if max_boost > 0.0 {
        preamp as f32 - max_boost
    } else {
        preamp as f32
    };
    let adjusted_preamp = adjusted_preamp.clamp(-20.0, 20.0);

    (adjusted_preamp, values)
}

fn add_media_option(media: &Media, option: &str) {
    unsafe {
        if let Ok(option_cstr) = std::ffi::CString::new(option) {
            sys::libvlc_media_add_option(media.raw(), option_cstr.as_ptr());
        }
    }
}

fn add_youtube_http_options(media: &Media, url: &str) {
    if !(url.contains("googlevideo.com") || url.contains("youtube.com") || url.contains("youtu.be"))
    {
        return;
    }

    add_media_option(
        media,
        &format!(":http-user-agent={YOUTUBE_ANDROID_USER_AGENT}"),
    );
    add_media_option(media, ":http-referrer=https://www.youtube.com/");
    add_media_option(media, ":network-caching=1500");
    add_media_option(media, ":http-reconnect=true");
}

#[derive(Clone)]
pub struct AudioEngine {
    inner: Arc<Mutex<AudioInner>>,
}

struct AudioInner {
    instance: Option<Instance>,
    player: Option<MediaPlayer>,
    secondary_player: Option<MediaPlayer>,
    state: PlayState,
    position_ms: i64,
    duration_ms: i64,
    volume: i32,
    stream_url: Option<String>,
    equalizer: Option<*mut sys::libvlc_equalizer_t>,
}

impl Drop for AudioInner {
    fn drop(&mut self) {
        if let Some(eq) = self.equalizer.take() {
            unsafe {
                sys::libvlc_audio_equalizer_release(eq);
            }
        }
        if let Some(p) = self.player.take() {
            p.stop();
        }
        if let Some(sp) = self.secondary_player.take() {
            sp.stop();
        }
    }
}

// Safe: all access is serialized through the Mutex
unsafe impl Send for AudioInner {}
unsafe impl Sync for AudioInner {}

impl AudioEngine {
    pub fn new() -> Self {
        let inner = Arc::new(Mutex::new(AudioInner {
            instance: None,
            player: None,
            secondary_player: None,
            state: PlayState::Stopped,
            position_ms: 0,
            duration_ms: 0,
            volume: 50,
            stream_url: None,
            equalizer: None,
        }));

        let engine = Self { inner };

        if let Some(instance) = Instance::new() {
            if let Some(player) = MediaPlayer::new(&instance) {
                if let Ok(mut inner) = engine.inner.lock() {
                    inner.instance = Some(instance);
                    inner.player = Some(player);
                    log::info!("VLC MediaPlayer created");
                }
            }
        }

        let band_count = unsafe { sys::libvlc_audio_equalizer_get_band_count() as usize };
        if band_count == EXPECTED_EQ_BANDS {
            log::info!("VLC equalizer capability verified: {band_count} bands");
        } else {
            log::warn!(
                "VLC exposes {band_count} equalizer bands; Doremi expects {EXPECTED_EQ_BANDS}"
            );
        }

        engine
    }

    pub fn is_available(&self) -> bool {
        self.inner
            .lock()
            .map(|i| i.player.is_some())
            .unwrap_or(false)
    }

    pub fn state(&self) -> PlayState {
        self.inner
            .lock()
            .map(|i| i.state)
            .unwrap_or(PlayState::Stopped)
    }

    pub fn has_error(&self) -> bool {
        if let Ok(inner) = self.inner.lock() {
            if let Some(player) = &inner.player {
                return player.state() == State::Error;
            }
        }
        false
    }

    pub fn has_ended(&self) -> bool {
        self.inner
            .lock()
            .ok()
            .and_then(|inner| {
                inner
                    .player
                    .as_ref()
                    .map(|player| player.state() == State::Ended)
            })
            .unwrap_or(false)
    }

    pub fn position_ms(&self) -> i64 {
        self.inner.lock().map(|i| i.position_ms).unwrap_or(0)
    }

    pub fn duration_ms(&self) -> i64 {
        self.inner.lock().map(|i| i.duration_ms).unwrap_or(0)
    }

    pub fn volume(&self) -> i32 {
        self.inner.lock().map(|i| i.volume).unwrap_or(50)
    }

    pub fn play_url(&self, url: &str, normalize: bool) {
        // Lock 1: get instance reference to create Media
        let playback_url = crate::player::stream_proxy::proxied_url(url);
        let media = {
            let inner = match self.inner.lock() {
                Ok(i) => i,
                Err(_) => return,
            };
            let instance = match &inner.instance {
                Some(i) => i,
                None => return,
            };
            Media::new_location(instance, &playback_url)
        };

        let media = match media {
            Some(m) => m,
            None => return,
        };
        add_youtube_http_options(&media, url);
        if normalize {
            add_media_option(&media, ":audio-filter=normvol");
        }
        media.parse();
        let dur = media.duration().unwrap_or(0);

        // Lock 2: update state
        if let Ok(mut inner) = self.inner.lock() {
            inner.duration_ms = dur;
            inner.stream_url = Some(url.to_string());
            inner.state = PlayState::Loading;
        }

        // Lock 3: set media and play
        if let Ok(inner) = self.inner.lock() {
            if let Some(player) = &inner.player {
                player.set_media(&media);
                let _ = player.play();
            }
        }
    }

    pub fn play_crossfade(&self, url: &str, normalize: bool) {
        let playback_url = crate::player::stream_proxy::proxied_url(url);
        let media = {
            let inner = match self.inner.lock() {
                Ok(i) => i,
                Err(_) => return,
            };
            let instance = match &inner.instance {
                Some(i) => i,
                None => return,
            };
            Media::new_location(instance, &playback_url)
        };

        let media = match media {
            Some(m) => m,
            None => return,
        };
        add_youtube_http_options(&media, url);
        if normalize {
            add_media_option(&media, ":audio-filter=normvol");
        }
        media.parse();

        if let Ok(mut inner) = self.inner.lock() {
            if let Some(instance) = &inner.instance {
                if inner.secondary_player.is_none() {
                    inner.secondary_player = MediaPlayer::new(instance);
                }
                if let Some(sec_player) = &inner.secondary_player {
                    sec_player.set_media(&media);
                    if let Some(eq) = inner.equalizer {
                        unsafe {
                            sys::libvlc_media_player_set_equalizer(sec_player.raw(), eq);
                        }
                    }
                    let _ = sec_player.set_volume(0);
                    let _ = sec_player.play();
                }
            }
        }
    }

    pub fn set_fade_volumes(&self, primary_pct: f64, secondary_pct: f64) {
        if let Ok(inner) = self.inner.lock() {
            let target_vol = inner.volume as f64;
            if let Some(p) = &inner.player {
                let _ = p.set_volume((target_vol * primary_pct) as i32);
            }
            if let Some(sp) = &inner.secondary_player {
                let _ = sp.set_volume((target_vol * secondary_pct) as i32);
            }
        }
    }

    pub fn complete_crossfade(&self) {
        if let Ok(mut inner) = self.inner.lock() {
            if let Some(p) = inner.player.take() {
                p.stop();
            }
            inner.player = inner.secondary_player.take();
            let target_vol = inner.volume;
            if let Some(p) = &inner.player {
                let _ = p.set_volume(target_vol);
            }
            if let Some(p) = &inner.player {
                if let Some(media) = p.get_media() {
                    inner.duration_ms = media.duration().unwrap_or(0);
                }
            }
            inner.position_ms = 0;
        }
    }

    pub fn toggle_play_pause(&self) {
        let mut inner = match self.inner.lock() {
            Ok(i) => i,
            Err(_) => return,
        };
        let player = match &inner.player {
            Some(p) => p,
            None => return,
        };

        match inner.state {
            PlayState::Stopped | PlayState::Loading => {
                let url = inner.stream_url.clone();
                drop(inner);
                if let Some(url) = url {
                    self.play_url(&url, false);
                }
            }
            PlayState::Playing => {
                player.set_pause(true);
                inner.state = PlayState::Paused;
            }
            PlayState::Paused => {
                player.set_pause(false);
                inner.state = PlayState::Playing;
            }
        }
    }

    pub fn pause(&self) {
        if let Ok(mut inner) = self.inner.lock() {
            if let Some(player) = &inner.player {
                player.set_pause(true);
                inner.state = PlayState::Paused;
            }
        }
    }

    pub fn resume(&self) {
        if let Ok(mut inner) = self.inner.lock() {
            if let Some(player) = &inner.player {
                player.set_pause(false);
                inner.state = PlayState::Playing;
            }
        }
    }

    pub fn stop(&self) {
        if let Ok(mut inner) = self.inner.lock() {
            if let Some(player) = &inner.player {
                player.stop();
                inner.state = PlayState::Stopped;
                inner.position_ms = 0;
            }
        }
    }

    pub fn seek(&self, position_ms: i64) {
        if let Ok(inner) = self.inner.lock() {
            if let Some(player) = &inner.player {
                player.set_time(position_ms);
            }
        }
    }

    pub fn set_volume(&self, volume: i32) {
        if let Ok(mut inner) = self.inner.lock() {
            inner.volume = volume.clamp(0, 100);
            if let Some(player) = &inner.player {
                let _ = player.set_volume(inner.volume);
            }
        }
    }

    pub fn adjust_volume(&self, delta: i32) {
        self.set_volume(self.volume() + delta);
    }

    pub fn poll_position(&self) {
        let pos;
        let vlc_state;
        {
            let inner = match self.inner.lock() {
                Ok(i) => i,
                Err(_) => return,
            };
            let player = match &inner.player {
                Some(p) => p,
                None => return,
            };
            pos = player.get_time();
            vlc_state = player.state();
        }
        if let Ok(mut inner) = self.inner.lock() {
            if let Some(t) = pos {
                inner.position_ms = t;
            }
            inner.state = match vlc_state {
                State::Playing => PlayState::Playing,
                State::Paused => PlayState::Paused,
                State::Stopped | State::Ended => PlayState::Stopped,
                _ => inner.state,
            };
        }
    }

    pub fn set_state(&self, state: PlayState) {
        if let Ok(mut inner) = self.inner.lock() {
            inner.state = state;
        }
    }

    pub fn apply_equalizer(&self, enabled: bool, preamp: f64, bands: &[f64], preset_name: &str) {
        if let Ok(mut inner) = self.inner.lock() {
            // First release old equalizer if any
            if let Some(eq) = inner.equalizer.take() {
                unsafe {
                    sys::libvlc_audio_equalizer_release(eq);
                }
            }

            if !enabled {
                // Remove equalizer from media player
                if let Some(player) = &inner.player {
                    unsafe {
                        sys::libvlc_media_player_set_equalizer(player.raw(), std::ptr::null_mut());
                    }
                }
                return;
            }

            // Create new equalizer
            unsafe {
                let eq = sys::libvlc_audio_equalizer_new();

                if eq.is_null() {
                    log::error!("Failed to create VLC equalizer");
                    return;
                }

                let num_bands = sys::libvlc_audio_equalizer_get_band_count() as usize;
                let (preamp, bands) = normalized_equalizer(preamp, bands, num_bands);
                if sys::libvlc_audio_equalizer_set_preamp(eq, preamp) != 0 {
                    log::warn!("VLC rejected equalizer preamp {preamp}dB");
                }
                for (index, value) in bands.iter().enumerate() {
                    if sys::libvlc_audio_equalizer_set_amp_at_index(eq, *value, index as u32) != 0 {
                        log::warn!("VLC rejected equalizer band {index} value {value}dB");
                    }
                }

                // Apply to player
                if let Some(player) = &inner.player {
                    if sys::libvlc_media_player_set_equalizer(player.raw(), eq) == 0 {
                        log::info!(
                            "Applied VLC equalizer (preset: {}, preamp: {}dB)",
                            preset_name,
                            preamp
                        );
                        inner.equalizer = Some(eq);
                    } else {
                        log::error!("Failed to apply equalizer to media player");
                        sys::libvlc_audio_equalizer_release(eq);
                    }
                } else {
                    inner.equalizer = Some(eq);
                }
            }
        }
    }
}

#[cfg(test)]
mod tests {
    use super::normalized_equalizer;

    #[test]
    fn equalizer_values_are_clamped_padded_and_truncated_for_runtime_band_count() {
        let (preamp, bands) = normalized_equalizer(30.0, &[-25.0, 2.5, 30.0], 2);
        assert_eq!(preamp, 20.0);
        assert_eq!(bands, vec![-20.0, 2.5]);

        let (_, padded) = normalized_equalizer(0.0, &[1.0], 3);
        assert_eq!(padded, vec![1.0, 0.0, 0.0]);
    }

    #[test]
    fn test_subtractive_equalizer_prevents_clipping() {
        // Equalizer with positive boost should decrease preamp
        let (preamp, bands) = normalized_equalizer(10.0, &[5.0, 2.0, -1.0], 3);
        assert_eq!(preamp, 5.0); // 10.0 - 5.0 (max boost)
        assert_eq!(bands, vec![5.0, 2.0, -1.0]);

        // Equalizer with only cut/negative boost should not decrease preamp
        let (preamp2, bands2) = normalized_equalizer(10.0, &[-5.0, -2.0, -10.0], 3);
        assert_eq!(preamp2, 10.0); // max boost is <= 0.0, so no change
        assert_eq!(bands2, vec![-5.0, -2.0, -10.0]);
    }
}
