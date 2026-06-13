#include "skeleton_loader.h"
#include "design_tokens.h"
#include <QPainter>
#include <QLinearGradient>

SkeletonLoader::SkeletonLoader(QWidget *parent)
    : QWidget(parent)
{
    m_shimmerAnim = new QVariantAnimation(this);
    m_shimmerAnim->setDuration(1400);
    m_shimmerAnim->setStartValue(-0.3);
    m_shimmerAnim->setEndValue(1.3);
    m_shimmerAnim->setLoopCount(-1); // Infinite loop
    m_shimmerAnim->setEasingCurve(QEasingCurve::InOutSine);
    
    connect(m_shimmerAnim, &QVariantAnimation::valueChanged, this, [this](const QVariant &val) {
        setShimmerOffset(val.toReal());
    });
    
    m_shimmerAnim->start();
}

void SkeletonLoader::paintEvent(QPaintEvent *) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    const auto &c = DesignTokens::current();
    
    // Draw background placeholder rounded rect
    QRectF r = rect();
    
    // Linear gradient for shimmer sweep
    qreal startX = r.width() * m_shimmerOffset;
    qreal endX = r.width() * (m_shimmerOffset + 0.4);
    
    QLinearGradient shimmerGrad(startX, 0, endX, 0);
    shimmerGrad.setColorAt(0.0, c.bg_elevated);
    
    // Shimmer peak: a blend of bg_elevated and accent_dim/light overlay
    QColor peakColor = c.bg_elevated;
    // Lighten the peakColor slightly
    peakColor.setRed(qMin(255, peakColor.red() + 15));
    peakColor.setGreen(qMin(255, peakColor.green() + 15));
    peakColor.setBlue(qMin(255, peakColor.blue() + 25)); // slightly more blue/violet tint
    
    shimmerGrad.setColorAt(0.5, peakColor);
    shimmerGrad.setColorAt(1.0, c.bg_elevated);
    
    painter.setPen(Qt::NoPen);
    painter.setBrush(shimmerGrad);
    painter.drawRoundedRect(r, 6, 6);
}
