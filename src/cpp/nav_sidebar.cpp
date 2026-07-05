#include "nav_sidebar.h"
#include <QVBoxLayout>
#include <QQmlContext>
#include "doremi/src/bridge.rs.h"

NavSidebar::NavSidebar(QWidget *parent) : QWidget(parent) {
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    
    // Default size is 220, compact is 72, managed by main_window.cpp
    
    quick_widget_ = new QQuickWidget(this);
    quick_widget_->setResizeMode(QQuickWidget::SizeRootObjectToView);
    quick_widget_->setAttribute(Qt::WA_TranslucentBackground);
    quick_widget_->setClearColor(Qt::transparent);

    quick_widget_->rootContext()->setContextProperty("SidebarCtrl", this);
    quick_widget_->setSource(QUrl("qrc:/qml/Sidebar.qml"));

    layout->addWidget(quick_widget_);
}

void NavSidebar::resizeEvent(QResizeEvent *event) {
    QWidget::resizeEvent(event);
    if (quick_widget_) {
        quick_widget_->resize(size());
    }
}

void NavSidebar::set_active_route(const std::string &route) {
    QString qRoute = QString::fromStdString(route);
    if (active_route_ != qRoute) {
        active_route_ = qRoute;
        emit activeRouteChanged();
    }
}

void NavSidebar::set_compact(bool compact) {
    if (compact_ != compact) {
        compact_ = compact;
        emit isCompactChanged();
    }
}

void NavSidebar::update_profile(bool authenticated, const std::string &name, const std::string &avatar_url) {
    QString qName = QString::fromStdString(name);
    QString qAvatar = QString::fromStdString(avatar_url);
    
    if (authenticated_ != authenticated) {
        authenticated_ = authenticated;
        emit isAuthenticatedChanged();
    }
    if (user_name_ != qName) {
        user_name_ = qName;
        emit userNameChanged();
    }
    if (avatar_url_ != qAvatar) {
        avatar_url_ = qAvatar;
        emit avatarUrlChanged();
    }
}

void NavSidebar::logout() {
    on_youtube_logout();
}

