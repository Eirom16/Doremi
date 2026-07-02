// Signal connections — extracted from DoremiMainWindow::connect_signals()
// to reduce main_window.cpp size and improve maintainability.

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
#include "now_playing_view.h"
#include "stats_view.h"
#include "history_view.h"
#include "album_detail_view.h"
#include "artist_detail_view.h"
#include "playlist_detail_view.h"
#include "show_detail_view.h"
#include "login_dialog.h"
#include "bridge_helpers.h"
#include "doremi/src/bridge.rs.h"

// Helper: convert std::vector<Track> to rust::Vec<Track> for bridge calls.
static rust::Vec<Track> to_rust_vec(const std::vector<Track> &src) {
    rust::Vec<Track> v;
    v.reserve(src.size());
    for (const auto &item : src) v.push_back(item);
    return v;
}

void DoremiMainWindow::connect_signals() {
    // ── Core playback signals ──────────────────────────────────────────
    QObject::connect(this, &DoremiMainWindow::play_pause_triggered, this, []() { on_play_pause_triggered(); });
    QObject::connect(this, &DoremiMainWindow::next_triggered, this, []() { on_next_triggered(); });
    QObject::connect(this, &DoremiMainWindow::previous_triggered, this, []() { on_previous_triggered(); });
    QObject::connect(this, &DoremiMainWindow::volume_set, this, [](int32_t v) { on_volume_set(v); });
    QObject::connect(this, &DoremiMainWindow::window_closed, this, []() { on_window_close_requested(); });
    QObject::connect(this, &DoremiMainWindow::shuffle_toggled, this,
        [](bool on) { on_shuffle_toggled(on); });
    QObject::connect(this, &DoremiMainWindow::repeat_cycled, this,
        []() { on_repeat_cycled(); });

    // ── Title bar (search) ─────────────────────────────────────────────
    guardOnline(title_bar_, &TitleBar::search_submitted, "buscar en YouTube Music", [this](const std::string &q) {
        search_view_->set_query(q);
        stack_->setCurrentIndex(2);
        on_search_submitted(q, "all");
    });
    QObject::connect(title_bar_, &TitleBar::search_text_changed, this, [this](const std::string &q) {
        if (!is_online_) return;
        on_search_suggestions_requested(q);
    });

    // ── Navigation sidebar ─────────────────────────────────────────────
    QObject::connect(nav_sidebar_, &NavSidebar::route_changed, this, [this](const std::string &r) {
        navigate_to(r);
        if (r == "search") on_search_history_requested();
    });

    // ── Player bar ─────────────────────────────────────────────────────
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

    // ── Now playing view ───────────────────────────────────────────────
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

    // ── Settings view ──────────────────────────────────────────────────
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

    // ── Search view ────────────────────────────────────────────────────
    QObject::connect(search_view_, &SearchView::play_requested, this,
        [](Track track) { on_search_item_clicked(track); });
    guardOnline(search_view_, &SearchView::search_requested, "buscar en YouTube Music", [this](const std::string &query, const std::string &filter) {
        title_bar_->set_search_text(query);
        search_view_->set_query(query, filter);
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
        [](Track track) { on_add_favorite(track); });
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

    // ── Home view ──────────────────────────────────────────────────────
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

    // ── Trending view ──────────────────────────────────────────────────
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

    // ── Library view ───────────────────────────────────────────────────
    QObject::connect(library_view_, &LibraryView::tab_changed, this,
        [](const std::string &tab) { on_library_tab_changed(tab); });
    QObject::connect(library_view_, &LibraryView::search_requested, this,
        [](const std::string &tab, const std::string &query, const std::string &sort_by) {
            on_library_search(tab, query, sort_by);
        });
    QObject::connect(library_view_, &LibraryView::filter_source_changed, this,
        [](int source) { on_library_set_filter_source(source); });
    QObject::connect(library_view_, &LibraryView::play_requested, this,
        [](Track track) { on_search_item_clicked(track); });
    QObject::connect(library_view_, &LibraryView::remove_favorite_requested, this,
        [](const std::string &info) { on_remove_favorite(info); });
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
        [](const std::string &id) { on_show_requested(id); });
    QObject::connect(library_view_, &LibraryView::playlist_requested, this,
        [](const std::string &id) { on_playlist_requested(id); });
    QObject::connect(library_view_, &LibraryView::album_requested, this,
        [](const std::string &id) { on_album_requested(id); });
    QObject::connect(library_view_, &LibraryView::artist_requested, this,
        [](const std::string &id) { on_artist_requested(id); });
    QObject::connect(library_view_, &LibraryView::create_playlist_requested, this,
        [](const std::string &name, const std::string &desc, const std::string &privacy) {
            on_create_playlist(name, desc, privacy);
        });
    QObject::connect(library_view_, &LibraryView::login_requested, this, [this]() {
        auto *dialog = new WebLoginDialog(this);
        dialog->exec();
        dialog->deleteLater();
    });

    // ── Downloads view ─────────────────────────────────────────────────
    QObject::connect(downloads_view_, &DownloadsView::play_requested, this,
        [](Track track) { on_search_item_clicked(track); });

    // ── Stats view ─────────────────────────────────────────────────────
    QObject::connect(stats_view_, &StatsView::play_requested, this,
        [](Track track) { on_search_item_clicked(track); });

    // ── History view ───────────────────────────────────────────────────
    QObject::connect(history_view_, &HistoryView::play_requested, this,
        [](Track track) { on_search_item_clicked(track); });

    // ── Album detail view ──────────────────────────────────────────────
    QObject::connect(album_detail_view_, &AlbumDetailView::play_requested, this,
        [](Track track) { on_search_item_clicked(track); });
    QObject::connect(album_detail_view_, &AlbumDetailView::back_requested, this,
        [this]() { navigate_back_from_detail(); });
    guardOnline(album_detail_view_, &AlbumDetailView::artist_requested, "abrir detalles de artista", [this](const std::string &artist_id) {
        on_artist_requested(artist_id);
    });
    QObject::connect(album_detail_view_, &AlbumDetailView::play_all_requested, this,
        [](std::vector<Track> tracks) {
            on_play_all(to_rust_vec(tracks), false);
        });
    guardOnline(album_detail_view_, &AlbumDetailView::download_all_requested, "descargar álbumes", [this](std::vector<Track> tracks, std::string parent_id, std::string parent_title, std::string parent_thumbnail) {
        on_batch_download_requested(to_rust_vec(tracks), parent_id, parent_title, parent_thumbnail);
    });

    // ── Artist detail view ─────────────────────────────────────────────
    QObject::connect(artist_detail_view_, &ArtistDetailView::play_requested, this,
        [](Track track) { on_search_item_clicked(track); });
    guardOnline(artist_detail_view_, &ArtistDetailView::album_requested, "abrir detalles de álbum", [this](const std::string &album_id) {
        on_album_requested(album_id);
    });
    QObject::connect(artist_detail_view_, &ArtistDetailView::back_requested, this,
        [this]() { navigate_back_from_detail(); });

    // ── Playlist detail view ───────────────────────────────────────────
    QObject::connect(playlist_detail_view_, &PlaylistDetailView::play_requested, this,
        [](Track track) { on_search_item_clicked(track); });
    QObject::connect(playlist_detail_view_, &PlaylistDetailView::back_requested, this,
        [this]() { navigate_back_from_detail(); });
    QObject::connect(playlist_detail_view_, &PlaylistDetailView::play_all_requested, this,
        [](std::vector<Track> tracks) {
            on_play_all(to_rust_vec(tracks), false);
        });
    guardOnline(playlist_detail_view_, &PlaylistDetailView::download_all_requested, "descargar playlists", [this](std::vector<Track> tracks, std::string parent_id, std::string parent_title, std::string parent_thumbnail) {
        on_batch_download_requested(to_rust_vec(tracks), parent_id, parent_title, parent_thumbnail);
    });
    QObject::connect(playlist_detail_view_, &PlaylistDetailView::shuffle_requested, this,
        [](std::vector<Track> tracks) {
            on_play_all(to_rust_vec(tracks), true);
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

    // ── Show detail view ───────────────────────────────────────────────
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
}
