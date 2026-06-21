#include "theme_controller.h"
#include "main_window.h"
#include "design_tokens.h"
#include "title_bar.h"
#include "nav_sidebar.h"
#include "player_bar.h"
#include "home_view.h"
#include "components/theme_transition.h"
#include <QPointer>

ThemeController::ThemeController(DoremiMainWindow *window)
    : QObject(window), window_(window) {}

void ThemeController::apply_theme(const std::string &theme_mode, const std::string &accent_color) {
    QPointer<DoremiMainWindow> window_ptr(window_);
    auto apply_fn = [window_ptr, theme_mode, accent_color]() {
        if (!window_ptr) return;
        bool dark = theme_mode != "light";
        DesignTokens::setTheme(dark ? DesignTokens::Theme::Dark : DesignTokens::Theme::Light);
        if (!accent_color.empty()) {
            DesignTokens::setAccentColor(QString::fromStdString(accent_color));
        }
        window_ptr->setStyleSheet(DesignTokens::getGlobalStyleSheet());
        if (window_ptr->title_bar()) window_ptr->title_bar()->update_theme();
        if (window_ptr->nav_sidebar()) window_ptr->nav_sidebar()->update_theme();
        if (window_ptr->player_bar()) window_ptr->player_bar()->update_theme();
        if (window_ptr->home_view()) window_ptr->home_view()->update_theme();
    };

    if (window_->isVisible() && window_->theme_transition()) {
        window_->theme_transition()->start_transition(apply_fn);
    } else {
        apply_fn();
    }
}
