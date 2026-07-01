#include "theme_controller.h"
#include "main_window.h"
#include "design_tokens.h"
#include "style_manager.h"
#include "components/theme_transition.h"
#include "title_bar.h"
#include "nav_sidebar.h"
#include "player_bar.h"
#include "home_view.h"
#include "search_view.h"
#include "library_view.h"
#include "settings_view.h"
#include "trending_view.h"
#include "downloads_view.h"
#include "stats_view.h"
#include "history_view.h"
#include "album_detail_view.h"
#include "artist_detail_view.h"
#include "playlist_detail_view.h"
#include "show_detail_view.h"
#include "welcome_view.h"
#include "now_playing_view.h"
#include "components/offline_banner.h"
#include "components/track_row.h"
#include "components/stat_card.h"
#include <QPointer>
#include <QStyle>

ThemeController::ThemeController(DoremiMainWindow *window)
    : QObject(window), window_(window) {}

void ThemeController::refresh_all_views() {
    if (!window_) return;
    auto *w = window_;

    if (w->title_bar()) w->title_bar()->update_theme();
    if (w->nav_sidebar()) w->nav_sidebar()->update_theme();
    if (w->player_bar()) w->player_bar()->update_theme();
    if (w->home_view()) w->home_view()->update_theme();
    if (w->search_view()) w->search_view()->update_theme();
    if (w->library_view()) w->library_view()->update_theme();
    if (w->settings_view()) w->settings_view()->update_theme();
    if (w->trending_view()) w->trending_view()->update_theme();
    if (w->downloads_view()) w->downloads_view()->update_theme();
    if (w->stats_view()) w->stats_view()->update_theme();
    if (w->history_view()) w->history_view()->update_theme();
    if (w->album_detail_view()) w->album_detail_view()->update_theme();
    if (w->artist_detail_view()) w->artist_detail_view()->update_theme();
    if (w->playlist_detail_view()) w->playlist_detail_view()->update_theme();
    if (w->show_detail_view()) w->show_detail_view()->update_theme();
    if (w->welcome_view()) w->welcome_view()->update_theme();
    if (w->now_playing_view()) w->now_playing_view()->update_theme();
    if (w->offline_banner()) w->offline_banner()->update_theme();

    w->style()->unpolish(w);
    w->style()->polish(w);
}

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
        if (window_ptr->theme_controller()) {
            window_ptr->theme_controller()->refresh_all_views();
        }
    };

    if (window_->isVisible() && window_->theme_transition()) {
        window_->theme_transition()->start_transition(apply_fn);
    } else {
        apply_fn();
    }
}
