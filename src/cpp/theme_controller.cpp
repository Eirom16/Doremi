#include "theme_controller.h"
#include "main_window.h"
#include "design_tokens.h"
#include "style_manager.h"
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
        StyleManager::applyTheme();
    };

    if (window_->isVisible() && window_->theme_transition()) {
        window_->theme_transition()->start_transition(apply_fn);
    } else {
        apply_fn();
    }
}
