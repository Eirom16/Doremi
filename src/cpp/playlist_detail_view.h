#ifndef DOREMI_PLAYLIST_DETAIL_VIEW_H
#define DOREMI_PLAYLIST_DETAIL_VIEW_H

#include <QWidget>
#include <QScrollArea>
#include <QVBoxLayout>
#include <QLabel>
#include <QHBoxLayout>
#include <QMouseEvent>
#include <QContextMenuEvent>
#include <QPushButton>
#include <QInputDialog>
#include <QMessageBox>
#include <vector>
#include <string>
#include "doremi/src/bridge.rs.h"

// Internal track row widget for playlist tracks
class PlaylistTrackRow : public QWidget {
    Q_OBJECT
public:
    PlaylistTrackRow(int num, const QString &title, const QString &artist,
                     const QString &duration, Track track,
                     QWidget *parent = nullptr);
    void setPlaylistId(const std::string &playlist_id) { playlist_id_ = playlist_id; }

signals:
    void play_requested(Track track);
    void remove_requested(const std::string &playlist_id, const std::string &track_id);

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void contextMenuEvent(QContextMenuEvent *event) override;
    void enterEvent(QEnterEvent *event) override;
    void leaveEvent(QEvent *event) override;

private:
    Track track_;
    std::string playlist_id_;
};

class PlaylistDetailView : public QWidget {
    Q_OBJECT
public:
    explicit PlaylistDetailView(QWidget *parent = nullptr);

    void set_playlist_info(const Playlist &playlist);
    void set_playlist_tracks(const std::vector<Track> &tracks);
    void clear();

signals:
    void play_requested(Track track);
    void play_all_requested(std::vector<Track> tracks);
    void shuffle_requested(std::vector<Track> tracks);
    void back_requested();
    void rename_playlist_requested(const std::string &playlist_id, const std::string &name);
    void delete_playlist_requested(const std::string &playlist_id);
    void remove_track_from_playlist_requested(const std::string &playlist_id, const std::string &track_id);
    void album_clicked(const std::string &album_id);
    void artist_clicked(const std::string &artist_id);
    void privacy_changed(const std::string &playlist_id, const std::string &privacy);

private:
    void setupLayout();

    QScrollArea *scroll_area_;
    QWidget *scroll_content_;
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
    std::vector<Track> tracks_;
};

#endif
