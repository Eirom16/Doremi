#ifndef DOREMI_HISTORY_VIEW_H
#define DOREMI_HISTORY_VIEW_H

#include <QWidget>
#include <QQuickWidget>
#include <QVariantList>
#include <vector>
#include <string>

#include "doremi/src/bridge.rs.h"

class HistoryView : public QWidget {
    Q_OBJECT
    Q_PROPERTY(QVariantList history READ history NOTIFY historyChanged)

public:
    explicit HistoryView(QWidget *parent = nullptr);
    void update_theme() {} // Kept for compatibility

    void set_history(const std::vector<Track> &tracks,
                     const std::vector<std::string> &played_at,
                     const std::vector<std::string> &feedback_tokens);
    void clear_history();

    QVariantList history() const { return history_list_; }

    Q_INVOKABLE void requestPlay(const QString &trackId);

signals:
    void historyChanged();
    void play_requested(Track track);

private:
    QQuickWidget *quick_widget_ = nullptr;
    QVariantList history_list_;
    std::vector<Track> raw_tracks_;
    
    Track getTrackById(const QString &id);
    QString formatRelativeTime(const QString &played_at) const;
};

#endif
