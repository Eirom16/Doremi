#include "fade_stack.h"
#include "../design_tokens.h"
#include <QGraphicsOpacityEffect>
#include <QPropertyAnimation>

FadeStack::FadeStack(QWidget *parent)
    : QStackedWidget(parent)
{
}

void FadeStack::setCurrentIndex(int index) {
    if (index == currentIndex() || m_isTransitioning) {
        QStackedWidget::setCurrentIndex(index);
        return;
    }
    
    QWidget *currentW = currentWidget();
    QWidget *nextW = widget(index);
    if (!currentW || !nextW) {
        QStackedWidget::setCurrentIndex(index);
        return;
    }

    currentW->setGraphicsEffect(nullptr);
    nextW->setGraphicsEffect(nullptr);

    if (DesignTokens::reducedMotion()) {
        m_isTransitioning = false;
        QStackedWidget::setCurrentIndex(index);
        return;
    }
    
    m_isTransitioning = true;
    
    // 1. Fade out current widget
    QGraphicsOpacityEffect *outEffect = new QGraphicsOpacityEffect(currentW);
    currentW->setGraphicsEffect(outEffect);
    
    QPropertyAnimation *outAnim = new QPropertyAnimation(outEffect, "opacity");
    outAnim->setDuration(DesignTokens::duration(120));
    outAnim->setStartValue(1.0);
    outAnim->setEndValue(0.0);
    outAnim->setEasingCurve(QEasingCurve::InOutQuad);
    
    connect(outAnim, &QPropertyAnimation::finished, this, [this, index, currentW, nextW, outAnim]() {
        currentW->setGraphicsEffect(nullptr);
        outAnim->deleteLater();
        
        // 2. Change page
        QStackedWidget::setCurrentIndex(index);
        
        // 3. Fade in next widget
        QGraphicsOpacityEffect *inEffect = new QGraphicsOpacityEffect(nextW);
        nextW->setGraphicsEffect(inEffect);
        
        QPropertyAnimation *inAnim = new QPropertyAnimation(inEffect, "opacity");
        inAnim->setDuration(DesignTokens::duration(120));
        inAnim->setStartValue(0.0);
        inAnim->setEndValue(1.0);
        inAnim->setEasingCurve(QEasingCurve::InOutQuad);
        
        connect(inAnim, &QPropertyAnimation::finished, this, [this, nextW, inAnim]() {
            nextW->setGraphicsEffect(nullptr);
            inAnim->deleteLater();
            m_isTransitioning = false;
        });
        
        inAnim->start();
    });
    
    outAnim->start();
}
