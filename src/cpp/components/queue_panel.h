#ifndef DOREMI_QUEUE_PANEL_H
#define DOREMI_QUEUE_PANEL_H

#include <QWidget>
#include <QScrollArea>
#include <QVBoxLayout>
#include <QLabel>
#include <QPoint>
#include <QPushButton>
#include <QFrame>
#include "doremi/src/bridge.rs.h"

// Interactive queue row widget
class QueueRow : public QWidget {
    Q_OBJECT
public:
    explicit QueueRow(int index, const Track &track,
                      bool is_current, QWidget *parent = nullptr);
    int index() const { return index_; }

signals:
    void clicked(int index);
    void remove_requested(int index);
    void move_requested(int from, int to);

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void enterEvent(QEnterEvent *event) override;
    void leaveEvent(QEvent *event) override;

private:
    int index_;
    bool is_current_;
    bool is_hovered_ = false;
    bool dragging_ = false;
    QPoint drag_start_position_;
};

// Main Visual Queue Panel
class QueuePanel : public QScrollArea {
    Q_OBJECT
public:
    explicit QueuePanel(QWidget *parent = nullptr);
    void setQueue(const std::vector<Track> &tracks, int current_index);

signals:
    void item_clicked(int index);
    void item_removed(int index);
    void item_moved(int from, int to);
    void clear_requested();

private:
    void clearLayout();
    bool eventFilter(QObject *watched, QEvent *event) override;
    int dropIndexAt(const QPoint &viewport_position, int source_index, int *indicator_y) const;
    void hideDropIndicator();

    QWidget *container_;
    QVBoxLayout *layout_;
    QFrame *drop_indicator_;
    int current_index_ = -1;
    int queue_size_ = 0;
};

#endif
