#ifndef DOREMI_DOWNLOADS_VIEW_H
#define DOREMI_DOWNLOADS_VIEW_H

#include <QWidget>
#include <QQuickWidget>
#include <QVariantList>
#include <QVariantMap>
#include <vector>
#include <string>

#include "doremi/src/bridge.rs.h"

class DownloadsView : public QWidget {
    Q_OBJECT
    Q_PROPERTY(QVariantList downloads READ downloads NOTIFY downloadsChanged)
    Q_PROPERTY(QString activeTab READ activeTab NOTIFY tabChanged)

public:
    explicit DownloadsView(QWidget *parent = nullptr);
    void update_theme() {} // Kept for compatibility

    void set_downloads(const std::vector<DownloadItem> &items);
    void set_progress(const std::string &video_id, double percent, const std::string &status);
    void set_batch_progress(const std::string &parent_id, int total, int completed, double percent);
    void clear_downloads();
    void update_view() {} // no-op now, driven by state

    QVariantList downloads() const { return downloads_list_; }
    QString activeTab() const { return active_tab_; }

    Q_INVOKABLE void requestPlay(const QString &trackId);
    Q_INVOKABLE void requestPlaylist(const QString &playlistId);
    Q_INVOKABLE void requestAlbum(const QString &albumId);
    Q_INVOKABLE void requestShow(const QString &showId);
    Q_INVOKABLE void requestTabChange(const QString &tab);

signals:
    void downloadsChanged();
    void tabChanged();
    void progressUpdated(const QString &id, double percent, const QString &status);
    void batchProgressUpdated(const QString &id, int total, int completed, double percent);

    // Original signals
    void play_requested(Track track);
    void playlist_requested(const std::string &playlist_id);
    void album_requested(const std::string &album_id);
    void show_requested(const std::string &show_id);

private:
    QQuickWidget *quick_widget_ = nullptr;
    
    QVariantList downloads_list_;
    std::vector<DownloadItem> raw_items_;
    QString active_tab_ = "all";
};

#endif
