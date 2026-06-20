use directories::ProjectDirs;
use once_cell::sync::OnceCell;
use std::path::PathBuf;

static APP_DIRS: OnceCell<AppDirs> = OnceCell::new();

#[derive(Debug, Clone)]
pub struct AppDirs {
    project: ProjectDirs,
}

impl AppDirs {
    pub fn new() -> Option<Self> {
        ProjectDirs::from("com", "doremi", "Doremi").map(|project| Self { project })
    }

    pub fn global() -> &'static AppDirs {
        APP_DIRS
            .get()
            .expect("AppDirs not initialized. Call AppDirs::setup() first.")
    }

    pub fn setup() {
        APP_DIRS.get_or_init(|| Self::new().expect("Could not determine application directories"));
        let dirs = Self::global();
        for dir in &[
            dirs.config_dir(),
            dirs.data_dir(),
            dirs.cache_dir(),
            dirs.artwork_cache_dir(),
            dirs.lyrics_cache_dir(),
            dirs.downloads_dir(),
            dirs.logs_dir(),
        ] {
            std::fs::create_dir_all(dir).ok();
        }
    }

    pub fn config_dir(&self) -> PathBuf {
        self.project.config_dir().to_path_buf()
    }

    pub fn data_dir(&self) -> PathBuf {
        self.project.data_dir().to_path_buf()
    }

    pub fn cache_dir(&self) -> PathBuf {
        self.project.cache_dir().to_path_buf()
    }

    pub fn artwork_cache_dir(&self) -> PathBuf {
        self.cache_dir().join("artwork")
    }

    pub fn lyrics_cache_dir(&self) -> PathBuf {
        self.cache_dir().join("lyrics")
    }

    pub fn downloads_dir(&self) -> PathBuf {
        self.data_dir().join("downloads")
    }

    pub fn logs_dir(&self) -> PathBuf {
        self.data_dir().join("logs")
    }

    pub fn database_path(&self) -> PathBuf {
        self.data_dir().join("doremi.db")
    }

    pub fn settings_path(&self) -> PathBuf {
        self.config_dir().join("settings.toml")
    }

    pub fn vlc_dir(&self) -> PathBuf {
        self.data_dir().join("vlc")
    }

    /// Project root directory (for assets bundled with the binary)
    pub fn root_dir() -> PathBuf {
        let exe = std::env::current_exe().ok();
        if let Some(path) = exe {
            if let Some(parent) = path.parent() {
                // In development, assets are at the project root
                let dev_root = parent.join("assets");
                if dev_root.exists() {
                    return parent.to_path_buf();
                }
            }
        }
        PathBuf::from(".")
    }
}
