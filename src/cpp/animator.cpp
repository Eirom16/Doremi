#include "animator.h"
#include "design_tokens.h"
#include <QGraphicsOpacityEffect>
#include <QPropertyAnimation>
#include <QPoint>
#include <QRect>

void Animator::fadeIn(QWidget *widget, int duration, QEasingCurve::Type easing) {
    if (!widget) return;
    duration = DesignTokens::duration(duration);
    if (duration == 0) {
        widget->show();
        widget->raise();
        widget->setGraphicsEffect(nullptr);
        return;
    }
    
    QGraphicsOpacityEffect *effect = qobject_cast<QGraphicsOpacityEffect*>(widget->graphicsEffect());
    if (!effect) {
        effect = new QGraphicsOpacityEffect(widget);
        widget->setGraphicsEffect(effect);
    }
    
    effect->setOpacity(0.0);
    widget->show();
    widget->raise();
    
    QPropertyAnimation *anim = new QPropertyAnimation(effect, "opacity");
    anim->setDuration(duration);
    anim->setStartValue(0.0);
    anim->setEndValue(1.0);
    anim->setEasingCurve(easing);
    
    QObject::connect(anim, &QPropertyAnimation::finished, [widget, anim]() {
        widget->setGraphicsEffect(nullptr);
        anim->deleteLater();
    });
    
    anim->start();
}

void Animator::fadeOut(QWidget *widget, int duration, QEasingCurve::Type easing) {
    if (!widget || !widget->isVisible()) return;
    duration = DesignTokens::duration(duration);
    if (duration == 0) {
        widget->hide();
        widget->setGraphicsEffect(nullptr);
        return;
    }
    
    QGraphicsOpacityEffect *effect = qobject_cast<QGraphicsOpacityEffect*>(widget->graphicsEffect());
    if (!effect) {
        effect = new QGraphicsOpacityEffect(widget);
        widget->setGraphicsEffect(effect);
    }
    
    QPropertyAnimation *anim = new QPropertyAnimation(effect, "opacity");
    anim->setDuration(duration);
    anim->setStartValue(effect->opacity());
    anim->setEndValue(0.0);
    anim->setEasingCurve(easing);
    
    QObject::connect(anim, &QPropertyAnimation::finished, [widget, anim]() {
        widget->hide();
        widget->setGraphicsEffect(nullptr);
        anim->deleteLater();
    });
    
    anim->start();
}

void Animator::slideIn(QWidget *widget, Direction direction, int duration, QEasingCurve::Type easing) {
    if (!widget) return;
    duration = DesignTokens::duration(duration);
    
    QWidget *parent = widget->parentWidget();
    if (!parent) return;
    
    QRect endRect = widget->geometry();
    QRect startRect = endRect;
    if (duration == 0) {
        widget->show();
        widget->raise();
        return;
    }
    
    int parentWidth = parent->width();
    int parentHeight = parent->height();
    
    switch (direction) {
        case Direction::Left:
            startRect.moveLeft(-endRect.width());
            break;
        case Direction::Right:
            startRect.moveLeft(parentWidth);
            break;
        case Direction::Up:
            startRect.moveTop(-endRect.height());
            break;
        case Direction::Down:
            startRect.moveTop(parentHeight);
            break;
    }
    
    widget->setGeometry(startRect);
    widget->show();
    widget->raise();
    
    QPropertyAnimation *anim = new QPropertyAnimation(widget, "geometry");
    anim->setDuration(duration);
    anim->setStartValue(startRect);
    anim->setEndValue(endRect);
    anim->setEasingCurve(easing);
    
    QObject::connect(anim, &QPropertyAnimation::finished, anim, &QObject::deleteLater);
    anim->start();
}

void Animator::scale(QWidget *widget, qreal startVal, qreal endVal, int duration, QEasingCurve::Type easing) {
    if (!widget) return;
    duration = DesignTokens::duration(duration);
    
    QRect orig = widget->geometry();
    QPoint center = orig.center();
    if (duration == 0) {
        const int w = static_cast<int>(orig.width() * endVal);
        const int h = static_cast<int>(orig.height() * endVal);
        widget->setGeometry(QRect(center.x() - w / 2, center.y() - h / 2, w, h));
        return;
    }
    
    QPropertyAnimation *anim = new QPropertyAnimation(widget, "geometry");
    anim->setDuration(duration);
    
    auto calcRect = [center, orig](qreal factor) {
        int w = static_cast<int>(orig.width() * factor);
        int h = static_cast<int>(orig.height() * factor);
        return QRect(center.x() - w / 2, center.y() - h / 2, w, h);
    };
    
    anim->setStartValue(calcRect(startVal));
    anim->setEndValue(calcRect(endVal));
    anim->setEasingCurve(easing);
    
    QObject::connect(anim, &QPropertyAnimation::finished, anim, &QObject::deleteLater);
    anim->start();
}

void Animator::animateHeight(QWidget *widget, int startHeight, int endHeight, int duration, QEasingCurve::Type easing) {
    if (!widget) return;
    duration = DesignTokens::duration(duration);
    
    widget->setFixedHeight(startHeight);
    widget->show();
    if (duration == 0) {
        widget->setFixedHeight(endHeight);
        return;
    }
    
    QPropertyAnimation *anim = new QPropertyAnimation(widget, "maximumHeight");
    anim->setDuration(duration);
    anim->setStartValue(startHeight);
    anim->setEndValue(endHeight);
    anim->setEasingCurve(easing);
    
    QObject::connect(anim, &QPropertyAnimation::finished, [widget, endHeight, anim]() {
        widget->setFixedHeight(endHeight);
        anim->deleteLater();
    });
    
    anim->start();
}
