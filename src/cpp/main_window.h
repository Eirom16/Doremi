#ifndef DOREMI_MAIN_WINDOW_H
#define DOREMI_MAIN_WINDOW_H

#include <QMainWindow>
#include <QStackedWidget>
#include "components/fade_stack.h"
#include <QLabel>
#include <QTimer>
#include <QSystemTrayIcon>
#include <QMenu>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <cstdint>
#include "rust/cxx.h"

struct Track;
struct Album;
struct Artist;
struct Playlist;
struct StatsData;

class TitleBar;
class NavSidebar;
class PlayerBar;
class HomeView;
class SearchView;
class LibraryView;
class SettingsView;
class TrendingView;
class DownloadsView;
class NowPlayingView;
class StatsView;
class HistoryView;
class AlbumDetailView;
class ArtistDetailView;
class PlaylistDetailView;
class WelcomeView;
class ThemeTransitionOverlay;



class DoremiMainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit DoremiMainWindow(QWidget *parent = nullptr);
    ~DoremiMainWindow() override;

    void navigate_to(const std::string &route);
    void show_notif(const std::string &message, const std::string &kind);
    void update_player_state(int32_t state, int32_t position_ms, int32_t duration_ms);
    void set_mini_player_info(const std::string &title, const std::string &artist,
                              const std::string &thumbnail);
    void set_playback_playing(bool playing);
    void set_dominant_colors(const std::vector<std::string> &colors);
    void set_track_lyrics(const std::string &plain, const std::string &synced);
    void set_playback_queue(const rust::Vec<Track> &queue, int32_t current_index);
    void set_history_data(const rust::Vec<Track> &history, const rust::Vec<rust::String> &played_at);
    void set_stats_data(const StatsData &stats);


    TitleBar* title_bar() const { return title_bar_; }
    NavSidebar* nav_sidebar() const { return nav_sidebar_; }
    PlayerBar* player_bar() const { return player_bar_; }
    HomeView* home_view() const { return home_view_; }
    SearchView* search_view() const { return search_view_; }
    LibraryView* library_view() const { return library_view_; }
    SettingsView* settings_view() const { return settings_view_; }
    TrendingView* trending_view() const { return trending_view_; }
    DownloadsView* downloads_view() const { return downloads_view_; }
    NowPlayingView* now_playing_view() const { return now_playing_view_; }
    StatsView* stats_view() const { return stats_view_; }
    HistoryView* history_view() const { return history_view_; }
    AlbumDetailView* album_detail_view() const { return album_detail_view_; }
    ArtistDetailView* artist_detail_view() const { return artist_detail_view_; }
    PlaylistDetailView* playlist_detail_view() const { return playlist_detail_view_; }
    WelcomeView* welcome_view() const { return welcome_view_; }
    ThemeTransitionOverlay* theme_transition() const { return theme_transition_; }


    void setup_tray();

signals:
    void play_pause_triggered();
    void next_triggered();
    void previous_triggered();
    void volume_changed(int32_t delta);
    void volume_set(int32_t volume);
    void seek_triggered(int32_t delta_ms);
    void shuffle_toggled(bool on);
    void repeat_cycled();
    void window_closed();
protected:
    void closeEvent(QCloseEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
private:
    void setup_shortcuts();
    void connect_signals();
    TitleBar *title_bar_;
    NavSidebar *nav_sidebar_;
    FadeStack *stack_;
    PlayerBar *player_bar_;
    HomeView *home_view_;
    SearchView *search_view_;
    LibraryView *library_view_;
    SettingsView *settings_view_;
    TrendingView *trending_view_;
    DownloadsView *downloads_view_;
    NowPlayingView *now_playing_view_;
    StatsView *stats_view_;
    HistoryView *history_view_;
    AlbumDetailView *album_detail_view_;
    ArtistDetailView *artist_detail_view_;
    PlaylistDetailView *playlist_detail_view_;
    WelcomeView *welcome_view_;

    QSystemTrayIcon *tray_icon_;

    QTimer *player_timer_;
    ThemeTransitionOverlay *theme_transition_;
};

extern DoremiMainWindow *g_main_window;

uint16_t bridge_contract_major();
uint16_t bridge_contract_minor();
void create_main_window(rust::Str app_name, rust::Str theme_mode,
                        rust::Str accent_color, int32_t font_size);
void show_main_window();
void navigate_to(rust::Str route);
void show_notification(rust::Str message, rust::Str kind);
void apply_theme(rust::Str theme_mode, rust::Str accent_color);
void update_player_state(int32_t state, int32_t pos, int32_t dur);
void set_mini_player(rust::Str title, rust::Str artist, rust::Str thumb);
rust::String get_search_bar_text();
void set_search_bar_text(rust::Str text);
void set_window_title(rust::Str title);
void set_playing(bool playing);
void set_player_volume(int32_t volume);
void run_event_loop();
void set_search_results(rust::Vec<Track> songs, rust::Vec<Artist> artists, rust::Vec<Album> albums);
void add_home_section(rust::Str title, rust::Vec<rust::String> items);
void clear_home_sections();
void set_library_songs(rust::Vec<Track> songs);
void set_library_playlists(rust::Vec<Playlist> playlists);
void set_library_albums(rust::Vec<Album> albums);
void set_library_artists(rust::Vec<Artist> artists);
void set_search_history(rust::Vec<rust::String> queries);
void apply_settings_to_ui();
void set_settings_theme(rust::Str mode);
void set_settings_accent(rust::Str color);
void set_settings_font_size(int32_t size);
void set_settings_language(rust::Str lang);
void set_settings_normalize(bool on);
void set_settings_crossfade(bool on);
void set_settings_equalizer_enabled(bool on);
void set_settings_equalizer_preset(rust::Str preset);
void set_settings_equalizer_values(double preamp, rust::Vec<double> bands);
void set_settings_sleep_timer(int32_t minutes);
void set_settings_discord_rpc(bool on);
void set_settings_lastfm_enabled(bool on);
void set_settings_lastfm_session(bool authenticated, rust::Str username, rust::Str apiKey, rust::Str apiSecret);
void set_track_lyrics(rust::Str plain, rust::Str synced);
void set_settings_subtitle_alignment(rust::Str align);
void set_settings_subtitle_font_size(int32_t size);
void set_settings_subtitle_line_spacing(double spacing);
void set_settings_subtitle_auto_scroll(bool on);
void set_settings_subtitle_glow_effect(bool on);


rust::String get_or_create_thumbnail(rust::Str title, int32_t variant);

void set_player_shuffle(bool on);
void set_player_repeat(int32_t mode);

void set_trending_items(rust::Vec<rust::String> titles,
                        rust::Vec<rust::String> subtitles,
                        rust::Vec<rust::String> thumbnails);
void set_downloads_list(rust::Vec<rust::String> titles,
                        rust::Vec<rust::String> artists,
                        rust::Vec<rust::String> thumbnails);

void set_dominant_colors(rust::Vec<rust::String> colors);
void set_playback_queue(rust::Vec<Track> queue, int32_t current_index);
void set_stats_data(StatsData stats);

void set_history_data(rust::Vec<Track> history, rust::Vec<rust::String> played_at);

void set_album_detail(Album album, rust::Vec<Track> tracks);

void set_artist_detail(Artist artist, rust::Vec<Track> tracks, rust::Vec<Album> albums);

void set_playlist_detail(Playlist playlist, rust::Vec<Track> tracks);

void update_youtube_auth_state(bool authenticated, rust::Str name, rust::Str avatar_url);

void set_update_available(rust::Str version, rust::Str notes, rust::Str url, rust::Str asset_url, rust::Str asset_name, int64_t asset_size);
void set_no_update_available();
void set_update_download_progress(double percent, rust::Str message);
void set_update_download_finished(rust::Str package_path);
void set_update_download_failed(rust::Str error);
void set_update_install_finished(bool success);

#include "doremi/src/bridge.rs.h"
#endif
