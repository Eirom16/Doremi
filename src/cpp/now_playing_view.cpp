#include "now_playing_view.h"
#include <QVBoxLayout>
#include <QPropertyAnimation>
#include <QQmlContext>
#include <QQuickItem>
#include "doremi/src/bridge.rs.h"

NowPlayingView::NowPlayingView(QWidget *parent)
    : QWidget(parent)
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    quick_widget_ = new QQuickWidget(this);
    quick_widget_->setResizeMode(QQuickWidget::SizeRootObjectToView);
    quick_widget_->rootContext()->setContextProperty("NowPlayingCtrl", this);
    
    // We will load the QML file from resources
    quick_widget_->setSource(QUrl("qrc:/qml/NowPlayingView.qml"));
    
    // Ensure transparency works
    quick_widget_->setAttribute(Qt::WA_TranslucentBackground);
    quick_widget_->setClearColor(Qt::transparent);

    layout->addWidget(quick_widget_);
    setLayout(layout);
}

void NowPlayingView::showView() {
    if (!parentWidget()) return;
    
    int parent_w = parentWidget()->width();
    int parent_h = parentWidget()->height();
    
    setGeometry(0, parent_h, parent_w, parent_h);
    show();
    raise();

    auto *anim = new QPropertyAnimation(this, "pos", this);
    anim->setDuration(350);
    anim->setEasingCurve(QEasingCurve::OutExpo);
    anim->setStartValue(QPoint(0, parent_h));
    anim->setEndValue(QPoint(0, 0));
    anim->start(QAbstractAnimation::DeleteWhenStopped);
}

void NowPlayingView::hideView() {
    if (!parentWidget()) return;
    
    int parent_h = parentWidget()->height();

    auto *anim = new QPropertyAnimation(this, "pos", this);
    anim->setDuration(250);
    anim->setEasingCurve(QEasingCurve::InCubic);
    anim->setStartValue(pos());
    anim->setEndValue(QPoint(0, parent_h));
    
    connect(anim, &QPropertyAnimation::finished, this, [this]() {
        hide();
        emit close_clicked();
    });
    
    anim->start(QAbstractAnimation::DeleteWhenStopped);
}

void NowPlayingView::setTrackInfo(const std::string &title, const std::string &artist, const std::string &thumbnail) {
    if (title_ != QString::fromStdString(title)) {
        title_ = QString::fromStdString(title);
        emit titleChanged();
    }
    if (artist_ != QString::fromStdString(artist)) {
        artist_ = QString::fromStdString(artist);
        emit artistChanged();
    }
    if (artworkUrl_ != QString::fromStdString(thumbnail)) {
        artworkUrl_ = QString::fromStdString(thumbnail);
        emit artworkUrlChanged();
    }
}

void NowPlayingView::setCurrentTrack(const Track &track) {
    current_track_ = track;
}

void NowPlayingView::setPlaybackState(int32_t state, int32_t position_ms, int32_t duration_ms) {
    if (position_ms_ != position_ms) {
        position_ms_ = position_ms;
        emit positionMsChanged();
    }
    if (duration_ms_ != duration_ms) {
        duration_ms_ = duration_ms;
        emit durationMsChanged();
    }
}

void NowPlayingView::setPlaying(bool playing) {
    if (is_playing_ != playing) {
        is_playing_ = playing;
        emit isPlayingChanged();
    }
}

void NowPlayingView::setShuffle(bool on) {
    if (shuffle_on_ != on) {
        shuffle_on_ = on;
        emit shuffleOnChanged();
    }
}

void NowPlayingView::setRepeatMode(int mode) {
    if (repeat_mode_ != mode) {
        repeat_mode_ = mode;
        emit repeatModeChanged();
    }
}

void NowPlayingView::setDominantColors(const QStringList &colors) {
    if (dominant_colors_ != colors) {
        dominant_colors_ = colors;
        emit dominantColorsChanged();
    }
}

void NowPlayingView::setLyrics(const QString &plain, const QString &synced) {
    if (plain_lyrics_ != plain) {
        plain_lyrics_ = plain;
        emit plainLyricsChanged();
    }
    if (synced_lyrics_ != synced) {
        synced_lyrics_ = synced;
        emit syncedLyricsChanged();
    }
}

void NowPlayingView::setQueue(const std::vector<Track> &tracks, int current_index) {
    QVariantList newQueue;
    for (const auto &track : tracks) {
        QVariantMap map;
        map["id"] = QString::fromStdString(static_cast<std::string>(track.id));
        map["title"] = QString::fromStdString(static_cast<std::string>(track.title));
        map["artist"] = QString::fromStdString(static_cast<std::string>(track.artist));
        map["thumbnail"] = QString::fromStdString(static_cast<std::string>(track.thumbnail));
        map["duration_ms"] = static_cast<int>(track.duration_ms);
        newQueue.append(map);
    }
    queue_ = newQueue;
    emit queueChanged();
    
    if (current_index_ != current_index) {
        current_index_ = current_index;
        emit currentIndexChanged();
    }
}

void NowPlayingView::setRelatedTracks(const std::vector<Track> &tracks) {
    raw_related_tracks_ = tracks;
    QVariantList newList;
    for (const auto &track : tracks) {
        QVariantMap map;
        map["id"] = QString::fromStdString(static_cast<std::string>(track.id));
        map["title"] = QString::fromStdString(static_cast<std::string>(track.title));
        map["artist"] = QString::fromStdString(static_cast<std::string>(track.artist));
        map["thumbnail"] = QString::fromStdString(static_cast<std::string>(track.thumbnail));
        newList.append(map);
    }
    related_tracks_ = newList;
    emit relatedTracksChanged();
}

void NowPlayingView::resizeEvent(QResizeEvent *event) {
    QWidget::resizeEvent(event);
}

// Actions from QML -> C++ -> Rust

void NowPlayingView::playQueueItem(int index) {
    on_queue_item_clicked(index);
}

void NowPlayingView::removeQueueItem(int index) {
    on_queue_item_removed(index);
}

void NowPlayingView::moveQueueItem(int from, int to) {
    on_queue_item_moved(from, to);
}

void NowPlayingView::clearQueue() {
    on_queue_clear_requested();
}

void NowPlayingView::playRelated(int index) {
    if (index >= 0 && index < raw_related_tracks_.size()) {
        emit related_play_requested(raw_related_tracks_[index]);
    }
}

void NowPlayingView::queueRelated(int index) {
    if (index >= 0 && index < raw_related_tracks_.size()) {
        emit related_add_to_queue_requested(raw_related_tracks_[index]);
    }
}
