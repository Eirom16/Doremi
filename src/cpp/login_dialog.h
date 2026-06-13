#ifndef DOREMI_LOGIN_DIALOG_H
#define DOREMI_LOGIN_DIALOG_H

#include <QDialog>
#include <QWebEngineView>
#include <QWebEnginePage>
#include <QWebEngineScriptCollection>
#include <QWebEngineProfile>
#include <QWebEngineCookieStore>
#include <QWebEngineScript>
#include <QNetworkCookie>
#include <QTimer>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <map>
#include <string>

class WebLoginDialog : public QDialog {
    Q_OBJECT
public:
    explicit WebLoginDialog(QWidget *parent = nullptr);
    ~WebLoginDialog() override;

signals:
    void login_successful(const QString &avatar_url, const QString &user_name);

private slots:
    void on_cookie_added(const QNetworkCookie &cookie);
    void on_load_finished(bool ok);
    void poll_login();
    void on_login_result(const QString &result_str);

private:
    void save_cookies_and_close(const QString &avatar_url, const QString &user_name);

    QVBoxLayout *layout_;
    QPushButton *btn_close_;
    QWebEngineView *view_;
    QWebEngineProfile *profile_;
    QWebEngineCookieStore *cookie_store_;
    QTimer *poll_timer_;
    std::map<std::string, std::string> cookies_;
    bool login_detected_ = false;
};

#endif // DOREMI_LOGIN_DIALOG_H
