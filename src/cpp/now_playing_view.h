#ifndef DOREMI_NOW_PLAYING_VIEW_H
#define DOREMI_NOW_PLAYING_VIEW_H

#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QStackedWidget>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include "components/artwork_backdrop.h"
#include "components/nebula_bg.h"
#include "components/vinyl_disc.h"
#include "components/lyrics_widget.h"
#include "components/queue_panel.h"
#include "components/related_tracks_widget.h"
#include "components/waveform_bars.h"
#include "components/animated_progress.h"
#include "doremi/src/bridge.rs.h"

class NowPlayingView : public QWidget {
    Q_OBJECT
public:
    explicit NowPlayingView(QWidget *parent = nullptr);
    
    void showView();
    void hideView();
    
    void setTrackInfo(const std::string &title, const std::string &artist, const std::string &thumbnail);
    void setCurrentTrack(const Track &track);
    void setPlaybackState(int32_t state, int32_t position_ms, int32_t duration_ms);
    void setPlaying(bool playing);
    void setShuffle(bool on);
    void setRepeatMode(int mode);
    void setDominantColors(const QStringList &colors);
    void setLyrics(const QString &plain, const QString &synced);
    void setQueue(const std::vector<Track> &tracks, int current_index);
    void setRelatedTracks(const std::vector<Track> &tracks);
    
    void setSubtitleAlignment(const std::string &alignment);
    void setSubtitleFontSize(int32_t size);
    void setSubtitleLineSpacing(double spacing);
    void setSubtitleAutoScroll(bool enabled);
    void setSubtitleGlowEffect(bool enabled);


    void update_theme();
signals:
    void close_clicked();
    void play_pause_clicked();
    void next_clicked();
    void previous_clicked();
    void seek_requested(int32_t position_ms);
    void shuffle_toggled(bool on);
    void repeat_cycled();
    void related_play_requested(const Track &track);
    void related_add_to_queue_requested(const Track &track);
    void like_clicked(const Track &track);
    void download_clicked(const Track &track);

protected:
    void resizeEvent(QResizeEvent *event) override;

private:
    void setupLayout();
    void updateButtonsStyle();
    void updateLikeButtonState(bool is_favorite);

    ArtworkBackdrop *artwork_backdrop_;
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
    QPushButton *like_btn_;
    QPushButton *download_btn_;
    
    // Right side: Tabs (Lyrics / Queue / Related)
    QPushButton *lyrics_tab_btn_;
    QPushButton *queue_tab_btn_;
    QPushButton *related_tab_btn_;
    QStackedWidget *tabs_stack_;
    
    LyricsWidget *lyrics_widget_;
    QueuePanel *queue_panel_;
    RelatedTracksWidget *related_widget_;
    
    bool is_playing_ = false;
    bool shuffle_on_ = false;
    int repeat_mode_ = 0;
    Track current_track_;
};

#endif
