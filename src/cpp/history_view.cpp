#include "history_view.h"
#include <QQmlContext>
#include <QVariantMap>
#include <QVBoxLayout>
#include <QDateTime>

HistoryView::HistoryView(QWidget *parent)
    : QWidget(parent)
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    quick_widget_ = new QQuickWidget(this);
    quick_widget_->setResizeMode(QQuickWidget::SizeRootObjectToView);
    quick_widget_->setAttribute(Qt::WA_TranslucentBackground);
    quick_widget_->setClearColor(Qt::transparent);

    quick_widget_->rootContext()->setContextProperty("HistoryCtrl", this);
    quick_widget_->setSource(QUrl("qrc:/qml/HistoryView.qml"));

    layout->addWidget(quick_widget_);
}

void HistoryView::set_history(const std::vector<Track> &tracks,
                 const std::vector<std::string> &played_at,
                 const std::vector<std::string> &feedback_tokens) {
    raw_tracks_ = tracks;
    history_list_.clear();
    
    for (size_t i = 0; i < tracks.size(); ++i) {
        const auto &track = tracks[i];
        QVariantMap map;
        map["id"] = QString::fromUtf8(track.id.data(), track.id.size());
        map["title"] = QString::fromUtf8(track.title.data(), track.title.size());
        map["artist"] = QString::fromUtf8(track.artist.data(), track.artist.size());
        map["album"] = QString::fromUtf8(track.album.data(), track.album.size());
        map["thumbnail"] = QString::fromUtf8(track.thumbnail.data(), track.thumbnail.size());
        
        int ts = track.duration_ms / 1000;
        map["duration"] = QString("%1:%2").arg(ts / 60).arg(ts % 60, 2, 10, QChar('0'));
        
        if (i < played_at.size()) {
            map["playedAt"] = formatRelativeTime(QString::fromStdString(played_at[i]));
        } else {
            map["playedAt"] = "";
        }
        
        history_list_.append(map);
    }
    
    emit historyChanged();
}

void HistoryView::clear_history() {
    raw_tracks_.clear();
    history_list_.clear();
    emit historyChanged();
}

Track HistoryView::getTrackById(const QString &id) {
    std::string stdId = id.toStdString();
    for (const auto &t : raw_tracks_) {
        if (std::string(t.id) == stdId) return t;
    }
    return Track();
}

void HistoryView::requestPlay(const QString &trackId) {
    Track t = getTrackById(trackId);
    if (!std::string(t.id).empty()) {
        emit play_requested(t);
    }
}

// Simple time formatter
QString HistoryView::formatRelativeTime(const QString &played_at) const {
    // Usually played_at is something like "Hoy", "Ayer", or a month. 
    // We just pass it through or format it if it's a raw timestamp.
    // For now we pass it through.
    return played_at;
}
