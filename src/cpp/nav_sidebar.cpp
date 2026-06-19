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
        
        // Horizontal layout inside button to position icon and text nicely
        auto *btn_layout = new QHBoxLayout(btn);
        btn_layout->setContentsMargins(20, 0, 16, 0);
        btn_layout->setSpacing(12);
        
        auto *icon_label = IconProvider::createIconLabel(routeInfo.icon, 20, c.text_secondary, true, btn);
        auto *text_label = new QLabel(routeInfo.title, btn);
        text_label->setFont(DesignTokens::getFont("body", 13));
        text_label->setStyleSheet("color: inherit; background: transparent;");
        
        btn_layout->addWidget(icon_label);
        btn_layout->addWidget(text_label);
        btn_layout->addStretch();
        
        btn->setLayout(btn_layout);
        
        // Styling with tokens
        QString style = QString(
            "QPushButton {\n"
            "    background: transparent;\n"
            "    border: none;\n"
            "    border-radius: 0px;\n"
            "    border-left: 3px solid transparent;\n"
            "    color: %1;\n"
            "}\n"
            "QPushButton:hover {\n"
            "    background: %2;\n"
            "    color: %3;\n"
            "}\n"
            "QPushButton:checked {\n"
            "    background: %4;\n"
            "    color: %5;\n"
            "    border-left: 3px solid %5;\n"
            "}\n"
        )
        .arg(c.text_secondary.name())
        .arg(QString("rgba(%1, %2, %3, %4)").arg(c.accent_dim.red()).arg(c.accent_dim.green()).arg(c.accent_dim.blue()).arg(c.accent_dim.alpha() / 255.0))
        .arg(c.text_primary.name())
        .arg(QString("rgba(%1, %2, %3, %4)").arg(c.accent_dim.red()).arg(c.accent_dim.green()).arg(c.accent_dim.blue()).arg(c.accent_dim.alpha() / 255.0))
        .arg(c.accent.name());
        
        btn->setStyleSheet(style);
        
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
    
    // Style profile button nicely
    QString profile_style = QString(
        "QPushButton {\n"
        "    background: transparent;\n"
        "    border: none;\n"
        "    border-radius: 12px;\n"
        "    text-align: left;\n"
        "    padding-left: 20px;\n"
        "    margin: 4px 8px;\n"
        "    font-weight: bold;\n"
        "    color: %1;\n"
        "}\n"
        "QPushButton:hover {\n"
        "    background: %2;\n"
        "    color: %3;\n"
        "}\n"
    )
    .arg(c.text_secondary.name())
    .arg(QString("rgba(%1, %2, %3, %4)").arg(c.accent_dim.red()).arg(c.accent_dim.green()).arg(c.accent_dim.blue()).arg(c.accent_dim.alpha() / 255.0))
    .arg(c.text_primary.name());
    
    profile_btn_->setStyleSheet(profile_style);
    connect(profile_btn_, &QPushButton::clicked, this, &NavSidebar::on_profile_clicked);
    layout->addWidget(profile_btn_);

    setFixedWidth(210);
    
    // Set sidebar base background
    setAttribute(Qt::WA_StyledBackground, true);
    setStyleSheet(QString("NavSidebar { background-color: %1; border-right: 1px solid %2; }")
        .arg(c.bg_surface.name())
        .arg(QString("rgba(%1, %2, %3, %4)").arg(c.border.red()).arg(c.border.green()).arg(c.border.blue()).arg(c.border.alpha() / 255.0))
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
            if (label->font().family() == "Material Symbols Rounded") {
                IconProvider::setupIconLabel(label, label->text(), 20, active ? c.accent : c.text_secondary, true);
            }
        }
    }
}

void NavSidebar::on_button_clicked(const std::string &route) {
    set_active_route(route);
    emit route_changed(route);
}

void NavSidebar::update_theme() {
    const auto &c = DesignTokens::current();
    
    setStyleSheet(QString("NavSidebar { background-color: %1; border-right: 1px solid %2; }")
        .arg(c.bg_surface.name())
        .arg(QString("rgba(%1, %2, %3, %4)").arg(c.border.red()).arg(c.border.green()).arg(c.border.blue()).arg(c.border.alpha() / 255.0))
    );
    
    for (auto &nb : buttons_) {
        QString style = QString(
            "QPushButton {\n"
            "    background: transparent;\n"
            "    border: none;\n"
            "    border-radius: 0px;\n"
            "    border-left: 3px solid transparent;\n"
            "    color: %1;\n"
            "}\n"
            "QPushButton:hover {\n"
            "    background: %2;\n"
            "    color: %3;\n"
            "}\n"
            "QPushButton:checked {\n"
            "    background: %4;\n"
            "    color: %5;\n"
            "    border-left: 3px solid %5;\n"
            "}\n"
        )
        .arg(c.text_secondary.name())
        .arg(QString("rgba(%1, %2, %3, %4)").arg(c.accent_dim.red()).arg(c.accent_dim.green()).arg(c.accent_dim.blue()).arg(c.accent_dim.alpha() / 255.0))
        .arg(c.text_primary.name())
        .arg(QString("rgba(%1, %2, %3, %4)").arg(c.accent_dim.red()).arg(c.accent_dim.green()).arg(c.accent_dim.blue()).arg(c.accent_dim.alpha() / 255.0))
        .arg(c.accent.name());
        
        nb.btn->setStyleSheet(style);
    }
    
    QString profile_style = QString(
        "QPushButton {\n"
        "    background: transparent;\n"
        "    border: none;\n"
        "    border-radius: 12px;\n"
        "    text-align: left;\n"
        "    padding-left: 20px;\n"
        "    margin: 4px 8px;\n"
        "    font-weight: bold;\n"
        "    color: %1;\n"
        "}\n"
        "QPushButton:hover {\n"
        "    background: %2;\n"
        "    color: %3;\n"
        "}\n"
    )
    .arg(c.text_secondary.name())
    .arg(QString("rgba(%1, %2, %3, %4)").arg(c.accent_dim.red()).arg(c.accent_dim.green()).arg(c.accent_dim.blue()).arg(c.accent_dim.alpha() / 255.0))
    .arg(c.text_primary.name());
    
    profile_btn_->setStyleSheet(profile_style);
    
    for (auto &nb : buttons_) {
        if (nb.btn->isChecked()) {
            set_active_route(nb.route);
            break;
        }
    }

    update_profile(authenticated_, user_name_, avatar_url_);
}

void NavSidebar::update_profile(bool authenticated, const std::string &name, const std::string &avatar_url) {
    authenticated_ = authenticated;
    user_name_ = name;
    avatar_url_ = avatar_url;

    const auto &c = DesignTokens::current();
    if (authenticated) {
        profile_btn_->setText(QString::fromStdString(" " + name));
        profile_btn_->setIcon(IconProvider::getIcon("account_circle", c.accent, 20));
    } else {
        profile_btn_->setText(" Iniciar sesión");
        profile_btn_->setIcon(IconProvider::getIcon("login", c.text_secondary, 20));
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
