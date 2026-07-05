#include "trending_view.h"
#include <QQmlContext>
#include <QVBoxLayout>

TrendingView::TrendingView(QWidget *parent)
    : QWidget(parent)
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    quick_widget_ = new QQuickWidget(this);
    quick_widget_->setResizeMode(QQuickWidget::SizeRootObjectToView);
    quick_widget_->rootContext()->setContextProperty("TrendingCtrl", this);
    
    quick_widget_->setAttribute(Qt::WA_TranslucentBackground);
    quick_widget_->setClearColor(Qt::transparent);
    
    quick_widget_->setSource(QUrl("qrc:/qml/TrendingView.qml"));
    
    layout->addWidget(quick_widget_);
    
    view_state_ = "loading";
}

void TrendingView::clear_items() {
    items_.clear();
    emit itemsChanged();
}

void TrendingView::add_item(const HomeCard &item) {
    QVariantMap map;
    map["id"] = QString::fromStdString(static_cast<std::string>(item.id));
    map["title"] = QString::fromStdString(static_cast<std::string>(item.title));
    map["subtitle"] = QString::fromStdString(static_cast<std::string>(item.subtitle));
    map["thumbnail"] = QString::fromStdString(static_cast<std::string>(item.thumbnail));
    map["itemType"] = QString::fromStdString(static_cast<std::string>(item.item_type));
    items_.append(map);
    emit itemsChanged();
}

void TrendingView::set_state(const std::string &state, const std::string &message) {
    QString newState = QString::fromStdString(state);
    if (view_state_ != newState) {
        view_state_ = newState;
        emit viewStateChanged();
    }
}

void TrendingView::requestPlay(const QString &id, const QString &type, const QString &title, const QString &artist, const QString &thumbnail) {
    Track track;
    track.id = id.toStdString();
    track.title = title.toStdString();
    track.artist = artist.toStdString();
    track.thumbnail = thumbnail.toStdString();
    emit play_requested(track);
}

void TrendingView::requestNavigate(const QString &id, const QString &type) {
    if (id.isEmpty()) return;
    if (type == "album") emit album_requested(id.toStdString());
    else if (type == "artist") emit artist_requested(id.toStdString());
    else emit playlist_requested(id.toStdString());
}

void TrendingView::requestRetry() {
    emit retry_requested();
}
