#include "waveform_bars.h"
#include "design_tokens.h"
#include <QPainter>
#include <QLinearGradient>
#include <cstdlib>

WaveformBars::WaveformBars(QWidget *parent, int barCount)
    : QWidget(parent), m_barCount(barCount)
{
    m_heights.resize(m_barCount, 3.0);
    m_targetHeights.resize(m_barCount, 3.0);

    m_timer = new QTimer(this);
    m_timer->setInterval(50); // 20 fps
    connect(m_timer, &QTimer::timeout, this, &WaveformBars::onTimerTick);

    setFixedSize(sizeHint());
}

void WaveformBars::setPlaying(bool playing) {
    if (m_isPlaying == playing) return;
    m_isPlaying = playing;

    if (DesignTokens::reducedMotion()) {
        m_timer->stop();
        for (int i = 0; i < m_barCount; ++i) {
            m_targetHeights[i] = 3.0;
            m_heights[i] = 3.0;
        }
        update();
        return;
    }

    if (m_isPlaying) {
        m_timer->start();
    } else {
        // Rest heights to 3px
        for (int i = 0; i < m_barCount; ++i) {
            m_targetHeights[i] = 3.0;
        }
        update();
    }
}

void WaveformBars::onTimerTick() {
    if (DesignTokens::reducedMotion()) {
        m_timer->stop();
        return;
    }

    bool hasDelta = false;
    for (int i = 0; i < m_barCount; ++i) {
        if (m_isPlaying) {
            // Generate random target heights every 2 ticks (approx 100ms)
            if (std::rand() % 2 == 0) {
                m_targetHeights[i] = 4.0 + (std::rand() % (height() - 6));
            }
        }

        // Smoothly interpolate current heights to target heights
        qreal diff = m_targetHeights[i] - m_heights[i];
        if (std::abs(diff) > 0.1) {
            m_heights[i] += diff * 0.3; // lerp factor 0.3
            hasDelta = true;
        } else {
            m_heights[i] = m_targetHeights[i];
        }
    }

    if (hasDelta) {
        update();
    } else if (!m_isPlaying) {
        m_timer->stop();
    }
}

void WaveformBars::paintEvent(QPaintEvent *) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    const auto &c = DesignTokens::current();

    int barWidth = 3;
    int spacing = 3;
    
    QLinearGradient grad(0, height(), 0, 0);
    grad.setColorAt(0.0, c.accent);
    grad.setColorAt(1.0, c.accent_bright);

    painter.setPen(Qt::NoPen);
    painter.setBrush(grad);

    for (int i = 0; i < m_barCount; ++i) {
        qreal barHeight = m_heights[i];
        qreal x = i * (barWidth + spacing);
        qreal y = height() - barHeight;
        
        painter.drawRoundedRect(QRectF(x, y, barWidth, barHeight), 1.5, 1.5);
    }
}
