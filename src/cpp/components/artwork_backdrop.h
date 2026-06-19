#ifndef DOREMI_ARTWORK_BACKDROP_H
#define DOREMI_ARTWORK_BACKDROP_H

#include <QWidget>
#include <QPixmap>
#include <QImage>
#include <QTimer>

class ArtworkBackdrop : public QWidget {
    Q_OBJECT
public:
    explicit ArtworkBackdrop(QWidget *parent = nullptr);

    void setImage(const QString &path);
    void setImage(const QPixmap &pixmap);
    void clear();

protected:
    void paintEvent(QPaintEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private:
    void updateBlurred();

    QPixmap source_;
    QPixmap blurred_;
    QTimer *debounce_timer_;
};

#endif
