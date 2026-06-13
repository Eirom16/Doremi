#pragma once

#include <QStackedWidget>

class FadeStack : public QStackedWidget {
    Q_OBJECT
public:
    explicit FadeStack(QWidget *parent = nullptr);
    
    void setCurrentIndex(int index);

private:
    bool m_isTransitioning = false;
};
