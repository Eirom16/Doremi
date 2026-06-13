#ifndef DOREMI_NEBULA_BG_H
#define DOREMI_NEBULA_BG_H

#include <QWidget>
#include <QColor>
#include <QTimer>
#include <QElapsedTimer>
#include <QVariantAnimation>

class NebulaBg : public QWidget {
    Q_OBJECT
public:
    explicit NebulaBg(QWidget *parent = nullptr);
    void setColors(const QStringList &hex_colors);
    void setPlaying(bool playing);

protected:
    void paintEvent(QPaintEvent *event) override;

private slots:
    void onTick();

private:
    QTimer *timer_;
    QElapsedTimer time_;
    bool playing_ = false;

    // Current displayed colors
    QColor c1_, c2_, c3_, c4_;
    // Target colors for interpolation
    QColor target_c1_, target_c2_, target_c3_, target_c4_;
    // Interpolation progress (0.0 to 1.0)
    float color_progress_ = 1.0f;
    QElapsedTimer color_transition_time_;

    static const int TRANSITION_DURATION_MS = 2500;

    // Blob parameters: position base + offset
    struct Blob {
        float base_x; // 0.0 to 1.0
        float base_y; // 0.0 to 1.0
        float speed;
        float phase;
        float radius_factor;
    };
    Blob blobs_[4];
};

#endif
