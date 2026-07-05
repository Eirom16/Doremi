#ifndef DOREMI_SHOW_DETAIL_VIEW_H
#define DOREMI_SHOW_DETAIL_VIEW_H

#include <QWidget>
#include <QQuickWidget>
#include <QVariantList>
#include <QVariantMap>
#include <vector>
#include <string>
#include "doremi/src/bridge.rs.h"

class ShowDetailView : public QWidget {
    Q_OBJECT
    Q_PROPERTY(QString viewState READ viewState NOTIFY viewStateChanged)
    Q_PROPERTY(QString showTitle READ showTitle NOTIFY showInfoChanged)
    Q_PROPERTY(QString showAuthor READ showAuthor NOTIFY showInfoChanged)
    Q_PROPERTY(QString showCover READ showCover NOTIFY showInfoChanged)
    Q_PROPERTY(QString showDescription READ showDescription NOTIFY showInfoChanged)
    Q_PROPERTY(QString dominantColor READ dominantColor NOTIFY showInfoChanged)
    Q_PROPERTY(QVariantList episodes READ episodes NOTIFY episodesChanged)

public:
    explicit ShowDetailView(QWidget *parent = nullptr);

    void set_show_info(const Show &show);
    void set_episodes(const std::vector<Episode> &episodes);
    void clear();
    void update_theme() {}

    QString viewState() const { return view_state_; }
    QString showTitle() const { return show_title_; }
    QString showAuthor() const { return show_author_; }
    QString showCover() const { return show_cover_; }
    QString showDescription() const { return show_description_; }
    QString dominantColor() const { return dominant_color_; }
    QVariantList episodes() const { return episodes_list_; }

    Q_INVOKABLE void requestPlayEpisode(int index);

signals:
    void viewStateChanged();
    void showInfoChanged();
    void episodesChanged();

    void back_requested();
    void play_episode_requested(Episode episode);

private:
    QQuickWidget *quick_widget_ = nullptr;
    
    QString view_state_ = "loading";
    QString show_title_;
    QString show_author_;
    QString show_cover_;
    QString show_description_;
    QString dominant_color_;
    
    QVariantList episodes_list_;
    
    std::vector<Episode> raw_episodes_;
    Show current_show_;
    
    void extractDominantColor(const QString &thumbnailUrl);
};

#endif
