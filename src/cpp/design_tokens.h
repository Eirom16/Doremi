#pragma once

#include <QColor>
#include <QFont>
#include <QString>
#include <QWidget>

struct SpacingTokens {
    int xs;
    int sm;
    int md;
    int lg;
    int xl;
    int xxl;
};

struct RadiusTokens {
    int sm;
    int md;
    int lg;
    int xl;
    int pill;
};

struct ElevationTokens {
    QColor subtle;
    QColor medium;
    QColor strong;
};

struct MotionTokens {
    int instant_ms;
    int fast_ms;
    int normal_ms;
    int slow_ms;
};

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
    static const SpacingTokens &spacing();
    static const RadiusTokens &radius();
    static const ElevationTokens &elevation();
    static const MotionTokens &motion();
    static void setTheme(Theme theme);
    static Theme activeTheme();
    static void setAccentColor(const QString &hexColor);

    static QString rgba(const QColor &color);
    
    // QSS generators
    static QString getGlobalStyleSheet();
    static QString iconButtonStyle(int radius = -1);
    static QString primaryButtonStyle(int radius = -1);
    static QString navButtonStyle();
    static QString profileButtonStyle();
    static QString textInputStyle();
    static QString sliderStyle(bool prominent);
    static void applyAccessible(QWidget *widget,
                                const QString &name,
                                const QString &description = QString(),
                                const QString &toolTip = QString());
    
    // Font registration and helpers
    static void loadFonts();
    static QFont getFont(const QString &level, int size = -1);
};
