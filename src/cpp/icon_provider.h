#pragma once

#include <QIcon>
#include <QString>
#include <QLabel>
#include <QColor>

class IconProvider {
public:
    static QFont getFont(int size = 20, bool filled = true);
    static QIcon getIcon(const QString &name, const QColor &color = QColor("#F1F0FF"), int size = 24, bool filled = true);
    static QLabel *createIconLabel(const QString &name, int size = 20, const QColor &color = QColor("#F1F0FF"), bool filled = true, QWidget *parent = nullptr);
    static void setupIconLabel(QLabel *label, const QString &name, int size = 20, const QColor &color = QColor("#F1F0FF"), bool filled = true);
};
