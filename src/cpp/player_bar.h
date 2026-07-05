#ifndef DOREMI_PLAYER_BAR_H
#define DOREMI_PLAYER_BAR_H

#include <QWidget>
#include <QQuickWidget>
#include <QString>
#include <string>

class PlayerBar : public QWidget {
    Q_OBJECT
    Q_PROPERTY(QString title READ title NOTIFY titleChanged)
    Q_PROPERTY(QString artist READ artist NOTIFY artistChanged)
    Q_PROPERTY(QString thumbnail READ thumbnail NOTIFY thumbnailChanged)
    Q_PROPERTY(int positionMs READ positionMs NOTIFY positionMsChanged)
    Q_PROPERTY(int durationMs READ durationMs NOTIFY durationMsChanged)
    Q_PROPERTY(bool isPlaying READ isPlaying NOTIFY isPlayingChanged)
    Q_PROPERTY(int volumeValue READ volumeValue NOTIFY volumeValueChanged)
    Q_PROPERTY(bool shuffleOn READ shuffleOn NOTIFY shuffleOnChanged)
    Q_PROPERTY(int repeatMode READ repeatMode NOTIFY repeatModeChanged)
    Q_PROPERTY(bool isCompact READ isCompact NOTIFY isCompactChanged)

public:
    explicit PlayerBar(QWidget *parent = nullptr);

    // C++ setters (called from MainWindow / rust bridge)
    void set_track_info(const std::string &title, const std::string &artist, const std::string &thumbnail);
    void set_progress(int32_t position_ms, int32_t duration_ms);
    void set_playing(bool playing);
    void set_volume_value(int32_t volume);
    void set_shuffle(bool on);
    void set_repeat_mode(int mode); // 0=None, 1=All, 2=One
    void set_compact(bool compact);
    
    bool shuffle_on() const { return shuffle_on_; }
    int  repeat_mode() const { return repeat_mode_; }
    
    // Stub for theme updates
    void update_theme() {}

    // Getters for Q_PROPERTY
    QString title() const { return title_; }
    QString artist() const { return artist_; }
    QString thumbnail() const { return thumbnail_; }
    int positionMs() const { return position_ms_; }
    int durationMs() const { return duration_ms_; }
    bool isPlaying() const { return is_playing_; }
    int volumeValue() const { return volume_value_; }
    bool shuffleOn() const { return shuffle_on_; }
    int repeatMode() const { return repeat_mode_; }
    bool isCompact() const { return compact_; }

signals:
    // C++ outward signals
    void play_pause_clicked();
    void next_clicked();
    void previous_clicked();
    void seek_requested(int32_t position_ms);
    void volume_changed(int32_t delta);
    void volume_set(int32_t volume);
    void shuffle_toggled(bool on);
    void repeat_cycled();
    void left_section_clicked();

    // Q_PROPERTY notify signals
    void titleChanged();
    void artistChanged();
    void thumbnailChanged();
    void positionMsChanged();
    void durationMsChanged();
    void isPlayingChanged();
    void volumeValueChanged();
    void shuffleOnChanged();
    void repeatModeChanged();
    void isCompactChanged();

public slots:
    // Invokables from QML
    void togglePlayPause() { emit play_pause_clicked(); }
    void next() { emit next_clicked(); }
    void previous() { emit previous_clicked(); }
    void seek(int position) { emit seek_requested(position); }
    void setVolume(int volume) { emit volume_set(volume); }
    void toggleShuffle() { emit shuffle_toggled(!shuffle_on_); }
    void cycleRepeat() { emit repeat_cycled(); }
    void clickLeftSection() { emit left_section_clicked(); }

protected:
    void resizeEvent(QResizeEvent *event) override;

private:
    QQuickWidget *quick_widget_;

    QString title_ = "No Track";
    QString artist_ = "Unknown Artist";
    QString thumbnail_ = "";
    int position_ms_ = 0;
    int duration_ms_ = 0;
    bool is_playing_ = false;
    int volume_value_ = 50;
    bool shuffle_on_ = false;
    int repeat_mode_ = 0;
    bool compact_ = false;
};

#endif
