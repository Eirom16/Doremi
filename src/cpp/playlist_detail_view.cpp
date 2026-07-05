#include "playlist_detail_view.h"
#include <QQmlContext>
#include <QVariantMap>
#include <QInputDialog>
#include <QMessageBox>
#include <QVBoxLayout>

PlaylistDetailView::PlaylistDetailView(QWidget *parent)
    : QWidget(parent)
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    quick_widget_ = new QQuickWidget(this);
    quick_widget_->setResizeMode(QQuickWidget::SizeRootObjectToView);
    quick_widget_->setAttribute(Qt::WA_TranslucentBackground);
    quick_widget_->setClearColor(Qt::transparent);

    quick_widget_->rootContext()->setContextProperty("PlaylistCtrl", this);
    quick_widget_->setSource(QUrl("qrc:/qml/PlaylistDetailView.qml"));

    layout->addWidget(quick_widget_);
}

void PlaylistDetailView::clear() {
    playlist_title_.clear();
    playlist_author_.clear();
    playlist_cover_.clear();
    playlist_song_count_.clear();
    dominant_color_.clear();
    tracks_list_.clear();
    raw_tracks_.clear();
    
    view_state_ = "loading";
    emit viewStateChanged();
    emit playlistInfoChanged();
    emit tracksChanged();
}

void PlaylistDetailView::set_playlist_info(const Playlist &playlist) {
    current_playlist_ = playlist;
    playlist_title_ = QString::fromUtf8(playlist.name.data(), playlist.name.size());
    playlist_author_ = playlist.owner.empty() ? "Tú" : QString::fromUtf8(playlist.owner.data(), playlist.owner.size());
    playlist_cover_ = QString::fromUtf8(playlist.thumbnail.data(), playlist.thumbnail.size());
    playlist_song_count_ = QString::number(playlist.track_count);
    
    extractDominantColor(playlist_cover_);
    
    view_state_ = "content";
    emit viewStateChanged();
    emit playlistInfoChanged();
}

void PlaylistDetailView::set_playlist_tracks(const std::vector<Track> &tracks) {
    raw_tracks_ = tracks;
    tracks_list_.clear();
    
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
        
        tracks_list_.append(map);
    }
    
    emit tracksChanged();
}

void PlaylistDetailView::extractDominantColor(const QString &thumbnailUrl) {
    dominant_color_ = "#18181a";
}

void PlaylistDetailView::requestPlay(int index) {
    if (index >= 0 && index < raw_tracks_.size()) {
        emit play_requested(raw_tracks_[index]);
    }
}

void PlaylistDetailView::requestPlayAll() {
    if (!raw_tracks_.empty()) {
        emit play_all_requested(raw_tracks_);
    }
}

void PlaylistDetailView::requestShuffle() {
    if (!raw_tracks_.empty()) {
        emit shuffle_requested(raw_tracks_);
    }
}

void PlaylistDetailView::requestDownload() {
    if (!raw_tracks_.empty()) {
        emit download_all_requested(raw_tracks_, std::string(current_playlist_.id), std::string(current_playlist_.name), std::string(current_playlist_.thumbnail));
    }
}

void PlaylistDetailView::requestRemoveTrack(int index) {
    if (index >= 0 && index < raw_tracks_.size()) {
        emit remove_track_from_playlist_requested(std::string(current_playlist_.id), std::string(raw_tracks_[index].id));
    }
}

void PlaylistDetailView::requestRename() {
    bool ok;
    QString newName = QInputDialog::getText(this, "Renombrar Playlist",
                                            "Nuevo nombre:", QLineEdit::Normal,
                                            QString::fromUtf8(current_playlist_.name.data(), current_playlist_.name.size()), &ok);
    if (ok && !newName.isEmpty()) {
        emit rename_playlist_requested(std::string(current_playlist_.id), newName.toStdString());
    }
}

void PlaylistDetailView::requestDelete() {
    QMessageBox::StandardButton reply = QMessageBox::question(
        this, "Eliminar Playlist", 
        "¿Estás seguro de que quieres eliminar esta playlist?",
        QMessageBox::Yes | QMessageBox::No
    );
    if (reply == QMessageBox::Yes) {
        emit delete_playlist_requested(std::string(current_playlist_.id));
    }
}
