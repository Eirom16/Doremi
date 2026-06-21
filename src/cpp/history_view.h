#ifndef DOREMI_HISTORY_VIEW_H
#define DOREMI_HISTORY_VIEW_H

#include <QWidget>
#include <QVBoxLayout>
#include <QLabel>
#include <vector>
#include <string>
#include "doremi/src/bridge.rs.h"

class HistoryRow : public QWidget {
    Q_OBJECT
public:
    HistoryRow(const Track &track,
               const std::string &played_at,
               const std::string &feedback_token,
               QWidget *parent = nullptr);

signals:
    void play_requested(Track track);
    void delete_requested(const std::string &track_id, const std::string &feedback_token);

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void contextMenuEvent(QContextMenuEvent *event) override;
    void enterEvent(QEnterEvent *event) override;
    void leaveEvent(QEvent *event) override;

private:
    Track track_;
    QString title_;
    QString artist_;
    std::string feedback_token_;
};

class HistoryView : public QWidget {
    Q_OBJECT
public:
    explicit HistoryView(QWidget *parent = nullptr);

    void set_history(const std::vector<Track> &tracks,
                     const std::vector<std::string> &played_at,
                     const std::vector<std::string> &feedback_tokens);
    void clear_history();

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
