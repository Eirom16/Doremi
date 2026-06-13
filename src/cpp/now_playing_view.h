#ifndef DOREMI_NOW_PLAYING_VIEW_H
#define DOREMI_NOW_PLAYING_VIEW_H

#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QStackedWidget>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include "components/nebula_bg.h"
#include "components/vinyl_disc.h"
#include "components/lyrics_widget.h"
#include "components/queue_panel.h"
#include "components/waveform_bars.h"
#include "components/animated_progress.h"

class NowPlayingView : public QWidget {
    Q_OBJECT
public:
    explicit NowPlayingView(QWidget *parent = nullptr);
    
    void showView();
    void hideView();
    
    void setTrackInfo(const std::string &title, const std::string &artist, const std::string &thumbnail);
    void setPlaybackState(int32_t state, int32_t position_ms, int32_t duration_ms);
    void setPlaying(bool playing);
    void setShuffle(bool on);
    void setRepeatMode(int mode);
    void setDominantColors(const QStringList &colors);
    void setLyrics(const QString &plain, const QString &synced);
    void setQueue(const QStringList &titles, const QStringList &artists, const QStringList &thumbnails, int current_index);
    
    void setSubtitleAlignment(const std::string &alignment);
    void setSubtitleFontSize(int32_t size);
    void setSubtitleLineSpacing(double spacing);
    void setSubtitleAutoScroll(bool enabled);
    void setSubtitleGlowEffect(bool enabled);


signals:
    void close_clicked();
    void play_pause_clicked();
    void next_clicked();
    void previous_clicked();
    void seek_requested(int32_t position_ms);
    void shuffle_toggled(bool on);
    void repeat_cycled();

protected:
    void resizeEvent(QResizeEvent *event) override;

private:
    void setupLayout();
    void updateButtonsStyle();

    NebulaBg *nebula_bg_;
    
    // Left side: Player Controls
    VinylDisc *vinyl_disc_;
    QLabel *title_label_;
    QLabel *artist_label_;
    WaveformBars *waveform_bars_;
    AnimatedProgress *progress_bar_;
    QLabel *time_label_;
    
    QPushButton *prev_btn_;
    QPushButton *play_btn_;
    QPushButton *next_btn_;
    QPushButton *shuffle_btn_;
    QPushButton *repeat_btn_;
    
    // Right side: Tabs (Lyrics / Queue)
    QPushButton *lyrics_tab_btn_;
    QPushButton *queue_tab_btn_;
    QStackedWidget *tabs_stack_;
    
    LyricsWidget *lyrics_widget_;
    QueuePanel *queue_panel_;
    
    bool is_playing_ = false;
    bool shuffle_on_ = false;
    int repeat_mode_ = 0;
};

#endif
