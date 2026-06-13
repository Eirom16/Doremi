#include "login_dialog.h"
#include "design_tokens.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QHBoxLayout>
#include <QUrl>
#include "doremi/src/bridge.rs.h"

WebLoginDialog::WebLoginDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(QString::fromStdString(std::string(doremi_tr("login_title"))));
    resize(1000, 700);

    const auto &c = DesignTokens::current();
    setStyleSheet(QString("background-color: %1;").arg(c.bg_base.name()));

    layout_ = new QVBoxLayout(this);
    layout_->setContentsMargins(0, 0, 0, 0);
    layout_->setSpacing(0);

    // Header bar
    auto *header = new QWidget(this);
    header->setFixedHeight(32);
    header->setStyleSheet(QString(
        "background-color: %1; border-bottom: 1px solid %2;"
    ).arg(c.bg_elevated.name()).arg(c.border.name()));

    auto *header_layout = new QHBoxLayout(header);
    header_layout->setContentsMargins(12, 0, 8, 0);

    auto *hint = new QLabel(QString::fromStdString(std::string(doremi_tr("login_hint"))), header);
    hint->setFont(DesignTokens::getFont("body", 11));
    hint->setStyleSheet(QString("color: %1; background: transparent;").arg(c.text_muted.name()));
    header_layout->addWidget(hint);
    header_layout->addStretch();

    btn_close_ = new QPushButton("✕", header);
    btn_close_->setFixedSize(24, 24);
    btn_close_->setCursor(Qt::PointingHandCursor);
    btn_close_->setAutoDefault(false);
    btn_close_->setDefault(false);
    btn_close_->setFocusPolicy(Qt::NoFocus);
    btn_close_->setStyleSheet(QString(
        "QPushButton {"
        "    background-color: transparent;"
        "    color: %1;"
        "    border: none;"
        "    border-radius: 12px;"
        "    font-size: 14px;"
        "    font-weight: bold;"
        "}"
        "QPushButton:hover {"
        "    color: %2;"
        "    background-color: rgba(239, 68, 68, 0.1);"
        "}"
    ).arg(c.text_secondary.name()).arg(c.error.name()));

    connect(btn_close_, &QPushButton::clicked, this, &QDialog::reject);
    header_layout->addWidget(btn_close_);

    layout_->addWidget(header);

    // Web View Setup
    view_ = new QWebEngineView(this);
    profile_ = QWebEngineProfile::defaultProfile();
    cookie_store_ = profile_->cookieStore();

    // Inject storage access script
    QWebEngineScript storage_script;
    storage_script.setName("pyrolist_storage_access");
    storage_script.setInjectionPoint(QWebEngineScript::DocumentCreation);
    storage_script.setWorldId(QWebEngineScript::MainWorld);
    storage_script.setRunsOnSubFrames(true);
    storage_script.setSourceCode(
        "Document.prototype.requestStorageAccess = function() { return Promise.resolve(); };"
        "Document.prototype.requestStorageAccessFor = function() { return Promise.resolve(); };"
    );
    view_->page()->scripts().insert(storage_script);

    connect(cookie_store_, &QWebEngineCookieStore::cookieAdded, this, &WebLoginDialog::on_cookie_added);
    connect(view_, &QWebEngineView::loadFinished, this, &WebLoginDialog::on_load_finished);

    layout_->addWidget(view_);

    view_->load(QUrl("https://music.youtube.com"));

    // Poll every 2 seconds
    poll_timer_ = new QTimer(this);
    poll_timer_->setInterval(2000);
    connect(poll_timer_, &QTimer::timeout, this, &WebLoginDialog::poll_login);
    poll_timer_->start();
}

WebLoginDialog::~WebLoginDialog() {
    poll_timer_->stop();
}

void WebLoginDialog::on_cookie_added(const QNetworkCookie &cookie) {
    QString domain = cookie.domain();
    if (domain.contains("youtube") || domain.contains("google")) {
        std::string name = cookie.name().toStdString();
        std::string value = cookie.value().toStdString();
        cookies_[name] = value;
    }
}

void WebLoginDialog::on_load_finished(bool ok) {
    if (ok) {
        poll_login();
    }
}

void WebLoginDialog::poll_login() {
    if (login_detected_) return;

    QString js = R"(
        (() => {
            let img = document.querySelector('ytmusic-settings-button img') ||
                      document.querySelector('#avatar-btn img');
            if (!img || !img.src) return JSON.stringify({avatar: '', name: ''});
            let src = img.src;
            if (src.startsWith('data:') || !src.startsWith('http')) return JSON.stringify({avatar: '', name: ''});
            let name = img.alt || '';
            return JSON.stringify({avatar: src, name: name});
        })()
    )";

    view_->page()->runJavaScript(js, [this](const QVariant &res) {
        on_login_result(res.toString());
    });
}

void WebLoginDialog::on_login_result(const QString &result_str) {
    if (login_detected_) return;

    QJsonDocument doc = QJsonDocument::fromJson(result_str.toUtf8());
    if (doc.isNull() || !doc.isObject()) return;

    QJsonObject obj = doc.object();
    QString avatar_url = obj.value("avatar").toString();
    QString user_name = obj.value("name").toString();

    if (avatar_url.isEmpty()) return;

    bool has_session = cookies_.count("SAPISID") || 
                       cookies_.count("__Secure-3PAPISID") || 
                       cookies_.count("__Secure-1PAPISID");

    if (has_session) {
        login_detected_ = true;
        poll_timer_->stop();
        save_cookies_and_close(avatar_url, user_name);
    }
}

void WebLoginDialog::save_cookies_and_close(const QString &avatar_url, const QString &user_name) {
    QJsonObject headers;
    QString cookie_str;
    for (const auto &pair : cookies_) {
        if (!cookie_str.isEmpty()) cookie_str += "; ";
        cookie_str += QString::fromStdString(pair.first + "=" + pair.second);
    }
    headers.insert("cookie", cookie_str);
    headers.insert("x-goog-authuser", "0");
    headers.insert("user-agent", "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/120.0.0.0 Safari/537.36");
    headers.insert("accept", "*/*");
    headers.insert("origin", "https://music.youtube.com");
    headers.insert("x-origin", "https://music.youtube.com");
    headers.insert("authorization", "SAPISIDHASH 1");

    QJsonDocument doc(headers);
    QString headers_json = doc.toJson(QJsonDocument::Compact);

    // Call Rust bridge function
    on_youtube_login_success(
        headers_json.toStdString(),
        user_name.toStdString(),
        avatar_url.toStdString()
    );

    emit login_successful(avatar_url, user_name);
    accept();
}
