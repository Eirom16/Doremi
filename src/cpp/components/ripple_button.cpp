#include "ripple_button.h"
#include "design_tokens.h"
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <cmath>

RippleButton::RippleButton(const QString &text, QWidget *parent, Variant variant)
    : QPushButton(text, parent), m_variant(variant)
{
    init();
}

RippleButton::RippleButton(QWidget *parent, Variant variant)
    : QPushButton(parent), m_variant(variant)
{
    init();
}

void RippleButton::init() {
    setCursor(Qt::PointingHandCursor);
    updateStyle();
    
    m_radiusAnim = new QVariantAnimation(this);
    m_radiusAnim->setDuration(DesignTokens::duration(450));
    m_radiusAnim->setEasingCurve(QEasingCurve::OutCubic);
    connect(m_radiusAnim, &QVariantAnimation::valueChanged, this, [this](const QVariant &val) {
        setRippleRadius(val.toReal());
    });
    
    m_opacityAnim = new QVariantAnimation(this);
    m_opacityAnim->setDuration(DesignTokens::duration(450));
    m_opacityAnim->setEasingCurve(QEasingCurve::OutCubic);
    connect(m_opacityAnim, &QVariantAnimation::valueChanged, this, [this](const QVariant &val) {
        setRippleOpacity(val.toReal());
    });
}

void RippleButton::setVariant(Variant var) {
    m_variant = var;
    updateStyle();
}

void RippleButton::updateStyle() {
    const auto &c = DesignTokens::current();
    QString bg_color, border_color, text_color, hover_bg;
    
    switch (m_variant) {
        case Variant::Primary:
            bg_color = c.accent.name();
            border_color = "transparent";
            text_color = "#FFFFFF";
            hover_bg = c.accent_bright.name();
            break;
        case Variant::Secondary:
            bg_color = "transparent";
            border_color = QString("rgba(%1, %2, %3, %4)").arg(c.border.red()).arg(c.border.green()).arg(c.border.blue()).arg(c.border.alpha()/255.0);
            text_color = c.text_primary.name();
            hover_bg = QString("rgba(%1, %2, %3, %4)").arg(c.accent_dim.red()).arg(c.accent_dim.green()).arg(c.accent_dim.blue()).arg(c.accent_dim.alpha()/255.0);
            break;
        case Variant::Ghost:
            bg_color = "transparent";
            border_color = "transparent";
            text_color = c.text_secondary.name();
            hover_bg = QString("rgba(%1, %2, %3, %4)").arg(c.accent_dim.red()).arg(c.accent_dim.green()).arg(c.accent_dim.blue()).arg(c.accent_dim.alpha()/255.0);
            break;
        case Variant::Danger:
            bg_color = c.error.name();
            border_color = "transparent";
            text_color = "#FFFFFF";
            hover_bg = c.error.lighter(110).name();
            break;
    }
    
    QString style = QString(
        "QPushButton {\n"
        "    background-color: %1;\n"
        "    border: 1px solid %2;\n"
        "    border-radius: 8px;\n"
        "    color: %3;\n"
        "    padding: 8px 16px;\n"
        "    font-weight: 500;\n"
        "}\n"
        "QPushButton:hover {\n"
        "    background-color: %4;\n"
        "}\n"
    )
    .arg(bg_color)
    .arg(border_color)
    .arg(text_color)
    .arg(hover_bg);
    
    setStyleSheet(style);
}

void RippleButton::mousePressEvent(QMouseEvent *event) {
    m_clickPos = event->pos();
    
    qreal w = width();
    qreal h = height();
    qreal dx = qMax(static_cast<qreal>(m_clickPos.x()), w - m_clickPos.x());
    qreal dy = qMax(static_cast<qreal>(m_clickPos.y()), h - m_clickPos.y());
    qreal maxRadius = std::sqrt(dx*dx + dy*dy);
    
    m_radiusAnim->stop();
    m_radiusAnim->setStartValue(0.0);
    m_radiusAnim->setEndValue(maxRadius);
    
    m_opacityAnim->stop();
    m_opacityAnim->setStartValue(0.4);
    m_opacityAnim->setEndValue(0.0);
    
    m_radiusAnim->start();
    m_opacityAnim->start();
    
    QPushButton::mousePressEvent(event);
}

void RippleButton::paintEvent(QPaintEvent *event) {
    QPushButton::paintEvent(event);
    
    if (m_rippleOpacity > 0.0 && m_rippleRadius > 0.0) {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);
        
        QPainterPath path;
        path.addRoundedRect(rect(), 8, 8);
        painter.setClipPath(path);
        
        QColor rippleColor;
        if (m_variant == Variant::Primary || m_variant == Variant::Danger) {
            rippleColor = QColor(255, 255, 255);
        } else {
            rippleColor = DesignTokens::current().accent;
        }
        
        rippleColor.setAlphaF(m_rippleOpacity);
        
        QRadialGradient gradient(m_clickPos, m_rippleRadius);
        gradient.setColorAt(0.0, rippleColor);
        gradient.setColorAt(1.0, QColor(rippleColor.red(), rippleColor.green(), rippleColor.blue(), 0));
        
        painter.setBrush(gradient);
        painter.setPen(Qt::NoPen);
        painter.drawEllipse(QPointF(m_clickPos), m_rippleRadius, m_rippleRadius);
    }
}
