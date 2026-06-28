#include "tray_controller.h"
#include "design_tokens.h"
#include "main_window.h"
#include <QMenu>
#include <QApplication>
#include <QPainter>
#include <QPixmap>

TrayController::TrayController(DoremiMainWindow *window)
    : QObject(window), window_(window)
{
    QPixmap px(16, 16);
    px.fill(Qt::transparent);
    QPainter pt(&px);
    pt.setRenderHint(QPainter::Antialiasing);
    pt.setBrush(DesignTokens::current().accent);
    pt.setPen(Qt::NoPen);
    pt.drawEllipse(1, 1, 14, 14);
    pt.end();
    auto icon = QIcon(px);

    tray_icon_ = new QSystemTrayIcon(icon, this);
    tray_icon_->setToolTip("Doremi");

    auto *menu = new QMenu(window_);
    play_action_ = menu->addAction("▶ Reproducir");
    auto *next_action = menu->addAction("⏭ Siguiente");
    auto *prev_action = menu->addAction("⏮ Anterior");
    menu->addSeparator();
    auto *show_action = menu->addAction("Mostrar ventana");
    auto *quit_action = menu->addAction("Salir");

    QObject::connect(play_action_, &QAction::triggered, window_, [window]() { emit window->play_pause_triggered(); });
    QObject::connect(next_action, &QAction::triggered, window_, [window]() { emit window->next_triggered(); });
    QObject::connect(prev_action, &QAction::triggered, window_, [window]() { emit window->previous_triggered(); });
    QObject::connect(show_action, &QAction::triggered, window_, [window]() {
        window->show();
        window->raise();
        window->activateWindow();
    });
    QObject::connect(quit_action, &QAction::triggered, window_, []() {
        on_app_quit();
        QApplication::quit();
    });
    
    QObject::connect(tray_icon_, &QSystemTrayIcon::activated, window_,
        [window](QSystemTrayIcon::ActivationReason reason) {
            if (reason == QSystemTrayIcon::DoubleClick || reason == QSystemTrayIcon::Trigger) {
                window->show();
                window->raise();
                window->activateWindow();
            }
        });

    tray_icon_->setContextMenu(menu);
    tray_icon_->show();
}

void TrayController::setPlaying(bool playing) {
    if (play_action_) {
        play_action_->setText(playing ? "⏸ Pausa" : "▶ Reproducir");
    }
}

void TrayController::showMessage(const QString &title, const QString &message, QSystemTrayIcon::MessageIcon icon, int milliseconds) {
    if (tray_icon_ && tray_icon_->isVisible() && tray_icon_->supportsMessages()) {
        tray_icon_->showMessage(title, message, icon, milliseconds);
    }
}

bool TrayController::isVisible() const {
    return tray_icon_ && tray_icon_->isVisible();
}
