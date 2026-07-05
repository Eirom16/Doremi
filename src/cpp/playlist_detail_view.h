#ifndef DOREMI_PLAYLIST_DETAIL_VIEW_H
#define DOREMI_PLAYLIST_DETAIL_VIEW_H

#include <QWidget>
#include <QQuickWidget>
#include <QVariantList>
#include <QVariantMap>
#include <vector>
#include <string>
#include "doremi/src/bridge.rs.h"

class PlaylistDetailView : public QWidget {
    Q_OBJECT
    Q_PROPERTY(QString viewState READ viewState NOTIFY viewStateChanged)
    Q_PROPERTY(QString playlistTitle READ playlistTitle NOTIFY playlistInfoChanged)
    Q_PROPERTY(QString playlistAuthor READ playlistAuthor NOTIFY playlistInfoChanged)
    Q_PROPERTY(QString playlistCover READ playlistCover NOTIFY playlistInfoChanged)
    Q_PROPERTY(QString playlistSongCount READ playlistSongCount NOTIFY playlistInfoChanged)
    Q_PROPERTY(QString dominantColor READ dominantColor NOTIFY playlistInfoChanged)
    Q_PROPERTY(QVariantList tracks READ tracks NOTIFY tracksChanged)

public:
    explicit PlaylistDetailView(QWidget *parent = nullptr);

    void set_playlist_info(const Playlist &playlist);
    void set_playlist_tracks(const std::vector<Track> &tracks);
    void clear();
    void update_theme() {} // Kept for compatibility

    QString viewState() const { return view_state_; }
    QString playlistTitle() const { return playlist_title_; }
    QString playlistAuthor() const { return playlist_author_; }
    QString playlistCover() const { return playlist_cover_; }
    QString playlistSongCount() const { return playlist_song_count_; }
    QString dominantColor() const { return dominant_color_; }
    QVariantList tracks() const { return tracks_list_; }

    Q_INVOKABLE void requestPlay(int index);
    Q_INVOKABLE void requestPlayAll();
    Q_INVOKABLE void requestShuffle();
    Q_INVOKABLE void requestDownload();
    Q_INVOKABLE void requestRemoveTrack(int index);
    Q_INVOKABLE void requestRename();
    Q_INVOKABLE void requestDelete();

signals:
    void viewStateChanged();
    void playlistInfoChanged();
    void tracksChanged();

    void play_requested(Track track);
    void play_all_requested(std::vector<Track> tracks);
    void shuffle_requested(std::vector<Track> tracks);
    void download_all_requested(std::vector<Track> tracks, std::string parent_id, std::string parent_title, std::string parent_thumbnail);
    void back_requested();
    void rename_playlist_requested(const std::string &playlist_id, const std::string &name);
    void delete_playlist_requested(const std::string &playlist_id);
    void remove_track_from_playlist_requested(const std::string &playlist_id, const std::string &track_id);
    void track_moved(const std::string &playlist_id, int from, int to);
    void album_clicked(const std::string &album_id);
    void artist_clicked(const std::string &artist_id);
    void privacy_changed(const std::string &playlist_id, const std::string &privacy);

private:
    QQuickWidget *quick_widget_ = nullptr;
    
    QString view_state_ = "loading";
    QString playlist_title_;
    QString playlist_author_;
    QString playlist_cover_;
    QString playlist_song_count_;
    QString dominant_color_;
    
    QVariantList tracks_list_;
    
    std::vector<Track> raw_tracks_;
    Playlist current_playlist_;
    
    void extractDominantColor(const QString &thumbnailUrl);
};

#endif
