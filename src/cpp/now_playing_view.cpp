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
    // Setup backgrounds (creation order determines z-order)
    artwork_backdrop_ = new ArtworkBackdrop(this);
    nebula_bg_ = new NebulaBg(this);

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
    close_btn->setObjectName("closeBtn");
    close_btn->setFixedSize(40, 40);
    close_btn->setCursor(Qt::PointingHandCursor);
    close_btn->setIcon(IconProvider::getIcon("expand_more", c.text_primary, 24));
    close_btn->setStyleSheet(QString("QPushButton { background: rgba(255,255,255,0.06); border: none; border-radius: %1px; }"
                             "QPushButton:hover { background: rgba(255,255,255,0.12); }").arg(DesignTokens::radius().xl));
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

    title_label_ = new QLabel(tr_q("no_playback"), this);
    title_label_->setFont(DesignTokens::getFont("heading_lg"));
    title_label_->setStyleSheet(QString("color: %1; font-weight: bold;").arg(c.text_primary.name()));
    title_label_->setAlignment(Qt::AlignCenter);

    artist_label_ = new QLabel(tr_q("no_track"), this);
    artist_label_->setFont(DesignTokens::getFont("body_sm"));
    artist_label_->setStyleSheet(QString("color: %1;").arg(c.text_secondary.name()));
    artist_label_->setAlignment(Qt::AlignCenter);

    meta_layout->addWidget(title_label_);
    meta_layout->addWidget(artist_label_);
    left_column->addLayout(meta_layout);

    // Action buttons: like + download
    auto *actions_layout = new QHBoxLayout();
    actions_layout->setSpacing(8);
    actions_layout->setAlignment(Qt::AlignCenter);

    like_btn_ = new QPushButton(this);
    like_btn_->setFixedSize(32, 32);
    like_btn_->setCursor(Qt::PointingHandCursor);
    like_btn_->setIcon(IconProvider::getIcon("favorite_border", c.text_secondary, 18));
    like_btn_->setToolTip(tr_q("add_favorite"));
    like_btn_->setStyleSheet(DesignTokens::iconButtonStyle(16));
    connect(like_btn_, &QPushButton::clicked, this, [this]() {
        std::string track_id = static_cast<std::string>(current_track_.id);
        if (track_id.empty()) return;
        bool is_fav = get_track_favorite_state(track_id);
        if (is_fav) {
            on_remove_favorite(track_id);
        } else {
            on_add_favorite(current_track_);
        }
        updateLikeButtonState(!is_fav);
    });
    actions_layout->addWidget(like_btn_);

    download_btn_ = new QPushButton(this);
    download_btn_->setFixedSize(32, 32);
    download_btn_->setCursor(Qt::PointingHandCursor);
    download_btn_->setIcon(IconProvider::getIcon("download", c.text_secondary, 18));
    download_btn_->setToolTip(tr_q("download"));
    download_btn_->setStyleSheet(DesignTokens::iconButtonStyle(16));
    connect(download_btn_, &QPushButton::clicked, this, [this]() {
        emit download_clicked(current_track_);
    });
    actions_layout->addWidget(download_btn_);

    left_column->addLayout(actions_layout);

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
    time_label_->setFont(DesignTokens::getFont("caption_sm"));
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
    shuffle_btn_->setStyleSheet(DesignTokens::iconButtonStyle());

    prev_btn_ = new QPushButton(this);
    prev_btn_->setFixedSize(40, 40);
    prev_btn_->setCursor(Qt::PointingHandCursor);
    prev_btn_->setIcon(IconProvider::getIcon("skip_previous", c.text_primary, 24));
    prev_btn_->setStyleSheet(DesignTokens::iconButtonStyle());

    play_btn_ = new QPushButton(this);
    play_btn_->setFixedSize(56, 56);
    play_btn_->setCursor(Qt::PointingHandCursor);
    play_btn_->setIcon(IconProvider::getIcon("play_arrow", c.text_on_accent, 32));
    play_btn_->setStyleSheet(DesignTokens::primaryButtonStyle(28));

    next_btn_ = new QPushButton(this);
    next_btn_->setFixedSize(40, 40);
    next_btn_->setCursor(Qt::PointingHandCursor);
    next_btn_->setIcon(IconProvider::getIcon("skip_next", c.text_primary, 24));
    next_btn_->setStyleSheet(DesignTokens::iconButtonStyle());

    repeat_btn_ = new QPushButton(this);
    repeat_btn_->setFixedSize(36, 36);
    repeat_btn_->setCursor(Qt::PointingHandCursor);
    repeat_btn_->setIcon(IconProvider::getIcon("repeat", c.text_secondary, 20));
    repeat_btn_->setStyleSheet(DesignTokens::iconButtonStyle());

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

    lyrics_tab_btn_ = new QPushButton(tr_q("lyrics"), this);
    lyrics_tab_btn_->setFont(DesignTokens::getFont("heading_sm", 13));
    lyrics_tab_btn_->setCursor(Qt::PointingHandCursor);
    lyrics_tab_btn_->setCheckable(true);
    lyrics_tab_btn_->setChecked(true);
    
    queue_tab_btn_ = new QPushButton(tr_q("queue"), this);
    queue_tab_btn_->setFont(DesignTokens::getFont("heading_sm", 13));
    queue_tab_btn_->setCursor(Qt::PointingHandCursor);
    queue_tab_btn_->setCheckable(true);

    related_tab_btn_ = new QPushButton(tr_q("related"), this);
    related_tab_btn_->setFont(DesignTokens::getFont("heading_sm", 13));
    related_tab_btn_->setCursor(Qt::PointingHandCursor);
    related_tab_btn_->setCheckable(true);

    tabs_bar->addWidget(lyrics_tab_btn_);
    tabs_bar->addWidget(queue_tab_btn_);
    tabs_bar->addWidget(related_tab_btn_);
    right_column->addLayout(tabs_bar);

    // Stacked widget containing lyrics and queue
    tabs_stack_ = new QStackedWidget(this);
    tabs_stack_->setStyleSheet("background: transparent;");

    lyrics_widget_ = new LyricsWidget(this);
    queue_panel_ = new QueuePanel(this);
    related_widget_ = new RelatedTracksWidget(this);

    tabs_stack_->addWidget(lyrics_widget_);
    tabs_stack_->addWidget(queue_panel_);
    tabs_stack_->addWidget(related_widget_);

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
    connect(queue_panel_, &QueuePanel::item_removed, this, [](int idx) {
        on_queue_item_removed(idx);
    });
    connect(queue_panel_, &QueuePanel::item_moved, this, [](int from, int to) {
        on_queue_item_moved(from, to);
    });
    connect(queue_panel_, &QueuePanel::clear_requested, this, []() {
        on_queue_clear_requested();
    });

    // Tab buttons functionality
    auto activate_tab = [this](int index, QPushButton *active) {
        lyrics_tab_btn_->setChecked(active == lyrics_tab_btn_);
        queue_tab_btn_->setChecked(active == queue_tab_btn_);
        related_tab_btn_->setChecked(active == related_tab_btn_);
        tabs_stack_->setCurrentIndex(index);
        updateButtonsStyle();
    };

    connect(lyrics_tab_btn_, &QPushButton::clicked, this, [this, activate_tab]() {
        activate_tab(0, lyrics_tab_btn_);
    });

    connect(queue_tab_btn_, &QPushButton::clicked, this, [this, activate_tab]() {
        activate_tab(1, queue_tab_btn_);
    });

    connect(related_tab_btn_, &QPushButton::clicked, this, [this, activate_tab]() {
        activate_tab(2, related_tab_btn_);
    });

    // Related tracks widget connections
    connect(related_widget_, &RelatedTracksWidget::play_requested, this, &NowPlayingView::related_play_requested);
    connect(related_widget_, &RelatedTracksWidget::add_to_queue_requested, this, &NowPlayingView::related_add_to_queue_requested);
}

void NowPlayingView::updateButtonsStyle() {
    const auto &c = DesignTokens::current();

    // Setup tab styles
    QString active_tab_style = QString(
        "QPushButton {\n"
        "    background: rgba(%1, %2, %3, 0.12);\n"
        "    border: 1px solid rgba(%1, %2, %3, 0.2);\n"
        "    border-radius: %5px;\n"
        "    color: %4;\n"
        "    padding: 6px 16px;\n"
        "}"
    ).arg(c.accent.red()).arg(c.accent.green()).arg(c.accent.blue()).arg(c.accent.name()).arg(DesignTokens::radius().xl);

    QString inactive_tab_style = QString(
        "QPushButton {\n"
        "    background: transparent;\n"
        "    border: 1px solid transparent;\n"
        "    border-radius: %2px;\n"
        "    color: %1;\n"
        "    padding: 6px 16px;\n"
        "}\n"
        "QPushButton:hover {\n"
        "    background: rgba(255, 255, 255, 0.05);\n"
        "}"
    ).arg(c.text_secondary.name()).arg(DesignTokens::radius().xl);

    lyrics_tab_btn_->setStyleSheet(lyrics_tab_btn_->isChecked() ? active_tab_style : inactive_tab_style);
    queue_tab_btn_->setStyleSheet(queue_tab_btn_->isChecked() ? active_tab_style : inactive_tab_style);
    related_tab_btn_->setStyleSheet(related_tab_btn_->isChecked() ? active_tab_style : inactive_tab_style);

    // Setup Shuffle style
    shuffle_btn_->setIcon(IconProvider::getIcon("shuffle", shuffle_on_ ? c.accent : c.text_secondary, 20));

    // Setup Repeat style
    if (repeat_mode_ == 0) {
        repeat_btn_->setIcon(IconProvider::getIcon("repeat", c.text_secondary, 20));
        repeat_btn_->setToolTip(tr_q("repeat_disabled_tooltip"));
    } else if (repeat_mode_ == 1) {
        repeat_btn_->setIcon(IconProvider::getIcon("repeat", c.accent, 20));
        repeat_btn_->setToolTip(tr_q("repeat_all_tooltip"));
    } else {
        repeat_btn_->setIcon(IconProvider::getIcon("repeat_one", c.accent, 20));
        repeat_btn_->setToolTip(tr_q("repeat_one_tooltip"));
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

    if (DesignTokens::reducedMotion()) {
        move(0, 0);
        nebula_bg_->setPlaying(is_playing_);
        waveform_bars_->setPlaying(is_playing_);
        return;
    }

    // Slide up animation
    auto *anim = new QPropertyAnimation(this, "pos", this);
    anim->setDuration(DesignTokens::duration(350));
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

    if (DesignTokens::reducedMotion()) {
        move(0, parent_h);
        hide();
        emit close_clicked();
        return;
    }

    // Slide down animation
    auto *anim = new QPropertyAnimation(this, "pos", this);
    anim->setDuration(DesignTokens::duration(250));
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

    if (!thumbnail.empty()) {
        artwork_backdrop_->setImage(QString::fromStdString(thumbnail));
    } else {
        artwork_backdrop_->clear();
    }
}

void NowPlayingView::setCurrentTrack(const Track &track) {
    current_track_ = track;
    bool is_fav = get_track_favorite_state(static_cast<std::string>(track.id));
    updateLikeButtonState(is_fav);
}

void NowPlayingView::updateLikeButtonState(bool is_favorite) {
    const auto &c = DesignTokens::current();
    if (is_favorite) {
        like_btn_->setIcon(IconProvider::getIcon("favorite", c.accent, 18));
        like_btn_->setToolTip(tr_q("remove_favorite"));
    } else {
        like_btn_->setIcon(IconProvider::getIcon("favorite_border", c.text_secondary, 18));
        like_btn_->setToolTip(tr_q("add_favorite"));
    }
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
    const auto &c = DesignTokens::current();
    play_btn_->setIcon(IconProvider::getIcon(playing ? "pause" : "play_arrow", c.text_on_accent, 32));
    
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

void NowPlayingView::setQueue(const std::vector<Track> &tracks, int current_index)
{
    queue_panel_->setQueue(tracks, current_index);
}

void NowPlayingView::setRelatedTracks(const std::vector<Track> &tracks)
{
    related_widget_->setTracks(tracks);
}

void NowPlayingView::resizeEvent(QResizeEvent *event) {
    QWidget::resizeEvent(event);
    artwork_backdrop_->setGeometry(rect());
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

void NowPlayingView::update_theme() {
    const auto &c = DesignTokens::current();
    title_label_->setStyleSheet(QString("color: %1; font-weight: bold;").arg(c.text_primary.name()));
    artist_label_->setStyleSheet(QString("color: %1;").arg(c.text_secondary.name()));
    time_label_->setStyleSheet(QString("color: %1;").arg(c.text_muted.name()));

    if (auto *close_btn = findChild<QPushButton*>("closeBtn")) {
        close_btn->setStyleSheet(QString(
            "QPushButton { background: rgba(255,255,255,0.06); border: none; border-radius: %1px; }"
            "QPushButton:hover { background: rgba(255,255,255,0.12); }"
        ).arg(DesignTokens::radius().xl));
        close_btn->setIcon(IconProvider::getIcon("expand_more", c.text_primary, 24));
    }

    prev_btn_->setIcon(IconProvider::getIcon("skip_previous", c.text_primary, 24));
    next_btn_->setIcon(IconProvider::getIcon("skip_next", c.text_primary, 24));

    play_btn_->setStyleSheet(DesignTokens::primaryButtonStyle(28));

    updateButtonsStyle();
}
