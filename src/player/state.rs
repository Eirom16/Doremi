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
