#include "library_view.h"
#include "design_tokens.h"
#include "icon_provider.h"
#include <QFrame>
#include <QHBoxLayout>
#include <QVariant>
#include <QPushButton>

namespace {
struct LibraryTabSpec {
    const char *key;
    const char *translation_key;
};

constexpr LibraryTabSpec kLibraryTabs[] = {
    {"playlists", "playlists"},
    {"songs", "songs"},
    {"albums", "albums"},
    {"artists", "artists"},
};
}

LibraryView::LibraryView(QWidget *parent)
    : QWidget(parent)
{
    const auto &c = DesignTokens::current();

    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    auto *tab_bar = new QWidget(this);
    tab_bar->setFixedHeight(44);
    
    // Bottom border under tab bar
    tab_bar->setStyleSheet(QString("background-color: transparent; border-bottom: 1px solid %1;")
        .arg(QString("rgba(%1, %2, %3, %4)").arg(c.border.red()).arg(c.border.green()).arg(c.border.blue()).arg(c.border.alpha() / 255.0))
    );
    
    auto *tab_lay = new QHBoxLayout(tab_bar);
    tab_lay->setContentsMargins(24, 0, 24, 0);
    tab_lay->setSpacing(8);
    
    for (const auto &tab : kLibraryTabs) {
        const std::string key(tab.key);
        auto *btn = new QPushButton(
            QString::fromStdString(std::string(doremi_tr(tab.translation_key))),
            tab_bar);
        btn->setProperty("tabKey", QString::fromStdString(key));
        btn->setCheckable(true);
        btn->setFixedHeight(43); // 1px less to overlap with border-bottom
        btn->setCursor(Qt::PointingHandCursor);
        btn->setFont(DesignTokens::getFont("body", 13));
        
        QString btnStyle = QString(
            "QPushButton {\n"
            "    background: transparent;\n"
            "    border: none;\n"
            "    border-bottom: 2px solid transparent;\n"
            "    color: %1;\n"
            "    padding: 0 12px;\n"
            "}\n"
            "QPushButton:hover {\n"
            "    color: %2;\n"
            "}\n"
            "QPushButton:checked {\n"
            "    color: %3;\n"
            "    border-bottom: 2px solid %3;\n"
            "    font-weight: 500;\n"
            "}\n"
        )
        .arg(c.text_secondary.name())
        .arg(c.text_primary.name())
        .arg(c.accent.name());
        
        btn->setStyleSheet(btnStyle);
        tab_lay->addWidget(btn);
        tab_btns_.push_back(btn);
        connect(btn, &QPushButton::clicked, this, [this, key]() { emit tab_changed(key); });
    }
    tab_lay->addStretch(1);
    root->addWidget(tab_bar);

    auto *scroll = new QScrollArea(this);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setStyleSheet("background: transparent; border: none;");

    auto *inner = new QWidget();
    inner->setStyleSheet("background: transparent;");
    list_ = new QVBoxLayout(inner);
    list_->setContentsMargins(24, 16, 24, 16);
    list_->setSpacing(6);

    auto *placeholder = new QLabel("Tu biblioteca está vacía", inner);
    placeholder->setFont(DesignTokens::getFont("body", 14));
    placeholder->setStyleSheet(QString("color: %1; padding: 24px;").arg(c.text_muted.name()));
    placeholder->setAlignment(Qt::AlignCenter);
    list_->addWidget(placeholder);
    list_->addStretch(1);

    scroll->setWidget(inner);
    root->addWidget(scroll, 1);
    setStyleSheet("background: transparent;");
}

QWidget *LibraryView::make_list_item(const std::string &text, const std::string &sub, const std::string &id) {
    auto *ci = new ClickableItem(text, sub, this);
    ci->set_item_id(id);
    ci->set_item_type(active_tab_ == "albums" ? "album" :
                      active_tab_ == "artists" ? "artist" :
                      active_tab_ == "playlists" ? "playlist" : "song");
    
    // Find icon label and set special icon based on active tab
    auto labels = ci->findChildren<QLabel*>();
    for (auto *label : labels) {
        if (label->font().family() == "Material Symbols Rounded") {
            const auto &c = DesignTokens::current();
            QString iconName = "music_note";
            if (active_tab_ == "playlists") iconName = "queue_music";
            else if (active_tab_ == "albums") iconName = "album";
            else if (active_tab_ == "artists") iconName = "person";
            
            label->setPixmap(IconProvider::getIcon(iconName, c.text_secondary, 18).pixmap(36, 36));
        }
    }
    
    connect(ci, &ClickableItem::clicked, this, [this, text, sub, id]() {
        Track track;
        track.id = id;
        track.title = text;
        track.artist = sub;
        emit play_requested(track);
    });
    connect(ci, &ClickableItem::context_action, this, [this, id](const std::string &action, const std::string &) {
        if (action == "add_favorite") {
            if (active_tab_ == "songs") {
                emit remove_favorite_requested(id);
            } else if (active_tab_ == "albums") {
                // Add -> Remove (toggle)
                emit remove_favorite_album_requested(id);
            } else if (active_tab_ == "artists") {
                emit remove_favorite_artist_requested(id);
            }
        }
    });
    return ci;
}

QWidget *LibraryView::make_song_item(const Track &track) {
    auto *ci = new ClickableItem(static_cast<std::string>(track.title), static_cast<std::string>(track.artist), this);
    ci->set_item_id(static_cast<std::string>(track.id));
    
    auto labels = ci->findChildren<QLabel*>();
    for (auto *label : labels) {
        if (label->font().family() == "Material Symbols Rounded") {
            const auto &c = DesignTokens::current();
            label->setPixmap(IconProvider::getIcon("music_note", c.text_secondary, 18).pixmap(36, 36));
        }
    }
    
    connect(ci, &ClickableItem::clicked, this, [this, track]() {
        emit play_requested(track);
    });
    connect(ci, &ClickableItem::context_action, this, [this, track](const std::string &action, const std::string &) {
        if (action == "add_favorite") {
            emit remove_favorite_requested(static_cast<std::string>(track.id));
        } else if (action == "download") {
            emit download_requested(track);
        } else if (action == "queue_next") {
            emit add_to_queue_next_requested(track);
        } else if (action == "queue_end") {
            emit add_to_queue_end_requested(track);
        }
    });
    return ci;
}

void LibraryView::clear_list() {
    while (list_->count() > 0) {
        auto *item = list_->takeAt(0);
        if (item->widget()) {
            item->widget()->deleteLater();
        }
        delete item;
    }
}

void LibraryView::set_playlists(const std::vector<Playlist> &playlists) {
    active_tab_ = "playlists";
    clear_list();
    const auto &c = DesignTokens::current();
    auto *create_btn = new QPushButton("+ Nueva playlist", this);
    create_btn->setCursor(Qt::PointingHandCursor);
    create_btn->setStyleSheet(QString(
        "QPushButton { background: %1; color: white; border: none; border-radius: 8px; padding: 10px 20px; font-size: 14px; font-weight: 600; }"
        "QPushButton:hover { background: %2; }"
    ).arg(c.accent.name()).arg(c.accent_bright.name()));
    connect(create_btn, &QPushButton::clicked, this, [this]() {
        CreatePlaylistDialog dlg(this);
        if (dlg.exec() == QDialog::Accepted) {
            emit create_playlist_requested(
                dlg.playlistName().toStdString(),
                dlg.description().toStdString(),
                dlg.privacy().toStdString()
            );
        }
    });
    list_->addWidget(create_btn);
    list_->addSpacing(8);
    for (const auto &p : playlists) {
        list_->addWidget(make_list_item(static_cast<std::string>(p.name), std::to_string(p.track_count) + " canciones", static_cast<std::string>(p.id)));
    }
    list_->addStretch(1);
    set_active_tab(active_tab_);
}

void LibraryView::set_songs(const std::vector<Track> &songs) {
    active_tab_ = "songs";
    clear_list();
    for (const auto &t : songs) {
        list_->addWidget(make_song_item(t));
    }
    list_->addStretch(1);
    set_active_tab(active_tab_);
}

void LibraryView::set_albums(const std::vector<Album> &albums) {
    active_tab_ = "albums";
    clear_list();
    for (const auto &a : albums) {
        list_->addWidget(make_list_item(static_cast<std::string>(a.title), static_cast<std::string>(a.artist), static_cast<std::string>(a.id)));
    }
    list_->addStretch(1);
    set_active_tab(active_tab_);
}

void LibraryView::set_artists(const std::vector<Artist> &artists) {
    active_tab_ = "artists";
    clear_list();
    for (const auto &a : artists) {
        list_->addWidget(make_list_item(static_cast<std::string>(a.name), "", static_cast<std::string>(a.id)));
    }
    list_->addStretch(1);
    set_active_tab(active_tab_);
}

std::string LibraryView::current_tab() const {
    return active_tab_;
}

void LibraryView::set_active_tab(const std::string &tab) {
    for (auto *btn : tab_btns_) {
        btn->setChecked(btn->property("tabKey").toString().toStdString() == tab);
    }
}

void LibraryView::set_search_results(
    const std::string &tab,
    const std::vector<Track> &songs,
    const std::vector<Album> &albums,
    const std::vector<Artist> &artists,
    const std::vector<Playlist> &playlists
) {
    active_tab_ = tab;
    clear_list();
    
    if (tab == "songs") {
        for (const auto &t : songs) {
            list_->addWidget(make_song_item(t));
        }
    } else if (tab == "albums") {
        for (const auto &a : albums) {
            list_->addWidget(make_list_item(static_cast<std::string>(a.title), static_cast<std::string>(a.artist), static_cast<std::string>(a.id)));
        }
    } else if (tab == "artists") {
        for (const auto &a : artists) {
            list_->addWidget(make_list_item(static_cast<std::string>(a.name), "", static_cast<std::string>(a.id)));
        }
    } else if (tab == "playlists") {
        for (const auto &p : playlists) {
            list_->addWidget(make_list_item(static_cast<std::string>(p.name), std::to_string(p.track_count) + " canciones", static_cast<std::string>(p.id)));
        }
    }
    
    list_->addStretch(1);
    set_active_tab(tab);
}

void LibraryView::set_library_state(const std::string &state, const std::string &message) {
    clear_list();
    
    auto *placeholder = new QLabel(QString::fromStdString(message), this);
    const auto &c = DesignTokens::current();
    placeholder->setFont(DesignTokens::getFont("body", 14));
    placeholder->setStyleSheet(QString("color: %1; padding: 24px;").arg(c.text_muted.name()));
    placeholder->setAlignment(Qt::AlignCenter);
    placeholder->setWordWrap(true);
    
    list_->addWidget(placeholder);
    
    // Show action button if not authenticated
    if (state == "not_authenticated") {
        auto *login_btn = new QPushButton("Iniciar sesión en YouTube Music", this);
        login_btn->setCursor(Qt::PointingHandCursor);
        login_btn->setStyleSheet(QString(
            "QPushButton { background: %1; color: white; border: none; border-radius: 8px; padding: 10px 20px; font-size: 14px; font-weight: 600; }"
            "QPushButton:hover { background: %2; }"
        ).arg(c.accent.name()).arg(c.accent_bright.name()));
        list_->addWidget(login_btn);
    }
    
    list_->addStretch(1);
}

void LibraryView::set_authenticated(bool authenticated) {
    authenticated_ = authenticated;
}

void LibraryView::show_empty_state() {
    set_library_state("empty", "Tu biblioteca está vacía");
}

void LibraryView::show_not_authenticated_state() {
    set_library_state("not_authenticated", "Inicia sesión en YouTube Music para acceder a tu biblioteca");
}
