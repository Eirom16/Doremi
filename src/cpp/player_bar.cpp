#include "player_bar.h"
#include <QVBoxLayout>
#include <QQmlContext>

PlayerBar::PlayerBar(QWidget *parent) : QWidget(parent) {
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    quick_widget_ = new QQuickWidget(this);
    quick_widget_->setResizeMode(QQuickWidget::SizeRootObjectToView);
    quick_widget_->setClearColor(Qt::transparent);

    quick_widget_->rootContext()->setContextProperty("PlayerCtrl", this);
    quick_widget_->setSource(QUrl("qrc:/qml/PlayerBar.qml"));

    layout->addWidget(quick_widget_);
}

void PlayerBar::resizeEvent(QResizeEvent *event) {
    QWidget::resizeEvent(event);
    if (quick_widget_) {
        quick_widget_->resize(size());
    }
}

void PlayerBar::set_track_info(const std::string &title, const std::string &artist, const std::string &thumbnail) {
    QString qTitle = QString::fromStdString(title);
    QString qArtist = QString::fromStdString(artist);
    QString qThumbnail = QString::fromStdString(thumbnail);
    
    if (title_ != qTitle) {
        title_ = qTitle;
        emit titleChanged();
    }
    if (artist_ != qArtist) {
        artist_ = qArtist;
        emit artistChanged();
    }
    if (thumbnail_ != qThumbnail) {
        thumbnail_ = qThumbnail;
        emit thumbnailChanged();
    }
}

void PlayerBar::set_progress(int32_t position_ms, int32_t duration_ms) {
    if (position_ms_ != position_ms) {
        position_ms_ = position_ms;
        emit positionMsChanged();
    }
    if (duration_ms_ != duration_ms) {
        duration_ms_ = duration_ms;
        emit durationMsChanged();
    }
}

void PlayerBar::set_playing(bool playing) {
    if (is_playing_ != playing) {
        is_playing_ = playing;
        emit isPlayingChanged();
    }
}

void PlayerBar::set_volume_value(int32_t volume) {
    if (volume_value_ != volume) {
        volume_value_ = volume;
        emit volumeValueChanged();
    }
}

void PlayerBar::set_shuffle(bool on) {
    if (shuffle_on_ != on) {
        shuffle_on_ = on;
        emit shuffleOnChanged();
    }
}

void PlayerBar::set_repeat_mode(int mode) {
    if (repeat_mode_ != mode) {
        repeat_mode_ = mode;
        emit repeatModeChanged();
    }
}

void PlayerBar::set_compact(bool compact) {
    if (compact_ != compact) {
        compact_ = compact;
        emit isCompactChanged();
    }
}
