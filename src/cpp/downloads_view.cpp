#include "downloads_view.h"
#include <QQmlContext>
#include <QVBoxLayout>

DownloadsView::DownloadsView(QWidget *parent)
    : QWidget(parent)
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    quick_widget_ = new QQuickWidget(this);
    quick_widget_->setResizeMode(QQuickWidget::SizeRootObjectToView);
    quick_widget_->setAttribute(Qt::WA_TranslucentBackground);
    quick_widget_->setClearColor(Qt::transparent);

    quick_widget_->rootContext()->setContextProperty("DownloadsCtrl", this);
    quick_widget_->setSource(QUrl("qrc:/qml/DownloadsView.qml"));

    layout->addWidget(quick_widget_);
}

void DownloadsView::set_downloads(const std::vector<DownloadItem> &items) {
    raw_items_ = items;
    downloads_list_.clear();
    
    for (const auto &item : items) {
        QVariantMap map;
        map["id"] = QString::fromUtf8(item.video_id.data(), item.video_id.size());
        map["title"] = QString::fromUtf8(item.title.data(), item.title.size());
        map["author"] = QString::fromUtf8(item.artist.data(), item.artist.size());
        map["thumbnail"] = QString::fromUtf8(item.thumbnail_url.data(), item.thumbnail_url.size());
        map["type"] = "track"; // Or determine based on parent_playlist_id if needed
        map["status"] = QString::fromUtf8(item.status.data(), item.status.size());
        map["progress"] = item.progress;
        
        downloads_list_.append(map);
    }
    
    emit downloadsChanged();
}

void DownloadsView::set_progress(const std::string &video_id, double percent, const std::string &status) {
    emit progressUpdated(QString::fromStdString(video_id), percent, QString::fromStdString(status));
}

void DownloadsView::set_batch_progress(const std::string &parent_id, int total, int completed, double percent) {
    emit batchProgressUpdated(QString::fromStdString(parent_id), total, completed, percent);
}

void DownloadsView::clear_downloads() {
    raw_items_.clear();
    downloads_list_.clear();
    emit downloadsChanged();
}

void DownloadsView::requestTabChange(const QString &tab) {
    if (active_tab_ != tab) {
        active_tab_ = tab;
        emit tabChanged();
    }
}

void DownloadsView::requestPlay(const QString &trackId) {
    // Only tracks can be played directly from here
    std::string stdId = trackId.toStdString();
    for (const auto &item : raw_items_) {
        if (std::string(item.video_id) == stdId) {
            Track dummy;
            dummy.id = item.video_id;
            dummy.title = item.title;
            dummy.artist = item.artist;
            dummy.thumbnail = item.thumbnail_url;
            emit play_requested(dummy);
            break;
        }
    }
}

void DownloadsView::requestPlaylist(const QString &playlistId) {
    emit playlist_requested(playlistId.toStdString());
}

void DownloadsView::requestAlbum(const QString &albumId) {
    emit album_requested(albumId.toStdString());
}

void DownloadsView::requestShow(const QString &showId) {
    emit show_requested(showId.toStdString());
}
