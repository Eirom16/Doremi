#ifndef DOREMI_DOWNLOADS_VIEW_H
#define DOREMI_DOWNLOADS_VIEW_H

#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QProgressBar>
#include <QScrollArea>
#include <QMap>
#include <QSet>
#include <QButtonGroup>
#include <vector>
#include <string>
#include "doremi/src/bridge.rs.h"
#include "components/empty_state.h"


class DownloadsView : public QWidget {
    Q_OBJECT
public:
    explicit DownloadsView(QWidget *parent = nullptr);
    void set_downloads(const std::vector<DownloadItem> &items);
    void set_progress(const std::string &video_id, double percent, const std::string &status);
    void set_batch_progress(const std::string &parent_id, int total, int completed, double percent);
    void clear_downloads();
    void update_theme();
    void update_view();
signals:
    void play_requested(Track track);
    void playlist_requested(const std::string &playlist_id);
    void album_requested(const std::string &album_id);
    void show_requested(const std::string &show_id);
private:
    QWidget *make_download_row(const std::string &video_id, const std::string &title,
                               const std::string &artist, const std::string &thumbnail_path,
                               double progress, const std::string &status);
    QWidget *make_batch_row(const std::string &parent_id, const std::string &parent_title,
                            int total, int completed, double percent);
    void update_row(QWidget *row, double percent, const std::string &status);
    void update_batch_row(QWidget *row, int total, int completed, double percent);
    QVBoxLayout *list_;
    QVBoxLayout *rows_layout_;
    EmptyState *status_label_;
    QMap<std::string, QWidget*> row_map_;
    QMap<std::string, QWidget*> batch_row_map_;
    std::string active_tab_;
    std::vector<DownloadItem> all_downloads_;
    std::vector<QPushButton*> tab_btns_;
};

#endif
