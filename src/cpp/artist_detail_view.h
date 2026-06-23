#ifndef DOREMI_ARTIST_DETAIL_VIEW_H
#define DOREMI_ARTIST_DETAIL_VIEW_H

#include <QWidget>
#include <QVBoxLayout>
#include <QLabel>
#include <QHBoxLayout>
#include <QMouseEvent>
#include <QContextMenuEvent>
#include <QPushButton>
#include <vector>
#include <string>
#include "doremi/src/bridge.rs.h"

// Internal track row widget for artist tracks
class ArtistTrackRow : public QWidget {
    Q_OBJECT
public:
    ArtistTrackRow(const QString &title, const QString &album,
                   const QString &duration, Track track,
                   QWidget *parent = nullptr);

    void update_theme();
signals:
    void play_requested(Track track);

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void contextMenuEvent(QContextMenuEvent *event) override;
    void enterEvent(QEnterEvent *event) override;
    void leaveEvent(QEvent *event) override;

private:
    Track track_;
};

class ArtistDetailView : public QWidget {
    Q_OBJECT
public:
    explicit ArtistDetailView(QWidget *parent = nullptr);

    void set_artist_info(const Artist &artist);
    void set_artist_tracks(const std::vector<Track> &tracks,
                           const std::vector<Album> &albums,
                           const std::vector<Album> &singles);
    void clear();
    void update_theme();

signals:
    void play_requested(Track track);
    void album_requested(const std::string &album_id);
    void album_clicked(const std::string &album_id);
    void back_requested();
    void favorite_toggled(const std::string &artist_id, bool is_favorite);

private:
    void setupLayout();

    QVBoxLayout *content_layout_;

    // Header widgets
    QLabel *avatar_label_;
    QLabel *name_label_;
    QLabel *meta_label_;
    QLabel *desc_label_;

    // Tracks container
    QWidget *tracks_widget_;
    QVBoxLayout *tracks_layout_;

    // Dynamic section containers
    QWidget *albums_container_ = nullptr;
    QWidget *singles_container_ = nullptr;
};

#endif
