#include <QApplication>
#include <QGuiApplication>
#include <QShortcut>
#include <QKeySequence>
#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
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
    settings_view_ = new SettingsView(stack_);
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
    int idx_settings = stack_->addWidget(settings_view_);
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
    Q_ASSERT(idx_settings == static_cast<int>(ViewIndex::Settings));
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

    StyleManager::applyTheme();
    update_responsive_layout();
    connect_signals();

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

void DoremiMainWindow::connect_signals() {
    QObject::connect(this, &DoremiMainWindow::play_pause_triggered, this, []() { on_play_pause_triggered(); });
    QObject::connect(this, &DoremiMainWindow::next_triggered, this, []() { on_next_triggered(); });
    QObject::connect(this, &DoremiMainWindow::previous_triggered, this, []() { on_previous_triggered(); });
    QObject::connect(this, &DoremiMainWindow::volume_set, this, [](int32_t v) { on_volume_set(v); });
    QObject::connect(this, &DoremiMainWindow::window_closed, this, []() { on_window_close_requested(); });

    guardOnline(title_bar_, &TitleBar::search_submitted, "buscar en YouTube Music", [this](const std::string &q) {
        search_view_->set_query(q);
        stack_->setCurrentIndex(2);
        on_search_submitted(q, "all");
    });
    QObject::connect(title_bar_, &TitleBar::search_text_changed, this, [this](const std::string &q) {
        if (!is_online_) return;
        on_search_suggestions_requested(q);
    });


    QObject::connect(nav_sidebar_, &NavSidebar::route_changed, this, [this](const std::string &r) {
        navigate_to(r);
        if (r == "search") on_search_history_requested();
    });

    QObject::connect(player_bar_, &PlayerBar::play_pause_clicked, this, [this]() { emit play_pause_triggered(); });
    QObject::connect(player_bar_, &PlayerBar::next_clicked, this, [this]() { emit next_triggered(); });
    QObject::connect(player_bar_, &PlayerBar::previous_clicked, this, [this]() { emit previous_triggered(); });
    QObject::connect(player_bar_, &PlayerBar::volume_changed, this, [this](int32_t d) { emit volume_changed(d); });
    QObject::connect(player_bar_, &PlayerBar::volume_set, this, [this](int32_t v) { emit volume_set(v); });
    QObject::connect(player_bar_, &PlayerBar::seek_requested, this, [](int32_t pos_ms) {
        on_seek_absolute(pos_ms);
    });
    QObject::connect(player_bar_, &PlayerBar::shuffle_toggled, this,
        [this](bool on) { emit shuffle_toggled(on); });
    QObject::connect(player_bar_, &PlayerBar::repeat_cycled, this,
        [this]() { emit repeat_cycled(); });

    QObject::connect(player_bar_, &PlayerBar::left_section_clicked, this, [this]() {
        if (now_playing_view_) {
            now_playing_view_->showView();
        }
    });

    QObject::connect(now_playing_view_, &NowPlayingView::play_pause_clicked, this, [this]() { emit play_pause_triggered(); });
    QObject::connect(now_playing_view_, &NowPlayingView::next_clicked, this, [this]() { emit next_triggered(); });
    QObject::connect(now_playing_view_, &NowPlayingView::previous_clicked, this, [this]() { emit previous_triggered(); });
    QObject::connect(now_playing_view_, &NowPlayingView::seek_requested, this, [](int32_t pos_ms) {
        on_seek_absolute(pos_ms);
    });
    QObject::connect(now_playing_view_, &NowPlayingView::shuffle_toggled, this,
        [this](bool on) { emit shuffle_toggled(on); });
    QObject::connect(now_playing_view_, &NowPlayingView::repeat_cycled, this,
        [this]() { emit repeat_cycled(); });
    QObject::connect(now_playing_view_, &NowPlayingView::related_play_requested, this,
        [](Track track) { on_search_item_clicked(track); });
    QObject::connect(now_playing_view_, &NowPlayingView::related_add_to_queue_requested, this,
        [](Track track) { on_add_to_queue_end(track); });
    guardOnline(now_playing_view_, &NowPlayingView::download_clicked, "descargar canciones", [this](Track track) {
        on_download_requested(track);
    });

    QObject::connect(this, &DoremiMainWindow::shuffle_toggled, this,
        [](bool on) { on_shuffle_toggled(on); });
    QObject::connect(this, &DoremiMainWindow::repeat_cycled, this,
        []() { on_repeat_cycled(); });

    QObject::connect(settings_view_, &SettingsView::setting_changed, this,
        [](const std::string &key, const std::string &value) {
            on_setting_changed(key, value);
        });

    guardOnline(settings_view_, &SettingsView::lastfm_auth_requested, "conectar Last.fm", [this](const std::string &apiKey, const std::string &apiSecret, const std::string &username, const std::string &password) {
        on_lastfm_auth_requested(apiKey, apiSecret, username, password);
    });

    QObject::connect(settings_view_, &SettingsView::lastfm_disconnect_requested, this,
        []() {
            on_lastfm_disconnect_requested();
        });

    QObject::connect(search_view_, &SearchView::play_requested, this,
        [](Track track) {
            on_search_item_clicked(track);
        });
    guardOnline(search_view_, &SearchView::search_requested, "buscar en YouTube Music", [this](const std::string &query, const std::string &filter) {
        title_bar_->set_search_text(query);
        search_view_->set_query(query);
        on_search_submitted(query, filter);
    });
    guardOnline(search_view_, &SearchView::album_requested, "abrir detalles de álbum", [this](const std::string &browse_id) {
        on_album_requested(browse_id);
    });
    guardOnline(search_view_, &SearchView::artist_requested, "abrir detalles de artista", [this](const std::string &browse_id) {
        on_artist_requested(browse_id);
    });
    guardOnline(search_view_, &SearchView::playlist_requested, "abrir detalles de playlist", [this](const std::string &playlist_id) {
        on_playlist_requested(playlist_id);
    });
    guardOnline(search_view_, &SearchView::show_requested, "abrir detalles de podcast", [this](const std::string &browse_id) {
        on_show_requested(browse_id);
    });
    QObject::connect(search_view_, &SearchView::add_favorite_requested, this,
        [](Track track) {
            on_add_favorite(track);
        });
    guardOnline(search_view_, &SearchView::download_requested, "descargar canciones", [this](Track track) {
        on_download_requested(track);
    });
    QObject::connect(search_view_, &SearchView::add_to_queue_next_requested, this,
        [](Track track) { on_add_to_queue_next(track); });
    QObject::connect(search_view_, &SearchView::add_to_queue_end_requested, this,
        [](Track track) { on_add_to_queue_end(track); });
    QObject::connect(search_view_, &SearchView::search_history_delete_requested, this,
        [](const std::string &query) { on_search_history_delete(query); });
    QObject::connect(search_view_, &SearchView::search_history_clear_requested, this,
        []() { on_search_history_clear(); });

    QObject::connect(home_view_, &HomeView::play_requested, this,
        [](Track track) { on_search_item_clicked(track); });
    guardOnline(home_view_, &HomeView::album_requested, "abrir detalles de álbum", [this](const std::string &id) {
        on_album_requested(id);
    });
    guardOnline(home_view_, &HomeView::artist_requested, "abrir detalles de artista", [this](const std::string &id) {
        on_artist_requested(id);
    });
    guardOnline(home_view_, &HomeView::playlist_requested, "abrir detalles de playlist", [this](const std::string &id, const std::string &title, const std::string &subtitle, const std::string &thumbnail) {
        on_playlist_requested_with_context(id, title, subtitle, thumbnail);
    });
    guardOnline(home_view_, &HomeView::show_requested, "abrir detalles de podcast", [this](const std::string &id) {
        on_show_requested(id);
    });
    guardOnline(home_view_, &HomeView::retry_requested, "recargar inicio", [this]() {
        on_home_retry_requested();
    });
    guardOnline(home_view_, &HomeView::load_more_requested, "cargar más contenido", [this]() {
        on_home_load_more_requested();
    });

    QObject::connect(trending_view_, &TrendingView::play_requested, this,
        [](Track track) { on_search_item_clicked(track); });
    guardOnline(trending_view_, &TrendingView::album_requested, "abrir detalles de álbum", [this](const std::string &id) {
        on_album_requested(id);
    });
    guardOnline(trending_view_, &TrendingView::artist_requested, "abrir detalles de artista", [this](const std::string &id) {
        on_artist_requested(id);
    });
    guardOnline(trending_view_, &TrendingView::playlist_requested, "abrir detalles de playlist", [this](const std::string &id) {
        on_playlist_requested(id);
    });
    guardOnline(trending_view_, &TrendingView::retry_requested, "recargar tendencias", [this]() {
        on_trending_retry_requested();
    });

    QObject::connect(library_view_, &LibraryView::tab_changed, this,
        [](const std::string &tab) {
            on_library_tab_changed(tab);
        });
    QObject::connect(library_view_, &LibraryView::search_requested, this,
        [](const std::string &tab, const std::string &query, const std::string &sort_by) {
            on_library_search(tab, query, sort_by);
        });
    QObject::connect(library_view_, &LibraryView::filter_source_changed, this,
        [](int source) {
            on_library_set_filter_source(source);
        });
    QObject::connect(library_view_, &LibraryView::play_requested, this,
        [](Track track) {
            on_search_item_clicked(track);
        });
    QObject::connect(library_view_, &LibraryView::remove_favorite_requested, this,
        [](const std::string &info) {
            on_remove_favorite(info);
        });
    QObject::connect(library_view_, &LibraryView::remove_favorite_album_requested, this,
        [](const std::string &id) { on_remove_favorite_album(id); });
    QObject::connect(library_view_, &LibraryView::remove_favorite_artist_requested, this,
        [](const std::string &id) { on_remove_favorite_artist(id); });
    QObject::connect(library_view_, &LibraryView::remove_favorite_show_requested, this,
        [](const std::string &id) { on_remove_favorite_show(id); });
    guardOnline(library_view_, &LibraryView::download_requested, "descargar canciones", [this](Track track) {
        on_download_requested(track);
    });
    QObject::connect(library_view_, &LibraryView::add_to_queue_next_requested, this,
        [](Track track) { on_add_to_queue_next(track); });
    QObject::connect(library_view_, &LibraryView::add_to_queue_end_requested, this,
        [](Track track) { on_add_to_queue_end(track); });
    QObject::connect(library_view_, &LibraryView::show_requested, this,
        [](const std::string &id) {
            on_show_requested(id);
        });
    QObject::connect(library_view_, &LibraryView::playlist_requested, this,
        [](const std::string &id) {
            on_playlist_requested(id);
        });
    QObject::connect(library_view_, &LibraryView::album_requested, this,
        [](const std::string &id) {
            on_album_requested(id);
        });
    QObject::connect(library_view_, &LibraryView::artist_requested, this,
        [](const std::string &id) {
            on_artist_requested(id);
        });
    QObject::connect(library_view_, &LibraryView::create_playlist_requested, this,
        [](const std::string &name, const std::string &desc, const std::string &privacy) {
            on_create_playlist(name, desc, privacy);
        });
    QObject::connect(library_view_, &LibraryView::login_requested, this, [this]() {
        auto *dialog = new WebLoginDialog(this);
        dialog->exec();
        dialog->deleteLater();
    });

    QObject::connect(downloads_view_, &DownloadsView::play_requested, this,
        [](Track track) {
            on_search_item_clicked(track);
        });

    QObject::connect(stats_view_, &StatsView::play_requested, this,
        [](Track track) {
            on_search_item_clicked(track);
        });

    // History view
    QObject::connect(history_view_, &HistoryView::play_requested, this,
        [](Track track) {
            on_search_item_clicked(track);
        });

    // Album detail view
    QObject::connect(album_detail_view_, &AlbumDetailView::play_requested, this,
        [](Track track) {
            on_search_item_clicked(track);
        });
    QObject::connect(album_detail_view_, &AlbumDetailView::back_requested, this,
        [this]() { navigate_back_from_detail(); });
    guardOnline(album_detail_view_, &AlbumDetailView::artist_requested, "abrir detalles de artista", [this](const std::string &artist_id) {
        on_artist_requested(artist_id);
    });

    // Artist detail view
    QObject::connect(artist_detail_view_, &ArtistDetailView::play_requested, this,
        [](Track track) {
            on_search_item_clicked(track);
        });
    guardOnline(artist_detail_view_, &ArtistDetailView::album_requested, "abrir detalles de álbum", [this](const std::string &album_id) {
        on_album_requested(album_id);
    });
    QObject::connect(artist_detail_view_, &ArtistDetailView::back_requested, this,
        [this]() { navigate_back_from_detail(); });

    // Playlist detail view
    QObject::connect(playlist_detail_view_, &PlaylistDetailView::play_requested, this,
        [](Track track) {
            on_search_item_clicked(track);
        });
    QObject::connect(playlist_detail_view_, &PlaylistDetailView::back_requested, this,
        [this]() { navigate_back_from_detail(); });

    // Show detail view
    QObject::connect(show_detail_view_, &ShowDetailView::back_requested, this,
        [this]() { navigate_back_from_detail(); });
    QObject::connect(show_detail_view_, &ShowDetailView::play_episode_requested, this,
        [](Episode ep) {
            Track t;
            t.id = ep.id;
            t.title = ep.title;
            t.artist = ep.show;
            t.album = "";
            t.duration_ms = ep.duration_ms;
            t.thumbnail = ep.thumbnail;
            on_search_item_clicked(t);
        });

    // Play all / shuffle buttons on detail views
    QObject::connect(album_detail_view_, &AlbumDetailView::play_all_requested, this,
        [](std::vector<Track> tracks) {
            rust::Vec<Track> v;
            for (const auto &t : tracks) v.push_back(t);
            on_play_all(v, false);
        });
    guardOnline(album_detail_view_, &AlbumDetailView::download_all_requested, "descargar álbumes", [this](std::vector<Track> tracks, std::string parent_id, std::string parent_title, std::string parent_thumbnail) {
        rust::Vec<Track> v;
        for (const auto &t : tracks) v.push_back(t);
        on_batch_download_requested(v, parent_id, parent_title, parent_thumbnail);
    });
    QObject::connect(playlist_detail_view_, &PlaylistDetailView::play_all_requested, this,
        [](std::vector<Track> tracks) {
            rust::Vec<Track> v;
            for (const auto &t : tracks) v.push_back(t);
            on_play_all(v, false);
        });
    guardOnline(playlist_detail_view_, &PlaylistDetailView::download_all_requested, "descargar playlists", [this](std::vector<Track> tracks, std::string parent_id, std::string parent_title, std::string parent_thumbnail) {
        rust::Vec<Track> v;
        for (const auto &t : tracks) v.push_back(t);
        on_batch_download_requested(v, parent_id, parent_title, parent_thumbnail);
    });
    QObject::connect(playlist_detail_view_, &PlaylistDetailView::shuffle_requested, this,
        [](std::vector<Track> tracks) {
            rust::Vec<Track> v;
            for (const auto &t : tracks) v.push_back(t);
            on_play_all(v, true);
        });
    QObject::connect(playlist_detail_view_, &PlaylistDetailView::rename_playlist_requested, this,
        [](const std::string &id, const std::string &name) {
            on_rename_playlist(id, name);
            // Refresh library list in background
            on_library_tab_changed("playlists");
        });
    QObject::connect(playlist_detail_view_, &PlaylistDetailView::delete_playlist_requested, this,
        [this](const std::string &id) {
            on_delete_playlist(id);
            navigate_back_from_detail();
        });
    QObject::connect(playlist_detail_view_, &PlaylistDetailView::remove_track_from_playlist_requested, this,
        [](const std::string &pl_id, const std::string &t_id) {
            on_remove_playlist_track(pl_id, t_id);
    });
    QObject::connect(playlist_detail_view_, &PlaylistDetailView::track_moved, this,
        [](const std::string &pl_id, int from, int to) {
            on_move_playlist_track(pl_id, from, to);
    });
}

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
    ToastNotification::repositionActiveToasts();
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
