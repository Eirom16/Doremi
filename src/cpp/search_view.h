#ifndef DOREMI_SEARCH_VIEW_H
#define DOREMI_SEARCH_VIEW_H

#include <QWidget>
#include <QVBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QHBoxLayout>
#include <QScrollArea>
#include <vector>
#include <string>

#include "widgets.h"
#include "doremi/src/bridge.rs.h"

class SearchView : public QWidget {
    Q_OBJECT
public:
    explicit SearchView(QWidget *parent = nullptr);
    void update_theme();
    void set_query(const std::string &query, const std::string &filter = "all");
    void set_results(const TopResult &top_result, bool has_top_result,
                     const std::vector<Track> &songs,
                     const std::vector<Track> &videos,
                     const std::vector<Artist> &artists,
                     const std::vector<Album> &albums,
                     const std::vector<Playlist> &playlists,
                     const std::vector<Show> &shows = {},
                     const std::vector<Episode> &episodes = {});
    void set_recent_searches(const std::vector<std::string> &queries);
signals:
    void filter_changed(const std::string &filter);
    void play_requested(Track track);
    void search_requested(const std::string &query, const std::string &filter);
    void album_requested(const std::string &browse_id);
    void artist_requested(const std::string &browse_id);
    void playlist_requested(const std::string &playlist_id);
    void show_requested(const std::string &browse_id);
    void add_favorite_requested(Track track);
    void download_requested(Track track);
    void add_to_queue_next_requested(Track track);
    void add_to_queue_end_requested(Track track);
    void search_history_delete_requested(const std::string &query);
    void search_history_clear_requested();
private:
    QLabel *header_;
    QVBoxLayout *results_;
    QHBoxLayout *filters_;
    std::vector<QPushButton *> filter_btns_;
    std::string current_query_;
    void set_active_filter(const std::string &filter);
    void show_recent_searches(const std::vector<std::string> &queries);
    void show_results(const TopResult &top_result, bool has_top_result,
                      const std::vector<Track> &songs,
                      const std::vector<Track> &videos,
                      const std::vector<Artist> &artists,
                      const std::vector<Album> &albums,
                      const std::vector<Playlist> &playlists,
                      const std::vector<Show> &shows = {},
                      const std::vector<Episode> &episodes = {});
    bool showing_recent_ = false;
};

#endif
