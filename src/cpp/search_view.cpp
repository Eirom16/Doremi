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
        {"Álbumes", "albums"}, {"Artistas", "artists"}, {"Playlists", "playlists"}
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

    auto *scroll = new QScrollArea(this);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setStyleSheet("background: transparent; border: none;");

    auto *inner = new QWidget();
    inner->setStyleSheet("background: transparent;");
    results_ = new QVBoxLayout(inner);
    results_->setSpacing(6);
    results_->setContentsMargins(0, 4, 0, 4);

    auto *placeholder = new QLabel("Escribe algo para buscar", inner);
    placeholder->setFont(DesignTokens::getFont("body", 14));
    placeholder->setStyleSheet(QString("color: %1; padding: 24px;").arg(c.text_muted.name()));
    placeholder->setAlignment(Qt::AlignCenter);
    results_->addWidget(placeholder);
    results_->addStretch(1);

    scroll->setWidget(inner);
    root->addWidget(scroll, 1);
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

void SearchView::show_results(const std::vector<Track> &songs,
                              const std::vector<Track> &videos,
                              const std::vector<Artist> &artists,
                              const std::vector<Album> &albums,
                              const std::vector<Playlist> &playlists) {
    showing_recent_ = false;
    clear_layout(results_);

    const auto &c = DesignTokens::current();

    bool has_any = !songs.empty() || !videos.empty() || !artists.empty() || !albums.empty() || !playlists.empty();
    if (!has_any) {
        auto *empty = new QLabel("Sin resultados", this);
        empty->setFont(DesignTokens::getFont("body", 14));
        empty->setStyleSheet(QString("color: %1; padding: 24px;").arg(c.text_muted.name()));
        empty->setAlignment(Qt::AlignCenter);
        results_->addWidget(empty);
        results_->addStretch(1);
        return;
    }

    // Top result: surface the single most relevant hit prominently. Artists and
    // albums are stronger intent signals than a song match, so prefer them.
    {
        auto *sec_header = new QLabel("Mejor resultado", this);
        sec_header->setFont(DesignTokens::getFont("micro", 11));
        sec_header->setStyleSheet(QString("color: %1; text-transform: uppercase; padding: 6px 12px 6px;")
            .arg(c.accent.name()));

        ClickableItem *top = nullptr;
        if (!artists.empty()) {
            const auto &a = artists.front();
            top = new ClickableItem(static_cast<std::string>(a.name), "Artista", this);
            top->set_item_id(static_cast<std::string>(a.id));
            top->set_item_type("artist");
            connect(top, &ClickableItem::clicked, this, [this, a]() {
                emit artist_requested(static_cast<std::string>(a.id));
            });
        } else if (!albums.empty()) {
            const auto &al = albums.front();
            top = new ClickableItem(static_cast<std::string>(al.title), static_cast<std::string>(al.artist), this);
            top->set_item_id(static_cast<std::string>(al.id));
            top->set_item_type("album");
            connect(top, &ClickableItem::clicked, this, [this, al]() {
                emit album_requested(static_cast<std::string>(al.id));
            });
        } else if (!songs.empty()) {
            const auto &t = songs.front();
            top = new ClickableItem(static_cast<std::string>(t.title), static_cast<std::string>(t.artist), this);
            top->set_item_id(static_cast<std::string>(t.id));
            top->set_item_type("song");
            connect(top, &ClickableItem::clicked, this, [this, t]() { emit play_requested(t); });
            connect(top, &ClickableItem::context_action, this, [this, t](const std::string &action, const std::string &) {
                if (action == "add_favorite") emit add_favorite_requested(t);
                else if (action == "download") emit download_requested(t);
                else if (action == "queue_next") emit add_to_queue_next_requested(t);
                else if (action == "queue_end") emit add_to_queue_end_requested(t);
            });
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

void SearchView::set_results(const std::vector<Track> &songs,
                             const std::vector<Track> &videos,
                             const std::vector<Artist> &artists,
                             const std::vector<Album> &albums,
                             const std::vector<Playlist> &playlists) {
    show_results(songs, videos, artists, albums, playlists);
}

void SearchView::set_recent_searches(const std::vector<std::string> &queries) {
    show_recent_searches(queries);
}

void SearchView::set_active_filter(const std::string &filter) {
    for (auto *btn : filter_btns_) {
        btn->setChecked(btn->property("filterValue").toString().toStdString() == filter);
    }
}
