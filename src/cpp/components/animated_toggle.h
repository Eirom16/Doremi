#pragma once

#include <QAbstractButton>
#include <QVariantAnimation>
#include <QColor>

class AnimatedToggle : public QAbstractButton {
    Q_OBJECT
    Q_PROPERTY(qreal thumbPosition READ thumbPosition WRITE setThumbPosition)
    Q_PROPERTY(QColor trackColor READ trackColor WRITE setTrackColor)
public:
    explicit AnimatedToggle(QWidget *parent = nullptr);
    void updateTheme();
    
    qreal thumbPosition() const { return m_thumbPosition; }
    void setThumbPosition(qreal pos) { m_thumbPosition = pos; update(); }
    
    QColor trackColor() const { return m_trackColor; }
    void setTrackColor(const QColor &color) { m_trackColor = color; update(); }

    QSize sizeHint() const override { return QSize(44, 24); }

protected:
    void paintEvent(QPaintEvent *event) override;
    void nextCheckState() override;

private:
    qreal m_thumbPosition = 0.0;
    QColor m_trackColor;
    QVariantAnimation *m_posAnim = nullptr;
    QVariantAnimation *m_colorAnim = nullptr;
};
