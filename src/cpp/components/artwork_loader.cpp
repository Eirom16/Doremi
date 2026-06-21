#include "artwork_loader.h"

#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QGuiApplication>
#include <QImage>
#include <QMetaObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QPixmapCache>
#include <QRegularExpression>
#include <QScreen>
#include <QStandardPaths>
#include <QUrl>

#include <cmath>
#include <functional>
#include <thread>

namespace {
QNetworkAccessManager *manager() {
    static auto *instance = new QNetworkAccessManager();
    return instance;
}

QString cache_path(const QString &source) {
    const QString root = QStandardPaths::writableLocation(QStandardPaths::CacheLocation)
        + "/artwork";
    QDir().mkpath(root);
    const QByteArray digest = QCryptographicHash::hash(
        source.toUtf8(), QCryptographicHash::Sha256).toHex();
    return root + "/" + QString::fromLatin1(digest) + ".img";
}

qreal current_dpr() {
    if (auto *screen = QGuiApplication::primaryScreen()) {
        return qMax<qreal>(1.0, screen->devicePixelRatio());
    }
    return 1.0;
}

int requested_artwork_px(const QSize &size, qreal dpr) {
    const int logicalSize = qMax(size.width(), size.height());
    const int physicalSize = static_cast<int>(std::ceil(logicalSize * dpr));
    const int bounded = qBound(64, physicalSize, 1024);
    return ((bounded + 15) / 16) * 16;
}

QString sized_remote_source(const QString &source, const QSize &size, qreal dpr) {
    const QUrl url(source);
    if (!url.isValid() || !url.scheme().startsWith("http")) return source;

    const QString host = url.host().toLower();
    const bool supportsSizeSuffix =
        host.contains("googleusercontent.com") ||
        host.contains("yt3.ggpht.com");
    if (!supportsSizeSuffix) return source;

    QString resized = url.toString(QUrl::FullyEncoded);
    const QString suffix = QString("=w%1-h%1-l90-rj").arg(requested_artwork_px(size, dpr));
    const QRegularExpression existingSuffix("=(?:w\\d+-h\\d+|s\\d+)[^/?#]*$");
    if (existingSuffix.match(resized).hasMatch()) {
        resized.replace(existingSuffix, suffix);
    } else {
        resized += suffix;
    }
    return resized;
}

QImage scaled_image(const QImage &image, const QSize &size, qreal dpr) {
    const QSize physicalSize = (QSizeF(size) * dpr).toSize();
    return image.scaled(physicalSize, Qt::KeepAspectRatioByExpanding,
                        Qt::SmoothTransformation);
}

void load_image_async(const QString &key,
                      const QSize &size,
                      qreal dpr,
                      ArtworkLoader::Callback callback,
                      std::function<QImage()> loadImage) {
    QObject *receiver = manager();
    std::thread([key, size, dpr, receiver, callback = std::move(callback), loadImage = std::move(loadImage)]() mutable {
        QImage image = loadImage();
        if (image.isNull()) return;
        image = scaled_image(image, size, dpr);
        if (image.isNull()) return;

        QMetaObject::invokeMethod(receiver, [key, dpr, image = std::move(image), callback = std::move(callback)]() mutable {
            QPixmap pixmap = QPixmap::fromImage(image);
            pixmap.setDevicePixelRatio(dpr);
            QPixmapCache::insert(key, pixmap);
            callback(pixmap);
        }, Qt::QueuedConnection);
    }).detach();
}
}

void ArtworkLoader::load(const QString &source, const QSize &size, Callback callback) {
    if (source.isEmpty()) return;
    const qreal dpr = current_dpr();
    const QString key = source + QString(":%1x%2@%3")
        .arg(size.width())
        .arg(size.height())
        .arg(dpr, 0, 'f', 2);
    QPixmap cached;
    if (QPixmapCache::find(key, &cached)) {
        callback(cached);
        return;
    }

    if (QFile::exists(source)) {
        load_image_async(key, size, dpr, std::move(callback), [source]() {
            return QImage(source);
        });
        return;
    }
    const QString remoteSource = sized_remote_source(source, size, dpr);
    const QUrl url(remoteSource);
    if (!url.isValid() || !url.scheme().startsWith("http")) return;

    const QString diskPath = cache_path(remoteSource);
    if (QFile::exists(diskPath)) {
        load_image_async(key, size, dpr, std::move(callback), [diskPath]() {
            return QImage(diskPath);
        });
        return;
    }

    QNetworkRequest request(url);
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                         QNetworkRequest::NoLessSafeRedirectPolicy);
    auto *reply = manager()->get(request);
    QObject::connect(reply, &QNetworkReply::finished, reply,
                     [reply, key, diskPath, size, dpr, callback = std::move(callback)]() {
        const QByteArray data = reply->readAll();
        reply->deleteLater();
        if (data.isEmpty()) return;

        load_image_async(key, size, dpr, std::move(callback), [data, diskPath]() {
            QImage downloaded;
            if (!downloaded.loadFromData(data)) return QImage();
            QFile file(diskPath);
            if (file.open(QIODevice::WriteOnly)) file.write(data);
            return downloaded;
        });
    });
}
