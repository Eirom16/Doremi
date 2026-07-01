# Changelog

All notable changes to Doremi will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [2.0.0-beta1] - 2026-07-01

### Added
- Full YouTube Music API integration (search, browse, library, playlists, history)
- VLC-based audio playback with equalizer, crossfade and gapless playback
- Queue management with shuffle, repeat, drag & drop
- Synchronized lyrics from LRCLIB with auto-scroll and seek-by-line
- Download support via yt-dlp with batch album/playlist downloads
- Offline mode with connectivity monitoring and automatic fallback
- MPRIS integration with media keys and system tray
- Last.fm scrobbling
- Discord Rich Presence
- Secure credential storage via Secret Service / KWallet
- Migration tool for Pyrolist users
- Dark and light themes with customizable accent colors
- Spanish and English localization
- Podcast and show support
- Backup and restore functionality
- Statistics and listening history with time range filters
- Mini player and compact mode
- Navigation history (back/forward)
- Single-instance guard with argument forwarding
- Sleep timer
- AppStream metadata and .desktop file for Linux integration

### Security
- Credentials stored in system keyring (never in plaintext files)
- Secret redaction in all log output
- Secure temp files for yt-dlp cookies (0600 permissions, auto-cleanup)
- Updater validates checksums before installation

[2.0.0-beta1]: https://github.com/Eirom16/Doremi/releases/tag/v2.0.0-beta1
