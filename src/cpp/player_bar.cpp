#include "player_bar.h"
#include "design_tokens.h"
#include "icon_provider.h"
#include <QPainter>
#include <QPainterPath>
#include <QDebug>
#include <QMouseEvent>


static QPixmap getRoundedPixmap(const QPixmap &src, int radius) {
    if (src.isNull()) return src;
    QPixmap dest(src.size());
    dest.fill(Qt::transparent);
    QPainter painter(&dest);
    painter.setRenderHint(QPainter::Antialiasing);
    QPainterPath path;
    path.addRoundedRect(src.rect(), radius, radius);
    painter.setClipPath(path);
    painter.drawPixmap(0, 0, src);
    return dest;
}

PlayerBar::PlayerBar(QWidget *parent)
    : QWidget(parent)
{
    const auto &c = DesignTokens::current();

    auto *main_layout = new QHBoxLayout(this);
    main_layout->setContentsMargins(16, 8, 16, 8);
    main_layout->setSpacing(16);

    // ── LEFT: Track Info & Artwork ─────────────────────────────────────────
    auto *left_container = new QWidget(this);
    auto *left_layout = new QHBoxLayout(left_container);
    left_layout->setContentsMargins(0, 0, 0, 0);
    left_layout->setSpacing(12);

    artwork_label_ = new QLabel(this);
    artwork_label_->setFixedSize(44, 44);
    artwork_label_->setAlignment(Qt::AlignCenter);
    artwork_label_->setStyleSheet(QString("background-color: %1; border-radius: 6px;")
        .arg(c.bg_elevated.name()));
    
    QPixmap default_art = IconProvider::getIcon("music_note", c.text_secondary, 22).pixmap(44, 44);
    artwork_label_->setPixmap(getRoundedPixmap(default_art, 6));

    track_label_ = new QLabel(this);
    track_label_->setFont(DesignTokens::getFont("body", 13));
    track_label_->setText("<b>Sin reproducción</b><br><font color=\"" + c.text_muted.name() + "\">Ningún track seleccionado</font>");
    track_label_->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    track_label_->setStyleSheet("background: transparent;");

    artwork_label_->setAttribute(Qt::WA_TransparentForMouseEvents);
    track_label_->setAttribute(Qt::WA_TransparentForMouseEvents);


    left_layout->addWidget(artwork_label_);
    left_layout->addWidget(track_label_);
    left_layout->addStretch();
    left_container->setLayout(left_layout);
    main_layout->addWidget(left_container, 1);

    // ── CENTER: Controls & Progress ─────────────────────────────────────────
    auto *center_container = new QWidget(this);
    auto *center_layout = new QVBoxLayout(center_container);
    center_layout->setContentsMargins(0, 0, 0, 0);
    center_layout->setSpacing(6);

    // Control Buttons Row
    auto *controls_widget = new QWidget(this);
    auto *controls_layout = new QHBoxLayout(controls_widget);
    controls_layout->setContentsMargins(0, 0, 0, 0);
    controls_layout->setSpacing(16);
    controls_layout->addStretch();

    // Shuffle Button
    shuffle_btn_ = new QPushButton(this);
    shuffle_btn_->setFixedSize(32, 32);
    shuffle_btn_->setCheckable(true);
    shuffle_btn_->setCursor(Qt::PointingHandCursor);
    shuffle_btn_->setIcon(IconProvider::getIcon("shuffle", c.text_secondary, 20));
    shuffle_btn_->setToolTip("Aleatorio");
    shuffle_btn_->setStyleSheet("QPushButton { background: transparent; border: none; }");

    // Previous Button
    prev_btn_ = new QPushButton(this);
    prev_btn_->setFixedSize(32, 32);
    prev_btn_->setCursor(Qt::PointingHandCursor);
    prev_btn_->setIcon(IconProvider::getIcon("skip_previous", c.text_primary, 22));
    prev_btn_->setStyleSheet("QPushButton { background: transparent; border: none; }");

    // Play/Pause Button (Solid Rounded Circle)
    play_btn_ = new QPushButton(this);
    play_btn_->setFixedSize(40, 40);
    play_btn_->setCursor(Qt::PointingHandCursor);
    play_btn_->setIcon(IconProvider::getIcon("play_arrow", QColor("#FFFFFF"), 24));
    
    QString play_style = QString(
        "QPushButton {\n"
        "    background-color: %1;\n"
        "    border: none;\n"
        "    border-radius: 20px;\n"
        "}\n"
        "QPushButton:hover {\n"
        "    background-color: %2;\n"
        "}\n"
        "QPushButton:pressed {\n"
        "    background-color: %3;\n"
        "}\n"
    )
    .arg(c.accent.name())
    .arg(c.accent_bright.name())
    .arg(c.accent.darker(115).name());
    play_btn_->setStyleSheet(play_style);

    // Next Button
    next_btn_ = new QPushButton(this);
    next_btn_->setFixedSize(32, 32);
    next_btn_->setCursor(Qt::PointingHandCursor);
    next_btn_->setIcon(IconProvider::getIcon("skip_next", c.text_primary, 22));
    next_btn_->setStyleSheet("QPushButton { background: transparent; border: none; }");

    // Repeat Button
    repeat_btn_ = new QPushButton(this);
    repeat_btn_->setFixedSize(32, 32);
    repeat_btn_->setCursor(Qt::PointingHandCursor);
    repeat_btn_->setIcon(IconProvider::getIcon("repeat", c.text_secondary, 20));
    repeat_btn_->setToolTip("Repetir: Desactivado");
    repeat_btn_->setStyleSheet("QPushButton { background: transparent; border: none; }");

    controls_layout->addWidget(shuffle_btn_);
    controls_layout->addWidget(prev_btn_);
    controls_layout->addWidget(play_btn_);
    controls_layout->addWidget(next_btn_);
    controls_layout->addWidget(repeat_btn_);
    controls_layout->addStretch();
    controls_widget->setLayout(controls_layout);
    center_layout->addWidget(controls_widget);

    // Progress Bar Row
    auto *progress_widget = new QWidget(this);
    auto *progress_layout = new QHBoxLayout(progress_widget);
    progress_layout->setContentsMargins(0, 0, 0, 0);
    progress_layout->setSpacing(8);

    progress_ = new QSlider(Qt::Horizontal, this);
    progress_->setRange(0, 0);
    progress_->setCursor(Qt::PointingHandCursor);
    
    QString progress_style = QString(
        "QSlider::groove:horizontal {\n"
        "    height: 4px;\n"
        "    background: rgba(255, 255, 255, 0.1);\n"
        "    border-radius: 2px;\n"
        "}\n"
        "QSlider::sub-page:horizontal {\n"
        "    background: %1;\n"
        "    border-radius: 2px;\n"
        "}\n"
        "QSlider::handle:horizontal {\n"
        "    background: %2;\n"
        "    width: 10px;\n"
        "    height: 10px;\n"
        "    margin: -3px 0;\n"
        "    border-radius: 5px;\n"
        "}\n"
        "QSlider::handle:horizontal:hover {\n"
        "    background: %3;\n"
        "}\n"
    )
    .arg(c.accent.name())
    .arg(c.accent_bright.name())
    .arg(c.accent_bright.name());
    
    progress_->setStyleSheet(progress_style);

    time_label_ = new QLabel("0:00 / 0:00", this);
    time_label_->setFont(DesignTokens::getFont("caption", 11));
    time_label_->setStyleSheet(QString("color: %1;").arg(c.text_muted.name()));

    progress_layout->addWidget(progress_, 1);
    progress_layout->addWidget(time_label_);
    progress_widget->setLayout(progress_layout);
    center_layout->addWidget(progress_widget);

    center_container->setLayout(center_layout);
    main_layout->addWidget(center_container, 2);

    // ── RIGHT: Volume Controls ──────────────────────────────────────────────
    auto *right_container = new QWidget(this);
    auto *right_layout = new QHBoxLayout(right_container);
    right_layout->setContentsMargins(0, 0, 0, 0);
    right_layout->setSpacing(8);
    right_layout->addStretch();

    QLabel *volume_icon = IconProvider::createIconLabel("volume_up", 18, c.text_secondary, true, this);
    
    volume_slider_ = new QSlider(Qt::Horizontal, this);
    volume_slider_->setRange(0, 100);
    volume_slider_->setValue(75);
    volume_slider_->setFixedWidth(80);
    volume_slider_->setCursor(Qt::PointingHandCursor);
    
    QString volume_style = QString(
        "QSlider::groove:horizontal {\n"
        "    height: 3px;\n"
        "    background: rgba(255, 255, 255, 0.1);\n"
        "    border-radius: 1px;\n"
        "}\n"
        "QSlider::sub-page:horizontal {\n"
        "    background: %1;\n"
        "    border-radius: 1px;\n"
        "}\n"
        "QSlider::handle:horizontal {\n"
        "    background: %2;\n"
        "    width: 8px;\n"
        "    height: 8px;\n"
        "    margin: -3px 0;\n"
        "    border-radius: 4px;\n"
        "}\n"
    )
    .arg(c.text_secondary.name())
    .arg(c.text_primary.name());
    
    volume_slider_->setStyleSheet(volume_style);

    right_layout->addWidget(volume_icon);
    right_layout->addWidget(volume_slider_);
    right_container->setLayout(right_layout);
    main_layout->addWidget(right_container, 1);

    setLayout(main_layout);
    setFixedHeight(64);

    // Set panel background
    setAttribute(Qt::WA_StyledBackground, true);
    setStyleSheet(QString("PlayerBar { background-color: %1; border-top: 1px solid %2; }")
        .arg(c.bg_surface.name())
        .arg(QString("rgba(%1, %2, %3, %4)").arg(c.border.red()).arg(c.border.green()).arg(c.border.blue()).arg(c.border.alpha() / 255.0))
    );

    // Connections
    connect(play_btn_, &QPushButton::clicked, this, &PlayerBar::play_pause_clicked);
    connect(next_btn_, &QPushButton::clicked, this, &PlayerBar::next_clicked);
    connect(prev_btn_, &QPushButton::clicked, this, &PlayerBar::previous_clicked);
    connect(progress_, &QSlider::sliderMoved, this, &PlayerBar::seek_requested);
    connect(volume_slider_, &QSlider::valueChanged, this, &PlayerBar::volume_set);
    
    connect(shuffle_btn_, &QPushButton::toggled, this, [this](bool on) {
        const auto &c = DesignTokens::current();
        shuffle_on_ = on;
        shuffle_btn_->setIcon(IconProvider::getIcon("shuffle", on ? c.accent : c.text_secondary, 20));
        emit shuffle_toggled(on);
    });

    connect(repeat_btn_, &QPushButton::clicked, this, [this]() {
        repeat_mode_ = (repeat_mode_ + 1) % 3;
        set_repeat_mode(repeat_mode_);
        emit repeat_cycled();
    });
}

void PlayerBar::set_track_info(const std::string &title, const std::string &artist,
                                const std::string &thumbnail) {
    const auto &c = DesignTokens::current();
    
    QString title_str = QString::fromStdString(title);
    QString artist_str = QString::fromStdString(artist);
    
    // Format track info using HTML for two lines of text
    track_label_->setText(QString("<b>%1</b><br><font color=\"%2\">%3</font>")
        .arg(title_str)
        .arg(c.text_secondary.name())
        .arg(artist_str)
    );

    // Load artwork
    QPixmap art_pix;
    if (!thumbnail.empty() && art_pix.load(QString::fromStdString(thumbnail))) {
        artwork_label_->setPixmap(getRoundedPixmap(
            art_pix.scaled(44, 44, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation), 6
        ));
    } else {
        QPixmap default_art = IconProvider::getIcon("music_note", c.text_secondary, 22).pixmap(44, 44);
        artwork_label_->setPixmap(getRoundedPixmap(default_art, 6));
    }
}

void PlayerBar::set_progress(int32_t pos_ms, int32_t dur_ms) {
    if (dur_ms > 0) {
        progress_->blockSignals(true);
        progress_->setRange(0, dur_ms);
        progress_->setValue(pos_ms);
        progress_->blockSignals(false);
        
        auto fmt = [](int32_t ms) {
            int m = ms / 60000;
            int s = (ms % 60000) / 1000;
            return QString("%1:%2").arg(m).arg(s, 2, 10, QChar('0'));
        };
        time_label_->setText(fmt(pos_ms) + " / " + fmt(dur_ms));
    }
}

void PlayerBar::set_playing(bool playing) {
    play_btn_->setIcon(IconProvider::getIcon(
        playing ? "pause" : "play_arrow",
        QColor("#FFFFFF"),
        24
    ));
}

void PlayerBar::set_volume_value(int32_t volume) {
    volume_slider_->blockSignals(true);
    volume_slider_->setValue(volume);
    volume_slider_->blockSignals(false);
}

void PlayerBar::set_shuffle(bool on) {
    const auto &c = DesignTokens::current();
    shuffle_on_ = on;
    shuffle_btn_->blockSignals(true);
    shuffle_btn_->setChecked(on);
    shuffle_btn_->setIcon(IconProvider::getIcon("shuffle", on ? c.accent : c.text_secondary, 20));
    shuffle_btn_->blockSignals(false);
}

void PlayerBar::set_repeat_mode(int mode) {
    const auto &c = DesignTokens::current();
    repeat_mode_ = mode;
    
    if (mode == 0) {
        repeat_btn_->setIcon(IconProvider::getIcon("repeat", c.text_secondary, 20));
        repeat_btn_->setToolTip("Repetir: Desactivado");
    } else if (mode == 1) {
        repeat_btn_->setIcon(IconProvider::getIcon("repeat", c.accent, 20));
        repeat_btn_->setToolTip("Repetir: Todas");
    } else {
        repeat_btn_->setIcon(IconProvider::getIcon("repeat_one", c.accent, 20));
        repeat_btn_->setToolTip("Repetir: Una");
    }
}

void PlayerBar::mousePressEvent(QMouseEvent *event) {
    QWidget::mousePressEvent(event);
    if (event->button() == Qt::LeftButton && event->pos().x() < 280) {
        emit left_section_clicked();
    }
}

void PlayerBar::update_theme() {
    const auto &c = DesignTokens::current();
    
    artwork_label_->setStyleSheet(QString("background-color: %1; border-radius: 6px;")
        .arg(c.bg_elevated.name()));
    
    QString play_style = QString(
        "QPushButton {\n"
        "    background-color: %1;\n"
        "    border: none;\n"
        "    border-radius: 20px;\n"
        "}\n"
        "QPushButton:hover {\n"
        "    background-color: %2;\n"
        "}\n"
        "QPushButton:pressed {\n"
        "    background-color: %3;\n"
        "}\n"
    )
    .arg(c.accent.name())
    .arg(c.accent_bright.name())
    .arg(c.accent.darker(115).name());
    play_btn_->setStyleSheet(play_style);
    
    QString progress_style = QString(
        "QSlider::groove:horizontal {\n"
        "    height: 4px;\n"
        "    background: rgba(255, 255, 255, 0.1);\n"
        "    border-radius: 2px;\n"
        "}\n"
        "QSlider::sub-page:horizontal {\n"
        "    background: %1;\n"
        "    border-radius: 2px;\n"
        "}\n"
        "QSlider::handle:horizontal {\n"
        "    background: %2;\n"
        "    width: 10px;\n"
        "    height: 10px;\n"
        "    margin: -3px 0;\n"
        "    border-radius: 5px;\n"
        "}\n"
        "QSlider::handle:horizontal:hover {\n"
        "    background: %3;\n"
        "}\n"
    )
    .arg(c.accent.name())
    .arg(c.accent_bright.name())
    .arg(c.accent_bright.name());
    progress_->setStyleSheet(progress_style);
    
    time_label_->setStyleSheet(QString("color: %1;").arg(c.text_muted.name()));
    
    shuffle_btn_->setIcon(IconProvider::getIcon("shuffle", shuffle_on_ ? c.accent : c.text_secondary, 20));
    prev_btn_->setIcon(IconProvider::getIcon("skip_previous", c.text_primary, 22));
    next_btn_->setIcon(IconProvider::getIcon("skip_next", c.text_primary, 22));
    
    QColor rep_color = repeat_mode_ > 0 ? c.accent : c.text_secondary;
    repeat_btn_->setIcon(IconProvider::getIcon(repeat_mode_ == 2 ? "repeat_one" : "repeat", rep_color, 20));
    
    QString volume_style = QString(
        "QSlider::groove:horizontal {\n"
        "    height: 4px;\n"
        "    background: rgba(255, 255, 255, 0.1);\n"
        "    border-radius: 2px;\n"
        "}\n"
        "QSlider::sub-page:horizontal {\n"
        "    background: %1;\n"
        "    border-radius: 2px;\n"
        "}\n"
        "QSlider::handle:horizontal {\n"
        "    background: %2;\n"
        "    width: 8px;\n"
        "    height: 8px;\n"
        "    margin: -2px 0;\n"
        "    border-radius: 4px;\n"
        "}\n"
    ).arg(c.accent.name()).arg(c.accent_bright.name());
    volume_slider_->setStyleSheet(volume_style);
}

