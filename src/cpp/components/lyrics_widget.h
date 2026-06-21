#ifndef DOREMI_LYRICS_WIDGET_H
#define DOREMI_LYRICS_WIDGET_H

#include <QScrollArea>
#include <QLabel>
#include <QVBoxLayout>
#include <QVector>
#include <QPair>
#include <QTime>
#include <QPropertyAnimation>
#include <QGraphicsOpacityEffect>

#include <QPushButton>
#include <QHBoxLayout>

// Custom interactive label for lyrics
class LyricLabel : public QLabel {
    Q_OBJECT
    Q_PROPERTY(int fontSize READ fontSize WRITE setFontSize)
public:
    explicit LyricLabel(const QString &text, int time_ms, QWidget *parent = nullptr);
    int timeMs() const { return time_ms_; }
    
    int fontSize() const { return font_size_; }
    void setFontSize(int size);

signals:
    void clicked(int time_ms);

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void enterEvent(QEnterEvent *event) override;
    void leaveEvent(QEvent *event) override;

private:
    int time_ms_;
    int font_size_;
    bool is_hovered_ = false;
};

// Main Lyrics Widget
class LyricsWidget : public QWidget {
    Q_OBJECT
public:
    explicit LyricsWidget(QWidget *parent = nullptr);
    void setLyrics(const QString &plain, const QString &synced);
    void updatePosition(int position_ms);

    void setSubtitleAlignment(const std::string &alignment);
    void setSubtitleFontSize(int size);
    void setSubtitleLineSpacing(double spacing);
    void setSubtitleAutoScroll(bool enabled);
    void setSubtitleGlowEffect(bool enabled);

signals:
    void seek_requested(int position_ms);

protected:
    void resizeEvent(QResizeEvent *event) override;

private slots:
    void onSyncMinusClicked();
    void onSyncPlusClicked();
    void onSyncResetClicked();

private:
    void parseLrc(const QString &lrc_text);
    void clearLayout();
    void buildLyricsLayout();
    void highlightLine(int index);
    void smoothScrollTo(int y_pos);
    void updateSyncLabel();
    void updateSyncControlBarVisibility();

    struct LyricLine {
        int time_ms;
        QString text;
        LyricLabel *label = nullptr;
        QGraphicsOpacityEffect *opacity_effect = nullptr;
    };

    QScrollArea *scroll_area_;
    QWidget *container_;
    QVBoxLayout *layout_;
    QVector<LyricLine> lines_;
    int active_index_ = -1;
    
    // Smooth scrolling animation
    QPropertyAnimation *scroll_animation_;
    
    // Active line zoom/fade animations
    QPropertyAnimation *font_animation_;
    QPropertyAnimation *opacity_animation_;
    
    // Spacers to allow centering top/bottom lines
    QWidget *top_spacer_;
    QWidget *bottom_spacer_;

    // Sync time caption
    QLabel *time_caption_;

    bool has_synced_lyrics_ = false;

    // Manual lyrics synchronization delay (in ms)
    int manual_delay_ms_ = 0;
    int last_position_ms_ = 0;

    // Sync Control Bar UI
    QWidget *sync_control_bar_ = nullptr;
    QLabel *sync_offset_lbl_ = nullptr;
    QPushButton *sync_minus_btn_ = nullptr;
    QPushButton *sync_plus_btn_ = nullptr;
    QPushButton *sync_reset_btn_ = nullptr;

    std::string alignment_ = "center";
    int base_font_size_ = 15;
    double line_spacing_multiplier_ = 1.5;
    bool auto_scroll_enabled_ = true;
    bool glow_effect_enabled_ = true;
};

#endif
