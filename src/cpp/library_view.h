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
#include "doremi/src/bridge.rs.h"
#include "components/create_playlist_dialog.h"

class LibraryView : public QWidget {
    Q_OBJECT
public:
    explicit LibraryView(QWidget *parent = nullptr);
    void set_playlists(const std::vector<Playlist> &playlists);
    void set_songs(const std::vector<Track> &songs);
    void set_albums(const std::vector<Album> &albums);
    void set_artists(const std::vector<Artist> &artists);
    std::string current_tab() const;
signals:
    void tab_changed(const std::string &tab);
    void play_requested(Track track);
    void remove_favorite_requested(const std::string &track_id);
    void add_favorite_album_requested(const std::string &album_id);
    void remove_favorite_album_requested(const std::string &album_id);
    void add_favorite_artist_requested(const std::string &artist_id);
    void remove_favorite_artist_requested(const std::string &artist_id);
    void create_playlist_requested(const std::string &name, const std::string &description, const std::string &privacy);
    void download_requested(Track track);
    void add_to_queue_next_requested(Track track);
    void add_to_queue_end_requested(Track track);
private:
    QVBoxLayout *list_;
    std::vector<QPushButton *> tab_btns_;
    std::string active_tab_;
    void set_active_tab(const std::string &tab);
    QWidget *make_list_item(const std::string &text, const std::string &sub, const std::string &id);
    QWidget *make_song_item(const Track &track);
    void clear_list();
};

#endif
