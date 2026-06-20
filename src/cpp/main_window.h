#ifndef DOREMI_MAIN_WINDOW_H
#define DOREMI_MAIN_WINDOW_H

#include <QMainWindow>
#include <QStackedWidget>
#include <QScrollArea>
#include "components/fade_stack.h"
#include <QLabel>
#include <QTimer>
#include <QSystemTrayIcon>
#include <QMenu>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QPointer>
#include <cstdint>
#include <map>
#include <vector>
#include "rust/cxx.h"

struct Track;
struct Album;
struct Artist;
struct Playlist;
struct Show;
struct Episode;
struct StatsData;
struct HomeCard;
struct TopResult;

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
class ShowDetailView;
class WelcomeView;
class ThemeTransitionOverlay;
class QNetworkCookie;



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
    void set_related_tracks(const rust::Vec<Track> &tracks);
    void set_current_track(const Track &track);
    void set_context_playlists(const rust::Vec<Playlist> &playlists);
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
    ShowDetailView* show_detail_view() const { return show_detail_view_; }
    WelcomeView* welcome_view() const { return welcome_view_; }
    ThemeTransitionOverlay* theme_transition() const { return theme_transition_; }
    std::string current_route() const { return current_route_; }

    void set_stop_on_close(bool stop) { stop_on_close_ = stop; }
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
    void setup_session_cookie_refresh();
    void update_session_cookie(const QNetworkCookie &cookie, bool removed);
    void persist_session_cookies();
    void navigate_to_internal(const std::string &route, bool record_history);
    void save_route_view_state();
    void restore_route_view_state(const std::string &route);
    void navigate_back();
    void navigate_back_from_detail();
    void navigate_forward();
    TitleBar *title_bar_;
    NavSidebar *nav_sidebar_;
    FadeStack *stack_;
    QScrollArea *body_scroll_;
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
    ShowDetailView *show_detail_view_;
    WelcomeView *welcome_view_;

    QSystemTrayIcon *tray_icon_;
    QAction *play_action_ = nullptr;
    bool stop_on_close_ = false;
    std::string current_route_ = "home";
    std::string detail_return_route_ = "home";
    std::vector<std::string> back_routes_;
    std::vector<std::string> forward_routes_;
    std::map<std::string, int> route_scroll_positions_;
    std::map<std::string, QPointer<QWidget>> route_focus_widgets_;

    QTimer *player_timer_;
    QTimer *session_cookie_timer_ = nullptr;
    std::map<std::string, std::string> session_cookies_;
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
    void set_search_results(TopResult top_result, bool has_top_result,
                            rust::Vec<Track> songs, rust::Vec<Track> videos, rust::Vec<Artist> artists,
                            rust::Vec<Album> albums, rust::Vec<Playlist> playlists,
                            rust::Vec<Show> shows, rust::Vec<Episode> episodes);
    void add_home_section(rust::Str title, rust::Vec<HomeCard> items);
void clear_home_sections();
void set_home_state(rust::Str state, rust::Str message);
void set_library_songs(rust::Vec<Track> songs);
void set_library_shows(rust::Vec<Show> shows);
void set_library_playlists(rust::Vec<Playlist> playlists);
void set_library_albums(rust::Vec<Album> albums);
void set_library_artists(rust::Vec<Artist> artists);
void set_search_history(rust::Vec<rust::String> queries);
void set_search_suggestions(rust::Str query, rust::Vec<rust::String> suggestions);
void apply_settings_to_ui();
void set_settings_theme(rust::Str mode);
void set_settings_accent(rust::Str color);
void set_settings_font_size(int32_t size);
void set_settings_language(rust::Str lang);
void set_settings_region(rust::Str region);
void set_settings_normalize(bool on);
void set_settings_crossfade(bool on);
void set_settings_equalizer_enabled(bool on);
void set_settings_equalizer_preset(rust::Str preset);
void set_settings_equalizer_values(double preamp, rust::Vec<double> bands);
void set_settings_sleep_timer(int32_t minutes);
void set_settings_discord_rpc(bool on);
void set_settings_lastfm_enabled(bool on);
void set_settings_lastfm_session(bool authenticated, rust::Str username, rust::Str apiKey, rust::Str apiSecret);
void set_settings_stop_on_close(bool stop);
void set_settings_mpris_enabled(bool on);
void set_track_lyrics(rust::Str plain, rust::Str synced);
void set_settings_subtitle_alignment(rust::Str align);
void set_settings_subtitle_font_size(int32_t size);
void set_settings_subtitle_line_spacing(double spacing);
void set_settings_subtitle_auto_scroll(bool on);
void set_settings_subtitle_glow_effect(bool on);
void set_settings_download_location(rust::Str location);
void set_settings_download_format(rust::Str format);
void set_settings_download_quality(rust::Str quality);


rust::String get_or_create_thumbnail(rust::Str title, int32_t variant);

void set_player_shuffle(bool on);
void set_player_repeat(int32_t mode);

void set_trending_items(rust::Vec<HomeCard> items);
void set_trending_state(rust::Str state, rust::Str message);
void set_downloads_list(rust::Vec<rust::String> titles,
                        rust::Vec<rust::String> artists,
                        rust::Vec<rust::String> thumbnails,
                        rust::Vec<rust::String> video_ids,
                        rust::Vec<rust::String> statuses,
                        rust::Vec<double> progresses);
void set_download_progress(rust::Str video_id, double percent, rust::Str status);
void set_batch_download_progress(rust::Str parent_id, int32_t total, int32_t completed, double percent);

void set_dominant_colors(rust::Vec<rust::String> colors);
void set_playback_queue(rust::Vec<Track> queue, int32_t current_index);
void set_context_playlists(rust::Vec<Playlist> playlists);
void set_related_tracks(rust::Vec<Track> tracks);
void set_current_track(Track track);
void set_prefetch_status(rust::Str track_id, rust::Str status);
void set_stats_data(StatsData stats);

void set_history_data(rust::Vec<Track> history, rust::Vec<rust::String> played_at);

void set_album_detail(Album album, rust::Vec<Track> tracks);

void set_artist_detail(Artist artist, rust::Vec<Track> tracks, rust::Vec<Album> albums);

void set_playlist_detail(Playlist playlist, rust::Vec<Track> tracks);
void set_show_detail(Show show, rust::Vec<Episode> episodes);

void update_youtube_auth_state(bool authenticated, rust::Str name, rust::Str avatar_url);

void set_update_available(rust::Str version, rust::Str notes, rust::Str url, rust::Str asset_url, rust::Str asset_name, int64_t asset_size);
void set_no_update_available();
void set_update_download_progress(double percent, rust::Str message);
void set_update_download_finished(rust::Str package_path);
void set_update_download_failed(rust::Str error);
void set_update_install_finished(bool success);

#include "doremi/src/bridge.rs.h"
#endif
