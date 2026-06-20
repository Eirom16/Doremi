#include <QFrame>
#include "search_view.h"
#include "design_tokens.h"
#include "icon_provider.h"

SearchView::SearchView(QWidget *parent)
    : QWidget(parent)
{
    const auto &c = DesignTokens::current();

    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(24, 24, 24, 24);
    root->setSpacing(16);

    header_ = new QLabel("Resultados", this);
    header_->setFont(DesignTokens::getFont("heading_lg", 20));
    header_->setStyleSheet(QString("color: %1; font-weight: bold; background: transparent;").arg(c.text_primary.name()));
    root->addWidget(header_);

    filters_ = new QHBoxLayout();
    filters_->setSpacing(8);
    const std::vector<std::pair<const char *, const char *>> filterDefinitions = {
        {"Todo", "all"}, {"Canciones", "songs"}, {"Videos", "videos"},
        {"Álbumes", "albums"}, {"Artistas", "artists"}, {"Playlists", "playlists"},
        {"Podcasts", "podcasts"}
    };
    for (const auto &[name, filterValue] : filterDefinitions) {
        auto *btn = new QPushButton(name, this);
        btn->setCheckable(true);
        btn->setFixedHeight(32);
        btn->setCursor(Qt::PointingHandCursor);
        btn->setFont(DesignTokens::getFont("body", 12));
        btn->setFocusPolicy(Qt::StrongFocus);
        btn->setAccessibleName(QString("Filtrar por ") + name);
        
        QString btnStyle = QString(
            "QPushButton {\n"
            "    background-color: %1;\n"
            "    border: 1px solid %2;\n"
            "    border-radius: 16px;\n"
            "    padding: 0 16px;\n"
            "    color: %3;\n"
            "}\n"
            "QPushButton:hover {\n"
            "    background-color: %4;\n"
            "    color: %5;\n"
            "}\n"
            "QPushButton:checked {\n"
            "    background-color: %6;\n"
            "    border-color: %6;\n"
            "    color: #FFFFFF;\n"
            "    font-weight: 500;\n"
            "}\n"
        )
        .arg(c.bg_surface.name())
        .arg(c.border.name())
        .arg(c.text_secondary.name())
        .arg(QString("rgba(%1, %2, %3, %4)").arg(c.accent_dim.red()).arg(c.accent_dim.green()).arg(c.accent_dim.blue()).arg(c.accent_dim.alpha() / 255.0))
        .arg(c.text_primary.name())
        .arg(c.accent.name());

        btn->setStyleSheet(btnStyle);
        filters_->addWidget(btn);
        filter_btns_.push_back(btn);
        btn->setProperty("filterValue", filterValue);
        connect(btn, &QPushButton::clicked, this, [this, btn, filterValue]() {
            for (auto *other : filter_btns_) {
                other->setChecked(other == btn);
            }
            emit search_requested(current_query_, filterValue);
        });
    }
    filter_btns_.front()->setChecked(true);
    filters_->addStretch(1);
    root->addLayout(filters_);

    results_ = new QVBoxLayout();
    results_->setSpacing(6);
    results_->setContentsMargins(0, 4, 0, 4);

    auto *placeholder = new QLabel("Escribe algo para buscar", this);
    placeholder->setFont(DesignTokens::getFont("body", 14));
    placeholder->setStyleSheet(QString("color: %1; padding: 24px;").arg(c.text_muted.name()));
    placeholder->setAlignment(Qt::AlignCenter);
    results_->addWidget(placeholder);
    results_->addStretch(1);

    root->addLayout(results_, 1);
    setStyleSheet("background: transparent;");
}

void SearchView::set_query(const std::string &query) {
    current_query_ = query;
    header_->setText("Resultados para \"" + QString::fromStdString(query) + "\"");
    for (auto *btn : filter_btns_) {
        btn->setChecked(btn->property("filterValue").toString() == "all");
    }
}

static void clear_layout(QVBoxLayout *lay) {
    while (lay->count() > 0) {
        auto *item = lay->takeAt(0);
        if (item->widget()) {
            item->widget()->deleteLater();
        }
        delete item;
    }
}

void SearchView::show_results(const TopResult &top_result, bool has_top_result,
                              const std::vector<Track> &songs,
                              const std::vector<Track> &videos,
                              const std::vector<Artist> &artists,
                              const std::vector<Album> &albums,
                              const std::vector<Playlist> &playlists,
                              const std::vector<Show> &shows,
                              const std::vector<Episode> &episodes) {
    showing_recent_ = false;
    clear_layout(results_);

    const auto &c = DesignTokens::current();

    bool has_any = has_top_result || !songs.empty() || !videos.empty() || !artists.empty() || !albums.empty() || !playlists.empty() || !shows.empty() || !episodes.empty();
    if (!has_any) {
        auto *empty = new QLabel("Sin resultados", this);
        empty->setFont(DesignTokens::getFont("body", 14));
        empty->setStyleSheet(QString("color: %1; padding: 24px;").arg(c.text_muted.name()));
        empty->setAlignment(Qt::AlignCenter);
        results_->addWidget(empty);
        results_->addStretch(1);
        return;
    }

    // Top result: surface the single most relevant hit prominently.
    {
        auto *sec_header = new QLabel("Mejor resultado", this);
        sec_header->setFont(DesignTokens::getFont("micro", 11));
        sec_header->setStyleSheet(QString("color: %1; text-transform: uppercase; padding: 6px 12px 6px;")
            .arg(c.accent.name()));

        ClickableItem *top = nullptr;
        if (has_top_result) {
            std::string label = "Resultado principal";
            if (top_result.item_type == "artist") label = "Artista";
            else if (top_result.item_type == "album") label = "Álbum";
            else if (top_result.item_type == "song") label = "Canción";
            else if (top_result.item_type == "video") label = "Video";
            else if (top_result.item_type == "playlist") label = "Playlist";
            else if (top_result.item_type == "show") label = "Podcast";

            top = new ClickableItem(
                static_cast<std::string>(top_result.title),
                QString("%1 • %2").arg(QString::fromStdString(label)).arg(QString::fromStdString(static_cast<std::string>(top_result.subtitle))).toStdString(),
                this
            );
            top->set_item_id(static_cast<std::string>(top_result.id));
            top->set_item_type(static_cast<std::string>(top_result.item_type));

            if (top_result.item_type == "artist") {
                connect(top, &ClickableItem::clicked, this, [this, id = static_cast<std::string>(top_result.id)]() {
                    emit artist_requested(id);
                });
                connect(top, &ClickableItem::context_action, this, [this, id = static_cast<std::string>(top_result.id), title = static_cast<std::string>(top_result.title), thumb = static_cast<std::string>(top_result.thumbnail)](const std::string &action, const std::string &) {
                    if (action == "add_favorite") {
                        Artist a;
                        a.id = id;
                        a.name = title;
                        a.thumbnail = thumb;
                        on_add_favorite_artist(a);
                    } else if (action == "remove_favorite") {
                        on_remove_favorite_artist(id);
                    }
                });
            } else if (top_result.item_type == "album") {
                connect(top, &ClickableItem::clicked, this, [this, id = static_cast<std::string>(top_result.id)]() {
                    emit album_requested(id);
                });
                connect(top, &ClickableItem::context_action, this, [this, id = static_cast<std::string>(top_result.id), title = static_cast<std::string>(top_result.title), artist = static_cast<std::string>(top_result.subtitle), thumb = static_cast<std::string>(top_result.thumbnail)](const std::string &action, const std::string &) {
                    if (action == "add_favorite") {
                        Album al;
                        al.id = id;
                        al.title = title;
                        al.artist = artist;
                        al.thumbnail = thumb;
                        on_add_favorite_album(al);
                    } else if (action == "remove_favorite") {
                        on_remove_favorite_album(id);
                    }
                });
            } else if (top_result.item_type == "playlist") {
                connect(top, &ClickableItem::clicked, this, [this, id = static_cast<std::string>(top_result.id)]() {
                    emit playlist_requested(id);
                });
            } else if (top_result.item_type == "show") {
                connect(top, &ClickableItem::clicked, this, [this, id = static_cast<std::string>(top_result.id)]() {
                    emit show_requested(id);
                });
            } else if (top_result.item_type == "song" || top_result.item_type == "video") {
                Track t;
                t.id = top_result.id;
                t.title = top_result.title;
                t.artist = top_result.subtitle;
                t.thumbnail = top_result.thumbnail;
                t.duration_ms = 0;
                
                connect(top, &ClickableItem::clicked, this, [this, t]() { emit play_requested(t); });
                connect(top, &ClickableItem::context_action, this, [this, t](const std::string &action, const std::string &) {
                    if (action == "add_favorite") emit add_favorite_requested(t);
                    else if (action == "remove_favorite") on_remove_favorite(static_cast<std::string>(t.id));
                    else if (action == "download") emit download_requested(t);
                    else if (action == "queue_next") emit add_to_queue_next_requested(t);
                    else if (action == "queue_end") emit add_to_queue_end_requested(t);
                });
            }
        } else {
            // Heuristic fallback
            if (!artists.empty()) {
                const auto &a = artists.front();
                top = new ClickableItem(static_cast<std::string>(a.name), "Artista", this);
                top->set_item_id(static_cast<std::string>(a.id));
                top->set_item_type("artist");
                connect(top, &ClickableItem::clicked, this, [this, a]() {
                    emit artist_requested(static_cast<std::string>(a.id));
                });
                connect(top, &ClickableItem::context_action, this, [this, a](const std::string &action, const std::string &) {
                    if (action == "add_favorite") {
                        on_add_favorite_artist(a);
                    } else if (action == "remove_favorite") {
                        on_remove_favorite_artist(static_cast<std::string>(a.id));
                    }
                });
            } else if (!albums.empty()) {
                const auto &al = albums.front();
                top = new ClickableItem(static_cast<std::string>(al.title), static_cast<std::string>(al.artist), this);
                top->set_item_id(static_cast<std::string>(al.id));
                top->set_item_type("album");
                connect(top, &ClickableItem::clicked, this, [this, al]() {
                    emit album_requested(static_cast<std::string>(al.id));
                });
                connect(top, &ClickableItem::context_action, this, [this, al](const std::string &action, const std::string &) {
                    if (action == "add_favorite") {
                        on_add_favorite_album(al);
                    } else if (action == "remove_favorite") {
                        on_remove_favorite_album(static_cast<std::string>(al.id));
                    }
                });
            } else if (!songs.empty()) {
                const auto &t = songs.front();
                top = new ClickableItem(static_cast<std::string>(t.title), static_cast<std::string>(t.artist), this);
                top->set_item_id(static_cast<std::string>(t.id));
                top->set_item_type("song");
                connect(top, &ClickableItem::clicked, this, [this, t]() { emit play_requested(t); });
                connect(top, &ClickableItem::context_action, this, [this, t](const std::string &action, const std::string &) {
                    if (action == "add_favorite") emit add_favorite_requested(t);
                    else if (action == "remove_favorite") on_remove_favorite(static_cast<std::string>(t.id));
                    else if (action == "download") emit download_requested(t);
                    else if (action == "queue_next") emit add_to_queue_next_requested(t);
                    else if (action == "queue_end") emit add_to_queue_end_requested(t);
                });
            }
        }
        if (top) {
            results_->addWidget(sec_header);
            results_->addWidget(top);
        } else {
            sec_header->deleteLater();
        }
    }

    if (!songs.empty()) {
        auto *sec_header = new QLabel("Canciones", this);
        sec_header->setFont(DesignTokens::getFont("micro", 11));
        sec_header->setStyleSheet(QString("color: %1; text-transform: uppercase; padding: 12px 12px 6px;")
            .arg(c.accent.name()));
        results_->addWidget(sec_header);
        for (const auto &track : songs) {
            auto *ci = new ClickableItem(static_cast<std::string>(track.title), static_cast<std::string>(track.artist), this);
            ci->set_item_id(static_cast<std::string>(track.id));
            ci->set_item_type("song");
            results_->addWidget(ci);
            
            connect(ci, &ClickableItem::clicked, this, [this, track]() {
                emit play_requested(track);
            });
            connect(ci, &ClickableItem::context_action, this, [this, track](const std::string &action, const std::string &) {
                if (action == "add_favorite") {
                    emit add_favorite_requested(track);
                } else if (action == "remove_favorite") {
                    on_remove_favorite(static_cast<std::string>(track.id));
                } else if (action == "download") {
                    emit download_requested(track);
                } else if (action == "queue_next") {
                    emit add_to_queue_next_requested(track);
                } else if (action == "queue_end") {
                    emit add_to_queue_end_requested(track);
                }
            });
        }
    }

    if (!videos.empty()) {
        auto *sec_header = new QLabel("Videos", this);
        sec_header->setFont(DesignTokens::getFont("micro", 11));
        sec_header->setStyleSheet(QString("color: %1; text-transform: uppercase; padding: 12px 12px 6px;")
            .arg(c.accent.name()));
        results_->addWidget(sec_header);
        for (const auto &track : videos) {
            auto *item = new ClickableItem(static_cast<std::string>(track.title),
                                           static_cast<std::string>(track.artist), this);
            item->set_item_id(static_cast<std::string>(track.id));
            item->set_item_type("video");
            results_->addWidget(item);
            connect(item, &ClickableItem::clicked, this, [this, track]() {
                emit play_requested(track);
            });
            connect(item, &ClickableItem::context_action, this,
                    [this, track](const std::string &action, const std::string &) {
                if (action == "add_favorite") emit add_favorite_requested(track);
                else if (action == "remove_favorite") on_remove_favorite(static_cast<std::string>(track.id));
                else if (action == "download") emit download_requested(track);
                else if (action == "queue_next") emit add_to_queue_next_requested(track);
                else if (action == "queue_end") emit add_to_queue_end_requested(track);
            });
        }
    }

    if (!artists.empty()) {
        auto *sec_header = new QLabel("Artistas", this);
        sec_header->setFont(DesignTokens::getFont("micro", 11));
        sec_header->setStyleSheet(QString("color: %1; text-transform: uppercase; padding: 12px 12px 6px;")
            .arg(c.accent.name()));
        results_->addWidget(sec_header);
        for (const auto &artist : artists) {
            auto *ci = new ClickableItem(static_cast<std::string>(artist.name), "Artista", this);
            ci->set_item_id(static_cast<std::string>(artist.id));
            ci->set_item_type("artist");
            results_->addWidget(ci);
            connect(ci, &ClickableItem::clicked, this, [this, artist]() {
                emit artist_requested(static_cast<std::string>(artist.id));
            });
            connect(ci, &ClickableItem::context_action, this, [this, artist](const std::string &action, const std::string &) {
                if (action == "add_favorite") {
                    on_add_favorite_artist(artist);
                } else if (action == "remove_favorite") {
                    on_remove_favorite_artist(static_cast<std::string>(artist.id));
                }
            });
        }
    }

    if (!albums.empty()) {
        auto *sec_header = new QLabel("Álbumes", this);
        sec_header->setFont(DesignTokens::getFont("micro", 11));
        sec_header->setStyleSheet(QString("color: %1; text-transform: uppercase; padding: 12px 12px 6px;")
            .arg(c.accent.name()));
        results_->addWidget(sec_header);
        for (const auto &album : albums) {
            auto *ci = new ClickableItem(static_cast<std::string>(album.title), static_cast<std::string>(album.artist), this);
            ci->set_item_id(static_cast<std::string>(album.id));
            ci->set_item_type("album");
            results_->addWidget(ci);
            connect(ci, &ClickableItem::clicked, this, [this, album]() {
                emit album_requested(static_cast<std::string>(album.id));
            });
            connect(ci, &ClickableItem::context_action, this, [this, album](const std::string &action, const std::string &) {
                if (action == "add_favorite") {
                    on_add_favorite_album(album);
                } else if (action == "remove_favorite") {
                    on_remove_favorite_album(static_cast<std::string>(album.id));
                }
            });
        }
    }

    if (!playlists.empty()) {
        auto *sec_header = new QLabel("Playlists", this);
        sec_header->setFont(DesignTokens::getFont("micro", 11));
        sec_header->setStyleSheet(QString("color: %1; text-transform: uppercase; padding: 12px 12px 6px;")
            .arg(c.accent.name()));
        results_->addWidget(sec_header);
        for (const auto &playlist : playlists) {
            auto *ci = new ClickableItem(static_cast<std::string>(playlist.name),
                                         static_cast<std::string>(playlist.description), this);
            ci->set_item_id(static_cast<std::string>(playlist.id));
            ci->set_item_type("playlist");
            results_->addWidget(ci);
            connect(ci, &ClickableItem::clicked, this, [this, playlist]() {
                emit playlist_requested(static_cast<std::string>(playlist.id));
            });
        }
    }

    if (!shows.empty()) {
        auto *sec_header = new QLabel("Podcasts", this);
        sec_header->setFont(DesignTokens::getFont("micro", 11));
        sec_header->setStyleSheet(QString("color: %1; text-transform: uppercase; padding: 12px 12px 6px;")
            .arg(c.accent.name()));
        results_->addWidget(sec_header);
        for (const auto &show : shows) {
            auto *ci = new ClickableItem(
                static_cast<std::string>(show.title),
                static_cast<std::string>(show.author),
                this);
            ci->set_item_id(static_cast<std::string>(show.id));
            ci->set_item_type("show");
            results_->addWidget(ci);
            connect(ci, &ClickableItem::clicked, this, [this, show]() {
                emit show_requested(static_cast<std::string>(show.id));
            });
        }
    }

    if (!episodes.empty()) {
        auto *sec_header = new QLabel("Episodios", this);
        sec_header->setFont(DesignTokens::getFont("micro", 11));
        sec_header->setStyleSheet(QString("color: %1; text-transform: uppercase; padding: 12px 12px 6px;")
            .arg(c.accent.name()));
        results_->addWidget(sec_header);
        for (const auto &ep : episodes) {
            auto *ci = new ClickableItem(
                static_cast<std::string>(ep.title),
                static_cast<std::string>(ep.show),
                this);
            ci->set_item_id(static_cast<std::string>(ep.id));
            ci->set_item_type("episode");
            results_->addWidget(ci);
            connect(ci, &ClickableItem::clicked, this, [this, ep]() {
                emit show_requested(static_cast<std::string>(ep.show_id));
            });
        }
    }

    results_->addStretch(1);
}

void SearchView::show_recent_searches(const std::vector<std::string> &queries) {
    showing_recent_ = true;
    clear_layout(results_);

    const auto &c = DesignTokens::current();

    if (queries.empty()) {
        auto *empty = new QLabel("Sin búsquedas recientes", this);
        empty->setFont(DesignTokens::getFont("body", 14));
        empty->setStyleSheet(QString("color: %1; padding: 24px;").arg(c.text_muted.name()));
        empty->setAlignment(Qt::AlignCenter);
        results_->addWidget(empty);
        results_->addStretch(1);
        return;
    }

    // Header row: title on the left, "clear all" action on the right.
    auto *header_row = new QWidget(this);
    auto *header_layout = new QHBoxLayout(header_row);
    header_layout->setContentsMargins(12, 12, 12, 6);
    header_layout->setSpacing(8);

    auto *sec_header = new QLabel("Búsquedas recientes", header_row);
    sec_header->setFont(DesignTokens::getFont("micro", 11));
    sec_header->setStyleSheet(QString("color: %1; text-transform: uppercase; background: transparent;")
        .arg(c.accent.name()));
    header_layout->addWidget(sec_header);
    header_layout->addStretch();

    auto *clear_all = new QPushButton("Limpiar todo", header_row);
    clear_all->setCursor(Qt::PointingHandCursor);
    clear_all->setFont(DesignTokens::getFont("micro", 11));
    clear_all->setFocusPolicy(Qt::StrongFocus);
    clear_all->setAccessibleName("Limpiar todo el historial de búsqueda");
    clear_all->setStyleSheet(QString(
        "QPushButton { background: transparent; border: none; color: %1; padding: 2px 6px; }\n"
        "QPushButton:hover { color: %2; }\n"
        "QPushButton:focus { outline: none; text-decoration: underline; }\n"
    ).arg(c.text_muted.name()).arg(c.error.name()));
    connect(clear_all, &QPushButton::clicked, this, [this]() {
        emit search_history_clear_requested();
    });
    header_layout->addWidget(clear_all);
    results_->addWidget(header_row);

    for (const auto &q : queries) {
        auto *row = new QWidget(this);
        row->setFixedHeight(40);
        auto *row_layout = new QHBoxLayout(row);
        row_layout->setContentsMargins(0, 0, 0, 0);
        row_layout->setSpacing(0);

        auto *btn = new QPushButton(row);
        btn->setFixedHeight(40);
        btn->setCursor(Qt::PointingHandCursor);
        btn->setFocusPolicy(Qt::StrongFocus);
        btn->setAccessibleName(QString::fromStdString("Buscar " + q));

        auto *btn_layout = new QHBoxLayout(btn);
        btn_layout->setContentsMargins(12, 0, 12, 0);
        btn_layout->setSpacing(8);

        auto *history_icon = IconProvider::createIconLabel("history", 16, c.text_secondary, true, btn);
        auto *text_label = new QLabel(QString::fromStdString(q), btn);
        text_label->setFont(DesignTokens::getFont("body", 13));
        text_label->setStyleSheet("color: inherit; background: transparent;");

        btn_layout->addWidget(history_icon);
        btn_layout->addWidget(text_label);
        btn_layout->addStretch();
        btn->setLayout(btn_layout);

        QString btnStyle = QString(
            "QPushButton {\n"
            "    background: transparent;\n"
            "    border: none;\n"
            "    border-radius: 6px;\n"
            "    color: %1;\n"
            "}\n"
            "QPushButton:hover {\n"
            "    background-color: %2;\n"
            "    color: %3;\n"
            "}\n"
            "QPushButton:focus {\n"
            "    background-color: %2;\n"
            "    color: %3;\n"
            "}\n"
        )
        .arg(c.text_secondary.name())
        .arg(QString("rgba(%1, %2, %3, %4)").arg(c.accent_dim.red()).arg(c.accent_dim.green()).arg(c.accent_dim.blue()).arg(c.accent_dim.alpha() / 255.0))
        .arg(c.text_primary.name());
        btn->setStyleSheet(btnStyle);

        connect(btn, &QPushButton::clicked, this, [this, q]() {
            emit search_requested(q, "all");
        });
        row_layout->addWidget(btn, 1);

        // Per-entry delete button.
        auto *del = new QPushButton("✕", row);
        del->setFixedSize(28, 28);
        del->setCursor(Qt::PointingHandCursor);
        del->setFocusPolicy(Qt::StrongFocus);
        del->setAccessibleName(QString::fromStdString("Eliminar " + q + " del historial"));
        del->setStyleSheet(QString(
            "QPushButton { background: transparent; border: none; border-radius: 14px; color: %1; font-size: 12px; }\n"
            "QPushButton:hover { background-color: rgba(239, 68, 68, 0.12); color: %2; }\n"
            "QPushButton:focus { background-color: rgba(239, 68, 68, 0.12); color: %2; }\n"
        ).arg(c.text_muted.name()).arg(c.error.name()));
        connect(del, &QPushButton::clicked, this, [this, q]() {
            emit search_history_delete_requested(q);
        });
        row_layout->addWidget(del);

        results_->addWidget(row);
    }
    results_->addStretch(1);
}

void SearchView::set_results(const TopResult &top_result, bool has_top_result,
                             const std::vector<Track> &songs,
                             const std::vector<Track> &videos,
                             const std::vector<Artist> &artists,
                             const std::vector<Album> &albums,
                             const std::vector<Playlist> &playlists,
                             const std::vector<Show> &shows,
                             const std::vector<Episode> &episodes) {
    show_results(top_result, has_top_result, songs, videos, artists, albums, playlists, shows, episodes);
}

void SearchView::set_recent_searches(const std::vector<std::string> &queries) {
    show_recent_searches(queries);
}

void SearchView::set_active_filter(const std::string &filter) {
    for (auto *btn : filter_btns_) {
        btn->setChecked(btn->property("filterValue").toString().toStdString() == filter);
    }
}
