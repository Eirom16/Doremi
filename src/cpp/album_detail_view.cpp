#include "album_detail_view.h"
#include <QQmlContext>
#include <QVariantMap>
#include <QColor>

AlbumDetailView::AlbumDetailView(QWidget *parent)
    : QWidget(parent)
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    quick_widget_ = new QQuickWidget(this);
    quick_widget_->setResizeMode(QQuickWidget::SizeRootObjectToView);
    quick_widget_->setAttribute(Qt::WA_TranslucentBackground);
    quick_widget_->setClearColor(Qt::transparent);

    quick_widget_->rootContext()->setContextProperty("AlbumCtrl", this);
    quick_widget_->setSource(QUrl("qrc:/qml/AlbumDetailView.qml"));

    layout->addWidget(quick_widget_);
}

void AlbumDetailView::set_state(const std::string &state, const std::string &message) {
    view_state_ = QString::fromUtf8(state.data(), state.size());
    emit viewStateChanged();
}

void AlbumDetailView::clear() {
    album_title_.clear();
    album_artist_.clear();
    album_cover_.clear();
    album_year_.clear();
    album_song_count_.clear();
    dominant_color_.clear();
    tracks_list_.clear();
    raw_tracks_.clear();
    
    emit albumInfoChanged();
    emit tracksChanged();
}

void AlbumDetailView::set_album_info(const Album &album) {
    current_album_ = album;
    album_title_ = QString::fromUtf8(album.title.data(), album.title.size());
    album_artist_ = QString::fromUtf8(album.artist.data(), album.artist.size());
    album_cover_ = QString::fromUtf8(album.thumbnail.data(), album.thumbnail.size());
    album_year_ = QString::fromUtf8(album.year.data(), album.year.size());
    album_song_count_ = QString::number(album.track_count);
    
    extractDominantColor(album_cover_);
    
    emit albumInfoChanged();
}

void AlbumDetailView::set_album_tracks(const std::vector<Track> &tracks) {
    raw_tracks_ = tracks;
    tracks_list_.clear();
    
    for (const auto &track : tracks) {
        QVariantMap map;
        map["id"] = QString::fromUtf8(track.id.data(), track.id.size());
        map["title"] = QString::fromUtf8(track.title.data(), track.title.size());
        map["artist"] = QString::fromUtf8(track.artist.data(), track.artist.size());
        
        int total_seconds = track.duration_ms / 1000;
        int minutes = total_seconds / 60;
        int seconds = total_seconds % 60;
        QString durationStr = QString("%1:%2").arg(minutes).arg(seconds, 2, 10, QChar('0'));
        
        map["duration"] = durationStr;
        
        tracks_list_.append(map);
    }
    
    emit tracksChanged();
}

void AlbumDetailView::extractDominantColor(const QString &thumbnailUrl) {
    // For now, use a fallback color. In a real implementation, 
    // we would use a similar approach to NowPlayingView to extract colors.
    dominant_color_ = "#18181a";
}

void AlbumDetailView::requestPlay(int index) {
    if (index >= 0 && index < raw_tracks_.size()) {
        emit play_requested(raw_tracks_[index]);
    }
}

void AlbumDetailView::requestPlayAll() {
    if (!raw_tracks_.empty()) {
        emit play_all_requested(raw_tracks_);
    }
}

void AlbumDetailView::requestShuffle() {
    if (!raw_tracks_.empty()) {
        emit shuffle_requested(raw_tracks_);
    }
}

void AlbumDetailView::requestDownload() {
    if (!raw_tracks_.empty()) {
        emit download_all_requested(raw_tracks_, std::string(current_album_.id), std::string(current_album_.title), std::string(current_album_.thumbnail));
    }
}

void AlbumDetailView::requestArtist() {
    if (!current_album_.artist_id.empty()) {
        emit artist_requested(std::string(current_album_.artist_id));
    }
}
