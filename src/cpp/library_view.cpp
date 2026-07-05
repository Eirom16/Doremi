#include "library_view.h"
#include <QQmlContext>
#include <QVariantMap>
#include <QVBoxLayout>
#include <QInputDialog>

LibraryView::LibraryView(QWidget *parent)
    : QWidget(parent)
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    quick_widget_ = new QQuickWidget(this);
    quick_widget_->setResizeMode(QQuickWidget::SizeRootObjectToView);
    quick_widget_->setAttribute(Qt::WA_TranslucentBackground);
    quick_widget_->setClearColor(Qt::transparent);

    quick_widget_->rootContext()->setContextProperty("LibraryCtrl", this);
    quick_widget_->setSource(QUrl("qrc:/qml/LibraryView.qml"));

    layout->addWidget(quick_widget_);
}

void LibraryView::set_playlists(const std::vector<Playlist> &playlists) {
    playlists_.clear();
    for (const auto &p : playlists) {
        QVariantMap map;
        map["id"] = QString::fromUtf8(p.id.data(), p.id.size());
        map["title"] = QString::fromUtf8(p.name.data(), p.name.size());
        map["author"] = QString::fromUtf8(p.owner.data(), p.owner.size());
        map["thumbnail"] = QString::fromUtf8(p.thumbnail.data(), p.thumbnail.size());
        map["trackCount"] = p.track_count;
        playlists_.append(map);
    }
    emit dataChanged();
}

void LibraryView::set_songs(const std::vector<Track> &songs) {
    raw_songs_ = songs;
    songs_.clear();
    for (const auto &track : songs) {
        QVariantMap map;
        map["id"] = QString::fromUtf8(track.id.data(), track.id.size());
        map["title"] = QString::fromUtf8(track.title.data(), track.title.size());
        map["artist"] = QString::fromUtf8(track.artist.data(), track.artist.size());
        map["album"] = QString::fromUtf8(track.album.data(), track.album.size());
        map["thumbnail"] = QString::fromUtf8(track.thumbnail.data(), track.thumbnail.size());
        
        int total_seconds = track.duration_ms / 1000;
        int minutes = total_seconds / 60;
        int seconds = total_seconds % 60;
        map["duration"] = QString("%1:%2").arg(minutes).arg(seconds, 2, 10, QChar('0'));
        
        songs_.append(map);
    }
    emit dataChanged();
}

void LibraryView::set_albums(const std::vector<Album> &albums) {
    albums_.clear();
    for (const auto &a : albums) {
        QVariantMap map;
        map["id"] = QString::fromUtf8(a.id.data(), a.id.size());
        map["title"] = QString::fromUtf8(a.title.data(), a.title.size());
        map["artist"] = QString::fromUtf8(a.artist.data(), a.artist.size());
        map["thumbnail"] = QString::fromUtf8(a.thumbnail.data(), a.thumbnail.size());
        albums_.append(map);
    }
    emit dataChanged();
}

void LibraryView::set_artists(const std::vector<Artist> &artists) {
    artists_.clear();
    for (const auto &a : artists) {
        QVariantMap map;
        map["id"] = QString::fromUtf8(a.id.data(), a.id.size());
        map["name"] = QString::fromUtf8(a.name.data(), a.name.size());
        map["thumbnail"] = QString::fromUtf8(a.thumbnail.data(), a.thumbnail.size());
        artists_.append(map);
    }
    emit dataChanged();
}

void LibraryView::set_shows(const std::vector<Show> &shows) {
    shows_.clear();
    for (const auto &s : shows) {
        QVariantMap map;
        map["id"] = QString::fromUtf8(s.id.data(), s.id.size());
        map["title"] = QString::fromUtf8(s.title.data(), s.title.size());
        map["author"] = QString::fromUtf8(s.author.data(), s.author.size());
        map["thumbnail"] = QString::fromUtf8(s.thumbnail.data(), s.thumbnail.size());
        shows_.append(map);
    }
    emit dataChanged();
}

void LibraryView::set_library_state(const std::string &state, const std::string &message) {
    library_state_ = QString::fromStdString(state);
    state_message_ = QString::fromStdString(message);
    emit libraryStateChanged();
}

void LibraryView::set_authenticated(bool authenticated) {
    authenticated_ = authenticated;
    emit authenticatedChanged();
}

void LibraryView::requestTabChange(const QString &tab) {
    if (active_tab_ != tab) {
        active_tab_ = tab;
        emit tabChanged();
        emit tab_changed(tab.toStdString());
    }
}

Track LibraryView::getTrackById(const QString &id) {
    std::string stdId = id.toStdString();
    for (const auto &t : raw_songs_) {
        if (std::string(t.id) == stdId) return t;
    }
    return Track();
}

void LibraryView::requestPlay(const QString &trackId) {
    Track t = getTrackById(trackId);
    if (!std::string(t.id).empty()) {
        emit play_requested(t);
    }
}

void LibraryView::requestPlaylist(const QString &playlistId) {
    emit playlist_requested(playlistId.toStdString());
}

void LibraryView::requestAlbum(const QString &albumId) {
    emit album_requested(albumId.toStdString());
}

void LibraryView::requestArtist(const QString &artistId) {
    emit artist_requested(artistId.toStdString());
}

void LibraryView::requestShow(const QString &showId) {
    emit show_requested(showId.toStdString());
}

void LibraryView::requestCreatePlaylist() {
    bool ok;
    QString name = QInputDialog::getText(this, "Crear Playlist", "Nombre de la playlist:", QLineEdit::Normal, "", &ok);
    if (ok && !name.isEmpty()) {
        emit create_playlist_requested(name.toStdString(), "", "PUBLIC");
    }
}

void LibraryView::requestLogin() {
    emit login_requested();
}

void LibraryView::requestFilterSourceChange(int source) {
    emit filter_source_changed(source);
}

void LibraryView::requestRemoveFavorite(const QString &trackId) {
    emit remove_favorite_requested(trackId.toStdString());
}
