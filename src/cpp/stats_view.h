#ifndef DOREMI_STATS_VIEW_H
#define DOREMI_STATS_VIEW_H

#include <QWidget>
#include <QScrollArea>
#include <QVBoxLayout>
#include <QLabel>
#include "components/stat_card.h"
#include "components/bar_chart.h"

// Row widget for Top Tracks list in StatsView
class TopTrackRow : public QWidget {
    Q_OBJECT
public:
    TopTrackRow(int rank, const QString &title, const QString &artist,
                int plays, int max_plays, const QString &thumbnail, QWidget *parent = nullptr);

signals:
    void clicked(const std::string &info);

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void enterEvent(QEnterEvent *event) override;
    void leaveEvent(QEvent *event) override;

private:
    QString title_;
    QString artist_;
};

// Main Stats View Dashboard
class StatsView : public QWidget {
    Q_OBJECT
public:
    explicit StatsView(QWidget *parent = nullptr);
    void setStatsData(const QString &total_play_time, int total_plays, int unique_artists,
                      const QList<int> &weekly_activity, const QStringList &top_titles,
                      const QStringList &top_artists, const QList<int> &top_plays,
                      const QStringList &top_thumbnails);

signals:
    void play_requested(const std::string &info);

protected:
    void showEvent(QShowEvent *event) override;

private:
    void setupLayout();
    void buildTopTracks(const QStringList &titles, const QStringList &artists,
                         const QList<int> &plays, const QStringList &thumbnails);

    QScrollArea *scroll_area_;
    QWidget *scroll_content_;
    QVBoxLayout *main_layout_;

    // Stat Cards
    StatCard *card_time_;
    StatCard *card_plays_;
    StatCard *card_artists_;

    // Bar Chart
    BarChart *bar_chart_;

    // Top Tracks container
    QWidget *top_tracks_widget_;
    QVBoxLayout *top_tracks_layout_;
};

#endif
