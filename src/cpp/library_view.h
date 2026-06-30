#ifndef DOREMI_LIBRARY_VIEW_H
#define DOREMI_LIBRARY_VIEW_H

#include <QWidget>
#include <QVBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QLineEdit>
#include <QScrollArea>
#include <QComboBox>
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
    void set_shows(const std::vector<Show> &shows);
    void set_search_results(
        const std::string &tab,
        const std::vector<Track> &songs,
        const std::vector<Album> &albums,
        const std::vector<Artist> &artists,
        const std::vector<Playlist> &playlists
    );
    void set_library_state(const std::string &state, const std::string &message);
    void set_authenticated(bool authenticated);
    std::string current_tab() const;
signals:
    void tab_changed(const std::string &tab);
    void play_requested(Track track);
    void playlist_requested(const std::string &playlist_id);
    void album_requested(const std::string &album_id);
    void artist_requested(const std::string &artist_id);
    void remove_favorite_requested(const std::string &track_id);
    void add_favorite_album_requested(const std::string &album_id);
    void remove_favorite_album_requested(const std::string &album_id);
    void add_favorite_artist_requested(const std::string &artist_id);
    void remove_favorite_artist_requested(const std::string &artist_id);
    void remove_favorite_show_requested(const std::string &show_id);
    void show_requested(const std::string &browse_id);
    void create_playlist_requested(const std::string &name, const std::string &description, const std::string &privacy);
    void download_requested(Track track);
    void add_to_queue_next_requested(Track track);
    void add_to_queue_end_requested(Track track);
    void search_requested(const std::string &tab, const std::string &query, const std::string &sort_by);
    void filter_source_changed(int source);
    void login_requested();
private:
    QVBoxLayout *list_;
    QLineEdit *search_box_;
    QComboBox *sort_combo_;
    QComboBox *source_combo_;
    std::vector<QPushButton *> tab_btns_;
    std::string active_tab_;
    bool authenticated_;
    void set_active_tab(const std::string &tab);
    void setup_search_bar();
    QWidget *make_list_item(const std::string &text, const std::string &sub, const std::string &id, const std::string &thumbnail);
    QWidget *make_song_item(const Track &track);
    void clear_list();
    void show_empty_state();
    void show_not_authenticated_state();
};

#endif
