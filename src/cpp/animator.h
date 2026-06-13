#pragma once

#include <QWidget>
#include <QEasingCurve>
#include <QPropertyAnimation>
#include <QColor>

class Animator {
public:
    enum class Direction {
        Left,
        Right,
        Up,
        Down
    };

    static void fadeIn(QWidget *widget, int duration = 250, QEasingCurve::Type easing = QEasingCurve::OutCubic);
    static void fadeOut(QWidget *widget, int duration = 200, QEasingCurve::Type easing = QEasingCurve::InCubic);
    static void slideIn(QWidget *widget, Direction direction, int duration = 300, QEasingCurve::Type easing = QEasingCurve::OutCubic);
    static void scale(QWidget *widget, qreal startVal, qreal endVal, int duration = 200, QEasingCurve::Type easing = QEasingCurve::OutCubic);
    static void animateHeight(QWidget *widget, int startHeight, int endHeight, int duration = 300, QEasingCurve::Type easing = QEasingCurve::OutCubic);
};
