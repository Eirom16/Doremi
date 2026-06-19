fn qt_cflags() -> Vec<String> {
    std::process::Command::new("pkg-config")
        .args(["--cflags", "Qt6Core", "Qt6Widgets", "Qt6Gui", "Qt6WebEngineWidgets", "Qt6WebEngineCore"])
        .output()
        .map(|o| {
            String::from_utf8_lossy(&o.stdout)
                .split_whitespace()
                .map(|s| s.to_string())
                .collect()
        })
        .unwrap_or_default()
}

fn qt_link_libs() {
    let output = std::process::Command::new("pkg-config")
        .args(["--libs", "Qt6Core", "Qt6Widgets", "Qt6Gui", "Qt6WebEngineWidgets", "Qt6WebEngineCore"])
        .output()
        .expect("pkg-config for Qt6 libs");
    let libs = String::from_utf8_lossy(&output.stdout);
    for token in libs.split_whitespace() {
        if let Some(lib) = token.strip_prefix("-l") {
            println!("cargo:rustc-link-lib={lib}");
        } else if let Some(path) = token.strip_prefix("-L") {
            println!("cargo:rustc-link-search={path}");
        }
    }
}

fn qt_moc_headers(build: &mut cc::Build, header: &str, out_dir: &std::path::Path) {
    let moc_path = "/usr/lib/qt6/moc";
    let header_path = std::path::Path::new("src/cpp").join(header);
    let stem = std::path::Path::new(header).file_stem().unwrap().to_str().unwrap();
    let moc_out = out_dir.join(format!("moc_{stem}.cpp"));
    let cxx_include = out_dir.join("cxxbridge").join("include");
    
    // Ensure parent directory exists
    if let Some(parent) = moc_out.parent() {
        std::fs::create_dir_all(parent).unwrap_or_default();
    }
    
    let status = std::process::Command::new(moc_path)
        .arg(&header_path)
        .arg("-o")
        .arg(&moc_out)
        .arg("-I")
        .arg("src/cpp")
        .arg("-I")
        .arg(&cxx_include)
        .status()
        .unwrap_or_else(|e| panic!("moc failed for {}: {}", header, e));
    assert!(status.success(), "moc returned non-zero for {}", header);
    build.file(moc_out);
}

fn main() {
    let qt_flags = qt_cflags();
    qt_link_libs();
    let out_dir = std::path::PathBuf::from(std::env::var("OUT_DIR").expect("OUT_DIR"));

    let mut build = cxx_build::bridge("src/bridge.rs");
    build
        .file("src/cpp/main_window.cpp")
        .file("src/cpp/title_bar.cpp")
        .file("src/cpp/nav_sidebar.cpp")
        .file("src/cpp/player_bar.cpp")
        .file("src/cpp/home_view.cpp")
        .file("src/cpp/search_view.cpp")
        .file("src/cpp/library_view.cpp")
        .file("src/cpp/settings_view.cpp")
        .file("src/cpp/trending_view.cpp")
        .file("src/cpp/downloads_view.cpp")
        .file("src/cpp/widgets.cpp")
        .file("src/cpp/design_tokens.cpp")
        .file("src/cpp/icon_provider.cpp")
        .file("src/cpp/animator.cpp")
        .file("src/cpp/components/ripple_button.cpp")
        .file("src/cpp/components/glass_panel.cpp")
        .file("src/cpp/components/animated_progress.cpp")
        .file("src/cpp/components/skeleton_loader.cpp")
        .file("src/cpp/components/scrolling_label.cpp")
        .file("src/cpp/components/animated_toggle.cpp")
        .file("src/cpp/components/toast_notification.cpp")
        .file("src/cpp/components/icon_button.cpp")
        .file("src/cpp/components/fade_stack.cpp")
        .file("src/cpp/components/song_card.cpp")
        .file("src/cpp/components/artwork_loader.cpp")
        .file("src/cpp/components/album_card.cpp")
        .file("src/cpp/components/artist_card.cpp")
        .file("src/cpp/components/horizontal_carousel.cpp")
        .file("src/cpp/components/waveform_bars.cpp")
        .file("src/cpp/now_playing_view.cpp")
        .file("src/cpp/components/nebula_bg.cpp")
        .file("src/cpp/components/lyrics_widget.cpp")
        .file("src/cpp/components/vinyl_disc.cpp")
        .file("src/cpp/components/queue_panel.cpp")
        .file("src/cpp/components/related_tracks_widget.cpp")
        .file("src/cpp/components/create_playlist_dialog.cpp")
        .file("src/cpp/stats_view.cpp")
        .file("src/cpp/components/stat_card.cpp")
        .file("src/cpp/components/bar_chart.cpp")
        .file("src/cpp/components/theme_transition.cpp")
        .file("src/cpp/history_view.cpp")
        .file("src/cpp/album_detail_view.cpp")
        .file("src/cpp/artist_detail_view.cpp")
        .file("src/cpp/playlist_detail_view.cpp")
        .file("src/cpp/welcome_view.cpp")
        .file("src/cpp/login_dialog.cpp")
        .file("src/cpp/sudo_dialog.cpp")
        .file("src/cpp/update_dialog.cpp")

        .flag_if_supported("-std=c++17")
        .flag_if_supported("-fPIC")
        .include("src/cpp");
    for flag in &qt_flags {
        build.flag(flag);
    }

    // Process all Q_OBJECT headers through Qt6 moc.
    let moc_headers = [
        "main_window.h",
        "title_bar.h",
        "nav_sidebar.h",
        "player_bar.h",
        "home_view.h",
        "search_view.h",
        "library_view.h",
        "settings_view.h",
        "trending_view.h",
        "downloads_view.h",
        "widgets.h",
        "components/ripple_button.h",
        "components/glass_panel.h",
        "components/animated_progress.h",
        "components/skeleton_loader.h",
        "components/scrolling_label.h",
        "components/animated_toggle.h",
        "components/toast_notification.h",
        "components/icon_button.h",
        "components/fade_stack.h",
        "components/song_card.h",
        "components/album_card.h",
        "components/artist_card.h",
        "components/horizontal_carousel.h",
        "components/waveform_bars.h",
        "now_playing_view.h",
        "components/nebula_bg.h",
        "components/lyrics_widget.h",
        "components/vinyl_disc.h",
        "components/queue_panel.h",
        "components/related_tracks_widget.h",
        "components/create_playlist_dialog.h",
        "stats_view.h",
        "components/stat_card.h",
        "components/bar_chart.h",
        "components/theme_transition.h",
        "history_view.h",
        "album_detail_view.h",
        "artist_detail_view.h",
        "playlist_detail_view.h",
        "welcome_view.h",
        "login_dialog.h",
        "sudo_dialog.h",
        "update_dialog.h",
    ];
    for hdr in &moc_headers {
        qt_moc_headers(&mut build, hdr, &out_dir);
    }

    build.compile("doremi-qt");

    println!("cargo:rerun-if-changed=src/bridge.rs");
    println!("cargo:rerun-if-changed=src/cpp/main_window.cpp");
    println!("cargo:rerun-if-changed=src/cpp/main_window.h");
    println!("cargo:rerun-if-changed=src/cpp/ffi_utils.h");
    println!("cargo:rerun-if-changed=src/cpp/title_bar.cpp");
    println!("cargo:rerun-if-changed=src/cpp/title_bar.h");
    println!("cargo:rerun-if-changed=src/cpp/nav_sidebar.cpp");
    println!("cargo:rerun-if-changed=src/cpp/nav_sidebar.h");
    println!("cargo:rerun-if-changed=src/cpp/player_bar.cpp");
    println!("cargo:rerun-if-changed=src/cpp/player_bar.h");
    println!("cargo:rerun-if-changed=src/cpp/home_view.cpp");
    println!("cargo:rerun-if-changed=src/cpp/home_view.h");
    println!("cargo:rerun-if-changed=src/cpp/search_view.cpp");
    println!("cargo:rerun-if-changed=src/cpp/search_view.h");
    println!("cargo:rerun-if-changed=src/cpp/library_view.cpp");
    println!("cargo:rerun-if-changed=src/cpp/library_view.h");
    println!("cargo:rerun-if-changed=src/cpp/settings_view.cpp");
    println!("cargo:rerun-if-changed=src/cpp/settings_view.h");
    println!("cargo:rerun-if-changed=src/cpp/trending_view.cpp");
    println!("cargo:rerun-if-changed=src/cpp/trending_view.h");
    println!("cargo:rerun-if-changed=src/cpp/downloads_view.cpp");
    println!("cargo:rerun-if-changed=src/cpp/downloads_view.h");
    println!("cargo:rerun-if-changed=src/cpp/widgets.cpp");
    println!("cargo:rerun-if-changed=src/cpp/widgets.h");
    println!("cargo:rerun-if-changed=src/cpp/design_tokens.cpp");
    println!("cargo:rerun-if-changed=src/cpp/design_tokens.h");
    println!("cargo:rerun-if-changed=src/cpp/icon_provider.cpp");
    println!("cargo:rerun-if-changed=src/cpp/icon_provider.h");
    println!("cargo:rerun-if-changed=src/cpp/animator.cpp");
    println!("cargo:rerun-if-changed=src/cpp/animator.h");
    println!("cargo:rerun-if-changed=src/cpp/components/ripple_button.cpp");
    println!("cargo:rerun-if-changed=src/cpp/components/ripple_button.h");
    println!("cargo:rerun-if-changed=src/cpp/components/glass_panel.cpp");
    println!("cargo:rerun-if-changed=src/cpp/components/glass_panel.h");
    println!("cargo:rerun-if-changed=src/cpp/components/animated_progress.cpp");
    println!("cargo:rerun-if-changed=src/cpp/components/animated_progress.h");
    println!("cargo:rerun-if-changed=src/cpp/components/skeleton_loader.cpp");
    println!("cargo:rerun-if-changed=src/cpp/components/skeleton_loader.h");
    println!("cargo:rerun-if-changed=src/cpp/components/scrolling_label.cpp");
    println!("cargo:rerun-if-changed=src/cpp/components/scrolling_label.h");
    println!("cargo:rerun-if-changed=src/cpp/components/animated_toggle.cpp");
    println!("cargo:rerun-if-changed=src/cpp/components/animated_toggle.h");
    println!("cargo:rerun-if-changed=src/cpp/components/toast_notification.cpp");
    println!("cargo:rerun-if-changed=src/cpp/components/toast_notification.h");
    println!("cargo:rerun-if-changed=src/cpp/components/icon_button.cpp");
    println!("cargo:rerun-if-changed=src/cpp/components/icon_button.h");
    println!("cargo:rerun-if-changed=src/cpp/components/fade_stack.cpp");
    println!("cargo:rerun-if-changed=src/cpp/components/fade_stack.h");
    println!("cargo:rerun-if-changed=src/cpp/components/song_card.cpp");
    println!("cargo:rerun-if-changed=src/cpp/components/song_card.h");
    println!("cargo:rerun-if-changed=src/cpp/components/artwork_loader.cpp");
    println!("cargo:rerun-if-changed=src/cpp/components/artwork_loader.h");
    println!("cargo:rerun-if-changed=src/cpp/components/album_card.cpp");
    println!("cargo:rerun-if-changed=src/cpp/components/album_card.h");
    println!("cargo:rerun-if-changed=src/cpp/components/artist_card.cpp");
    println!("cargo:rerun-if-changed=src/cpp/components/artist_card.h");
    println!("cargo:rerun-if-changed=src/cpp/components/horizontal_carousel.cpp");
    println!("cargo:rerun-if-changed=src/cpp/components/horizontal_carousel.h");
    println!("cargo:rerun-if-changed=src/cpp/components/waveform_bars.cpp");
    println!("cargo:rerun-if-changed=src/cpp/components/waveform_bars.h");
    println!("cargo:rerun-if-changed=src/cpp/now_playing_view.cpp");
    println!("cargo:rerun-if-changed=src/cpp/now_playing_view.h");
    println!("cargo:rerun-if-changed=src/cpp/components/nebula_bg.cpp");
    println!("cargo:rerun-if-changed=src/cpp/components/nebula_bg.h");
    println!("cargo:rerun-if-changed=src/cpp/components/lyrics_widget.cpp");
    println!("cargo:rerun-if-changed=src/cpp/components/lyrics_widget.h");
    println!("cargo:rerun-if-changed=src/cpp/components/vinyl_disc.cpp");
    println!("cargo:rerun-if-changed=src/cpp/components/vinyl_disc.h");
    println!("cargo:rerun-if-changed=src/cpp/components/queue_panel.cpp");
    println!("cargo:rerun-if-changed=src/cpp/components/queue_panel.h");
    println!("cargo:rerun-if-changed=src/cpp/components/related_tracks_widget.cpp");
    println!("cargo:rerun-if-changed=src/cpp/components/related_tracks_widget.h");
    println!("cargo:rerun-if-changed=src/cpp/components/create_playlist_dialog.cpp");
    println!("cargo:rerun-if-changed=src/cpp/components/create_playlist_dialog.h");
    println!("cargo:rerun-if-changed=src/cpp/stats_view.cpp");
    println!("cargo:rerun-if-changed=src/cpp/stats_view.h");
    println!("cargo:rerun-if-changed=src/cpp/components/stat_card.cpp");
    println!("cargo:rerun-if-changed=src/cpp/components/stat_card.h");
    println!("cargo:rerun-if-changed=src/cpp/components/bar_chart.cpp");
    println!("cargo:rerun-if-changed=src/cpp/components/bar_chart.h");
    println!("cargo:rerun-if-changed=src/cpp/components/theme_transition.cpp");
    println!("cargo:rerun-if-changed=src/cpp/components/theme_transition.h");
    println!("cargo:rerun-if-changed=src/cpp/history_view.cpp");
    println!("cargo:rerun-if-changed=src/cpp/history_view.h");
    println!("cargo:rerun-if-changed=src/cpp/album_detail_view.cpp");
    println!("cargo:rerun-if-changed=src/cpp/album_detail_view.h");
    println!("cargo:rerun-if-changed=src/cpp/artist_detail_view.cpp");
    println!("cargo:rerun-if-changed=src/cpp/artist_detail_view.h");
    println!("cargo:rerun-if-changed=src/cpp/playlist_detail_view.cpp");
    println!("cargo:rerun-if-changed=src/cpp/playlist_detail_view.h");
    println!("cargo:rerun-if-changed=src/cpp/welcome_view.cpp");
    println!("cargo:rerun-if-changed=src/cpp/welcome_view.h");
    println!("cargo:rerun-if-changed=src/cpp/login_dialog.cpp");
    println!("cargo:rerun-if-changed=src/cpp/login_dialog.h");
    println!("cargo:rerun-if-changed=src/cpp/sudo_dialog.cpp");
    println!("cargo:rerun-if-changed=src/cpp/sudo_dialog.h");
    println!("cargo:rerun-if-changed=src/cpp/update_dialog.cpp");
    println!("cargo:rerun-if-changed=src/cpp/update_dialog.h");
}
