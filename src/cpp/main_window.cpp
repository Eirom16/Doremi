#include <QApplication>
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
#include <QScrollBar>
#include <QFile>
#include <QCryptographicHash>
#include <QStandardPaths>
#include <QDir>

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
#include "welcome_view.h"
#include "components/theme_transition.h"
#include "doremi/src/bridge.rs.h"



DoremiMainWindow *g_main_window = nullptr;

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
    stack_->addWidget(welcome_view_);           // idx 12


    stack_->setCurrentIndex(1); // start at home
    body->addWidget(stack_, 1);

    root->addLayout(body, 1);

    player_bar_ = new PlayerBar(central);
    root->addWidget(player_bar_);

    setCentralWidget(central);

    now_playing_view_ = new NowPlayingView(this);
    now_playing_view_->hide();

    theme_transition_ = new ThemeTransitionOverlay(this);
    theme_transition_->hide();


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
    setup_shortcuts();
    setup_tray();
    connect_signals();

    player_timer_ = new QTimer(this);
    player_timer_->setInterval(250);
    QObject::connect(player_timer_, &QTimer::timeout, this, []() { on_timer_tick(); });
    player_timer_->start();
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
        search_view_->set_query(q);
        stack_->setCurrentIndex(2);
        on_search_submitted(q);
    });


    QObject::connect(nav_sidebar_, &NavSidebar::route_changed, this, [this](const std::string &r) {
        navigate_to(r);
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

    QObject::connect(this, &DoremiMainWindow::shuffle_toggled, this,
        [](bool on) { on_shuffle_toggled(on); });
    QObject::connect(this, &DoremiMainWindow::repeat_cycled, this,
        []() { on_repeat_cycled(); });

    QObject::connect(settings_view_, &SettingsView::setting_changed, this,
        [](const std::string &key, const std::string &value) {
            on_setting_changed(key, value);
        });

    QObject::connect(settings_view_, &SettingsView::lastfm_auth_requested, this,
        [](const std::string &apiKey, const std::string &apiSecret, const std::string &username, const std::string &password) {
            on_lastfm_auth_requested(apiKey, apiSecret, username, password);
        });

    QObject::connect(settings_view_, &SettingsView::lastfm_disconnect_requested, this,
        []() {
            on_lastfm_disconnect_requested();
        });

    QObject::connect(search_view_, &SearchView::play_requested, this,
        [](const std::string &info) {
            on_search_item_clicked(info);
        });
    QObject::connect(search_view_, &SearchView::search_requested, this,
        [](const std::string &query) {
            on_search_submitted(query);
        });
    QObject::connect(search_view_, &SearchView::add_favorite_requested, this,
        [](const std::string &info) {
            on_add_favorite(info);
        });
    QObject::connect(search_view_, &SearchView::download_requested, this,
        [](const std::string &info) {
            on_download_requested(info);
        });

    QObject::connect(library_view_, &LibraryView::tab_changed, this,
        [](const std::string &tab) {
            on_library_tab_changed(tab);
        });
    QObject::connect(library_view_, &LibraryView::play_requested, this,
        [](const std::string &info) {
            on_search_item_clicked(info);
        });
    QObject::connect(library_view_, &LibraryView::remove_favorite_requested, this,
        [](const std::string &info) {
            on_remove_favorite(info);
        });
    QObject::connect(library_view_, &LibraryView::download_requested, this,
        [](const std::string &info) {
            on_download_requested(info);
        });

    QObject::connect(trending_view_, &TrendingView::play_requested, this,
        [](const std::string &info) {
            on_search_item_clicked(info);
        });

    QObject::connect(downloads_view_, &DownloadsView::play_requested, this,
        [](const std::string &info) {
            on_search_item_clicked(info);
        });

    QObject::connect(stats_view_, &StatsView::play_requested, this,
        [](const std::string &info) {
            on_search_item_clicked(info);
        });

    // History view
    QObject::connect(history_view_, &HistoryView::play_requested, this,
        [](const std::string &info) {
            on_search_item_clicked(info);
        });

    // Album detail view
    QObject::connect(album_detail_view_, &AlbumDetailView::play_requested, this,
        [](const std::string &info) {
            on_search_item_clicked(info);
        });
    QObject::connect(album_detail_view_, &AlbumDetailView::back_requested, this,
        [this]() {
            navigate_to("library");
        });

    // Artist detail view
    QObject::connect(artist_detail_view_, &ArtistDetailView::play_requested, this,
        [](const std::string &info) {
            on_search_item_clicked(info);
        });
    QObject::connect(artist_detail_view_, &ArtistDetailView::back_requested, this,
        [this]() {
            navigate_to("library");
        });

    // Playlist detail view
    QObject::connect(playlist_detail_view_, &PlaylistDetailView::play_requested, this,
        [](const std::string &info) {
            on_search_item_clicked(info);
        });
    QObject::connect(playlist_detail_view_, &PlaylistDetailView::back_requested, this,
        [this]() {
            navigate_to("library");
        });
}


void DoremiMainWindow::closeEvent(QCloseEvent *event) {
    if (tray_icon_ && tray_icon_->isVisible()) {
        hide();
        event->ignore();
    } else {
        emit window_closed();
        event->accept();
    }
}

void DoremiMainWindow::navigate_to(const std::string &route) {
    nav_sidebar_->set_active_route(route);
    if (route == "home") {
        stack_->setCurrentIndex(1);
        auto *sa = home_view_->findChild<QScrollArea *>();
        if (sa) sa->verticalScrollBar()->setValue(0);
    } else if (route == "search") {
        stack_->setCurrentIndex(2);
        auto *sa = search_view_->findChild<QScrollArea *>();
        if (sa) sa->verticalScrollBar()->setValue(0);
    } else if (route == "library") {
        stack_->setCurrentIndex(3);
        auto *sa = library_view_->findChild<QScrollArea *>();
        if (sa) sa->verticalScrollBar()->setValue(0);
    } else if (route == "settings") {
        stack_->setCurrentIndex(4);
    } else if (route == "trending") {
        stack_->setCurrentIndex(5);
        auto *sa = trending_view_->findChild<QScrollArea *>();
        if (sa) sa->verticalScrollBar()->setValue(0);
    } else if (route == "downloads") {
        stack_->setCurrentIndex(6);
        auto *sa = downloads_view_->findChild<QScrollArea *>();
        if (sa) sa->verticalScrollBar()->setValue(0);
    } else if (route == "stats") {
        stack_->setCurrentIndex(7);
        auto *sa = stats_view_->findChild<QScrollArea *>();
        if (sa) sa->verticalScrollBar()->setValue(0);
    } else if (route == "history") {
        stack_->setCurrentIndex(8);
        auto *sa = history_view_->findChild<QScrollArea *>();
        if (sa) sa->verticalScrollBar()->setValue(0);
    } else if (route == "album_detail") {
        stack_->setCurrentIndex(9);
        auto *sa = album_detail_view_->findChild<QScrollArea *>();
        if (sa) sa->verticalScrollBar()->setValue(0);
    } else if (route == "artist_detail") {
        stack_->setCurrentIndex(10);
        auto *sa = artist_detail_view_->findChild<QScrollArea *>();
        if (sa) sa->verticalScrollBar()->setValue(0);
    } else if (route == "playlist_detail") {
        stack_->setCurrentIndex(11);
        auto *sa = playlist_detail_view_->findChild<QScrollArea *>();
        if (sa) sa->verticalScrollBar()->setValue(0);
    } else if (route == "welcome") {
        stack_->setCurrentIndex(12);
        auto *sa = welcome_view_->findChild<QScrollArea *>();
        if (sa) sa->verticalScrollBar()->setValue(0);
    } else {
        stack_->setCurrentIndex(1);
    }

}

void DoremiMainWindow::show_notif(const std::string &message, const std::string &kind) {
    if (tray_icon_ && tray_icon_->isVisible() && tray_icon_->supportsMessages()) {
        tray_icon_->showMessage(
            kind == "error" ? "Doremi — Error" : "Doremi",
            QString::fromStdString(message),
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
    if (!title.empty() && (title != g_last_track_title || artist != g_last_track_artist)) {
        g_last_track_title = title;
        g_last_track_artist = artist;
        show_notif("Reproduciendo: " + title + " — " + artist, "info");
    }
}

void DoremiMainWindow::set_playback_playing(bool playing) {
    player_bar_->set_playing(playing);
    if (now_playing_view_) {
        now_playing_view_->setPlaying(playing);
    }
}

void DoremiMainWindow::resizeEvent(QResizeEvent *event) {
    QMainWindow::resizeEvent(event);
    if (now_playing_view_) {
        now_playing_view_->setGeometry(rect());
    }
    if (theme_transition_) {
        theme_transition_->setGeometry(rect());
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

void DoremiMainWindow::set_playback_queue(const std::vector<std::string> &titles,
                                          const std::vector<std::string> &artists,
                                          const std::vector<std::string> &thumbnails,
                                          int32_t current_index) {
    if (now_playing_view_) {
        QStringList t_list, a_list, th_list;
        for (const auto &t : titles) t_list << QString::fromStdString(t);
        for (const auto &a : artists) a_list << QString::fromStdString(a);
        for (const auto &th : thumbnails) th_list << QString::fromStdString(th);
        now_playing_view_->setQueue(t_list, a_list, th_list, current_index);
    }
}

void DoremiMainWindow::set_stats_data(const std::string &total_play_time, int32_t total_plays, int32_t unique_artists,
                                      const std::vector<int32_t> &weekly_activity, const std::vector<std::string> &top_titles,
                                      const std::vector<std::string> &top_artists, const std::vector<int32_t> &top_plays,
                                      const std::vector<std::string> &top_thumbnails) {
    if (stats_view_) {
        QString time_str = QString::fromStdString(total_play_time);
        
        QList<int> weekly_activity_list;
        for (int v : weekly_activity) weekly_activity_list.append(v);
        
        QStringList t_list, a_list, th_list;
        for (const auto &t : top_titles) t_list << QString::fromStdString(t);
        for (const auto &a : top_artists) a_list << QString::fromStdString(a);
        for (const auto &th : top_thumbnails) th_list << QString::fromStdString(th);
        
        QList<int> plays_list;
        for (int v : top_plays) plays_list.append(v);

        stats_view_->setStatsData(time_str, total_plays, unique_artists,
                                  weekly_activity_list, t_list, a_list,
                                  plays_list, th_list);
    }
}


void DoremiMainWindow::set_history_data(const std::vector<std::string> &titles,
                                        const std::vector<std::string> &artists,
                                        const std::vector<std::string> &durations,
                                        const std::vector<std::string> &thumbnails,
                                        const std::vector<std::string> &played_at,
                                        const std::vector<std::string> &item_ids)
{
    if (history_view_) {
        history_view_->set_history(titles, artists, durations, thumbnails, played_at, item_ids);
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
    auto *play_action = menu->addAction("▶ Reproducir/Pausa");
    auto *next_action = menu->addAction("⏭ Siguiente");
    auto *prev_action = menu->addAction("⏮ Anterior");
    menu->addSeparator();
    auto *show_action = menu->addAction("Mostrar ventana");
    auto *quit_action = menu->addAction("Salir");

    QObject::connect(play_action, &QAction::triggered, this, [this]() { emit play_pause_triggered(); });
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
    std::string t = std::string(title);
    QByteArray hash = QCryptographicHash::hash(
        QByteArray::fromStdString(t + std::to_string(variant)),
        QCryptographicHash::Md5
    );
    int r = 50 + (static_cast<unsigned char>(hash[0]) % 156);
    int g = 30 + (static_cast<unsigned char>(hash[1]) % 120);
    int b = 70 + (static_cast<unsigned char>(hash[2]) % 140);

    QColor c1(r, g, b);
    QColor c2((r + 60) % 256, (g + 40) % 256, (b + 80) % 256);

    QString cache_dir = QStandardPaths::writableLocation(QStandardPaths::CacheLocation);
    QString thumb_dir = cache_dir + "/artwork";
    QDir().mkpath(thumb_dir);

    QString filename = QString("placeholder_%1_%2.png")
        .arg(QString(hash.toHex().left(12)))
        .arg(variant);
    QString filepath = thumb_dir + "/" + filename;

    if (QFile::exists(filepath))
        return rust::String(filepath.toStdString());

    QPixmap px(128, 128);
    px.fill(Qt::transparent);
    QPainter p(&px);
    p.setRenderHint(QPainter::Antialiasing);

    QLinearGradient grad(0, 0, 128, 128);
    grad.setColorAt(0.0, c1);
    grad.setColorAt(1.0, c2);
    p.setBrush(grad);
    p.setPen(Qt::NoPen);
    p.drawRoundedRect(0, 0, 128, 128, 12, 12);

    QChar first = QString::fromStdString(t).at(0).toUpper();
    p.setPen(QPen(QColor(255, 255, 255, 200), 2));
    QFont f = p.font();
    f.setPixelSize(52);
    f.setBold(true);
    p.setFont(f);
    p.drawText(QRect(0, 0, 128, 128), Qt::AlignCenter, QString(first));

    p.end();
    px.save(filepath, "PNG");
    return rust::String(filepath.toStdString());
}

// ── Bridge functions ──────────────────────────────────────

void create_main_window(rust::Str, rust::Str, rust::Str, int32_t) {
    if (!QApplication::instance()) {
        static const char *argv[] = {"doremi", nullptr};
        static int argc = 1;
        new QApplication(argc, const_cast<char **>(argv));
    }
    g_main_window = new DoremiMainWindow();
}

void show_main_window() {
    if (g_main_window) g_main_window->show();
}

void navigate_to(rust::Str route) {
    if (g_main_window) g_main_window->navigate_to(std::string(route));
}

void show_notification(rust::Str message, rust::Str kind) {
    if (g_main_window) g_main_window->show_notif(std::string(message), std::string(kind));
}

void apply_theme(rust::Str theme_mode, rust::Str accent_color) {
    if (!g_main_window) return;
    
    std::string t_mode = std::string(theme_mode);
    std::string a_color = std::string(accent_color);
    
    auto apply_fn = [t_mode, a_color]() {
        bool dark = t_mode != "light";
        DesignTokens::setTheme(dark ? DesignTokens::Theme::Dark : DesignTokens::Theme::Light);
        if (!a_color.empty()) {
            DesignTokens::setAccentColor(QString::fromStdString(a_color));
        }
        
        g_main_window->setStyleSheet(DesignTokens::getGlobalStyleSheet());
        
        if (g_main_window->title_bar()) g_main_window->title_bar()->update_theme();
        if (g_main_window->nav_sidebar()) g_main_window->nav_sidebar()->update_theme();
        if (g_main_window->player_bar()) g_main_window->player_bar()->update_theme();
    };

    if (g_main_window->isVisible() && g_main_window->theme_transition()) {
        g_main_window->theme_transition()->start_transition(apply_fn);
    } else {
        apply_fn();
    }
}

void update_player_state(int32_t state, int32_t pos, int32_t dur) {
    if (g_main_window) g_main_window->update_player_state(state, pos, dur);
}

void set_mini_player(rust::Str title, rust::Str artist, rust::Str thumb) {
    if (g_main_window) g_main_window->set_mini_player_info(std::string(title), std::string(artist), std::string(thumb));
}

rust::String get_search_bar_text() {
    if (g_main_window && g_main_window->title_bar())
        return rust::String(g_main_window->title_bar()->search_text());
    return rust::String("");
}

void set_search_bar_text(rust::Str text) {
    if (g_main_window && g_main_window->title_bar())
        g_main_window->title_bar()->set_search_text(std::string(text));
}

void set_window_title(rust::Str title) {
    if (g_main_window) g_main_window->setWindowTitle(QString::fromStdString(std::string(title)));
}

void set_playing(rust::Str playing) {
    if (g_main_window) {
        bool is_playing = std::string(playing) == "true";
        g_main_window->set_playback_playing(is_playing);
    }
}

void set_player_volume(int32_t volume) {
    if (g_main_window && g_main_window->player_bar())
        g_main_window->player_bar()->set_volume_value(volume);
}

void run_event_loop() {
    if (auto *app = QApplication::instance())
        app->exec();
}

void set_search_history(rust::Vec<rust::String> queries) {
    if (!g_main_window || !g_main_window->search_view()) return;
    std::vector<std::string> q;
    for (auto &x : queries) q.push_back(std::string(x));
    g_main_window->search_view()->set_recent_searches(q);
}

void set_search_results(rust::Vec<rust::String> songs, rust::Vec<rust::String> artists, rust::Vec<rust::String> albums) {
    if (!g_main_window || !g_main_window->search_view()) return;
    std::vector<std::string> s, a, al;
    for (auto &x : songs) s.push_back(std::string(x));
    for (auto &x : artists) a.push_back(std::string(x));
    for (auto &x : albums) al.push_back(std::string(x));
    g_main_window->search_view()->set_results(s, a, al);
}

void add_home_section(rust::Str title, rust::Vec<rust::String> items) {
    if (!g_main_window || !g_main_window->home_view()) return;
    std::vector<std::string> v;
    for (auto &x : items) v.push_back(std::string(x));
    g_main_window->home_view()->add_section(std::string(title), v);
}

void clear_home_sections() {
    if (!g_main_window || !g_main_window->home_view()) return;
    g_main_window->home_view()->clear_sections();
}

void set_library_albums(rust::Vec<rust::String> titles,
                         rust::Vec<rust::String> artists) {
    if (!g_main_window || !g_main_window->library_view()) return;
    std::vector<std::string> t, a;
    for (auto &x : titles) t.push_back(std::string(x));
    for (auto &x : artists) a.push_back(std::string(x));
    g_main_window->library_view()->set_albums(t, a);
}

void set_library_artists(rust::Vec<rust::String> names) {
    if (!g_main_window || !g_main_window->library_view()) return;
    std::vector<std::string> n;
    for (auto &x : names) n.push_back(std::string(x));
    g_main_window->library_view()->set_artists(n);
}

void set_library_songs(rust::Vec<rust::String> titles) {
    if (!g_main_window || !g_main_window->library_view()) return;
    std::vector<std::string> v;
    for (auto &x : titles) v.push_back(std::string(x));
    g_main_window->library_view()->set_songs(v);
}

void set_library_playlists(rust::Vec<rust::String> names, rust::Vec<rust::String> counts) {
    if (!g_main_window || !g_main_window->library_view()) return;
    std::vector<std::string> n, c;
    for (auto &x : names) n.push_back(std::string(x));
    for (auto &x : counts) c.push_back(std::string(x));
    g_main_window->library_view()->set_playlists(n, c);
}

void apply_settings_to_ui() {
    // Implemented in Rust — reads AppSettings and calls individual setters
}

void set_settings_theme(rust::Str mode) {
    if (g_main_window && g_main_window->settings_view())
        g_main_window->settings_view()->set_theme(std::string(mode));
}

void set_settings_accent(rust::Str color) {
    if (g_main_window && g_main_window->settings_view())
        g_main_window->settings_view()->set_accent(std::string(color));
}

void set_settings_font_size(int32_t size) {
    if (g_main_window && g_main_window->settings_view())
        g_main_window->settings_view()->set_font_size(size);
}

void set_settings_language(rust::Str lang) {
    if (g_main_window && g_main_window->settings_view())
        g_main_window->settings_view()->set_language(std::string(lang));
}

void set_settings_normalize(bool on) {
    if (g_main_window && g_main_window->settings_view())
        g_main_window->settings_view()->set_normalize(on);
}

void set_settings_crossfade(bool on) {
    if (g_main_window && g_main_window->settings_view())
        g_main_window->settings_view()->set_crossfade(on);
}

void set_settings_equalizer_enabled(bool on) {
    if (g_main_window && g_main_window->settings_view())
        g_main_window->settings_view()->set_equalizer_enabled(on);
}

void set_settings_equalizer_preset(rust::Str preset) {
    if (g_main_window && g_main_window->settings_view())
        g_main_window->settings_view()->set_equalizer_preset(std::string(preset));
}

void set_settings_sleep_timer(int32_t minutes) {
    if (g_main_window && g_main_window->settings_view())
        g_main_window->settings_view()->set_sleep_timer(minutes);
}

void set_settings_discord_rpc(bool on) {
    if (g_main_window && g_main_window->settings_view())
        g_main_window->settings_view()->set_settings_discord_rpc(on);
}

void set_settings_lastfm_enabled(bool on) {
    if (g_main_window && g_main_window->settings_view())
        g_main_window->settings_view()->set_settings_lastfm_enabled(on);
}

void set_settings_lastfm_session(bool authenticated, rust::Str username, rust::Str apiKey, rust::Str apiSecret) {
    if (g_main_window && g_main_window->settings_view()) {
        g_main_window->settings_view()->set_settings_lastfm_session(
            authenticated,
            std::string(username),
            std::string(apiKey),
            std::string(apiSecret)
        );
    }
}

void set_track_lyrics(rust::Str plain, rust::Str synced) {
    if (g_main_window) {
        std::string p_str = std::string(plain);
        std::string s_str = std::string(synced);
        QMetaObject::invokeMethod(g_main_window, [=]() {
            g_main_window->set_track_lyrics(p_str, s_str);
        }, Qt::QueuedConnection);
    }
}

void set_settings_subtitle_alignment(rust::Str align) {
    if (g_main_window) {
        std::string a_str = std::string(align);
        if (g_main_window->settings_view())
            g_main_window->settings_view()->set_subtitle_alignment(a_str);
        if (g_main_window->now_playing_view()) {
            QMetaObject::invokeMethod(g_main_window->now_playing_view(), [=]() {
                g_main_window->now_playing_view()->setSubtitleAlignment(a_str);
            }, Qt::QueuedConnection);
        }
    }
}

void set_settings_subtitle_font_size(int32_t size) {
    if (g_main_window) {
        if (g_main_window->settings_view())
            g_main_window->settings_view()->set_subtitle_font_size(size);
        if (g_main_window->now_playing_view()) {
            QMetaObject::invokeMethod(g_main_window->now_playing_view(), [=]() {
                g_main_window->now_playing_view()->setSubtitleFontSize(size);
            }, Qt::QueuedConnection);
        }
    }
}

void set_settings_subtitle_line_spacing(double spacing) {
    if (g_main_window) {
        if (g_main_window->settings_view())
            g_main_window->settings_view()->set_subtitle_line_spacing(spacing);
        if (g_main_window->now_playing_view()) {
            QMetaObject::invokeMethod(g_main_window->now_playing_view(), [=]() {
                g_main_window->now_playing_view()->setSubtitleLineSpacing(spacing);
            }, Qt::QueuedConnection);
        }
    }
}

void set_settings_subtitle_auto_scroll(bool on) {
    if (g_main_window) {
        if (g_main_window->settings_view())
            g_main_window->settings_view()->set_subtitle_auto_scroll(on);
        if (g_main_window->now_playing_view()) {
            QMetaObject::invokeMethod(g_main_window->now_playing_view(), [=]() {
                g_main_window->now_playing_view()->setSubtitleAutoScroll(on);
            }, Qt::QueuedConnection);
        }
    }
}

void set_settings_subtitle_glow_effect(bool on) {
    if (g_main_window) {
        if (g_main_window->settings_view())
            g_main_window->settings_view()->set_subtitle_glow_effect(on);
        if (g_main_window->now_playing_view()) {
            QMetaObject::invokeMethod(g_main_window->now_playing_view(), [=]() {
                g_main_window->now_playing_view()->setSubtitleGlowEffect(on);
            }, Qt::QueuedConnection);
        }
    }
}



void set_dominant_colors(rust::Vec<rust::String> colors) {
    if (g_main_window) {
        std::vector<std::string> c_vec;
        for (const auto &c : colors) c_vec.push_back(std::string(c));
        QMetaObject::invokeMethod(g_main_window, [=]() {
            g_main_window->set_dominant_colors(c_vec);
        }, Qt::QueuedConnection);
    }
}

void set_playback_queue(rust::Vec<rust::String> titles,
                        rust::Vec<rust::String> artists,
                        rust::Vec<rust::String> thumbnails,
                        int32_t current_index) {
    if (g_main_window) {
        std::vector<std::string> t_vec;
        std::vector<std::string> a_vec;
        std::vector<std::string> th_vec;
        for (const auto &t : titles) t_vec.push_back(std::string(t));
        for (const auto &a : artists) a_vec.push_back(std::string(a));
        for (const auto &th : thumbnails) th_vec.push_back(std::string(th));
        QMetaObject::invokeMethod(g_main_window, [=]() {
            g_main_window->set_playback_queue(t_vec, a_vec, th_vec, current_index);
        }, Qt::QueuedConnection);
    }
}

void set_stats_data(rust::Str total_play_time, int32_t total_plays, int32_t unique_artists,
                    rust::Vec<int32_t> weekly_activity, rust::Vec<rust::String> top_titles,
                    rust::Vec<rust::String> top_artists, rust::Vec<int32_t> top_plays,
                    rust::Vec<rust::String> top_thumbnails) {
    if (g_main_window) {
        std::string t_time = std::string(total_play_time);
        std::vector<int32_t> w_act;
        for (int32_t v : weekly_activity) w_act.push_back(v);
        
        std::vector<std::string> t_titles, t_artists, t_thumbs;
        for (const auto &t : top_titles) t_titles.push_back(std::string(t));
        for (const auto &a : top_artists) t_artists.push_back(std::string(a));
        for (const auto &th : top_thumbnails) t_thumbs.push_back(std::string(th));
        
        std::vector<int32_t> t_plays;
        for (int32_t p : top_plays) t_plays.push_back(p);
        
        QMetaObject::invokeMethod(g_main_window, [=]() {
            g_main_window->set_stats_data(t_time, total_plays, unique_artists,
                                          w_act, t_titles, t_artists, t_plays, t_thumbs);
        }, Qt::QueuedConnection);
    }
}



// ── Player state sync ──

void set_player_shuffle(bool on) {
    if (g_main_window) {
        if (g_main_window->player_bar()) g_main_window->player_bar()->set_shuffle(on);
        if (g_main_window->now_playing_view()) g_main_window->now_playing_view()->setShuffle(on);
    }
}

void set_player_repeat(int32_t mode) {
    if (g_main_window) {
        if (g_main_window->player_bar()) g_main_window->player_bar()->set_repeat_mode(mode);
        if (g_main_window->now_playing_view()) g_main_window->now_playing_view()->setRepeatMode(mode);
    }
}


// ── Trending ──

void set_trending_items(rust::Vec<rust::String> titles,
                         rust::Vec<rust::String> subtitles,
                         rust::Vec<rust::String> thumbnails) {
    if (!g_main_window || !g_main_window->trending_view()) return;
    g_main_window->trending_view()->clear_items();
    std::vector<std::string> t, s, th;
    for (auto &x : titles) t.push_back(std::string(x));
    for (auto &x : subtitles) s.push_back(std::string(x));
    for (auto &x : thumbnails) th.push_back(std::string(x));
    size_t n = std::min({t.size(), s.size(), th.size()});
    for (size_t i = 0; i < n; ++i)
        g_main_window->trending_view()->add_item(t[i], s[i], th[i]);
}

// ── Downloads ──

void set_downloads_list(rust::Vec<rust::String> titles,
                         rust::Vec<rust::String> artists,
                         rust::Vec<rust::String> thumbnails) {
    if (!g_main_window || !g_main_window->downloads_view()) return;
    std::vector<std::string> t, a, th;
    for (auto &x : titles) t.push_back(std::string(x));
    for (auto &x : artists) a.push_back(std::string(x));
    for (auto &x : thumbnails) th.push_back(std::string(x));
    g_main_window->downloads_view()->set_downloads(t, a, th);
}

// ── History ──

void set_history_data(rust::Vec<rust::String> titles, rust::Vec<rust::String> artists,
                      rust::Vec<rust::String> durations, rust::Vec<rust::String> thumbnails,
                      rust::Vec<rust::String> played_at, rust::Vec<rust::String> item_ids) {
    if (!g_main_window || !g_main_window->history_view()) return;
    std::vector<std::string> t, a, d, th, pa, ids;
    for (auto &x : titles) t.push_back(std::string(x));
    for (auto &x : artists) a.push_back(std::string(x));
    for (auto &x : durations) d.push_back(std::string(x));
    for (auto &x : thumbnails) th.push_back(std::string(x));
    for (auto &x : played_at) pa.push_back(std::string(x));
    for (auto &x : item_ids) ids.push_back(std::string(x));
    g_main_window->set_history_data(t, a, d, th, pa, ids);
}

// ── Album Detail ──

void set_album_detail(rust::Str title, rust::Str artist, rust::Str year,
                      rust::Str thumbnail, int32_t track_count,
                      rust::Vec<rust::String> track_titles, rust::Vec<rust::String> track_artists,
                      rust::Vec<rust::String> track_durations, rust::Vec<rust::String> track_ids) {
    if (!g_main_window || !g_main_window->album_detail_view()) return;
    std::vector<std::string> tt, ta, td, ti;
    for (auto &x : track_titles) tt.push_back(std::string(x));
    for (auto &x : track_artists) ta.push_back(std::string(x));
    for (auto &x : track_durations) td.push_back(std::string(x));
    for (auto &x : track_ids) ti.push_back(std::string(x));
    g_main_window->album_detail_view()->set_album_info(
        std::string(title), std::string(artist), std::string(year),
        std::string(thumbnail), track_count);
    g_main_window->album_detail_view()->set_album_tracks(tt, ta, td, ti);
    g_main_window->navigate_to("album_detail");
}

// ── Artist Detail ──

void set_artist_detail(rust::Str name, rust::Str thumbnail, rust::Str subscriber_count,
                       rust::Str description,
                       rust::Vec<rust::String> track_titles, rust::Vec<rust::String> track_albums,
                       rust::Vec<rust::String> track_durations, rust::Vec<rust::String> track_ids) {
    if (!g_main_window || !g_main_window->artist_detail_view()) return;
    std::vector<std::string> tt, ta, td, ti;
    for (auto &x : track_titles) tt.push_back(std::string(x));
    for (auto &x : track_albums) ta.push_back(std::string(x));
    for (auto &x : track_durations) td.push_back(std::string(x));
    for (auto &x : track_ids) ti.push_back(std::string(x));
    g_main_window->artist_detail_view()->set_artist_info(
        std::string(name), std::string(thumbnail),
        std::string(subscriber_count), std::string(description));
    g_main_window->artist_detail_view()->set_artist_tracks(tt, ta, td, ti);
    g_main_window->navigate_to("artist_detail");
}

// ── Playlist Detail ──

void set_playlist_detail(rust::Str name, rust::Str description, rust::Str thumbnail,
                         int32_t track_count,
                         rust::Vec<rust::String> track_titles, rust::Vec<rust::String> track_artists,
                         rust::Vec<rust::String> track_durations, rust::Vec<rust::String> track_ids) {
    if (!g_main_window || !g_main_window->playlist_detail_view()) return;
    std::vector<std::string> tt, ta, td, ti;
    for (auto &x : track_titles) tt.push_back(std::string(x));
    for (auto &x : track_artists) ta.push_back(std::string(x));
    for (auto &x : track_durations) td.push_back(std::string(x));
    for (auto &x : track_ids) ti.push_back(std::string(x));
    g_main_window->playlist_detail_view()->set_playlist_info(
        std::string(name), std::string(description),
        std::string(thumbnail), track_count);
    g_main_window->playlist_detail_view()->set_playlist_tracks(tt, ta, td, ti);
    g_main_window->navigate_to("playlist_detail");
}

void update_youtube_auth_state(bool authenticated, rust::Str name, rust::Str avatar_url) {
    if (!g_main_window) return;
    if (g_main_window->nav_sidebar()) {
        g_main_window->nav_sidebar()->update_profile(authenticated, std::string(name), std::string(avatar_url));
    }
    if (g_main_window->welcome_view()) {
        g_main_window->welcome_view()->update_theme();
    }
}
