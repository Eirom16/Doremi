/// Format milliseconds to "m:ss" short format (e.g. "3:45")
pub fn format_duration_short(ms: i64) -> String {
    if ms <= 0 {
        return "0:00".to_string();
    }
    let total_secs = ms / 1000;
    let minutes = total_secs / 60;
    let seconds = total_secs % 60;
    format!("{}:{:02}", minutes, seconds)
}

/// Format milliseconds to "h:mm:ss" or "m:ss" long format
pub fn format_duration_long(ms: i64) -> String {
    if ms <= 0 {
        return "0:00".to_string();
    }
    let total_secs = ms / 1000;
    let hours = total_secs / 3600;
    let minutes = (total_secs % 3600) / 60;
    let seconds = total_secs % 60;
    if hours > 0 {
        format!("{}:{:02}:{:02}", hours, minutes, seconds)
    } else {
        format!("{}:{:02}", minutes, seconds)
    }
}

/// Format milliseconds to a human-readable remaining time
pub fn format_remaining(ms: i64) -> String {
    if ms <= 0 {
        return "0:00".to_string();
    }
    let total_secs = ms / 1000;
    let minutes = total_secs / 60;
    let seconds = total_secs % 60;
    format!("-{}:{:02}", minutes, seconds)
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_format_short() {
        assert_eq!(format_duration_short(0), "0:00");
        assert_eq!(format_duration_short(1000), "0:01");
        assert_eq!(format_duration_short(65000), "1:05");
        assert_eq!(format_duration_short(3661000), "61:01");
    }

    #[test]
    fn test_format_long() {
        assert_eq!(format_duration_long(3661000), "1:01:01");
        assert_eq!(format_duration_long(65000), "1:05");
    }
}
