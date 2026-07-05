#include "show_detail_view.h"
#include <QQmlContext>
#include <QVariantMap>
#include <QVBoxLayout>

ShowDetailView::ShowDetailView(QWidget *parent)
    : QWidget(parent)
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    quick_widget_ = new QQuickWidget(this);
    quick_widget_->setResizeMode(QQuickWidget::SizeRootObjectToView);
    quick_widget_->setAttribute(Qt::WA_TranslucentBackground);
    quick_widget_->setClearColor(Qt::transparent);

    quick_widget_->rootContext()->setContextProperty("ShowCtrl", this);
    quick_widget_->setSource(QUrl("qrc:/qml/ShowDetailView.qml"));

    layout->addWidget(quick_widget_);
}

void ShowDetailView::clear() {
    show_title_.clear();
    show_author_.clear();
    show_cover_.clear();
    show_description_.clear();
    dominant_color_.clear();
    episodes_list_.clear();
    raw_episodes_.clear();
    
    view_state_ = "loading";
    emit viewStateChanged();
    emit showInfoChanged();
    emit episodesChanged();
}

void ShowDetailView::set_show_info(const Show &show) {
    current_show_ = show;
    show_title_ = QString::fromUtf8(show.title.data(), show.title.size());
    show_author_ = QString::fromUtf8(show.author.data(), show.author.size());
    show_cover_ = QString::fromUtf8(show.thumbnail.data(), show.thumbnail.size());
    show_description_ = QString::fromUtf8(show.description.data(), show.description.size());
    
    extractDominantColor(show_cover_);
    
    emit showInfoChanged();
}

void ShowDetailView::set_episodes(const std::vector<Episode> &episodes) {
    raw_episodes_ = episodes;
    episodes_list_.clear();
    
    for (const auto &episode : episodes) {
        QVariantMap map;
        map["id"] = QString::fromUtf8(episode.id.data(), episode.id.size());
        map["title"] = QString::fromUtf8(episode.title.data(), episode.title.size());
        map["description"] = QString::fromUtf8(episode.description.data(), episode.description.size());
        map["publishedAt"] = "";
        
        int total_seconds = episode.duration_ms / 1000;
        int minutes = total_seconds / 60;
        int seconds = total_seconds % 60;
        QString durationStr = QString("%1:%2").arg(minutes).arg(seconds, 2, 10, QChar('0'));
        
        map["duration"] = durationStr;
        
        episodes_list_.append(map);
    }
    
    view_state_ = "content";
    emit viewStateChanged();
    emit episodesChanged();
}

void ShowDetailView::extractDominantColor(const QString &thumbnailUrl) {
    dominant_color_ = "#18181a";
}

void ShowDetailView::requestPlayEpisode(int index) {
    if (index >= 0 && index < raw_episodes_.size()) {
        emit play_episode_requested(raw_episodes_[index]);
    }
}
