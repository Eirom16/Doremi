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
class LyricsWidget : public QScrollArea {
    Q_OBJECT
public:
    explicit LyricsWidget(QWidget *parent = nullptr);
    void setLyrics(const QString &plain, const QString &synced);
    void updatePosition(int position_ms);

signals:
    void seek_requested(int position_ms);

protected:
    void resizeEvent(QResizeEvent *event) override;

private:
    void parseLrc(const QString &lrc_text);
    void clearLayout();
    void buildLyricsLayout();
    void highlightLine(int index);
    void smoothScrollTo(int y_pos);

    struct LyricLine {
        int time_ms;
        QString text;
        LyricLabel *label = nullptr;
        QGraphicsOpacityEffect *opacity_effect = nullptr;
    };

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
    
    bool has_synced_lyrics_ = false;
};

#endif
