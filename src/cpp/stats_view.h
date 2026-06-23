#ifndef DOREMI_STATS_VIEW_H
#define DOREMI_STATS_VIEW_H

#include <QWidget>
#include <QVBoxLayout>
#include <QLabel>
#include <QString>
#include "components/stat_card.h"
#include "components/bar_chart.h"
#include "doremi/src/bridge.rs.h"

class TopTrackRow : public QWidget {
    Q_OBJECT
public:
    TopTrackRow(int rank, const Track &track,
                int plays, int max_plays, QWidget *parent = nullptr);

signals:
    void clicked(Track track);

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void enterEvent(QEnterEvent *event) override;
    void leaveEvent(QEvent *event) override;

private:
    Track track_;
};

class StatsView : public QWidget {
    Q_OBJECT
public:
    explicit StatsView(QWidget *parent = nullptr);
    void setStatsData(const StatsData &stats);
    void update_theme();

signals:
    void play_requested(Track track);

protected:
    void showEvent(QShowEvent *event) override;

private:
    void setupLayout();
    void buildTopTracks(const std::vector<Track> &tracks, const std::vector<int> &plays);
    void exportStatsAsJson();
    void exportStatsAsCsv();
    bool writeStatsJson(const QString &path) const;
    bool writeStatsCsv(const QString &path) const;

    QVBoxLayout *main_layout_;

    StatCard *card_time_;
    StatCard *card_plays_;
    StatCard *card_artists_;

    BarChart *bar_chart_;

    QWidget *top_tracks_widget_;
    QVBoxLayout *top_tracks_layout_;

    StatsData current_stats_;
    bool has_stats_ = false;
};

#endif
