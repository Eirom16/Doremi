#ifndef DOREMI_RELATED_TRACKS_WIDGET_H
#define DOREMI_RELATED_TRACKS_WIDGET_H

#include <QWidget>
#include <QScrollArea>
#include <QVBoxLayout>
#include <QLabel>
#include <QContextMenuEvent>
#include "doremi/src/bridge.rs.h"

class RelatedTrackRow : public QWidget {
    Q_OBJECT
public:
    explicit RelatedTrackRow(const Track &track, QWidget *parent = nullptr);

signals:
    void clicked(const Track &track);
    void add_to_queue_requested(const Track &track);

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void contextMenuEvent(QContextMenuEvent *event) override;
    void enterEvent(QEnterEvent *event) override;
    void leaveEvent(QEvent *event) override;

private:
    Track track_;
    bool pressed_ = false;
};

class RelatedTracksWidget : public QScrollArea {
    Q_OBJECT
public:
    explicit RelatedTracksWidget(QWidget *parent = nullptr);
    void setTracks(const std::vector<Track> &tracks);

signals:
    void play_requested(const Track &track);
    void add_to_queue_requested(const Track &track);

private:
    void clearLayout();
    QWidget *container_;
    QVBoxLayout *layout_;
};

#endif
