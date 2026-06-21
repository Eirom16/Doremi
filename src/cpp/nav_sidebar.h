#ifndef DOREMI_NAV_SIDEBAR_H
#define DOREMI_NAV_SIDEBAR_H

#include <QWidget>
#include <QVBoxLayout>
#include <QPushButton>
#include <QMenu>
#include <QAction>
#include <vector>
#include <string>

class NavSidebar : public QWidget {
    Q_OBJECT
public:
    explicit NavSidebar(QWidget *parent = nullptr);
    void set_active_route(const std::string &route);
    void set_compact(bool compact);
    void update_theme();
    void update_profile(bool authenticated, const std::string &name, const std::string &avatar_url);

signals:
    void route_changed(const std::string &route);

private slots:
    void on_profile_clicked();

private:
    struct NavButton {
        std::string route;
        QPushButton *btn;
    };
    std::vector<NavButton> buttons_;
    QPushButton *profile_btn_ = nullptr;
    bool authenticated_ = false;
    bool compact_ = false;
    std::string user_name_;
    std::string avatar_url_;

    void on_button_clicked(const std::string &route);
};

#endif // DOREMI_NAV_SIDEBAR_H
