#ifndef DOREMI_ALBUM_DETAIL_VIEW_H
#define DOREMI_ALBUM_DETAIL_VIEW_H

#include <QWidget>
#include <QQuickWidget>
#include <QVariantList>
#include <QVariantMap>
#include <vector>
#include <string>
#include "doremi/src/bridge.rs.h"

class AlbumDetailView : public QWidget {
    Q_OBJECT
    Q_PROPERTY(QString viewState READ viewState NOTIFY viewStateChanged)
    Q_PROPERTY(QString albumTitle READ albumTitle NOTIFY albumInfoChanged)
    Q_PROPERTY(QString albumArtist READ albumArtist NOTIFY albumInfoChanged)
    Q_PROPERTY(QString albumCover READ albumCover NOTIFY albumInfoChanged)
    Q_PROPERTY(QString albumYear READ albumYear NOTIFY albumInfoChanged)
    Q_PROPERTY(QString albumSongCount READ albumSongCount NOTIFY albumInfoChanged)
    Q_PROPERTY(QString dominantColor READ dominantColor NOTIFY albumInfoChanged)
    Q_PROPERTY(QVariantList tracks READ tracks NOTIFY tracksChanged)

public:
    explicit AlbumDetailView(QWidget *parent = nullptr);

    void set_album_info(const Album &album);
    void set_album_tracks(const std::vector<Track> &tracks);
    void set_state(const std::string &state, const std::string &message = "");
    void clear();
    void update_theme() {} // Kept for compatibility

    QString viewState() const { return view_state_; }
    QString albumTitle() const { return album_title_; }
    QString albumArtist() const { return album_artist_; }
    QString albumCover() const { return album_cover_; }
    QString albumYear() const { return album_year_; }
    QString albumSongCount() const { return album_song_count_; }
    QString dominantColor() const { return dominant_color_; }
    QVariantList tracks() const { return tracks_list_; }

    Q_INVOKABLE void requestPlay(int index);
    Q_INVOKABLE void requestPlayAll();
    Q_INVOKABLE void requestShuffle();
    Q_INVOKABLE void requestDownload();
    Q_INVOKABLE void requestArtist();

signals:
    void viewStateChanged();
    void albumInfoChanged();
    void tracksChanged();

    void play_requested(Track track);
    void play_all_requested(std::vector<Track> tracks);
    void shuffle_requested(std::vector<Track> tracks);
    void download_all_requested(std::vector<Track> tracks, std::string parent_id, std::string parent_title, std::string parent_thumbnail);
    void back_requested();
    void artist_requested(const std::string &artist_id);

private:
    QQuickWidget *quick_widget_ = nullptr;
    
    QString view_state_ = "loading";
    QString album_title_;
    QString album_artist_;
    QString album_cover_;
    QString album_year_;
    QString album_song_count_;
    QString dominant_color_;
    
    QVariantList tracks_list_;
    
    std::vector<Track> raw_tracks_;
    Album current_album_;
    
    void extractDominantColor(const QString &thumbnailUrl);
};

#endif
