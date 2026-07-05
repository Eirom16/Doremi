#include "update_dialog.h"
#include "sudo_dialog.h"
#include "design_tokens.h"
#include <QQmlContext>
#include <QVBoxLayout>
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

    setWindowTitle(tr_q("update_window_title"));
    setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint | Qt::NoDropShadowWindowHint);
    setAttribute(Qt::WA_TranslucentBackground);
    setModal(true);
    setFixedSize(600, 480);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    quick_widget_ = new QQuickWidget(this);
    quick_widget_->setResizeMode(QQuickWidget::SizeRootObjectToView);
    quick_widget_->setAttribute(Qt::WA_TranslucentBackground);
    quick_widget_->setClearColor(Qt::transparent);

    quick_widget_->rootContext()->setContextProperty("UpdateCtrl", this);
    quick_widget_->setSource(QUrl("qrc:/qml/UpdateDialog.qml"));

    layout->addWidget(quick_widget_);
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

void UpdateDialog::set_release_info(const QString &version, const QString &notes,
                                      const QString &url, const QString &asset_url,
                                      const QString &asset_name, qint64 asset_size) {
    version_ = version;
    notes_ = notes;
    url_ = url;
    asset_url_ = asset_url;
    asset_name_ = asset_name;
    asset_size_ = asset_size;

    emit versionChanged();
    emit notesChanged();
}

void UpdateDialog::openGithub() {
    QDesktopServices::openUrl(QUrl(url_));
}

void UpdateDialog::requestDownload() {
    if (asset_url_.isEmpty()) {
        status_message_ = tr_q("update_no_package");
        emit statusMessageChanged();
        return;
    }

    downloading_ = true;
    emit isDownloadingChanged();
    
    status_message_ = tr_q("update_starting");
    emit statusMessageChanged();

    // Call Rust to start downloading
    on_download_update_requested(
        Ffi::to_std_string(asset_url_),
        Ffi::to_std_string(asset_name_));
}

void UpdateDialog::set_download_progress(double percent, const QString &message) {
    download_progress_ = percent;
    status_message_ = message;
    emit downloadProgressChanged();
    emit statusMessageChanged();
}

void UpdateDialog::set_download_failed(const QString &error) {
    status_message_ = error;
    downloading_ = false;
    emit statusMessageChanged();
    emit isDownloadingChanged();
}

void UpdateDialog::set_download_finished(const QString &package_path) {
    package_path_ = package_path;
    download_progress_ = 100;
    emit downloadProgressChanged();
    
    status_message_ = tr_q("update_auth_required");
    emit statusMessageChanged();

    // Request Sudo Password Dialog
    QString pwd;
    SudoPasswordDialog sudo_dlg(this);
    if (sudo_dlg.exec() == QDialog::Accepted) {
        pwd = sudo_dlg.get_password();
    } else {
        status_message_ = tr_q("update_cancelled") + package_path_;
        emit statusMessageChanged();
        downloading_ = false;
        emit isDownloadingChanged();
        return;
    }

    status_message_ = tr_q("update_installing");
    emit statusMessageChanged();

    // Call Rust to start installation
    on_install_update_requested(
        Ffi::to_std_string(package_path_),
        Ffi::to_std_string(pwd));
}

void UpdateDialog::set_install_finished(bool success) {
    if (success) {
        install_success_ = true;
        ready_to_restart_ = true;
        status_message_ = tr_q("update_install_success");
        emit isInstallSuccessChanged();
        emit isReadyToRestartChanged();
        emit statusMessageChanged();
        
        QTimer::singleShot(3000, this, &UpdateDialog::restart_app);
    } else {
        install_failed_ = true;
        downloading_ = false;
        status_message_ = tr_q("update_install_failed") + package_path_;
        emit isInstallFailedChanged();
        emit isDownloadingChanged();
        emit statusMessageChanged();
    }
}

void UpdateDialog::requestRestart() {
    restart_app();
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
