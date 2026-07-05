#ifndef DOREMI_SEARCH_VIEW_H
#define DOREMI_SEARCH_VIEW_H

#include <QWidget>
#include <QQuickWidget>
#include <QVariantList>
#include <QVariantMap>
#include <vector>
#include <string>

#include "doremi/src/bridge.rs.h"

class SearchView : public QWidget {
    Q_OBJECT
    Q_PROPERTY(QString currentQuery READ currentQuery NOTIFY queryChanged)
    Q_PROPERTY(QString activeFilter READ activeFilter NOTIFY filterChanged)
    Q_PROPERTY(bool showingRecent READ isShowingRecent NOTIFY recentChanged)
    Q_PROPERTY(QVariantList recentSearches READ recentSearches NOTIFY recentChanged)
    
    Q_PROPERTY(bool hasTopResult READ hasTopResult NOTIFY resultsChanged)
    Q_PROPERTY(QVariantMap topResult READ getTopResult NOTIFY resultsChanged)
    
    Q_PROPERTY(QVariantList songs READ songs NOTIFY resultsChanged)
    Q_PROPERTY(QVariantList videos READ videos NOTIFY resultsChanged)
    Q_PROPERTY(QVariantList artists READ artists NOTIFY resultsChanged)
    Q_PROPERTY(QVariantList albums READ albums NOTIFY resultsChanged)
    Q_PROPERTY(QVariantList playlists READ playlists NOTIFY resultsChanged)
    Q_PROPERTY(QVariantList shows READ shows NOTIFY resultsChanged)
    Q_PROPERTY(QVariantList episodes READ episodes NOTIFY resultsChanged)

public:
    explicit SearchView(QWidget *parent = nullptr);
    void update_theme() {} // Kept for compatibility

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
    
    QString currentQuery() const { return current_query_; }
    QString activeFilter() const { return active_filter_; }
    bool isShowingRecent() const { return showing_recent_; }
    QVariantList recentSearches() const { return recent_searches_list_; }
    
    bool hasTopResult() const { return has_top_result_; }
    QVariantMap getTopResult() const { return top_result_map_; }
    
    QVariantList songs() const { return songs_list_; }
    QVariantList videos() const { return videos_list_; }
    QVariantList artists() const { return artists_list_; }
    QVariantList albums() const { return albums_list_; }
    QVariantList playlists() const { return playlists_list_; }
    QVariantList shows() const { return shows_list_; }
    QVariantList episodes() const { return episodes_list_; }

    Q_INVOKABLE void requestFilterChange(const QString &filter);
    Q_INVOKABLE void requestSearch(const QString &query);
    Q_INVOKABLE void requestClearSearchHistory();
    Q_INVOKABLE void requestDeleteSearchHistory(const QString &query);
    Q_INVOKABLE void requestPlayTrack(const QString &trackId);
    Q_INVOKABLE void requestAlbum(const QString &browseId);
    Q_INVOKABLE void requestArtist(const QString &browseId);
    Q_INVOKABLE void requestPlaylist(const QString &playlistId);
    Q_INVOKABLE void requestShow(const QString &browseId);
    Q_INVOKABLE void requestTopResult();

signals:
    void queryChanged();
    void filterChanged();
    void recentChanged();
    void resultsChanged();

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
    QQuickWidget *quick_widget_ = nullptr;
    
    QString current_query_;
    QString active_filter_ = "all";
    bool showing_recent_ = false;
    
    bool has_top_result_ = false;
    QVariantMap top_result_map_;
    
    QVariantList recent_searches_list_;
    QVariantList songs_list_;
    QVariantList videos_list_;
    QVariantList artists_list_;
    QVariantList albums_list_;
    QVariantList playlists_list_;
    QVariantList shows_list_;
    QVariantList episodes_list_;
    
    std::vector<Track> raw_songs_;
    std::vector<Track> raw_videos_;
    TopResult raw_top_result_;
    
    Track getTrackById(const QString &id);
};

#endif
