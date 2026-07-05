#ifndef DOREMI_UPDATE_DIALOG_H
#define DOREMI_UPDATE_DIALOG_H

#include <QDialog>
#include <QQuickWidget>
#include <QString>

class UpdateDialog : public QDialog {
    Q_OBJECT
    Q_PROPERTY(QString version READ version NOTIFY versionChanged)
    Q_PROPERTY(QString notes READ notes NOTIFY notesChanged)
    Q_PROPERTY(bool isDownloading READ isDownloading NOTIFY isDownloadingChanged)
    Q_PROPERTY(double downloadProgress READ downloadProgress NOTIFY downloadProgressChanged)
    Q_PROPERTY(QString statusMessage READ statusMessage NOTIFY statusMessageChanged)
    Q_PROPERTY(bool isInstallSuccess READ isInstallSuccess NOTIFY isInstallSuccessChanged)
    Q_PROPERTY(bool isInstallFailed READ isInstallFailed NOTIFY isInstallFailedChanged)
    Q_PROPERTY(bool isReadyToRestart READ isReadyToRestart NOTIFY isReadyToRestartChanged)

public:
    explicit UpdateDialog(QWidget *parent = nullptr);
    ~UpdateDialog() override;

    static void show_if_available(const QString &version, const QString &notes,
                                  const QString &url, const QString &asset_url,
                                  const QString &asset_name, qint64 asset_size);

    static UpdateDialog *active_instance() { return active_instance_; }

    void set_release_info(const QString &version, const QString &notes,
                          const QString &url, const QString &asset_url,
                          const QString &asset_name, qint64 asset_size);

    void set_download_progress(double percent, const QString &message);
    void set_download_finished(const QString &package_path);
    void set_download_failed(const QString &error);
    void set_install_finished(bool success);

    QString version() const { return version_; }
    QString notes() const { return notes_; }
    bool isDownloading() const { return downloading_; }
    double downloadProgress() const { return download_progress_; }
    QString statusMessage() const { return status_message_; }
    bool isInstallSuccess() const { return install_success_; }
    bool isInstallFailed() const { return install_failed_; }
    bool isReadyToRestart() const { return ready_to_restart_; }

    Q_INVOKABLE void requestDownload();
    Q_INVOKABLE void openGithub();
    Q_INVOKABLE void requestRestart();
    Q_INVOKABLE void requestClose() { reject(); }

signals:
    void versionChanged();
    void notesChanged();
    void isDownloadingChanged();
    void downloadProgressChanged();
    void statusMessageChanged();
    void isInstallSuccessChanged();
    void isInstallFailedChanged();
    void isReadyToRestartChanged();

private:
    void center_on_parent();
    void restart_app();

    static UpdateDialog *active_instance_;

    QQuickWidget *quick_widget_ = nullptr;

    QString version_;
    QString notes_;
    QString url_;
    QString asset_url_;
    QString asset_name_;
    qint64 asset_size_ = 0;
    QString package_path_;

    bool downloading_ = false;
    double download_progress_ = 0.0;
    QString status_message_;
    bool install_success_ = false;
    bool install_failed_ = false;
    bool ready_to_restart_ = false;
};

#endif // DOREMI_UPDATE_DIALOG_H
