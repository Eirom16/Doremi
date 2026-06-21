#ifndef SESSION_COOKIE_MANAGER_H
#define SESSION_COOKIE_MANAGER_H

#include <QObject>
#include <QTimer>
#include <QNetworkCookie>
#include <map>
#include <string>

class DoremiMainWindow;

class SessionCookieManager : public QObject {
    Q_OBJECT
public:
    explicit SessionCookieManager(DoremiMainWindow *window);
    void setup_session_cookie_refresh();
private slots:
    void persist_session_cookies();
private:
    void update_session_cookie(const QNetworkCookie &cookie, bool removed);

    DoremiMainWindow *window_;
    QTimer *session_cookie_timer_ = nullptr;
    std::map<std::string, std::string> session_cookies_;
};

#endif // SESSION_COOKIE_MANAGER_H
