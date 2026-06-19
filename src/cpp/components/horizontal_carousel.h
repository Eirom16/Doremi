#pragma once

#include <QWidget>
#include <QScrollArea>
#include <QPushButton>
#include <QHBoxLayout>
#include <QVariantAnimation>

class HorizontalCarousel : public QWidget {
    Q_OBJECT
    Q_PROPERTY(int scrollValue READ scrollValue WRITE setScrollValue)
public:
    explicit HorizontalCarousel(QWidget *parent = nullptr);
    
    void addWidget(QWidget *widget);
    void clear();
    
    int scrollValue() const;
    void setScrollValue(int val);

protected:
    void enterEvent(QEnterEvent *event) override;
    void leaveEvent(QEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    QSize sizeHint() const override;

private:
    QScrollArea *m_scrollArea = nullptr;
    QWidget *m_contentWidget = nullptr;
    QHBoxLayout *m_contentLayout = nullptr;
    QPushButton *m_leftBtn = nullptr;
    QPushButton *m_rightBtn = nullptr;
    QVariantAnimation *m_scrollAnim = nullptr;
    int m_minContentHeight = 0;

    void init();
    void scroll(int delta);
    void updateButtonVisibility();
};
