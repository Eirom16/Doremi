#include "artist_detail_view.h"
#include <QQmlContext>
#include <QVariantMap>
#include <QVBoxLayout>

ArtistDetailView::ArtistDetailView(QWidget *parent)
    : QWidget(parent)
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    quick_widget_ = new QQuickWidget(this);
    quick_widget_->setResizeMode(QQuickWidget::SizeRootObjectToView);
    quick_widget_->setAttribute(Qt::WA_TranslucentBackground);
    quick_widget_->setClearColor(Qt::transparent);

    quick_widget_->rootContext()->setContextProperty("ArtistCtrl", this);
    quick_widget_->setSource(QUrl("qrc:/qml/ArtistDetailView.qml"));

    layout->addWidget(quick_widget_);
}

void ArtistDetailView::clear() {
    artist_name_.clear();
    artist_avatar_.clear();
    dominant_color_.clear();
    top_tracks_list_.clear();
    albums_list_.clear();
    singles_list_.clear();
    raw_top_tracks_.clear();
    
    view_state_ = "loading";
    emit viewStateChanged();
    emit artistInfoChanged();
    emit tracksChanged();
    emit albumsChanged();
}

void ArtistDetailView::set_artist_info(const Artist &artist) {
    current_artist_id_ = std::string(artist.id);
    artist_name_ = QString::fromUtf8(artist.name.data(), artist.name.size());
    artist_avatar_ = QString::fromUtf8(artist.thumbnail.data(), artist.thumbnail.size());
    
    extractDominantColor(artist_avatar_);
    
    emit artistInfoChanged();
}

void ArtistDetailView::set_artist_tracks(const std::vector<Track> &tracks,
                                         const std::vector<Album> &albums,
                                         const std::vector<Album> &singles) {
    raw_top_tracks_ = tracks;
    top_tracks_list_.clear();
    
    for (const auto &track : tracks) {
        QVariantMap map;
        map["id"] = QString::fromUtf8(track.id.data(), track.id.size());
        map["title"] = QString::fromUtf8(track.title.data(), track.title.size());
        map["artist"] = QString::fromUtf8(track.artist.data(), track.artist.size());
        map["album"] = QString::fromUtf8(track.album.data(), track.album.size());
        map["thumbnail"] = QString::fromUtf8(track.thumbnail.data(), track.thumbnail.size());
        
        int total_seconds = track.duration_ms / 1000;
        int minutes = total_seconds / 60;
        int seconds = total_seconds % 60;
        QString durationStr = QString("%1:%2").arg(minutes).arg(seconds, 2, 10, QChar('0'));
        
        map["duration"] = durationStr;
        
        top_tracks_list_.append(map);
    }
    
    albums_list_ = convertAlbumsToVariantList(albums);
    singles_list_ = convertAlbumsToVariantList(singles);
    
    view_state_ = "content";
    emit viewStateChanged();
    emit tracksChanged();
    emit albumsChanged();
}

QVariantList ArtistDetailView::convertAlbumsToVariantList(const std::vector<Album> &albums) {
    QVariantList list;
    for (const auto &album : albums) {
        QVariantMap map;
        map["id"] = QString::fromUtf8(album.id.data(), album.id.size());
        map["title"] = QString::fromUtf8(album.title.data(), album.title.size());
        map["artist"] = QString::fromUtf8(album.artist.data(), album.artist.size());
        map["thumbnail"] = QString::fromUtf8(album.thumbnail.data(), album.thumbnail.size());
        map["type"] = "Álbum";
        list.append(map);
    }
    return list;
}

void ArtistDetailView::extractDominantColor(const QString &thumbnailUrl) {
    dominant_color_ = "#18181a";
}

void ArtistDetailView::requestPlay(int index) {
    if (index >= 0 && index < raw_top_tracks_.size()) {
        emit play_requested(raw_top_tracks_[index]);
    }
}

void ArtistDetailView::requestAlbum(const QString &albumId) {
    emit album_requested(albumId.toStdString());
    emit album_clicked(albumId.toStdString());
}
