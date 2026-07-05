#ifndef DOREMI_NOW_PLAYING_VIEW_H
#define DOREMI_NOW_PLAYING_VIEW_H

#include <QWidget>
#include <QQuickWidget>
#include <QVariantList>
#include <QVariantMap>
#include "doremi/src/bridge.rs.h"

class NowPlayingView : public QWidget {
    Q_OBJECT
    Q_PROPERTY(QString title READ title NOTIFY titleChanged)
    Q_PROPERTY(QString artist READ artist NOTIFY artistChanged)
    Q_PROPERTY(QString artworkUrl READ artworkUrl NOTIFY artworkUrlChanged)
    Q_PROPERTY(bool isPlaying READ isPlaying NOTIFY isPlayingChanged)
    Q_PROPERTY(int durationMs READ durationMs NOTIFY durationMsChanged)
    Q_PROPERTY(int positionMs READ positionMs NOTIFY positionMsChanged)
    Q_PROPERTY(bool shuffleOn READ shuffleOn NOTIFY shuffleOnChanged)
    Q_PROPERTY(int repeatMode READ repeatMode NOTIFY repeatModeChanged)
    Q_PROPERTY(QVariantList queue READ queue NOTIFY queueChanged)
    Q_PROPERTY(int currentIndex READ currentIndex NOTIFY currentIndexChanged)
    Q_PROPERTY(QVariantList relatedTracks READ relatedTracks NOTIFY relatedTracksChanged)
    Q_PROPERTY(QString plainLyrics READ plainLyrics NOTIFY plainLyricsChanged)
    Q_PROPERTY(QString syncedLyrics READ syncedLyrics NOTIFY syncedLyricsChanged)
    Q_PROPERTY(QStringList dominantColors READ dominantColors NOTIFY dominantColorsChanged)
    
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
    
    // Stub these for compatibility
    void setSubtitleAlignment(const std::string &) {}
    void setSubtitleFontSize(int32_t) {}
    void setSubtitleLineSpacing(double) {}
    void setSubtitleAutoScroll(bool) {}
    void setSubtitleGlowEffect(bool) {}
    void update_theme() {}

    // Getters
    QString title() const { return title_; }
    QString artist() const { return artist_; }
    QString artworkUrl() const { return artworkUrl_; }
    bool isPlaying() const { return is_playing_; }
    int durationMs() const { return duration_ms_; }
    int positionMs() const { return position_ms_; }
    bool shuffleOn() const { return shuffle_on_; }
    int repeatMode() const { return repeat_mode_; }
    QVariantList queue() const { return queue_; }
    int currentIndex() const { return current_index_; }
    QVariantList relatedTracks() const { return related_tracks_; }
    QString plainLyrics() const { return plain_lyrics_; }
    QString syncedLyrics() const { return synced_lyrics_; }
    QStringList dominantColors() const { return dominant_colors_; }

signals:
    // Model signals
    void titleChanged();
    void artistChanged();
    void artworkUrlChanged();
    void isPlayingChanged();
    void durationMsChanged();
    void positionMsChanged();
    void shuffleOnChanged();
    void repeatModeChanged();
    void queueChanged();
    void currentIndexChanged();
    void relatedTracksChanged();
    void plainLyricsChanged();
    void syncedLyricsChanged();
    void dominantColorsChanged();

    // Action signals (routed to main_window)
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
    
public slots:
    // Invokables from QML
    void togglePlayPause() { emit play_pause_clicked(); }
    void next() { emit next_clicked(); }
    void previous() { emit previous_clicked(); }
    void seek(int position_ms) { emit seek_requested(position_ms); }
    void toggleShuffle() { emit shuffle_toggled(!shuffle_on_); }
    void cycleRepeat() { emit repeat_cycled(); }
    void closeView() { hideView(); }
    void playQueueItem(int index);
    void removeQueueItem(int index);
    void moveQueueItem(int from, int to);
    void clearQueue();
    void toggleLike() { emit like_clicked(current_track_); }
    void downloadCurrent() { emit download_clicked(current_track_); }
    
    void playRelated(int index);
    void queueRelated(int index);

protected:
    void resizeEvent(QResizeEvent *event) override;

private:
    QQuickWidget *quick_widget_;
    
    QString title_;
    QString artist_;
    QString artworkUrl_;
    bool is_playing_ = false;
    int duration_ms_ = 0;
    int position_ms_ = 0;
    bool shuffle_on_ = false;
    int repeat_mode_ = 0;
    QVariantList queue_;
    int32_t current_index_ = -1;
    QVariantList related_tracks_;
    std::vector<Track> raw_related_tracks_;
    QString plain_lyrics_;
    QString synced_lyrics_;
    QStringList dominant_colors_;
    
    Track current_track_;
};

#endif
