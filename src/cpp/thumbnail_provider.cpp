#include "main_window.h"
#include "ffi_utils.h"
#include <QMutex>
#include <QMutexLocker>
#include <QCryptographicHash>
#include <QStandardPaths>
#include <QDir>
#include <QHash>
#include <QPainter>
#include <QImage>
#include <QFont>
#include <QPixmapCache>
#include <QFile>

rust::String get_or_create_thumbnail(rust::Str title, int32_t variant) {
    static QMutex placeholder_cache_mutex;
    static QHash<QString, std::string> placeholder_cache;

    const std::string title_copy = Ffi::to_std_string(title);
    const QByteArray hash = QCryptographicHash::hash(
        QByteArray::fromStdString(title_copy + std::to_string(variant)),
        QCryptographicHash::Md5);
    const QString thumb_dir = QStandardPaths::writableLocation(QStandardPaths::CacheLocation) + "/artwork";
    const QString filename = QString("placeholder_%1_%2.png")
        .arg(QString(hash.toHex().left(12)))
        .arg(variant);
    const QString filepath_q = thumb_dir + "/" + filename;

    {
        QMutexLocker locker(&placeholder_cache_mutex);
        auto it = placeholder_cache.constFind(filepath_q);
        if (it != placeholder_cache.constEnd() && QFile::exists(filepath_q)) {
            return rust::String(it.value());
        }
    }

    QDir().mkpath(thumb_dir);
    if (QFile::exists(filepath_q)) {
        const std::string cached_path = Ffi::to_std_string(filepath_q);
        QMutexLocker locker(&placeholder_cache_mutex);
        placeholder_cache.insert(filepath_q, cached_path);
        return rust::String(cached_path);
    }

    int r = 50 + (static_cast<unsigned char>(hash[0]) % 156);
    int g = 30 + (static_cast<unsigned char>(hash[1]) % 120);
    int b = 70 + (static_cast<unsigned char>(hash[2]) % 140);

    QColor c1(r, g, b);
    QColor c2((r + 60) % 256, (g + 40) % 256, (b + 80) % 256);

    QImage img(128, 128, QImage::Format_ARGB32_Premultiplied);
    img.fill(Qt::transparent);
    QPainter painter(&img);
    painter.setRenderHint(QPainter::Antialiasing);
    QLinearGradient gradient(0, 0, 128, 128);
    gradient.setColorAt(0.0, c1);
    gradient.setColorAt(1.0, c2);
    painter.setBrush(gradient);
    painter.setPen(Qt::NoPen);
    painter.drawRoundedRect(0, 0, 128, 128, 12, 12);

    const QString display_title = Ffi::to_qstring(title_copy);
    const QChar first = display_title.isEmpty() ? QChar('?') : display_title.at(0).toUpper();
    painter.setPen(QPen(QColor(255, 255, 255, 200), 2));
    QFont font = painter.font();
    font.setPixelSize(52);
    font.setBold(true);
    painter.setFont(font);
    painter.drawText(QRect(0, 0, 128, 128), Qt::AlignCenter, QString(first));
    painter.end();
    img.save(filepath_q, "PNG");

    const std::string filepath = Ffi::to_std_string(filepath_q);
    if (!filepath.empty()) {
        QMutexLocker locker(&placeholder_cache_mutex);
        placeholder_cache.insert(filepath_q, filepath);
    }
    return rust::String(filepath);
}
