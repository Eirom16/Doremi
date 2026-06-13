#include "update_dialog.h"
#include "design_tokens.h"
#include "icon_provider.h"
#include "sudo_dialog.h"
#include <QGraphicsDropShadowEffect>
#include <QApplication>
#include <QScreen>
#include <QDesktopServices>
#include <QUrl>
#include <QTimer>
#include <QProcess>
#include <QCoreApplication>
#include "ffi_utils.h"
#include "doremi/src/bridge.rs.h"

UpdateDialog *UpdateDialog::active_instance_ = nullptr;

UpdateDialog::UpdateDialog(QWidget *parent)
    : QDialog(parent)
{
    if (active_instance_) {
        active_instance_->close();
        active_instance_->deleteLater();
    }
    active_instance_ = this;

    setWindowTitle(QString::fromStdString(std::string(doremi_tr("update_window_title"))));
    setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);
    setAttribute(Qt::WA_TranslucentBackground);
    setModal(true);
    setMinimumWidth(520);

    build_ui();
    center_on_parent();
}

UpdateDialog::~UpdateDialog() {
    if (active_instance_ == this) {
        active_instance_ = nullptr;
    }
}

void UpdateDialog::show_if_available(const QString &version, const QString &notes,
                                      const QString &url, const QString &asset_url,
                                      const QString &asset_name, qint64 asset_size) {
    auto *dlg = new UpdateDialog(QApplication::activeWindow());
    dlg->set_release_info(version, notes, url, asset_url, asset_name, asset_size);
    dlg->show();
}

void UpdateDialog::center_on_parent() {
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

void UpdateDialog::build_ui() {
    const auto &c = DesignTokens::current();

    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);

    panel_ = new QFrame(this);
    panel_->setObjectName("updatePanel");
    panel_->setStyleSheet(QString(
        "#updatePanel {"
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

    // Header row: Icon + Title
    auto *header_row = new QHBoxLayout();
    auto *update_icon = IconProvider::createIconLabel("new_releases", 32, c.accent, true, panel_);
    
    auto *title_col = new QVBoxLayout();
    title_col->setSpacing(2);

    title_lbl_ = new QLabel(QString::fromStdString(std::string(doremi_tr("update_available_title"))), panel_);
    title_lbl_->setFont(DesignTokens::getFont("heading", 18));
    title_lbl_->setStyleSheet(QString("color: %1; background: transparent; font-weight: bold;").arg(c.text_primary.name()));

    version_lbl_ = new QLabel(panel_);
    version_lbl_->setFont(DesignTokens::getFont("body", 13));
    version_lbl_->setStyleSheet(QString("color: %1; background: transparent; font-family: monospace;").arg(c.accent.name()));

    title_col->addWidget(title_lbl_);
    title_col->addWidget(version_lbl_);

    header_row->addWidget(update_icon);
    header_row->addSpacing(12);
    header_row->addLayout(title_col);
    header_row->addStretch();

    // Close button
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

    // Release Notes Box
    notes_label_ = new QLabel(QString::fromStdString(std::string(doremi_tr("update_notes_label"))), panel_);
    notes_label_->setFont(DesignTokens::getFont("body", 12));
    notes_label_->setStyleSheet(QString("color: %1; background: transparent;").arg(c.text_secondary.name()));
    layout->addWidget(notes_label_);

    notes_box_ = new QTextEdit(panel_);
    notes_box_->setReadOnly(true);
    notes_box_->setFont(DesignTokens::getFont("body", 13));
    notes_box_->setFixedHeight(140);
    notes_box_->setStyleSheet(QString(
        "QTextEdit {"
        "    background: %1;"
        "    border: 1px solid rgba(%2, %3, %4, 0.08);"
        "    border-radius: 12px;"
        "    color: %5;"
        "    padding: 10px;"
        "}"
    ).arg(c.bg_surface.name())
     .arg(c.accent.red()).arg(c.accent.green()).arg(c.accent.blue())
     .arg(c.text_secondary.name()));
    layout->addWidget(notes_box_);

    // Progress Bar Container (hidden until downloading)
    progress_container_ = new QWidget(panel_);
    auto *prog_layout = new QVBoxLayout(progress_container_);
    prog_layout->setContentsMargins(0, 0, 0, 0);
    prog_layout->setSpacing(6);

    progress_bar_ = new QProgressBar(progress_container_);
    progress_bar_->setRange(0, 100);
    progress_bar_->setValue(0);
    progress_bar_->setTextVisible(false);
    progress_bar_->setFixedHeight(8);
    progress_bar_->setStyleSheet(QString(
        "QProgressBar {"
        "    background-color: %1;"
        "    border: none;"
        "    border-radius: 4px;"
        "}"
        "QProgressBar::chunk {"
        "    background-color: %2;"
        "    border-radius: 4px;"
        "}"
    ).arg(c.bg_surface.name()).arg(c.accent.name()));

    progress_label_ = new QLabel(QString::fromStdString(std::string(doremi_tr("update_preparing"))), progress_container_);
    progress_label_->setFont(DesignTokens::getFont("body", 12));
    progress_label_->setStyleSheet(QString("color: %1; background: transparent;").arg(c.text_secondary.name()));

    prog_layout->addWidget(progress_bar_);
    prog_layout->addWidget(progress_label_);
    progress_container_->hide();
    layout->addWidget(progress_container_);

    // Action buttons
    auto *btn_row = new QHBoxLayout();
    btn_row->setSpacing(10);

    github_btn_ = new RippleButton(QString::fromStdString(std::string(doremi_tr("update_btn_github"))), panel_, RippleButton::Variant::Ghost);
    github_btn_->setIcon(IconProvider::getIcon("open_in_new", c.text_secondary, 20));
    github_btn_->setFont(DesignTokens::getFont("body", 13));
    connect(github_btn_, &QPushButton::clicked, this, &UpdateDialog::on_github_clicked);

    postpone_btn_ = new RippleButton(QString::fromStdString(std::string(doremi_tr("update_btn_postpone"))), panel_, RippleButton::Variant::Secondary);
    connect(postpone_btn_, &QPushButton::clicked, this, &QDialog::reject);

    update_btn_ = new RippleButton(panel_, RippleButton::Variant::Primary);
    update_btn_->setIcon(IconProvider::getIcon("download", c.text_primary, 20));
    update_btn_->setFont(DesignTokens::getFont("body", 14));
    update_btn_->setMinimumHeight(44);
    connect(update_btn_, &QPushButton::clicked, this, &UpdateDialog::on_update_clicked);

    btn_row->addWidget(github_btn_);
    btn_row->addStretch();
    btn_row->addWidget(postpone_btn_);
    btn_row->addWidget(update_btn_);

    layout->addLayout(btn_row);
    root->addWidget(panel_);
}

void UpdateDialog::set_release_info(const QString &version, const QString &notes,
                                      const QString &url, const QString &asset_url,
                                      const QString &asset_name, qint64 asset_size) {
    version_ = version;
    notes_ = notes;
    url_ = url;
    asset_url_ = asset_url;
    asset_name_ = asset_name;
    asset_size_ = asset_size;

    version_lbl_->setText(QString("%1  →  %2").arg(QString::fromStdString(std::string(get_app_version()))).arg(version));
    notes_box_->setPlainText(notes);
    update_btn_->setText(QString::fromStdString(std::string(doremi_tr("update_btn_upgrade"))) + version);
}

void UpdateDialog::on_github_clicked() {
    QDesktopServices::openUrl(QUrl(url_));
}

void UpdateDialog::on_update_clicked() {
    if (asset_url_.isEmpty()) {
        progress_label_->setText(QString::fromStdString(std::string(doremi_tr("update_no_package"))));
        progress_container_->show();
        return;
    }

    update_btn_->setEnabled(false);
    postpone_btn_->setEnabled(false);
    progress_container_->show();
    progress_label_->setText(QString::fromStdString(std::string(doremi_tr("update_starting"))));
    downloading_ = true;

    // Call Rust to start downloading
    on_download_update_requested(
        Ffi::to_std_string(asset_url_),
        Ffi::to_std_string(asset_name_));
}

void UpdateDialog::set_download_progress(double percent, const QString &message) {
    progress_bar_->setValue(static_cast<int>(percent));
    progress_label_->setText(message);
}

void UpdateDialog::set_download_failed(const QString &error) {
    progress_label_->setText(error);
    update_btn_->setEnabled(true);
    postpone_btn_->setEnabled(true);
    downloading_ = false;
}

void UpdateDialog::set_download_finished(const QString &package_path) {
    package_path_ = package_path;
    progress_bar_->setValue(100);
    progress_label_->setText(QString::fromStdString(std::string(doremi_tr("update_auth_required"))));

    // Request Sudo Password Dialog
    QString pwd;
    SudoPasswordDialog sudo_dlg(this);
    if (sudo_dlg.exec() == QDialog::Accepted) {
        pwd = sudo_dlg.get_password();
    } else {
        progress_label_->setText(QString::fromStdString(std::string(doremi_tr("update_cancelled"))) + package_path_);
        update_btn_->setEnabled(true);
        postpone_btn_->setEnabled(true);
        downloading_ = false;
        return;
    }

    progress_label_->setText(QString::fromStdString(std::string(doremi_tr("update_installing"))));

    // Call Rust to start installation
    on_install_update_requested(
        Ffi::to_std_string(package_path_),
        Ffi::to_std_string(pwd));
}

void UpdateDialog::set_install_finished(bool success) {
    const auto &c = DesignTokens::current();
    if (success) {
        progress_label_->setText(QString::fromStdString(std::string(doremi_tr("update_install_success"))));
        update_btn_->setText(QString::fromStdString(std::string(doremi_tr("update_install_restarting"))));
        update_btn_->setIcon(IconProvider::getIcon("check_circle", c.accent, 20));
        QTimer::singleShot(2000, this, &UpdateDialog::restart_app);
    } else {
        progress_label_->setText(QString::fromStdString(std::string(doremi_tr("update_install_failed"))) + package_path_);
        update_btn_->setEnabled(true);
        postpone_btn_->setEnabled(true);
        downloading_ = false;
    }
}

void UpdateDialog::restart_app() {
    QStringList args = QCoreApplication::arguments();
    QString app_path = args.takeFirst();
    QProcess::startDetached(app_path, args);
    QCoreApplication::quit();
}

// ── CXX Bridge Callbacks ──

void set_update_available(rust::Str version, rust::Str notes, rust::Str url, rust::Str asset_url, rust::Str asset_name, int64_t asset_size) {
    const QString version_copy = Ffi::to_qstring(version);
    const QString notes_copy = Ffi::to_qstring(notes);
    const QString url_copy = Ffi::to_qstring(url);
    const QString asset_url_copy = Ffi::to_qstring(asset_url);
    const QString asset_name_copy = Ffi::to_qstring(asset_name);
    Ffi::on_gui("set_update_available", [=]() {
        UpdateDialog::show_if_available(
            version_copy,
            notes_copy,
            url_copy,
            asset_url_copy,
            asset_name_copy,
            asset_size
        );
    });
}

void set_no_update_available() {
    // Silently ignore or log check complete
}

void set_update_download_progress(double percent, rust::Str message) {
    const QString message_copy = Ffi::to_qstring(message);
    Ffi::on_gui("set_update_download_progress", [=]() {
        if (UpdateDialog::active_instance()) {
            UpdateDialog::active_instance()->set_download_progress(percent, message_copy);
        }
    });
}

void set_update_download_finished(rust::Str package_path) {
    const QString path_copy = Ffi::to_qstring(package_path);
    Ffi::on_gui("set_update_download_finished", [=]() {
        if (UpdateDialog::active_instance()) {
            UpdateDialog::active_instance()->set_download_finished(path_copy);
        }
    });
}

void set_update_download_failed(rust::Str error) {
    const QString error_copy = Ffi::to_qstring(error);
    Ffi::on_gui("set_update_download_failed", [=]() {
        if (UpdateDialog::active_instance()) {
            UpdateDialog::active_instance()->set_download_failed(error_copy);
        }
    });
}

void set_update_install_finished(bool success) {
    Ffi::on_gui("set_update_install_finished", [=]() {
        if (UpdateDialog::active_instance()) {
            UpdateDialog::active_instance()->set_install_finished(success);
        }
    });
}
