#include <QApplication>
#include <QGuiApplication>
#include <QPixmapCache>
#include <QWebEngineProfile>
#include <QWebEngineCookieStore>
#include <QNetworkCookie>
#include <QDebug>

#include "main_window.h"
#include "bridge_helpers.h"
#include "ffi_utils.h"
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
#include "welcome_view.h"
#include "theme_controller.h"
#include "doremi/src/bridge.rs.h"

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

void apply_settings_to_ui() {}

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

void set_album_detail(Album album, rust::Vec<Track> tracks) {
    std::vector<Track> tt;
    tt.reserve(tracks.size());
    for (const auto &x : tracks) tt.push_back(x);
    mutate_main_window("set_album_detail", [album = std::move(album), tt = std::move(tt)](DoremiMainWindow &window) {
        if (!window.album_detail_view()) return;
        window.album_detail_view()->clear();
        window.album_detail_view()->set_album_info(album);
        window.album_detail_view()->set_album_tracks(tt);
        window.navigate_to("album_detail");
    });
}

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
        window.artist_detail_view()->clear();
        window.artist_detail_view()->set_artist_info(artist);
        window.artist_detail_view()->set_artist_tracks(tt, al, si);
        window.navigate_to("artist_detail");
    });
}

void set_playlist_detail(Playlist playlist, rust::Vec<Track> tracks) {
    std::vector<Track> tt;
    tt.reserve(tracks.size());
    for (const auto &x : tracks) tt.push_back(x);
    mutate_main_window("set_playlist_detail", [playlist = std::move(playlist), tt = std::move(tt)](DoremiMainWindow &window) {
        if (!window.playlist_detail_view()) return;
        window.playlist_detail_view()->clear();
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

void setup_ui_test(rust::Str view, rust::Str screenshot_path) {
    std::string v_str = Ffi::to_std_string(view);
    std::string path_str = Ffi::to_std_string(screenshot_path);
    mutate_main_window("setup_ui_test", [v_str, path_str](DoremiMainWindow &window) {
        window.setup_ui_test(v_str, path_str);
    });
}
