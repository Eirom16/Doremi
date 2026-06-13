use reqwest::Client;
use serde::{Deserialize, Serialize};

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct LyricsResponse {
    pub id: Option<i64>,
    pub name: String,
    #[serde(rename = "artistName")]
    pub artist_name: String,
    #[serde(rename = "albumName")]
    pub album_name: Option<String>,
    pub duration: Option<f64>,
    #[serde(rename = "plainLyrics")]
    pub plain_lyrics: Option<String>,
    #[serde(rename = "syncedLyrics")]
    pub synced_lyrics: Option<String>,
}

pub struct LyricsService {
    client: Client,
}

impl LyricsService {
    pub fn new() -> Self {
        Self {
            client: Client::builder()
                .timeout(std::time::Duration::from_secs(5))
                .build()
                .unwrap_or_default(),
        }
    }

    pub async fn fetch_lyrics(&self, title: &str, artist: &str) -> Result<Option<LyricsResponse>, String> {
        // Clean metadata like Pyrolist did
        let clean_title = Self::clean_title(title);
        let clean_artist = Self::clean_artist(artist);

        log::info!("Fetching lyrics for '{}' by '{}'...", clean_title, clean_artist);

        let url = "https://lrclib.net/api/get";
        let query = [("title", &clean_title), ("artist", &clean_artist)];
        
        let resp = self.client.get(url)
            .query(&query)
            .header("User-Agent", "Doremi Music Player v2.0.0 (https://github.com/eirom/doremi)")
            .send()
            .await
            .map_err(|e| format!("Request error: {e}"))?;

        if resp.status() == 404 {
            log::info!("No lyrics found for '{}' by '{}'", clean_title, clean_artist);
            return Ok(None);
        }

        if !resp.status().is_success() {
            return Err(format!("LrcLib returned status: {}", resp.status()));
        }

        let lyrics: LyricsResponse = resp.json()
            .await
            .map_err(|e| format!("JSON parse error: {e}"))?;

        Ok(Some(lyrics))
    }

    fn clean_title(title: &str) -> String {
        let mut t = title.to_string();
        // Remove common suffixes in video titles
        let regexes = [
            r"(?i)\s*[\(\[][Oo]fficial\s+[Vv]ideo[\)\]]",
            r"(?i)\s*[\(\[][Oo]fficial\s+[Aa]udio[\)\]]",
            r"(?i)\s*[\(\[][Oo]fficial\s+[Mm]usic\s+[Vv]ideo[\)\]]",
            r"(?i)\s*[\(\[][Oo]fficial\s+[Ll]yric\s+[Vv]ideo[\)\]]",
            r"(?i)\s*[\(\[][Ll]yric\s+[Vv]ideo[\)\]]",
            r"(?i)\s*[\(\[][Vv]ideo\s+[Cc]lip[\)\]]",
            r"(?i)\s*[\(\[][Hh][Dd][\)\]]",
            r"(?i)\s*[\(\[][4]k[\)\]]",
            r"(?i)\s*[\(\[][Ff]eat\.\s+.*?[\)\]]",
            r"(?i)\s*[\(\[][Ff]t\.\s+.*?[\)\]]"
        ];
        for re_str in regexes {
            if let Ok(re) = regex::Regex::new(re_str) {
                t = re.replace_all(&t, "").to_string();
            }
        }
        t.trim().to_string()
    }

    fn clean_artist(artist: &str) -> String {
        let mut a = artist.to_string();
        if let Ok(re) = regex::Regex::new(r"(?i)\s*-\s*[Tt]opic") {
            a = re.replace_all(&a, "").to_string();
        }
        a.trim().to_string()
    }
}

impl Default for LyricsService {
    fn default() -> Self {
        Self::new()
    }
}
