#pragma once

#include <QPushButton>
#include <QVariantAnimation>

class IconButton : public QPushButton {
    Q_OBJECT
    Q_PROPERTY(qreal hoverProgress READ hoverProgress WRITE setHoverProgress)
public:
    explicit IconButton(const QString &iconName, QWidget *parent = nullptr, int iconSize = 20);

    qreal hoverProgress() const { return m_hoverProgress; }
    void setHoverProgress(qreal p) { m_hoverProgress = p; update(); }

protected:
    void enterEvent(QEnterEvent *event) override;
    void leaveEvent(QEvent *event) override;
    void paintEvent(QPaintEvent *event) override;

private:
    QString m_iconName;
    int m_iconSize;
    qreal m_hoverProgress = 0.0;
    QVariantAnimation *m_hoverAnim = nullptr;
};
