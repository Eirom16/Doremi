#include "nebula_bg.h"
#include <QPainter>
#include <QRadialGradient>
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

static QColor interpolateColor(const QColor &start, const QColor &end, float t) {
    t = qBound(0.0f, t, 1.0f);
    // Smoothstep interpolation
    float eased_t = t * t * (3.0f - 2.0f * t);
    
    int r = start.red() + static_cast<int>((end.red() - start.red()) * eased_t);
    int g = start.green() + static_cast<int>((end.green() - start.green()) * eased_t);
    int b = start.blue() + static_cast<int>((end.blue() - start.blue()) * eased_t);
    int a = start.alpha() + static_cast<int>((end.alpha() - start.alpha()) * eased_t);
    return QColor(r, g, b, a);
}

NebulaBg::NebulaBg(QWidget *parent)
    : QWidget(parent)
{
    // Default beautiful colors (Purple, Blue, Pink, Deep Violet)
    c1_ = QColor("#5A3791");
    c2_ = QColor("#224A86");
    c3_ = QColor("#802856");
    c4_ = QColor("#1C1635");

    target_c1_ = c1_;
    target_c2_ = c2_;
    target_c3_ = c3_;
    target_c4_ = c4_;

    // Set up blobs
    // Blob 0: Top-left-ish
    blobs_[0] = {0.25f, 0.3f, 0.15f, 0.0f, 0.45f};
    // Blob 1: Bottom-right-ish
    blobs_[1] = {0.75f, 0.7f, 0.12f, 2.0f, 0.50f};
    // Blob 2: Top-right-ish
    blobs_[2] = {0.8f, 0.25f, 0.18f, 4.0f, 0.40f};
    // Blob 3: Center large pulse
    blobs_[3] = {0.5f, 0.5f, 0.08f, 1.0f, 0.65f};

    time_.start();
    
    timer_ = new QTimer(this);
    connect(timer_, &QTimer::timeout, this, &NebulaBg::onTick);
    // 20 fps (50ms interval) for smooth animations but low CPU usage
    timer_->start(50);
}

void NebulaBg::setColors(const QStringList &hex_colors) {
    if (hex_colors.size() < 3) return;

    // Source current colors
    c1_ = interpolateColor(c1_, target_c1_, color_progress_);
    c2_ = interpolateColor(c2_, target_c2_, color_progress_);
    c3_ = interpolateColor(c3_, target_c3_, color_progress_);
    c4_ = interpolateColor(c4_, target_c4_, color_progress_);

    // Set new target colors
    target_c1_ = QColor(hex_colors[0]);
    target_c2_ = QColor(hex_colors[1]);
    target_c3_ = QColor(hex_colors[2]);
    
    // c4 is a blend or darker variant of primary for ambient base
    target_c4_ = target_c1_.darker(250);

    color_progress_ = 0.0f;
    color_transition_time_.start();
    update();
}

void NebulaBg::setPlaying(bool playing) {
    if (playing_ == playing) return;
    playing_ = playing;
    update();
}

void NebulaBg::onTick() {
    if (color_progress_ < 1.0f) {
        int elapsed = color_transition_time_.elapsed();
        color_progress_ = static_cast<float>(elapsed) / TRANSITION_DURATION_MS;
        if (color_progress_ >= 1.0f) {
            color_progress_ = 1.0f;
            c1_ = target_c1_;
            c2_ = target_c2_;
            c3_ = target_c3_;
            c4_ = target_c4_;
        }
        update();
    } else if (playing_) {
        update();
    }
}

void NebulaBg::paintEvent(QPaintEvent *) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    int w = width();
    int h = height();

    // 1. Draw base solid color (dark deep violet/black)
    QColor base_color = interpolateColor(c4_, target_c4_, color_progress_);
    painter.fillRect(rect(), base_color);

    // 2. Interpolate active colors
    QColor active_c1 = interpolateColor(c1_, target_c1_, color_progress_);
    QColor active_c2 = interpolateColor(c2_, target_c2_, color_progress_);
    QColor active_c3 = interpolateColor(c3_, target_c3_, color_progress_);

    // Scale down alpha to keep background subtle and readable
    active_c1.setAlpha(60);
    active_c2.setAlpha(60);
    active_c3.setAlpha(50);

    float time_sec = time_.elapsed() / 1000.0f;

    // Use CompositionMode_Plus for premium organic lighting/glowing effect
    painter.setCompositionMode(QPainter::CompositionMode_Plus);

    // Render 4 blobs
    for (int i = 0; i < 4; ++i) {
        const auto &blob = blobs_[i];
        
        // Sinusoidal position offsets
        float offset_x = 0.12f * std::sin(time_sec * blob.speed + blob.phase);
        float offset_y = 0.12f * std::cos(time_sec * blob.speed * 0.8f + blob.phase);

        float cx = (blob.base_x + offset_x) * w;
        float cy = (blob.base_y + offset_y) * h;

        // Size pulsation
        float pulse = 1.0f;
        if (i == 3) {
            // Central blob pulses dynamically (simulating music beats)
            pulse = 1.0f + 0.08f * std::sin(time_sec * (playing_ ? 4.5f : 1.5f));
        } else {
            pulse = 1.0f + 0.05f * std::sin(time_sec * 0.8f + blob.phase);
        }

        float radius = blob.radius_factor * std::min(w, h) * pulse;
        if (radius <= 0) radius = 100;

        QColor blob_color;
        if (i == 0) blob_color = active_c1;
        else if (i == 1) blob_color = active_c2;
        else if (i == 2) blob_color = active_c3;
        else {
            // 4th blob is a blend, very soft
            blob_color = active_c1;
            blob_color.setAlpha(30);
        }

        QRadialGradient gradient(cx, cy, radius);
        gradient.setColorAt(0.0, blob_color);
        gradient.setColorAt(1.0, Qt::transparent);

        painter.setBrush(gradient);
        painter.setPen(Qt::NoPen);
        painter.drawEllipse(QPointF(cx, cy), radius, radius);
    }
}
