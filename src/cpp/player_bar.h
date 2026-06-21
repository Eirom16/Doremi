#ifndef DOREMI_PLAYER_BAR_H
#define DOREMI_PLAYER_BAR_H

#include <QWidget>
#include <QPushButton>
#include <QLabel>
#include <QSlider>
#include <QHBoxLayout>

class PlayerBar : public QWidget {
    Q_OBJECT
public:
    explicit PlayerBar(QWidget *parent = nullptr);
    void set_track_info(const std::string &title, const std::string &artist,
                        const std::string &thumbnail);
    void set_progress(int32_t position_ms, int32_t duration_ms);
    void set_playing(bool playing);
    void set_volume_value(int32_t volume);
    void set_shuffle(bool on);
    void set_repeat_mode(int mode); // 0=None, 1=All, 2=One
    void set_compact(bool compact);
    void update_theme();
    bool shuffle_on() const { return shuffle_on_; }
    int  repeat_mode() const { return repeat_mode_; }
signals:
    void play_pause_clicked();
    void next_clicked();
    void previous_clicked();
    void seek_requested(int32_t position_ms);
    void volume_changed(int32_t delta);
    void volume_set(int32_t volume);
    void shuffle_toggled(bool on);
    void repeat_cycled();
    void left_section_clicked();
protected:
    void mousePressEvent(QMouseEvent *event) override;
private:
    QPushButton *prev_btn_;
    QPushButton *play_btn_;
    QPushButton *next_btn_;
    QPushButton *shuffle_btn_;
    QPushButton *repeat_btn_;
    QSlider    *progress_;
    QWidget    *left_container_ = nullptr;
    QWidget    *right_container_ = nullptr;
    QLabel     *track_label_;
    QLabel     *artwork_label_;
    QLabel     *time_label_;
    QSlider    *volume_slider_;
    bool        compact_ = false;
    bool        shuffle_on_ = false;
    int         repeat_mode_ = 0;
    static const char *REPEAT_LABELS[];
};

#endif
