#ifndef DOREMI_PLAYLIST_DETAIL_VIEW_H
#define DOREMI_PLAYLIST_DETAIL_VIEW_H

#include <QWidget>
#include <QVBoxLayout>
#include <QLabel>
#include <QHBoxLayout>
#include <QPushButton>
#include <QInputDialog>
#include <QMessageBox>
#include <QPoint>
#include <QFrame>
#include <QScrollArea>
#include <vector>
#include <string>
#include "components/track_row.h"
#include "doremi/src/bridge.rs.h"

class PlaylistDetailView : public QWidget {
    Q_OBJECT
public:
    explicit PlaylistDetailView(QWidget *parent = nullptr);

    void set_playlist_info(const Playlist &playlist);
    void set_playlist_tracks(const std::vector<Track> &tracks);
    void clear();
    void update_theme();

signals:
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
    void setupLayout();
    void rebuild_tracks();
    bool eventFilter(QObject *watched, QEvent *event) override;
    int dropIndexAt(const QPoint &position, int source_index, int *indicator_y) const;
    void hideDropIndicator();

    QVBoxLayout *content_layout_;

    // Header
    QLabel *cover_label_;
    QLabel *title_label_;
    QLabel *desc_label_;
    QLabel *meta_label_;

    // Actions
    QPushButton *edit_btn_;
    QPushButton *delete_btn_;
    std::string playlist_id_;

    // Tracks
    QWidget *tracks_widget_;
    QVBoxLayout *tracks_layout_;
    QFrame *drop_indicator_;
    std::vector<Track> tracks_;
    Playlist current_playlist_;
    QScrollArea *scroll_area_ = nullptr;
};

#endif
