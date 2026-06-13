#pragma once

#include <QWidget>

class GlassPanel : public QWidget {
    Q_OBJECT
public:
    explicit GlassPanel(QWidget *parent = nullptr);
    
    void showAnimated();
    void hideAnimated();

protected:
    void paintEvent(QPaintEvent *event) override;
};
