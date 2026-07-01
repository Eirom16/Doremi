#ifndef THEME_CONTROLLER_H
#define THEME_CONTROLLER_H

#include <QObject>
#include <string>

class DoremiMainWindow;

class ThemeController : public QObject {
    Q_OBJECT
public:
    explicit ThemeController(DoremiMainWindow *window);
    void apply_theme(const std::string &theme_mode, const std::string &accent_color);
    void refresh_all_views();
private:
    DoremiMainWindow *window_;
};

#endif // THEME_CONTROLLER_H
