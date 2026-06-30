#pragma once

#include <QString>

class StyleManager {
public:
    static QString getStyleSheet();
    static void applyTheme();

private:
    static QString resolvePlaceholders(const QString &qss);
};
