#include "artwork_backdrop.h"
#include <QDebug>
#include <QElapsedTimer>
#include <QPainter>
#include <QResizeEvent>

namespace {
QSize bounded_blur_size(const QSize &size) {
    constexpr int kMaxBlurDimension = 720;
    if (size.width() <= kMaxBlurDimension && size.height() <= kMaxBlurDimension) {
        return size;
    }
    return size.scaled(kMaxBlurDimension, kMaxBlurDimension, Qt::KeepAspectRatio);
}
}

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
    blurred_ = QPixmap();
    blurred_size_ = QSize();
    debounce_timer_->start();
}

void ArtworkBackdrop::clear() {
    source_ = QPixmap();
    blurred_ = QPixmap();
    blurred_size_ = QSize();
    last_blur_ms_ = 0;
    update();
}

void ArtworkBackdrop::updateBlurred() {
    if (source_.isNull() || width() <= 0 || height() <= 0) {
        blurred_ = QPixmap();
        blurred_size_ = QSize();
        update();
        return;
    }

    const QSize widget_size(width(), height());
    if (!blurred_.isNull() && blurred_size_ == widget_size) {
        update();
        return;
    }

    QElapsedTimer timer;
    timer.start();

    const QSize work_size = bounded_blur_size(widget_size);
    QPixmap scaled = source_.scaled(work_size, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);

    const int pixels = work_size.width() * work_size.height();
    const int blur_factor = pixels > 300000 ? 12 : 8;
    const QSize tiny_size(qMax(1, work_size.width() / blur_factor),
                          qMax(1, work_size.height() / blur_factor));
    QPixmap tiny = scaled.scaled(tiny_size,
                                 Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    blurred_ = tiny.scaled(widget_size, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    blurred_size_ = widget_size;
    last_blur_ms_ = timer.elapsed();
    if (last_blur_ms_ > 24) {
        qDebug() << "ArtworkBackdrop CPU blur" << last_blur_ms_ << "ms"
                 << "widget" << widget_size << "work" << work_size
                 << "factor" << blur_factor;
    }
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
