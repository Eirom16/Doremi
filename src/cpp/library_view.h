#ifndef DOREMI_LIBRARY_VIEW_H
#define DOREMI_LIBRARY_VIEW_H

#include <QWidget>
#include <QQuickWidget>
#include <QVariantList>
#include <vector>
#include <string>

#include "doremi/src/bridge.rs.h"

class LibraryView : public QWidget {
    Q_OBJECT
    Q_PROPERTY(QString activeTab READ activeTab NOTIFY tabChanged)
    Q_PROPERTY(bool authenticated READ isAuthenticated NOTIFY authenticatedChanged)
    Q_PROPERTY(QString libraryState READ libraryState NOTIFY libraryStateChanged)
    Q_PROPERTY(QString stateMessage READ stateMessage NOTIFY libraryStateChanged)
    
    Q_PROPERTY(QVariantList playlists READ playlists NOTIFY dataChanged)
    Q_PROPERTY(QVariantList songs READ songs NOTIFY dataChanged)
    Q_PROPERTY(QVariantList albums READ albums NOTIFY dataChanged)
    Q_PROPERTY(QVariantList artists READ artists NOTIFY dataChanged)
    Q_PROPERTY(QVariantList shows READ shows NOTIFY dataChanged)

public:
    explicit LibraryView(QWidget *parent = nullptr);
    void update_theme() {} // Kept for compatibility

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
    ) {} // Not used directly in LibraryView UI anymore, usually search results go to SearchView. Wait, the Rust side calls this? We'll just ignore for QML or map it.
    
    void set_library_state(const std::string &state, const std::string &message);
    void set_authenticated(bool authenticated);
    
    std::string current_tab() const { return active_tab_.toStdString(); }
    QString activeTab() const { return active_tab_; }
    bool isAuthenticated() const { return authenticated_; }
    QString libraryState() const { return library_state_; }
    QString stateMessage() const { return state_message_; }
    
    QVariantList playlists() const { return playlists_; }
    QVariantList songs() const { return songs_; }
    QVariantList albums() const { return albums_; }
    QVariantList artists() const { return artists_; }
    QVariantList shows() const { return shows_; }

    Q_INVOKABLE void requestTabChange(const QString &tab);
    Q_INVOKABLE void requestPlay(const QString &trackId);
    Q_INVOKABLE void requestPlaylist(const QString &playlistId);
    Q_INVOKABLE void requestAlbum(const QString &albumId);
    Q_INVOKABLE void requestArtist(const QString &artistId);
    Q_INVOKABLE void requestShow(const QString &showId);
    Q_INVOKABLE void requestCreatePlaylist();
    Q_INVOKABLE void requestLogin();
    Q_INVOKABLE void requestFilterSourceChange(int source);
    Q_INVOKABLE void requestRemoveFavorite(const QString &trackId);

signals:
    void tabChanged();
    void authenticatedChanged();
    void libraryStateChanged();
    void dataChanged();

    // Original signals
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
    QQuickWidget *quick_widget_ = nullptr;
    
    QString active_tab_ = "playlists";
    bool authenticated_ = false;
    QString library_state_ = "loading";
    QString state_message_ = "";
    
    QVariantList playlists_;
    QVariantList songs_;
    QVariantList albums_;
    QVariantList artists_;
    QVariantList shows_;
    
    std::vector<Track> raw_songs_;
    
    Track getTrackById(const QString &id);
};

#endif
