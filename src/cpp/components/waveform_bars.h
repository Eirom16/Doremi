#pragma once

#include <QWidget>
#include <QTimer>
#include <vector>

class WaveformBars : public QWidget {
    Q_OBJECT
public:
    explicit WaveformBars(QWidget *parent = nullptr, int barCount = 5);

    void setPlaying(bool playing);
    bool isPlaying() const { return m_isPlaying; }

    QSize sizeHint() const override { return QSize(m_barCount * 6, 24); }

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    int m_barCount;
    bool m_isPlaying = false;
    QTimer *m_timer = nullptr;
    std::vector<qreal> m_heights;
    std::vector<qreal> m_targetHeights;
    
    void onTimerTick();
};
