#include "animated_toggle.h"
#include "design_tokens.h"
#include <QPainter>
#include <QPainterPath>

AnimatedToggle::AnimatedToggle(QWidget *parent)
    : QAbstractButton(parent)
{
    setCheckable(true);
    setCursor(Qt::PointingHandCursor);
    setFixedSize(sizeHint());
    
    const auto &c = DesignTokens::current();
    m_trackColor = c.bg_elevated;
    
    m_posAnim = new QVariantAnimation(this);
    m_posAnim->setDuration(DesignTokens::duration(220));
    m_posAnim->setEasingCurve(QEasingCurve::InOutCubic);
    connect(m_posAnim, &QVariantAnimation::valueChanged, this, [this](const QVariant &val) {
        setThumbPosition(val.toReal());
    });
    
    m_colorAnim = new QVariantAnimation(this);
    m_colorAnim->setDuration(DesignTokens::duration(220));
    m_colorAnim->setEasingCurve(QEasingCurve::InOutCubic);
    connect(m_colorAnim, &QVariantAnimation::valueChanged, this, [this](const QVariant &val) {
        setTrackColor(val.value<QColor>());
    });
}

void AnimatedToggle::nextCheckState() {
    QAbstractButton::nextCheckState();
    
    const auto &c = DesignTokens::current();
    bool checked = isChecked();
    
    m_posAnim->stop();
    m_posAnim->setStartValue(m_thumbPosition);
    m_posAnim->setEndValue(checked ? 1.0 : 0.0);
    m_posAnim->start();
    
    m_colorAnim->stop();
    m_colorAnim->setStartValue(m_trackColor);
    m_colorAnim->setEndValue(checked ? c.accent : c.bg_elevated);
    m_colorAnim->start();
}

void AnimatedToggle::paintEvent(QPaintEvent *) {
    const auto &c = DesignTokens::current();
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    
    // Draw track
    int trackWidth = width();
    int trackHeight = height();
    
    painter.setPen(Qt::NoPen);
    painter.setBrush(m_trackColor);
    painter.drawRoundedRect(0, 0, trackWidth, trackHeight, trackHeight / 2.0, trackHeight / 2.0);
    
    // Draw thumb
    int thumbMargin = 3;
    int thumbDiam = trackHeight - (thumbMargin * 2);
    
    qreal startX = thumbMargin;
    qreal endX = trackWidth - thumbMargin - thumbDiam;
    qreal thumbX = startX + (endX - startX) * m_thumbPosition;
    qreal thumbY = thumbMargin;
    
    // Draw sutil thumb shadow
    painter.setBrush(QColor(0, 0, 0, 40));
    painter.drawEllipse(QRectF(thumbX, thumbY + 1.0, thumbDiam, thumbDiam));
    
    // Draw thumb
    painter.setBrush(c.text_on_accent);
    painter.drawEllipse(QRectF(thumbX, thumbY, thumbDiam, thumbDiam));
}

void AnimatedToggle::updateTheme() {
    const auto &c = DesignTokens::current();
    m_trackColor = isChecked() ? c.accent : c.bg_elevated;
    update();
}
