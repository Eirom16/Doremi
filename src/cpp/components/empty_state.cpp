#include "empty_state.h"
#include "design_tokens.h"
#include "icon_provider.h"
#include <QVBoxLayout>
#include <QLabel>
#include <QPushButton>

EmptyState::EmptyState(QWidget *parent)
    : QWidget(parent)
{
    setObjectName("emptyState");
    layout_ = new QVBoxLayout(this);
    layout_->setContentsMargins(28, 28, 28, 28);
    layout_->setSpacing(10);
    layout_->setAlignment(Qt::AlignCenter);
}

void EmptyState::applyPanelStyle(const QString &state) {
    const auto &c = DesignTokens::current();
    setStyleSheet(QString(
        "QWidget#emptyState {"
        "    background: %1;"
        "    border: 1px solid %2;"
        "    border-radius: %3px;"
        "}")
        .arg(state == "error" ? DesignTokens::rgba(c.danger_surface) : DesignTokens::rgba(c.surface_selected))
        .arg(state == "error" ? DesignTokens::rgba(c.error) : DesignTokens::rgba(c.border_accent))
        .arg(DesignTokens::radius().lg));
}

void EmptyState::setIcon(const QString &name, int size) {
    if (!icon_) {
        icon_ = IconProvider::createIconLabel(name, size, DesignTokens::current().accent, true, this);
        icon_->setAlignment(Qt::AlignCenter);
        layout_->insertWidget(0, icon_, 0, Qt::AlignHCenter);
    } else {
        IconProvider::setupIconLabel(icon_, name, size, DesignTokens::current().accent, true);
    }
}

void EmptyState::setTitle(const QString &text) {
    if (!title_) {
        title_ = new QLabel(text, this);
        title_->setFont(DesignTokens::getFont("heading_sm"));
        title_->setStyleSheet(QString("color: %1; background: transparent; font-weight: 600;").arg(DesignTokens::current().text_primary.name()));
        title_->setAlignment(Qt::AlignCenter);
        if (icon_)
            layout_->insertWidget(1, title_, 0, Qt::AlignHCenter);
        else
            layout_->insertWidget(0, title_, 0, Qt::AlignHCenter);
    } else {
        title_->setText(text);
    }
}

void EmptyState::setDescription(const QString &text) {
    if (!description_) {
        description_ = new QLabel(text, this);
        description_->setFont(DesignTokens::getFont("body_sm"));
        description_->setStyleSheet(QString("color: %1; background: transparent;").arg(DesignTokens::current().text_secondary.name()));
        description_->setAlignment(Qt::AlignCenter);
        description_->setWordWrap(true);
        description_->setMaximumWidth(460);
        int idx = 0;
        if (icon_) idx++;
        if (title_) idx++;
        layout_->insertWidget(idx, description_, 0, Qt::AlignHCenter);
    } else {
        description_->setText(text);
    }
}

void EmptyState::setMessage(const QString &text) {
    auto *lbl = new QLabel(text, this);
    lbl->setFont(DesignTokens::getFont("body"));
    lbl->setStyleSheet(QString("color: %1; padding: 24px;").arg(DesignTokens::current().text_muted.name()));
    lbl->setAlignment(Qt::AlignCenter);
    layout_->addWidget(lbl, 0, Qt::AlignHCenter);
}

QPushButton *EmptyState::addButton(const QString &text) {
    auto *btn = new QPushButton(text, this);
    btn->setCursor(Qt::PointingHandCursor);
    btn->setMinimumHeight(38);
    btn->setProperty("buttonRole", "primary");
    layout_->addWidget(btn, 0, Qt::AlignHCenter);
    return btn;
}

QPushButton *EmptyState::addRetryButton() {
    auto *btn = addButton(tr_q("retry"));
    return btn;
}
