#ifndef TRAY_CONTROLLER_H
#define TRAY_CONTROLLER_H

#include <QObject>
#include <QSystemTrayIcon>
#include <QAction>

class DoremiMainWindow;

class TrayController : public QObject {
    Q_OBJECT
public:
    explicit TrayController(DoremiMainWindow *window);
    void setPlaying(bool playing);
    void showMessage(const QString &title, const QString &message, QSystemTrayIcon::MessageIcon icon, int milliseconds);
    bool isVisible() const;
private:
    DoremiMainWindow *window_;
    QSystemTrayIcon *tray_icon_;
    QAction *play_action_ = nullptr;
};

#endif // TRAY_CONTROLLER_H
