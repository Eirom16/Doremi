#ifndef DOREMI_TRENDING_VIEW_H
#define DOREMI_TRENDING_VIEW_H

#include <QWidget>
#include <QQuickWidget>
#include <QVariantList>
#include <QVariantMap>
#include "doremi/src/bridge.rs.h"

class TrendingView : public QWidget {
    Q_OBJECT
    Q_PROPERTY(QVariantList items READ items NOTIFY itemsChanged)
    Q_PROPERTY(QString viewState READ viewState NOTIFY viewStateChanged)

public:
    explicit TrendingView(QWidget *parent = nullptr);
    void update_theme() {}
    void clear_items();
    void add_item(const HomeCard &item);
    void set_state(const std::string &state, const std::string &message);

    QVariantList items() const { return items_; }
    QString viewState() const { return view_state_; }

    Q_INVOKABLE void requestPlay(const QString &id, const QString &type, const QString &title, const QString &artist, const QString &thumbnail);
    Q_INVOKABLE void requestNavigate(const QString &id, const QString &type);
    Q_INVOKABLE void requestRetry();
signals:
    void play_requested(Track track);
    void album_requested(const std::string &browse_id);
    void artist_requested(const std::string &browse_id);
    void playlist_requested(const std::string &playlist_id);
    void retry_requested();
    
    void itemsChanged();
    void viewStateChanged();
    
private:
    QQuickWidget *quick_widget_ = nullptr;
    QVariantList items_;
    QString view_state_;
};

#endif
