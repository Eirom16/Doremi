#pragma once

#include <QLabel>
#include <QTimer>

class ScrollingLabel : public QLabel {
    Q_OBJECT
public:
    explicit ScrollingLabel(QWidget *parent = nullptr);
    explicit ScrollingLabel(const QString &text, QWidget *parent = nullptr);
    
    void setText(const QString &text);

protected:
    void paintEvent(QPaintEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private:
    int m_scrollOffset = 0;
    int m_direction = 1;
    QTimer *m_scrollTimer = nullptr;
    int m_pauseTicks = 0;
    int m_textWidth = 0;
    
    void init();
    void updateTextWidth();
    void onTimerTick();
};
