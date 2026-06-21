#ifndef NAVIGATION_CONTROLLER_H
#define NAVIGATION_CONTROLLER_H

#include <QObject>
#include <QPointer>
#include <QWidget>
#include <string>
#include <vector>
#include <map>

class DoremiMainWindow;

class NavigationController : public QObject {
    Q_OBJECT
public:
    explicit NavigationController(DoremiMainWindow *window);
    void navigate_to(const std::string &route);
    void navigate_back();
    void navigate_back_from_detail();
    void navigate_forward();
    void save_route_view_state();
    void restore_route_view_state(const std::string &route);

    std::string current_route() const { return current_route_; }
private:
    void navigate_to_internal(const std::string &route, bool record_history);

    DoremiMainWindow *window_;
    std::string current_route_ = "home";
    std::string detail_return_route_ = "home";
    std::vector<std::string> back_routes_;
    std::vector<std::string> forward_routes_;
    std::map<std::string, int> route_scroll_positions_;
    std::map<std::string, QPointer<QWidget>> route_focus_widgets_;
};

#endif // NAVIGATION_CONTROLLER_H
