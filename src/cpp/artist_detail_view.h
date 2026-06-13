#ifndef DOREMI_ARTIST_DETAIL_VIEW_H
#define DOREMI_ARTIST_DETAIL_VIEW_H

#include <QWidget>
#include <QScrollArea>
#include <QVBoxLayout>
#include <QLabel>
#include <QHBoxLayout>
#include <QMouseEvent>
#include <vector>
#include <string>

// Internal track row widget for artist tracks
class ArtistTrackRow : public QWidget {
    Q_OBJECT
public:
    ArtistTrackRow(const QString &title, const QString &album,
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

class ArtistDetailView : public QWidget {
    Q_OBJECT
public:
    explicit ArtistDetailView(QWidget *parent = nullptr);

    void set_artist_info(const std::string &name, const std::string &thumbnail,
                         const std::string &subscriber_count, const std::string &description);

    void set_artist_tracks(const std::vector<std::string> &titles,
                           const std::vector<std::string> &albums,
                           const std::vector<std::string> &durations,
                           const std::vector<std::string> &item_ids);

    void clear();

signals:
    void play_requested(const std::string &info);
    void back_requested();

private:
    void setupLayout();

    QScrollArea *scroll_area_;
    QWidget *scroll_content_;
    QVBoxLayout *content_layout_;

    // Header widgets
    QLabel *avatar_label_;
    QLabel *name_label_;
    QLabel *meta_label_;
    QLabel *desc_label_;

    // Tracks container
    QWidget *tracks_widget_;
    QVBoxLayout *tracks_layout_;
};

#endif
