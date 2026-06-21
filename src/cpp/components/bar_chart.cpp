#include "bar_chart.h"
#include "../design_tokens.h"
#include <QPainter>
#include <QPainterPath>
#include <QMouseEvent>
#include <cmath>

const QString BarChart::DAYS[] = {"Lun", "Mar", "Mié", "Jue", "Vie", "Sáb", "Dom"};

BarChart::BarChart(QWidget *parent)
    : QWidget(parent)
{
    setMinimumHeight(200);
    setMouseTracking(true);
    
    // Default mock values
    values_ = {0, 0, 0, 0, 0, 0, 0};
    
    grow_anim_ = new QVariantAnimation(this);
    grow_anim_->setDuration(DesignTokens::duration(800));
    grow_anim_->setEasingCurve(QEasingCurve::OutCubic);
    grow_anim_->setStartValue(0.0);
    grow_anim_->setEndValue(1.0);
    
    connect(grow_anim_, &QVariantAnimation::valueChanged, this, [this](const QVariant &val) {
        setAnimationProgress(val.toReal());
    });
}

void BarChart::setData(const QVector<int> &values) {
    values_ = values;
    if (values_.size() < 7) {
        values_.resize(7);
    }
    
    grow_anim_->stop();
    if (DesignTokens::reducedMotion()) {
        setAnimationProgress(1.0);
        return;
    }
    grow_anim_->start();
}

void BarChart::setAnimationProgress(qreal progress) {
    animation_progress_ = progress;
    update();
}

void BarChart::showEvent(QShowEvent *event) {
    QWidget::showEvent(event);
    grow_anim_->stop();
    if (DesignTokens::reducedMotion()) {
        setAnimationProgress(1.0);
        return;
    }
    grow_anim_->start();
}

void BarChart::mouseMoveEvent(QMouseEvent *event) {
    mouse_pos_ = event->pos();
    
    int left_margin = 40;
    int right_margin = 20;
    int chart_w = width() - left_margin - right_margin;
    int col_w = chart_w / 7;
    
    int new_hover = -1;
    for (int i = 0; i < 7; ++i) {
        int x_start = left_margin + i * col_w;
        int x_end = x_start + col_w;
        if (event->pos().x() >= x_start && event->pos().x() <= x_end) {
            new_hover = i;
            break;
        }
    }
    
    if (hovered_index_ != new_hover) {
        hovered_index_ = new_hover;
        update();
    }
}

void BarChart::leaveEvent(QEvent *event) {
    QWidget::leaveEvent(event);
    hovered_index_ = -1;
    update();
}

void BarChart::paintEvent(QPaintEvent *) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    const auto &c = DesignTokens::current();
    int w = width();
    int h = height();

    int left_margin = 40;
    int right_margin = 20;
    int bottom_margin = 30;
    int top_margin = 20;
    
    int chart_w = w - left_margin - right_margin;
    int chart_h = h - bottom_margin - top_margin;

    // Calculate maximum value for scaling
    int max_val = 1;
    for (int v : values_) {
        if (v > max_val) max_val = v;
    }
    // Round max_val to a nice round number for grid lines
    if (max_val < 5) max_val = 5;
    else if (max_val < 10) max_val = 10;
    else max_val = ((max_val + 9) / 10) * 10;

    // 1. Draw horizontal grid lines and labels
    painter.setFont(DesignTokens::getFont("caption", 10));
    painter.setPen(QPen(c.border, 1, Qt::DashLine));
    
    int grid_steps = 4;
    for (int i = 0; i <= grid_steps; ++i) {
        int y = top_margin + chart_h - (i * chart_h / grid_steps);
        int grid_val = i * max_val / grid_steps;
        
        // Grid line
        painter.drawLine(left_margin, y, w - right_margin, y);
        
        // Grid label
        painter.setPen(c.text_muted);
        painter.drawText(QRect(0, y - 8, left_margin - 8, 16), Qt::AlignRight | Qt::AlignVCenter, QString::number(grid_val));
        painter.setPen(QPen(c.border, 1, Qt::DashLine));
    }

    // 2. Draw bars
    int col_w = chart_w / 7;
    int bar_w = qBound(12, col_w / 2, 40);

    for (int i = 0; i < 7; ++i) {
        int val = values_[i];
        int bar_h = static_cast<int>((static_cast<double>(val) / max_val) * chart_h * animation_progress_);
        
        int cx = left_margin + i * col_w + col_w / 2;
        int rx = cx - bar_w / 2;
        int ry = top_margin + chart_h - bar_h;

        // Draw day label
        painter.setPen(c.text_secondary);
        painter.setFont(DesignTokens::getFont("caption", 10));
        painter.drawText(QRect(cx - col_w / 2, h - bottom_margin + 6, col_w, 20), Qt::AlignCenter, DAYS[i]);

        if (bar_h <= 0) continue;

        // Draw bar
        painter.save();
        QLinearGradient grad(rx, ry + bar_h, rx, ry);
        
        if (i == hovered_index_) {
            grad.setColorAt(0.0, c.accent_dim.lighter(120));
            grad.setColorAt(1.0, c.accent_bright);
        } else {
            grad.setColorAt(0.0, c.accent_dim);
            grad.setColorAt(1.0, c.accent);
        }
        
        painter.setBrush(grad);
        painter.setPen(Qt::NoPen);
        
        QPainterPath path;
        // Rounded corners at the top only
        path.addRoundedRect(QRect(rx, ry, bar_w, bar_h), qMin(6, bar_w / 2), qMin(6, bar_w / 2));
        painter.drawPath(path);
        
        painter.restore();
    }

    // 3. Draw tooltip overlay for hovered bar
    if (hovered_index_ != -1 && hovered_index_ < values_.size()) {
        int val = values_[hovered_index_];
        int bar_h = static_cast<int>((static_cast<double>(val) / max_val) * chart_h * animation_progress_);
        int cx = left_margin + hovered_index_ * col_w + col_w / 2;
        int ry = top_margin + chart_h - bar_h;

        QString tooltip_text = QString("%1 reproduc.").arg(val);
        painter.setFont(DesignTokens::getFont("caption", 9));
        
        int text_w = painter.fontMetrics().horizontalAdvance(tooltip_text) + 16;
        int text_h = 24;
        
        int tx = cx - text_w / 2;
        int ty = ry - text_h - 8;

        // Clamp tooltip inside widget boundaries
        if (tx < 0) tx = 4;
        if (tx + text_w > w) tx = w - text_w - 4;
        if (ty < 0) ty = ry + 8; // draw below if no space above

        // Draw tooltip card
        painter.setPen(QPen(c.border_accent, 1));
        painter.setBrush(c.bg_elevated);
        QPainterPath t_path;
        t_path.addRoundedRect(QRect(tx, ty, text_w, text_h), 6, 6);
        painter.drawPath(t_path);

        // Draw text
        painter.setPen(c.text_primary);
        painter.drawText(QRect(tx, ty, text_w, text_h), Qt::AlignCenter, tooltip_text);
    }
}
