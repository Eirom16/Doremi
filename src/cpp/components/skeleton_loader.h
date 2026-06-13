#pragma once

#include <QWidget>
#include <QVariantAnimation>

class SkeletonLoader : public QWidget {
    Q_OBJECT
    Q_PROPERTY(qreal shimmerOffset READ shimmerOffset WRITE setShimmerOffset)
public:
    explicit SkeletonLoader(QWidget *parent = nullptr);
    
    qreal shimmerOffset() const { return m_shimmerOffset; }
    void setShimmerOffset(qreal offset) { m_shimmerOffset = offset; update(); }

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    qreal m_shimmerOffset = -0.3;
    QVariantAnimation *m_shimmerAnim = nullptr;
};
