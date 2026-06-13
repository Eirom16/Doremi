#ifndef DOREMI_ALBUM_DETAIL_VIEW_H
#define DOREMI_ALBUM_DETAIL_VIEW_H

#include <QWidget>
#include <QScrollArea>
#include <QVBoxLayout>
#include <QLabel>
#include <QHBoxLayout>
#include <QMouseEvent>
#include <vector>
#include <string>
#include "doremi/src/bridge.rs.h"

// Internal track row widget for album tracks
class AlbumTrackRow : public QWidget {
    Q_OBJECT
public:
    AlbumTrackRow(int num, const QString &title, const QString &artist,
                  const QString &duration, Track track,
                  QWidget *parent = nullptr);

signals:
    void play_requested(Track track);

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void enterEvent(QEnterEvent *event) override;
    void leaveEvent(QEvent *event) override;

private:
    Track track_;
};

class AlbumDetailView : public QWidget {
    Q_OBJECT
public:
    explicit AlbumDetailView(QWidget *parent = nullptr);

    void set_album_info(const Album &album);
    void set_album_tracks(const std::vector<Track> &tracks);
    void clear();

signals:
    void play_requested(Track track);
    void play_all_requested();
    void back_requested();

private:
    void setupLayout();

    QScrollArea *scroll_area_;
    QWidget *scroll_content_;
    QVBoxLayout *content_layout_;

    // Header widgets
    QLabel *cover_label_;
    QLabel *title_label_;
    QLabel *artist_label_;
    QLabel *meta_label_;

    // Tracks container
    QWidget *tracks_widget_;
    QVBoxLayout *tracks_layout_;
};

#endif
