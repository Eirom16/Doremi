#include "theme_transition.h"
#include "design_tokens.h"
#include <QPainter>
#include <QVariant>
#include <QEasingCurve>

ThemeTransitionOverlay::ThemeTransitionOverlay(QWidget *parent)
    : QWidget(parent), progress_anim_(nullptr), fade_anim_(nullptr), midpoint_fired_(false)
{
    setAttribute(Qt::WA_StyledBackground, true);
    setAttribute(Qt::WA_NoSystemBackground, true);
    
    opacity_effect_ = new QGraphicsOpacityEffect(this);
    setGraphicsEffect(opacity_effect_);
    opacity_effect_->setOpacity(1.0);
    
    auto *main_layout = new QVBoxLayout(this);
    main_layout->setAlignment(Qt::AlignCenter);
    main_layout->setContentsMargins(0, 0, 0, 0);
    
    card_ = new QFrame(this);
    card_->setObjectName("transitionCard");
    card_->setFixedSize(360, 160);
    
    auto *card_layout = new QVBoxLayout(card_);
    card_layout->setContentsMargins(28, 24, 28, 24);
    card_layout->setSpacing(14);
    card_layout->setAlignment(Qt::AlignCenter);
    
    status_lbl_ = new QLabel("Aplicando apariencia...", card_);
    status_lbl_->setFont(DesignTokens::getFont("heading_sm", 15));
    status_lbl_->setAlignment(Qt::AlignCenter);
    card_layout->addWidget(status_lbl_);
    
    progress_bar_ = new QProgressBar(card_);
    progress_bar_->setFixedHeight(6);
    progress_bar_->setTextVisible(false);
    progress_bar_->setRange(0, 100);
    progress_bar_->setValue(0);
    card_layout->addWidget(progress_bar_);
    
    percent_lbl_ = new QLabel("0%", card_);
    percent_lbl_->setFont(DesignTokens::getFont("body", 11));
    percent_lbl_->setAlignment(Qt::AlignCenter);
    card_layout->addWidget(percent_lbl_);
    
    main_layout->addWidget(card_);
    
    update_styles();
}

void ThemeTransitionOverlay::update_styles() {
    const auto &c = DesignTokens::current();
    
    card_->setStyleSheet(QString(
        "QFrame#transitionCard {"
        "    background-color: %1;"
        "    border: 1.5px solid %2;"
        "    border-radius: 20px;"
        "}"
    ).arg(c.bg_surface.name()).arg(QString("rgba(%1, %2, %3, %4)").arg(c.border.red()).arg(c.border.green()).arg(c.border.blue()).arg(c.border.alpha() / 255.0)));
    
    status_lbl_->setStyleSheet(QString("color: %1; background: transparent; border: none;").arg(c.text_primary.name()));
    percent_lbl_->setStyleSheet(QString("color: %1; background: transparent; border: none;").arg(c.text_secondary.name()));
    
    progress_bar_->setStyleSheet(QString(
        "QProgressBar {"
        "    background-color: %1;"
        "    border-radius: 3px;"
        "    border: none;"
        "}"
        "QProgressBar::chunk {"
        "    background-color: %2;"
        "    border-radius: 3px;"
        "}"
    ).arg(c.bg_elevated.name()).arg(c.accent.name()));
}

void ThemeTransitionOverlay::paintEvent(QPaintEvent *event) {
    Q_UNUSED(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    
    QColor base_color = DesignTokens::current().bg_base;
    base_color.setAlpha(225);
    painter.fillRect(rect(), base_color);
}

void ThemeTransitionOverlay::resizeEvent(QResizeEvent *event) {
    QWidget::resizeEvent(event);
    if (parentWidget()) {
        setGeometry(parentWidget()->rect());
    }
}

void ThemeTransitionOverlay::start_transition(std::function<void()> on_midpoint) {
    on_midpoint_callback_ = on_midpoint;
    midpoint_fired_ = false;
    
    update_styles();
    opacity_effect_->setOpacity(1.0);
    progress_bar_->setValue(0);
    percent_lbl_->setText("0%");
    
    show();
    raise();

    if (DesignTokens::reducedMotion()) {
        midpoint_fired_ = true;
        if (on_midpoint_callback_) {
            on_midpoint_callback_();
        }
        update_styles();
        update();
        progress_bar_->setValue(100);
        percent_lbl_->setText("100%");
        opacity_effect_->setOpacity(1.0);
        hide();
        return;
    }
    
    if (progress_anim_) {
        progress_anim_->stop();
        progress_anim_->deleteLater();
    }
    
    progress_anim_ = new QVariantAnimation(this);
    progress_anim_->setDuration(DesignTokens::duration(450));
    progress_anim_->setStartValue(0);
    progress_anim_->setEndValue(100);
    progress_anim_->setEasingCurve(QEasingCurve::OutCubic);
    
    connect(progress_anim_, &QVariantAnimation::valueChanged, this, [this](const QVariant &value) {
        int val = value.toInt();
        progress_bar_->setValue(val);
        percent_lbl_->setText(QString("%1%").arg(val));
        
        if (val >= 50 && !midpoint_fired_) {
            midpoint_fired_ = true;
            if (on_midpoint_callback_) {
                on_midpoint_callback_();
            }
            update_styles();
            update();
        }
    });
    
    connect(progress_anim_, &QVariantAnimation::finished, this, [this]() {
        if (fade_anim_) {
            fade_anim_->stop();
            fade_anim_->deleteLater();
        }
        fade_anim_ = new QPropertyAnimation(opacity_effect_, "opacity", this);
        fade_anim_->setDuration(DesignTokens::duration(250));
        fade_anim_->setStartValue(1.0);
        fade_anim_->setEndValue(0.0);
        fade_anim_->setEasingCurve(QEasingCurve::InOutSine);
        connect(fade_anim_, &QPropertyAnimation::finished, this, &ThemeTransitionOverlay::hide);
        fade_anim_->start();
    });
    
    progress_anim_->start();
}
