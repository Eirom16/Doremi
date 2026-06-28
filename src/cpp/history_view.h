#ifndef DOREMI_HISTORY_VIEW_H
#define DOREMI_HISTORY_VIEW_H

#include <QWidget>
#include <QVBoxLayout>
#include <QLabel>
#include <vector>
#include <string>
#include "components/track_row.h"
#include "doremi/src/bridge.rs.h"

class HistoryView : public QWidget {
    Q_OBJECT
public:
    explicit HistoryView(QWidget *parent = nullptr);

    void set_history(const std::vector<Track> &tracks,
                     const std::vector<std::string> &played_at,
                     const std::vector<std::string> &feedback_tokens);
    void clear_history();
    void update_theme();

signals:
    void play_requested(Track track);

protected:
    void showEvent(QShowEvent *event) override;

private:
    void setupLayout();
    QString formatRelativeTime(const QString &played_at) const;
    QString getGroupLabel(const QString &played_at) const;

    QVBoxLayout *content_layout_;
    QLabel *empty_label_;
};

#endif
