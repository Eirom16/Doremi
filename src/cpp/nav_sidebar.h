#ifndef DOREMI_NAV_SIDEBAR_H
#define DOREMI_NAV_SIDEBAR_H

#include <QWidget>
#include <QQuickWidget>
#include <QString>
#include <string>

class NavSidebar : public QWidget {
    Q_OBJECT
    Q_PROPERTY(QString activeRoute READ activeRoute NOTIFY activeRouteChanged)
    Q_PROPERTY(bool isCompact READ isCompact NOTIFY isCompactChanged)
    Q_PROPERTY(bool isAuthenticated READ isAuthenticated NOTIFY isAuthenticatedChanged)
    Q_PROPERTY(QString userName READ userName NOTIFY userNameChanged)
    Q_PROPERTY(QString avatarUrl READ avatarUrl NOTIFY avatarUrlChanged)

public:
    explicit NavSidebar(QWidget *parent = nullptr);

    // C++ setters
    void set_active_route(const std::string &route);
    void set_compact(bool compact);
    void update_profile(bool authenticated, const std::string &name, const std::string &avatar_url);
    
    // Stub for theme updates
    void update_theme() {}

    // Getters for Q_PROPERTY
    QString activeRoute() const { return active_route_; }
    bool isCompact() const { return compact_; }
    bool isAuthenticated() const { return authenticated_; }
    QString userName() const { return user_name_; }
    QString avatarUrl() const { return avatar_url_; }

signals:
    // Signal emitted to C++ when a button is clicked
    void route_changed(const std::string &route);

    // Q_PROPERTY notify signals
    void activeRouteChanged();
    void isCompactChanged();
    void isAuthenticatedChanged();
    void userNameChanged();
    void avatarUrlChanged();

public slots:
    // Invokable from QML
    void navigate(const QString &route) {
        emit route_changed(route.toStdString());
    }
    void logout();

protected:
    void resizeEvent(QResizeEvent *event) override;

private:
    QQuickWidget *quick_widget_;

    QString active_route_ = "home";
    bool compact_ = false;
    bool authenticated_ = false;
    QString user_name_ = "";
    QString avatar_url_ = "";
};

#endif // DOREMI_NAV_SIDEBAR_H
