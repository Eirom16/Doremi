#include "login_dialog.h"
#include "ffi_utils.h"
#include "design_tokens.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QHBoxLayout>
#include <QUrl>
#include <QGraphicsDropShadowEffect>
#include <QFrame>
#include "doremi/src/bridge.rs.h"

WebLoginDialog::WebLoginDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(tr_q("login_title"));
    setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint | Qt::NoDropShadowWindowHint);
    setAttribute(Qt::WA_TranslucentBackground);
    setModal(true);
    resize(1000, 700);

    layout_ = new QVBoxLayout(this);
    layout_->setContentsMargins(16, 16, 16, 16); // Margins for shadow

    auto *panel = new QFrame(this);
    panel->setObjectName("loginPanel");
    panel->setStyleSheet("QFrame#loginPanel { background-color: #18181a; border-radius: 12px; border: 1px solid #2a2a2a; }");
    
    auto *shadow = new QGraphicsDropShadowEffect(panel);
    shadow->setBlurRadius(32);
    shadow->setOffset(0, 8);
    shadow->setColor(QColor(0, 0, 0, 128));
    panel->setGraphicsEffect(shadow);

    auto *panel_layout = new QVBoxLayout(panel);
    panel_layout->setContentsMargins(0, 0, 0, 0);
    panel_layout->setSpacing(0);

    // Header bar
    auto *header = new QWidget(panel);
    header->setFixedHeight(48);
    header->setObjectName("loginHeader");
    header->setStyleSheet("QWidget#loginHeader { background-color: #18181a; border-top-left-radius: 12px; border-top-right-radius: 12px; }");

    auto *header_layout = new QHBoxLayout(header);
    header_layout->setContentsMargins(12, 0, 8, 0);

    auto *hint = new QLabel(tr_q("login_hint"), header);
    hint->setFont(DesignTokens::getFont("body", 13));
    hint->setStyleSheet("color: #b3b3b3;");
    hint_ = hint;
    header_layout->addWidget(hint);
    header_layout->addStretch();

    btn_close_ = new QPushButton("close", header);
    btn_close_->setFont(QFont("Material Symbols Rounded", 20));
    btn_close_->setFixedSize(32, 32);
    btn_close_->setCursor(Qt::PointingHandCursor);
    btn_close_->setStyleSheet("QPushButton { color: #b3b3b3; background: transparent; border: none; border-radius: 16px; } QPushButton:hover { background: #33ffffff; }");
    btn_close_->setAutoDefault(false);
    btn_close_->setDefault(false);
    btn_close_->setFocusPolicy(Qt::NoFocus);

    connect(btn_close_, &QPushButton::clicked, this, &QDialog::reject);
    header_layout->addWidget(btn_close_);

    panel_layout->addWidget(header);

    // Separator line
    auto *sep = new QFrame(panel);
    sep->setFixedHeight(1);
    sep->setStyleSheet("background-color: #2a2a2a;");
    panel_layout->addWidget(sep);

    // Web View Setup
    view_ = new QWebEngineView(panel);
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
    connect(view_, &QWebEngineView::urlChanged, this, &WebLoginDialog::on_url_changed);

    panel_layout->addWidget(view_);
    
    layout_->addWidget(panel);

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
    // Normalise a leading dot ("\.youtube.com" -> "youtube.com") so we can match
    // the registrable domain exactly instead of an unanchored substring.
    QString bare = domain.startsWith('.') ? domain.mid(1) : domain;
    bare = bare.toLower();

    bool trusted = bare == "youtube.com" || bare.endsWith(".youtube.com");
    if (!trusted) {
        return;
    }

    std::string name = cookie.name().toStdString();
    std::string value = cookie.value().toStdString();
    if (name.empty() || value.empty()) {
        return;
    }
    cookies_[name] = value;
}

void WebLoginDialog::on_load_finished(bool ok) {
    if (ok) {
        poll_login();
    }
}

void WebLoginDialog::poll_login() {
    if (login_detected_) return;

    // Bound the login session: after 5 minutes without a detected sign-in we
    // treat it as expired rather than polling forever.
    elapsed_secs_ += 2;
    if (elapsed_secs_ >= 300) {
        poll_timer_->stop();
        set_status(tr_q("login_status_expired"));
        return;
    }

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

    bool has_session = (cookies_.count("SAPISID") || 
                        cookies_.count("__Secure-3PAPISID") || 
                        cookies_.count("__Secure-1PAPISID")) &&
                       (cookies_.count("SID") || 
                        cookies_.count("__Secure-3PSID"));

    if (has_session) {
        login_detected_ = true;
        poll_timer_->stop();
        QTimer::singleShot(1500, this, [this, avatar_url, user_name]() {
            save_cookies_and_close(avatar_url, user_name);
        });
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
        Ffi::to_std_string(headers_json),
        Ffi::to_std_string(user_name),
        Ffi::to_std_string(avatar_url)
    );

    emit login_successful(avatar_url, user_name);
    accept();
}

void WebLoginDialog::on_url_changed(const QUrl &url) {
    QString host = url.host().toLower();
    if (host.isEmpty()) return;

    if (!is_trusted_host(host)) {
        // Confine the login flow to Google/YouTube; bounce anything else back.
        view_->load(QUrl("https://music.youtube.com"));
        return;
    }

    // Surface an account challenge (2FA, captcha, consent, recovery) so the
    // user understands why sign-in has not completed yet.
    QString path = url.path().toLower();
    bool is_challenge = host.startsWith("accounts.google") &&
                        (path.contains("challenge") || path.contains("signin/v2") ||
                         path.contains("/challenge/") || path.contains("consent"));
    if (is_challenge && !challenge_seen_) {
        challenge_seen_ = true;
        set_status(tr_q("login_status_challenge"));
    }
}

bool WebLoginDialog::is_trusted_host(const QString &host) {
    return host == "youtube.com" || host.endsWith(".youtube.com") ||
           host == "google.com" || host.endsWith(".google.com") ||
           host == "accounts.google" || host == "music.youtube";
}

void WebLoginDialog::set_status(const QString &message) {
    if (hint_) {
        hint_->setText(message);
    }
}
