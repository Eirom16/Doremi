#ifndef SHORTCUT_MANAGER_H
#define SHORTCUT_MANAGER_H

#include <QObject>

class DoremiMainWindow;

class ShortcutManager : public QObject {
    Q_OBJECT
public:
    explicit ShortcutManager(DoremiMainWindow *window);
};

#endif // SHORTCUT_MANAGER_H
