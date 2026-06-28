#include "vinyl_disc.h"
#include "../icon_provider.h"
#include "../design_tokens.h"
#include <QPainter>
#include <QPainterPath>
#include <QRadialGradient>
#include <cmath>

VinylDisc::VinylDisc(QWidget *parent)
    : QWidget(parent)
{
    setFixedSize(260, 260);

    timer_ = new QTimer(this);
    connect(timer_, &QTimer::timeout, this, &VinylDisc::onTick);
    // 60fps (16ms) for fluid continuous rotation
    if (!DesignTokens::reducedMotion()) {
        timer_->start(16);
    }
}

void VinylDisc::setArtwork(const QString &thumbnail_path) {
    if (!thumbnail_path.isEmpty()) {
        QPixmap pm(thumbnail_path);
        if (!pm.isNull()) {
            artwork_ = pm;
            update();
            return;
        }
    }

    // Default placeholder
    const auto &c = DesignTokens::current();
    artwork_ = IconProvider::getIcon("music_note", c.text_secondary, 48).pixmap(128, 128);
    update();
}

void VinylDisc::setPlaying(bool playing) {
    playing_ = playing;
    if (DesignTokens::reducedMotion()) {
        target_speed_ = 0.0f;
        current_speed_ = 0.0f;
        if (timer_) timer_->stop();
        update();
        return;
    }
    if (timer_ && !timer_->isActive()) {
        timer_->start(16);
    }
    target_speed_ = playing ? MAX_SPEED : 0.0f;
}

void VinylDisc::onTick() {
    if (DesignTokens::reducedMotion()) {
        current_speed_ = 0.0f;
        target_speed_ = 0.0f;
        if (timer_) timer_->stop();
        return;
    }

    // Smooth speed interpolation (accel / decel)
    if (std::abs(current_speed_ - target_speed_) > 0.001f) {
        if (current_speed_ < target_speed_) {
            current_speed_ = std::min(target_speed_, current_speed_ + ACCEL);
        } else {
            current_speed_ = std::max(target_speed_, current_speed_ - DECEL);
        }
    }

    if (current_speed_ > 0.0f) {
        angle_ += current_speed_;
        if (angle_ >= 360.0f) angle_ -= 360.0f;
        update();
    }
}

void VinylDisc::paintEvent(QPaintEvent *) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    const auto &c = DesignTokens::current();

    int w = width();
    int h = height();
    int size = std::min(w, h);
    int radius = size / 2;
    QPoint center(w / 2, h / 2);

    // 1. Draw vinyl record outer body (Deep black/grey)
    painter.setBrush(QColor("#09090C"));
    painter.setPen(QPen(QColor("#18181F"), 2));
    painter.drawEllipse(center, radius - 4, radius - 4);

    // 2. Draw glossy vinyl reflection (subtle highlights)
    QRadialGradient reflection(center, radius);
    reflection.setColorAt(0.0, QColor(255, 255, 255, 10));
    reflection.setColorAt(0.7, QColor(255, 255, 255, 2));
    reflection.setColorAt(1.0, QColor(255, 255, 255, 0));
    painter.setBrush(reflection);
    painter.setPen(Qt::NoPen);
    painter.drawEllipse(center, radius - 6, radius - 6);

    // 3. Draw concentric grooves (sound waves)
    painter.setBrush(Qt::NoBrush);
    QColor groove_color(255, 255, 255, 8);
    for (int r = radius - 15; r > radius * 0.5f; r -= 6) {
        painter.setPen(QPen(groove_color, 1));
        painter.drawEllipse(center, r, r);
    }
    
    // Draw some thicker track divider rings
    painter.setPen(QPen(QColor(255, 255, 255, 14), 1.5));
    painter.drawEllipse(center, static_cast<int>(radius * 0.82f), static_cast<int>(radius * 0.82f));
    painter.drawEllipse(center, static_cast<int>(radius * 0.65f), static_cast<int>(radius * 0.65f));

    // 4. Draw rotated artwork in the center
    int art_diameter = static_cast<int>(size * 0.46f);
    int art_radius = art_diameter / 2;

    painter.save();
    
    // Set clipping path for the circular cover art
    QPainterPath clipPath;
    clipPath.addEllipse(center, art_radius, art_radius);
    painter.setClipPath(clipPath);

    // Translate and rotate painter to render artwork rotated
    painter.translate(center);
    painter.rotate(angle_);
    
    if (!artwork_.isNull()) {
        QRect target_rect(-art_radius, -art_radius, art_diameter, art_diameter);
        // Paint scaled artwork
        painter.drawPixmap(target_rect, artwork_.scaled(art_diameter, art_diameter, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation));
    }
    
    painter.restore();

    // 5. Draw center vinyl label border
    painter.setBrush(Qt::NoBrush);
    painter.setPen(QPen(QColor(0, 0, 0, 80), 3));
    painter.drawEllipse(center, art_radius, art_radius);

    // 6. Draw spindle center hole (with metallic ring)
    int spindle_d = 12;
    painter.setBrush(c.bg_base); // base background color
    painter.setPen(QPen(QColor("#8A8A8F"), 1.5));
    painter.drawEllipse(center, spindle_d / 2, spindle_d / 2);
}
