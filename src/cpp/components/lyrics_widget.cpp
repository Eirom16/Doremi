#include "lyrics_widget.h"
#include "../design_tokens.h"
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
    QLabel::mousePressEvent(event);
    if (event->button() == Qt::LeftButton) {
        emit clicked(time_ms_);
    }
}

void LyricLabel::enterEvent(QEnterEvent *event) {
    QLabel::enterEvent(event);
    is_hovered_ = true;
    if (auto *effect = qobject_cast<QGraphicsOpacityEffect*>(graphicsEffect())) {
        if (effect->opacity() < 0.9) {
            auto *anim = new QPropertyAnimation(effect, "opacity", this);
            anim->setDuration(120);
            anim->setStartValue(effect->opacity());
            anim->setEndValue(0.85);
            anim->start(QAbstractAnimation::DeleteWhenStopped);
        }
    }
}

void LyricLabel::leaveEvent(QEvent *event) {
    QLabel::leaveEvent(event);
    is_hovered_ = false;
    if (auto *effect = qobject_cast<QGraphicsOpacityEffect*>(graphicsEffect())) {
        // If it's highlighted (opacity close to 1.0), don't fade out
        if (effect->opacity() < 0.95 && font_size_ < 18) {
            auto *anim = new QPropertyAnimation(effect, "opacity", this);
            anim->setDuration(150);
            anim->setStartValue(effect->opacity());
            anim->setEndValue(0.4);
            anim->start(QAbstractAnimation::DeleteWhenStopped);
        }
    }
}

// ─────────────────────────────────────────────────────────────────────────────

LyricsWidget::LyricsWidget(QWidget *parent)
    : QScrollArea(parent), scroll_animation_(nullptr)
{
    setWidgetResizable(true);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setFrameShape(QFrame::NoFrame);
    setStyleSheet("background: transparent;");

    container_ = new QWidget(this);
    container_->setStyleSheet("background: transparent;");
    
    layout_ = new QVBoxLayout(container_);
    layout_->setContentsMargins(24, 0, 24, 0);
    layout_->setSpacing(24);
    layout_->setAlignment(Qt::AlignCenter);

    top_spacer_ = nullptr;
    bottom_spacer_ = nullptr;

    setWidget(container_);
}

void LyricsWidget::setLyrics(const QString &plain, const QString &synced) {
    if (scroll_animation_) scroll_animation_->stop();
    active_index_ = -1;
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
    verticalScrollBar()->setValue(0);
}

void LyricsWidget::updatePosition(int position_ms) {
    if (!has_synced_lyrics_ || lines_.isEmpty()) return;

    // Binary search for last line with time_ms <= position_ms
    int lo = 0, hi = lines_.size() - 1;
    int idx = -1;
    while (lo <= hi) {
        int mid = lo + (hi - lo) / 2;
        if (lines_[mid].time_ms <= position_ms) {
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

    // Matches [mm:ss.xx] Lyric text
    QRegularExpression rx(R"(\[(\d+):(\d+)(?:\.(\d+))?\](.*))");
    QStringList raw_lines = lrc_text.split('\n');
    for (const QString &line : raw_lines) {
        QString trimmed = line.trimmed();
        if (trimmed.isEmpty()) continue;

        auto match = rx.match(trimmed);
        if (match.hasMatch()) {
            has_synced_lyrics_ = true;
            int mins = match.captured(1).toInt();
            int secs = match.captured(2).toInt();
            int ms = match.captured(3).isEmpty() ? 0 : match.captured(3).toInt();
            if (match.captured(3).length() == 2) {
                ms *= 10;
            } else if (match.captured(3).length() == 1) {
                ms *= 100;
            }
            int total_ms = mins * 60000 + secs * 1000 + ms;
            QString text = match.captured(4).trimmed();
            
            // Filter metadata tags like [offset:0] or [ar:Artist]
            if (text.startsWith("ar:") || text.startsWith("ti:") || text.startsWith("al:") || text.startsWith("by:")) {
                continue;
            }
            lines_.push_back({total_ms, text, nullptr, nullptr});
        }
    }

    std::sort(lines_.begin(), lines_.end(), [](const LyricLine &a, const LyricLine &b) {
        return a.time_ms < b.time_ms;
    });
}

void LyricsWidget::clearLayout() {
    // Delete existing widgets in layout
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

    int mid_h = viewport()->height() / 2;

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
            
            // Clickable label
            line.label = new LyricLabel(line.text, line.time_ms, container_);
            line.label->setFontSize(base_font_size_);
            line.label->setAlignment(flag);
            line.label->setStyleSheet(QString("color: %1;").arg(c.text_secondary.name()));
            
            if (has_synced_lyrics_) {
                // Apply opacity effect for fading in/out
                line.opacity_effect = new QGraphicsOpacityEffect(line.label);
                line.opacity_effect->setOpacity(0.4);
                line.label->setGraphicsEffect(line.opacity_effect);
                
                connect(line.label, &LyricLabel::clicked, this, &LyricsWidget::seek_requested);
            } else {
                // For plain lyrics, keep it fully visible and static
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

    // Sync time caption (hidden by default)
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

    // 1. Reset old active line and its neighbors
    if (active_index_ >= 0 && active_index_ < lines_.size()) {
        auto &old_line = lines_[active_index_];
        if (old_line.label) {
            auto *f_anim = new QPropertyAnimation(old_line.label, "fontSize", this);
            f_anim->setDuration(200);
            f_anim->setStartValue(old_line.label->fontSize());
            f_anim->setEndValue(base_font_size_);
            f_anim->start(QAbstractAnimation::DeleteWhenStopped);

            if (old_line.opacity_effect) {
                auto *o_anim = new QPropertyAnimation(old_line.opacity_effect, "opacity", this);
                o_anim->setDuration(200);
                o_anim->setStartValue(old_line.opacity_effect->opacity());
                o_anim->setEndValue(0.4);
                o_anim->start(QAbstractAnimation::DeleteWhenStopped);
            }
            old_line.label->setStyleSheet(QString("color: %1;").arg(c.text_secondary.name()));
        }
    }

    // 2. Set new active index
    active_index_ = index;

    const int stagger_range = 2;
    // 3. Stagger animation for surrounding lines
    for (int offset = -stagger_range; offset <= stagger_range; ++offset) {
        if (offset == 0) continue;
        int idx = index + offset;
        if (idx < 0 || idx >= lines_.size()) continue;
        auto &line = lines_[idx];
        if (!line.label || !line.opacity_effect) continue;

        float target_opacity = 0.4 + (stagger_range - abs(offset) + 1) * 0.15f;
        if (target_opacity > 0.95f) target_opacity = 0.95f;

        auto *o_anim = new QPropertyAnimation(line.opacity_effect, "opacity", this);
        o_anim->setDuration(200);
        o_anim->setStartValue(line.opacity_effect->opacity());
        o_anim->setEndValue(target_opacity);
        o_anim->setEasingCurve(QEasingCurve::OutCubic);
        int delay = (abs(offset) - 1) * 40;
        if (delay > 0) o_anim->setLoopCount(1);
        o_anim->start(QAbstractAnimation::DeleteWhenStopped);
    }

    // 4. Highlight new active line
    auto &new_line = lines_[active_index_];
    if (new_line.label) {
        auto *f_anim = new QPropertyAnimation(new_line.label, "fontSize", this);
        f_anim->setDuration(250);
        f_anim->setEasingCurve(QEasingCurve::OutBack);
        f_anim->setStartValue(new_line.label->fontSize());
        f_anim->setEndValue(base_font_size_ + 4);
        f_anim->start(QAbstractAnimation::DeleteWhenStopped);

        if (new_line.opacity_effect) {
            auto *o_anim = new QPropertyAnimation(new_line.opacity_effect, "opacity", this);
            o_anim->setDuration(250);
            o_anim->setStartValue(new_line.opacity_effect->opacity());
            o_anim->setEndValue(1.0);
            o_anim->start(QAbstractAnimation::DeleteWhenStopped);
        }
        
        // Highlight active lyric line using accent color or vibrant white
        if (glow_effect_enabled_) {
            new_line.label->setStyleSheet(QString("color: %1; font-weight: bold;").arg(c.accent.name()));
        } else {
            new_line.label->setStyleSheet(QString("color: %1; font-weight: bold;").arg(c.text_primary.name()));
        }

        // 5. Update sync time caption
        if (has_synced_lyrics_ && time_caption_) {
            int ms = new_line.time_ms;
            int mins = ms / 60000;
            int secs = (ms % 60000) / 1000;
            int cent = (ms % 1000) / 10;
            time_caption_->setText(QString("[%1:%2.%3]").arg(mins).arg(secs, 2, 10, QChar('0')).arg(cent, 2, 10, QChar('0')));
            time_caption_->show();
        }

        // 6. Center scroll view on new active label
        if (auto_scroll_enabled_) {
            int target_y = new_line.label->geometry().center().y() - viewport()->height() / 2;
            smoothScrollTo(target_y);
        }
    }
}

void LyricsWidget::smoothScrollTo(int y_pos) {
    if (scroll_animation_) {
        scroll_animation_->stop();
    } else {
        scroll_animation_ = new QPropertyAnimation(verticalScrollBar(), "value", this);
    }
    scroll_animation_->setDuration(350);
    scroll_animation_->setEasingCurve(QEasingCurve::InOutSine);
    scroll_animation_->setStartValue(verticalScrollBar()->value());
    scroll_animation_->setEndValue(y_pos);
    scroll_animation_->start();
}

void LyricsWidget::resizeEvent(QResizeEvent *event) {
    QScrollArea::resizeEvent(event);
    int mid_h = viewport()->height() / 2;
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
            int target_y = line.label->geometry().center().y() - viewport()->height() / 2;
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

