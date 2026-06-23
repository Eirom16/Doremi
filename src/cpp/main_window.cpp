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
#include "ffi_utils.h"
#include "doremi/src/bridge.rs.h"



// Non-owning global pointer pointing to the active DoremiMainWindow instance.
// The window's lifetime is managed by the application event loop, but we ensure
// any existing instance is deleted upon re-initialization to prevent memory leaks.
DoremiMainWindow *g_main_window = nullptr;

namespace {
template <typename Callback>
void mutate_main_window(const char *operation, Callback callback) {
    Ffi::on_gui(operation, [callback = std::move(callback)]() mutable {
        if (g_main_window) {
            callback(*g_main_window);
        }
    });
}

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
    placeholder->setStyleSheet("color: #6A6A8A; font-size: 16px;");
    
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
    body_scroll_->setStyleSheet(DesignTokens::scrollAreaStyle());
    body_scroll_->setWidget(stack_);
    right_layout->addWidget(body_scroll_, 1);

    body->addWidget(right_container, 1);

    player_shell_ = new QWidget(right_container);
    player_shell_->setAttribute(Qt::WA_StyledBackground, true);
    player_shell_->setStyleSheet("background: transparent;");
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
    grad.setColorAt(0.0, QColor("#7C4DFF"));
    grad.setColorAt(1.0, QColor("#448AFF"));
    p.setBrush(grad);
    p.drawRoundedRect(4, 4, 56, 56, 12, 12);
    p.setPen(QPen(QColor("#FFFFFF"), 3));
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

    setStyleSheet(DesignTokens::getGlobalStyleSheet());
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

    QObject::connect(title_bar_, &TitleBar::search_submitted, this, [this](const std::string &q) {
        if (!ensure_online_action("buscar en YouTube Music")) return;
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
    QObject::connect(now_playing_view_, &NowPlayingView::download_clicked, this,
        [this](Track track) {
            if (!ensure_online_action("descargar canciones")) return;
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

    QObject::connect(settings_view_, &SettingsView::lastfm_auth_requested, this,
        [this](const std::string &apiKey, const std::string &apiSecret, const std::string &username, const std::string &password) {
            if (!ensure_online_action("conectar Last.fm")) return;
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
    QObject::connect(search_view_, &SearchView::search_requested, this,
        [this](const std::string &query, const std::string &filter) {
            if (!ensure_online_action("buscar en YouTube Music")) return;
            title_bar_->set_search_text(query);
            search_view_->set_query(query);
            on_search_submitted(query, filter);
        });
    QObject::connect(search_view_, &SearchView::album_requested, this,
        [this](const std::string &browse_id) {
            if (!ensure_online_action("abrir detalles de álbum")) return;
            on_album_requested(browse_id);
        });
    QObject::connect(search_view_, &SearchView::artist_requested, this,
        [this](const std::string &browse_id) {
            if (!ensure_online_action("abrir detalles de artista")) return;
            on_artist_requested(browse_id);
        });
    QObject::connect(search_view_, &SearchView::playlist_requested, this,
        [this](const std::string &playlist_id) {
            if (!ensure_online_action("abrir detalles de playlist")) return;
            on_playlist_requested(playlist_id);
        });
    QObject::connect(search_view_, &SearchView::show_requested, this,
        [this](const std::string &browse_id) {
            if (!ensure_online_action("abrir detalles de podcast")) return;
            on_show_requested(browse_id);
        });
    QObject::connect(search_view_, &SearchView::add_favorite_requested, this,
        [](Track track) {
            on_add_favorite(track);
        });
    QObject::connect(search_view_, &SearchView::download_requested, this,
        [this](Track track) {
            if (!ensure_online_action("descargar canciones")) return;
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
    QObject::connect(home_view_, &HomeView::album_requested, this,
        [this](const std::string &id) {
            if (!ensure_online_action("abrir detalles de álbum")) return;
            on_album_requested(id);
        });
    QObject::connect(home_view_, &HomeView::artist_requested, this,
        [this](const std::string &id) {
            if (!ensure_online_action("abrir detalles de artista")) return;
            on_artist_requested(id);
        });
    QObject::connect(home_view_, &HomeView::playlist_requested, this,
        [this](const std::string &id,
               const std::string &title,
               const std::string &subtitle,
               const std::string &thumbnail) {
            if (!ensure_online_action("abrir detalles de playlist")) return;
            on_playlist_requested_with_context(id, title, subtitle, thumbnail);
        });
    QObject::connect(home_view_, &HomeView::show_requested, this,
        [this](const std::string &id) {
            if (!ensure_online_action("abrir detalles de podcast")) return;
            on_show_requested(id);
        });
    QObject::connect(home_view_, &HomeView::retry_requested, this,
        [this]() {
            if (!ensure_online_action("recargar inicio")) return;
            on_home_retry_requested();
        });
    QObject::connect(home_view_, &HomeView::load_more_requested, this,
        [this]() {
            if (!ensure_online_action("cargar más contenido")) return;
            on_home_load_more_requested();
        });

    QObject::connect(trending_view_, &TrendingView::play_requested, this,
        [](Track track) { on_search_item_clicked(track); });
    QObject::connect(trending_view_, &TrendingView::album_requested, this,
        [this](const std::string &id) {
            if (!ensure_online_action("abrir detalles de álbum")) return;
            on_album_requested(id);
        });
    QObject::connect(trending_view_, &TrendingView::artist_requested, this,
        [this](const std::string &id) {
            if (!ensure_online_action("abrir detalles de artista")) return;
            on_artist_requested(id);
        });
    QObject::connect(trending_view_, &TrendingView::playlist_requested, this,
        [this](const std::string &id) {
            if (!ensure_online_action("abrir detalles de playlist")) return;
            on_playlist_requested(id);
        });
    QObject::connect(trending_view_, &TrendingView::retry_requested, this,
        [this]() {
            if (!ensure_online_action("recargar tendencias")) return;
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
    QObject::connect(library_view_, &LibraryView::download_requested, this,
        [this](Track track) {
            if (!ensure_online_action("descargar canciones")) return;
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
    QObject::connect(album_detail_view_, &AlbumDetailView::artist_requested, this,
        [this](const std::string &artist_id) {
            if (!ensure_online_action("abrir detalles de artista")) return;
            on_artist_requested(artist_id);
        });

    // Artist detail view
    QObject::connect(artist_detail_view_, &ArtistDetailView::play_requested, this,
        [](Track track) {
            on_search_item_clicked(track);
        });
    QObject::connect(artist_detail_view_, &ArtistDetailView::album_requested, this,
        [this](const std::string &album_id) {
            if (!ensure_online_action("abrir detalles de álbum")) return;
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
    QObject::connect(album_detail_view_, &AlbumDetailView::download_all_requested, this,
        [this](std::vector<Track> tracks, std::string parent_id, std::string parent_title, std::string parent_thumbnail) {
            if (!ensure_online_action("descargar álbumes")) return;
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
    QObject::connect(playlist_detail_view_, &PlaylistDetailView::download_all_requested, this,
        [this](std::vector<Track> tracks, std::string parent_id, std::string parent_title, std::string parent_thumbnail) {
            if (!ensure_online_action("descargar playlists")) return;
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




// ── Placeholder thumbnail generation ──────────────────────

rust::String get_or_create_thumbnail(rust::Str title, int32_t variant) {
    static QMutex placeholder_cache_mutex;
    static QHash<QString, std::string> placeholder_cache;

    const std::string title_copy = Ffi::to_std_string(title);
    const QByteArray hash = QCryptographicHash::hash(
        QByteArray::fromStdString(title_copy + std::to_string(variant)),
        QCryptographicHash::Md5);
    const QString thumb_dir = QStandardPaths::writableLocation(QStandardPaths::CacheLocation) + "/artwork";
    const QString filename = QString("placeholder_%1_%2.png")
        .arg(QString(hash.toHex().left(12)))
        .arg(variant);
    const QString filepath_q = thumb_dir + "/" + filename;

    {
        QMutexLocker locker(&placeholder_cache_mutex);
        auto it = placeholder_cache.constFind(filepath_q);
        if (it != placeholder_cache.constEnd() && QFile::exists(filepath_q)) {
            return rust::String(it.value());
        }
    }

    QDir().mkpath(thumb_dir);
    if (QFile::exists(filepath_q)) {
        const std::string cached_path = Ffi::to_std_string(filepath_q);
        QMutexLocker locker(&placeholder_cache_mutex);
        placeholder_cache.insert(filepath_q, cached_path);
        return rust::String(cached_path);
    }

    int r = 50 + (static_cast<unsigned char>(hash[0]) % 156);
    int g = 30 + (static_cast<unsigned char>(hash[1]) % 120);
    int b = 70 + (static_cast<unsigned char>(hash[2]) % 140);

    QColor c1(r, g, b);
    QColor c2((r + 60) % 256, (g + 40) % 256, (b + 80) % 256);

    QImage img(128, 128, QImage::Format_ARGB32_Premultiplied);
    img.fill(Qt::transparent);
    QPainter painter(&img);
    painter.setRenderHint(QPainter::Antialiasing);
    QLinearGradient gradient(0, 0, 128, 128);
    gradient.setColorAt(0.0, c1);
    gradient.setColorAt(1.0, c2);
    painter.setBrush(gradient);
    painter.setPen(Qt::NoPen);
    painter.drawRoundedRect(0, 0, 128, 128, 12, 12);

    const QString display_title = Ffi::to_qstring(title_copy);
    const QChar first = display_title.isEmpty() ? QChar('?') : display_title.at(0).toUpper();
    painter.setPen(QPen(QColor(255, 255, 255, 200), 2));
    QFont font = painter.font();
    font.setPixelSize(52);
    font.setBold(true);
    painter.setFont(font);
    painter.drawText(QRect(0, 0, 128, 128), Qt::AlignCenter, QString(first));
    painter.end();
    img.save(filepath_q, "PNG");

    const std::string filepath = Ffi::to_std_string(filepath_q);
    if (!filepath.empty()) {
        QMutexLocker locker(&placeholder_cache_mutex);
        placeholder_cache.insert(filepath_q, filepath);
    }
    return rust::String(filepath);
}

// ── Bridge functions ──────────────────────────────────────

namespace {
constexpr uint16_t kBridgeContractMajor = 1;
constexpr uint16_t kBridgeContractMinor = 3;
}

uint16_t bridge_contract_major() {
    return kBridgeContractMajor;
}

uint16_t bridge_contract_minor() {
    return kBridgeContractMinor;
}

void create_main_window(rust::Str, rust::Str, rust::Str, int32_t) {
    if (!QApplication::instance()) {
        QGuiApplication::setHighDpiScaleFactorRoundingPolicy(
            Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);
        static const char *argv[] = {"doremi", nullptr};
        static int argc = 1;
        new QApplication(argc, const_cast<char **>(argv));
        QPixmapCache::setCacheLimit(65536);
    }
    if (g_main_window) {
        delete g_main_window;
    }
    // DoremiMainWindow's constructor assigns g_main_window = this;
    new DoremiMainWindow();
}

void show_main_window() {
    mutate_main_window("show_main_window", [](DoremiMainWindow &window) { window.show(); });
}

void navigate_to(rust::Str route) {
    const std::string route_copy = Ffi::to_std_string(route);
    mutate_main_window("navigate_to", [route_copy](DoremiMainWindow &window) {
        window.navigate_to(route_copy);
    });
}

void show_notification(rust::Str message, rust::Str kind) {
    const std::string message_copy = Ffi::to_std_string(message);
    const std::string kind_copy = Ffi::to_std_string(kind);
    mutate_main_window("show_notification", [message_copy, kind_copy](DoremiMainWindow &window) {
        window.show_notif(message_copy, kind_copy);
    });
}

void apply_theme(rust::Str theme_mode, rust::Str accent_color) {
    std::string t_mode = Ffi::to_std_string(theme_mode);
    std::string a_color = Ffi::to_std_string(accent_color);
    mutate_main_window("apply_theme", [t_mode, a_color](DoremiMainWindow &window) {
        if (window.theme_controller()) {
            window.theme_controller()->apply_theme(t_mode, a_color);
        }
    });
}

void update_player_state(int32_t state, int32_t pos, int32_t dur) {
    mutate_main_window("update_player_state", [=](DoremiMainWindow &window) {
        window.update_player_state(state, pos, dur);
    });
}

void set_mini_player(rust::Str title, rust::Str artist, rust::Str thumb) {
    const std::string title_copy = Ffi::to_std_string(title);
    const std::string artist_copy = Ffi::to_std_string(artist);
    const std::string thumb_copy = Ffi::to_std_string(thumb);
    mutate_main_window("set_mini_player", [=](DoremiMainWindow &window) {
        window.set_mini_player_info(title_copy, artist_copy, thumb_copy);
    });
}

rust::String get_search_bar_text() {
    const std::string text = Ffi::on_gui_blocking("get_search_bar_text", std::string(), []() {
        if (g_main_window && g_main_window->title_bar()) {
            return g_main_window->title_bar()->search_text();
        }
        return std::string();
    });
    return rust::String(text);
}

void set_search_bar_text(rust::Str text) {
    const std::string text_copy = Ffi::to_std_string(text);
    mutate_main_window("set_search_bar_text", [text_copy](DoremiMainWindow &window) {
        if (window.title_bar()) window.title_bar()->set_search_text(text_copy);
    });
}

void set_window_title(rust::Str title) {
    const QString title_copy = Ffi::to_qstring(title);
    mutate_main_window("set_window_title", [title_copy](DoremiMainWindow &window) {
        window.setWindowTitle(title_copy);
    });
}

void set_playing(bool playing) {
    mutate_main_window("set_playing", [playing](DoremiMainWindow &window) {
        window.set_playback_playing(playing);
    });
}

void set_player_volume(int32_t volume) {
    mutate_main_window("set_player_volume", [volume](DoremiMainWindow &window) {
        if (window.player_bar()) window.player_bar()->set_volume_value(volume);
    });
}

void run_event_loop() {
    if (auto *app = QApplication::instance())
        app->exec();
}

void set_search_history(rust::Vec<rust::String> queries) {
    std::vector<std::string> q;
    for (auto &x : queries) q.push_back(Ffi::to_std_string(x));
    mutate_main_window("set_search_history", [q = std::move(q)](DoremiMainWindow &window) {
        if (window.search_view()) window.search_view()->set_recent_searches(q);
    });
}

void set_search_suggestions(rust::Str query, rust::Vec<rust::String> suggestions) {
    const std::string q_str = Ffi::to_std_string(query);
    std::vector<std::string> values;
    values.reserve(suggestions.size());
    for (const auto &suggestion : suggestions) values.push_back(static_cast<std::string>(suggestion));
    mutate_main_window("set_search_suggestions", [q_str, values = std::move(values)](DoremiMainWindow &window) {
        if (window.title_bar()) window.title_bar()->set_search_suggestions(q_str, values);
    });
}

void set_search_results(TopResult top_result, bool has_top_result,
                        rust::Vec<Track> songs, rust::Vec<Track> videos, rust::Vec<Artist> artists,
                        rust::Vec<Album> albums, rust::Vec<Playlist> playlists,
                        rust::Vec<Show> shows, rust::Vec<Episode> episodes) {
    std::vector<Track> s;
    for (const auto &x : songs) s.push_back(x);
    std::vector<Track> v;
    for (const auto &x : videos) v.push_back(x);
    std::vector<Artist> a;
    for (const auto &x : artists) a.push_back(x);
    std::vector<Album> al;
    for (const auto &x : albums) al.push_back(x);
    std::vector<Playlist> p;
    for (const auto &x : playlists) p.push_back(x);
    std::vector<Show> sh;
    for (const auto &x : shows) sh.push_back(x);
    std::vector<Episode> e;
    for (const auto &x : episodes) e.push_back(x);
    mutate_main_window("set_search_results", [top_result, has_top_result, s = std::move(s), v = std::move(v), a = std::move(a), al = std::move(al), p = std::move(p), sh = std::move(sh), e = std::move(e)](DoremiMainWindow &window) {
        if (window.search_view()) window.search_view()->set_results(top_result, has_top_result, s, v, a, al, p, sh, e);
    });
}

void add_home_section(rust::Str title, rust::Vec<HomeCard> items) {
    const std::string title_copy = Ffi::to_std_string(title);
    std::vector<HomeCard> v;
    for (const auto &x : items) v.push_back(x);
    mutate_main_window("add_home_section", [title_copy, v = std::move(v)](DoremiMainWindow &window) {
        if (window.home_view()) window.home_view()->add_section(title_copy, v);
    });
}

void clear_home_sections() {
    mutate_main_window("clear_home_sections", [](DoremiMainWindow &window) {
        if (window.home_view()) window.home_view()->clear_sections();
    });
}

void set_home_state(rust::Str state, rust::Str message) {
    const std::string state_copy = Ffi::to_std_string(state);
    const std::string message_copy = Ffi::to_std_string(message);
    mutate_main_window("set_home_state", [state_copy, message_copy](DoremiMainWindow &window) {
        if (window.home_view()) window.home_view()->set_state(state_copy, message_copy);
    });
}

void set_library_albums(rust::Vec<Album> albums) {
    std::vector<Album> a;
    for (const auto &x : albums) a.push_back(x);
    mutate_main_window("set_library_albums", [a = std::move(a)](DoremiMainWindow &window) {
        if (window.library_view()) window.library_view()->set_albums(a);
    });
}

void set_library_artists(rust::Vec<Artist> artists) {
    std::vector<Artist> a;
    for (const auto &x : artists) a.push_back(x);
    mutate_main_window("set_library_artists", [a = std::move(a)](DoremiMainWindow &window) {
        if (window.library_view()) window.library_view()->set_artists(a);
    });
}

void set_library_state(rust::Str state, rust::Str message) {
    const std::string state_copy = Ffi::to_std_string(state);
    const std::string message_copy = Ffi::to_std_string(message);
    mutate_main_window("set_library_state", [state_copy, message_copy](DoremiMainWindow &window) {
        if (window.library_view()) window.library_view()->set_library_state(state_copy, message_copy);
    });
}

void set_library_songs(rust::Vec<Track> songs) {
    std::vector<Track> v;
    for (const auto &x : songs) v.push_back(x);
    mutate_main_window("set_library_songs", [v = std::move(v)](DoremiMainWindow &window) {
        if (window.library_view()) window.library_view()->set_songs(v);
    });
}

void set_library_shows(rust::Vec<Show> shows) {
    std::vector<Show> v;
    for (const auto &x : shows) v.push_back(x);
    mutate_main_window("set_library_shows", [v = std::move(v)](DoremiMainWindow &window) {
        if (window.library_view()) window.library_view()->set_shows(v);
    });
}

void set_library_playlists(rust::Vec<Playlist> playlists) {
    std::vector<Playlist> p;
    for (const auto &x : playlists) p.push_back(x);
    mutate_main_window("set_library_playlists", [p = std::move(p)](DoremiMainWindow &window) {
        if (window.library_view()) window.library_view()->set_playlists(p);
    });
}

void DoremiMainWindow::set_context_playlists(const std::vector<Playlist> &playlists) {
    context_playlists_ = playlists;
}

const std::vector<Playlist> &get_context_playlists() {
    static const std::vector<Playlist> empty_playlists;
    if (g_main_window) {
        return g_main_window->context_playlists();
    }
    return empty_playlists;
}

void set_context_playlists(rust::Vec<Playlist> playlists) {
    std::vector<Playlist> p;
    p.reserve(playlists.size());
    for (const auto &x : playlists) p.push_back(x);
    mutate_main_window("set_context_playlists", [p = std::move(p)](DoremiMainWindow &window) {
        window.set_context_playlists(p);
    });
}

void apply_settings_to_ui() {
    // Implemented in Rust — reads AppSettings and calls individual setters
}

void set_settings_theme(rust::Str mode) {
    const std::string value = Ffi::to_std_string(mode);
    mutate_main_window("set_settings_theme", [value](DoremiMainWindow &window) {
        if (window.settings_view()) window.settings_view()->set_theme(value);
    });
}

void set_settings_accent(rust::Str color) {
    const std::string value = Ffi::to_std_string(color);
    mutate_main_window("set_settings_accent", [value](DoremiMainWindow &window) {
        if (window.settings_view()) window.settings_view()->set_accent(value);
    });
}

void set_settings_font_size(int32_t size) {
    mutate_main_window("set_settings_font_size", [size](DoremiMainWindow &window) {
        if (window.settings_view()) window.settings_view()->set_font_size(size);
    });
}

void set_settings_language(rust::Str lang) {
    const std::string value = Ffi::to_std_string(lang);
    mutate_main_window("set_settings_language", [value](DoremiMainWindow &window) {
        if (window.settings_view()) window.settings_view()->set_language(value);
    });
}

void set_settings_region(rust::Str region) {
    const std::string value = Ffi::to_std_string(region);
    mutate_main_window("set_settings_region", [value](DoremiMainWindow &window) {
        if (window.settings_view()) window.settings_view()->set_region(value);
    });
}

void set_settings_normalize(bool on) {
    mutate_main_window("set_settings_normalize", [on](DoremiMainWindow &window) {
        if (window.settings_view()) window.settings_view()->set_normalize(on);
    });
}

void set_settings_crossfade(bool on) {
    mutate_main_window("set_settings_crossfade", [on](DoremiMainWindow &window) {
        if (window.settings_view()) window.settings_view()->set_crossfade(on);
    });
}

void set_settings_equalizer_enabled(bool on) {
    mutate_main_window("set_settings_equalizer_enabled", [on](DoremiMainWindow &window) {
        if (window.settings_view()) window.settings_view()->set_equalizer_enabled(on);
    });
}

void set_settings_equalizer_preset(rust::Str preset) {
    const std::string value = Ffi::to_std_string(preset);
    mutate_main_window("set_settings_equalizer_preset", [value](DoremiMainWindow &window) {
        if (window.settings_view()) window.settings_view()->set_equalizer_preset(value);
    });
}

void set_settings_equalizer_values(double preamp, rust::Vec<double> bands) {
    std::vector<double> values;
    values.reserve(bands.size());
    for (double band : bands) values.push_back(band);
    mutate_main_window("set_settings_equalizer_values", [preamp, values = std::move(values)](DoremiMainWindow &window) {
        if (window.settings_view()) window.settings_view()->set_equalizer_values(preamp, values);
    });
}

void set_settings_sleep_timer(int32_t minutes) {
    mutate_main_window("set_settings_sleep_timer", [minutes](DoremiMainWindow &window) {
        if (window.settings_view()) window.settings_view()->set_sleep_timer(minutes);
    });
}

void set_settings_discord_rpc(bool on) {
    mutate_main_window("set_settings_discord_rpc", [on](DoremiMainWindow &window) {
        if (window.settings_view()) window.settings_view()->set_settings_discord_rpc(on);
    });
}

void set_settings_lastfm_enabled(bool on) {
    mutate_main_window("set_settings_lastfm_enabled", [on](DoremiMainWindow &window) {
        if (window.settings_view()) window.settings_view()->set_settings_lastfm_enabled(on);
    });
}

void set_settings_lastfm_session(bool authenticated, rust::Str username, rust::Str apiKey, rust::Str apiSecret) {
    const std::string username_copy = Ffi::to_std_string(username);
    const std::string api_key_copy = Ffi::to_std_string(apiKey);
    std::string api_secret_copy = Ffi::to_std_string(apiSecret);
    mutate_main_window("set_settings_lastfm_session", [authenticated, username_copy, api_key_copy, api_secret_copy](DoremiMainWindow &window) mutable {
        if (window.settings_view()) {
            window.settings_view()->set_settings_lastfm_session(
                authenticated, username_copy, api_key_copy, api_secret_copy);
        }
        std::fill(api_secret_copy.begin(), api_secret_copy.end(), '\0');
    });
}

void set_settings_stop_on_close(bool stop) {
    mutate_main_window("set_settings_stop_on_close", [stop](DoremiMainWindow &window) {
        window.set_stop_on_close(stop);
    });
}

void set_settings_mpris_enabled(bool on) {
    mutate_main_window("set_settings_mpris_enabled", [on](DoremiMainWindow &window) {
        if (window.settings_view()) window.settings_view()->set_settings_mpris_enabled(on);
    });
}

void set_track_lyrics(rust::Str plain, rust::Str synced) {
    const std::string plain_copy = Ffi::to_std_string(plain);
    const std::string synced_copy = Ffi::to_std_string(synced);
    mutate_main_window("set_track_lyrics", [=](DoremiMainWindow &window) {
        window.set_track_lyrics(plain_copy, synced_copy);
    });
}

void set_settings_subtitle_alignment(rust::Str align) {
    const std::string value = Ffi::to_std_string(align);
    mutate_main_window("set_settings_subtitle_alignment", [value](DoremiMainWindow &window) {
        if (window.settings_view()) window.settings_view()->set_subtitle_alignment(value);
        if (window.now_playing_view()) window.now_playing_view()->setSubtitleAlignment(value);
    });
}

void set_settings_subtitle_font_size(int32_t size) {
    mutate_main_window("set_settings_subtitle_font_size", [size](DoremiMainWindow &window) {
        if (window.settings_view()) window.settings_view()->set_subtitle_font_size(size);
        if (window.now_playing_view()) window.now_playing_view()->setSubtitleFontSize(size);
    });
}

void set_settings_subtitle_line_spacing(double spacing) {
    mutate_main_window("set_settings_subtitle_line_spacing", [spacing](DoremiMainWindow &window) {
        if (window.settings_view()) window.settings_view()->set_subtitle_line_spacing(spacing);
        if (window.now_playing_view()) window.now_playing_view()->setSubtitleLineSpacing(spacing);
    });
}

void set_settings_subtitle_auto_scroll(bool on) {
    mutate_main_window("set_settings_subtitle_auto_scroll", [on](DoremiMainWindow &window) {
        if (window.settings_view()) window.settings_view()->set_subtitle_auto_scroll(on);
        if (window.now_playing_view()) window.now_playing_view()->setSubtitleAutoScroll(on);
    });
}

void set_settings_subtitle_glow_effect(bool on) {
    mutate_main_window("set_settings_subtitle_glow_effect", [on](DoremiMainWindow &window) {
        if (window.settings_view()) window.settings_view()->set_subtitle_glow_effect(on);
        if (window.now_playing_view()) window.now_playing_view()->setSubtitleGlowEffect(on);
    });
}

void set_settings_download_location(rust::Str location) {
    const std::string value = Ffi::to_std_string(location);
    mutate_main_window("set_settings_download_location", [value](DoremiMainWindow &window) {
        if (window.settings_view()) window.settings_view()->set_download_location(value);
    });
}

void set_settings_download_format(rust::Str format) {
    const std::string value = Ffi::to_std_string(format);
    mutate_main_window("set_settings_download_format", [value](DoremiMainWindow &window) {
        if (window.settings_view()) window.settings_view()->set_download_format(value);
    });
}

void set_settings_download_quality(rust::Str quality) {
    const std::string value = Ffi::to_std_string(quality);
    mutate_main_window("set_settings_download_quality", [value](DoremiMainWindow &window) {
        if (window.settings_view()) window.settings_view()->set_download_quality(value);
    });
}



void set_dominant_colors(rust::Vec<rust::String> colors) {
    std::vector<std::string> values;
    for (const auto &color : colors) values.push_back(Ffi::to_std_string(color));
    mutate_main_window("set_dominant_colors", [values = std::move(values)](DoremiMainWindow &window) {
        window.set_dominant_colors(values);
    });
}

void set_playback_queue(rust::Vec<Track> queue, int32_t current_index) {
    std::vector<Track> tracks;
    tracks.reserve(queue.size());
    for (const auto &track : queue) tracks.push_back(track);
    mutate_main_window("set_playback_queue", [tracks = std::move(tracks), current_index](DoremiMainWindow &window) {
        window.set_playback_queue(tracks, current_index);
    });
}

void set_related_tracks(rust::Vec<Track> tracks) {
    std::vector<Track> t;
    t.reserve(tracks.size());
    for (const auto &tr : tracks) t.push_back(tr);
    mutate_main_window("set_related_tracks", [t = std::move(t)](DoremiMainWindow &window) {
        window.set_related_tracks(t);
    });
}

void set_current_track(Track track) {
    mutate_main_window("set_current_track", [track = std::move(track)](DoremiMainWindow &window) mutable {
        window.set_current_track(track);
    });
}

void set_stats_data(StatsData stats) {
    mutate_main_window("set_stats_data", [stats = std::move(stats)](DoremiMainWindow &window) {
        window.set_stats_data(stats);
    });
}

void set_online_status(bool is_online) {
    mutate_main_window("set_online_status", [is_online](DoremiMainWindow &window) {
        window.set_online_status(is_online);
    });
}



// ── Player state sync ──

void set_player_shuffle(bool on) {
    mutate_main_window("set_player_shuffle", [on](DoremiMainWindow &window) {
        if (window.player_bar()) window.player_bar()->set_shuffle(on);
        if (window.now_playing_view()) window.now_playing_view()->setShuffle(on);
    });
}

void set_player_repeat(int32_t mode) {
    mutate_main_window("set_player_repeat", [mode](DoremiMainWindow &window) {
        if (window.player_bar()) window.player_bar()->set_repeat_mode(mode);
        if (window.now_playing_view()) window.now_playing_view()->setRepeatMode(mode);
    });
}


// ── Trending ──

void set_trending_items(rust::Vec<HomeCard> items) {
    std::vector<HomeCard> cards;
    for (const auto &item : items) cards.push_back(item);
    mutate_main_window("set_trending_items", [cards = std::move(cards)](DoremiMainWindow &window) {
        if (!window.trending_view()) return;
        window.trending_view()->clear_items();
        for (const auto &item : cards) window.trending_view()->add_item(item);
    });
}

void set_trending_state(rust::Str state, rust::Str message) {
    const std::string state_copy = Ffi::to_std_string(state);
    const std::string message_copy = Ffi::to_std_string(message);
    mutate_main_window("set_trending_state", [state_copy, message_copy](DoremiMainWindow &window) {
        if (window.trending_view()) window.trending_view()->set_state(state_copy, message_copy);
    });
}

// ── Downloads ──

void set_downloads_list(rust::Vec<DownloadItem> items) {
    std::vector<DownloadItem> list;
    for (auto &x : items) {
        list.push_back(x);
    }
    mutate_main_window("set_downloads_list", [list = std::move(list)](DoremiMainWindow &window) {
        if (window.downloads_view()) window.downloads_view()->set_downloads(list);
    });
}

void set_download_progress(rust::Str video_id, double percent, rust::Str status) {
    std::string vid = Ffi::to_std_string(video_id);
    std::string st = Ffi::to_std_string(status);
    qDebug() << "Download" << vid.c_str() << ":" << st.c_str() << percent << "%";
    mutate_main_window("set_download_progress", [vid, percent, st](DoremiMainWindow &window) {
        if (window.downloads_view()) {
            window.downloads_view()->set_progress(vid, percent, st);
        }
    });
}

void set_batch_download_progress(rust::Str parent_id, int32_t total, int32_t completed, double percent) {
    std::string pid = Ffi::to_std_string(parent_id);
    qDebug() << "Batch" << pid.c_str() << ":" << completed << "/" << total << "(" << percent << "%)";
    mutate_main_window("set_batch_download_progress", [pid, total, completed, percent](DoremiMainWindow &window) {
        if (window.downloads_view()) {
            window.downloads_view()->set_batch_progress(pid, total, completed, percent);
        }
    });
}

// ── History ──

void set_history_data(rust::Vec<Track> history, rust::Vec<rust::String> played_at, rust::Vec<rust::String> feedback_tokens) {
    std::vector<Track> h;
    h.reserve(history.size());
    for (const auto &t : history) h.push_back(t);
    std::vector<std::string> pa;
    pa.reserve(played_at.size());
    for (const auto &x : played_at) pa.push_back(Ffi::to_std_string(x));
    std::vector<std::string> ft;
    ft.reserve(feedback_tokens.size());
    for (const auto &y : feedback_tokens) ft.push_back(Ffi::to_std_string(y));
    mutate_main_window("set_history_data", [h = std::move(h), pa = std::move(pa), ft = std::move(ft)](DoremiMainWindow &window) {
        window.set_history_data(h, pa, ft);
    });
}

// ── Album Detail ──

void set_album_detail(Album album, rust::Vec<Track> tracks) {
    std::vector<Track> tt;
    tt.reserve(tracks.size());
    for (const auto &x : tracks) tt.push_back(x);
    mutate_main_window("set_album_detail", [album = std::move(album), tt = std::move(tt)](DoremiMainWindow &window) {
        if (!window.album_detail_view()) return;
        window.album_detail_view()->set_album_info(album);
        window.album_detail_view()->set_album_tracks(tt);
        window.navigate_to("album_detail");
    });
}

// ── Artist Detail ──

void set_artist_detail(Artist artist, rust::Vec<Track> tracks, rust::Vec<Album> albums, rust::Vec<Album> singles) {
    std::vector<Track> tt;
    tt.reserve(tracks.size());
    for (const auto &x : tracks) tt.push_back(x);
    std::vector<Album> al;
    al.reserve(albums.size());
    for (const auto &x : albums) al.push_back(x);
    std::vector<Album> si;
    si.reserve(singles.size());
    for (const auto &x : singles) si.push_back(x);
    mutate_main_window("set_artist_detail", [artist = std::move(artist), tt = std::move(tt), al = std::move(al), si = std::move(si)](DoremiMainWindow &window) {
        if (!window.artist_detail_view()) return;
        window.artist_detail_view()->set_artist_info(artist);
        window.artist_detail_view()->set_artist_tracks(tt, al, si);
        window.navigate_to("artist_detail");
    });
}

// ── Playlist Detail ──

void set_playlist_detail(Playlist playlist, rust::Vec<Track> tracks) {
    std::vector<Track> tt;
    tt.reserve(tracks.size());
    for (const auto &x : tracks) tt.push_back(x);
    mutate_main_window("set_playlist_detail", [playlist = std::move(playlist), tt = std::move(tt)](DoremiMainWindow &window) {
        if (!window.playlist_detail_view()) return;
        window.playlist_detail_view()->set_playlist_info(playlist);
        window.playlist_detail_view()->set_playlist_tracks(tt);
        window.navigate_to("playlist_detail");
    });
}

void set_show_detail(Show show, rust::Vec<Episode> episodes) {
    std::vector<Episode> ee;
    for (const auto &x : episodes) ee.push_back(x);
    mutate_main_window("set_show_detail", [show = std::move(show), ee = std::move(ee)](DoremiMainWindow &window) mutable {
        if (!window.show_detail_view()) return;
        window.show_detail_view()->set_show_info(show);
        window.show_detail_view()->set_episodes(ee);
        window.navigate_to("show_detail");
    });
}

void set_prefetch_status(rust::Str track_id, rust::Str status) {
    std::string tid = Ffi::to_std_string(track_id);
    std::string st = Ffi::to_std_string(status);
    qDebug() << "Prefetch" << tid.c_str() << ":" << st.c_str();
}

void update_youtube_auth_state(bool authenticated, rust::Str name, rust::Str avatar_url) {
    const std::string name_copy = Ffi::to_std_string(name);
    const std::string avatar_copy = Ffi::to_std_string(avatar_url);
    mutate_main_window("update_youtube_auth_state", [=](DoremiMainWindow &window) {
        if (window.nav_sidebar()) {
            window.nav_sidebar()->update_profile(authenticated, name_copy, avatar_copy);
        }
        if (window.welcome_view()) window.welcome_view()->update_theme();
        if (window.library_view()) {
            window.library_view()->set_authenticated(authenticated);
            if (window.current_route() == "library") {
                std::string tab = window.library_view()->current_tab();
                if (tab.empty()) tab = "playlists";
                emit window.library_view()->tab_changed(tab);
            }
        }
        if (!authenticated) {
            auto *profile = QWebEngineProfile::defaultProfile();
            if (profile) {
                profile->cookieStore()->deleteAllCookies();
                profile->clearHttpCache();
            }
        }
    });
}

void DoremiMainWindow::setup_ui_test(const std::string &view, const std::string &screenshot_path) {
    // 1. Set fixed size
    resize(1300, 820);
    setFixedSize(1300, 820);

    // 2. Navigate to target view
    if (view == "now-playing" || view == "now_playing") {
        navigate_to("home");
        if (now_playing_view_) {
            now_playing_view_->showView();
        }
    } else {
        navigate_to(view);
    }

    // 3. Set timer to take screenshot and exit.
    // Delay is 3500ms to ensure: navigation completes (queued first), then
    // mock data arrives and renders (queued second by Rust), before capture.
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

void setup_ui_test(rust::Str view, rust::Str screenshot_path) {
    std::string v_str = Ffi::to_std_string(view);
    std::string path_str = Ffi::to_std_string(screenshot_path);
    mutate_main_window("setup_ui_test", [v_str, path_str](DoremiMainWindow &window) {
        window.setup_ui_test(v_str, path_str);
    });
}
