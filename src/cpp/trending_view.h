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
    void add_item(const std::string &title, const std::string &subtitle,
                  const std::string &thumbnail_path);
signals:
    void play_requested(Track track);
private:
    QWidget *make_trending_card(const std::string &title, const std::string &subtitle,
                                const std::string &thumbnail_path);
    QVBoxLayout *list_;
};

#endif
