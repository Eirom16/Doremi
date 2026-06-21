#include "scrolling_label.h"
#include "design_tokens.h"
#include <QPainter>
#include <QLinearGradient>
#include <QPixmap>

ScrollingLabel::ScrollingLabel(QWidget *parent)
    : QLabel(parent)
{
    init();
}

ScrollingLabel::ScrollingLabel(const QString &text, QWidget *parent)
    : QLabel(text, parent)
{
    init();
}

void ScrollingLabel::init() {
    m_scrollTimer = new QTimer(this);
    m_scrollTimer->setInterval(40); // 25 fps
    connect(m_scrollTimer, &QTimer::timeout, this, &ScrollingLabel::onTimerTick);
    
    updateTextWidth();
}

void ScrollingLabel::setText(const QString &text) {
    QLabel::setText(text);
    updateTextWidth();
}

void ScrollingLabel::updateTextWidth() {
    m_textWidth = fontMetrics().horizontalAdvance(text());
    m_scrollOffset = 0;
    m_direction = 1;
    m_pauseTicks = 50; // Pause at the beginning (2 seconds)
    
    if (DesignTokens::reducedMotion()) {
        m_scrollTimer->stop();
    } else if (m_textWidth > width() && width() > 0) {
        if (!m_scrollTimer->isActive()) {
            m_scrollTimer->start();
        }
    } else {
        m_scrollTimer->stop();
    }
    update();
}

void ScrollingLabel::resizeEvent(QResizeEvent *event) {
    QLabel::resizeEvent(event);
    updateTextWidth();
}

void ScrollingLabel::onTimerTick() {
    if (DesignTokens::reducedMotion()) {
        m_scrollOffset = 0;
        m_scrollTimer->stop();
        update();
        return;
    }

    if (m_textWidth <= width()) {
        m_scrollTimer->stop();
        return;
    }
    
    if (m_pauseTicks > 0) {
        m_pauseTicks--;
        return;
    }
    
    int maxScroll = m_textWidth - width() + 24; // 24px extra margin at the end
    m_scrollOffset += m_direction;
    
    if (m_scrollOffset >= maxScroll) {
        m_scrollOffset = maxScroll;
        m_direction = -1;
        m_pauseTicks = 50; // Pause at the end (2 seconds)
    } else if (m_scrollOffset <= 0) {
        m_scrollOffset = 0;
        m_direction = 1;
        m_pauseTicks = 50; // Pause at the start
    }
    
    update();
}

void ScrollingLabel::paintEvent(QPaintEvent *) {
    if (text().isEmpty()) return;
    
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    
    if (m_textWidth <= width()) {
        // Draw normally
        painter.setFont(font());
        painter.setPen(palette().color(QPalette::WindowText));
        painter.drawText(rect(), Qt::AlignLeft | Qt::AlignVCenter, text());
        return;
    }
    
    // Draw scrollable text with gradient fade edges
    QPixmap pixmap(size());
    pixmap.fill(Qt::transparent);
    
    QPainter pixPainter(&pixmap);
    pixPainter.setRenderHint(QPainter::Antialiasing);
    pixPainter.setFont(font());
    pixPainter.setPen(palette().color(QPalette::WindowText));
    
    // Draw the text shifted
    QRect textRect(-m_scrollOffset, 0, m_textWidth + 100, height());
    pixPainter.drawText(textRect, Qt::AlignLeft | Qt::AlignVCenter, text());
    
    // Apply fade edge mask on the left (20px) and right (20px)
    pixPainter.setCompositionMode(QPainter::CompositionMode_DestinationIn);
    
    // Left fade
    QLinearGradient leftGrad(0, 0, 20, 0);
    leftGrad.setColorAt(0.0, QColor(0, 0, 0, 0));
    leftGrad.setColorAt(1.0, QColor(0, 0, 0, 255));
    pixPainter.fillRect(0, 0, 20, height(), leftGrad);
    
    // Right fade
    QLinearGradient rightGrad(width(), 0, width() - 20, 0);
    rightGrad.setColorAt(0.0, QColor(0, 0, 0, 0));
    rightGrad.setColorAt(1.0, QColor(0, 0, 0, 255));
    pixPainter.fillRect(width() - 20, 0, 20, height(), rightGrad);
    
    pixPainter.end();
    
    painter.drawPixmap(0, 0, pixmap);
}
