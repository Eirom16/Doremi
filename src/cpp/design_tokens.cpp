#include "design_tokens.h"
#include "doremi/src/bridge.rs.h"
#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFontDatabase>
#include <QGuiApplication>
#include <QLocale>
#include <QStyleHints>

static DesignTokens::Theme g_active_theme = DesignTokens::Theme::Dark;

static ColorScheme g_dark_colors = {
    QColor("#07070F"), // bg_base
    QColor("#0D0D1A"), // bg_surface
    QColor("#14142B"), // bg_elevated
    QColor(7, 7, 15, 217), // bg_overlay (rgba 0.85)
    QColor("#0B0B16"), // surface
    QColor("#17172B"), // surface_raised
    QColor(139, 92, 246, 31), // surface_selected
    QColor("#8B5CF6"), // accent
    QColor("#A78BFA"), // accent_bright
    QColor(139, 92, 246, 38), // accent_dim (rgba 0.15)
    QColor(139, 92, 246, 64), // accent_glow (rgba 0.25)
    QColor("#F1F0FF"), // text_primary
    QColor("#FFFFFF"), // text_on_accent
    QColor("#B8B4D8"), // text_secondary
    QColor("#77739D"), // text_muted
    QColor(255, 255, 255, 24), // border (rgba 0.09)
    QColor(139, 92, 246, 76), // border_accent (rgba 0.30)
    QColor("#34D399"), // success
    QColor("#FBBF24"), // warning
    QColor("#F87171"), // error
    QColor(248, 113, 113, 24), // danger_surface
    QColor(251, 191, 36, 24), // warning_surface
    QColor("#60A5FA"), // type_album
    QColor("#2DD4BF"), // type_playlist
    QColor("#FBBF24"), // type_mix
    QColor("#FB7185")  // type_podcast
};

static ColorScheme g_light_colors = {
    QColor("#F8F7FF"), // bg_base
    QColor("#FFFFFF"), // bg_surface
    QColor("#F0EFFF"), // bg_elevated
    QColor(248, 247, 255, 217), // bg_overlay (rgba 0.85)
    QColor("#FFFFFF"), // surface
    QColor("#F3F1FF"), // surface_raised
    QColor(124, 58, 237, 25), // surface_selected
    QColor("#7C3AED"), // accent
    QColor("#8B5CF6"), // accent_bright
    QColor(124, 58, 237, 25), // accent_dim (rgba 0.10)
    QColor(124, 58, 237, 38), // accent_glow (rgba 0.15)
    QColor("#0D0D1A"), // text_primary
    QColor("#FFFFFF"), // text_on_accent
    QColor("#4F4A68"), // text_secondary
    QColor("#716B88"), // text_muted
    QColor(0, 0, 0, 31), // border (rgba 0.12)
    QColor(124, 58, 237, 64), // border_accent (rgba 0.25)
    QColor("#059669"), // success
    QColor("#D97706"), // warning
    QColor("#DC2626"), // error
    QColor(220, 38, 38, 20), // danger_surface
    QColor(217, 119, 6, 20), // warning_surface
    QColor("#2563EB"), // type_album
    QColor("#0F766E"), // type_playlist
    QColor("#D97706"), // type_mix
    QColor("#E11D48")  // type_podcast
};

static const SpacingTokens g_spacing = {
    4,  // xs
    8,  // sm
    12, // md
    16, // lg
    24, // xl
    32  // xxl
};

static const RadiusTokens g_radius = {
    2,   // xs
    6,   // sm
    8,   // md
    12,  // lg
    16,  // xl
    999  // pill
};

static const MotionTokens g_motion = {
    0,   // instant_ms
    120, // fast_ms
    180, // normal_ms
    260  // slow_ms
};

static const QStringList g_accent_palette = {
    "#7C4DFF", "#A78BFA", "#22D3EE", "#F472B6", "#34D399"
};

static ElevationTokens makeElevationTokens(const ColorScheme &c) {
    return {
        QColor(0, 0, 0, g_active_theme == DesignTokens::Theme::Dark ? 35 : 18),
        QColor(0, 0, 0, g_active_theme == DesignTokens::Theme::Dark ? 60 : 28),
        QColor(c.accent.red(), c.accent.green(), c.accent.blue(), 46)
    };
}

const ColorScheme &DesignTokens::current() {
    if (g_active_theme == Theme::Light) {
        return g_light_colors;
    }
    return g_dark_colors;
}

const SpacingTokens &DesignTokens::spacing() {
    return g_spacing;
}

const RadiusTokens &DesignTokens::radius() {
    return g_radius;
}

const ElevationTokens &DesignTokens::elevation() {
    static ElevationTokens tokens = makeElevationTokens(current());
    tokens = makeElevationTokens(current());
    return tokens;
}

const MotionTokens &DesignTokens::motion() {
    return g_motion;
}

const QStringList &DesignTokens::accentPalette() {
    return g_accent_palette;
}

QMargins DesignTokens::pagePadding() {
    return QMargins(g_spacing.xl, g_spacing.xl, g_spacing.xl, g_spacing.xl);
}

QMargins DesignTokens::pagePaddingNarrow() {
    return QMargins(g_spacing.xl, g_spacing.lg, g_spacing.xl, g_spacing.xl);
}

void DesignTokens::setTheme(Theme theme) {
    g_active_theme = theme;
}

DesignTokens::Theme DesignTokens::activeTheme() {
    return g_active_theme;
}

void DesignTokens::setAccentColor(const QString &hexColor) {
    QColor accent(hexColor);
    if (!accent.isValid()) return;

    // Update dark scheme accent colors
    g_dark_colors.accent = accent;
    g_dark_colors.accent_bright = accent.lighter(120);
    g_dark_colors.accent_dim = QColor(accent.red(), accent.green(), accent.blue(), 38);
    g_dark_colors.accent_glow = QColor(accent.red(), accent.green(), accent.blue(), 64);
    g_dark_colors.border_accent = QColor(accent.red(), accent.green(), accent.blue(), 51);

    // Update light scheme accent colors
    g_light_colors.accent = accent;
    g_light_colors.accent_bright = accent.darker(120);
    g_light_colors.accent_dim = QColor(accent.red(), accent.green(), accent.blue(), 25);
    g_light_colors.accent_glow = QColor(accent.red(), accent.green(), accent.blue(), 38);
    g_light_colors.border_accent = QColor(accent.red(), accent.green(), accent.blue(), 38);
}

QString DesignTokens::rgba(const QColor &color) {
    return QString("rgba(%1, %2, %3, %4)")
        .arg(color.red())
        .arg(color.green())
        .arg(color.blue())
        .arg(QLocale::c().toString(color.alphaF(), 'f', 3));
}

bool DesignTokens::reducedMotion() {
    const QString env = qEnvironmentVariable("DOREMI_REDUCE_MOTION").trimmed().toLower();
    if (env == "1" || env == "true" || env == "yes" || env == "on") {
        return true;
    }
    if (QGuiApplication::instance()) {
        if (auto *hints = QGuiApplication::styleHints()) {
            return !hints->useHoverEffects();
        }
    }
    return false;
}

int DesignTokens::duration(int milliseconds) {
    return reducedMotion() ? 0 : milliseconds;
}

void DesignTokens::loadFonts() {
    static bool fonts_loaded = false;
    if (fonts_loaded) return;

    QStringList search_paths = {
        ":/assets/fonts/MaterialSymbolsRounded.ttf",
        QCoreApplication::applicationDirPath() + "/assets/fonts/MaterialSymbolsRounded.ttf",
        QCoreApplication::applicationDirPath() + "/../assets/fonts/MaterialSymbolsRounded.ttf",
        QDir::currentPath() + "/assets/fonts/MaterialSymbolsRounded.ttf"
    };

    bool loaded = false;
    for (const QString &path : search_paths) {
        if (QFile::exists(path)) {
            int id = QFontDatabase::addApplicationFont(path);
            if (id != -1) {
                qDebug() << "Successfully loaded Material Symbols Rounded from:" << path;
                loaded = true;
                break;
            }
        }
    }

    if (!loaded) {
        qWarning() << "Warning: Could not load Material Symbols Rounded font from any standard path.";
    }

    fonts_loaded = true;
}

QFont DesignTokens::getFont(const QString &level, int size) {
    QFont font;
    if (level == "display") {
        font.setFamily("Inter");
        font.setPixelSize(size > 0 ? size : 32);
        font.setBold(true);
    } else if (level == "heading_lg") {
        font.setFamily("Inter");
        font.setPixelSize(size > 0 ? size : 22);
        font.setWeight(QFont::DemiBold);
    } else if (level == "heading_sm") {
        font.setFamily("Inter");
        font.setPixelSize(size > 0 ? size : 16);
        font.setWeight(QFont::DemiBold);
    } else if (level == "body") {
        font.setFamily("Inter");
        font.setPixelSize(size > 0 ? size : 14);
        font.setWeight(QFont::Normal);
    } else if (level == "body_sm") {
        font.setFamily("Inter");
        font.setPixelSize(13);
        font.setWeight(QFont::Normal);
    } else if (level == "caption") {
        font.setFamily("Inter");
        font.setPixelSize(size > 0 ? size : 12);
        font.setWeight(QFont::Normal);
    } else if (level == "caption_sm") {
        font.setFamily("Inter");
        font.setPixelSize(11);
        font.setWeight(QFont::Normal);
    } else if (level == "micro") {
        font.setFamily("Inter");
        font.setPixelSize(size > 0 ? size : 10);
        font.setWeight(QFont::Medium);
    } else if (level == "icon") {
        font.setFamily("Material Symbols Rounded");
        font.setPixelSize(size > 0 ? size : 24);
        font.setStyleStrategy(QFont::PreferAntialias);
        font.setHintingPreference(QFont::PreferNoHinting);
    } else {
        font.setFamily("Inter");
        font.setPixelSize(size > 0 ? size : 14);
    }
    return font;
}

void DesignTokens::applyAccessible(QWidget *widget,
                                    const QString &name,
                                    const QString &description,
                                    const QString &toolTip) {
    widget->setAccessibleName(name);
    if (!description.isEmpty())
        widget->setAccessibleDescription(description);
    if (!toolTip.isEmpty())
        widget->setToolTip(toolTip);
}

QString tr_q(const char *key) {
    rust::String res = doremi_tr(key);
    return QString::fromUtf8(res.data(), static_cast<qsizetype>(res.size()));
}
