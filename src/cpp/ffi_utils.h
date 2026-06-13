#ifndef DOREMI_FFI_UTILS_H
#define DOREMI_FFI_UTILS_H

#include <QByteArray>
#include <QCoreApplication>
#include <QDebug>
#include <QMetaObject>
#include <QString>
#include <QThread>
#include <exception>
#include <functional>
#include <string>
#include <utility>
#include "rust/cxx.h"

namespace Ffi {

inline QString to_qstring(rust::Str value) {
    return QString::fromUtf8(value.data(), static_cast<qsizetype>(value.size()));
}

inline QString to_qstring(const rust::String &value) {
    return QString::fromUtf8(value.data(), static_cast<qsizetype>(value.size()));
}

inline QString to_qstring(const std::string &value) {
    return QString::fromUtf8(value.data(), static_cast<qsizetype>(value.size()));
}

inline std::string to_std_string(rust::Str value) {
    return std::string(value.data(), value.size());
}

inline std::string to_std_string(const rust::String &value) {
    return std::string(value.data(), value.size());
}

inline std::string to_std_string(const QString &value) {
    const QByteArray utf8 = value.toUtf8();
    return std::string(utf8.constData(), static_cast<size_t>(utf8.size()));
}

inline rust::String to_rust_string(const QString &value) {
    return rust::String(to_std_string(value));
}

template <typename Callback>
void guard(const char *operation, Callback &&callback) noexcept {
    try {
        std::invoke(std::forward<Callback>(callback));
    } catch (const std::exception &error) {
        qCritical().noquote() << "FFI operation failed:" << operation << "-" << error.what();
    } catch (...) {
        qCritical().noquote() << "FFI operation failed:" << operation << "- unknown C++ exception";
    }
}

template <typename Callback>
void on_gui(const char *operation, Callback callback) noexcept {
    auto *app = QCoreApplication::instance();
    if (!app || QThread::currentThread() == app->thread()) {
        guard(operation, std::move(callback));
        return;
    }

    QMetaObject::invokeMethod(
        app,
        [operation, callback = std::move(callback)]() mutable {
            guard(operation, std::move(callback));
        },
        Qt::QueuedConnection);
}

template <typename Result, typename Callback>
Result on_gui_blocking(const char *operation, Result fallback, Callback callback) noexcept {
    auto *app = QCoreApplication::instance();
    if (!app || QThread::currentThread() == app->thread()) {
        try {
            return std::invoke(std::move(callback));
        } catch (const std::exception &error) {
            qCritical().noquote() << "FFI operation failed:" << operation << "-" << error.what();
        } catch (...) {
            qCritical().noquote() << "FFI operation failed:" << operation << "- unknown C++ exception";
        }
        return fallback;
    }

    Result result = fallback;
    QMetaObject::invokeMethod(
        app,
        [&]() {
            try {
                result = std::invoke(callback);
            } catch (const std::exception &error) {
                qCritical().noquote() << "FFI operation failed:" << operation << "-" << error.what();
            } catch (...) {
                qCritical().noquote() << "FFI operation failed:" << operation << "- unknown C++ exception";
            }
        },
        Qt::BlockingQueuedConnection);
    return result;
}

} // namespace Ffi

#endif
