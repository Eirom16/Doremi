#include "icon_button.h"
#include "design_tokens.h"
#include "icon_provider.h"
#include <QPainter>

IconButton::IconButton(const QString &iconName, QWidget *parent, int iconSize)
    : QPushButton(parent), m_iconName(iconName), m_iconSize(iconSize)
{
    setFixedSize(iconSize + 16, iconSize + 16);
    setCursor(Qt::PointingHandCursor);
    setStyleSheet("QPushButton { background: transparent; border: none; }");
    
    m_hoverAnim = new QVariantAnimation(this);
    m_hoverAnim->setDuration(120);
    m_hoverAnim->setEasingCurve(QEasingCurve::OutCubic);
    connect(m_hoverAnim, &QVariantAnimation::valueChanged, this, [this](const QVariant &val) {
        setHoverProgress(val.toReal());
    });
}

void IconButton::enterEvent(QEnterEvent *) {
    m_hoverAnim->stop();
    m_hoverAnim->setStartValue(m_hoverProgress);
    m_hoverAnim->setEndValue(1.0);
    m_hoverAnim->start();
}

void IconButton::leaveEvent(QEvent *) {
    m_hoverAnim->stop();
    m_hoverAnim->setStartValue(m_hoverProgress);
    m_hoverAnim->setEndValue(0.0);
    m_hoverAnim->start();
}

void IconButton::paintEvent(QPaintEvent *) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setRenderHint(QPainter::SmoothPixmapTransform);
    
    const auto &c = DesignTokens::current();
    
    // Draw background
    if (m_hoverProgress > 0.0) {
        QColor bg = c.accent_dim;
        bg.setAlphaF(bg.alphaF() * m_hoverProgress);
        painter.setBrush(bg);
        painter.setPen(Qt::NoPen);
        painter.drawRoundedRect(rect(), 6, 6);
    }
    
    // Calculate icon size with scale
    qreal scale = 1.0 + 0.08 * m_hoverProgress;
    qreal currentSize = m_iconSize * scale;
    
    // Smooth color interpolation
    QColor iconColor = QColor::fromRgbF(
        c.text_primary.redF() + (c.accent_bright.redF() - c.text_primary.redF()) * m_hoverProgress,
        c.text_primary.greenF() + (c.accent_bright.greenF() - c.text_primary.greenF()) * m_hoverProgress,
        c.text_primary.blueF() + (c.accent_bright.blueF() - c.text_primary.blueF()) * m_hoverProgress
    );
    
    QIcon icon = IconProvider::getIcon(m_iconName, iconColor, m_iconSize);
    QPixmap pm = icon.pixmap(currentSize, currentSize);
    
    // Draw pixmap centered, accounting for device pixel ratio
    qreal dpr = pm.devicePixelRatio();
    qreal drawW = pm.width() / dpr;
    qreal drawH = pm.height() / dpr;
    
    qreal px = (width() - drawW) / 2.0;
    qreal py = (height() - drawH) / 2.0;
    
    painter.drawPixmap(QRectF(px, py, drawW, drawH), pm, QRectF(0, 0, pm.width(), pm.height()));
}
