use vlc::*;
use std::sync::{Arc, Mutex};

use super::state::PlayState;

#[derive(Clone)]
pub struct AudioEngine {
    inner: Arc<Mutex<AudioInner>>,
}

struct AudioInner {
    instance: Option<Instance>,
    player: Option<MediaPlayer>,
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

        engine
    }

    pub fn is_available(&self) -> bool {
        self.inner.lock().map(|i| i.player.is_some()).unwrap_or(false)
    }

    pub fn state(&self) -> PlayState {
        self.inner.lock().map(|i| i.state).unwrap_or(PlayState::Stopped)
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

    pub fn play_url(&self, url: &str) {
        // Lock 1: get instance reference to create Media
        let media = {
            let inner = match self.inner.lock() {
                Ok(i) => i,
                Err(_) => return,
            };
            let instance = match &inner.instance {
                Some(i) => i,
                None => return,
            };
            Media::new_location(instance, url)
        };

        let media = match media {
            Some(m) => m,
            None => return,
        };
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
                    self.play_url(&url);
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
                let eq = if !preset_name.is_empty() && preset_name != "Flat" {
                    // Try to find preset index
                    let mut preset_idx = None;
                    let count = sys::libvlc_audio_equalizer_get_preset_count();
                    for i in 0..count {
                        let ptr = sys::libvlc_audio_equalizer_get_preset_name(i);
                        if !ptr.is_null() {
                            if let Ok(s) = std::ffi::CStr::from_ptr(ptr).to_str() {
                                if s.eq_ignore_ascii_case(preset_name) {
                                    preset_idx = Some(i);
                                    break;
                                }
                            }
                        }
                    }
                    if let Some(idx) = preset_idx {
                        sys::libvlc_audio_equalizer_new_from_preset(idx)
                    } else {
                        sys::libvlc_audio_equalizer_new()
                    }
                } else {
                    sys::libvlc_audio_equalizer_new()
                };

                if eq.is_null() {
                    log::error!("Failed to create VLC equalizer");
                    return;
                }

                // Apply preamp
                let _ = sys::libvlc_audio_equalizer_set_preamp(eq, preamp as f32);

                // Apply bands (usually 10 bands)
                let num_bands = sys::libvlc_audio_equalizer_get_band_count() as usize;
                for i in 0..bands.len().min(num_bands) {
                    let _ = sys::libvlc_audio_equalizer_set_amp_at_index(eq, bands[i] as f32, i as u32);
                }

                // Apply to player
                if let Some(player) = &inner.player {
                    if sys::libvlc_media_player_set_equalizer(player.raw(), eq) == 0 {
                        log::info!("Applied VLC equalizer (preset: {}, preamp: {}dB)", preset_name, preamp);
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
