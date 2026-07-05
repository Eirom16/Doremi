#include "search_view.h"
#include <QQmlContext>
#include <QVariantMap>
#include <QVBoxLayout>

SearchView::SearchView(QWidget *parent)
    : QWidget(parent)
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    quick_widget_ = new QQuickWidget(this);
    quick_widget_->setResizeMode(QQuickWidget::SizeRootObjectToView);
    quick_widget_->setAttribute(Qt::WA_TranslucentBackground);
    quick_widget_->setClearColor(Qt::transparent);

    quick_widget_->rootContext()->setContextProperty("SearchCtrl", this);
    quick_widget_->setSource(QUrl("qrc:/qml/SearchView.qml"));

    layout->addWidget(quick_widget_);
}

void SearchView::set_query(const std::string &query, const std::string &filter) {
    current_query_ = QString::fromUtf8(query.data(), query.size());
    active_filter_ = QString::fromStdString(filter);
    
    // Clear old results
    has_top_result_ = false;
    top_result_map_.clear();
    songs_list_.clear();
    videos_list_.clear();
    artists_list_.clear();
    albums_list_.clear();
    playlists_list_.clear();
    shows_list_.clear();
    episodes_list_.clear();
    
    emit queryChanged();
    emit filterChanged();
    emit resultsChanged();
}

void SearchView::set_results(const TopResult &top_result, bool has_top_result,
                 const std::vector<Track> &songs,
                 const std::vector<Track> &videos,
                 const std::vector<Artist> &artists,
                 const std::vector<Album> &albums,
                 const std::vector<Playlist> &playlists,
                 const std::vector<Show> &shows,
                 const std::vector<Episode> &episodes) {
                     
    has_top_result_ = has_top_result;
    raw_top_result_ = top_result;
    if (has_top_result) {
        top_result_map_["title"] = QString::fromUtf8(top_result.title.data(), top_result.title.size());
        top_result_map_["subtitle"] = QString::fromUtf8(top_result.subtitle.data(), top_result.subtitle.size());
        top_result_map_["thumbnail"] = QString::fromUtf8(top_result.thumbnail.data(), top_result.thumbnail.size());
        top_result_map_["type"] = QString::fromUtf8(top_result.item_type.data(), top_result.item_type.size());
    } else {
        top_result_map_.clear();
    }
    
    raw_songs_ = songs;
    songs_list_.clear();
    for (const auto &track : songs) {
        QVariantMap map;
        map["id"] = QString::fromUtf8(track.id.data(), track.id.size());
        map["title"] = QString::fromUtf8(track.title.data(), track.title.size());
        map["artist"] = QString::fromUtf8(track.artist.data(), track.artist.size());
        map["album"] = QString::fromUtf8(track.album.data(), track.album.size());
        map["thumbnail"] = QString::fromUtf8(track.thumbnail.data(), track.thumbnail.size());
        
        int ts = track.duration_ms / 1000;
        map["duration"] = QString("%1:%2").arg(ts / 60).arg(ts % 60, 2, 10, QChar('0'));
        songs_list_.append(map);
    }
    
    raw_videos_ = videos;
    videos_list_.clear();
    for (const auto &track : videos) {
        QVariantMap map;
        map["id"] = QString::fromUtf8(track.id.data(), track.id.size());
        map["title"] = QString::fromUtf8(track.title.data(), track.title.size());
        map["artist"] = QString::fromUtf8(track.artist.data(), track.artist.size());
        map["album"] = QString::fromUtf8(track.album.data(), track.album.size());
        map["thumbnail"] = QString::fromUtf8(track.thumbnail.data(), track.thumbnail.size());
        
        int ts = track.duration_ms / 1000;
        map["duration"] = QString("%1:%2").arg(ts / 60).arg(ts % 60, 2, 10, QChar('0'));
        videos_list_.append(map);
    }
    
    artists_list_.clear();
    for (const auto &a : artists) {
        QVariantMap map;
        map["id"] = QString::fromUtf8(a.id.data(), a.id.size());
        map["name"] = QString::fromUtf8(a.name.data(), a.name.size());
        map["thumbnail"] = QString::fromUtf8(a.thumbnail.data(), a.thumbnail.size());
        artists_list_.append(map);
    }
    
    albums_list_.clear();
    for (const auto &a : albums) {
        QVariantMap map;
        map["id"] = QString::fromUtf8(a.id.data(), a.id.size());
        map["title"] = QString::fromUtf8(a.title.data(), a.title.size());
        map["artist"] = QString::fromUtf8(a.artist.data(), a.artist.size());
        map["thumbnail"] = QString::fromUtf8(a.thumbnail.data(), a.thumbnail.size());
        albums_list_.append(map);
    }
    
    playlists_list_.clear();
    for (const auto &p : playlists) {
        QVariantMap map;
        map["id"] = QString::fromUtf8(p.id.data(), p.id.size());
        map["title"] = QString::fromUtf8(p.name.data(), p.name.size());
        map["author"] = QString::fromUtf8(p.owner.data(), p.owner.size());
        map["thumbnail"] = QString::fromUtf8(p.thumbnail.data(), p.thumbnail.size());
        playlists_list_.append(map);
    }
    
    shows_list_.clear();
    for (const auto &s : shows) {
        QVariantMap map;
        map["id"] = QString::fromUtf8(s.id.data(), s.id.size());
        map["title"] = QString::fromUtf8(s.title.data(), s.title.size());
        map["author"] = QString::fromUtf8(s.author.data(), s.author.size());
        map["thumbnail"] = QString::fromUtf8(s.thumbnail.data(), s.thumbnail.size());
        shows_list_.append(map);
    }
    
    episodes_list_.clear();
    for (const auto &e : episodes) {
        QVariantMap map;
        map["id"] = QString::fromUtf8(e.id.data(), e.id.size());
        map["title"] = QString::fromUtf8(e.title.data(), e.title.size());
        map["description"] = QString::fromUtf8(e.description.data(), e.description.size());
        int ts = e.duration_ms / 1000;
        map["duration"] = QString("%1:%2").arg(ts / 60).arg(ts % 60, 2, 10, QChar('0'));
        episodes_list_.append(map);
    }
    
    showing_recent_ = false;
    emit recentChanged();
    emit resultsChanged();
}

void SearchView::set_recent_searches(const std::vector<std::string> &queries) {
    recent_searches_list_.clear();
    for (const auto &q : queries) {
        recent_searches_list_.append(QString::fromStdString(q));
    }
    showing_recent_ = true;
    emit recentChanged();
}

void SearchView::requestFilterChange(const QString &filter) {
    if (active_filter_ != filter) {
        active_filter_ = filter;
        emit filterChanged();
        emit search_requested(current_query_.toStdString(), filter.toStdString());
    }
}

void SearchView::requestSearch(const QString &query) {
    emit search_requested(query.toStdString(), active_filter_.toStdString());
}

void SearchView::requestClearSearchHistory() {
    emit search_history_clear_requested();
}

void SearchView::requestDeleteSearchHistory(const QString &query) {
    emit search_history_delete_requested(query.toStdString());
}

Track SearchView::getTrackById(const QString &id) {
    std::string stdId = id.toStdString();
    for (const auto &t : raw_songs_) {
        if (std::string(t.id) == stdId) return t;
    }
    for (const auto &t : raw_videos_) {
        if (std::string(t.id) == stdId) return t;
    }
    if (has_top_result_) {
        // Find track in top result? Not applicable since top result doesn't hold tracks.
    }
    return Track();
}

void SearchView::requestPlayTrack(const QString &trackId) {
    Track t = getTrackById(trackId);
    if (!std::string(t.id).empty()) {
        emit play_requested(t);
    }
}

void SearchView::requestAlbum(const QString &browseId) {
    emit album_requested(browseId.toStdString());
}

void SearchView::requestArtist(const QString &browseId) {
    emit artist_requested(browseId.toStdString());
}

void SearchView::requestPlaylist(const QString &playlistId) {
    emit playlist_requested(playlistId.toStdString());
}

void SearchView::requestShow(const QString &browseId) {
    emit show_requested(browseId.toStdString());
}

void SearchView::requestTopResult() {
    if (!has_top_result_) return;
    std::string type = std::string(raw_top_result_.item_type);
    std::string id = std::string(raw_top_result_.id);
    
    if (type == "artist") {
        emit artist_requested(id);
    } else if (type == "album") {
        emit album_requested(id);
    } else if (type == "playlist") {
        emit playlist_requested(id);
    } else if (type == "song" || type == "video") {
        // Request play
        Track dummy;
        dummy.id = raw_top_result_.id;
        emit play_requested(dummy);
    }
}
