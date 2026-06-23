#include "playlist_detail_view.h"
#include "design_tokens.h"
#include "icon_provider.h"
#include "components/artwork_loader.h"
#include <QApplication>
#include <QDrag>
#include <QDragEnterEvent>
#include <QDragMoveEvent>
#include <QDropEvent>
#include <QMimeData>
#include <QPainter>
#include <QPainterPath>
#include <QPushButton>
#include <QMenu>
#include <QPointer>
#include "doremi/src/bridge.rs.h"

// ─────────────────────────────────────────────────────────────────────────────
// PlaylistTrackRow
// ─────────────────────────────────────────────────────────────────────────────

PlaylistTrackRow::PlaylistTrackRow(int num, const QString &title, const QString &artist,
                                   const QString &duration, Track track,
                                   QWidget *parent)
    : QWidget(parent), index_(num - 1), track_(std::move(track))
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
    num_lbl->setObjectName("numLabel");
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
    if (unavailable) title_text = tr_q("not_available");
    auto *t_lbl = new QLabel(title_text, this);
    t_lbl->setObjectName("titleLabel");
    t_lbl->setFont(DesignTokens::getFont("body", 13));
    QString title_color = unavailable ? c.text_muted.name() : c.text_primary.name();
    t_lbl->setStyleSheet(QString("color: %1; font-weight: 600;").arg(title_color));

    auto *a_lbl = new QLabel(unavailable ? "" : artist, this);
    a_lbl->setObjectName("artistLabel");
    a_lbl->setFont(DesignTokens::getFont("caption", 11));
    a_lbl->setStyleSheet(QString("color: %1;").arg(c.text_muted.name()));

    text_l->addWidget(t_lbl);
    text_l->addWidget(a_lbl);
    layout->addWidget(text, 2);

    // Duration
    if (!duration.isEmpty()) {
        auto *dur = new QLabel(duration, this);
    dur->setObjectName("durationLabel");
        dur->setFont(DesignTokens::getFont("caption", 11));
        dur->setStyleSheet(QString("color: %1;").arg(c.text_muted.name()));
        dur->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        layout->addWidget(dur);
    }

    // Favorite button (only for available tracks)
    if (!unavailable) {
        auto *fav_btn = new QPushButton(this);
    fav_btn->setObjectName("favBtn");
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
    if (event->button() == Qt::LeftButton && !track_.id.empty()) {
        drag_start_position_ = event->position().toPoint();
        dragging_ = false;
    }
    QWidget::mousePressEvent(event);
}

void PlaylistTrackRow::mouseMoveEvent(QMouseEvent *event) {
    if (track_.id.empty() ||
        !(event->buttons() & Qt::LeftButton) ||
        (event->position().toPoint() - drag_start_position_).manhattanLength() < QApplication::startDragDistance()) {
        QWidget::mouseMoveEvent(event);
        return;
    }

    dragging_ = true;
    auto *mime = new QMimeData();
    mime->setData("application/x-doremi-playlist-track-index", QByteArray::number(index_));
    auto *drag = new QDrag(this);
    drag->setMimeData(mime);
    drag->setPixmap(grab());
    drag->setHotSpot(event->position().toPoint());
    drag->exec(Qt::MoveAction);
}

void PlaylistTrackRow::mouseReleaseEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton && !dragging_ && !track_.id.empty()) {
        emit play_requested(track_);
    }
    dragging_ = false;
    QWidget::mouseReleaseEvent(event);
}

void PlaylistTrackRow::contextMenuEvent(QContextMenuEvent *event) {
    QMenu menu;
    QAction *play = menu.addAction(tr_q("play"));
    
    bool is_fav = get_track_favorite_state(static_cast<std::string>(track_.id));
    QAction *fav = menu.addAction(is_fav 
        ? tr_q("remove_favorite")
        : tr_q("add_favorite"));
    
    QAction *dl = menu.addAction(tr_q("download"));
    menu.addSeparator();
    QAction *next = menu.addAction(tr_q("play_next"));
    QAction *end = menu.addAction(tr_q("add_to_queue"));
    menu.addSeparator();
    QAction *remove = menu.addAction(tr_q("remove_from_playlist"));
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
        auto reply = QMessageBox::question(
            this,
            tr_q("confirm_remove_from_playlist_title"),
            tr_q("confirm_remove_from_playlist_desc"),
            QMessageBox::Yes | QMessageBox::No,
            QMessageBox::No);
        if (reply == QMessageBox::Yes) {
            emit remove_requested(playlist_id_, static_cast<std::string>(track_.id));
        }
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
    back_btn->setObjectName("backBtn");
    auto *back_layout = new QHBoxLayout(back_btn);
    back_layout->setContentsMargins(8, 4, 12, 4);
    back_layout->setSpacing(6);
    auto *back_icon = IconProvider::createIconLabel("arrow_back", 18, c.text_secondary, false, back_btn);
    auto *back_text = new QLabel(tr_q("go_back"), back_btn);
    back_text->setObjectName("backText");
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

    auto *type_lbl = new QLabel(tr_q("playlist_singular").toUpper(), this);
    type_lbl->setObjectName("typeLabel");
    type_lbl->setFont(DesignTokens::getFont("caption", 10));
    type_lbl->setStyleSheet(QString("color: %1; font-weight: bold; letter-spacing: 2px;").arg(c.text_muted.name()));
    info->addWidget(type_lbl);

    title_label_ = new QLabel(tr_q("playlist_singular"), this);
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
    play_btn->setObjectName("playBtn");
    auto *play_l = new QHBoxLayout(play_btn);
    play_l->setContentsMargins(16, 8, 20, 8);
    play_l->setSpacing(8);
    play_l->addWidget(IconProvider::createIconLabel("play_arrow", 20, QColor("#FFFFFF"), false, play_btn));
    auto *play_t = new QLabel(tr_q("play"), play_btn);
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
    shuffle_btn->setObjectName("shuffleBtn");
    auto *shuffle_l = new QHBoxLayout(shuffle_btn);
    shuffle_l->setContentsMargins(16, 8, 20, 8);
    shuffle_l->setSpacing(8);
    shuffle_l->addWidget(IconProvider::createIconLabel("shuffle", 18, c.text_primary, false, shuffle_btn));
    auto *shuffle_t = new QLabel(tr_q("shuffle"), shuffle_btn);
    shuffle_t->setObjectName("shuffleText");
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
    dl_btn->setObjectName("dlBtn");
    auto *dl_l = new QHBoxLayout(dl_btn);
    dl_l->setContentsMargins(16, 8, 20, 8);
    dl_l->setSpacing(8);
    auto *dl_icon = IconProvider::createIconLabel("download", 18, c.accent, false, dl_btn);
    dl_icon->setObjectName("dlIcon");
    dl_l->addWidget(dl_icon);
    auto *dl_t = new QLabel(tr_q("download_all"), dl_btn);
    dl_t->setObjectName("dlText");
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
    edit_btn_->setToolTip(tr_q("edit_playlist"));
    edit_btn_->setIcon(IconProvider::getIcon("edit", c.text_secondary, 18));
    edit_btn_->setStyleSheet("QPushButton { background: transparent; border: none; border-radius: 18px; }"
                             "QPushButton:hover { background: rgba(255,255,255,0.08); }");
    connect(edit_btn_, &QPushButton::clicked, this, [this]() {
        bool ok;
        QString new_name = QInputDialog::getText(this, tr_q("rename_playlist"),
            tr_q("new_name"), QLineEdit::Normal,
            QString::fromStdString(static_cast<std::string>(title_label_->text().toUtf8())), &ok);
        if (ok && !new_name.trimmed().isEmpty()) {
            emit rename_playlist_requested(playlist_id_, new_name.trimmed().toStdString());
        }

        QString new_desc = QInputDialog::getText(this, tr_q("edit_description"),
            tr_q("new_description"), QLineEdit::Normal,
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
    delete_btn_->setToolTip(tr_q("delete_playlist"));
    delete_btn_->setIcon(IconProvider::getIcon("delete", c.error, 18));
    delete_btn_->setStyleSheet("QPushButton { background: transparent; border: none; border-radius: 18px; }"
                               "QPushButton:hover { background: rgba(255,255,255,0.08); }");
    connect(delete_btn_, &QPushButton::clicked, this, [this]() {
        auto reply = QMessageBox::question(this, tr_q("delete_playlist"),
            tr_q("confirm_delete_playlist"),
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
    tracks_widget_->setAcceptDrops(true);
    tracks_widget_->installEventFilter(this);
    tracks_layout_ = new QVBoxLayout(tracks_widget_);
    tracks_layout_->setContentsMargins(0, 0, 0, 0);
    tracks_layout_->setSpacing(2);

    drop_indicator_ = new QFrame(tracks_widget_);
    drop_indicator_->setFixedHeight(2);
    drop_indicator_->setStyleSheet(QString("background-color: %1; border-radius: 1px;")
        .arg(c.accent.name()));
    drop_indicator_->hide();

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

    if (!playlist.thumbnail.empty()) {
        QPointer<QLabel> label_ptr(cover_label_);
        ArtworkLoader::load(QString::fromStdString(static_cast<std::string>(playlist.thumbnail)), QSize(160, 160), [label_ptr](const QPixmap &pixmap) {
            if (!label_ptr) return;
            QPixmap dest(pixmap.size());
            dest.fill(Qt::transparent);
            QPainter painter(&dest);
            painter.setRenderHint(QPainter::Antialiasing);
            QPainterPath path;
            path.addRoundedRect(pixmap.rect(), 12, 12);
            painter.setClipPath(path);
            painter.drawPixmap(0, 0, pixmap);
            label_ptr->setPixmap(dest.scaled(160, 160, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation));
        });
    } else {
        auto icon = IconProvider::getIcon("queue_music", c.text_secondary, 60);
        cover_label_->setPixmap(icon.pixmap(60, 60));
    }
}

void PlaylistDetailView::set_playlist_tracks(const std::vector<Track> &tracks) {
    tracks_ = tracks;
    rebuild_tracks();
}

void PlaylistDetailView::rebuild_tracks() {
    hideDropIndicator();
    QLayoutItem *item;
    while ((item = tracks_layout_->takeAt(0)) != nullptr) {
        if (item->widget()) item->widget()->deleteLater();
        delete item;
    }

    for (size_t i = 0; i < tracks_.size(); ++i) {
        const auto &t = tracks_[i];
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
        connect(row, &PlaylistTrackRow::move_requested, this, [this](int from, int to) {
            if (from < 0 || from >= static_cast<int>(tracks_.size()) ||
                to < 0 || to >= static_cast<int>(tracks_.size()) ||
                from == to) {
                return;
            }
            auto moved = tracks_[static_cast<size_t>(from)];
            tracks_.erase(tracks_.begin() + from);
            tracks_.insert(tracks_.begin() + to, moved);
            rebuild_tracks();
            emit track_moved(playlist_id_, from, to);
        });
        tracks_layout_->addWidget(row);
    }

    if (tracks_.empty()) {
        const auto &c = DesignTokens::current();
        auto *empty = new QLabel("Esta playlist está vacía.", tracks_widget_);
        empty->setAlignment(Qt::AlignCenter);
        empty->setFont(DesignTokens::getFont("caption", 12));
        empty->setStyleSheet(QString("color: %1; padding: 30px;").arg(c.text_muted.name()));
        tracks_layout_->addWidget(empty);
    }
}

bool PlaylistDetailView::eventFilter(QObject *watched, QEvent *event) {
    if (watched != tracks_widget_) {
        return QWidget::eventFilter(watched, event);
    }

    constexpr auto mime_type = "application/x-doremi-playlist-track-index";
    if (event->type() == QEvent::DragEnter) {
        auto *drag_event = static_cast<QDragEnterEvent *>(event);
        if (drag_event->mimeData()->hasFormat(mime_type)) {
            drag_event->acceptProposedAction();
            return true;
        }
    } else if (event->type() == QEvent::DragMove) {
        auto *drag_event = static_cast<QDragMoveEvent *>(event);
        if (drag_event->mimeData()->hasFormat(mime_type)) {
            bool ok = false;
            const int source = drag_event->mimeData()->data(mime_type).toInt(&ok);
            int indicator_y = 0;
            const int target = ok ? dropIndexAt(drag_event->position().toPoint(), source, &indicator_y) : -1;
            if (target >= 0) {
                drop_indicator_->setGeometry(0, indicator_y, tracks_widget_->width(), 2);
                drop_indicator_->raise();
                drop_indicator_->show();
                drag_event->acceptProposedAction();
                return true;
            }
        }
    } else if (event->type() == QEvent::DragLeave) {
        hideDropIndicator();
        return true;
    } else if (event->type() == QEvent::Drop) {
        auto *drop_event = static_cast<QDropEvent *>(event);
        if (drop_event->mimeData()->hasFormat(mime_type)) {
            bool ok = false;
            const int source = drop_event->mimeData()->data(mime_type).toInt(&ok);
            const int target = ok ? dropIndexAt(drop_event->position().toPoint(), source, nullptr) : -1;
            hideDropIndicator();
            if (target >= 0 && target != source) {
                auto moved = tracks_[static_cast<size_t>(source)];
                tracks_.erase(tracks_.begin() + source);
                tracks_.insert(tracks_.begin() + target, moved);
                rebuild_tracks();
                emit track_moved(playlist_id_, source, target);
            }
            drop_event->acceptProposedAction();
            return true;
        }
    }

    return QWidget::eventFilter(watched, event);
}

int PlaylistDetailView::dropIndexAt(const QPoint &position, int source_index, int *indicator_y) const {
    const int count = static_cast<int>(tracks_.size());
    if (count <= 1 || source_index < 0 || source_index >= count) return -1;

    int row_number = 0;
    for (int i = 0; i < tracks_layout_->count(); ++i) {
        QWidget *w = tracks_layout_->itemAt(i)->widget();
        auto *row = qobject_cast<PlaylistTrackRow *>(w);
        if (!row) continue;

        const QRect row_rect = row->geometry();
        if (position.y() < row_rect.center().y()) {
            if (indicator_y) *indicator_y = row_rect.top();
            return row_number > source_index ? row_number - 1 : row_number;
        }
        row_number++;
    }

    if (indicator_y) {
        QWidget *last = tracks_layout_->itemAt(tracks_layout_->count() - 1)->widget();
        *indicator_y = last ? last->geometry().bottom() + tracks_layout_->spacing() : tracks_widget_->height();
    }
    return count - 1;
}

void PlaylistDetailView::hideDropIndicator() {
    if (drop_indicator_) drop_indicator_->hide();
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

void PlaylistDetailView::update_theme() {
    const auto &c = DesignTokens::current();
    if (cover_label_) {
        cover_label_->setStyleSheet(QString("background-color: %1; border-radius: 12px;").arg(c.bg_elevated.name()));
    }
    if (title_label_) {
        title_label_->setStyleSheet(QString("color: %1; font-weight: bold;").arg(c.text_primary.name()));
    }
    if (desc_label_) {
        desc_label_->setStyleSheet(QString("color: %1;").arg(c.text_secondary.name()));
    }
    if (meta_label_) {
        meta_label_->setStyleSheet(QString("color: %1;").arg(c.text_muted.name()));
    }
    if (auto *back_btn = findChild<QPushButton*>("backBtn")) {
        back_btn->setStyleSheet(QString(
            "QPushButton { background: transparent; border: none; border-radius: 6px; }\n"
            "QPushButton:hover { background: rgba(%1, %2, %3, 0.08); }")
            .arg(c.text_primary.red()).arg(c.text_primary.green()).arg(c.text_primary.blue()));
    }
    if (auto *back_text = findChild<QLabel*>("backText")) {
        back_text->setStyleSheet(QString("color: %1; background: transparent;").arg(c.text_secondary.name()));
    }
    if (auto *type_lbl = findChild<QLabel*>("typeLabel")) {
        type_lbl->setStyleSheet(QString("color: %1; font-weight: bold; letter-spacing: 2px;").arg(c.text_muted.name()));
    }
    if (auto *play_btn = findChild<QPushButton*>("playBtn")) {
        play_btn->setStyleSheet(QString(
            "QPushButton { background: %1; border: none; border-radius: 20px; }\n"
            "QPushButton:hover { background: %2; }")
            .arg(c.accent.name()).arg(c.accent.lighter(115).name()));
    }
    if (auto *shuffle_btn = findChild<QPushButton*>("shuffleBtn")) {
        shuffle_btn->setStyleSheet(QString(
            "QPushButton { background: transparent; border: 1px solid %1; border-radius: 20px; }\n"
            "QPushButton:hover { background: rgba(%2, %3, %4, 0.08); }")
            .arg(c.border.name())
            .arg(c.text_primary.red()).arg(c.text_primary.green()).arg(c.text_primary.blue()));
    }
    if (auto *shuffle_t = findChild<QLabel*>("shuffleText")) {
        shuffle_t->setStyleSheet(QString("color: %1; background: transparent; font-weight: 600;").arg(c.text_primary.name()));
    }
    if (auto *dl_btn = findChild<QPushButton*>("dlBtn")) {
        dl_btn->setStyleSheet(QString(
            "QPushButton { background: transparent; border: 1px solid %1; border-radius: 20px; }\n"
            "QPushButton:hover { background: rgba(%2, %3, %4, 0.08); }")
            .arg(c.accent.name())
            .arg(c.accent.red()).arg(c.accent.green()).arg(c.accent.blue()));
    }
    if (auto *dl_t = findChild<QLabel*>("dlText")) {
        dl_t->setStyleSheet(QString("color: %1; background: transparent; font-weight: 600;").arg(c.accent.name()));
    }
    if (auto *dl_icon = findChild<QLabel*>("dlIcon")) {
        IconProvider::setupIconLabel(dl_icon, "download", 18, c.accent, false);
    }
    if (edit_btn_) {
        edit_btn_->setIcon(IconProvider::getIcon("edit", c.text_secondary, 18));
    }
    if (delete_btn_) {
        delete_btn_->setIcon(IconProvider::getIcon("delete", c.error, 18));
    }
    for (auto *row : findChildren<PlaylistTrackRow*>()) {
        row->update_theme();
    }
}

void PlaylistTrackRow::update_theme() {
    const auto &c = DesignTokens::current();
    bool unavailable = track_.id.empty();
    if (auto *num_lbl = findChild<QLabel*>("numLabel")) {
        num_lbl->setStyleSheet(QString("color: %1;").arg(c.text_muted.name()));
    }
    if (auto *t_lbl = findChild<QLabel*>("titleLabel")) {
        QString title_color = unavailable ? c.text_muted.name() : c.text_primary.name();
        t_lbl->setStyleSheet(QString("color: %1; font-weight: 600;").arg(title_color));
    }
    if (auto *a_lbl = findChild<QLabel*>("artistLabel")) {
        a_lbl->setStyleSheet(QString("color: %1;").arg(c.text_muted.name()));
    }
    if (auto *dur = findChild<QLabel*>("durationLabel")) {
        dur->setStyleSheet(QString("color: %1;").arg(c.text_muted.name()));
    }
    if (auto *fav_btn = findChild<QPushButton*>("favBtn")) {
        bool is_fav = get_track_favorite_state(static_cast<std::string>(track_.id));
        fav_btn->setIcon(IconProvider::getIcon(
            is_fav ? "favorite" : "favorite_border",
            is_fav ? c.accent : c.text_muted, 16));
    }
    if (unavailable) {
        setStyleSheet(QString("QWidget#PlaylistTrackRow_unavailable { background-color: transparent; border-radius: 6px; }\n"
                              "QWidget#PlaylistTrackRow_unavailable:hover { background-color: rgba(%1, %2, %3, 0.03); }")
            .arg(c.text_primary.red()).arg(c.text_primary.green()).arg(c.text_primary.blue()));
    } else {
        setStyleSheet("QWidget#PlaylistTrackRow { background-color: transparent; border-radius: 6px; }");
    }
}
