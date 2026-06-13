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

    // Binary search for current line
    int idx = -1;
    for (int i = 0; i < lines_.size(); ++i) {
        if (lines_[i].time_ms <= position_ms) {
            idx = i;
        } else {
            break;
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

    if (lines_.isEmpty()) {
        auto *lbl = new QLabel("Instrumental / No se encontraron letras", container_);
        lbl->setAlignment(Qt::AlignCenter);
        lbl->setFont(DesignTokens::getFont("heading_sm", 15));
        lbl->setStyleSheet(QString("color: %1;").arg(c.text_muted.name()));
        layout_->addWidget(lbl);
    } else {
        for (int i = 0; i < lines_.size(); ++i) {
            auto &line = lines_[i];
            
            // Clickable label
            line.label = new LyricLabel(line.text, line.time_ms, container_);
            line.label->setFontSize(15);
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
}

void LyricsWidget::highlightLine(int index) {
    if (index < 0 || index >= lines_.size()) return;
    if (active_index_ == index) return;

    const auto &c = DesignTokens::current();

    // 1. Reset old active line
    if (active_index_ >= 0 && active_index_ < lines_.size()) {
        auto &old_line = lines_[active_index_];
        if (old_line.label) {
            auto *f_anim = new QPropertyAnimation(old_line.label, "fontSize", this);
            f_anim->setDuration(200);
            f_anim->setStartValue(old_line.label->fontSize());
            f_anim->setEndValue(15);
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

    // 3. Highlight new active line
    auto &new_line = lines_[active_index_];
    if (new_line.label) {
        auto *f_anim = new QPropertyAnimation(new_line.label, "fontSize", this);
        f_anim->setDuration(250);
        f_anim->setEasingCurve(QEasingCurve::OutBack);
        f_anim->setStartValue(new_line.label->fontSize());
        f_anim->setEndValue(20);
        f_anim->start(QAbstractAnimation::DeleteWhenStopped);

        if (new_line.opacity_effect) {
            auto *o_anim = new QPropertyAnimation(new_line.opacity_effect, "opacity", this);
            o_anim->setDuration(250);
            o_anim->setStartValue(new_line.opacity_effect->opacity());
            o_anim->setEndValue(1.0);
            o_anim->start(QAbstractAnimation::DeleteWhenStopped);
        }
        
        // Highlight active lyric line using accent color or vibrant white
        new_line.label->setStyleSheet(QString("color: %1; font-weight: bold;").arg(c.text_primary.name()));

        // 4. Center scroll view on new active label
        int target_y = new_line.label->geometry().center().y() - viewport()->height() / 2;
        smoothScrollTo(target_y);
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
