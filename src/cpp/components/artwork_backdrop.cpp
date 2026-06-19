#include "artwork_backdrop.h"
#include <QPainter>
#include <QResizeEvent>

ArtworkBackdrop::ArtworkBackdrop(QWidget *parent)
    : QWidget(parent)
{
    debounce_timer_ = new QTimer(this);
    debounce_timer_->setSingleShot(true);
    debounce_timer_->setInterval(150);
    connect(debounce_timer_, &QTimer::timeout, this, &ArtworkBackdrop::updateBlurred);
}

void ArtworkBackdrop::setImage(const QString &path) {
    QPixmap pm(path);
    if (!pm.isNull()) {
        setImage(pm);
    }
}

void ArtworkBackdrop::setImage(const QPixmap &pixmap) {
    source_ = pixmap;
    debounce_timer_->start();
}

void ArtworkBackdrop::clear() {
    source_ = QPixmap();
    blurred_ = QPixmap();
    update();
}

void ArtworkBackdrop::updateBlurred() {
    if (source_.isNull() || width() <= 0 || height() <= 0) {
        blurred_ = QPixmap();
        update();
        return;
    }

    // Scale to widget size
    QPixmap scaled = source_.scaled(width(), height(), Qt::IgnoreAspectRatio, Qt::SmoothTransformation);

    // Cheap blur: scale down aggressively then back up
    int blur_factor = 8;
    QPixmap tiny = scaled.scaled(width() / blur_factor, height() / blur_factor,
                                 Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    blurred_ = tiny.scaled(width(), height(), Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    update();
}

void ArtworkBackdrop::resizeEvent(QResizeEvent *event) {
    QWidget::resizeEvent(event);
    if (!source_.isNull()) {
        updateBlurred();
    }
}

void ArtworkBackdrop::paintEvent(QPaintEvent *) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::SmoothPixmapTransform);

    if (!blurred_.isNull()) {
        painter.drawPixmap(0, 0, blurred_);
    } else {
        painter.fillRect(rect(), QColor(0, 0, 0, 0));
    }

    // Semi-transparent dark overlay for readability
    painter.fillRect(rect(), QColor(0, 0, 0, 140));
}
