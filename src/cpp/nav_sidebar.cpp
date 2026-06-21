#include <QSpacerItem>
#include <QHBoxLayout>
#include <QLabel>
#include "nav_sidebar.h"
#include "design_tokens.h"
#include "icon_provider.h"
#include "doremi/src/bridge.rs.h"

struct RouteInfo {
    const char *route;
    const char *title;
    const char *icon;
};

static const RouteInfo ROUTES[] = {
    {"home", "Inicio", "home"},
    {"trending", "Tendencias", "trending_up"},
    {"library", "Biblioteca", "library_music"},
    {"history", "Historial", "history"},
    {"downloads", "Descargas", "download"},
    {"stats", "Estadísticas", "bar_chart"},
    {"settings", "Configuración", "settings"}
};

NavSidebar::NavSidebar(QWidget *parent)
    : QWidget(parent)
{
    const auto &c = DesignTokens::current();
    
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 16, 0, 16);
    layout->setSpacing(4);

    for (const auto &routeInfo : ROUTES) {
        auto *btn = new QPushButton(this);
        btn->setCheckable(true);
        btn->setFixedHeight(44);
        btn->setCursor(Qt::PointingHandCursor);
        btn->setFocusPolicy(Qt::StrongFocus);
        DesignTokens::applyAccessible(
            btn,
            QString("Ir a %1").arg(routeInfo.title),
            QString("Abre la seccion %1").arg(routeInfo.title),
            routeInfo.title);
        
        // Horizontal layout inside button to position icon and text nicely
        auto *btn_layout = new QHBoxLayout(btn);
        btn_layout->setContentsMargins(20, 0, 16, 0);
        btn_layout->setSpacing(12);
        
        auto *icon_label = IconProvider::createIconLabel(routeInfo.icon, 20, c.text_secondary, true, btn);
        icon_label->setObjectName("nav_icon");
        auto *text_label = new QLabel(routeInfo.title, btn);
        text_label->setObjectName("nav_text");
        text_label->setFont(DesignTokens::getFont("body", 13));
        text_label->setStyleSheet(QString("color: %1; background: transparent;").arg(c.text_secondary.name()));
        
        btn_layout->addWidget(icon_label);
        btn_layout->addWidget(text_label);
        btn_layout->addStretch();
        
        btn->setLayout(btn_layout);
        
        btn->setStyleSheet(DesignTokens::navButtonStyle());
        
        layout->addWidget(btn);
        buttons_.push_back({routeInfo.route, btn});

        connect(btn, &QPushButton::clicked, this, [this, routeInfo]() {
            on_button_clicked(routeInfo.route);
        });
    }

    layout->addStretch(1);

    // Profile button at the bottom
    profile_btn_ = new QPushButton(this);
    profile_btn_->setFixedHeight(44);
    profile_btn_->setCursor(Qt::PointingHandCursor);
    profile_btn_->setFocusPolicy(Qt::StrongFocus);
    
    profile_btn_->setStyleSheet(DesignTokens::profileButtonStyle());
    DesignTokens::applyAccessible(
        profile_btn_,
        "Cuenta de usuario",
        "Abre la pantalla de inicio de sesion o el menu de cuenta.",
        "Cuenta");
    connect(profile_btn_, &QPushButton::clicked, this, &NavSidebar::on_profile_clicked);
    layout->addWidget(profile_btn_);

    setFixedWidth(210);
    
    // Set sidebar base background
    setAttribute(Qt::WA_StyledBackground, true);
    setStyleSheet(QString("NavSidebar { background-color: %1; border-right: 1px solid %2; }")
        .arg(c.bg_surface.name())
        .arg(DesignTokens::rgba(c.border))
    );

    // Initial auth UI state
    update_profile(false, "", "");
}

void NavSidebar::set_active_route(const std::string &route) {
    const auto &c = DesignTokens::current();
    for (auto &nb : buttons_) {
        bool active = (nb.route == route);
        nb.btn->setChecked(active);
        
        // Find icon label and text label to update their style/color
        auto labels = nb.btn->findChildren<QLabel*>();
        for (auto *label : labels) {
            if (label->objectName() == "nav_icon") {
                IconProvider::setupIconLabel(label, label->text(), 20, active ? c.accent : c.text_secondary, true);
            } else if (label->objectName() == "nav_text") {
                label->setStyleSheet(QString("color: %1; background: transparent; font-weight: %2;")
                    .arg(active ? c.text_primary.name() : c.text_secondary.name())
                    .arg(active ? "600" : "500"));
            }
        }
    }
}

void NavSidebar::set_compact(bool compact) {
    if (compact_ == compact) {
        return;
    }
    compact_ = compact;
    setFixedWidth(compact_ ? 76 : 210);

    for (auto &nb : buttons_) {
        if (auto *layout = qobject_cast<QHBoxLayout *>(nb.btn->layout())) {
            layout->setContentsMargins(compact_ ? 22 : 20, 0, compact_ ? 0 : 16, 0);
            layout->setSpacing(compact_ ? 0 : 12);
        }
        for (auto *label : nb.btn->findChildren<QLabel *>()) {
            if (label->objectName() == "nav_text") {
                label->setVisible(!compact_);
            }
        }
    }

    update_profile(authenticated_, user_name_, avatar_url_);
}

void NavSidebar::on_button_clicked(const std::string &route) {
    set_active_route(route);
    emit route_changed(route);
}

void NavSidebar::update_theme() {
    const auto &c = DesignTokens::current();
    
    setStyleSheet(QString("NavSidebar { background-color: %1; border-right: 1px solid %2; }")
        .arg(c.bg_surface.name())
        .arg(DesignTokens::rgba(c.border))
    );
    
    for (auto &nb : buttons_) {
        nb.btn->setStyleSheet(DesignTokens::navButtonStyle());
    }
    
    profile_btn_->setStyleSheet(DesignTokens::profileButtonStyle());
    
    for (auto &nb : buttons_) {
        if (nb.btn->isChecked()) {
            set_active_route(nb.route);
            break;
        }
    }
    const bool was_compact = compact_;
    compact_ = !was_compact;
    set_compact(was_compact);

    update_profile(authenticated_, user_name_, avatar_url_);
}

void NavSidebar::update_profile(bool authenticated, const std::string &name, const std::string &avatar_url) {
    authenticated_ = authenticated;
    user_name_ = name;
    avatar_url_ = avatar_url;

    const auto &c = DesignTokens::current();
    if (authenticated) {
        profile_btn_->setText(compact_ ? "" : QString::fromStdString(" " + name));
        profile_btn_->setIcon(IconProvider::getIcon("account_circle", c.accent, 20));
        DesignTokens::applyAccessible(
            profile_btn_,
            QString("Cuenta de %1").arg(QString::fromStdString(name)),
            "Abre el menu de cuenta.",
            "Cuenta");
    } else {
        profile_btn_->setText(compact_ ? "" : " Iniciar sesión");
        profile_btn_->setIcon(IconProvider::getIcon("login", c.text_secondary, 20));
        DesignTokens::applyAccessible(
            profile_btn_,
            "Iniciar sesion",
            "Abre la pantalla para iniciar sesion.",
            "Iniciar sesion");
    }
}

void NavSidebar::on_profile_clicked() {
    if (authenticated_) {
        QMenu menu(this);
        QAction *logout_action = menu.addAction("Cerrar sesión");
        connect(logout_action, &QAction::triggered, this, []() {
            on_youtube_logout();
        });
        menu.exec(profile_btn_->mapToGlobal(QPoint(0, -menu.sizeHint().height())));
    } else {
        emit route_changed("welcome");
    }
}
