#include "sudo_dialog.h"
#include "ffi_utils.h"
#include "design_tokens.h"
#include "icon_provider.h"
#include <QGraphicsDropShadowEffect>
#include <QGuiApplication>
#include <QScreen>
#include <QThread>
#include <QFile>
#include "doremi/src/bridge.rs.h"

SudoPasswordDialog::SudoPasswordDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(tr_q("sudo_title") + " — Doremi");
    setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);
    setAttribute(Qt::WA_TranslucentBackground);
    setModal(true);
    setMinimumWidth(460);

    build_ui();
    center_on_parent();
}

SudoPasswordDialog::~SudoPasswordDialog() = default;

void SudoPasswordDialog::center_on_parent() {
    auto *p = parentWidget();
    if (p && p->isVisible()) {
        auto p_geo = p->geometry();
        int x = p_geo.x() + (p_geo.width() - width()) / 2;
        int y = p_geo.y() + (p_geo.height() - height()) / 2;
        move(x, y);
    } else {
        auto *screen = QGuiApplication::primaryScreen();
        if (screen) {
            auto screen_geo = screen->geometry();
            int x = screen_geo.x() + (screen_geo.width() - width()) / 2;
            int y = screen_geo.y() + (screen_geo.height() - height()) / 2;
            move(x, y);
        }
    }
}

void SudoPasswordDialog::build_ui() {
    const auto &c = DesignTokens::current();

    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);

    panel_ = new QFrame(this);
    panel_->setObjectName("sudoPanel");
    panel_->setStyleSheet(QString(
        "#sudoPanel {"
        "    background-color: %1;"
        "    border-radius: 20px;"
        "    border: 1px solid rgba(%2, %3, %4, 0.25);"
        "}"
    ).arg(c.bg_elevated.name())
     .arg(c.accent.red()).arg(c.accent.green()).arg(c.accent.blue()));

    auto *shadow = new QGraphicsDropShadowEffect(panel_);
    shadow->setBlurRadius(48);
    shadow->setOffset(0, 12);
    shadow->setColor(QColor(0, 0, 0, 120));
    panel_->setGraphicsEffect(shadow);

    auto *layout = new QVBoxLayout(panel_);
    layout->setContentsMargins(28, 24, 28, 24);
    layout->setSpacing(16);

    // Header Row: Security Icon + Title
    auto *header_row = new QHBoxLayout();
    
    auto *shield_icon = IconProvider::createIconLabel("security", 32, c.accent, true, panel_);
    
    auto *title_col = new QVBoxLayout();
    title_col->setSpacing(2);
    
    title_lbl_ = new QLabel(tr_q("sudo_title"), panel_);
    title_lbl_->setFont(DesignTokens::getFont("heading", 16));
    title_lbl_->setStyleSheet(QString("color: %1; background: transparent; font-weight: bold;").arg(c.text_primary.name()));
    
    subtitle_lbl_ = new QLabel(tr_q("sudo_subtitle"), panel_);
    subtitle_lbl_->setFont(DesignTokens::getFont("body", 12));
    subtitle_lbl_->setStyleSheet(QString("color: %1; background: transparent;").arg(c.text_secondary.name()));
    
    title_col->addWidget(title_lbl_);
    title_col->addWidget(subtitle_lbl_);

    header_row->addWidget(shield_icon);
    header_row->addSpacing(12);
    header_row->addLayout(title_col);
    header_row->addStretch();

    // Close Dialog Button
    auto *close_btn = new RippleButton(panel_, RippleButton::Variant::Ghost);
    close_btn->setText("✕");
    close_btn->setFont(DesignTokens::getFont("body", 14));
    close_btn->setFixedSize(36, 36);
    connect(close_btn, &QPushButton::clicked, this, &QDialog::reject);
    header_row->addWidget(close_btn);

    layout->addLayout(header_row);

    // Separator line
    auto *sep = new QFrame(panel_);
    sep->setFrameShape(QFrame::HLine);
    sep->setStyleSheet(QString("color: rgba(%1, %2, %3, 0.10);").arg(c.accent.red()).arg(c.accent.green()).arg(c.accent.blue()));
    layout->addWidget(sep);

    // Prompt Label
    prompt_lbl_ = new QLabel(tr_q("sudo_prompt"), panel_);
    prompt_lbl_->setWordWrap(true);
    prompt_lbl_->setFont(DesignTokens::getFont("body", 13));
    prompt_lbl_->setStyleSheet(QString("color: %1; background: transparent;").arg(c.text_secondary.name()));
    layout->addWidget(prompt_lbl_);

    // Password input Row
    auto *input_container = new QWidget(panel_);
    auto *input_layout = new QHBoxLayout(input_container);
    input_layout->setContentsMargins(0, 0, 0, 0);
    input_layout->setSpacing(8);

    password_input_ = new QLineEdit(input_container);
    password_input_->setEchoMode(QLineEdit::Password);
    password_input_->setPlaceholderText(tr_q("sudo_placeholder"));
    password_input_->setMinimumHeight(42);
    password_input_->setStyleSheet(QString(
        "QLineEdit {"
        "    background-color: %1;"
        "    border: 1px solid rgba(%2, %3, %4, 0.15);"
        "    border-radius: 12px;"
        "    padding: 6px 14px;"
        "    color: %5;"
        "    font-size: 14px;"
        "}"
        "QLineEdit:focus {"
        "    border: 1px solid %6;"
        "}"
    ).arg(c.bg_surface.name())
     .arg(c.accent.red()).arg(c.accent.green()).arg(c.accent.blue())
     .arg(c.text_primary.name())
     .arg(c.accent.name()));
    
    connect(password_input_, &QLineEdit::returnPressed, this, &SudoPasswordDialog::on_accept);
    input_layout->addWidget(password_input_);

    // Toggle password visibility button
    toggle_visible_btn_ = new RippleButton(input_container, RippleButton::Variant::Ghost);
    toggle_visible_btn_->setIcon(IconProvider::getIcon("visibility", c.text_secondary, 18));
    toggle_visible_btn_->setFixedSize(40, 40);
    connect(toggle_visible_btn_, &QPushButton::clicked, this, &SudoPasswordDialog::toggle_password_visibility);
    input_layout->addWidget(toggle_visible_btn_);

    layout->addWidget(input_container);

    // Error label
    error_lbl_ = new QLabel("", panel_);
    error_lbl_->setWordWrap(true);
    error_lbl_->setFont(DesignTokens::getFont("body", 12));
    error_lbl_->setStyleSheet(QString("color: %1; background: transparent;").arg(c.error.name()));
    error_lbl_->hide();
    layout->addWidget(error_lbl_);

    // Action buttons
    auto *btn_row = new QHBoxLayout();
    btn_row->setSpacing(10);

    cancel_btn_ = new RippleButton(tr_q("cancel"), panel_, RippleButton::Variant::Secondary);
    connect(cancel_btn_, &QPushButton::clicked, this, &QDialog::reject);

    confirm_btn_ = new RippleButton(tr_q("sudo_confirm_install"), panel_, RippleButton::Variant::Primary);
    confirm_btn_->setFont(DesignTokens::getFont("body", 14));
    confirm_btn_->setMinimumHeight(44);
    connect(confirm_btn_, &QPushButton::clicked, this, &SudoPasswordDialog::on_accept);

    btn_row->addStretch();
    btn_row->addWidget(cancel_btn_);
    btn_row->addWidget(confirm_btn_);
    layout->addLayout(btn_row);

    root->addWidget(panel_);
}

void SudoPasswordDialog::toggle_password_visibility() {
    const auto &c = DesignTokens::current();
    if (password_input_->echoMode() == QLineEdit::Password) {
        password_input_->setEchoMode(QLineEdit::Normal);
        toggle_visible_btn_->setIcon(IconProvider::getIcon("visibility_off", c.text_secondary, 18));
    } else {
        password_input_->setEchoMode(QLineEdit::Password);
        toggle_visible_btn_->setIcon(IconProvider::getIcon("visibility", c.text_secondary, 18));
    }
}

void SudoPasswordDialog::on_accept() {
    QString pwd = password_input_->text();
    if (pwd.isEmpty()) {
        error_lbl_->setText(tr_q("sudo_error_empty"));
        error_lbl_->show();
        return;
    }

    error_lbl_->hide();
    confirm_btn_->setEnabled(false);
    cancel_btn_->setEnabled(false);
    password_input_->setEnabled(false);
    confirm_btn_->setText(tr_q("sudo_verifying"));

    // Run validate password in background thread to avoid freezing UI
    auto *worker = QThread::create([this, pwd]() {
        bool is_valid = on_validate_sudo_password(Ffi::to_std_string(pwd));
        QMetaObject::invokeMethod(this, [this, pwd, is_valid]() {
            if (is_valid) {
                password_ = pwd;
                accept();
            } else {
                error_lbl_->setText(tr_q("sudo_error_incorrect"));
                error_lbl_->show();
                confirm_btn_->setEnabled(true);
                cancel_btn_->setEnabled(true);
                password_input_->setEnabled(true);
                password_input_->clear();
                password_input_->setFocus();
                confirm_btn_->setText(tr_q("sudo_confirm_install"));
            }
        });
    });
    connect(worker, &QThread::finished, worker, &QObject::deleteLater);
    worker->start();
}
