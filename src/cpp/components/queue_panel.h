#ifndef DOREMI_QUEUE_PANEL_H
#define DOREMI_QUEUE_PANEL_H

#include <QWidget>
#include <QScrollArea>
#include <QVBoxLayout>
#include <QLabel>
#include <QPushButton>
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

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void enterEvent(QEnterEvent *event) override;
    void leaveEvent(QEvent *event) override;

private:
    int index_;
    bool is_current_;
    bool is_hovered_ = false;
};

// Main Visual Queue Panel
class QueuePanel : public QScrollArea {
    Q_OBJECT
public:
    explicit QueuePanel(QWidget *parent = nullptr);
    void setQueue(const std::vector<Track> &tracks, int current_index);

signals:
    void item_clicked(int index);

private:
    void clearLayout();

    QWidget *container_;
    QVBoxLayout *layout_;
    int current_index_ = -1;
};

#endif
