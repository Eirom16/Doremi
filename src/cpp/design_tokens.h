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
    QColor surface;
    QColor surface_raised;
    QColor surface_selected;
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
    QColor danger_surface;
    QColor warning_surface;
    QColor type_album;
    QColor type_playlist;
    QColor type_mix;
    QColor type_podcast;
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
    static bool reducedMotion();
    static int duration(int milliseconds);
    
    // QSS generators
    static QString getGlobalStyleSheet();
    static QString iconButtonStyle(int radius = -1);
    static QString primaryButtonStyle(int radius = -1);
    static QString navButtonStyle();
    static QString profileButtonStyle();
    static QString textInputStyle();
    static QString sliderStyle(bool prominent);
    static QString textStyle(const QString &level, const QString &customColor = QString());
    static QString panelStyle(const QString &type = "surface", int radiusValue = -1);
    static QString scrollAreaStyle();
    static void applyAccessible(QWidget *widget,
                                const QString &name,
                                const QString &description = QString(),
                                const QString &toolTip = QString());
    
    // Font registration and helpers
    static void loadFonts();
    static QFont getFont(const QString &level, int size = -1);
};

// Translation helper to avoid double conversion: rust::String -> std::string -> QString
QString tr_q(const char *key);
