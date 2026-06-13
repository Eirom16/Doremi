use once_cell::sync::OnceCell;
use std::collections::HashMap;
use std::sync::RwLock;

static TRANSLATIONS: OnceCell<RwLock<HashMap<String, String>>> = OnceCell::new();
static CURRENT_LANGUAGE: OnceCell<RwLock<String>> = OnceCell::new();

fn translations() -> &'static RwLock<HashMap<String, String>> {
    TRANSLATIONS.get_or_init(|| RwLock::new(HashMap::new()))
}

fn current_language() -> &'static RwLock<String> {
    CURRENT_LANGUAGE.get_or_init(|| RwLock::new("es".to_string()))
}

/// Load translations from a JSON locale file
pub fn load_locale(lang: &str) {
    let path = match lang {
        "es" => include_str!("../locales/es.json"),
        "en" => include_str!("../locales/en.json"),
        _ => include_str!("../locales/es.json"),
    };

    if let Ok(map) = serde_json::from_str::<HashMap<String, String>>(path) {
        if let Ok(mut t) = translations().write() {
            t.clear();
            t.extend(map);
        }
    }

    if let Ok(mut l) = current_language().write() {
        *l = lang.to_string();
    }
}

/// Set the active language and load its locale
pub fn set_language(lang: &str) {
    load_locale(lang);
}

/// Get a translated string by key. Falls back to the key itself if not found.
pub fn tr(key: &str) -> String {
    if let Ok(t) = translations().read() {
        if let Some(val) = t.get(key) {
            return val.clone();
        }
    }
    key.to_string()
}

/// Get current language code
pub fn current_lang() -> String {
    if let Ok(l) = current_language().read() {
        l.clone()
    } else {
        "es".to_string()
    }
}

/// Macro for concise translation calls
#[macro_export]
macro_rules! tr {
    ($key:expr) => {
        $crate::utils::i18n::tr($key)
    };
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_load_and_translate() {
        set_language("es");
        let t = tr("home");
        assert!(!t.is_empty());
    }

    #[test]
    fn test_fallback_to_key() {
        set_language("es");
        let t = tr("nonexistent_key_xyz");
        assert_eq!(t, "nonexistent_key_xyz");
    }
}
