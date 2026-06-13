#ifndef DOREMI_HISTORY_VIEW_H
#define DOREMI_HISTORY_VIEW_H

#include <QWidget>
#include <QScrollArea>
#include <QVBoxLayout>
#include <QLabel>
#include <vector>
#include <string>

// Row widget for a single history entry
class HistoryRow : public QWidget {
    Q_OBJECT
public:
    HistoryRow(const QString &title, const QString &artist,
               const QString &duration, const QString &thumbnail,
               const QString &played_at, const std::string &item_id,
               QWidget *parent = nullptr);

signals:
    void play_requested(const std::string &info);
    void download_requested(const std::string &info);

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void enterEvent(QEnterEvent *event) override;
    void leaveEvent(QEvent *event) override;

private:
    QString title_;
    QString artist_;
    std::string item_id_;
};

// Main History View
class HistoryView : public QWidget {
    Q_OBJECT
public:
    explicit HistoryView(QWidget *parent = nullptr);

    void set_history(const std::vector<std::string> &titles,
                     const std::vector<std::string> &artists,
                     const std::vector<std::string> &durations,
                     const std::vector<std::string> &thumbnails,
                     const std::vector<std::string> &played_at,
                     const std::vector<std::string> &item_ids);

    void clear_history();

signals:
    void play_requested(const std::string &info);
    void download_requested(const std::string &info);

protected:
    void showEvent(QShowEvent *event) override;

private:
    void setupLayout();
    QString formatRelativeTime(const QString &played_at) const;
    QString getGroupLabel(const QString &played_at) const;

    QScrollArea *scroll_area_;
    QWidget *scroll_content_;
    QVBoxLayout *content_layout_;
    QLabel *empty_label_;
};

#endif
