#pragma once

#include <QPixmap>
#include <QSize>
#include <QString>
#include <functional>

class ArtworkLoader {
public:
    using Callback = std::function<void(const QPixmap &)>;

    static void load(const QString &source, const QSize &size, Callback callback);
};
