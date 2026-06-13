#include "now_playing_view.h"
#include "design_tokens.h"
#include "icon_provider.h"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QPropertyAnimation>
#include <QPainter>
#include <QGraphicsOpacityEffect>
#include "doremi/src/bridge.rs.h"


NowPlayingView::NowPlayingView(QWidget *parent)
    : QWidget(parent)
{
    // Setup background first
    nebula_bg_ = new NebulaBg(this);
    nebula_bg_->lower(); // Send to back

    setupLayout();

    // Default styles for buttons
    updateButtonsStyle();
}

void NowPlayingView::setupLayout() {
    const auto &c = DesignTokens::current();

    // Main layout of the view
    auto *main_layout = new QVBoxLayout(this);
    main_layout->setContentsMargins(32, 24, 32, 32);
    main_layout->setSpacing(20);

    // ── TOP HEADER BAR ──
    auto *header_layout = new QHBoxLayout();
    header_layout->setContentsMargins(0, 0, 0, 0);

    auto *close_btn = new QPushButton(this);
    close_btn->setFixedSize(40, 40);
    close_btn->setCursor(Qt::PointingHandCursor);
    close_btn->setIcon(IconProvider::getIcon("expand_more", c.text_primary, 24));
    close_btn->setStyleSheet("QPushButton { background: rgba(255,255,255,0.06); border: none; border-radius: 20px; }"
                             "QPushButton:hover { background: rgba(255,255,255,0.12); }");
    header_layout->addWidget(close_btn);
    header_layout->addStretch();
    
    main_layout->addLayout(header_layout);

    // ── BODY LAYOUT (SPLIT LEFT/RIGHT) ──
    auto *body_layout = new QHBoxLayout();
    body_layout->setContentsMargins(0, 0, 0, 0);
    body_layout->setSpacing(48);

    // LEFT COLUMN: Artwork, metadata, progress, playback controls
    auto *left_column = new QVBoxLayout();
    left_column->setContentsMargins(0, 0, 0, 0);
    left_column->setSpacing(16);
    left_column->setAlignment(Qt::AlignCenter);

    vinyl_disc_ = new VinylDisc(this);
    left_column->addWidget(vinyl_disc_, 0, Qt::AlignCenter);

    // Metadata layout
    auto *meta_layout = new QVBoxLayout();
    meta_layout->setContentsMargins(0, 8, 0, 0);
    meta_layout->setSpacing(4);
    meta_layout->setAlignment(Qt::AlignCenter);

    title_label_ = new QLabel("Sin reproducción", this);
    title_label_->setFont(DesignTokens::getFont("heading_lg", 18));
    title_label_->setStyleSheet(QString("color: %1; font-weight: bold;").arg(c.text_primary.name()));
    title_label_->setAlignment(Qt::AlignCenter);

    artist_label_ = new QLabel("Ningún track", this);
    artist_label_->setFont(DesignTokens::getFont("body", 13));
    artist_label_->setStyleSheet(QString("color: %1;").arg(c.text_secondary.name()));
    artist_label_->setAlignment(Qt::AlignCenter);

    meta_layout->addWidget(title_label_);
    meta_layout->addWidget(artist_label_);
    left_column->addLayout(meta_layout);

    // Waveform bars visualizer
    waveform_bars_ = new WaveformBars(this);
    left_column->addWidget(waveform_bars_, 0, Qt::AlignCenter);

    // Progress row
    auto *progress_layout = new QVBoxLayout();
    progress_layout->setSpacing(4);
    progress_layout->setContentsMargins(24, 0, 24, 0);

    progress_bar_ = new AnimatedProgress(Qt::Horizontal, this);
    progress_bar_->setFixedHeight(16);

    
    time_label_ = new QLabel("0:00 / 0:00", this);
    time_label_->setFont(DesignTokens::getFont("caption", 11));
    time_label_->setStyleSheet(QString("color: %1;").arg(c.text_muted.name()));
    time_label_->setAlignment(Qt::AlignCenter);

    progress_layout->addWidget(progress_bar_);
    progress_layout->addWidget(time_label_);
    left_column->addLayout(progress_layout);

    // Playback control buttons row
    auto *controls_layout = new QHBoxLayout();
    controls_layout->setSpacing(20);
    controls_layout->setAlignment(Qt::AlignCenter);

    shuffle_btn_ = new QPushButton(this);
    shuffle_btn_->setFixedSize(36, 36);
    shuffle_btn_->setCursor(Qt::PointingHandCursor);
    shuffle_btn_->setIcon(IconProvider::getIcon("shuffle", c.text_secondary, 20));
    shuffle_btn_->setStyleSheet("QPushButton { background: transparent; border: none; }");

    prev_btn_ = new QPushButton(this);
    prev_btn_->setFixedSize(40, 40);
    prev_btn_->setCursor(Qt::PointingHandCursor);
    prev_btn_->setIcon(IconProvider::getIcon("skip_previous", c.text_primary, 24));
    prev_btn_->setStyleSheet("QPushButton { background: transparent; border: none; }");

    play_btn_ = new QPushButton(this);
    play_btn_->setFixedSize(56, 56);
    play_btn_->setCursor(Qt::PointingHandCursor);
    play_btn_->setIcon(IconProvider::getIcon("play_arrow", QColor("#FFFFFF"), 32));
    
    // Play button layout
    QString play_style = QString(
        "QPushButton {\n"
        "    background-color: %1;\n"
        "    border: none;\n"
        "    border-radius: 28px;\n"
        "}\n"
        "QPushButton:hover {\n"
        "    background-color: %2;\n"
        "}\n"
    )
    .arg(c.accent.name())
    .arg(c.accent_bright.name());
    play_btn_->setStyleSheet(play_style);

    next_btn_ = new QPushButton(this);
    next_btn_->setFixedSize(40, 40);
    next_btn_->setCursor(Qt::PointingHandCursor);
    next_btn_->setIcon(IconProvider::getIcon("skip_next", c.text_primary, 24));
    next_btn_->setStyleSheet("QPushButton { background: transparent; border: none; }");

    repeat_btn_ = new QPushButton(this);
    repeat_btn_->setFixedSize(36, 36);
    repeat_btn_->setCursor(Qt::PointingHandCursor);
    repeat_btn_->setIcon(IconProvider::getIcon("repeat", c.text_secondary, 20));
    repeat_btn_->setStyleSheet("QPushButton { background: transparent; border: none; }");

    controls_layout->addWidget(shuffle_btn_);
    controls_layout->addWidget(prev_btn_);
    controls_layout->addWidget(play_btn_);
    controls_layout->addWidget(next_btn_);
    controls_layout->addWidget(repeat_btn_);
    left_column->addLayout(controls_layout);

    body_layout->addLayout(left_column, 4);

    // RIGHT COLUMN: Tabs (Lyrics & Queue)
    auto *right_column = new QVBoxLayout();
    right_column->setContentsMargins(0, 0, 0, 0);
    right_column->setSpacing(16);

    // Tab buttons header
    auto *tabs_bar = new QHBoxLayout();
    tabs_bar->setSpacing(8);
    tabs_bar->setAlignment(Qt::AlignLeft);

    lyrics_tab_btn_ = new QPushButton("Letras", this);
    lyrics_tab_btn_->setFont(DesignTokens::getFont("heading_sm", 13));
    lyrics_tab_btn_->setCursor(Qt::PointingHandCursor);
    lyrics_tab_btn_->setCheckable(true);
    lyrics_tab_btn_->setChecked(true);
    
    queue_tab_btn_ = new QPushButton("Cola de reproducción", this);
    queue_tab_btn_->setFont(DesignTokens::getFont("heading_sm", 13));
    queue_tab_btn_->setCursor(Qt::PointingHandCursor);
    queue_tab_btn_->setCheckable(true);

    tabs_bar->addWidget(lyrics_tab_btn_);
    tabs_bar->addWidget(queue_tab_btn_);
    right_column->addLayout(tabs_bar);

    // Stacked widget containing lyrics and queue
    tabs_stack_ = new QStackedWidget(this);
    tabs_stack_->setStyleSheet("background: transparent;");

    lyrics_widget_ = new LyricsWidget(this);
    queue_panel_ = new QueuePanel(this);

    tabs_stack_->addWidget(lyrics_widget_);
    tabs_stack_->addWidget(queue_panel_);

    right_column->addWidget(tabs_stack_, 1);

    body_layout->addLayout(right_column, 5);

    main_layout->addLayout(body_layout, 1);
    setLayout(main_layout);

    // Setup connections
    connect(close_btn, &QPushButton::clicked, this, &NowPlayingView::hideView);
    connect(play_btn_, &QPushButton::clicked, this, &NowPlayingView::play_pause_clicked);
    connect(next_btn_, &QPushButton::clicked, this, &NowPlayingView::next_clicked);
    connect(prev_btn_, &QPushButton::clicked, this, &NowPlayingView::previous_clicked);
    
    connect(shuffle_btn_, &QPushButton::clicked, this, [this]() {
        shuffle_on_ = !shuffle_on_;
        updateButtonsStyle();
        emit shuffle_toggled(shuffle_on_);
    });

    connect(repeat_btn_, &QPushButton::clicked, this, [this]() {
        repeat_mode_ = (repeat_mode_ + 1) % 3;
        updateButtonsStyle();
        emit repeat_cycled();
    });

    connect(progress_bar_, &QSlider::sliderMoved, this, &NowPlayingView::seek_requested);
    connect(lyrics_widget_, &LyricsWidget::seek_requested, this, &NowPlayingView::seek_requested);


    connect(queue_panel_, &QueuePanel::item_clicked, this, [](int idx) {
        on_queue_item_clicked(idx);
    });

    // Tab buttons functionality
    connect(lyrics_tab_btn_, &QPushButton::clicked, this, [this]() {
        lyrics_tab_btn_->setChecked(true);
        queue_tab_btn_->setChecked(false);
        tabs_stack_->setCurrentIndex(0);
        updateButtonsStyle();
    });

    connect(queue_tab_btn_, &QPushButton::clicked, this, [this]() {
        lyrics_tab_btn_->setChecked(false);
        queue_tab_btn_->setChecked(true);
        tabs_stack_->setCurrentIndex(1);
        updateButtonsStyle();
    });
}

void NowPlayingView::updateButtonsStyle() {
    const auto &c = DesignTokens::current();

    // Setup tab styles
    QString active_tab_style = QString(
        "QPushButton {\n"
        "    background: rgba(%1, %2, %3, 0.12);\n"
        "    border: 1px solid rgba(%1, %2, %3, 0.2);\n"
        "    border-radius: 16px;\n"
        "    color: %4;\n"
        "    padding: 6px 16px;\n"
        "}"
    ).arg(c.accent.red()).arg(c.accent.green()).arg(c.accent.blue()).arg(c.accent.name());

    QString inactive_tab_style = QString(
        "QPushButton {\n"
        "    background: transparent;\n"
        "    border: 1px solid transparent;\n"
        "    border-radius: 16px;\n"
        "    color: %1;\n"
        "    padding: 6px 16px;\n"
        "}\n"
        "QPushButton:hover {\n"
        "    background: rgba(255, 255, 255, 0.05);\n"
        "}"
    ).arg(c.text_secondary.name());

    lyrics_tab_btn_->setStyleSheet(lyrics_tab_btn_->isChecked() ? active_tab_style : inactive_tab_style);
    queue_tab_btn_->setStyleSheet(queue_tab_btn_->isChecked() ? active_tab_style : inactive_tab_style);

    // Setup Shuffle style
    shuffle_btn_->setIcon(IconProvider::getIcon("shuffle", shuffle_on_ ? c.accent : c.text_secondary, 20));

    // Setup Repeat style
    if (repeat_mode_ == 0) {
        repeat_btn_->setIcon(IconProvider::getIcon("repeat", c.text_secondary, 20));
        repeat_btn_->setToolTip("Repetir: Desactivado");
    } else if (repeat_mode_ == 1) {
        repeat_btn_->setIcon(IconProvider::getIcon("repeat", c.accent, 20));
        repeat_btn_->setToolTip("Repetir: Todas");
    } else {
        repeat_btn_->setIcon(IconProvider::getIcon("repeat_one", c.accent, 20));
        repeat_btn_->setToolTip("Repetir: Una");
    }
}

void NowPlayingView::showView() {
    if (!parentWidget()) return;
    
    // Position at bottom, then slide up
    int parent_w = parentWidget()->width();
    int parent_h = parentWidget()->height();
    
    setGeometry(0, parent_h, parent_w, parent_h);
    show();
    raise();

    // Slide up animation
    auto *anim = new QPropertyAnimation(this, "pos", this);
    anim->setDuration(350);
    anim->setEasingCurve(QEasingCurve::OutExpo);
    anim->setStartValue(QPoint(0, parent_h));
    anim->setEndValue(QPoint(0, 0));
    anim->start(QAbstractAnimation::DeleteWhenStopped);

    nebula_bg_->setPlaying(is_playing_);
    waveform_bars_->setPlaying(is_playing_);
}

void NowPlayingView::hideView() {
    if (!parentWidget()) return;
    
    int parent_h = parentWidget()->height();

    // Slide down animation
    auto *anim = new QPropertyAnimation(this, "pos", this);
    anim->setDuration(250);
    anim->setEasingCurve(QEasingCurve::InCubic);
    anim->setStartValue(pos());
    anim->setEndValue(QPoint(0, parent_h));
    
    connect(anim, &QPropertyAnimation::finished, this, [this]() {
        hide();
        emit close_clicked();
    });
    
    anim->start(QAbstractAnimation::DeleteWhenStopped);
}

void NowPlayingView::setTrackInfo(const std::string &title, const std::string &artist, const std::string &thumbnail) {
    title_label_->setText(QString::fromStdString(title));
    artist_label_->setText(QString::fromStdString(artist));
    
    vinyl_disc_->setArtwork(QString::fromStdString(thumbnail));
}

void NowPlayingView::setPlaybackState(int32_t, int32_t position_ms, int32_t duration_ms) {
    if (duration_ms > 0) {
        progress_bar_->blockSignals(true);
        progress_bar_->setRange(0, duration_ms);
        progress_bar_->setValue(position_ms);
        progress_bar_->blockSignals(false);
    }
    lyrics_widget_->updatePosition(position_ms);


    auto fmt = [](int32_t ms) {
        int m = ms / 60000;
        int s = (ms % 60000) / 1000;
        return QString("%1:%2").arg(m).arg(s, 2, 10, QChar('0'));
    };
    time_label_->setText(fmt(position_ms) + " / " + fmt(duration_ms));
}

void NowPlayingView::setPlaying(bool playing) {
    is_playing_ = playing;
    play_btn_->setIcon(IconProvider::getIcon(playing ? "pause" : "play_arrow", QColor("#FFFFFF"), 32));
    
    vinyl_disc_->setPlaying(playing);
    nebula_bg_->setPlaying(playing);
    waveform_bars_->setPlaying(playing);
}

void NowPlayingView::setShuffle(bool on) {
    shuffle_on_ = on;
    updateButtonsStyle();
}

void NowPlayingView::setRepeatMode(int mode) {
    repeat_mode_ = mode;
    updateButtonsStyle();
}

void NowPlayingView::setDominantColors(const QStringList &colors) {
    nebula_bg_->setColors(colors);
}

void NowPlayingView::setLyrics(const QString &plain, const QString &synced) {
    lyrics_widget_->setLyrics(plain, synced);
}

void NowPlayingView::setQueue(const QStringList &titles, const QStringList &artists,
                              const QStringList &thumbnails, int current_index)
{
    queue_panel_->setQueue(titles, artists, thumbnails, current_index);
}

void NowPlayingView::resizeEvent(QResizeEvent *event) {
    QWidget::resizeEvent(event);
    nebula_bg_->setGeometry(rect());
}

void NowPlayingView::setSubtitleAlignment(const std::string &alignment) {
    if (lyrics_widget_) {
        lyrics_widget_->setSubtitleAlignment(alignment);
    }
}

void NowPlayingView::setSubtitleFontSize(int32_t size) {
    if (lyrics_widget_) {
        lyrics_widget_->setSubtitleFontSize(size);
    }
}

void NowPlayingView::setSubtitleLineSpacing(double spacing) {
    if (lyrics_widget_) {
        lyrics_widget_->setSubtitleLineSpacing(spacing);
    }
}

void NowPlayingView::setSubtitleAutoScroll(bool enabled) {
    if (lyrics_widget_) {
        lyrics_widget_->setSubtitleAutoScroll(enabled);
    }
}

void NowPlayingView::setSubtitleGlowEffect(bool enabled) {
    if (lyrics_widget_) {
        lyrics_widget_->setSubtitleGlowEffect(enabled);
    }
}

