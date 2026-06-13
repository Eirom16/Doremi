#ifndef DOREMI_SEARCH_VIEW_H
#define DOREMI_SEARCH_VIEW_H

#include <QWidget>
#include <QVBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QHBoxLayout>
#include <QScrollArea>
#include <vector>
#include <string>

#include "widgets.h"

class SearchView : public QWidget {
    Q_OBJECT
public:
    explicit SearchView(QWidget *parent = nullptr);
    void set_query(const std::string &query);
    void set_results(const std::vector<std::string> &songs,
                     const std::vector<std::string> &artists,
                     const std::vector<std::string> &albums);
    void set_recent_searches(const std::vector<std::string> &queries);
signals:
    void filter_changed(const std::string &filter);
    void play_requested(const std::string &title_artist);
    void search_requested(const std::string &query);
    void add_favorite_requested(const std::string &title_artist);
    void download_requested(const std::string &title_artist);
private:
    QLabel *header_;
    QVBoxLayout *results_;
    QHBoxLayout *filters_;
    std::vector<QPushButton *> filter_btns_;
    void set_active_filter(const std::string &filter);
    void show_recent_searches(const std::vector<std::string> &queries);
    void show_results(const std::vector<std::string> &songs,
                      const std::vector<std::string> &artists,
                      const std::vector<std::string> &albums);
    bool showing_recent_ = false;
};

#endif
