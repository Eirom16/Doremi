#include "glass_panel.h"
#include "design_tokens.h"
#include "animator.h"
#include <QPainter>
#include <QPainterPath>

GlassPanel::GlassPanel(QWidget *parent)
    : QWidget(parent)
{
    setAttribute(Qt::WA_TranslucentBackground);
}

void GlassPanel::showAnimated() {
    Animator::fadeIn(this, 200);
}

void GlassPanel::hideAnimated() {
    Animator::fadeOut(this, 180);
}

void GlassPanel::paintEvent(QPaintEvent *) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    const auto &c = DesignTokens::current();

    // Draw soft drop shadow
    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(0, 0, 0, 80));
    painter.drawRoundedRect(rect().adjusted(2, 2, -2, -2), 12, 12);

    // Draw glass body (bg_overlay)
    painter.setBrush(c.bg_overlay);
    painter.drawRoundedRect(rect().adjusted(0, 0, -2, -2), 10, 10);

    // Setup border paths
    QRectF borderRect = rect().adjusted(0, 0, -2, -2);
    QPainterPath borderPath;
    borderPath.addRoundedRect(borderRect, 10, 10);

    // Draw base subtle border
    painter.setBrush(Qt::NoBrush);
    painter.setPen(QPen(c.border, 1));
    painter.drawPath(borderPath);

    // Overlay accent top border gradient
    QLinearGradient topBorderGrad(borderRect.topLeft(), borderRect.bottomLeft());
    topBorderGrad.setColorAt(0.0, QColor(c.accent.red(), c.accent.green(), c.accent.blue(), 102)); // ~40% opacity
    topBorderGrad.setColorAt(0.3, QColor(c.accent.red(), c.accent.green(), c.accent.blue(), 26));  // ~10% opacity
    topBorderGrad.setColorAt(1.0, QColor(c.accent.red(), c.accent.green(), c.accent.blue(), 0));   // 0% opacity

    painter.setPen(QPen(QBrush(topBorderGrad), 1.5));
    painter.drawPath(borderPath);
}
