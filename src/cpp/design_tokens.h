#pragma once

#include <QColor>
#include <QString>
#include <QFont>

struct ColorScheme {
    QColor bg_base;
    QColor bg_surface;
    QColor bg_elevated;
    QColor bg_overlay;
    QColor accent;
    QColor accent_bright;
    QColor accent_dim;
    QColor accent_glow;
    QColor text_primary;
    QColor text_secondary;
    QColor text_muted;
    QColor border;
    QColor border_accent;
    QColor success;
    QColor warning;
    QColor error;
};

class DesignTokens {
public:
    enum class Theme {
        Dark,
        Light
    };

    static const ColorScheme &current();
    static void setTheme(Theme theme);
    static Theme activeTheme();
    static void setAccentColor(const QString &hexColor);
    
    // QSS generators
    static QString getGlobalStyleSheet();
    
    // Font registration and helpers
    static void loadFonts();
    static QFont getFont(const QString &level, int size = -1);
};
