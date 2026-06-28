#include "library_view.h"
#include "design_tokens.h"
#include "icon_provider.h"
#include "components/loading_state.h"
#include "components/empty_state.h"
#include <QFrame>
#include <QHBoxLayout>
#include <QVariant>
#include <QPushButton>
#include <QLineEdit>
#include <QComboBox>

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
    {"shows", "shows"},
};
}

LibraryView::LibraryView(QWidget *parent)
    : QWidget(parent), active_tab_(""), authenticated_(false)
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
            tr_q(tab.translation_key),
            tab_bar);
        btn->setProperty("tabKey", QString::fromStdString(key));
        btn->setCheckable(true);
        btn->setFixedHeight(43); // 1px less to overlap with border-bottom
        btn->setCursor(Qt::PointingHandCursor);
        btn->setFont(DesignTokens::getFont("body_sm"));
        
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

    // Search bar + sort combo
    auto *search_row = new QHBoxLayout();
    search_row->setContentsMargins(24, 8, 24, 4);
    search_row->setSpacing(8);
    setup_search_bar();
    search_row->addWidget(search_box_, 1);
    search_row->addWidget(source_combo_);
    search_row->addWidget(sort_combo_);
    root->addLayout(search_row);

    list_ = new QVBoxLayout();
    list_->setContentsMargins(24, 16, 24, 16);
    list_->setSpacing(6);

    auto *placeholder = new QLabel(tr_q("library_empty_title"), this);
    placeholder->setFont(DesignTokens::getFont("body", 14));
    placeholder->setStyleSheet(QString("color: %1; padding: 24px;").arg(c.text_muted.name()));
    placeholder->setAlignment(Qt::AlignCenter);
    list_->addWidget(placeholder);
    list_->addStretch(1);

    root->addLayout(list_, 1);
    setStyleSheet("background: transparent;");
}

QWidget *LibraryView::make_list_item(const std::string &text, const std::string &sub, const std::string &id, const std::string &thumbnail) {
    auto *ci = new ClickableItem(text, sub, this);
    ci->set_item_id(id);
    ci->set_item_type(active_tab_ == "albums" ? "album" :
                      active_tab_ == "artists" ? "artist" :
                      active_tab_ == "playlists" ? "playlist" :
                      active_tab_ == "shows" ? "show" : "song");
    ci->set_thumbnail(thumbnail);
    
    connect(ci, &ClickableItem::clicked, this, [this, text, sub, id]() {
        if (active_tab_ == "shows") {
            emit show_requested(id);
        } else if (active_tab_ == "playlists") {
            emit playlist_requested(id);
        } else if (active_tab_ == "albums") {
            emit album_requested(id);
        } else if (active_tab_ == "artists") {
            emit artist_requested(id);
        } else {
            Track track;
            track.id = id;
            track.title = text;
            track.artist = sub;
            emit play_requested(track);
        }
    });
    connect(ci, &ClickableItem::context_action, this, [this, id](const std::string &action, const std::string &) {
        if (action == "add_favorite" || action == "remove_favorite") {
            if (active_tab_ == "songs") {
                emit remove_favorite_requested(id);
            } else if (active_tab_ == "albums") {
                emit remove_favorite_album_requested(id);
            } else if (active_tab_ == "artists") {
                emit remove_favorite_artist_requested(id);
            } else if (active_tab_ == "shows") {
                emit remove_favorite_show_requested(id);
            }
        }
    });
    return ci;
}

QWidget *LibraryView::make_song_item(const Track &track) {
    auto *ci = new ClickableItem(static_cast<std::string>(track.title), static_cast<std::string>(track.artist), this);
    ci->set_item_id(static_cast<std::string>(track.id));
    ci->set_item_type("song");
    ci->set_thumbnail(static_cast<std::string>(track.thumbnail));
    
    connect(ci, &ClickableItem::clicked, this, [this, track]() {
        emit play_requested(track);
    });
    connect(ci, &ClickableItem::context_action, this, [this, track](const std::string &action, const std::string &) {
        if (action == "add_favorite" || action == "remove_favorite") {
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

void LibraryView::setup_search_bar() {
    const auto &c = DesignTokens::current();

    search_box_ = new QLineEdit(this);
    search_box_->setPlaceholderText(tr_q("search_library_placeholder"));
    search_box_->setClearButtonEnabled(true);
    search_box_->setFixedHeight(36);
    search_box_->setStyleSheet(QString(
        "QLineEdit { background: %1; border: 1px solid %2; border-radius: %5px; padding: 0 16px; color: %3; font-size: 13px; }"
        "QLineEdit:focus { border-color: %4; }")
        .arg(c.bg_elevated.name()).arg(c.border.name()).arg(c.text_primary.name()).arg(c.accent.name()).arg(DesignTokens::radius().pill));

    source_combo_ = new QComboBox(this);
    source_combo_->addItem(tr_q("source_all"), 0);
    source_combo_->addItem(tr_q("source_cloud"), 1);
    source_combo_->addItem(tr_q("source_downloads"), 2);
    source_combo_->addItem(tr_q("source_local"), 3);
    source_combo_->setFixedHeight(32);
    source_combo_->setStyleSheet(QString(
        "QComboBox { background: %1; border: 1px solid %2; border-radius: %4px; padding: 0 12px; color: %3; font-size: 12px; }"
        "QComboBox::drop-down { border: none; width: 20px; }")
        .arg(c.bg_elevated.name()).arg(c.border.name()).arg(c.text_primary.name()).arg(DesignTokens::radius().xl));

    sort_combo_ = new QComboBox(this);
    sort_combo_->addItem(tr_q("sort_name_asc"), "name_asc");
    sort_combo_->addItem(tr_q("sort_name_desc"), "name_desc");
    sort_combo_->addItem(tr_q("sort_recent"), "recent");
    sort_combo_->addItem(tr_q("sort_oldest"), "oldest");
    sort_combo_->setFixedHeight(32);
    sort_combo_->setStyleSheet(QString(
        "QComboBox { background: %1; border: 1px solid %2; border-radius: %4px; padding: 0 12px; color: %3; font-size: 12px; }"
        "QComboBox::drop-down { border: none; width: 20px; }")
        .arg(c.bg_elevated.name()).arg(c.border.name()).arg(c.text_primary.name()).arg(DesignTokens::radius().xl));

    connect(search_box_, &QLineEdit::textChanged, this, [this](const QString &text) {
        emit search_requested(active_tab_, text.toStdString(), sort_combo_->currentData().toString().toStdString());
    });
    connect(source_combo_, &QComboBox::currentIndexChanged, this, [this](int) {
        int source_val = source_combo_->currentData().toInt();
        emit filter_source_changed(source_val);
        emit search_requested(active_tab_, search_box_->text().toStdString(), sort_combo_->currentData().toString().toStdString());
    });
    connect(sort_combo_, &QComboBox::currentIndexChanged, this, [this](int) {
        emit search_requested(active_tab_, search_box_->text().toStdString(), sort_combo_->currentData().toString().toStdString());
    });
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
    auto *create_btn = new QPushButton(tr_q("new_playlist"), this);
    create_btn->setCursor(Qt::PointingHandCursor);
    create_btn->setStyleSheet(QString(
        "QPushButton { background: %1; color: white; border: none; border-radius: %3px; padding: 10px 20px; font-size: 14px; font-weight: 600; }"
        "QPushButton:hover { background: %2; }"
    ).arg(c.accent.name()).arg(c.accent_bright.name()).arg(DesignTokens::radius().md));
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
    if (playlists.empty()) {
        auto *lbl = new QLabel(tr_q("no_playlists_message"), this);
        lbl->setFont(DesignTokens::getFont("body_sm"));
        lbl->setStyleSheet(QString("color: %1; padding: 12px;").arg(c.text_muted.name()));
        list_->addWidget(lbl);
    } else {
        for (const auto &p : playlists) {
            QString sub = QString("%1 %2").arg(p.track_count).arg(tr_q("songs").toLower());
            list_->addWidget(make_list_item(static_cast<std::string>(p.name), sub.toStdString(), static_cast<std::string>(p.id), static_cast<std::string>(p.thumbnail)));
        }
    }
    list_->addStretch(1);
    set_active_tab(active_tab_);
}

void LibraryView::set_songs(const std::vector<Track> &songs) {
    active_tab_ = "songs";
    clear_list();
    if (songs.empty()) {
        show_empty_state();
        set_active_tab(active_tab_);
        return;
    }
    for (const auto &t : songs) {
        list_->addWidget(make_song_item(t));
    }
    list_->addStretch(1);
    set_active_tab(active_tab_);
}

void LibraryView::set_albums(const std::vector<Album> &albums) {
    active_tab_ = "albums";
    clear_list();
    if (albums.empty()) {
        show_empty_state();
        set_active_tab(active_tab_);
        return;
    }
    for (const auto &a : albums) {
        list_->addWidget(make_list_item(static_cast<std::string>(a.title), static_cast<std::string>(a.artist), static_cast<std::string>(a.id), static_cast<std::string>(a.thumbnail)));
    }
    list_->addStretch(1);
    set_active_tab(active_tab_);
}

void LibraryView::set_artists(const std::vector<Artist> &artists) {
    active_tab_ = "artists";
    clear_list();
    if (artists.empty()) {
        show_empty_state();
        set_active_tab(active_tab_);
        return;
    }
    for (const auto &a : artists) {
        list_->addWidget(make_list_item(static_cast<std::string>(a.name), "", static_cast<std::string>(a.id), static_cast<std::string>(a.thumbnail)));
    }
    list_->addStretch(1);
    set_active_tab(active_tab_);
}

void LibraryView::set_shows(const std::vector<Show> &shows) {
    active_tab_ = "shows";
    clear_list();
    if (shows.empty()) {
        show_empty_state();
        set_active_tab(active_tab_);
        return;
    }
    for (const auto &s : shows) {
        auto *ci = make_list_item(
            static_cast<std::string>(s.title),
            static_cast<std::string>(s.author),
            static_cast<std::string>(s.id),
            static_cast<std::string>(s.thumbnail));
        list_->addWidget(ci);
    }
    list_->addStretch(1);
    set_active_tab(active_tab_);
}

std::string LibraryView::current_tab() const {
    return active_tab_;
}

void LibraryView::set_active_tab(const std::string &tab) {
    active_tab_ = tab;
    if (search_box_) search_box_->clear();
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
        if (songs.empty()) {
            show_empty_state();
            set_active_tab(tab);
            return;
        }
        for (const auto &t : songs) {
            list_->addWidget(make_song_item(t));
        }
    } else if (tab == "albums") {
        if (albums.empty()) {
            show_empty_state();
            set_active_tab(tab);
            return;
        }
        for (const auto &a : albums) {
            list_->addWidget(make_list_item(static_cast<std::string>(a.title), static_cast<std::string>(a.artist), static_cast<std::string>(a.id), static_cast<std::string>(a.thumbnail)));
        }
    } else if (tab == "artists") {
        if (artists.empty()) {
            show_empty_state();
            set_active_tab(tab);
            return;
        }
        for (const auto &a : artists) {
            list_->addWidget(make_list_item(static_cast<std::string>(a.name), "", static_cast<std::string>(a.id), static_cast<std::string>(a.thumbnail)));
        }
    } else if (tab == "playlists") {
        const auto &c = DesignTokens::current();
        auto *create_btn = new QPushButton(tr_q("new_playlist"), this);
        create_btn->setCursor(Qt::PointingHandCursor);
        create_btn->setStyleSheet(QString(
            "QPushButton { background: %1; color: white; border: none; border-radius: %3px; padding: 10px 20px; font-size: 14px; font-weight: 600; }"
            "QPushButton:hover { background: %2; }"
        ).arg(c.accent.name()).arg(c.accent_bright.name()).arg(DesignTokens::radius().md));
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
        if (playlists.empty()) {
            auto *lbl = new QLabel(tr_q("no_playlists_message"), this);
            lbl->setFont(DesignTokens::getFont("body_sm"));
            lbl->setStyleSheet(QString("color: %1; padding: 12px;").arg(c.text_muted.name()));
            list_->addWidget(lbl);
        } else {
            for (const auto &p : playlists) {
                QString sub = QString("%1 %2").arg(p.track_count).arg(tr_q("songs").toLower());
                list_->addWidget(make_list_item(static_cast<std::string>(p.name), sub.toStdString(), static_cast<std::string>(p.id), static_cast<std::string>(p.thumbnail)));
            }
        }
    }
    
    list_->addStretch(1);
    set_active_tab(tab);
}

void LibraryView::set_library_state(const std::string &state, const std::string &message) {
    clear_list();

    if (state == "loading") {
        auto *loading = new LoadingState(LoadingState::ListRows, this);
        list_->addStretch(1);
        list_->addWidget(loading);
        list_->addStretch(1);
        return;
    }

    auto *panel = new EmptyState(this);
    panel->applyPanelStyle(QString::fromStdString(state));

    QString iconName = state == "error" ? "error" :
                       state == "not_authenticated" ? "login" : "library_music";
    panel->setIcon(iconName);

    QString title = state == "error" ? "No se pudo cargar esta sección" :
                    state == "not_authenticated" ? tr_q("login_yt_music") :
                    tr_q("library_empty_title");
    panel->setTitle(title);
    panel->setDescription(QString::fromStdString(message));

    if (state == "not_authenticated") {
        auto *login_btn = panel->addButton(tr_q("login_yt_music"));
        connect(login_btn, &QPushButton::clicked, this, &LibraryView::login_requested);
    } else if (state == "error") {
        auto *retry_btn = panel->addButton("Reintentar");
        connect(retry_btn, &QPushButton::clicked, this, [this]() {
            emit tab_changed(active_tab_.empty() ? "playlists" : active_tab_);
        });
    }

    list_->addStretch(1);
    list_->addWidget(panel);
    list_->addStretch(1);
}

void LibraryView::set_authenticated(bool authenticated) {
    authenticated_ = authenticated;
}

void LibraryView::show_empty_state() {
    if (authenticated_) {
        set_library_state("empty", std::string(doremi_tr("library_empty_title")));
    } else {
        set_library_state("not_authenticated", std::string(doremi_tr("library_not_auth_desc")));
    }
}

void LibraryView::show_not_authenticated_state() {
    set_library_state("not_authenticated", std::string(doremi_tr("library_not_auth_title")));
}

void LibraryView::update_theme() {
    const auto &c = DesignTokens::current();
    if (search_box_) search_box_->setStyleSheet(DesignTokens::textInputStyle());
    if (source_combo_) source_combo_->setStyleSheet(DesignTokens::textInputStyle());
    if (sort_combo_) sort_combo_->setStyleSheet(DesignTokens::textInputStyle());
    for (auto *btn : tab_btns_) {
        if (!btn) continue;
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
    }
}
