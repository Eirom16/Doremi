#pragma once

#include <QPushButton>
#include <QPoint>
#include <QVariantAnimation>

class RippleButton : public QPushButton {
    Q_OBJECT
    Q_PROPERTY(qreal rippleRadius READ rippleRadius WRITE setRippleRadius)
    Q_PROPERTY(qreal rippleOpacity READ rippleOpacity WRITE setRippleOpacity)
public:
    enum class Variant {
        Primary,
        Secondary,
        Ghost,
        Danger
    };

    explicit RippleButton(const QString &text, QWidget *parent = nullptr, Variant variant = Variant::Secondary);
    explicit RippleButton(QWidget *parent = nullptr, Variant variant = Variant::Secondary);
    
    void setVariant(Variant var);
    void updateStyle();
    
    qreal rippleRadius() const { return m_rippleRadius; }
    void setRippleRadius(qreal r) { m_rippleRadius = r; update(); }
    
    qreal rippleOpacity() const { return m_rippleOpacity; }
    void setRippleOpacity(qreal o) { m_rippleOpacity = o; update(); }

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void paintEvent(QPaintEvent *event) override;

private:
    Variant m_variant;
    QPoint m_clickPos;
    qreal m_rippleRadius = 0.0;
    qreal m_rippleOpacity = 0.0;
    QVariantAnimation *m_radiusAnim = nullptr;
    QVariantAnimation *m_opacityAnim = nullptr;
    
    void init();
};
