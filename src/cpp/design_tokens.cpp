#include "design_tokens.h"
#include <QCoreApplication>
#include <QFontDatabase>
#include <QDir>
#include <QDebug>

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
    QColor("#8B8BAF"), // text_secondary
    QColor("#4A4A6A"), // text_muted
    QColor(255, 255, 255, 15), // border (rgba 0.06)
    QColor(139, 92, 246, 51), // border_accent (rgba 0.20)
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
    QColor("#6B6B8A"), // text_secondary
    QColor("#9B9BB0"), // text_muted
    QColor(0, 0, 0, 15), // border (rgba 0.06)
    QColor(124, 58, 237, 38), // border_accent (rgba 0.15)
    QColor("#059669"), // success
    QColor("#D97706"), // warning
    QColor("#DC2626")  // error
};

const ColorScheme &DesignTokens::current() {
    if (g_active_theme == Theme::Light) {
        return g_light_colors;
    }
    return g_dark_colors;
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

void DesignTokens::loadFonts() {
    static bool fonts_loaded = false;
    if (fonts_loaded) return;

    QStringList search_paths = {
        QCoreApplication::applicationDirPath() + "/assets/fonts/MaterialSymbolsRounded.ttf",
        QCoreApplication::applicationDirPath() + "/../assets/fonts/MaterialSymbolsRounded.ttf",
        QDir::currentPath() + "/assets/fonts/MaterialSymbolsRounded.ttf",
        "/home/eirom/Documents/Port/Doremi/assets/fonts/MaterialSymbolsRounded.ttf"
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
    QString bg_elevated = c.bg_elevated.name();
    QString accent = c.accent.name();
    QString accent_bright = c.accent_bright.name();
    QString text_primary = c.text_primary.name();
    QString text_secondary = c.text_secondary.name();
    QString text_muted = c.text_muted.name();
    
    QString border = QString("rgba(%1, %2, %3, %4)")
        .arg(c.border.red()).arg(c.border.green()).arg(c.border.blue()).arg(c.border.alpha() / 255.0);
    QString border_accent = QString("rgba(%1, %2, %3, %4)")
        .arg(c.border_accent.red()).arg(c.border_accent.green()).arg(c.border_accent.blue()).arg(c.border_accent.alpha() / 255.0);
    QString accent_dim = QString("rgba(%1, %2, %3, %4)")
        .arg(c.accent_dim.red()).arg(c.accent_dim.green()).arg(c.accent_dim.blue()).arg(c.accent_dim.alpha() / 255.0);

    return QString(
        "/* Global Style Sheet */\n"
        "QMainWindow {\n"
        "    background-color: %1;\n"
        "    color: %4;\n"
        "}\n"
        "\n"
        "QWidget {\n"
        "    font-family: 'Inter', sans-serif;\n"
        "    font-size: 14px;\n"
        "    color: %4;\n"
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
        "    background: %6;\n"
        "    border-radius: 3px;\n"
        "    min-height: 20px;\n"
        "}\n"
        "QScrollBar::handle:vertical:hover {\n"
        "    background: %3;\n"
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
        "    background: %6;\n"
        "    border-radius: 3px;\n"
        "    min-width: 20px;\n"
        "}\n"
        "QScrollBar::handle:horizontal:hover {\n"
        "    background: %3;\n"
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
        "    background-color: %2;\n"
        "    color: %4;\n"
        "    border: 1px solid %8;\n"
        "    border-radius: 6px;\n"
        "    padding: 6px 10px;\n"
        "    font-size: 12px;\n"
        "}\n"
        "\n"
        "/* Glass Context Menus */\n"
        "QMenu {\n"
        "    background-color: %2;\n"
        "    border: 1px solid %8;\n"
        "    border-radius: 10px;\n"
        "    padding: 6px 0px;\n"
        "}\n"
        "QMenu::item {\n"
        "    padding: 8px 20px 8px 12px;\n"
        "    color: %5;\n"
        "    font-size: 13px;\n"
        "}\n"
        "QMenu::item:selected {\n"
        "    background-color: %9;\n"
        "    color: %4;\n"
        "}\n"
        "QMenu::separator {\n"
        "    height: 1px;\n"
        "    background: %8;\n"
        "    margin: 6px 0px;\n"
        "}\n"
    )
    .arg(bg_base)       // %1
    .arg(bg_surface)    // %2
    .arg(accent)        // %3
    .arg(text_primary)  // %4
    .arg(text_secondary)// %5
    .arg(text_muted)     // %6
    .arg(accent_bright) // %7
    .arg(border)        // %8
    .arg(accent_dim);   // %9
}
