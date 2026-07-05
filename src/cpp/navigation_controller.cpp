#include "navigation_controller.h"
#include "main_window.h"
#include "title_bar.h"
#include "home_view.h"
#include "nav_sidebar.h"
#include "library_view.h"
#include "search_view.h"
#include "settings_view.h"
#include "components/fade_stack.h"
#include <QScrollBar>
#include <QTimer>
#include <QApplication>

NavigationController::NavigationController(DoremiMainWindow *window)
    : QObject(window), window_(window) {}

void NavigationController::navigate_to(const std::string &route) {
    navigate_to_internal(route, true);
}

void NavigationController::navigate_to_internal(const std::string &route, bool record_history) {
    if (route.empty()) {
        return;
    }
    save_route_view_state();

    if (record_history && route != current_route_) {
        back_routes_.push_back(current_route_);
        if (back_routes_.size() > 80) {
            back_routes_.erase(back_routes_.begin());
        }
        forward_routes_.clear();
    }

    const bool opens_detail = route == "album_detail" || route == "artist_detail" ||
                              route == "playlist_detail" || route == "show_detail";
    if (opens_detail && current_route_ != "album_detail" &&
        current_route_ != "artist_detail" && current_route_ != "playlist_detail" &&
        current_route_ != "show_detail") {
        detail_return_route_ = current_route_;
    }

    if (window_->nav_sidebar()) {
        window_->nav_sidebar()->set_active_route(route);
    }
    if (window_->title_bar()) {
        window_->title_bar()->set_context(route);
    }

    if (window_->stack_) {
        if (route == "home") {
            window_->stack_->setCurrentIndex(static_cast<int>(ViewIndex::Home));
        } else if (route == "search") {
            window_->stack_->setCurrentIndex(static_cast<int>(ViewIndex::Search));
        } else if (route == "library") {
            window_->stack_->setCurrentIndex(static_cast<int>(ViewIndex::Library));
            if (window_->library_view()) {
                std::string tab = window_->library_view()->current_tab();
                if (tab.empty()) tab = "playlists";
                emit window_->library_view()->tab_changed(tab);
            }
        } else if (route == "settings") {
            window_->settings_view_->exec();
            return;
        } else if (route == "trending") {
            window_->stack_->setCurrentIndex(static_cast<int>(ViewIndex::Trending));
        } else if (route == "downloads") {
            window_->stack_->setCurrentIndex(static_cast<int>(ViewIndex::Downloads));
            on_downloads_requested();
        } else if (route == "stats") {
            window_->stack_->setCurrentIndex(static_cast<int>(ViewIndex::Stats));
        } else if (route == "history") {
            window_->stack_->setCurrentIndex(static_cast<int>(ViewIndex::History));
            on_history_requested();
        } else if (route == "album_detail") {
            window_->stack_->setCurrentIndex(static_cast<int>(ViewIndex::AlbumDetail));
        } else if (route == "artist_detail") {
            window_->stack_->setCurrentIndex(static_cast<int>(ViewIndex::ArtistDetail));
        } else if (route == "playlist_detail") {
            window_->stack_->setCurrentIndex(static_cast<int>(ViewIndex::PlaylistDetail));
        } else if (route == "show_detail") {
            window_->stack_->setCurrentIndex(static_cast<int>(ViewIndex::ShowDetail));
        } else if (route == "welcome") {
            window_->stack_->setCurrentIndex(static_cast<int>(ViewIndex::Welcome));
        } else {
            window_->stack_->setCurrentIndex(static_cast<int>(ViewIndex::Home));
        }
    }
    
    window_->update_fab_visibility();

    current_route_ = route;
    if (window_->player_shell_) {
        window_->player_shell_->setVisible(route != "welcome");
    }
    restore_route_view_state(route);
}

void NavigationController::save_route_view_state() {
    if (current_route_.empty()) {
        return;
    }
    if (window_->body_scroll_ && window_->body_scroll_->verticalScrollBar()) {
        route_scroll_positions_[current_route_] = window_->body_scroll_->verticalScrollBar()->value();
    }
    if (auto *focused = QApplication::focusWidget()) {
        if (focused == window_ || window_->isAncestorOf(focused)) {
            route_focus_widgets_[current_route_] = focused;
        }
    }
}

void NavigationController::restore_route_view_state(const std::string &route) {
    const int scroll = route_scroll_positions_.count(route) ? route_scroll_positions_[route] : 0;
    QPointer<QWidget> focus = route_focus_widgets_.count(route) ? route_focus_widgets_[route] : QPointer<QWidget>();
    QTimer::singleShot(0, this, [this, route, scroll, focus]() {
        if (route != current_route_) {
            return;
        }
        if (window_->body_scroll_ && window_->body_scroll_->verticalScrollBar()) {
            window_->body_scroll_->verticalScrollBar()->setValue(scroll);
        }
        if (focus && focus->isVisible() && focus->isEnabled()) {
            focus->setFocus(Qt::OtherFocusReason);
        } else if (window_->stack_) {
            window_->stack_->setFocus(Qt::OtherFocusReason);
        }
    });
}

void NavigationController::navigate_back() {
    while (!back_routes_.empty() && back_routes_.back() == current_route_) {
        back_routes_.pop_back();
    }
    if (back_routes_.empty()) {
        return;
    }
    const std::string target = back_routes_.back();
    back_routes_.pop_back();
    forward_routes_.push_back(current_route_);
    navigate_to_internal(target, false);
    if (target == "search") {
        on_search_history_requested();
    }
}

void NavigationController::navigate_back_from_detail() {
    navigate_to(detail_return_route_.empty() ? "home" : detail_return_route_);
}

void NavigationController::navigate_forward() {
    while (!forward_routes_.empty() && forward_routes_.back() == current_route_) {
        forward_routes_.pop_back();
    }
    if (forward_routes_.empty()) {
        return;
    }
    const std::string target = forward_routes_.back();
    forward_routes_.pop_back();
    back_routes_.push_back(current_route_);
    navigate_to_internal(target, false);
    if (target == "search") {
        on_search_history_requested();
    }
}
