#ifndef DOREMI_LIBRARY_VIEW_H
#define DOREMI_LIBRARY_VIEW_H

#include <QWidget>
#include <QVBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QScrollArea>
#include <vector>
#include <string>

#include "widgets.h"

class LibraryView : public QWidget {
    Q_OBJECT
public:
    explicit LibraryView(QWidget *parent = nullptr);
    void set_playlists(const std::vector<std::string> &names,
                       const std::vector<std::string> &counts);
    void set_songs(const std::vector<std::string> &titles);
    void set_albums(const std::vector<std::string> &titles,
                    const std::vector<std::string> &artists);
    void set_artists(const std::vector<std::string> &names);
    std::string current_tab() const;
signals:
    void tab_changed(const std::string &tab);
    void play_requested(const std::string &title_artist);
    void remove_favorite_requested(const std::string &item_id);
    void download_requested(const std::string &title_artist);
private:
    QVBoxLayout *list_;
    std::vector<QPushButton *> tab_btns_;
    std::string active_tab_;
    void set_active_tab(const std::string &tab);
    QWidget *make_list_item(const std::string &text, const std::string &sub);
    void clear_list();
};

#endif
