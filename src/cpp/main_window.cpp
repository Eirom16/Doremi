#include <QApplication>
#include <QGuiApplication>
#include <QShortcut>
#include <QKeySequence>
#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QStackedWidget>
#include <QLabel>
#include <QCloseEvent>
#include <QMessageBox>
#include <QStyle>
#include <QIcon>
#include <QPixmap>
#include <QPainter>
#include <QPointer>
#include <QPixmapCache>
#include <QScrollArea>
#include <QScrollBar>
#include <QFile>
#include <QCryptographicHash>
#include <QStandardPaths>
#include <QDir>
#include <QHash>
#include <QMutex>
#include <QMutexLocker>
#include <QWebEngineProfile>
#include <QWebEngineCookieStore>
#include <QNetworkCookie>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDateTime>

#include "main_window.h"
#include "shortcut_manager.h"
#include "tray_controller.h"
#include "navigation_controller.h"
#include "session_cookie_manager.h"
#include "theme_controller.h"
#include "title_bar.h"
#include "nav_sidebar.h"
#include "player_bar.h"
#include "home_view.h"
#include "search_view.h"
#include "library_view.h"
#include "settings_view.h"
#include "trending_view.h"
#include "downloads_view.h"
#include "design_tokens.h"
#include "style_manager.h"
#include "now_playing_view.h"
#include "stats_view.h"
#include "history_view.h"
#include "album_detail_view.h"
#include "artist_detail_view.h"
#include "playlist_detail_view.h"
#include "show_detail_view.h"
#include "welcome_view.h"
#include "login_dialog.h"
#include "components/toast_notification.h"
#include "components/offline_banner.h"
#include "components/theme_transition.h"
#include "bridge_helpers.h"
#include "ffi_utils.h"
#include "doremi/src/bridge.rs.h"
#include "components/create_playlist_dialog.h"
#include "icon_provider.h"



DoremiMainWindow *g_main_window = nullptr;

QString user_facing_notification(QString message) {
    const QString lower = message.toLower();
    if (lower.contains("playlist") && lower.contains("schema changed")) {
        return "No se pudo abrir la playlist. El formato de YouTube Music cambió.";
    }
    if (lower.contains("álbum") && (lower.contains("400 bad request") || lower.contains("invalid_argument"))) {
        return "No se pudieron cargar los álbumes de tu biblioteca. YouTube Music rechazó la solicitud.";
    }
    if (lower.contains("schema changed")) {
        return "No se pudo cargar este contenido. El formato de YouTube Music cambió.";
    }
    if (lower.contains("400 bad request") || lower.contains("invalid_argument")) {
        return "No se pudo cargar el contenido. YouTube Music rechazó la solicitud.";
    }
    if (lower.contains("innertube endpoint") || lower.contains("{\n")) {
        return "No se pudo completar la solicitud a YouTube Music.";
    }
    return message;
}

DoremiMainWindow::DoremiMainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    DesignTokens::loadFonts();
    g_main_window = this;
    setWindowTitle("Doremi");
    setMinimumSize(960, 640);
    resize(1300, 820);

    auto *central = new QWidget(this);
    auto *root = new QVBoxLayout(central);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    title_bar_ = new TitleBar(central);
    root->addWidget(title_bar_);

    auto *body = new QHBoxLayout();
    body->setContentsMargins(0, 0, 0, 0);
    body->setSpacing(0);

    nav_sidebar_ = new NavSidebar(central);
    body->addWidget(nav_sidebar_);

    stack_ = new FadeStack(central);

    home_view_ = new HomeView(stack_);
    search_view_ = new SearchView(stack_);
    library_view_ = new LibraryView(stack_);
    settings_view_ = new SettingsView(this);
    trending_view_ = new TrendingView(stack_);
    downloads_view_ = new DownloadsView(stack_);
    stats_view_ = new StatsView(stack_);
    history_view_ = new HistoryView(stack_);
    album_detail_view_ = new AlbumDetailView(stack_);
    artist_detail_view_ = new ArtistDetailView(stack_);
    playlist_detail_view_ = new PlaylistDetailView(stack_);
    show_detail_view_ = new ShowDetailView(stack_);
    welcome_view_ = new WelcomeView(stack_);

    // Index 0
    auto *placeholder = new QLabel("Selecciona una sección", stack_);
    placeholder->setAlignment(Qt::AlignCenter);
    placeholder->setProperty("textRole", "muted");
    placeholder->setStyleSheet("font-size: 16px;");
    
    int idx_placeholder = stack_->addWidget(placeholder);
    int idx_home = stack_->addWidget(home_view_);
    int idx_search = stack_->addWidget(search_view_);
    int idx_library = stack_->addWidget(library_view_);
    int idx_trending = stack_->addWidget(trending_view_);
    int idx_downloads = stack_->addWidget(downloads_view_);
    int idx_stats = stack_->addWidget(stats_view_);
    int idx_history = stack_->addWidget(history_view_);
    int idx_album = stack_->addWidget(album_detail_view_);
    int idx_artist = stack_->addWidget(artist_detail_view_);
    int idx_playlist = stack_->addWidget(playlist_detail_view_);
    int idx_show = stack_->addWidget(show_detail_view_);
    int idx_welcome = stack_->addWidget(welcome_view_);

    Q_ASSERT(idx_placeholder == static_cast<int>(ViewIndex::Placeholder));
    Q_ASSERT(idx_home == static_cast<int>(ViewIndex::Home));
    Q_ASSERT(idx_search == static_cast<int>(ViewIndex::Search));
    Q_ASSERT(idx_library == static_cast<int>(ViewIndex::Library));
    Q_ASSERT(idx_trending == static_cast<int>(ViewIndex::Trending));
    Q_ASSERT(idx_downloads == static_cast<int>(ViewIndex::Downloads));
    Q_ASSERT(idx_stats == static_cast<int>(ViewIndex::Stats));
    Q_ASSERT(idx_history == static_cast<int>(ViewIndex::History));
    Q_ASSERT(idx_album == static_cast<int>(ViewIndex::AlbumDetail));
    Q_ASSERT(idx_artist == static_cast<int>(ViewIndex::ArtistDetail));
    Q_ASSERT(idx_playlist == static_cast<int>(ViewIndex::PlaylistDetail));
    Q_ASSERT(idx_show == static_cast<int>(ViewIndex::ShowDetail));
    Q_ASSERT(idx_welcome == static_cast<int>(ViewIndex::Welcome));

    stack_->setCurrentIndex(static_cast<int>(ViewIndex::Home)); // start at home
    auto *right_container = new QWidget(central);
    auto *right_layout = new QVBoxLayout(right_container);
    right_layout->setContentsMargins(0, 0, 0, 0);
    right_layout->setSpacing(0);

    offline_banner_ = new OfflineBannerWidget(right_container);
    right_layout->addWidget(offline_banner_);

    body_scroll_ = new QScrollArea(right_container);
    body_scroll_->setWidgetResizable(true);
    body_scroll_->setFrameShape(QFrame::NoFrame);
    body_scroll_->setStyleSheet("QScrollArea { background: transparent; border: none; }");
    body_scroll_->setWidget(stack_);
    right_layout->addWidget(body_scroll_, 1);

    body->addWidget(right_container, 1);

    player_shell_ = new QWidget(right_container);
    player_shell_->setAttribute(Qt::WA_StyledBackground, true);
    player_shell_->setProperty("bgRole", "transparent");
    player_shell_layout_ = new QHBoxLayout(player_shell_);
    player_shell_layout_->setContentsMargins(16, 0, 16, 12);
    player_shell_layout_->setSpacing(0);
    player_bar_ = new PlayerBar(player_shell_);
    player_shell_layout_->addWidget(player_bar_);
    right_layout->addWidget(player_shell_);

    fab_playlist_ = new QPushButton(IconProvider::getIcon("add", DesignTokens::current().text_on_accent, 24), "", right_container);
    fab_playlist_->setCursor(Qt::PointingHandCursor);
    fab_playlist_->setFixedSize(56, 56);
    fab_playlist_->setStyleSheet(QString(
        "QPushButton {"
        "  background-color: %1;"
        "  border-radius: 28px;"
        "  border: none;"
        "}"
        "QPushButton:hover {"
        "  background-color: %2;"
        "}"
        "QPushButton:pressed {"
        "  background-color: %3;"
        "}"
    ).arg(DesignTokens::current().accent.name(),
          DesignTokens::current().accent_bright.name(),
          DesignTokens::current().accent_dim.name()));
    fab_playlist_->setToolTip(tr_q("new_playlist"));
    fab_playlist_->hide();

    connect(fab_playlist_, &QPushButton::clicked, this, [this]() {
        CreatePlaylistDialog dlg(this);
        if (dlg.exec() == QDialog::Accepted) {
            on_create_playlist(
                dlg.playlistName().toStdString(),
                dlg.description().toStdString(),
                dlg.privacy().toStdString()
            );
        }
    });

    root->addLayout(body, 1);

    setCentralWidget(central);

    now_playing_view_ = new NowPlayingView(this);
    now_playing_view_->hide();

    theme_transition_ = new ThemeTransitionOverlay(this);
    theme_transition_->hide();

    // Initialize controllers
    navigation_controller_ = new NavigationController(this);
    shortcut_manager_ = new ShortcutManager(this);
    tray_controller_ = new TrayController(this);
    session_cookie_manager_ = new SessionCookieManager(this);
    theme_controller_ = new ThemeController(this);

    session_cookie_manager_->setup_session_cookie_refresh();


    // Window icon
    QPixmap icon_px(64, 64);
    icon_px.fill(Qt::transparent);
    QPainter p(&icon_px);
    p.setRenderHint(QPainter::Antialiasing);
    p.setPen(Qt::NoPen);
    QLinearGradient grad(0, 0, 64, 64);
    const auto &c = DesignTokens::current();
    grad.setColorAt(0.0, c.accent);
    grad.setColorAt(1.0, c.accent_bright);
    p.setBrush(grad);
    p.drawRoundedRect(4, 4, 56, 56, 12, 12);
    p.setPen(QPen(c.text_on_accent, 3));
    p.setBrush(Qt::NoBrush);
    QFont f = p.font();
    f.setPixelSize(28);
    f.setBold(true);
    p.setFont(f);
    p.drawText(QRect(4, 4, 56, 56), Qt::AlignCenter, "D");
    p.end();
    QIcon win_icon(icon_px);
    setWindowIcon(win_icon);
    if (auto *app = qobject_cast<QApplication *>(QApplication::instance()))
        app->setWindowIcon(win_icon);

    Q_INIT_RESOURCE(resources);

    StyleManager::applyTheme();
    update_responsive_layout();
    connect_signals();

    connect(title_bar_, &TitleBar::open_settings_requested, this, [this]() {
        settings_view_->exec();
    });

    player_timer_ = new QTimer(this);
    player_timer_->setInterval(250);
    QObject::connect(player_timer_, &QTimer::timeout, this, []() { on_timer_tick(); });
    player_timer_->start();
}

DoremiMainWindow::~DoremiMainWindow() { g_main_window = nullptr; }

std::string DoremiMainWindow::current_route() const {
    return navigation_controller_ ? navigation_controller_->current_route() : "home";
}

void DoremiMainWindow::navigate_to(const std::string &route) {
    if (navigation_controller_) navigation_controller_->navigate_to(route);
}

void DoremiMainWindow::navigate_back() {
    if (navigation_controller_) navigation_controller_->navigate_back();
}

void DoremiMainWindow::navigate_back_from_detail() {
    if (navigation_controller_) navigation_controller_->navigate_back_from_detail();
}

void DoremiMainWindow::navigate_forward() {
    if (navigation_controller_) navigation_controller_->navigate_forward();
}

// connect_signals() is defined in signal_connections.cpp

bool DoremiMainWindow::ensure_online_action(const QString &action_description) {
    if (is_online_) {
        return true;
    }

    show_notif(
        QString("Sin conexión: no se puede %1. Puedes seguir usando descargas, historial y biblioteca local.")
            .arg(action_description)
            .toStdString(),
        "warning");
    if (offline_banner_) {
        offline_banner_->showBanner();
    }
    return false;
}


void DoremiMainWindow::closeEvent(QCloseEvent *event) {
    if (stop_on_close_) {
        on_app_quit();
        event->accept();
    } else if (tray_controller_ && tray_controller_->isVisible()) {
        hide();
        event->ignore();
    } else {
        emit window_closed();
        event->accept();
    }
}

void DoremiMainWindow::show_notif(const std::string &message, const std::string &kind) {
    const QString rawMessage = QString::fromStdString(message);
    const QString qMessage = user_facing_notification(rawMessage);
    ToastNotification::Type toastType = ToastNotification::Type::Info;
    if (kind == "success") {
        toastType = ToastNotification::Type::Success;
    } else if (kind == "error") {
        toastType = ToastNotification::Type::Error;
    } else if (kind == "warning") {
        toastType = ToastNotification::Type::Warning;
    }

    if (qMessage.startsWith("Reproduciendo:")) {
        ToastNotification::showToast(
            this,
            qMessage,
            toastType,
            "Abrir",
            [this]() {
                if (now_playing_view_) {
                    now_playing_view_->showView();
                }
            });
    } else {
        ToastNotification::showToast(this, qMessage, toastType);
    }

    if (tray_controller_) {
        tray_controller_->showMessage(
            kind == "error" ? "Doremi — Error" : "Doremi",
            qMessage,
            kind == "error" ? QSystemTrayIcon::Critical :
            kind == "warning" ? QSystemTrayIcon::Warning :
                                QSystemTrayIcon::Information,
            4000
        );
    }
}

void DoremiMainWindow::update_player_state(int32_t state, int32_t pos, int32_t dur) {
    player_bar_->set_progress(pos, dur);
    if (now_playing_view_) {
        now_playing_view_->setPlaybackState(state, pos, dur);
    }
}

void DoremiMainWindow::set_mini_player_info(const std::string &title, const std::string &artist,
                                             const std::string &thumb) {
    player_bar_->set_track_info(title, artist, thumb);
    if (now_playing_view_) {
        now_playing_view_->setTrackInfo(title, artist, thumb);
    }
    if (playback_playing_ && !title.empty() && (title != last_track_title_ || artist != last_track_artist_)) {
        last_track_title_ = title;
        last_track_artist_ = artist;
        show_notif("Reproduciendo: " + title + " — " + artist, "info");
    }
}

void DoremiMainWindow::set_playback_playing(bool playing) {
    playback_playing_ = playing;
    player_bar_->set_playing(playing);
    if (now_playing_view_) {
        now_playing_view_->setPlaying(playing);
    }
    if (tray_controller_) {
        tray_controller_->setPlaying(playing);
    }
}

void DoremiMainWindow::resizeEvent(QResizeEvent *event) {
    QMainWindow::resizeEvent(event);
    update_responsive_layout();
    if (now_playing_view_) {
        now_playing_view_->setGeometry(rect());
    }
    if (theme_transition_) {
        theme_transition_->setGeometry(rect());
    }
    
    if (fab_playlist_ && player_shell_) {
        // Find the right_container (parent of fab_playlist_)
        QWidget *rc = fab_playlist_->parentWidget();
        if (rc) {
            int fab_x = rc->width() - fab_playlist_->width() - 24;
            int fab_y = rc->height() - player_shell_->height() - fab_playlist_->height() - 24;
            fab_playlist_->move(fab_x, fab_y);
        }
    }
    
    ToastNotification::repositionActiveToasts();
}

void DoremiMainWindow::update_fab_visibility() {
    if (!fab_playlist_) return;
    if (current_route() == "library" && library_view_ && library_view_->current_tab() == "playlists" && is_online_) {
        fab_playlist_->show();
    } else {
        fab_playlist_->hide();
    }
}

void DoremiMainWindow::update_responsive_layout() {
    if (nav_sidebar_) {
        nav_sidebar_->set_compact(width() < 1120);
    }
    const int sidebar_width = nav_sidebar_ ? nav_sidebar_->width() : 0;
    if (title_bar_) {
        title_bar_->set_sidebar_offset(sidebar_width);
    }
    if (player_shell_layout_) {
        player_shell_layout_->setContentsMargins(16, 0, 16, 12);
    }
    if (player_bar_) {
        player_bar_->set_compact(width() < 1120);
    }
}

void DoremiMainWindow::set_dominant_colors(const std::vector<std::string> &colors) {
    if (now_playing_view_) {
        QStringList qlist;
        for (const auto &col : colors) {
            qlist.append(QString::fromStdString(col));
        }
        now_playing_view_->setDominantColors(qlist);
    }
}

void DoremiMainWindow::set_track_lyrics(const std::string &plain, const std::string &synced) {
    if (now_playing_view_) {
        now_playing_view_->setLyrics(QString::fromStdString(plain), QString::fromStdString(synced));
    }
}

void DoremiMainWindow::set_playback_queue(const std::vector<Track> &queue, int32_t current_index) {
    if (now_playing_view_) {
        now_playing_view_->setQueue(queue, current_index);
    }
    if (player_bar_ && current_index >= 0 && current_index < static_cast<int32_t>(queue.size())) {
        const auto &t = queue[current_index];
        player_bar_->set_track_info(
            static_cast<std::string>(t.title),
            static_cast<std::string>(t.artist),
            static_cast<std::string>(t.thumbnail)
        );
    }
}

void DoremiMainWindow::set_related_tracks(const std::vector<Track> &tracks) {
    if (now_playing_view_) {
        now_playing_view_->setRelatedTracks(tracks);
    }
}

void DoremiMainWindow::set_current_track(const Track &track) {
    if (now_playing_view_) {
        now_playing_view_->setCurrentTrack(track);
    }
}

void DoremiMainWindow::set_stats_data(const StatsData &stats) {
    if (stats_view_) {
        stats_view_->setStatsData(stats);
    }
}

void DoremiMainWindow::set_online_status(bool is_online) {
    is_online_ = is_online;
    if (offline_banner_) {
        if (is_online) {
            offline_banner_->hideBanner();
        } else {
            offline_banner_->showBanner();
        }
    }
}

void DoremiMainWindow::set_history_data(const std::vector<Track> &history, const std::vector<std::string> &played_at, const std::vector<std::string> &feedback_tokens) {
    if (history_view_) {
        history_view_->set_history(history, played_at, feedback_tokens);
    }
}






void DoremiMainWindow::set_context_playlists(const std::vector<Playlist> &playlists) {
    context_playlists_ = playlists;
}

void DoremiMainWindow::setup_ui_test(const std::string &view, const std::string &screenshot_path) {
    resize(1300, 820);
    setFixedSize(1300, 820);

    if (view == "now-playing" || view == "now_playing") {
        navigate_to("home");
        if (now_playing_view_) {
            now_playing_view_->showView();
        }
    } else {
        navigate_to(view);
    }

    QTimer::singleShot(3500, this, [this, screenshot_path]() {
        QPixmap pixmap = grab();
        if (pixmap.save(QString::fromStdString(screenshot_path))) {
            qDebug() << "UI Test screenshot successfully saved to:" << QString::fromStdString(screenshot_path);
        } else {
            qWarning() << "UI Test screenshot FAILED to save to:" << QString::fromStdString(screenshot_path);
        }
        QApplication::quit();
    });
}
