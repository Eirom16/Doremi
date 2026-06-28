#ifndef DOREMI_ALBUM_DETAIL_VIEW_H
#define DOREMI_ALBUM_DETAIL_VIEW_H

#include <QWidget>
#include <QVBoxLayout>
#include <QLabel>
#include <QHBoxLayout>
#include <vector>
#include <string>
#include "components/track_row.h"
#include "doremi/src/bridge.rs.h"

class AlbumDetailView : public QWidget {
    Q_OBJECT
public:
    explicit AlbumDetailView(QWidget *parent = nullptr);

    void set_album_info(const Album &album);
    void set_album_tracks(const std::vector<Track> &tracks);
    void clear();
    void update_theme();

signals:
    void play_requested(Track track);
    void play_all_requested(std::vector<Track> tracks);
    void download_all_requested(std::vector<Track> tracks, std::string parent_id, std::string parent_title, std::string parent_thumbnail);
    void back_requested();
    void artist_requested(const std::string &artist_id);
    void artist_name_clicked(const std::string &artist_id);
    void favorite_toggled(const std::string &album_id, bool is_favorite);

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;

private:
    void setupLayout();

    QVBoxLayout *content_layout_;

    // Header widgets
    QLabel *cover_label_;
    QLabel *title_label_;
    QLabel *artist_label_;
    QLabel *meta_label_;

    // Tracks container
    QWidget *tracks_widget_;
    QVBoxLayout *tracks_layout_;
    std::vector<Track> tracks_;
    std::string artist_id_;
    Album current_album_;
};

#endif
