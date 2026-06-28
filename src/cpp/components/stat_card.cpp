#include "stat_card.h"
#include "../design_tokens.h"
#include "../icon_provider.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPainter>
#include <QPainterPath>

StatCard::StatCard(const QString &title, const QString &icon_name, QWidget *parent)
    : QWidget(parent), title_(title), icon_name_(icon_name)
{
    const auto &c = DesignTokens::current();
    setMinimumHeight(100);

    auto *card_layout = new QVBoxLayout(this);
    card_layout->setContentsMargins(16, 16, 16, 16);
    card_layout->setSpacing(8);

    // Header row: Title + Icon
    auto *header = new QHBoxLayout();
    header->setContentsMargins(0, 0, 0, 0);

    title_lbl_ = new QLabel(title_, this);
    title_lbl_->setFont(DesignTokens::getFont("caption", 12));
    title_lbl_->setStyleSheet(QString("color: %1; font-weight: bold;").arg(c.text_secondary.name()));
    
    icon_lbl_ = IconProvider::createIconLabel(icon_name_, 20, c.accent, true, this);

    header->addWidget(title_lbl_);
    header->addStretch();
    header->addWidget(icon_lbl_);
    card_layout->addLayout(header);

    // Large value label
    value_lbl_ = new QLabel("0", this);
    value_lbl_->setFont(DesignTokens::getFont("heading_lg"));
    value_lbl_->setStyleSheet(QString("color: %1; font-weight: bold;").arg(c.text_primary.name()));
    card_layout->addWidget(value_lbl_);

    setLayout(card_layout);

    // Setup border and background in stylesheet
    setObjectName("StatCard");
    setStyleSheet(QString("QWidget#StatCard { %1 }").arg(DesignTokens::panelStyle("surface", 12)));
}

void StatCard::setValue(int target_value, const QString &prefix, const QString &suffix) {
    is_numeric_ = true;
    target_value_ = target_value;
    prefix_ = prefix;
    suffix_ = suffix;

    if (count_anim_) {
        count_anim_->stop();
        delete count_anim_;
    }

    count_anim_ = new QVariantAnimation(this);
    count_anim_->setDuration(DesignTokens::duration(800));
    count_anim_->setEasingCurve(QEasingCurve::OutCubic);
    count_anim_->setStartValue(0);
    count_anim_->setEndValue(target_value_);
    
    connect(count_anim_, &QVariantAnimation::valueChanged, this, [this](const QVariant &val) {
        setDisplayValue(val.toInt());
    });

    if (DesignTokens::reducedMotion()) {
        setDisplayValue(target_value_);
    } else {
        count_anim_->start();
    }
}

void StatCard::setValueText(const QString &text) {
    is_numeric_ = false;
    static_text_ = text;
    value_lbl_->setText(static_text_);
}

void StatCard::setDisplayValue(int val) {
    display_value_ = val;
    updateLabels();
}

void StatCard::updateLabels() {
    if (is_numeric_) {
        value_lbl_->setText(prefix_ + QString::number(display_value_) + suffix_);
    } else {
        value_lbl_->setText(static_text_);
    }
}

void StatCard::paintEvent(QPaintEvent *event) {
    QWidget::paintEvent(event);
    
    // Draw subtle border accent if hovered
    if (is_hovered_) {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);
        painter.setPen(QPen(DesignTokens::current().accent, 1.5));
        painter.setBrush(Qt::NoBrush);
        
        QPainterPath path;
        path.addRoundedRect(rect(), 12, 12);
        painter.drawPath(path);
    }
}

void StatCard::enterEvent(QEnterEvent *event) {
    QWidget::enterEvent(event);
    is_hovered_ = true;
    const auto &c = DesignTokens::current();
    setStyleSheet(QString("QWidget#StatCard { %1 border-color: %2; }")
        .arg(DesignTokens::panelStyle("elevated", 12))
        .arg(DesignTokens::rgba(c.border_accent)));
    update();
}

void StatCard::leaveEvent(QEvent *event) {
    QWidget::leaveEvent(event);
    is_hovered_ = false;
    setStyleSheet(QString("QWidget#StatCard { %1 }").arg(DesignTokens::panelStyle("surface", 12)));
    update();
}

void StatCard::update_theme() {
    const auto &c = DesignTokens::current();
    title_lbl_->setStyleSheet(QString("color: %1; font-weight: bold;").arg(c.text_secondary.name()));
    value_lbl_->setStyleSheet(QString("color: %1; font-weight: bold;").arg(c.text_primary.name()));
    IconProvider::setupIconLabel(icon_lbl_, icon_name_, 20, c.accent, true);
    if (is_hovered_) {
        setStyleSheet(QString("QWidget#StatCard { %1 border-color: %2; }")
            .arg(DesignTokens::panelStyle("elevated", 12))
            .arg(DesignTokens::rgba(c.border_accent)));
    } else {
        setStyleSheet(QString("QWidget#StatCard { %1 }").arg(DesignTokens::panelStyle("surface", 12)));
    }
    update();
}
