#include "animated_progress.h"
#include "design_tokens.h"
#include <QMouseEvent>
#include <QPainter>
#include <QRadialGradient>
#include <QLinearGradient>

AnimatedProgress::AnimatedProgress(Qt::Orientation orientation, QWidget *parent)
    : QSlider(orientation, parent)
{
    setMinimumHeight(20);
    setCursor(Qt::PointingHandCursor);
    
    m_hoverAnim = new QVariantAnimation(this);
    m_hoverAnim->setDuration(150);
    m_hoverAnim->setEasingCurve(QEasingCurve::OutCubic);
    connect(m_hoverAnim, &QVariantAnimation::valueChanged, this, [this](const QVariant &val) {
        setHoverProgress(val.toReal());
    });
}

void AnimatedProgress::enterEvent(QEnterEvent *) {
    m_isHovered = true;
    m_hoverAnim->stop();
    m_hoverAnim->setStartValue(m_hoverProgress);
    m_hoverAnim->setEndValue(1.0);
    m_hoverAnim->start();
}

void AnimatedProgress::leaveEvent(QEvent *) {
    m_isHovered = false;
    m_hoverAnim->stop();
    m_hoverAnim->setStartValue(m_hoverProgress);
    m_hoverAnim->setEndValue(0.0);
    m_hoverAnim->start();
}

void AnimatedProgress::mousePressEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton) {
        qreal pct = static_cast<qreal>(event->position().x()) / width();
        int val = minimum() + static_cast<int>((maximum() - minimum()) * pct);
        setValue(val);
        emit sliderMoved(val);
        event->accept();
        return;
    }
    QSlider::mousePressEvent(event);
}

void AnimatedProgress::paintEvent(QPaintEvent *) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    const auto &c = DesignTokens::current();
    
    // Calculate animated properties
    qreal trackHeight = 3.0 + 2.0 * m_hoverProgress;
    qreal thumbRadius = 5.0 * m_hoverProgress;
    
    qreal y = (height() - trackHeight) / 2.0;
    
    // 1. Draw track background
    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(255, 255, 255, 30)); // 12% opacity white
    painter.drawRoundedRect(QRectF(0, y, width(), trackHeight), trackHeight/2.0, trackHeight/2.0);
    
    // Calculate progress ratio
    qreal val = value();
    qreal max = maximum();
    qreal min = minimum();
    qreal range = max - min;
    qreal pct = range > 0 ? (val - min) / range : 0.0;
    qreal progressWidth = width() * pct;
    
    // 2. Draw progress fill with horizontal linear gradient
    if (progressWidth > 0) {
        QLinearGradient grad(0, 0, width(), 0);
        grad.setColorAt(0.0, c.accent);
        grad.setColorAt(1.0, c.accent_bright);
        
        painter.setBrush(grad);
        painter.drawRoundedRect(QRectF(0, y, progressWidth, trackHeight), trackHeight/2.0, trackHeight/2.0);
    }
    
    // 3. Draw thumb if active/hovered
    if (m_hoverProgress > 0.0 && progressWidth >= 0.0) {
        QPointF thumbCenter(progressWidth, height() / 2.0);
        
        // Draw outer glow/shadow
        QRadialGradient glowGrad(thumbCenter, thumbRadius + 4);
        glowGrad.setColorAt(0.0, QColor(c.accent_bright.red(), c.accent_bright.green(), c.accent_bright.blue(), 100));
        glowGrad.setColorAt(1.0, QColor(c.accent_bright.red(), c.accent_bright.green(), c.accent_bright.blue(), 0));
        painter.setBrush(glowGrad);
        painter.drawEllipse(thumbCenter, thumbRadius + 4, thumbRadius + 4);
        
        // Draw solid thumb
        painter.setBrush(c.accent_bright);
        painter.drawEllipse(thumbCenter, thumbRadius, thumbRadius);
    }
}
