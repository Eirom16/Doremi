#ifndef DOREMI_BAR_CHART_H
#define DOREMI_BAR_CHART_H

#include <QWidget>
#include <QVariantAnimation>
#include <QVector>

class BarChart : public QWidget {
    Q_OBJECT
    Q_PROPERTY(qreal animationProgress READ animationProgress WRITE setAnimationProgress)
public:
    explicit BarChart(QWidget *parent = nullptr);
    void setData(const QVector<int> &values);

    qreal animationProgress() const { return animation_progress_; }
    void setAnimationProgress(qreal progress);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void leaveEvent(QEvent *event) override;
    void showEvent(QShowEvent *event) override;

private:
    QVector<int> values_;
    qreal animation_progress_ = 0.0;
    int hovered_index_ = -1;
    QPoint mouse_pos_;
    
    QVariantAnimation *grow_anim_ = nullptr;
    static const QString DAYS[];
};

#endif
