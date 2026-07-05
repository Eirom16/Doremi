#ifndef DOREMI_ARTIST_DETAIL_VIEW_H
#define DOREMI_ARTIST_DETAIL_VIEW_H

#include <QWidget>
#include <QQuickWidget>
#include <QVariantList>
#include <QVariantMap>
#include <vector>
#include <string>
#include "doremi/src/bridge.rs.h"

class ArtistDetailView : public QWidget {
    Q_OBJECT
    Q_PROPERTY(QString viewState READ viewState NOTIFY viewStateChanged)
    Q_PROPERTY(QString artistName READ artistName NOTIFY artistInfoChanged)
    Q_PROPERTY(QString artistAvatar READ artistAvatar NOTIFY artistInfoChanged)
    Q_PROPERTY(QString dominantColor READ dominantColor NOTIFY artistInfoChanged)
    Q_PROPERTY(QVariantList topTracks READ topTracks NOTIFY tracksChanged)
    Q_PROPERTY(QVariantList albums READ albums NOTIFY albumsChanged)
    Q_PROPERTY(QVariantList singles READ singles NOTIFY albumsChanged)

public:
    explicit ArtistDetailView(QWidget *parent = nullptr);

    void set_artist_info(const Artist &artist);
    void set_artist_tracks(const std::vector<Track> &tracks,
                           const std::vector<Album> &albums,
                           const std::vector<Album> &singles);
    void clear();
    void update_theme() {}

    QString viewState() const { return view_state_; }
    QString artistName() const { return artist_name_; }
    QString artistAvatar() const { return artist_avatar_; }
    QString dominantColor() const { return dominant_color_; }
    QVariantList topTracks() const { return top_tracks_list_; }
    QVariantList albums() const { return albums_list_; }
    QVariantList singles() const { return singles_list_; }

    Q_INVOKABLE void requestPlay(int index);
    Q_INVOKABLE void requestAlbum(const QString &albumId);

signals:
    void viewStateChanged();
    void artistInfoChanged();
    void tracksChanged();
    void albumsChanged();

    void play_requested(Track track);
    void album_requested(const std::string &album_id);
    void album_clicked(const std::string &album_id);
    void back_requested();
    void favorite_toggled(const std::string &artist_id, bool is_favorite);

private:
    QQuickWidget *quick_widget_ = nullptr;
    
    QString view_state_ = "loading";
    QString artist_name_;
    QString artist_avatar_;
    QString dominant_color_;
    
    QVariantList top_tracks_list_;
    QVariantList albums_list_;
    QVariantList singles_list_;
    
    std::vector<Track> raw_top_tracks_;
    std::string current_artist_id_;
    
    void extractDominantColor(const QString &thumbnailUrl);
    QVariantList convertAlbumsToVariantList(const std::vector<Album> &albums);
};

#endif
