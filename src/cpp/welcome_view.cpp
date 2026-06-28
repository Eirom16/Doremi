#include "welcome_view.h"
#include "design_tokens.h"
#include "icon_provider.h"
#include "login_dialog.h"
#include <QPixmap>
#include <QDir>
#include <QCoreApplication>
#include <QFile>
#include <QStringList>
#include <QTimer>
#include <QGraphicsColorizeEffect>
#include "doremi/src/bridge.rs.h"

WelcomeView::WelcomeView(QWidget *parent)
    : QWidget(parent)
{
    const auto &c = DesignTokens::current();

    auto *layout = new QVBoxLayout(this);
    layout->setAlignment(Qt::AlignCenter);
    layout->setContentsMargins(80, 40, 80, 40);
    layout->setSpacing(20);

    // Try to load logo.png from packaged or adjacent assets
    QString app_dir = QCoreApplication::applicationDirPath();
    QStringList logo_paths = {
        ":/assets/logo.png",
        app_dir + "/assets/logo.png",
        QDir(app_dir).filePath("../assets/logo.png")
    };
    QString logo_path;
    for (const QString &candidate : logo_paths) {
        if (QFile::exists(candidate)) {
            logo_path = candidate;
            break;
        }
    }

    if (!logo_path.isEmpty()) {
        logo_ = new QLabel(this);
        QPixmap pix(logo_path);
        logo_->setPixmap(pix.scaledToWidth(180, Qt::SmoothTransformation));
        logo_->setAlignment(Qt::AlignCenter);
        
        // Colorize effect for the logo matching accent color
        auto *effect = new QGraphicsColorizeEffect(logo_);
        effect->setColor(c.accent);
        effect->setStrength(1.0);
        logo_->setGraphicsEffect(effect);
        
        layout->addWidget(logo_);
    } else {
        // Fallback logo icon
        logo_ = IconProvider::createIconLabel("album", 80, c.accent, true, this);
        logo_->setAlignment(Qt::AlignCenter);
        layout->addWidget(logo_);
    }

    title_ = new QLabel("Doremi", this);
    title_->setFont(DesignTokens::getFont("heading_lg"));
    title_->setStyleSheet(QString("color: %1; background: transparent; font-weight: bold;").arg(c.accent.name()));
    title_->setAlignment(Qt::AlignCenter);
    layout->addWidget(title_);

    subtitle_ = new QLabel(tr_q("welcome_subtitle"), this);
    subtitle_->setFont(DesignTokens::getFont("body", 14));
    subtitle_->setStyleSheet(QString("color: %1; background: transparent;").arg(c.text_secondary.name()));
    subtitle_->setAlignment(Qt::AlignCenter);
    layout->addWidget(subtitle_);

    layout->addSpacing(20);

    // Card Glass Panel container
    card_ = new GlassPanel(this);
    card_->setObjectName("welcomeCard");
    card_->setMaximumWidth(380);
    card_->setMinimumWidth(340);
    card_->setMaximumHeight(300);
    auto *card_layout = new QVBoxLayout(card_);
    card_layout->setSpacing(16);
    card_layout->setContentsMargins(32, 28, 32, 28);
    card_layout->setAlignment(Qt::AlignCenter);

    welcome_text_ = new QLabel(tr_q("welcome_title"), card_);
    welcome_text_->setFont(DesignTokens::getFont("heading_lg", 20));
    welcome_text_->setStyleSheet(QString("color: %1; background: transparent; font-weight: bold;").arg(c.text_primary.name()));
    welcome_text_->setAlignment(Qt::AlignCenter);
    card_layout->addWidget(welcome_text_);

    desc_text_ = new QLabel(tr_q("welcome_desc"), card_);
    desc_text_->setFont(DesignTokens::getFont("body_sm"));
    desc_text_->setStyleSheet(QString("color: %1; background: transparent;").arg(c.text_secondary.name()));
    desc_text_->setAlignment(Qt::AlignCenter);
    card_layout->addWidget(desc_text_);

    login_btn_ = new RippleButton(tr_q("welcome_btn_login"), card_, RippleButton::Variant::Primary);
    login_btn_->setMinimumHeight(50);
    login_btn_->setMinimumWidth(240);
    connect(login_btn_, &QPushButton::clicked, this, &WelcomeView::on_login_clicked);
    card_layout->addWidget(login_btn_);

    status_label_ = new QLabel("", card_);
    status_label_->setFont(DesignTokens::getFont("body_sm"));
    status_label_->setStyleSheet(QString("color: %1; background: transparent;").arg(c.text_secondary.name()));
    status_label_->setAlignment(Qt::AlignCenter);
    status_label_->setVisible(false);
    card_layout->addWidget(status_label_);

    progress_ = new QLabel("", card_);
    progress_->setFont(DesignTokens::getFont("body", 12));
    progress_->setStyleSheet(QString("color: %1; background: transparent;").arg(c.text_muted.name()));
    progress_->setAlignment(Qt::AlignCenter);
    progress_->setVisible(false);
    card_layout->addWidget(progress_);

    layout->addWidget(card_);
    layout->addStretch();

    setStyleSheet("background: transparent;");
    update_theme();
}

void WelcomeView::update_theme() {
    const auto &c = DesignTokens::current();
    title_->setStyleSheet(QString("color: %1; background: transparent; font-weight: bold;").arg(c.accent.name()));
    subtitle_->setStyleSheet(QString("color: %1; background: transparent;").arg(c.text_secondary.name()));
    
    if (logo_) {
        auto *effect = qobject_cast<QGraphicsColorizeEffect*>(logo_->graphicsEffect());
        if (effect) {
            effect->setColor(c.accent);
        } else {
            // IconProvider label fallback color update
            logo_->setStyleSheet(QString("color: %1; background: transparent;").arg(c.accent.name()));
        }
    }

    card_->setStyleSheet(
        "GlassPanel#welcomeCard {"
        "    background-color: transparent;"
        "    border: none;"
        "}");

    welcome_text_->setStyleSheet(QString("color: %1; background: transparent; font-weight: bold;").arg(c.text_primary.name()));
    desc_text_->setStyleSheet(QString("color: %1; background: transparent;").arg(c.text_secondary.name()));
    status_label_->setStyleSheet(QString("color: %1; background: transparent;").arg(c.text_secondary.name()));
    progress_->setStyleSheet(QString("color: %1; background: transparent;").arg(c.text_muted.name()));
}

void WelcomeView::on_login_clicked() {
    login_btn_->setEnabled(false);
    login_btn_->setText(tr_q("welcome_btn_logging_in"));
    status_label_->setText(tr_q("welcome_status_opening"));
    status_label_->setVisible(true);
    progress_->clear();
    progress_->setVisible(false);

    auto *dialog = new WebLoginDialog(this);
    connect(dialog, &WebLoginDialog::login_successful, this, &WelcomeView::handle_login_success);
    
    int result = dialog->exec();
    dialog->deleteLater();

    // Check if authentication succeeded, if not reset state
    if (result != QDialog::Accepted) {
        login_btn_->setEnabled(true);
        login_btn_->setText(tr_q("welcome_btn_login"));
        status_label_->setText("");
        status_label_->setVisible(false);
    }
}

void WelcomeView::handle_login_success(const QString &avatar_url, const QString &user_name) {
    Q_UNUSED(avatar_url);
    Q_UNUSED(user_name);
    status_label_->setText(tr_q("welcome_status_success"));
    progress_->setText(tr_q("welcome_status_loading"));
    status_label_->setVisible(true);
    progress_->setVisible(true);
    
    QTimer::singleShot(1000, this, []() {
        // Trigger navigating to home
        navigate_to("home");
    });
}
