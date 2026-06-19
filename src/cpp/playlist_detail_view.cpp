#include "playlist_detail_view.h"
#include "design_tokens.h"
#include "icon_provider.h"
#include <QPainter>
#include <QPainterPath>
#include <QPushButton>
#include <QMenu>
#include "doremi/src/bridge.rs.h"

// ─────────────────────────────────────────────────────────────────────────────
// PlaylistTrackRow
// ─────────────────────────────────────────────────────────────────────────────

PlaylistTrackRow::PlaylistTrackRow(int num, const QString &title, const QString &artist,
                                   const QString &duration, Track track,
                                   QWidget *parent)
    : QWidget(parent), track_(std::move(track))
{
    const auto &c = DesignTokens::current();
    bool unavailable = track_.id.empty();
    setFixedHeight(48);
    setCursor(unavailable ? Qt::ArrowCursor : Qt::PointingHandCursor);

    auto *layout = new QHBoxLayout(this);
    layout->setContentsMargins(16, 4, 16, 4);
    layout->setSpacing(14);

    // Track number
    auto *num_lbl = new QLabel(QString::number(num), this);
    num_lbl->setFont(DesignTokens::getFont("caption", 12));
    num_lbl->setFixedWidth(24);
    num_lbl->setAlignment(Qt::AlignCenter);
    num_lbl->setStyleSheet(QString("color: %1;").arg(c.text_muted.name()));
    layout->addWidget(num_lbl);

    // Title + Artist
    auto *text = new QWidget(this);
    auto *text_l = new QVBoxLayout(text);
    text_l->setContentsMargins(0, 0, 0, 0);
    text_l->setSpacing(1);

    QString title_text = title;
    if (unavailable) title_text = "No disponible";
    auto *t_lbl = new QLabel(title_text, this);
    t_lbl->setFont(DesignTokens::getFont("body", 13));
    QString title_color = unavailable ? c.text_muted.name() : c.text_primary.name();
    t_lbl->setStyleSheet(QString("color: %1; font-weight: 600;").arg(title_color));

    auto *a_lbl = new QLabel(unavailable ? "" : artist, this);
    a_lbl->setFont(DesignTokens::getFont("caption", 11));
    a_lbl->setStyleSheet(QString("color: %1;").arg(c.text_muted.name()));

    text_l->addWidget(t_lbl);
    text_l->addWidget(a_lbl);
    layout->addWidget(text, 2);

    // Duration
    if (!duration.isEmpty()) {
        auto *dur = new QLabel(duration, this);
        dur->setFont(DesignTokens::getFont("caption", 11));
        dur->setStyleSheet(QString("color: %1;").arg(c.text_muted.name()));
        dur->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        layout->addWidget(dur);
    }

    // Favorite button (only for available tracks)
    if (!unavailable) {
        auto *fav_btn = new QPushButton(this);
        fav_btn->setFixedSize(28, 28);
        fav_btn->setCursor(Qt::PointingHandCursor);
        bool is_fav = get_track_favorite_state(static_cast<std::string>(track_.id));
        fav_btn->setIcon(IconProvider::getIcon(
            is_fav ? "favorite" : "favorite_border",
            is_fav ? c.accent : c.text_muted, 16));
        fav_btn->setStyleSheet("QPushButton { background: transparent; border: none; }");
        connect(fav_btn, &QPushButton::clicked, this, [this, fav_btn, is_fav]() {
            if (is_fav) {
                on_remove_favorite(static_cast<std::string>(track_.id));
            } else {
                on_add_favorite(track_);
            }
        });
        layout->addWidget(fav_btn);
    }

    setObjectName(unavailable ? "PlaylistTrackRow_unavailable" : "PlaylistTrackRow");
    if (unavailable) {
        setStyleSheet(QString("QWidget#PlaylistTrackRow_unavailable { background-color: transparent; border-radius: 6px; }"
                              "QWidget#PlaylistTrackRow_unavailable:hover { background-color: rgba(%1, %2, %3, 0.03); }")
            .arg(c.text_primary.red()).arg(c.text_primary.green()).arg(c.text_primary.blue()));
    } else {
        setStyleSheet("QWidget#PlaylistTrackRow { background-color: transparent; border-radius: 6px; }");
    }
}

void PlaylistTrackRow::mousePressEvent(QMouseEvent *event) {
    QWidget::mousePressEvent(event);
    if (event->button() == Qt::LeftButton && !track_.id.empty())
        emit play_requested(track_);
}

void PlaylistTrackRow::contextMenuEvent(QContextMenuEvent *event) {
    QMenu menu;
    QAction *play = menu.addAction("Reproducir");
    
    bool is_fav = get_track_favorite_state(static_cast<std::string>(track_.id));
    QAction *fav = menu.addAction(is_fav ? "Quitar de favoritos" : "Agregar a favoritos");
    
    QAction *dl = menu.addAction("Descargar");
    menu.addSeparator();
    QAction *next = menu.addAction("Reproducir siguiente");
    QAction *end = menu.addAction("Agregar a la cola");
    menu.addSeparator();
    QAction *remove = menu.addAction("Quitar de la playlist");
    remove->setIcon(IconProvider::getIcon("remove_circle", DesignTokens::current().error, 16));

    QAction *chosen = menu.exec(event->globalPos());
    if (chosen == play) {
        emit play_requested(track_);
    } else if (chosen == fav) {
        if (is_fav) {
            on_remove_favorite(static_cast<std::string>(track_.id));
        } else {
            on_add_favorite(track_);
        }
    } else if (chosen == dl) {
        on_download_requested(track_);
    } else if (chosen == next) {
        on_add_to_queue_next(track_);
    } else if (chosen == end) {
        on_add_to_queue_end(track_);
    } else if (chosen == remove) {
        emit remove_requested(playlist_id_, static_cast<std::string>(track_.id));
    }
}

void PlaylistTrackRow::enterEvent(QEnterEvent *event) {
    QWidget::enterEvent(event);
    const auto &c = DesignTokens::current();
    setStyleSheet(QString("QWidget#PlaylistTrackRow { background-color: rgba(%1, %2, %3, 0.06); border-radius: 6px; }")
        .arg(c.text_primary.red()).arg(c.text_primary.green()).arg(c.text_primary.blue()));
}

void PlaylistTrackRow::leaveEvent(QEvent *event) {
    QWidget::leaveEvent(event);
    setStyleSheet("QWidget#PlaylistTrackRow { background-color: transparent; border-radius: 6px; }");
}

// ─────────────────────────────────────────────────────────────────────────────
// PlaylistDetailView
// ─────────────────────────────────────────────────────────────────────────────

PlaylistDetailView::PlaylistDetailView(QWidget *parent)
    : QWidget(parent)
{
    setupLayout();
}

void PlaylistDetailView::setupLayout() {
    const auto &c = DesignTokens::current();

    auto *main_vbox = new QVBoxLayout(this);
    main_vbox->setContentsMargins(0, 0, 0, 0);
    main_vbox->setSpacing(0);

    content_layout_ = new QVBoxLayout();
    content_layout_->setContentsMargins(24, 16, 24, 24);
    content_layout_->setSpacing(8);
    content_layout_->setAlignment(Qt::AlignTop);

    // Back button
    auto *back_btn = new QPushButton(this);
    auto *back_layout = new QHBoxLayout(back_btn);
    back_layout->setContentsMargins(8, 4, 12, 4);
    back_layout->setSpacing(6);
    auto *back_icon = IconProvider::createIconLabel("arrow_back", 18, c.text_secondary, false, back_btn);
    auto *back_text = new QLabel("Volver", back_btn);
    back_text->setFont(DesignTokens::getFont("caption", 12));
    back_text->setStyleSheet(QString("color: %1; background: transparent;").arg(c.text_secondary.name()));
    back_layout->addWidget(back_icon);
    back_layout->addWidget(back_text);
    back_btn->setLayout(back_layout);
    back_btn->setFixedHeight(32);
    back_btn->setCursor(Qt::PointingHandCursor);
    back_btn->setStyleSheet(QString(
        "QPushButton { background: transparent; border: none; border-radius: 6px; }"
        "QPushButton:hover { background: rgba(%1, %2, %3, 0.08); }")
        .arg(c.text_primary.red()).arg(c.text_primary.green()).arg(c.text_primary.blue()));
    connect(back_btn, &QPushButton::clicked, this, &PlaylistDetailView::back_requested);
    content_layout_->addWidget(back_btn, 0, Qt::AlignLeft);

    // Header: cover + info
    auto *header = new QHBoxLayout();
    header->setSpacing(24);

    cover_label_ = new QLabel(this);
    cover_label_->setFixedSize(160, 160);
    cover_label_->setStyleSheet(QString("background-color: %1; border-radius: 12px;").arg(c.bg_elevated.name()));
    cover_label_->setAlignment(Qt::AlignCenter);
    header->addWidget(cover_label_);

    auto *info = new QVBoxLayout();
    info->setSpacing(6);

    auto *type_lbl = new QLabel("PLAYLIST", this);
    type_lbl->setFont(DesignTokens::getFont("caption", 10));
    type_lbl->setStyleSheet(QString("color: %1; font-weight: bold; letter-spacing: 2px;").arg(c.text_muted.name()));
    info->addWidget(type_lbl);

    title_label_ = new QLabel("Playlist", this);
    title_label_->setFont(DesignTokens::getFont("display", 24));
    title_label_->setStyleSheet(QString("color: %1; font-weight: bold;").arg(c.text_primary.name()));
    title_label_->setWordWrap(true);
    info->addWidget(title_label_);

    desc_label_ = new QLabel("", this);
    desc_label_->setFont(DesignTokens::getFont("caption", 11));
    desc_label_->setStyleSheet(QString("color: %1;").arg(c.text_secondary.name()));
    desc_label_->setWordWrap(true);
    desc_label_->setMaximumWidth(400);
    desc_label_->hide();
    info->addWidget(desc_label_);

    meta_label_ = new QLabel("", this);
    meta_label_->setFont(DesignTokens::getFont("caption", 12));
    meta_label_->setStyleSheet(QString("color: %1;").arg(c.text_muted.name()));
    info->addWidget(meta_label_);

    // Action buttons
    auto *actions = new QHBoxLayout();
    actions->setSpacing(10);

    // Play all
    auto *play_btn = new QPushButton(this);
    auto *play_l = new QHBoxLayout(play_btn);
    play_l->setContentsMargins(16, 8, 20, 8);
    play_l->setSpacing(8);
    play_l->addWidget(IconProvider::createIconLabel("play_arrow", 20, QColor("#FFFFFF"), false, play_btn));
    auto *play_t = new QLabel("Reproducir", play_btn);
    play_t->setFont(DesignTokens::getFont("body", 13));
    play_t->setStyleSheet("color: #FFFFFF; background: transparent; font-weight: bold;");
    play_l->addWidget(play_t);
    play_btn->setLayout(play_l);
    play_btn->setFixedHeight(40);
    play_btn->setCursor(Qt::PointingHandCursor);
    play_btn->setStyleSheet(QString(
        "QPushButton { background: %1; border: none; border-radius: 20px; }"
        "QPushButton:hover { background: %2; }")
        .arg(c.accent.name()).arg(c.accent.lighter(115).name()));
    connect(play_btn, &QPushButton::clicked, this, [this]() {
        emit play_all_requested(tracks_);
    });
    actions->addWidget(play_btn);

    // Shuffle
    auto *shuffle_btn = new QPushButton(this);
    auto *shuffle_l = new QHBoxLayout(shuffle_btn);
    shuffle_l->setContentsMargins(16, 8, 20, 8);
    shuffle_l->setSpacing(8);
    shuffle_l->addWidget(IconProvider::createIconLabel("shuffle", 18, c.text_primary, false, shuffle_btn));
    auto *shuffle_t = new QLabel("Aleatorio", shuffle_btn);
    shuffle_t->setFont(DesignTokens::getFont("body", 13));
    shuffle_t->setStyleSheet(QString("color: %1; background: transparent; font-weight: 600;").arg(c.text_primary.name()));
    shuffle_l->addWidget(shuffle_t);
    shuffle_btn->setLayout(shuffle_l);
    shuffle_btn->setFixedHeight(40);
    shuffle_btn->setCursor(Qt::PointingHandCursor);
    shuffle_btn->setStyleSheet(QString(
        "QPushButton { background: transparent; border: 1px solid %1; border-radius: 20px; }"
        "QPushButton:hover { background: rgba(%2, %3, %4, 0.08); }")
        .arg(c.border.name())
        .arg(c.text_primary.red()).arg(c.text_primary.green()).arg(c.text_primary.blue()));
    connect(shuffle_btn, &QPushButton::clicked, this, [this]() {
        emit shuffle_requested(tracks_);
    });
    actions->addWidget(shuffle_btn);

    // Download all
    auto *dl_btn = new QPushButton(this);
    auto *dl_l = new QHBoxLayout(dl_btn);
    dl_l->setContentsMargins(16, 8, 20, 8);
    dl_l->setSpacing(8);
    dl_l->addWidget(IconProvider::createIconLabel("download", 18, c.accent, false, dl_btn));
    auto *dl_t = new QLabel("Descargar todo", dl_btn);
    dl_t->setFont(DesignTokens::getFont("body", 13));
    dl_t->setStyleSheet(QString("color: %1; background: transparent; font-weight: 600;").arg(c.accent.name()));
    dl_l->addWidget(dl_t);
    dl_btn->setLayout(dl_l);
    dl_btn->setFixedHeight(40);
    dl_btn->setCursor(Qt::PointingHandCursor);
    dl_btn->setStyleSheet(QString(
        "QPushButton { background: transparent; border: 1px solid %1; border-radius: 20px; }"
        "QPushButton:hover { background: rgba(%2, %3, %4, 0.08); }")
        .arg(c.accent.name())
        .arg(c.accent.red()).arg(c.accent.green()).arg(c.accent.blue()));
    connect(dl_btn, &QPushButton::clicked, this, [this]() {
        if (!tracks_.empty()) {
            const auto &p = current_playlist_;
            std::string pid = static_cast<std::string>(p.id);
            std::string pt = static_cast<std::string>(p.name);
            std::string pth = static_cast<std::string>(p.thumbnail);
            emit download_all_requested(tracks_, pid, pt, pth);
        }
    });
    actions->addWidget(dl_btn);

    // Edit
    edit_btn_ = new QPushButton(this);
    edit_btn_->setFixedSize(36, 36);
    edit_btn_->setCursor(Qt::PointingHandCursor);
    edit_btn_->setToolTip("Editar playlist");
    edit_btn_->setIcon(IconProvider::getIcon("edit", c.text_secondary, 18));
    edit_btn_->setStyleSheet("QPushButton { background: transparent; border: none; border-radius: 18px; }"
                             "QPushButton:hover { background: rgba(255,255,255,0.08); }");
    connect(edit_btn_, &QPushButton::clicked, this, [this]() {
        bool ok;
        QString new_name = QInputDialog::getText(this, "Renombrar playlist",
            "Nuevo nombre:", QLineEdit::Normal,
            QString::fromStdString(static_cast<std::string>(title_label_->text().toUtf8())), &ok);
        if (ok && !new_name.trimmed().isEmpty()) {
            emit rename_playlist_requested(playlist_id_, new_name.trimmed().toStdString());
        }

        QString new_desc = QInputDialog::getText(this, "Editar descripción",
            "Nueva descripción:", QLineEdit::Normal,
            desc_label_->isVisible() ? desc_label_->text() : "", &ok);
        if (ok) {
            // TODO: emit signal for description update when backend supports it
            desc_label_->setText(new_desc);
            desc_label_->setVisible(!new_desc.isEmpty());
        }
    });
    actions->addWidget(edit_btn_);

    // Delete
    delete_btn_ = new QPushButton(this);
    delete_btn_->setFixedSize(36, 36);
    delete_btn_->setCursor(Qt::PointingHandCursor);
    delete_btn_->setToolTip("Eliminar playlist");
    delete_btn_->setIcon(IconProvider::getIcon("delete", c.error, 18));
    delete_btn_->setStyleSheet("QPushButton { background: transparent; border: none; border-radius: 18px; }"
                               "QPushButton:hover { background: rgba(255,255,255,0.08); }");
    connect(delete_btn_, &QPushButton::clicked, this, [this]() {
        auto reply = QMessageBox::question(this, "Eliminar playlist",
            "¿Estás seguro de que deseas eliminar esta playlist?",
            QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
        if (reply == QMessageBox::Yes) {
            emit delete_playlist_requested(playlist_id_);
        }
    });
    actions->addWidget(delete_btn_);

    actions->addStretch();

    info->addSpacing(8);
    info->addLayout(actions);
    info->addStretch();

    header->addLayout(info, 1);
    content_layout_->addLayout(header);

    // Separator
    auto *sep = new QWidget(this);
    sep->setFixedHeight(1);
    sep->setStyleSheet(QString("background-color: %1;").arg(c.border.name()));
    content_layout_->addSpacing(8);
    content_layout_->addWidget(sep);
    content_layout_->addSpacing(4);

    // Tracks container
    tracks_widget_ = new QWidget(this);
    tracks_widget_->setStyleSheet("background: transparent;");
    tracks_layout_ = new QVBoxLayout(tracks_widget_);
    tracks_layout_->setContentsMargins(0, 0, 0, 0);
    tracks_layout_->setSpacing(2);
    content_layout_->addWidget(tracks_widget_);

    main_vbox->addLayout(content_layout_);
    setLayout(main_vbox);
}

void PlaylistDetailView::set_playlist_info(const Playlist &playlist) {
    current_playlist_ = playlist;
    const auto &c = DesignTokens::current();
    playlist_id_ = static_cast<std::string>(playlist.id);
    title_label_->setText(QString::fromStdString(static_cast<std::string>(playlist.name)));

    if (!playlist.description.empty()) {
        desc_label_->setText(QString::fromStdString(static_cast<std::string>(playlist.description)));
        desc_label_->show();
    } else {
        desc_label_->hide();
    }

    QString meta_text;
    if (!playlist.owner.empty()) {
        meta_text += QString("Creada por %1").arg(QString::fromStdString(static_cast<std::string>(playlist.owner)));
    }
    if (!playlist.privacy.empty()) {
        if (!meta_text.isEmpty()) meta_text += " • ";
        QString priv_es = QString::fromStdString(static_cast<std::string>(playlist.privacy));
        if (priv_es == "PUBLIC") priv_es = "Pública";
        else if (priv_es == "PRIVATE") priv_es = "Privada";
        else if (priv_es == "UNLISTED") priv_es = "No listada";
        meta_text += priv_es;
    }
    if (playlist.track_count > 0) {
        if (!meta_text.isEmpty()) meta_text += " • ";
        meta_text += QString("%1 canciones").arg(playlist.track_count);
    }
    meta_label_->setText(meta_text);

    QPixmap pm;
    if (!playlist.thumbnail.empty() && pm.load(QString::fromStdString(static_cast<std::string>(playlist.thumbnail)))) {
        QPixmap dest(pm.size());
        dest.fill(Qt::transparent);
        QPainter painter(&dest);
        painter.setRenderHint(QPainter::Antialiasing);
        QPainterPath path;
        path.addRoundedRect(pm.rect(), 12, 12);
        painter.setClipPath(path);
        painter.drawPixmap(0, 0, pm);
        cover_label_->setPixmap(dest.scaled(160, 160, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation));
    } else {
        auto icon = IconProvider::getIcon("queue_music", c.text_secondary, 60);
        cover_label_->setPixmap(icon.pixmap(60, 60));
    }
}

void PlaylistDetailView::set_playlist_tracks(const std::vector<Track> &tracks) {
    tracks_ = tracks;
    QLayoutItem *item;
    while ((item = tracks_layout_->takeAt(0)) != nullptr) {
        if (item->widget()) item->widget()->deleteLater();
        delete item;
    }

    for (size_t i = 0; i < tracks.size(); ++i) {
        const auto &t = tracks[i];
        QString dur;
        if (t.duration_ms > 0) {
            int secs = static_cast<int>(t.duration_ms / 1000);
            dur = QString("%1:%2").arg(secs / 60).arg(secs % 60, 2, 10, QChar('0'));
        }

        auto *row = new PlaylistTrackRow(
            static_cast<int>(i) + 1,
            QString::fromStdString(static_cast<std::string>(t.title)),
            QString::fromStdString(static_cast<std::string>(t.artist)),
            dur,
            t,
            tracks_widget_
        );
        row->setPlaylistId(playlist_id_);
        connect(row, &PlaylistTrackRow::play_requested, this, &PlaylistDetailView::play_requested);
        connect(row, &PlaylistTrackRow::remove_requested, this, &PlaylistDetailView::remove_track_from_playlist_requested);
        tracks_layout_->addWidget(row);
    }

    if (tracks.empty()) {
        const auto &c = DesignTokens::current();
        auto *empty = new QLabel("Esta playlist está vacía.", tracks_widget_);
        empty->setAlignment(Qt::AlignCenter);
        empty->setFont(DesignTokens::getFont("caption", 12));
        empty->setStyleSheet(QString("color: %1; padding: 30px;").arg(c.text_muted.name()));
        tracks_layout_->addWidget(empty);
    }
}

void PlaylistDetailView::clear() {
    title_label_->setText("Playlist");
    desc_label_->hide();
    meta_label_->setText("");
    cover_label_->clear();
    QLayoutItem *item;
    while ((item = tracks_layout_->takeAt(0)) != nullptr) {
        if (item->widget()) item->widget()->deleteLater();
        delete item;
    }
}
