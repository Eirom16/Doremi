use rand::seq::SliceRandom;
use serde::{Deserialize, Serialize};

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
    shuffle_position: usize,
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
            shuffle_position: 0,
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
        } else {
            self.shuffled.clear();
            self.shuffle_position = 0;
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
        if n == 0 {
            self.shuffled.clear();
            self.shuffle_position = 0;
            self.current_index = 0;
            return;
        }

        self.current_index = self.current_index.min(n - 1);
        self.shuffled = (0..n)
            .filter(|index| *index != self.current_index)
            .collect();
        self.shuffled.shuffle(&mut rand::rng());
        self.shuffled.insert(0, self.current_index);
        self.shuffle_position = 0;
    }

    pub fn enqueue(&mut self, track: TrackInfo) {
        self.tracks.push(track);
        if self.shuffle_mode {
            self.build_shuffle();
        }
    }

    pub fn enqueue_next(&mut self, track: TrackInfo) {
        let insert_at = if self.tracks.is_empty() {
            0
        } else {
            (self.current_index + 1).min(self.tracks.len())
        };
        self.tracks.insert(insert_at, track);
        if self.shuffle_mode {
            self.build_shuffle();
        }
    }

    pub fn remove(&mut self, index: usize) -> Option<TrackInfo> {
        if index >= self.tracks.len() {
            return None;
        }
        let removed = self.tracks.remove(index);
        if self.tracks.is_empty() {
            self.current_index = 0;
        } else if index < self.current_index {
            self.current_index -= 1;
        } else if self.current_index >= self.tracks.len() {
            self.current_index = self.tracks.len() - 1;
        }
        if self.shuffle_mode {
            self.build_shuffle();
        }
        Some(removed)
    }

    pub fn move_item(&mut self, from: usize, to: usize) -> bool {
        if from >= self.tracks.len() || to >= self.tracks.len() || from == to {
            return false;
        }

        let current = self.current_index;
        let track = self.tracks.remove(from);
        self.tracks.insert(to, track);
        self.current_index = if current == from {
            to
        } else if from < current && to >= current {
            current - 1
        } else if from > current && to <= current {
            current + 1
        } else {
            current
        };

        if self.shuffle_mode {
            self.build_shuffle();
        }
        true
    }

    pub fn clear(&mut self) {
        self.tracks.clear();
        self.current_index = 0;
        self.shuffled.clear();
        self.shuffle_position = 0;
    }

    pub fn next(&mut self) -> Option<&TrackInfo> {
        if self.tracks.is_empty() {
            return None;
        }
        if self.repeat_mode == RepeatMode::One {
            return self.tracks.get(self.current_index);
        }
        if self.shuffle_mode {
            if self.shuffle_position + 1 < self.shuffled.len() {
                self.shuffle_position += 1;
            } else if self.repeat_mode == RepeatMode::All {
                self.shuffle_position = 0;
            } else {
                return None;
            }
            self.current_index = self.shuffled[self.shuffle_position];
            return self.current();
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
        if self.shuffle_mode {
            if self.shuffle_position > 0 {
                self.shuffle_position -= 1;
            } else if self.repeat_mode == RepeatMode::All {
                self.shuffle_position = self.shuffled.len() - 1;
            }
            self.current_index = self.shuffled[self.shuffle_position];
            return self.current();
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
            if self.shuffle_mode {
                if let Some(position) = self.shuffled.iter().position(|value| *value == index) {
                    self.shuffle_position = position;
                } else {
                    self.build_shuffle();
                }
            }
            self.current()
        } else {
            None
        }
    }

    pub fn set_tracks(&mut self, tracks: Vec<TrackInfo>) {
        self.tracks = tracks;
        self.current_index = 0;
        self.shuffle_position = 0;
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

#[cfg(test)]
mod tests {
    use super::*;

    fn track(id: &str) -> TrackInfo {
        TrackInfo {
            id: id.to_string(),
            title: id.to_string(),
            artist: String::new(),
            album: String::new(),
            duration_ms: 0,
            thumbnail: String::new(),
            stream_url: String::new(),
        }
    }

    fn ids(queue: &PlaybackQueue) -> Vec<&str> {
        queue
            .all_tracks()
            .iter()
            .map(|track| track.id.as_str())
            .collect()
    }

    #[test]
    fn enqueue_next_inserts_after_current_track() {
        let mut queue = PlaybackQueue::new();
        queue.set_tracks(vec![track("a"), track("b"), track("c")]);
        queue.jump_to(1);

        queue.enqueue_next(track("next"));

        assert_eq!(ids(&queue), vec!["a", "b", "next", "c"]);
        assert_eq!(queue.current().map(|track| track.id.as_str()), Some("b"));
    }

    #[test]
    fn remove_and_move_preserve_current_track_identity() {
        let mut queue = PlaybackQueue::new();
        queue.set_tracks(vec![track("a"), track("b"), track("c"), track("d")]);
        queue.jump_to(2);

        assert!(queue.move_item(0, 3));
        assert_eq!(ids(&queue), vec!["b", "c", "d", "a"]);
        assert_eq!(queue.current().map(|track| track.id.as_str()), Some("c"));

        assert_eq!(queue.remove(0).map(|track| track.id), Some("b".to_string()));
        assert_eq!(queue.current().map(|track| track.id.as_str()), Some("c"));
    }

    #[test]
    fn queue_operations_reject_invalid_indices_and_reset_when_empty() {
        let mut queue = PlaybackQueue::new();
        queue.enqueue(track("a"));

        assert!(!queue.move_item(0, 1));
        assert!(queue.remove(1).is_none());
        assert_eq!(queue.remove(0).map(|track| track.id), Some("a".to_string()));
        assert!(queue.is_empty());
        assert_eq!(queue.current_index(), 0);
        assert!(queue.jump_to(0).is_none());
    }

    #[test]
    fn shuffle_preserves_original_order_and_current_track_when_toggled() {
        let mut queue = PlaybackQueue::new();
        queue.set_tracks(vec![track("a"), track("b"), track("c"), track("d")]);
        queue.jump_to(2);
        let original: Vec<String> = ids(&queue).into_iter().map(str::to_string).collect();

        queue.toggle_shuffle();

        assert!(queue.shuffle_mode());
        assert_eq!(ids(&queue), vec!["a", "b", "c", "d"]);
        assert_eq!(queue.current().map(|track| track.id.as_str()), Some("c"));
        assert_eq!(queue.current_index(), 2);

        queue.toggle_shuffle();

        assert!(!queue.shuffle_mode());
        assert_eq!(
            ids(&queue),
            original.iter().map(String::as_str).collect::<Vec<_>>()
        );
        assert_eq!(queue.current().map(|track| track.id.as_str()), Some("c"));
        assert_eq!(queue.next().map(|track| track.id.as_str()), Some("d"));
    }

    #[test]
    fn shuffle_visits_each_track_once_without_mutating_the_queue() {
        let mut queue = PlaybackQueue::new();
        queue.set_tracks(vec![track("a"), track("b"), track("c"), track("d")]);
        queue.jump_to(1);
        queue.toggle_shuffle();

        let mut visited = vec![queue.current().unwrap().id.clone()];
        while let Some(next) = queue.next() {
            visited.push(next.id.clone());
        }
        visited.sort();

        assert_eq!(visited, vec!["a", "b", "c", "d"]);
        assert_eq!(ids(&queue), vec!["a", "b", "c", "d"]);
    }
}
