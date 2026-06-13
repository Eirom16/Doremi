#ifndef DOREMI_DOWNLOADS_VIEW_H
#define DOREMI_DOWNLOADS_VIEW_H

#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QScrollArea>
#include <vector>
#include <string>
#include "doremi/src/bridge.rs.h"

class DownloadsView : public QWidget {
    Q_OBJECT
public:
    explicit DownloadsView(QWidget *parent = nullptr);
    void set_downloads(const std::vector<std::string> &titles,
                       const std::vector<std::string> &artists,
                       const std::vector<std::string> &thumbnails);
    void clear_downloads();
signals:
    void play_requested(Track track);
private:
    QWidget *make_download_row(const std::string &title, const std::string &artist,
                               const std::string &thumbnail_path);
    QVBoxLayout *list_;
    QLabel *status_label_;
};

#endif
