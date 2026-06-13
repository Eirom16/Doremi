use serde::{Deserialize, Serialize};
use rand::seq::SliceRandom;

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct TrackInfo {
    pub id: String,
    pub title: String,
    pub artist: String,
    pub album: String,
    pub duration_ms: i64,
    pub thumbnail: String,
    pub stream_url: String,
}

#[derive(Debug, Clone)]
pub struct PlaybackQueue {
    tracks: Vec<TrackInfo>,
    current_index: usize,
    shuffled: Vec<usize>,
    shuffle_mode: bool,
    repeat_mode: RepeatMode,
}

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum RepeatMode {
    None,
    All,
    One,
}

impl PlaybackQueue {
    pub fn new() -> Self {
        Self {
            tracks: Vec::new(),
            current_index: 0,
            shuffled: Vec::new(),
            shuffle_mode: false,
            repeat_mode: RepeatMode::None,
        }
    }

    pub fn is_empty(&self) -> bool {
        self.tracks.is_empty()
    }

    pub fn len(&self) -> usize {
        self.tracks.len()
    }

    pub fn current_index(&self) -> usize {
        self.current_index
    }

    pub fn current(&self) -> Option<&TrackInfo> {
        self.tracks.get(self.current_index)
    }

    pub fn current_mut(&mut self) -> Option<&mut TrackInfo> {
        self.tracks.get_mut(self.current_index)
    }

    pub fn shuffle_mode(&self) -> bool {
        self.shuffle_mode
    }

    pub fn repeat_mode(&self) -> RepeatMode {
        self.repeat_mode
    }

    pub fn toggle_shuffle(&mut self) {
        self.shuffle_mode = !self.shuffle_mode;
        if self.shuffle_mode {
            self.build_shuffle();
        }
    }

    pub fn set_repeat_mode(&mut self, mode: RepeatMode) {
        self.repeat_mode = mode;
    }

    pub fn cycle_repeat(&mut self) {
        self.repeat_mode = match self.repeat_mode {
            RepeatMode::None => RepeatMode::All,
            RepeatMode::All => RepeatMode::One,
            RepeatMode::One => RepeatMode::None,
        };
    }

    fn build_shuffle(&mut self) {
        let n = self.tracks.len();
        self.shuffled = (0..n).collect();
        self.shuffled.shuffle(&mut rand::rng());
        if self.current_index < n {
            self.shuffled.swap(0, self.current_index);
        }
        self.current_index = 0;
    }

    #[allow(dead_code)]
    fn actual_index(&self) -> usize {
        if self.shuffle_mode && !self.shuffled.is_empty() {
            self.shuffled.get(self.current_index).copied().unwrap_or(0)
        } else {
            self.current_index
        }
    }

    pub fn enqueue(&mut self, track: TrackInfo) {
        self.tracks.push(track);
        if self.shuffle_mode {
            self.build_shuffle();
        }
    }

    pub fn enqueue_front(&mut self, track: TrackInfo) {
        self.tracks.insert(0, track);
        if self.shuffle_mode {
            self.build_shuffle();
        }
    }

    pub fn remove(&mut self, index: usize) -> Option<TrackInfo> {
        if index >= self.tracks.len() {
            return None;
        }
        let removed = self.tracks.remove(index);
        if self.shuffle_mode {
            self.build_shuffle();
        } else if index < self.current_index {
            self.current_index = self.current_index.saturating_sub(1);
        }
        Some(removed)
    }

    pub fn clear(&mut self) {
        self.tracks.clear();
        self.current_index = 0;
        self.shuffled.clear();
    }

    pub fn next(&mut self) -> Option<&TrackInfo> {
        if self.tracks.is_empty() {
            return None;
        }
        if self.repeat_mode == RepeatMode::One {
            return self.tracks.get(self.current_index);
        }
        if self.current_index + 1 < self.tracks.len() {
            self.current_index += 1;
        } else if self.repeat_mode == RepeatMode::All {
            self.current_index = 0;
        } else {
            return None;
        }
        self.current()
    }

    pub fn previous(&mut self) -> Option<&TrackInfo> {
        if self.tracks.is_empty() {
            return None;
        }
        if self.current_index > 0 {
            self.current_index -= 1;
        } else if self.repeat_mode == RepeatMode::All {
            self.current_index = self.tracks.len() - 1;
        }
        self.current()
    }

    pub fn jump_to(&mut self, index: usize) -> Option<&TrackInfo> {
        if index < self.tracks.len() {
            self.current_index = index;
            self.current()
        } else {
            None
        }
    }

    pub fn set_tracks(&mut self, tracks: Vec<TrackInfo>) {
        self.tracks = tracks;
        self.current_index = 0;
        if self.shuffle_mode {
            self.build_shuffle();
        }
    }

    pub fn all_tracks(&self) -> &[TrackInfo] {
        &self.tracks
    }

    pub fn iter(&self) -> impl Iterator<Item = (usize, &TrackInfo)> {
        self.tracks.iter().enumerate()
    }
}
