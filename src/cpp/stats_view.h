#ifndef DOREMI_STATS_VIEW_H
#define DOREMI_STATS_VIEW_H

#include <QWidget>
#include <QQuickWidget>
#include <QVariantList>
#include <QVariantMap>
#include "doremi/src/bridge.rs.h"

class StatsView : public QWidget {
    Q_OBJECT
    Q_PROPERTY(QVariantMap summary READ summary NOTIFY statsChanged)
    Q_PROPERTY(QVariantList dailyPlays READ dailyPlays NOTIFY statsChanged)
    Q_PROPERTY(QVariantList topTracks READ topTracks NOTIFY statsChanged)

public:
    explicit StatsView(QWidget *parent = nullptr);
    void setStatsData(const StatsData &stats);
    void update_theme() {} // Kept for compatibility

    QVariantMap summary() const { return summary_; }
    QVariantList dailyPlays() const { return daily_plays_; }
    QVariantList topTracks() const { return top_tracks_; }

    Q_INVOKABLE void exportStatsAsJson();
    Q_INVOKABLE void exportStatsAsCsv();
    Q_INVOKABLE void requestPlay(const QString &trackId);
    Q_INVOKABLE void requestStats(int days);

signals:
    void statsChanged();
    void play_requested(Track track); // Need to construct Track to pass back to main window if used.

protected:
    void showEvent(QShowEvent *event) override;

private:
    bool writeStatsJson(const QString &path) const;
    bool writeStatsCsv(const QString &path) const;
    
    StatsData current_stats_;

    QQuickWidget *quick_widget_ = nullptr;
    QVariantMap summary_;
    QVariantList daily_plays_;
    QVariantList top_tracks_;
};

#endif
