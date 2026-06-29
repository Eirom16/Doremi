#pragma once

#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QMouseEvent>
#include <QContextMenuEvent>
#include "doremi/src/bridge.rs.h"

struct TrackRowConfig {
    bool show_fav = true;
    bool drag_drop = false;
    int height = 48;
    int corner_radius = 6;
};

class TrackRow : public QWidget {
    Q_OBJECT
public:
    explicit TrackRow(Track track, TrackRowConfig config = TrackRowConfig(), QWidget *parent = nullptr);
    virtual ~TrackRow() = default;

    virtual void update_theme();

    int index() const { return index_; }
    void setIndex(int idx) { index_ = idx; }

signals:
    void play_requested(Track track);

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void contextMenuEvent(QContextMenuEvent *event) override;
    void enterEvent(QEnterEvent *event) override;
    void leaveEvent(QEvent *event) override;

    Track track_;
    TrackRowConfig config_;
    int index_ = -1;
    bool hovered_ = false;
    bool dragging_ = false;
    QPoint drag_start_position_;

    QHBoxLayout *layout_ = nullptr;
    QLabel *title_label_ = nullptr;
    QLabel *subtitle_label_ = nullptr;
    QLabel *duration_label_ = nullptr;
    QPushButton *fav_btn_ = nullptr;

    void setupCommonWidgets(const QString &title, const QString &subtitle, const QString &duration);
    void addFavButton();
    QWidget* createTextContainer(const QString &title, const QString &subtitle);
};

class AlbumTrackRow : public TrackRow {
    Q_OBJECT
public:
    AlbumTrackRow(int num, const QString &title, const QString &artist,
                  const QString &duration, Track track,
                  QWidget *parent = nullptr);
};

class PlaylistTrackRow : public TrackRow {
    Q_OBJECT
public:
    PlaylistTrackRow(int num, const QString &title, const QString &artist,
                     const QString &duration, Track track,
                     QWidget *parent = nullptr);
    void setPlaylistId(const std::string &playlist_id) { playlist_id_ = playlist_id; }
    int rowIndex() const { return index_; }

signals:
    void remove_requested(const std::string &playlist_id, const std::string &track_id);
    void move_requested(int from, int to);

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void contextMenuEvent(QContextMenuEvent *event) override;

private:
    std::string playlist_id_;
};

class ArtistTrackRow : public TrackRow {
    Q_OBJECT
public:
    ArtistTrackRow(const QString &title, const QString &album,
                   const QString &duration, Track track,
                   QWidget *parent = nullptr);
};

class HistoryRow : public TrackRow {
    Q_OBJECT
public:
    HistoryRow(const Track &track,
               const std::string &played_at,
               const std::string &feedback_token,
               QWidget *parent = nullptr);

    void update_theme() override;

signals:
    void delete_requested(const std::string &track_id, const std::string &feedback_token);

protected:
    void contextMenuEvent(QContextMenuEvent *event) override;

private:
    std::string feedback_token_;
    QLabel *time_ago_label_ = nullptr;
    QPushButton *delete_btn_ = nullptr;
    QLabel *thumb_label_ = nullptr;

    void loadThumbnail(const Track &track);
};

class EpisodeRow : public QWidget {
    Q_OBJECT
public:
    EpisodeRow(const QString &title, const QString &description,
               const QString &duration, Episode episode,
               QWidget *parent = nullptr);

    void update_theme();
signals:
    void play_requested(Episode episode);

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void contextMenuEvent(QContextMenuEvent *event) override;
    void enterEvent(QEnterEvent *event) override;
    void leaveEvent(QEvent *event) override;

private:
    Episode episode_;
};

class TopTrackRow : public QWidget {
    Q_OBJECT
public:
    TopTrackRow(int rank, const Track &track,
                int plays, int max_plays, QWidget *parent = nullptr);

signals:
    void clicked(Track track);

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void enterEvent(QEnterEvent *event) override;
    void leaveEvent(QEvent *event) override;

private:
    Track track_;
};

