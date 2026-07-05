#include "welcome_view.h"
#include "login_dialog.h"
#include "doremi/src/bridge.rs.h"
#include <QQmlContext>
#include <QVBoxLayout>
#include <QTimer>
#include <QVariantMap>

WelcomeView::WelcomeView(QWidget *parent)
    : QWidget(parent)
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    quick_widget_ = new QQuickWidget(this);
    quick_widget_->setResizeMode(QQuickWidget::SizeRootObjectToView);
    quick_widget_->setAttribute(Qt::WA_TranslucentBackground);
    quick_widget_->setClearColor(Qt::transparent);

    quick_widget_->rootContext()->setContextProperty("WelcomeCtrl", this);
    quick_widget_->setSource(QUrl("qrc:/qml/WelcomeView.qml"));

    layout->addWidget(quick_widget_);
}

void WelcomeView::setStatusText(const QString &text) {
    if (status_text_ != text) {
        status_text_ = text;
        emit statusTextChanged();
    }
}

void WelcomeView::setIsLoggingIn(bool val) {
    if (is_logging_in_ != val) {
        is_logging_in_ = val;
        emit isLoggingInChanged();
    }
}

void WelcomeView::setIsSuccess(bool val) {
    if (is_success_ != val) {
        is_success_ = val;
        emit isSuccessChanged();
    }
}

void WelcomeView::requestLogin() {
    setIsLoggingIn(true);
    setStatusText("Abriendo ventana de inicio de sesión...");
    setIsSuccess(false);

    auto *dialog = new WebLoginDialog(this);
    connect(dialog, &WebLoginDialog::login_successful, this, &WelcomeView::handle_login_success);

    int result = dialog->exec();
    dialog->deleteLater();

    if (result != QDialog::Accepted) {
        setIsLoggingIn(false);
        setStatusText("");
    }
}

void WelcomeView::handle_login_success(const QString &avatar_url, const QString &user_name) {
    Q_UNUSED(avatar_url);
    Q_UNUSED(user_name);
    
    setIsSuccess(true);
    setStatusText("Inicio de sesión exitoso. Cargando...");

    QTimer::singleShot(1000, this, []() {
        navigate_to("home");
    });
}
