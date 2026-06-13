#ifndef DOREMI_UPDATE_DIALOG_H
#define DOREMI_UPDATE_DIALOG_H

#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QTextEdit>
#include <QProgressBar>
#include <QFrame>
#include "components/ripple_button.h"

class UpdateDialog : public QDialog {
    Q_OBJECT
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

private slots:
    void on_update_clicked();
    void on_github_clicked();

private:
    void center_on_parent();
    void build_ui();
    void restart_app();

    static UpdateDialog *active_instance_;

    QString version_;
    QString notes_;
    QString url_;
    QString asset_url_;
    QString asset_name_;
    qint64 asset_size_ = 0;
    QString package_path_;
    bool downloading_ = false;

    QFrame *panel_ = nullptr;
    QLabel *title_lbl_ = nullptr;
    QLabel *version_lbl_ = nullptr;
    QLabel *notes_label_ = nullptr;
    QTextEdit *notes_box_ = nullptr;
    
    QWidget *progress_container_ = nullptr;
    QProgressBar *progress_bar_ = nullptr;
    QLabel *progress_label_ = nullptr;

    RippleButton *github_btn_ = nullptr;
    RippleButton *postpone_btn_ = nullptr;
    RippleButton *update_btn_ = nullptr;
};

#endif // DOREMI_UPDATE_DIALOG_H
