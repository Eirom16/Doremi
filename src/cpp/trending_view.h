#ifndef DOREMI_TRENDING_VIEW_H
#define DOREMI_TRENDING_VIEW_H

#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QScrollArea>
#include <QPushButton>
#include <vector>
#include <string>
#include "doremi/src/bridge.rs.h"

class TrendingView : public QWidget {
    Q_OBJECT
public:
    explicit TrendingView(QWidget *parent = nullptr);
    void clear_items();
    void add_item(const HomeCard &item);
    void set_state(const std::string &state, const std::string &message);
    void update_theme();
signals:
    void play_requested(Track track);
    void album_requested(const std::string &browse_id);
    void artist_requested(const std::string &browse_id);
    void playlist_requested(const std::string &playlist_id);
    void retry_requested();
private:
    QWidget *make_trending_card(const HomeCard &item);
    QVBoxLayout *list_;
    QWidget *state_widget_ = nullptr;
};

#endif
