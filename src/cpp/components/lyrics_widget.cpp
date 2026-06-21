#include "lyrics_widget.h"
#include "../design_tokens.h"
#include "../icon_provider.h"
#include "doremi/src/bridge.rs.h"
#include <QScrollBar>
#include <QRegularExpression>
#include <QMouseEvent>
#include <QEvent>

LyricLabel::LyricLabel(const QString &text, int time_ms, QWidget *parent)
    : QLabel(text, parent), time_ms_(time_ms), font_size_(15)
{
    setAlignment(Qt::AlignCenter);
    setWordWrap(true);
    setCursor(Qt::PointingHandCursor);
    setFont(DesignTokens::getFont("body", font_size_));
}

void LyricLabel::setFontSize(int size) {
    font_size_ = size;
    setFont(DesignTokens::getFont("body", font_size_));
}

void LyricLabel::mousePressEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton) {
        emit clicked(time_ms_);
    }
    QLabel::mousePressEvent(event);
}

void LyricLabel::enterEvent(QEnterEvent *event) {
    is_hovered_ = true;
    if (auto *effect = qobject_cast<QGraphicsOpacityEffect*>(graphicsEffect())) {
        if (effect->opacity() < 0.9) {
            if (DesignTokens::reducedMotion()) {
                effect->setOpacity(0.85);
                return;
            }
            auto *anim = new QPropertyAnimation(effect, "opacity", this);
            anim->setDuration(DesignTokens::duration(120));
            anim->setStartValue(effect->opacity());
            anim->setEndValue(0.85);
            anim->start(QAbstractAnimation::DeleteWhenStopped);
        }
    }
    QLabel::enterEvent(event);
}

void LyricLabel::leaveEvent(QEvent *event) {
    is_hovered_ = false;
    if (auto *effect = qobject_cast<QGraphicsOpacityEffect*>(graphicsEffect())) {
        // If it's highlighted (opacity close to 1.0), don't fade out
        if (effect->opacity() < 0.95 && font_size_ < 18) {
            if (DesignTokens::reducedMotion()) {
                effect->setOpacity(0.4);
                return;
            }
            auto *anim = new QPropertyAnimation(effect, "opacity", this);
            anim->setDuration(DesignTokens::duration(150));
            anim->setStartValue(effect->opacity());
            anim->setEndValue(0.4);
            anim->start(QAbstractAnimation::DeleteWhenStopped);
        }
    }
    QLabel::leaveEvent(event);
}

// ─────────────────────────────────────────────────────────────────────────────

LyricsWidget::LyricsWidget(QWidget *parent)
    : QWidget(parent), scroll_animation_(nullptr)
{
    const auto &c = DesignTokens::current();

    auto *main_layout = new QVBoxLayout(this);
    main_layout->setContentsMargins(0, 0, 0, 0);
    main_layout->setSpacing(0);

    // Setup QScrollArea internally
    scroll_area_ = new QScrollArea(this);
    scroll_area_->setWidgetResizable(true);
    scroll_area_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scroll_area_->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scroll_area_->setFrameShape(QFrame::NoFrame);
    scroll_area_->setStyleSheet("background: transparent;");

    container_ = new QWidget(scroll_area_);
    container_->setStyleSheet("background: transparent;");
    
    layout_ = new QVBoxLayout(container_);
    layout_->setContentsMargins(24, 0, 24, 0);
    layout_->setSpacing(24);
    layout_->setAlignment(Qt::AlignCenter);

    top_spacer_ = nullptr;
    bottom_spacer_ = nullptr;

    scroll_area_->setWidget(container_);
    main_layout->addWidget(scroll_area_, 1);

    // Sync Control Bar setup (hidden by default, only shown for synced lyrics)
    sync_control_bar_ = new QWidget(this);
    sync_control_bar_->setObjectName("syncControlBar");
    sync_control_bar_->setFixedHeight(40);
    
    // Set style: modern elevated capsule floating above bottom
    QString barStyle = QString(
        "QWidget#syncControlBar {"
        "    background-color: %1;"
        "    border: 1px solid %2;"
        "    border-radius: 20px;"
        "}"
    ).arg(DesignTokens::rgba(c.bg_elevated)).arg(c.border.name());
    sync_control_bar_->setStyleSheet(barStyle);

    auto *bar_layout = new QHBoxLayout(sync_control_bar_);
    bar_layout->setContentsMargins(16, 0, 16, 0);
    bar_layout->setSpacing(12);

    // Timing icon
    auto *icon_lbl = IconProvider::createIconLabel("schedule", 16, c.text_secondary, true, sync_control_bar_);
    bar_layout->addWidget(icon_lbl);

    // Synchronization offset label
    sync_offset_lbl_ = new QLabel(sync_control_bar_);
    sync_offset_lbl_->setFont(DesignTokens::getFont("body", 11));
    sync_offset_lbl_->setStyleSheet(QString("color: %1; border: none; background: transparent;").arg(c.text_secondary.name()));
    bar_layout->addWidget(sync_offset_lbl_);

    // Button style sheet
    QString btnStyle = QString(
        "QPushButton {"
        "    background: transparent;"
        "    border: none;"
        "    color: %1;"
        "    padding: 4px;"
        "    border-radius: 12px;"
        "    min-width: 24px;"
        "    min-height: 24px;"
        "}"
        "QPushButton:hover {"
        "    background-color: %2;"
        "}"
        "QPushButton:pressed {"
        "    background-color: %3;"
        "}"
    ).arg(c.text_primary.name()).arg(DesignTokens::rgba(c.bg_overlay)).arg(DesignTokens::rgba(c.border));

    // Minus button (-0.5s)
    sync_minus_btn_ = new QPushButton(sync_control_bar_);
    sync_minus_btn_->setIcon(IconProvider::getIcon("remove", c.text_primary, 14));
    sync_minus_btn_->setStyleSheet(btnStyle);
    sync_minus_btn_->setCursor(Qt::PointingHandCursor);
    sync_minus_btn_->setToolTip(QString::fromStdString(std::string(doremi_tr("sync_delay_tooltip_minus"))));
    bar_layout->addWidget(sync_minus_btn_);

    // Plus button (+0.5s)
    sync_plus_btn_ = new QPushButton(sync_control_bar_);
    sync_plus_btn_->setIcon(IconProvider::getIcon("add", c.text_primary, 14));
    sync_plus_btn_->setStyleSheet(btnStyle);
    sync_plus_btn_->setCursor(Qt::PointingHandCursor);
    sync_plus_btn_->setToolTip(QString::fromStdString(std::string(doremi_tr("sync_delay_tooltip_plus"))));
    bar_layout->addWidget(sync_plus_btn_);

    // Reset button
    sync_reset_btn_ = new QPushButton(sync_control_bar_);
    sync_reset_btn_->setIcon(IconProvider::getIcon("restart_alt", c.text_secondary, 14));
    sync_reset_btn_->setStyleSheet(btnStyle);
    sync_reset_btn_->setCursor(Qt::PointingHandCursor);
    sync_reset_btn_->setToolTip(QString::fromStdString(std::string(doremi_tr("sync_delay_tooltip_reset"))));
    bar_layout->addWidget(sync_reset_btn_);

    main_layout->addWidget(sync_control_bar_, 0, Qt::AlignCenter);
    
    // Add small bottom margin to bar layout
    main_layout->setContentsMargins(0, 0, 0, 12);

    updateSyncLabel();
    sync_control_bar_->hide();

    // Connect signals
    connect(sync_minus_btn_, &QPushButton::clicked, this, &LyricsWidget::onSyncMinusClicked);
    connect(sync_plus_btn_, &QPushButton::clicked, this, &LyricsWidget::onSyncPlusClicked);
    connect(sync_reset_btn_, &QPushButton::clicked, this, &LyricsWidget::onSyncResetClicked);
}

void LyricsWidget::setLyrics(const QString &plain, const QString &synced) {
    if (scroll_animation_) scroll_animation_->stop();
    active_index_ = -1;
    manual_delay_ms_ = 0;
    last_position_ms_ = 0;
    updateSyncLabel();
    clearLayout();

    if (!synced.isEmpty()) {
        parseLrc(synced);
    } else if (!plain.isEmpty()) {
        lines_.clear();
        has_synced_lyrics_ = false;
        QStringList list = plain.split('\n');
        for (const QString &l : list) {
            QString trimmed = l.trimmed();
            lines_.push_back({0, trimmed, nullptr, nullptr});
        }
    } else {
        lines_.clear();
        has_synced_lyrics_ = false;
    }

    buildLyricsLayout();
    updateSyncControlBarVisibility();
    scroll_area_->verticalScrollBar()->setValue(0);
}

void LyricsWidget::updatePosition(int position_ms) {
    last_position_ms_ = position_ms;
    if (!has_synced_lyrics_ || lines_.isEmpty()) return;

    // Apply manual synchronization delay
    int adjusted_pos = position_ms - manual_delay_ms_;

    // Binary search for last line with time_ms <= adjusted_pos
    int lo = 0, hi = lines_.size() - 1;
    int idx = -1;
    while (lo <= hi) {
        int mid = lo + (hi - lo) / 2;
        if (lines_[mid].time_ms <= adjusted_pos) {
            idx = mid;
            lo = mid + 1;
        } else {
            hi = mid - 1;
        }
    }

    if (idx != -1) {
        highlightLine(idx);
    }
}

void LyricsWidget::parseLrc(const QString &lrc_text) {
    lines_.clear();
    has_synced_lyrics_ = false;
    int file_offset_ms = 0;

    QRegularExpression offset_rx(R"(\[offset:\s*([+-]?\d+)\])", QRegularExpression::CaseInsensitiveOption);
    QRegularExpression time_rx(R"(\[(\d+):(\d+)(?:\.(\d+))?\])");

    QStringList raw_lines = lrc_text.split('\n');

    // First pass: extract file offset
    for (const QString &line : raw_lines) {
        QString trimmed = line.trimmed();
        auto offset_match = offset_rx.match(trimmed);
        if (offset_match.hasMatch()) {
            file_offset_ms = offset_match.captured(1).toInt();
            break;
        }
    }

    // Second pass: parse timestamps and text
    for (const QString &line : raw_lines) {
        QString trimmed = line.trimmed();
        if (trimmed.isEmpty()) continue;

        // Skip metadata headers
        if (trimmed.startsWith("[ar:") || trimmed.startsWith("[ti:") || 
            trimmed.startsWith("[al:") || trimmed.startsWith("[by:") || 
            trimmed.startsWith("[length:") || trimmed.startsWith("[offset:")) {
            continue;
        }

        // Find all time tags in this line (handles multiple timestamps per line)
        auto matches = time_rx.globalMatch(trimmed);
        QVector<int> times;
        int last_match_end = 0;

        while (matches.hasNext()) {
            auto match = matches.next();
            int mins = match.captured(1).toInt();
            int secs = match.captured(2).toInt();
            int ms = match.captured(3).isEmpty() ? 0 : match.captured(3).toInt();
            if (match.captured(3).length() == 2) {
                ms *= 10;
            } else if (match.captured(3).length() == 1) {
                ms *= 100;
            }
            int total_ms = mins * 60000 + secs * 1000 + ms;
            times.push_back(total_ms);
            last_match_end = match.capturedEnd();
        }

        if (!times.isEmpty()) {
            has_synced_lyrics_ = true;
            QString text = trimmed.mid(last_match_end).trimmed();
            
            for (int time_ms : times) {
                int adjusted_time = qMax(0, time_ms + file_offset_ms);
                lines_.push_back({adjusted_time, text, nullptr, nullptr});
            }
        }
    }

    std::sort(lines_.begin(), lines_.end(), [](const LyricLine &a, const LyricLine &b) {
        return a.time_ms < b.time_ms;
    });
}

void LyricsWidget::clearLayout() {
    QLayoutItem *item;
    while ((item = layout_->takeAt(0)) != nullptr) {
        if (item->widget()) {
            item->widget()->deleteLater();
        }
        delete item;
    }
    top_spacer_ = nullptr;
    bottom_spacer_ = nullptr;
}

void LyricsWidget::buildLyricsLayout() {
    const auto &c = DesignTokens::current();

    int mid_h = scroll_area_->viewport()->height() / 2;

    // Top spacer
    top_spacer_ = new QWidget(container_);
    top_spacer_->setFixedHeight(mid_h);
    top_spacer_->setStyleSheet("background: transparent;");
    layout_->addWidget(top_spacer_);

    Qt::Alignment flag = Qt::AlignCenter;
    if (alignment_ == "left") flag = Qt::AlignLeft | Qt::AlignVCenter;
    else if (alignment_ == "right") flag = Qt::AlignRight | Qt::AlignVCenter;
    layout_->setAlignment(flag);
    layout_->setSpacing(static_cast<int>(16 * line_spacing_multiplier_));

    if (lines_.isEmpty()) {
        auto *lbl = new QLabel("Instrumental / No se encontraron letras", container_);
        lbl->setAlignment(Qt::AlignCenter);
        lbl->setFont(DesignTokens::getFont("heading_sm", base_font_size_));
        lbl->setStyleSheet(QString("color: %1;").arg(c.text_muted.name()));
        layout_->addWidget(lbl);
    } else {
        for (int i = 0; i < lines_.size(); ++i) {
            auto &line = lines_[i];
            
            line.label = new LyricLabel(line.text, line.time_ms, container_);
            line.label->setFontSize(base_font_size_);
            line.label->setAlignment(flag);
            line.label->setStyleSheet(QString("color: %1;").arg(c.text_secondary.name()));
            
            if (has_synced_lyrics_) {
                line.opacity_effect = new QGraphicsOpacityEffect(line.label);
                line.opacity_effect->setOpacity(0.4);
                line.label->setGraphicsEffect(line.opacity_effect);
                
                connect(line.label, &LyricLabel::clicked, this, &LyricsWidget::seek_requested);
            } else {
                line.opacity_effect = new QGraphicsOpacityEffect(line.label);
                line.opacity_effect->setOpacity(0.95);
                line.label->setGraphicsEffect(line.opacity_effect);
            }
            
            layout_->addWidget(line.label);
        }
    }

    // Bottom spacer
    bottom_spacer_ = new QWidget(container_);
    bottom_spacer_->setFixedHeight(mid_h);
    bottom_spacer_->setStyleSheet("background: transparent;");
    layout_->addWidget(bottom_spacer_);

    // Sync time caption
    time_caption_ = new QLabel(container_);
    time_caption_->setFont(DesignTokens::getFont("caption", 10));
    time_caption_->setStyleSheet(QString("color: %1;").arg(c.text_muted.name()));
    time_caption_->setAlignment(Qt::AlignCenter);
    time_caption_->setFixedHeight(20);
    time_caption_->hide();
    layout_->addWidget(time_caption_);
}

void LyricsWidget::highlightLine(int index) {
    if (index < 0 || index >= lines_.size()) return;
    if (active_index_ == index) return;

    const auto &c = DesignTokens::current();

    // 1. Reset old active line
    if (active_index_ >= 0 && active_index_ < lines_.size()) {
        auto &old_line = lines_[active_index_];
        if (old_line.label) {
            if (DesignTokens::reducedMotion()) {
                old_line.label->setFontSize(base_font_size_);
            } else {
                auto *f_anim = new QPropertyAnimation(old_line.label, "fontSize", this);
                f_anim->setDuration(DesignTokens::duration(200));
                f_anim->setStartValue(old_line.label->fontSize());
                f_anim->setEndValue(base_font_size_);
                f_anim->start(QAbstractAnimation::DeleteWhenStopped);
            }

            if (old_line.opacity_effect) {
                if (DesignTokens::reducedMotion()) {
                    old_line.opacity_effect->setOpacity(0.4);
                } else {
                    auto *o_anim = new QPropertyAnimation(old_line.opacity_effect, "opacity", this);
                    o_anim->setDuration(DesignTokens::duration(200));
                    o_anim->setStartValue(old_line.opacity_effect->opacity());
                    o_anim->setEndValue(0.4);
                    o_anim->start(QAbstractAnimation::DeleteWhenStopped);
                }
            }
            old_line.label->setStyleSheet(QString("color: %1;").arg(c.text_secondary.name()));
        }
    }

    // 2. Set new active index
    active_index_ = index;

    // 3. Highlight new active line
    auto &new_line = lines_[active_index_];
    if (new_line.label) {
        if (DesignTokens::reducedMotion()) {
            new_line.label->setFontSize(base_font_size_ + 4);
        } else {
            auto *f_anim = new QPropertyAnimation(new_line.label, "fontSize", this);
            f_anim->setDuration(DesignTokens::duration(250));
            f_anim->setEasingCurve(QEasingCurve::OutBack);
            f_anim->setStartValue(new_line.label->fontSize());
            f_anim->setEndValue(base_font_size_ + 4);
            f_anim->start(QAbstractAnimation::DeleteWhenStopped);
        }

        if (new_line.opacity_effect) {
            if (DesignTokens::reducedMotion()) {
                new_line.opacity_effect->setOpacity(1.0);
            } else {
                auto *o_anim = new QPropertyAnimation(new_line.opacity_effect, "opacity", this);
                o_anim->setDuration(DesignTokens::duration(250));
                o_anim->setStartValue(new_line.opacity_effect->opacity());
                o_anim->setEndValue(1.0);
                o_anim->start(QAbstractAnimation::DeleteWhenStopped);
            }
        }
        
        if (glow_effect_enabled_) {
            new_line.label->setStyleSheet(QString("color: %1; font-weight: bold;").arg(c.accent.name()));
        } else {
            new_line.label->setStyleSheet(QString("color: %1; font-weight: bold;").arg(c.text_primary.name()));
        }

        // 4. Update sync time caption
        if (has_synced_lyrics_ && time_caption_) {
            int ms = new_line.time_ms;
            int mins = ms / 60000;
            int secs = (ms % 60000) / 1000;
            int cent = (ms % 1000) / 10;
            time_caption_->setText(QString("[%1:%2.%3]").arg(mins).arg(secs, 2, 10, QChar('0')).arg(cent, 2, 10, QChar('0')));
            time_caption_->show();
        }

        // 5. Center scroll view on new active label
        if (auto_scroll_enabled_) {
            int target_y = new_line.label->geometry().center().y() - scroll_area_->viewport()->height() / 2;
            smoothScrollTo(target_y);
        }
    }
}

void LyricsWidget::smoothScrollTo(int y_pos) {
    if (scroll_animation_) {
        scroll_animation_->stop();
    } else {
        scroll_animation_ = new QPropertyAnimation(scroll_area_->verticalScrollBar(), "value", this);
    }
    if (DesignTokens::reducedMotion()) {
        scroll_area_->verticalScrollBar()->setValue(y_pos);
        return;
    }
    scroll_animation_->setDuration(DesignTokens::duration(350));
    scroll_animation_->setEasingCurve(QEasingCurve::InOutSine);
    scroll_animation_->setStartValue(scroll_area_->verticalScrollBar()->value());
    scroll_animation_->setEndValue(y_pos);
    scroll_animation_->start();
}

void LyricsWidget::resizeEvent(QResizeEvent *event) {
    QWidget::resizeEvent(event);
    int mid_h = scroll_area_->viewport()->height() / 2;
    if (top_spacer_) top_spacer_->setFixedHeight(mid_h);
    if (bottom_spacer_) bottom_spacer_->setFixedHeight(mid_h);
}

void LyricsWidget::setSubtitleAlignment(const std::string &alignment) {
    alignment_ = alignment;
    Qt::Alignment flag = Qt::AlignCenter;
    if (alignment == "left") flag = Qt::AlignLeft | Qt::AlignVCenter;
    else if (alignment == "right") flag = Qt::AlignRight | Qt::AlignVCenter;
    
    layout_->blockSignals(true);
    layout_->setAlignment(flag);
    layout_->blockSignals(false);
    
    for (auto &line : lines_) {
        if (line.label) {
            line.label->setAlignment(flag);
        }
    }
}

void LyricsWidget::setSubtitleFontSize(int size) {
    base_font_size_ = size;
    for (int i = 0; i < lines_.size(); ++i) {
        auto &line = lines_[i];
        if (line.label) {
            int target_size = (i == active_index_) ? (base_font_size_ + 4) : base_font_size_;
            line.label->setFontSize(target_size);
        }
    }
}

void LyricsWidget::setSubtitleLineSpacing(double spacing) {
    line_spacing_multiplier_ = spacing;
    layout_->setSpacing(static_cast<int>(16 * line_spacing_multiplier_));
}

void LyricsWidget::setSubtitleAutoScroll(bool enabled) {
    auto_scroll_enabled_ = enabled;
    if (enabled && active_index_ >= 0 && active_index_ < lines_.size()) {
        auto &line = lines_[active_index_];
        if (line.label) {
            int target_y = line.label->geometry().center().y() - scroll_area_->viewport()->height() / 2;
            smoothScrollTo(target_y);
        }
    }
}

void LyricsWidget::setSubtitleGlowEffect(bool enabled) {
    glow_effect_enabled_ = enabled;
    if (active_index_ >= 0 && active_index_ < lines_.size()) {
        const auto &c = DesignTokens::current();
        auto &line = lines_[active_index_];
        if (line.label) {
            if (glow_effect_enabled_) {
                line.label->setStyleSheet(QString("color: %1; font-weight: bold;").arg(c.accent.name()));
            } else {
                line.label->setStyleSheet(QString("color: %1; font-weight: bold;").arg(c.text_primary.name()));
            }
        }
    }
}

void LyricsWidget::onSyncMinusClicked() {
    manual_delay_ms_ -= 500;
    updateSyncLabel();
    updatePosition(last_position_ms_);
}

void LyricsWidget::onSyncPlusClicked() {
    manual_delay_ms_ += 500;
    updateSyncLabel();
    updatePosition(last_position_ms_);
}

void LyricsWidget::onSyncResetClicked() {
    manual_delay_ms_ = 0;
    updateSyncLabel();
    updatePosition(last_position_ms_);
}

void LyricsWidget::updateSyncLabel() {
    if (!sync_offset_lbl_) return;
    double secs = manual_delay_ms_ / 1000.0;
    QString sign = "";
    if (secs > 0) sign = "+";
    sync_offset_lbl_->setText(QString("%1: %2%3s")
        .arg(QString::fromStdString(std::string(doremi_tr("sync_offset"))))
        .arg(sign)
        .arg(QString::number(secs, 'f', 1)));
}

void LyricsWidget::updateSyncControlBarVisibility() {
    if (sync_control_bar_) {
        sync_control_bar_->setVisible(has_synced_lyrics_);
    }
}
