#include "shortcut_manager.h"
#include "main_window.h"
#include "title_bar.h"
#include <QShortcut>
#include <QKeySequence>

ShortcutManager::ShortcutManager(DoremiMainWindow *window)
    : QObject(window)
{
    auto *space = new QShortcut(QKeySequence(Qt::Key_Space), window);
    QObject::connect(space, &QShortcut::activated, window, [window]() { emit window->play_pause_triggered(); });
    
    auto *right = new QShortcut(QKeySequence(Qt::Key_Right), window);
    QObject::connect(right, &QShortcut::activated, window, [window]() { emit window->seek_triggered(5000); });
    
    auto *left = new QShortcut(QKeySequence(Qt::Key_Left), window);
    QObject::connect(left, &QShortcut::activated, window, [window]() { emit window->seek_triggered(-5000); });
    
    auto *up = new QShortcut(QKeySequence(Qt::Key_Up), window);
    QObject::connect(up, &QShortcut::activated, window, [window]() { emit window->volume_changed(5); });
    
    auto *down = new QShortcut(QKeySequence(Qt::Key_Down), window);
    QObject::connect(down, &QShortcut::activated, window, [window]() { emit window->volume_changed(-5); });
    
    auto *focus_search = new QShortcut(QKeySequence("Ctrl+L"), window);
    focus_search->setWhatsThis("Enfocar el buscador");
    QObject::connect(focus_search, &QShortcut::activated, window, [window]() {
        if (window->title_bar()) window->title_bar()->focus_search();
    });
    
    auto *focus_search_alt = new QShortcut(QKeySequence("Ctrl+K"), window);
    focus_search_alt->setWhatsThis("Enfocar el buscador");
    QObject::connect(focus_search_alt, &QShortcut::activated, window, [window]() {
        if (window->title_bar()) window->title_bar()->focus_search();
    });
    
    auto *back = new QShortcut(QKeySequence::Back, window);
    back->setWhatsThis("Volver a la pantalla anterior");
    QObject::connect(back, &QShortcut::activated, window, [window]() { window->navigate_back(); });
    
    auto *forward = new QShortcut(QKeySequence::Forward, window);
    forward->setWhatsThis("Avanzar a la pantalla siguiente");
    QObject::connect(forward, &QShortcut::activated, window, [window]() { window->navigate_forward(); });

    // Media Keys
    auto *media_play = new QShortcut(QKeySequence(Qt::Key_MediaPlay), window);
    QObject::connect(media_play, &QShortcut::activated, window, [window]() { emit window->play_pause_triggered(); });
    
    auto *media_pause = new QShortcut(QKeySequence(Qt::Key_MediaPause), window);
    QObject::connect(media_pause, &QShortcut::activated, window, [window]() { emit window->play_pause_triggered(); });
    
    auto *media_toggle = new QShortcut(QKeySequence(Qt::Key_MediaTogglePlayPause), window);
    QObject::connect(media_toggle, &QShortcut::activated, window, [window]() { emit window->play_pause_triggered(); });
    
    auto *media_next = new QShortcut(QKeySequence(Qt::Key_MediaNext), window);
    QObject::connect(media_next, &QShortcut::activated, window, [window]() { emit window->next_triggered(); });
    
    auto *media_prev = new QShortcut(QKeySequence(Qt::Key_MediaPrevious), window);
    QObject::connect(media_prev, &QShortcut::activated, window, [window]() { emit window->previous_triggered(); });
}
