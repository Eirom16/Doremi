#include "session_cookie_manager.h"
#include "main_window.h"
#include <QWebEngineProfile>
#include <QWebEngineCookieStore>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDateTime>

SessionCookieManager::SessionCookieManager(DoremiMainWindow *window)
    : QObject(window), window_(window) {}

void SessionCookieManager::setup_session_cookie_refresh() {
    auto *profile = QWebEngineProfile::defaultProfile();
    if (!profile) return;
    auto *store = profile->cookieStore();
    if (!store) return;

    session_cookie_timer_ = new QTimer(this);
    session_cookie_timer_->setSingleShot(true);
    session_cookie_timer_->setInterval(1500);
    connect(session_cookie_timer_, &QTimer::timeout,
            this, &SessionCookieManager::persist_session_cookies);
    connect(store, &QWebEngineCookieStore::cookieAdded, this,
            [this](const QNetworkCookie &cookie) {
                update_session_cookie(cookie, false);
            });
    connect(store, &QWebEngineCookieStore::cookieRemoved, this,
            [this](const QNetworkCookie &cookie) {
                update_session_cookie(cookie, true);
            });
    store->loadAllCookies();
}

void SessionCookieManager::update_session_cookie(const QNetworkCookie &cookie, bool removed) {
    QString domain = cookie.domain().toLower();
    if (domain.startsWith('.')) domain.remove(0, 1);
    const bool trusted = domain == "youtube.com" || domain.endsWith(".youtube.com") ||
                         domain == "google.com" || domain.endsWith(".google.com");
    if (!trusted) return;

    const std::string name = cookie.name().toStdString();
    if (name.empty()) return;
    const bool expired = !cookie.isSessionCookie() &&
                         cookie.expirationDate().isValid() &&
                         cookie.expirationDate() <= QDateTime::currentDateTimeUtc();
    if (removed || expired || cookie.value().isEmpty()) {
        session_cookies_.erase(name);
    } else {
        session_cookies_[name] = cookie.value().toStdString();
    }
    session_cookie_timer_->start();
}

void SessionCookieManager::persist_session_cookies() {
    const bool has_sapisid = session_cookies_.count("SAPISID") ||
                             session_cookies_.count("__Secure-3PAPISID") ||
                             session_cookies_.count("__Secure-1PAPISID");
    if (!has_sapisid) return;

    QString cookie_string;
    for (const auto &[name, value] : session_cookies_) {
        if (!cookie_string.isEmpty()) cookie_string += "; ";
        cookie_string += QString::fromStdString(name + "=" + value);
    }
    QJsonObject headers;
    headers.insert("cookie", cookie_string);
    headers.insert("x-goog-authuser", "0");
    headers.insert("user-agent", "Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 Doremi/2");
    headers.insert("accept", "*/*");
    headers.insert("origin", "https://music.youtube.com");
    headers.insert("x-origin", "https://music.youtube.com");
    const QString json = QJsonDocument(headers).toJson(QJsonDocument::Compact);
    on_youtube_session_refresh(json.toStdString());
}
