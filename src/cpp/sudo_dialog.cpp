#include "sudo_dialog.h"
#include "ffi_utils.h"
#include <QGuiApplication>
#include <QScreen>
#include <QThread>
#include <QQmlContext>
#include <QVBoxLayout>
#include "design_tokens.h"
#include "doremi/src/bridge.rs.h"

SudoPasswordDialog::SudoPasswordDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(tr_q("sudo_title") + " — Doremi");
    setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint | Qt::NoDropShadowWindowHint);
    setAttribute(Qt::WA_TranslucentBackground);
    setModal(true);
    setFixedSize(480, 320);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    quick_widget_ = new QQuickWidget(this);
    quick_widget_->setResizeMode(QQuickWidget::SizeRootObjectToView);
    quick_widget_->setAttribute(Qt::WA_TranslucentBackground);
    quick_widget_->setClearColor(Qt::transparent);

    quick_widget_->rootContext()->setContextProperty("SudoCtrl", this);
    quick_widget_->setSource(QUrl("qrc:/qml/SudoDialog.qml"));

    layout->addWidget(quick_widget_);
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

void SudoPasswordDialog::requestCancel() {
    reject();
}

void SudoPasswordDialog::requestAccept(const QString &pwd) {
    if (pwd.isEmpty()) {
        emit errorOccurred(tr_q("sudo_error_empty"));
        return;
    }

    emit validatingChanged(true);

    auto *worker = QThread::create([this, pwd]() {
        bool is_valid = on_validate_sudo_password(Ffi::to_std_string(pwd));
        QMetaObject::invokeMethod(this, [this, pwd, is_valid]() {
            emit validatingChanged(false);
            if (is_valid) {
                password_ = pwd;
                accept();
            } else {
                emit errorOccurred(tr_q("sudo_error_incorrect"));
            }
        });
    });
    connect(worker, &QThread::finished, worker, &QObject::deleteLater);
    worker->start();
}
