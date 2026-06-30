#include "create_playlist_dialog.h"
#include "design_tokens.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>

CreatePlaylistDialog::CreatePlaylistDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(tr_q("create_playlist"));
    setFixedSize(400, 300);
    const auto &c = DesignTokens::current();

    auto *layout = new QVBoxLayout(this);
    layout->setSpacing(16);
    layout->setContentsMargins(DesignTokens::pagePadding());

    auto *title_lbl = new QLabel(tr_q("dialog_new_playlist"), this);
    title_lbl->setFont(DesignTokens::getFont("heading_lg"));
    title_lbl->setStyleSheet(QString("color: %1; font-weight: 700;").arg(c.text_primary.name()));
    layout->addWidget(title_lbl);

    name_edit_ = new QLineEdit(this);
    name_edit_->setPlaceholderText(tr_q("playlist_name_placeholder"));
    name_edit_->setStyleSheet(QString(
        "QLineEdit { background: %1; color: %2; border: 1px solid %3; border-radius: %5px; padding: 10px 14px; font-size: 14px; }"
        "QLineEdit:focus { border-color: %4; }"
    ).arg(c.bg_surface.name(), c.text_primary.name(), c.border.name(), c.accent.name(), QString::number(DesignTokens::radius().md)));
    layout->addWidget(name_edit_);

    desc_edit_ = new QTextEdit(this);
    desc_edit_->setPlaceholderText(tr_q("playlist_desc_placeholder"));
    desc_edit_->setFixedHeight(80);
    desc_edit_->setStyleSheet(QString(
        "QTextEdit { background: %1; color: %2; border: 1px solid %3; border-radius: %5px; padding: 10px 14px; font-size: 14px; }"
        "QTextEdit:focus { border-color: %4; }"
    ).arg(c.bg_surface.name(), c.text_primary.name(), c.border.name(), c.accent.name(), QString::number(DesignTokens::radius().md)));
    layout->addWidget(desc_edit_);

    privacy_combo_ = new QComboBox(this);
    privacy_combo_->addItem(tr_q("privacy_public"), QString("PUBLIC"));
    privacy_combo_->addItem(tr_q("privacy_unlisted"), QString("UNLISTED"));
    privacy_combo_->addItem(tr_q("privacy_private"), QString("PRIVATE"));
    privacy_combo_->setStyleSheet(QString(
        "QComboBox { background: %1; color: %2; border: 1px solid %3; border-radius: %5px; padding: 8px 14px; font-size: 14px; }"
        "QComboBox::drop-down { border: none; }"
        "QComboBox QAbstractItemView { background: %1; color: %2; selection-background-color: %4; }"
    ).arg(c.bg_surface.name(), c.text_primary.name(), c.border.name(), c.accent_dim.name(), QString::number(DesignTokens::radius().md)));
    layout->addWidget(privacy_combo_);

    layout->addStretch();

    auto *btn_layout = new QHBoxLayout();
    btn_layout->addStretch();

    auto *cancel_btn = new QPushButton(tr_q("cancel"), this);
    cancel_btn->setStyleSheet(QString(
        "QPushButton { background: transparent; color: %1; border: 1px solid %2; border-radius: %3px; padding: 8px 20px; font-size: 14px; }"
        "QPushButton:hover { background: rgba(255,255,255,0.05); }"
    ).arg(c.text_secondary.name(), c.border.name(), QString::number(DesignTokens::radius().md)));
    connect(cancel_btn, &QPushButton::clicked, this, &QDialog::reject);
    btn_layout->addWidget(cancel_btn);

    auto *create_btn = new QPushButton(tr_q("create"), this);
    create_btn->setProperty("buttonRole", "primary");
    connect(create_btn, &QPushButton::clicked, this, [this]() {
        if (name_edit_->text().trimmed().isEmpty()) {
            return;
        }
        accept();
    });
    btn_layout->addWidget(create_btn);

    layout->addLayout(btn_layout);

    setStyleSheet(QString("QDialog { background: %1; }").arg(c.bg_base.name()));

    name_edit_->setFocus();
}

QString CreatePlaylistDialog::playlistName() const { return name_edit_->text().trimmed(); }
QString CreatePlaylistDialog::description() const { return desc_edit_->toPlainText().trimmed(); }
QString CreatePlaylistDialog::privacy() const {
    return privacy_combo_->currentData().toString();
}