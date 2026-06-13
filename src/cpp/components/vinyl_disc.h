#ifndef DOREMI_VINYL_DISC_H
#define DOREMI_VINYL_DISC_H

#include <QWidget>
#include <QPixmap>
#include <QTimer>
#include <QTime>

class VinylDisc : public QWidget {
    Q_OBJECT
public:
    explicit VinylDisc(QWidget *parent = nullptr);
    void setArtwork(const QString &thumbnail_path);
    void setPlaying(bool playing);

protected:
    void paintEvent(QPaintEvent *event) override;

private slots:
    void onTick();

private:
    QPixmap artwork_;
    QTimer *timer_;
    float angle_ = 0.0f;
    bool playing_ = false;

    float current_speed_ = 0.0f;
    float target_speed_ = 0.0f;
    static constexpr float MAX_SPEED = 0.75f; // degrees per frame
    static constexpr float ACCEL = 0.02f;     // acceleration rate
    static constexpr float DECEL = 0.015f;    // deceleration rate
};

#endif
