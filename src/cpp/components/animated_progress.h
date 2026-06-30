#pragma once

#include <QSlider>
#include <QVariantAnimation>

class AnimatedProgress : public QSlider {
    Q_OBJECT
    Q_PROPERTY(qreal hoverProgress READ hoverProgress WRITE setHoverProgress)
public:
    explicit AnimatedProgress(Qt::Orientation orientation, QWidget *parent = nullptr);

    qreal hoverProgress() const { return m_hoverProgress; }
    void setHoverProgress(qreal p) { m_hoverProgress = p; update(); }
    bool isSliderDown() const;

protected:
    void enterEvent(QEnterEvent *event) override;
    void leaveEvent(QEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void paintEvent(QPaintEvent *event) override;

private:
    qreal m_hoverProgress = 0.0;
    QVariantAnimation *m_hoverAnim = nullptr;
    bool m_isHovered = false;
    bool m_isDragging = false;
};
