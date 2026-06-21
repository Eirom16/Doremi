#include "design_tokens.h"
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
    QColor("#8B5CF6"), // accent
    QColor("#A78BFA"), // accent_bright
    QColor(139, 92, 246, 38), // accent_dim (rgba 0.15)
    QColor(139, 92, 246, 64), // accent_glow (rgba 0.25)
    QColor("#F1F0FF"), // text_primary
    QColor("#B8B4D8"), // text_secondary
    QColor("#77739D"), // text_muted
    QColor(255, 255, 255, 24), // border (rgba 0.09)
    QColor(139, 92, 246, 76), // border_accent (rgba 0.30)
    QColor("#34D399"), // success
    QColor("#FBBF24"), // warning
    QColor("#F87171")  // error
};

static ColorScheme g_light_colors = {
    QColor("#F8F7FF"), // bg_base
    QColor("#FFFFFF"), // bg_surface
    QColor("#F0EFFF"), // bg_elevated
    QColor(248, 247, 255, 217), // bg_overlay (rgba 0.85)
    QColor("#7C3AED"), // accent
    QColor("#8B5CF6"), // accent_bright
    QColor(124, 58, 237, 25), // accent_dim (rgba 0.10)
    QColor(124, 58, 237, 38), // accent_glow (rgba 0.15)
    QColor("#0D0D1A"), // text_primary
    QColor("#4F4A68"), // text_secondary
    QColor("#716B88"), // text_muted
    QColor(0, 0, 0, 31), // border (rgba 0.12)
    QColor(124, 58, 237, 64), // border_accent (rgba 0.25)
    QColor("#059669"), // success
    QColor("#D97706"), // warning
    QColor("#DC2626")  // error
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
    } else if (level == "caption") {
        font.setFamily("Inter");
        font.setPixelSize(size > 0 ? size : 12);
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

QString DesignTokens::getGlobalStyleSheet() {
    const auto &c = current();
    
    // Convert colors to css strings
    QString bg_base = c.bg_base.name();
    QString bg_surface = c.bg_surface.name();
    QString accent = c.accent.name();
    QString text_primary = c.text_primary.name();
    QString text_secondary = c.text_secondary.name();
    QString text_muted = c.text_muted.name();
    
    QString border = rgba(c.border);
    QString accent_dim = rgba(c.accent_dim);

    QString style = QString(
        "/* Global Style Sheet */\n"
        "QMainWindow {\n"
        "    background-color: {{bg_base}};\n"
        "    color: {{text_primary}};\n"
        "}\n"
        "\n"
        "QWidget {\n"
        "    font-family: 'Inter', sans-serif;\n"
        "    font-size: 14px;\n"
        "    color: {{text_primary}};\n"
        "}\n"
        "\n"
        "QWidget:disabled {\n"
        "    color: {{text_muted}};\n"
        "}\n"
        "\n"
        "QPushButton:focus, QToolButton:focus, QLineEdit:focus, QComboBox:focus, QSlider:focus {\n"
        "    outline: none;\n"
        "    border-color: {{accent}};\n"
        "}\n"
        "\n"
        "/* Premium Scrollbar */\n"
        "QScrollBar:vertical {\n"
        "    border: none;\n"
        "    background: transparent;\n"
        "    width: 6px;\n"
        "    margin: 0px 0px 0px 0px;\n"
        "}\n"
        "QScrollBar::handle:vertical {\n"
        "    background: {{text_muted}};\n"
        "    border-radius: 3px;\n"
        "    min-height: 20px;\n"
        "}\n"
        "QScrollBar::handle:vertical:hover {\n"
        "    background: {{accent}};\n"
        "}\n"
        "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {\n"
        "    height: 0px;\n"
        "    background: none;\n"
        "}\n"
        "QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical {\n"
        "    background: transparent;\n"
        "}\n"
        "\n"
        "QScrollBar:horizontal {\n"
        "    border: none;\n"
        "    background: transparent;\n"
        "    height: 6px;\n"
        "    margin: 0px 0px 0px 0px;\n"
        "}\n"
        "QScrollBar::handle:horizontal {\n"
        "    background: {{text_muted}};\n"
        "    border-radius: 3px;\n"
        "    min-width: 20px;\n"
        "}\n"
        "QScrollBar::handle:horizontal:hover {\n"
        "    background: {{accent}};\n"
        "}\n"
        "QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal {\n"
        "    width: 0px;\n"
        "    background: none;\n"
        "}\n"
        "QScrollBar::add-page:horizontal, QScrollBar::sub-page:horizontal {\n"
        "    background: transparent;\n"
        "}\n"
        "\n"
        "/* Tooltips */\n"
        "QToolTip {\n"
        "    background-color: {{bg_surface}};\n"
        "    color: {{text_primary}};\n"
        "    border: 1px solid {{border}};\n"
        "    border-radius: 6px;\n"
        "    padding: 6px 10px;\n"
        "    font-size: 12px;\n"
        "}\n"
        "\n"
        "/* Glass Context Menus */\n"
        "QMenu {\n"
        "    background-color: {{bg_surface}};\n"
        "    border: 1px solid {{border}};\n"
        "    border-radius: 10px;\n"
        "    padding: 6px 0px;\n"
        "}\n"
        "QMenu::item {\n"
        "    padding: 8px 20px 8px 12px;\n"
        "    color: {{text_secondary}};\n"
        "    font-size: 13px;\n"
        "}\n"
        "QMenu::item:selected {\n"
        "    background-color: {{accent_dim}};\n"
        "    color: {{text_primary}};\n"
        "}\n"
        "QMenu::separator {\n"
        "    height: 1px;\n"
        "    background: {{border}};\n"
        "    margin: 6px 0px;\n"
        "}\n"
    );

    style.replace("{{bg_base}}", bg_base);
    style.replace("{{bg_surface}}", bg_surface);
    style.replace("{{accent}}", accent);
    style.replace("{{text_primary}}", text_primary);
    style.replace("{{text_secondary}}", text_secondary);
    style.replace("{{text_muted}}", text_muted);
    style.replace("{{border}}", border);
    style.replace("{{accent_dim}}", accent_dim);
    return style;
}

QString DesignTokens::iconButtonStyle(int radiusValue) {
    const auto &c = current();
    const int r = radiusValue >= 0 ? radiusValue : radius().md;
    return QString(
        "QPushButton {\n"
        "    background: transparent;\n"
        "    border: 1px solid transparent;\n"
        "    border-radius: %1px;\n"
        "    color: %2;\n"
        "}\n"
        "QPushButton:hover {\n"
        "    background: %3;\n"
        "    border-color: %4;\n"
        "    color: %5;\n"
        "}\n"
        "QPushButton:pressed {\n"
        "    background: %6;\n"
        "    border-color: %7;\n"
        "}\n"
        "QPushButton:checked {\n"
        "    background: %3;\n"
        "    border-color: %7;\n"
        "    color: %7;\n"
        "}\n"
        "QPushButton:focus {\n"
        "    border: 2px solid %7;\n"
        "}\n"
        "QPushButton:disabled {\n"
        "    color: %8;\n"
        "    background: transparent;\n"
        "}\n"
    )
        .arg(r)
        .arg(c.text_secondary.name())
        .arg(rgba(c.accent_dim))
        .arg(rgba(c.border))
        .arg(c.text_primary.name())
        .arg(rgba(c.accent_glow))
        .arg(c.accent.name())
        .arg(c.text_muted.name());
}

QString DesignTokens::primaryButtonStyle(int radiusValue) {
    const auto &c = current();
    const int r = radiusValue >= 0 ? radiusValue : radius().pill;
    return QString(
        "QPushButton {\n"
        "    background-color: %1;\n"
        "    border: 1px solid %1;\n"
        "    border-radius: %2px;\n"
        "    color: #FFFFFF;\n"
        "    font-weight: 600;\n"
        "}\n"
        "QPushButton:hover {\n"
        "    background-color: %3;\n"
        "    border-color: %3;\n"
        "}\n"
        "QPushButton:pressed {\n"
        "    background-color: %4;\n"
        "    border-color: %4;\n"
        "}\n"
        "QPushButton:focus {\n"
        "    border: 2px solid %5;\n"
        "}\n"
        "QPushButton:disabled {\n"
        "    background-color: %6;\n"
        "    border-color: %6;\n"
        "    color: %7;\n"
        "}\n"
    )
        .arg(c.accent.name())
        .arg(r)
        .arg(c.accent_bright.name())
        .arg(c.accent.darker(115).name())
        .arg(c.text_primary.name())
        .arg(rgba(c.border_accent))
        .arg(c.text_muted.name());
}

QString DesignTokens::navButtonStyle() {
    const auto &c = current();
    return QString(
        "QPushButton {\n"
        "    background: transparent;\n"
        "    border: none;\n"
        "    border-radius: 0px;\n"
        "    border-left: 3px solid transparent;\n"
        "    color: %1;\n"
        "}\n"
        "QPushButton:hover {\n"
        "    background: %2;\n"
        "    color: %3;\n"
        "}\n"
        "QPushButton:pressed {\n"
        "    background: %4;\n"
        "}\n"
        "QPushButton:checked {\n"
        "    background: %2;\n"
        "    color: %5;\n"
        "    border-left: 3px solid %5;\n"
        "}\n"
        "QPushButton:focus {\n"
        "    border: 2px solid %5;\n"
        "    border-left: 3px solid %5;\n"
        "}\n"
        "QPushButton:disabled {\n"
        "    color: %6;\n"
        "}\n"
    )
        .arg(c.text_secondary.name())
        .arg(rgba(c.accent_dim))
        .arg(c.text_primary.name())
        .arg(rgba(c.accent_glow))
        .arg(c.accent.name())
        .arg(c.text_muted.name());
}

QString DesignTokens::profileButtonStyle() {
    const auto &c = current();
    return QString(
        "QPushButton {\n"
        "    background: transparent;\n"
        "    border: 1px solid transparent;\n"
        "    border-radius: %1px;\n"
        "    text-align: left;\n"
        "    padding-left: 20px;\n"
        "    margin: 4px 8px;\n"
        "    font-weight: bold;\n"
        "    color: %2;\n"
        "}\n"
        "QPushButton:hover {\n"
        "    background: %3;\n"
        "    color: %4;\n"
        "}\n"
        "QPushButton:pressed {\n"
        "    background: %5;\n"
        "}\n"
        "QPushButton:focus {\n"
        "    border: 2px solid %6;\n"
        "}\n"
    )
        .arg(radius().lg)
        .arg(c.text_secondary.name())
        .arg(rgba(c.accent_dim))
        .arg(c.text_primary.name())
        .arg(rgba(c.accent_glow))
        .arg(c.accent.name());
}

QString DesignTokens::textInputStyle() {
    const auto &c = current();
    return QString(
        "QLineEdit {\n"
        "    background-color: %1;\n"
        "    border: 1px solid %2;\n"
        "    border-radius: %3px;\n"
        "    padding: 6px 12px 6px 36px;\n"
        "    color: %4;\n"
        "    selection-background-color: %5;\n"
        "}\n"
        "QLineEdit:hover {\n"
        "    border-color: %6;\n"
        "}\n"
        "QLineEdit:focus {\n"
        "    border: 2px solid %7;\n"
        "    background-color: %8;\n"
        "}\n"
        "QLineEdit:disabled {\n"
        "    color: %9;\n"
        "    background-color: %10;\n"
        "}\n"
    )
        .arg(c.bg_base.name())
        .arg(rgba(c.border))
        .arg(radius().md)
        .arg(c.text_primary.name())
        .arg(rgba(c.accent_dim))
        .arg(c.text_secondary.name())
        .arg(c.accent.name())
        .arg(c.bg_elevated.name())
        .arg(c.text_muted.name())
        .arg(rgba(c.border));
}

QString DesignTokens::sliderStyle(bool prominent) {
    const auto &c = current();
    const QColor groove = activeTheme() == Theme::Dark ? QColor(255, 255, 255, 26) : QColor(0, 0, 0, 20);
    const QColor subPage = prominent ? c.accent : c.text_secondary;
    const QColor handle = prominent ? c.accent_bright : c.text_primary;
    const int grooveHeight = prominent ? 4 : 3;
    const int handleSize = prominent ? 12 : 10;
    const int margin = -(handleSize - grooveHeight) / 2;
    return QString(
        "QSlider::groove:horizontal {\n"
        "    height: %1px;\n"
        "    background: %2;\n"
        "    border-radius: %3px;\n"
        "}\n"
        "QSlider::sub-page:horizontal {\n"
        "    background: %4;\n"
        "    border-radius: %3px;\n"
        "}\n"
        "QSlider::handle:horizontal {\n"
        "    background: %5;\n"
        "    width: %6px;\n"
        "    height: %6px;\n"
        "    margin: %7px 0;\n"
        "    border-radius: %8px;\n"
        "}\n"
        "QSlider::handle:horizontal:hover {\n"
        "    background: %9;\n"
        "}\n"
        "QSlider:focus::groove:horizontal {\n"
        "    background: %10;\n"
        "}\n"
    )
        .arg(grooveHeight)
        .arg(rgba(groove))
        .arg(grooveHeight / 2)
        .arg(subPage.name())
        .arg(handle.name())
        .arg(handleSize)
        .arg(margin)
        .arg(handleSize / 2)
        .arg(c.accent_bright.name())
        .arg(rgba(c.accent_glow));
}

void DesignTokens::applyAccessible(QWidget *widget,
                                   const QString &name,
                                   const QString &description,
                                   const QString &toolTip) {
    if (!widget) {
        return;
    }
    widget->setAccessibleName(name);
    if (!description.isEmpty()) {
        widget->setAccessibleDescription(description);
    }
    const QString resolvedToolTip = toolTip.isEmpty() ? name : toolTip;
    widget->setToolTip(resolvedToolTip);
    widget->setStatusTip(resolvedToolTip);
}
