#ifndef DOREMI_BRIDGE_HELPERS_H
#define DOREMI_BRIDGE_HELPERS_H

#include <QString>
#include <functional>
#include "main_window.h"
#include "ffi_utils.h"

template <typename Callback>
void mutate_main_window(const char *operation, Callback callback) {
    Ffi::on_gui(operation, [callback = std::move(callback)]() mutable {
        if (g_main_window) {
            callback(*g_main_window);
        }
    });
}

QString user_facing_notification(QString message);

#endif
