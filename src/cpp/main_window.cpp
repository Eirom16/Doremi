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
    stack_->addWidget(placeholder);             // idx 0
    stack_->addWidget(home_view_);              // idx 1
    stack_->addWidget(search_view_);            // idx 2
    stack_->addWidget(library_view_);           // idx 3
    stack_->addWidget(settings_view_);          // idx 4
    stack_->addWidget(trending_view_);          // idx 5
    stack_->addWidget(downloads_view_);         // idx 6
    stack_->addWidget(stats_view_);             // idx 7
    stack_->addWidget(history_view_);           // idx 8
    stack_->addWidget(album_detail_view_);      // idx 9
    stack_->addWidget(artist_detail_view_);     // idx 10
    stack_->addWidget(playlist_detail_view_);   // idx 11
    stack_->addWidget(show_detail_view_);        // idx 12
    stack_->addWidget(welcome_view_);           // idx 13


    stack_->setCurrentIndex(1); // start at home
    auto *right_container = new QWidget(central);
    auto *right_layout = new QVBoxLayout(right_container);
    right_layout->setContentsMargins(0, 0, 0, 0);
    right_layout->setSpacing(0);

    offline_banner_ = new OfflineBannerWidget(right_container);
    right_layout->addWidget(offline_banner_);

    body_scroll_ = new QScrollArea(right_container);
    body_scroll_->setWidgetResizable(true);
    body_scroll_->setFrameShape(QFrame::NoFrame);
    body_scroll_->setStyleSheet("background: transparent; border: none;");
    body_scroll_->setWidget(stack_);
    right_layout->addWidget(body_scroll_, 1);

    body->addWidget(right_container, 1);

    root->addLayout(body, 1);

    player_shell_ = new QWidget(central);
    player_shell_->setAttribute(Qt::WA_StyledBackground, true);
    player_shell_->setStyleSheet("background: transparent;");
    player_shell_layout_ = new QHBoxLayout(player_shell_);
    player_shell_layout_->setContentsMargins(16, 0, 16, 12);
    player_shell_layout_->setSpacing(0);
    player_bar_ = new PlayerBar(player_shell_);
    player_shell_layout_->addWidget(player_bar_);
    root->addWidget(player_shell_);

    setCentralWidget(central);

    now_playing_view_ = new NowPlayingView(this);
    now_playing_view_->hide();

    theme_transition_ = new ThemeTransitionOverlay(this);
    theme_transition_->hide();
    setup_session_cookie_refresh();


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
    setup_shortcuts();
    setup_tray();
    connect_signals();

    player_timer_ = new QTimer(this);
    player_timer_->setInterval(250);
    QObject::connect(player_timer_, &QTimer::timeout, this, []() { on_timer_tick(); });
    player_timer_->start();
}

void DoremiMainWindow::setup_session_cookie_refresh() {
    auto *profile = QWebEngineProfile::defaultProfile();
    if (!profile) return;
    auto *store = profile->cookieStore();
    if (!store) return;

    session_cookie_timer_ = new QTimer(this);
    session_cookie_timer_->setSingleShot(true);
    session_cookie_timer_->setInterval(1500);
    connect(session_cookie_timer_, &QTimer::timeout,
            this, &DoremiMainWindow::persist_session_cookies);
    connect(store, &QWebEngineCookieStore::cookieAdded, this,
            [this](const QNetworkCookie &cookie) {
                update_session_cookie(cookie, false);
            });
    connect(store, &QWebEngineCookieStore::cookieRemoved, this,
            [this](const QNetworkCookie &cookie) {
                update_session_cookie(cookie, true);
            });
    store->loadAllCookies();
}

void DoremiMainWindow::update_session_cookie(const QNetworkCookie &cookie, bool removed) {
    QString domain = cookie.domain().toLower();
    if (domain.startsWith('.')) domain.remove(0, 1);
    const bool trusted = domain == "youtube.com" || domain.endsWith(".youtube.com") ||
                         domain == "google.com" || domain.endsWith(".google.com");
    if (!trusted) return;

    const std::string name = cookie.name().toStdString();
    if (name.empty()) return;
    const bool expired = !cookie.isSessionCookie() &&
                         cookie.expirationDate().isValid() &&
                         cookie.expirationDate() <= QDateTime::currentDateTimeUtc();
    if (removed || expired || cookie.value().isEmpty()) {
        session_cookies_.erase(name);
    } else {
        session_cookies_[name] = cookie.value().toStdString();
    }
    session_cookie_timer_->start();
}

void DoremiMainWindow::persist_session_cookies() {
    const bool has_sapisid = session_cookies_.count("SAPISID") ||
                             session_cookies_.count("__Secure-3PAPISID") ||
                             session_cookies_.count("__Secure-1PAPISID");
    if (!has_sapisid) return;

    QString cookie_string;
    for (const auto &[name, value] : session_cookies_) {
        if (!cookie_string.isEmpty()) cookie_string += "; ";
        cookie_string += QString::fromStdString(name + "=" + value);
    }
    QJsonObject headers;
    headers.insert("cookie", cookie_string);
    headers.insert("x-goog-authuser", "0");
    headers.insert("user-agent", "Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 Doremi/2");
    headers.insert("accept", "*/*");
    headers.insert("origin", "https://music.youtube.com");
    headers.insert("x-origin", "https://music.youtube.com");
    const QString json = QJsonDocument(headers).toJson(QJsonDocument::Compact);
    on_youtube_session_refresh(Ffi::to_std_string(json));
}

DoremiMainWindow::~DoremiMainWindow() { g_main_window = nullptr; }

void DoremiMainWindow::setup_shortcuts() {
    auto *space = new QShortcut(QKeySequence(Qt::Key_Space), this);
    QObject::connect(space, &QShortcut::activated, this, [this]() { emit play_pause_triggered(); });
    auto *right = new QShortcut(QKeySequence(Qt::Key_Right), this);
    QObject::connect(right, &QShortcut::activated, this, [this]() { emit seek_triggered(5000); });
    auto *left = new QShortcut(QKeySequence(Qt::Key_Left), this);
    QObject::connect(left, &QShortcut::activated, this, [this]() { emit seek_triggered(-5000); });
    auto *up = new QShortcut(QKeySequence(Qt::Key_Up), this);
    QObject::connect(up, &QShortcut::activated, this, [this]() { emit volume_changed(5); });
    auto *down = new QShortcut(QKeySequence(Qt::Key_Down), this);
    QObject::connect(down, &QShortcut::activated, this, [this]() { emit volume_changed(-5); });
    auto *focus_search = new QShortcut(QKeySequence("Ctrl+L"), this);
    focus_search->setWhatsThis("Enfocar el buscador");
    QObject::connect(focus_search, &QShortcut::activated, this, [this]() {
        title_bar_->focus_search();
    });
    auto *focus_search_alt = new QShortcut(QKeySequence("Ctrl+K"), this);
    focus_search_alt->setWhatsThis("Enfocar el buscador");
    QObject::connect(focus_search_alt, &QShortcut::activated, this, [this]() {
        title_bar_->focus_search();
    });
    auto *back = new QShortcut(QKeySequence::Back, this);
    back->setWhatsThis("Volver a la pantalla anterior");
    QObject::connect(back, &QShortcut::activated, this, [this]() { navigate_back(); });
    auto *forward = new QShortcut(QKeySequence::Forward, this);
    forward->setWhatsThis("Avanzar a la pantalla siguiente");
    QObject::connect(forward, &QShortcut::activated, this, [this]() { navigate_forward(); });

    // Media Keys
    auto *media_play = new QShortcut(QKeySequence(Qt::Key_MediaPlay), this);
    QObject::connect(media_play, &QShortcut::activated, this, [this]() { emit play_pause_triggered(); });
    auto *media_pause = new QShortcut(QKeySequence(Qt::Key_MediaPause), this);
    QObject::connect(media_pause, &QShortcut::activated, this, [this]() { emit play_pause_triggered(); });
    auto *media_toggle = new QShortcut(QKeySequence(Qt::Key_MediaTogglePlayPause), this);
    QObject::connect(media_toggle, &QShortcut::activated, this, [this]() { emit play_pause_triggered(); });
    auto *media_next = new QShortcut(QKeySequence(Qt::Key_MediaNext), this);
    QObject::connect(media_next, &QShortcut::activated, this, [this]() { emit next_triggered(); });
    auto *media_prev = new QShortcut(QKeySequence(Qt::Key_MediaPrevious), this);
    QObject::connect(media_prev, &QShortcut::activated, this, [this]() { emit previous_triggered(); });
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
        [this](const std::string &id) {
            if (!ensure_online_action("abrir detalles de playlist")) return;
            on_playlist_requested(id);
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
        [this](const std::string &id) {
            if (!ensure_online_action("abrir detalles de podcast")) return;
            on_show_requested(id);
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
    } else if (tray_icon_ && tray_icon_->isVisible()) {
        hide();
        event->ignore();
    } else {
        emit window_closed();
        event->accept();
    }
}

void DoremiMainWindow::navigate_to(const std::string &route) {
    navigate_to_internal(route, true);
}

void DoremiMainWindow::navigate_to_internal(const std::string &route, bool record_history) {
    if (route.empty()) {
        return;
    }
    save_route_view_state();

    if (record_history && route != current_route_) {
        back_routes_.push_back(current_route_);
        if (back_routes_.size() > 80) {
            back_routes_.erase(back_routes_.begin());
        }
        forward_routes_.clear();
    }

    const bool opens_detail = route == "album_detail" || route == "artist_detail" ||
                              route == "playlist_detail" || route == "show_detail";
    if (opens_detail && current_route_ != "album_detail" &&
        current_route_ != "artist_detail" && current_route_ != "playlist_detail" &&
        current_route_ != "show_detail") {
        detail_return_route_ = current_route_;
    }
    nav_sidebar_->set_active_route(route);
    if (route == "home") {
        stack_->setCurrentIndex(1);
    } else if (route == "search") {
        stack_->setCurrentIndex(2);
    } else if (route == "library") {
        stack_->setCurrentIndex(3);
        std::string tab = library_view_->current_tab();
        if (tab.empty()) tab = "playlists";
        emit library_view_->tab_changed(tab);
    } else if (route == "settings") {
        stack_->setCurrentIndex(4);
    } else if (route == "trending") {
        stack_->setCurrentIndex(5);
    } else if (route == "downloads") {
        stack_->setCurrentIndex(6);
        on_downloads_requested();
    } else if (route == "stats") {
        stack_->setCurrentIndex(7);
    } else if (route == "history") {
        stack_->setCurrentIndex(8);
        on_history_requested();
    } else if (route == "album_detail") {
        stack_->setCurrentIndex(9);
    } else if (route == "artist_detail") {
        stack_->setCurrentIndex(10);
    } else if (route == "playlist_detail") {
        stack_->setCurrentIndex(11);
    } else if (route == "show_detail") {
        stack_->setCurrentIndex(12);
    } else if (route == "welcome") {
        stack_->setCurrentIndex(13);
    } else {
        stack_->setCurrentIndex(1);
    }
    current_route_ = route;
    restore_route_view_state(route);
}

void DoremiMainWindow::save_route_view_state() {
    if (current_route_.empty()) {
        return;
    }
    if (body_scroll_ && body_scroll_->verticalScrollBar()) {
        route_scroll_positions_[current_route_] = body_scroll_->verticalScrollBar()->value();
    }
    if (auto *focused = QApplication::focusWidget()) {
        if (focused == this || isAncestorOf(focused)) {
            route_focus_widgets_[current_route_] = focused;
        }
    }
}

void DoremiMainWindow::restore_route_view_state(const std::string &route) {
    const int scroll = route_scroll_positions_.count(route) ? route_scroll_positions_[route] : 0;
    QPointer<QWidget> focus = route_focus_widgets_.count(route) ? route_focus_widgets_[route] : QPointer<QWidget>();
    QTimer::singleShot(0, this, [this, route, scroll, focus]() {
        if (route != current_route_) {
            return;
        }
        if (body_scroll_ && body_scroll_->verticalScrollBar()) {
            body_scroll_->verticalScrollBar()->setValue(scroll);
        }
        if (focus && focus->isVisible() && focus->isEnabled()) {
            focus->setFocus(Qt::OtherFocusReason);
        } else if (stack_) {
            stack_->setFocus(Qt::OtherFocusReason);
        }
    });
}

void DoremiMainWindow::navigate_back() {
    while (!back_routes_.empty() && back_routes_.back() == current_route_) {
        back_routes_.pop_back();
    }
    if (back_routes_.empty()) {
        return;
    }
    const std::string target = back_routes_.back();
    back_routes_.pop_back();
    forward_routes_.push_back(current_route_);
    navigate_to_internal(target, false);
    if (target == "search") {
        on_search_history_requested();
    }
}

void DoremiMainWindow::navigate_back_from_detail() {
    navigate_to(detail_return_route_.empty() ? "home" : detail_return_route_);
}

void DoremiMainWindow::navigate_forward() {
    while (!forward_routes_.empty() && forward_routes_.back() == current_route_) {
        forward_routes_.pop_back();
    }
    if (forward_routes_.empty()) {
        return;
    }
    const std::string target = forward_routes_.back();
    forward_routes_.pop_back();
    back_routes_.push_back(current_route_);
    navigate_to_internal(target, false);
    if (target == "search") {
        on_search_history_requested();
    }
}

void DoremiMainWindow::show_notif(const std::string &message, const std::string &kind) {
    const QString qMessage = QString::fromStdString(message);
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

    if (tray_icon_ && tray_icon_->isVisible() && tray_icon_->supportsMessages()) {
        tray_icon_->showMessage(
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

static std::string g_last_track_title;
static std::string g_last_track_artist;

void DoremiMainWindow::set_mini_player_info(const std::string &title, const std::string &artist,
                                             const std::string &thumb) {
    player_bar_->set_track_info(title, artist, thumb);
    if (now_playing_view_) {
        now_playing_view_->setTrackInfo(title, artist, thumb);
    }
    if (playback_playing_ && !title.empty() && (title != g_last_track_title || artist != g_last_track_artist)) {
        g_last_track_title = title;
        g_last_track_artist = artist;
        show_notif("Reproduciendo: " + title + " — " + artist, "info");
    }
}

void DoremiMainWindow::set_playback_playing(bool playing) {
    playback_playing_ = playing;
    player_bar_->set_playing(playing);
    if (now_playing_view_) {
        now_playing_view_->setPlaying(playing);
    }
    if (play_action_) {
        play_action_->setText(playing ? "⏸ Pausa" : "▶ Reproducir");
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
        player_shell_layout_->setContentsMargins(sidebar_width + 16, 0, 16, 12);
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

void DoremiMainWindow::set_playback_queue(const rust::Vec<Track> &queue, int32_t current_index) {
    if (now_playing_view_) {
        std::vector<Track> q;
        for (const auto &t : queue) q.push_back(t);
        now_playing_view_->setQueue(q, current_index);
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

void DoremiMainWindow::set_related_tracks(const rust::Vec<Track> &tracks) {
    if (now_playing_view_) {
        std::vector<Track> t;
        for (const auto &tr : tracks) t.push_back(tr);
        now_playing_view_->setRelatedTracks(t);
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

void DoremiMainWindow::set_history_data(const rust::Vec<Track> &history, const rust::Vec<rust::String> &played_at, const rust::Vec<rust::String> &feedback_tokens) {
    if (history_view_) {
        std::vector<Track> h;
        for (const auto &t : history) h.push_back(t);
        std::vector<std::string> pa;
        for (const auto &x : played_at) pa.push_back(Ffi::to_std_string(x));
        std::vector<std::string> ft;
        for (const auto &y : feedback_tokens) ft.push_back(Ffi::to_std_string(y));
        history_view_->set_history(h, pa, ft);
    }
}


void DoremiMainWindow::setup_tray() {
    QPixmap px(16, 16);
    px.fill(Qt::transparent);
    QPainter pt(&px);
    pt.setRenderHint(QPainter::Antialiasing);
    pt.setBrush(QColor("#7C7CF0"));
    pt.setPen(Qt::NoPen);
    pt.drawEllipse(1, 1, 14, 14);
    pt.end();
    auto icon = QIcon(px);

    tray_icon_ = new QSystemTrayIcon(icon, this);
    tray_icon_->setToolTip("Doremi");

    auto *menu = new QMenu(this);
    play_action_ = menu->addAction("▶ Reproducir");
    auto *next_action = menu->addAction("⏭ Siguiente");
    auto *prev_action = menu->addAction("⏮ Anterior");
    menu->addSeparator();
    auto *show_action = menu->addAction("Mostrar ventana");
    auto *quit_action = menu->addAction("Salir");

    QObject::connect(play_action_, &QAction::triggered, this, [this]() { emit play_pause_triggered(); });
    QObject::connect(next_action, &QAction::triggered, this, [this]() { emit next_triggered(); });
    QObject::connect(prev_action, &QAction::triggered, this, [this]() { emit previous_triggered(); });
    QObject::connect(show_action, &QAction::triggered, this, [this]() { show(); raise(); activateWindow(); });
    QObject::connect(quit_action, &QAction::triggered, this, [this]() {
        on_app_quit();
        QApplication::quit();
    });
    QObject::connect(tray_icon_, &QSystemTrayIcon::activated, this,
        [this](QSystemTrayIcon::ActivationReason reason) {
            if (reason == QSystemTrayIcon::DoubleClick || reason == QSystemTrayIcon::Trigger) {
                show();
                raise();
                activateWindow();
            }
        });

    tray_icon_->setContextMenu(menu);
    tray_icon_->show();
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

    const std::string filepath = Ffi::on_gui_blocking(
        "get_or_create_thumbnail",
        std::string(),
        [title_copy, variant, hash, filepath_q]() {
            int r = 50 + (static_cast<unsigned char>(hash[0]) % 156);
            int g = 30 + (static_cast<unsigned char>(hash[1]) % 120);
            int b = 70 + (static_cast<unsigned char>(hash[2]) % 140);

            QColor c1(r, g, b);
            QColor c2((r + 60) % 256, (g + 40) % 256, (b + 80) % 256);
            if (QFile::exists(filepath_q)) return Ffi::to_std_string(filepath_q);

            QPixmap px(128, 128);
            px.fill(Qt::transparent);
            QPainter painter(&px);
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
            px.save(filepath_q, "PNG");
            return Ffi::to_std_string(filepath_q);
        });
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
    g_main_window = new DoremiMainWindow();
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
        QPointer<DoremiMainWindow> window_ptr(&window);
        auto apply_fn = [window_ptr, t_mode, a_color]() {
            if (!window_ptr) return;
            bool dark = t_mode != "light";
            DesignTokens::setTheme(dark ? DesignTokens::Theme::Dark : DesignTokens::Theme::Light);
            if (!a_color.empty()) {
                DesignTokens::setAccentColor(Ffi::to_qstring(a_color));
            }
            window_ptr->setStyleSheet(DesignTokens::getGlobalStyleSheet());
            if (window_ptr->title_bar()) window_ptr->title_bar()->update_theme();
            if (window_ptr->nav_sidebar()) window_ptr->nav_sidebar()->update_theme();
            if (window_ptr->player_bar()) window_ptr->player_bar()->update_theme();
            if (window_ptr->home_view()) window_ptr->home_view()->update_theme();
        };
        if (window.isVisible() && window.theme_transition()) {
            window.theme_transition()->start_transition(apply_fn);
        } else {
            apply_fn();
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

static std::vector<Playlist> s_context_playlists;

const std::vector<Playlist> &get_context_playlists() {
    return s_context_playlists;
}

void set_context_playlists(rust::Vec<Playlist> playlists) {
    std::vector<Playlist> p;
    for (const auto &x : playlists) p.push_back(x);
    mutate_main_window("set_context_playlists", [p = std::move(p)](DoremiMainWindow &) mutable {
        s_context_playlists = std::move(p);
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
    const std::string api_secret_copy = Ffi::to_std_string(apiSecret);
    mutate_main_window("set_settings_lastfm_session", [=](DoremiMainWindow &window) {
        if (window.settings_view()) {
            window.settings_view()->set_settings_lastfm_session(
                authenticated, username_copy, api_key_copy, api_secret_copy);
        }
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
    for (const auto &track : queue) tracks.push_back(track);
    mutate_main_window("set_playback_queue", [tracks = std::move(tracks), current_index](DoremiMainWindow &window) {
        rust::Vec<Track> queue_copy;
        for (const auto &track : tracks) queue_copy.push_back(track);
        window.set_playback_queue(queue_copy, current_index);
    });
}

void set_related_tracks(rust::Vec<Track> tracks) {
    std::vector<Track> t;
    for (const auto &tr : tracks) t.push_back(tr);
    mutate_main_window("set_related_tracks", [t = std::move(t)](DoremiMainWindow &window) {
        rust::Vec<Track> tracks_copy;
        for (const auto &tr : t) tracks_copy.push_back(tr);
        window.set_related_tracks(tracks_copy);
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

void set_downloads_list(rust::Vec<rust::String> titles,
                         rust::Vec<rust::String> artists,
                         rust::Vec<rust::String> thumbnails,
                         rust::Vec<rust::String> video_ids,
                         rust::Vec<rust::String> statuses,
                         rust::Vec<double> progresses) {
    std::vector<std::string> t, a, th, v, st;
    std::vector<double> pr;
    for (auto &x : titles) t.push_back(Ffi::to_std_string(x));
    for (auto &x : artists) a.push_back(Ffi::to_std_string(x));
    for (auto &x : thumbnails) th.push_back(Ffi::to_std_string(x));
    for (auto &x : video_ids) v.push_back(Ffi::to_std_string(x));
    for (auto &x : statuses) st.push_back(Ffi::to_std_string(x));
    for (auto &x : progresses) pr.push_back(x);
    mutate_main_window("set_downloads_list", [t = std::move(t), a = std::move(a), th = std::move(th),
                                               v = std::move(v), st = std::move(st), pr = std::move(pr)](DoremiMainWindow &window) {
        if (window.downloads_view()) window.downloads_view()->set_downloads(t, a, th, v, st, pr);
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
    for (const auto &t : history) h.push_back(t);
    std::vector<std::string> pa;
    for (const auto &x : played_at) pa.push_back(Ffi::to_std_string(x));
    std::vector<std::string> ft;
    for (const auto &y : feedback_tokens) ft.push_back(Ffi::to_std_string(y));
    mutate_main_window("set_history_data", [h = std::move(h), pa = std::move(pa), ft = std::move(ft)](DoremiMainWindow &window) {
        rust::Vec<Track> r_history;
        for (const auto &t : h) r_history.push_back(t);
        rust::Vec<rust::String> r_played_at;
        for (const auto &x : pa) r_played_at.push_back(x);
        rust::Vec<rust::String> r_feedback_tokens;
        for (const auto &y : ft) r_feedback_tokens.push_back(y);
        window.set_history_data(r_history, r_played_at, r_feedback_tokens);
    });
}

// ── Album Detail ──

void set_album_detail(Album album, rust::Vec<Track> tracks) {
    std::vector<Track> tt;
    for (const auto &x : tracks) tt.push_back(x);
    mutate_main_window("set_album_detail", [album = std::move(album), tt = std::move(tt)](DoremiMainWindow &window) {
        if (!window.album_detail_view()) return;
        std::vector<Track> r_tracks;
        for (const auto &t : tt) r_tracks.push_back(t);
        window.album_detail_view()->set_album_info(album);
        window.album_detail_view()->set_album_tracks(r_tracks);
        window.navigate_to("album_detail");
    });
}

// ── Artist Detail ──

void set_artist_detail(Artist artist, rust::Vec<Track> tracks, rust::Vec<Album> albums) {
    std::vector<Track> tt;
    for (const auto &x : tracks) tt.push_back(x);
    std::vector<Album> al;
    for (const auto &x : albums) al.push_back(x);
    mutate_main_window("set_artist_detail", [artist = std::move(artist), tt = std::move(tt), al = std::move(al)](DoremiMainWindow &window) {
        if (!window.artist_detail_view()) return;
        std::vector<Track> r_tracks;
        for (const auto &t : tt) r_tracks.push_back(t);
        std::vector<Album> r_albums;
        for (const auto &a : al) r_albums.push_back(a);
        window.artist_detail_view()->set_artist_info(artist);
        window.artist_detail_view()->set_artist_tracks(r_tracks, r_albums);
        window.navigate_to("artist_detail");
    });
}

// ── Playlist Detail ──

void set_playlist_detail(Playlist playlist, rust::Vec<Track> tracks) {
    std::vector<Track> tt;
    for (const auto &x : tracks) tt.push_back(x);
    mutate_main_window("set_playlist_detail", [playlist = std::move(playlist), tt = std::move(tt)](DoremiMainWindow &window) {
        if (!window.playlist_detail_view()) return;
        std::vector<Track> r_tracks;
        for (const auto &t : tt) r_tracks.push_back(t);
        window.playlist_detail_view()->set_playlist_info(playlist);
        window.playlist_detail_view()->set_playlist_tracks(r_tracks);
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
