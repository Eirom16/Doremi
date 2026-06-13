#pragma once

#include <QWidget>
#include <QVariantAnimation>
#include <QPixmap>
#include <string>

class AlbumCard : public QWidget {
    Q_OBJECT
    Q_PROPERTY(qreal hoverProgress READ hoverProgress WRITE setHoverProgress)
public:
    explicit AlbumCard(const QString &title, const QString &artist, const QString &thumbnail, QWidget *parent = nullptr);
    
    void setItemId(const std::string &id) { m_itemId = id; }
    std::string itemId() const { return m_itemId; }
    
    qreal hoverProgress() const { return m_hoverProgress; }
    void setHoverProgress(qreal p) { m_hoverProgress = p; update(); }

signals:
    void clicked();
    void playRequested(const std::string &itemId);

protected:
    void enterEvent(QEnterEvent *event) override;
    void leaveEvent(QEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void paintEvent(QPaintEvent *event) override;

private:
    QString m_title;
    QString m_artist;
    QString m_thumbnail;
    std::string m_itemId;
    qreal m_hoverProgress = 0.0;
    QVariantAnimation *m_hoverAnim = nullptr;
    QPixmap m_artPixmap;
    bool m_artLoaded = false;
};
