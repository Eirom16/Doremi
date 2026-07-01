#ifndef DOREMI_ARTIST_DETAIL_VIEW_H
#define DOREMI_ARTIST_DETAIL_VIEW_H

#include <QWidget>
#include <QVBoxLayout>
#include <QLabel>
#include <QHBoxLayout>
#include <QPushButton>
#include <QScrollArea>
#include <vector>
#include <string>
#include "components/track_row.h"
#include "doremi/src/bridge.rs.h"

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
    QScrollArea *scroll_area_ = nullptr;
};

#endif
