#ifndef DOREMI_PLAYLIST_DETAIL_VIEW_H
#define DOREMI_PLAYLIST_DETAIL_VIEW_H

#include <QWidget>
#include <QScrollArea>
#include <QVBoxLayout>
#include <QLabel>
#include <QHBoxLayout>
#include <QMouseEvent>
#include <vector>
#include <string>

// Internal track row widget for playlist tracks
class PlaylistTrackRow : public QWidget {
    Q_OBJECT
public:
    PlaylistTrackRow(int num, const QString &title, const QString &artist,
                     const QString &duration, const std::string &item_id,
                     QWidget *parent = nullptr);

signals:
    void play_requested(const std::string &info);

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void enterEvent(QEnterEvent *event) override;
    void leaveEvent(QEvent *event) override;

private:
    std::string item_id_;
};

class PlaylistDetailView : public QWidget {
    Q_OBJECT
public:
    explicit PlaylistDetailView(QWidget *parent = nullptr);

    void set_playlist_info(const std::string &name, const std::string &description,
                           const std::string &thumbnail, int32_t track_count);

    void set_playlist_tracks(const std::vector<std::string> &titles,
                             const std::vector<std::string> &artists,
                             const std::vector<std::string> &durations,
                             const std::vector<std::string> &item_ids);

    void clear();

signals:
    void play_requested(const std::string &info);
    void play_all_requested();
    void shuffle_requested();
    void back_requested();

private:
    void setupLayout();

    QScrollArea *scroll_area_;
    QWidget *scroll_content_;
    QVBoxLayout *content_layout_;

    // Header
    QLabel *cover_label_;
    QLabel *title_label_;
    QLabel *desc_label_;
    QLabel *meta_label_;

    // Tracks
    QWidget *tracks_widget_;
    QVBoxLayout *tracks_layout_;
};

#endif
