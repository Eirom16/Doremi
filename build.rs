fn find_qt_binary(bin_name: &str) -> String {
    // 1. Check environment variable (e.g. QT_MOC or QT_RCC)
    let env_var_name = format!("QT_{}", bin_name.to_uppercase());
    if let Ok(path) = std::env::var(&env_var_name) {
        return path;
    }

    // 2. Prioritize standard Qt6 paths
    let qt6_path = if bin_name == "moc" {
        "/usr/lib/qt6/moc"
    } else {
        "/usr/bin/rcc"
    };
    if std::path::Path::new(qt6_path).exists() {
        return qt6_path.to_string();
    }

    // 3. Query qtpaths or qtpaths6
    for qtpaths_cmd in &["qtpaths6", "qtpaths"] {
        if let Ok(output) = std::process::Command::new(qtpaths_cmd)
            .arg("--binaries-dir")
            .output()
        {
            if output.status.success() {
                let bin_dir = String::from_utf8_lossy(&output.stdout).trim().to_string();
                let path = std::path::Path::new(&bin_dir).join(bin_name);
                if path.exists() {
                    return path.to_string_lossy().to_string();
                }
            }
        }
    }

    // 4. Check if in PATH
    if let Ok(output) = std::process::Command::new("which").arg(bin_name).output() {
        if output.status.success() {
            let path = String::from_utf8_lossy(&output.stdout).trim().to_string();
            if !path.is_empty() {
                return path;
            }
        }
    }

    // 5. Default fallback paths
    let fallback = if bin_name == "moc" {
        "/usr/lib/qt6/moc"
    } else {
        "/usr/bin/rcc"
    };
    fallback.to_string()
}

fn qt_cflags() -> Vec<String> {
    std::process::Command::new("pkg-config")
        .args([
            "--cflags",
            "Qt6Core",
            "Qt6Widgets",
            "Qt6Gui",
            "Qt6WebEngineWidgets",
            "Qt6WebEngineCore",
        ])
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
        .args([
            "--libs",
            "Qt6Core",
            "Qt6Widgets",
            "Qt6Gui",
            "Qt6WebEngineWidgets",
            "Qt6WebEngineCore",
        ])
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
    let moc_path = find_qt_binary("moc");
    let header_path = std::path::Path::new("src/cpp").join(header);
    let stem = std::path::Path::new(header)
        .file_stem()
        .unwrap()
        .to_str()
        .unwrap();
    let moc_out = out_dir.join(format!("moc_{stem}.cpp"));
    let cxx_include = out_dir.join("cxxbridge").join("include");

    // Ensure parent directory exists
    if let Some(parent) = moc_out.parent() {
        std::fs::create_dir_all(parent).unwrap_or_default();
    }

    let status = std::process::Command::new(&moc_path)
        .arg(&header_path)
        .arg("-o")
        .arg(&moc_out)
        .arg("-I")
        .arg("src/cpp")
        .arg("-I")
        .arg(&cxx_include)
        .status()
        .unwrap_or_else(|e| panic!("moc failed ({}) for {}: {}", moc_path, header, e));
    assert!(status.success(), "moc returned non-zero for {}", header);
    build.file(moc_out);
}

fn qt_resources(build: &mut cc::Build, qrc: &str, out_dir: &std::path::Path) {
    let rcc_path = find_qt_binary("rcc");
    let qrc_path = std::path::Path::new(qrc);
    let stem = qrc_path.file_stem().unwrap().to_str().unwrap();
    let rcc_out = out_dir.join(format!("qrc_{stem}.cpp"));

    let status = std::process::Command::new(&rcc_path)
        .arg(qrc_path)
        .arg("-o")
        .arg(&rcc_out)
        .status()
        .unwrap_or_else(|e| panic!("rcc failed ({}) for {}: {}", rcc_path, qrc, e));
    assert!(status.success(), "rcc returned non-zero for {}", qrc);
    build.file(rcc_out);
}

fn rerun_if_changed_dir(dir: &std::path::Path) {
    if let Ok(entries) = std::fs::read_dir(dir) {
        for entry in entries.flatten() {
            let path = entry.path();
            if path.is_dir() {
                rerun_if_changed_dir(&path);
            } else {
                let ext = path.extension().and_then(|s| s.to_str()).unwrap_or("");
                if ext == "cpp" || ext == "h" {
                    println!("cargo:rerun-if-changed={}", path.to_string_lossy());
                }
            }
        }
    }
}

fn main() {
    let qt_flags = qt_cflags();
    qt_link_libs();
    let out_dir = std::path::PathBuf::from(std::env::var("OUT_DIR").expect("OUT_DIR"));

    let mut build = cxx_build::bridge("src/bridge.rs");

    let cpp_sources = [
        "main_window.cpp",
        "title_bar.cpp",
        "nav_sidebar.cpp",
        "player_bar.cpp",
        "home_view.cpp",
        "search_view.cpp",
        "library_view.cpp",
        "settings_view.cpp",
        "trending_view.cpp",
        "downloads_view.cpp",
        "widgets.cpp",
        "design_tokens.cpp",
        "icon_provider.cpp",
        "animator.cpp",
        "components/ripple_button.cpp",
        "components/glass_panel.cpp",
        "components/animated_progress.cpp",
        "components/skeleton_loader.cpp",
        "components/scrolling_label.cpp",
        "components/animated_toggle.cpp",
        "components/toast_notification.cpp",
        "components/offline_banner.cpp",
        "components/icon_button.cpp",
        "components/fade_stack.cpp",
        "components/song_card.cpp",
        "components/artwork_loader.cpp",
        "components/artwork_backdrop.cpp",
        "components/album_card.cpp",
        "components/artist_card.cpp",
        "components/horizontal_carousel.cpp",
        "components/waveform_bars.cpp",
        "now_playing_view.cpp",
        "components/nebula_bg.cpp",
        "components/lyrics_widget.cpp",
        "components/vinyl_disc.cpp",
        "components/queue_panel.cpp",
        "components/related_tracks_widget.cpp",
        "components/create_playlist_dialog.cpp",
        "stats_view.cpp",
        "components/stat_card.cpp",
        "components/bar_chart.cpp",
        "components/theme_transition.cpp",
        "history_view.cpp",
        "album_detail_view.cpp",
        "artist_detail_view.cpp",
        "playlist_detail_view.cpp",
        "show_detail_view.cpp",
        "welcome_view.cpp",
        "login_dialog.cpp",
        "sudo_dialog.cpp",
        "update_dialog.cpp",
        // Controllers extracted from DoremiMainWindow
        "shortcut_manager.cpp",
        "tray_controller.cpp",
        "navigation_controller.cpp",
        "session_cookie_manager.cpp",
        "theme_controller.cpp",
    ];

    for src in &cpp_sources {
        build.file(format!("src/cpp/{src}"));
    }

    build
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
        "components/offline_banner.h",
        "components/icon_button.h",
        "components/fade_stack.h",
        "components/song_card.h",
        "components/artwork_backdrop.h",
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
        "show_detail_view.h",
        "welcome_view.h",
        "login_dialog.h",
        "sudo_dialog.h",
        "update_dialog.h",
        // Controllers extracted from DoremiMainWindow
        "shortcut_manager.h",
        "tray_controller.h",
        "navigation_controller.h",
        "session_cookie_manager.h",
        "theme_controller.h",
    ];

    for hdr in &moc_headers {
        qt_moc_headers(&mut build, hdr, &out_dir);
    }

    qt_resources(&mut build, "assets/resources.qrc", &out_dir);

    build.compile("doremi-qt");

    println!("cargo:rerun-if-changed=assets/resources.qrc");
    println!("cargo:rerun-if-changed=assets/fonts/MaterialSymbolsRounded.ttf");
    println!("cargo:rerun-if-changed=src/bridge.rs");

    // Automatically watch all source files in src/cpp
    rerun_if_changed_dir(std::path::Path::new("src/cpp"));
}
