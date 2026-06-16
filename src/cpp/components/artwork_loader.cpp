#include "artwork_loader.h"

#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QPixmapCache>
#include <QStandardPaths>
#include <QUrl>

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

QPixmap scaled(const QPixmap &pixmap, const QSize &size) {
    return pixmap.scaled(size, Qt::KeepAspectRatioByExpanding,
                         Qt::SmoothTransformation);
}
}

void ArtworkLoader::load(const QString &source, const QSize &size, Callback callback) {
    if (source.isEmpty()) return;
    const QString key = source + QString(":%1x%2").arg(size.width()).arg(size.height());
    QPixmap cached;
    if (QPixmapCache::find(key, &cached)) {
        callback(cached);
        return;
    }

    if (QFile::exists(source)) {
        QPixmap local(source);
        if (!local.isNull()) {
            local = scaled(local, size);
            QPixmapCache::insert(key, local);
            callback(local);
        }
        return;
    }
    const QUrl url(source);
    if (!url.isValid() || !url.scheme().startsWith("http")) return;

    const QString diskPath = cache_path(source);
    QPixmap disk(diskPath);
    if (!disk.isNull()) {
        disk = scaled(disk, size);
        QPixmapCache::insert(key, disk);
        callback(disk);
        return;
    }

    QNetworkRequest request(url);
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                         QNetworkRequest::NoLessSafeRedirectPolicy);
    auto *reply = manager()->get(request);
    QObject::connect(reply, &QNetworkReply::finished, reply,
                     [reply, key, diskPath, size, callback = std::move(callback)]() {
        const QByteArray data = reply->readAll();
        reply->deleteLater();
        QPixmap downloaded;
        if (!downloaded.loadFromData(data)) return;
        QFile file(diskPath);
        if (file.open(QIODevice::WriteOnly)) file.write(data);
        downloaded = scaled(downloaded, size);
        QPixmapCache::insert(key, downloaded);
        callback(downloaded);
    });
}
