use std::path::Path;

pub fn redact_url(value: &str) -> String {
    let Ok(mut url) = reqwest::Url::parse(value) else {
        return "<redacted-url>".to_string();
    };
    url.set_query(None);
    url.set_fragment(None);
    if url.password().is_some() {
        let _ = url.set_password(Some("<redacted>"));
    }
    if !url.username().is_empty() {
        let _ = url.set_username("<redacted>");
    }
    url.to_string()
}

pub fn redact_secrets(text: &str) -> String {
    use regex::Regex;
    use std::sync::OnceLock;

    let sentinels = [
        "sentinel-google-secret",
        "sentinel-lastfm-secret",
        "sentinel-session-token",
        "sentinel-youtube-cookie",
    ];
    let mut redacted = text.to_string();
    for sentinel in &sentinels {
        redacted = redacted.replace(sentinel, "<redacted>");
    }

    static RE_HEADER: OnceLock<Regex> = OnceLock::new();
    let re_header = RE_HEADER.get_or_init(|| {
        Regex::new(r"(?i)(authorization|auth|cookie|cookies)(\s*[:=]\s*)[^\r\n]+").unwrap()
    });
    redacted = re_header
        .replace_all(&redacted, |caps: &regex::Captures| {
            format!("{}{}<redacted>", &caps[1], &caps[2])
        })
        .into_owned();

    static RE_KEYVAL: OnceLock<Regex> = OnceLock::new();
    let re_keyval = RE_KEYVAL.get_or_init(|| {
        Regex::new(r"(?i)(password|passwd|pass|contraseña|token|session_key|sessionkey|sk|api_key|apikey|api_secret|apisecret|client_secret|clientsecret|api_sig)(\s*[:=]\s*)[^\s\r\n,;]+").unwrap()
    });
    redacted = re_keyval
        .replace_all(&redacted, |caps: &regex::Captures| {
            format!("{}{}<redacted>", &caps[1], &caps[2])
        })
        .into_owned();

    static RE_URL: OnceLock<Regex> = OnceLock::new();
    let re_url = RE_URL.get_or_init(|| Regex::new(r"https?://[^\s]+").unwrap());
    redacted = re_url
        .replace_all(&redacted, |caps: &regex::Captures| redact_url(&caps[0]))
        .into_owned();

    redacted
}

pub fn file_contains_any(path: &Path, secrets: &[&str]) -> std::io::Result<bool> {
    let data = std::fs::read(path)?;
    Ok(secrets.iter().any(|secret| {
        !secret.is_empty()
            && data
                .windows(secret.len())
                .any(|window| window == secret.as_bytes())
    }))
}

#[cfg(test)]
mod tests {
    use super::*;
    use std::io::{Read, Write};

    const SECRETS: [&str; 4] = [
        "sentinel-google-secret",
        "sentinel-lastfm-secret",
        "sentinel-session-token",
        "sentinel-youtube-cookie",
    ];

    fn temp_dir() -> std::path::PathBuf {
        let path = std::env::temp_dir().join(format!(
            "doremi-security-audit-{}-{:?}",
            std::process::id(),
            std::thread::current().id()
        ));
        std::fs::create_dir_all(&path).unwrap();
        path
    }

    #[test]
    fn redacts_url_credentials_query_and_fragment() {
        let redacted =
            redact_url("https://user:password@example.com/update.deb?token=secret#fragment");
        assert!(!redacted.contains("password"));
        assert!(!redacted.contains("token="));
        assert!(!redacted.contains("fragment"));
        assert!(redacted.contains("example.com/update.deb"));
    }

    #[test]
    fn test_redact_secrets() {
        let text =
            "Login failed for user test with password=supersecret, sk: 123456, session_key = abcde";
        let redacted = redact_secrets(text);
        assert!(!redacted.contains("supersecret"));
        assert!(!redacted.contains("123456"));
        assert!(!redacted.contains("abcde"));
        assert!(redacted.contains("password=<redacted>"));
        assert!(redacted.contains("sk: <redacted>"));
        assert!(redacted.contains("session_key = <redacted>"));

        let text_sentinel =
            "Found sentinel-google-secret and sentinel-lastfm-secret in the config.";
        let redacted_sentinel = redact_secrets(text_sentinel);
        assert!(!redacted_sentinel.contains("sentinel-google-secret"));
        assert!(!redacted_sentinel.contains("sentinel-lastfm-secret"));
        assert_eq!(
            redacted_sentinel,
            "Found <redacted> and <redacted> in the config."
        );

        let text_url = "Download update from https://user:pass@example.com/file.deb?token=123#frag";
        let redacted_url = redact_secrets(text_url);
        assert!(!redacted_url.contains("pass"));
        assert!(!redacted_url.contains("token="));
        assert!(!redacted_url.contains("frag"));
        assert!(redacted_url.contains("example.com/file.deb"));
    }

    #[test]
    fn persisted_artifacts_do_not_contain_secrets() {
        let dir = temp_dir();

        let mut settings = crate::config::settings::AppSettings::default();
        settings.google_client_secret = SECRETS[0].to_string();
        settings.integrations.lastfm_api_secret = SECRETS[1].to_string();
        settings.integrations.lastfm_session_key = SECRETS[2].to_string();
        let settings_path = dir.join("settings.toml");
        crate::config::settings::write_private_file(
            &settings_path,
            settings.sanitized_toml().unwrap().as_bytes(),
        )
        .unwrap();

        let db_path = dir.join("doremi.db");
        let conn = rusqlite::Connection::open(&db_path).unwrap();
        conn.execute_batch(
            "CREATE TABLE app_data (id INTEGER PRIMARY KEY, value TEXT NOT NULL);
             INSERT INTO app_data (value) VALUES ('ordinary user data');",
        )
        .unwrap();
        drop(conn);

        let log_path = dir.join("doremi.log");
        std::fs::write(&log_path, "authentication failed: credentials redacted\n").unwrap();

        let backup_path = dir.join("backup.zip");
        let file = std::fs::File::create(&backup_path).unwrap();
        let mut zip = zip::ZipWriter::new(file);
        zip.start_file("settings.toml", zip::write::FileOptions::default())
            .unwrap();
        zip.write_all(settings.sanitized_toml().unwrap().as_bytes())
            .unwrap();
        zip.start_file("doremi.db", zip::write::FileOptions::default())
            .unwrap();
        zip.write_all(&std::fs::read(&db_path).unwrap()).unwrap();
        zip.finish().unwrap();

        for path in [&settings_path, &db_path, &log_path, &backup_path] {
            assert!(
                !file_contains_any(path, &SECRETS).unwrap(),
                "{}",
                path.display()
            );
        }

        let mut archive = zip::ZipArchive::new(std::fs::File::open(&backup_path).unwrap()).unwrap();
        for index in 0..archive.len() {
            let mut entry = archive.by_index(index).unwrap();
            let mut content = Vec::new();
            entry.read_to_end(&mut content).unwrap();
            for secret in SECRETS {
                assert!(!content
                    .windows(secret.len())
                    .any(|part| part == secret.as_bytes()));
            }
        }

        let _ = std::fs::remove_dir_all(dir);
    }
}
