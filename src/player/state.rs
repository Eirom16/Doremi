#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum PlayState {
    Stopped,
    Loading,
    Playing,
    Paused,
}

impl PlayState {
    pub fn is_active(self) -> bool {
        matches!(self, Self::Playing | Self::Paused)
    }

    pub fn is_playing(self) -> bool {
        self == Self::Playing
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn stopped_is_not_active_or_playing() {
        let state = PlayState::Stopped;
        assert!(!state.is_active());
        assert!(!state.is_playing());
    }

    #[test]
    fn loading_is_not_active_or_playing() {
        let state = PlayState::Loading;
        assert!(!state.is_active());
        assert!(!state.is_playing());
    }

    #[test]
    fn playing_is_active_and_playing() {
        let state = PlayState::Playing;
        assert!(state.is_active(), "Playing should be active");
        assert!(state.is_playing(), "Playing should report is_playing");
    }

    #[test]
    fn paused_is_active_but_not_playing() {
        let state = PlayState::Paused;
        assert!(
            state.is_active(),
            "Paused should be active (track is loaded)"
        );
        assert!(!state.is_playing(), "Paused should not report is_playing");
    }

    #[test]
    fn all_states_are_distinct() {
        let states = [
            PlayState::Stopped,
            PlayState::Loading,
            PlayState::Playing,
            PlayState::Paused,
        ];
        // Each pair should be distinct
        for (i, a) in states.iter().enumerate() {
            for (j, b) in states.iter().enumerate() {
                if i == j {
                    assert_eq!(a, b);
                } else {
                    assert_ne!(a, b, "States {a:?} and {b:?} should be distinct");
                }
            }
        }
    }
}
