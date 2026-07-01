#include "playlist_detail_view.h"
#include "design_tokens.h"
#include "icon_provider.h"
#include "components/artwork_loader.h"
#include <QApplication>
#include <QDrag>
#include <QStyle>
#include <QDragEnterEvent>
#include <QDragMoveEvent>
#include <QDropEvent>
#include <QMimeData>
#include <QPainter>
#include <QPainterPath>
#include <QPushButton>
#include <QPointer>
#include "doremi/src/bridge.rs.h"
#include "components/loading_state.h"
#include "components/empty_state.h"


// ─────────────────────────────────────────────────────────────────────────────
// PlaylistDetailView
// ─────────────────────────────────────────────────────────────────────────────

PlaylistDetailView::PlaylistDetailView(QWidget *parent)
    : QWidget(parent)
{
    setupLayout();
}

void PlaylistDetailView::setupLayout() {
    const auto &s = DesignTokens::spacing();

    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    scroll_area_ = new QScrollArea(this);
    scroll_area_->setWidgetResizable(true);
    scroll_area_->setFrameShape(QFrame::NoFrame);
    scroll_area_->setStyleSheet("QScrollArea { background: transparent; border: none; }");

    auto *scroll_content = new QWidget(scroll_area_);
    scroll_content->setObjectName("playlistScrollContent");
    content_layout_ = new QVBoxLayout(scroll_content);
    content_layout_->setContentsMargins(DesignTokens::pagePaddingNarrow());
    content_layout_->setSpacing(s.md);
    content_layout_->setAlignment(Qt::AlignTop);

    // Back button
    auto *back_btn = new QPushButton(scroll_content);
    back_btn->setObjectName("backBtn");
    auto *back_layout = new QHBoxLayout(back_btn);
    back_layout->setContentsMargins(8, 4, 12, 4);
    back_layout->setSpacing(6);
    auto *back_icon = IconProvider::createIconLabel("arrow_back", 18, DesignTokens::current().text_secondary, false, back_btn);
    auto *back_text = new QLabel(tr_q("go_back"), back_btn);
    back_text->setObjectName("backText");
    back_text->setFont(DesignTokens::getFont("caption", 12));
    back_text->setProperty("textRole", "secondary");
    back_layout->addWidget(back_icon);
    back_layout->addWidget(back_text);
    back_btn->setLayout(back_layout);
    back_btn->setFixedHeight(32);
    back_btn->setCursor(Qt::PointingHandCursor);
    back_btn->setObjectName("backBtn");
    connect(back_btn, &QPushButton::clicked, this, &PlaylistDetailView::back_requested);
    content_layout_->addWidget(back_btn, 0, Qt::AlignLeft);

    // Header card: cover + info
    auto *header_card = new QWidget(scroll_content);
    header_card->setProperty("boxRole", "card");
    auto *header = new QHBoxLayout(header_card);
    int m = s.lg;
    header->setContentsMargins(m, m, m, m);
    header->setSpacing(24);

    cover_label_ = new QLabel(header_card);
    cover_label_->setFixedSize(160, 160);
    cover_label_->setStyleSheet(QString("background-color: %1; border-radius: %2px;").arg(DesignTokens::current().bg_elevated.name()).arg(DesignTokens::radius().lg));
    cover_label_->setAlignment(Qt::AlignCenter);
    header->addWidget(cover_label_);

    auto *info = new QVBoxLayout();
    info->setSpacing(6);

    auto *type_lbl = new QLabel(tr_q("playlist_singular").toUpper(), header_card);
    type_lbl->setObjectName("typeLabel");
    type_lbl->setFont(DesignTokens::getFont("caption", 10));
    type_lbl->setProperty("textRole", "muted");
    info->addWidget(type_lbl);

    title_label_ = new QLabel(tr_q("playlist_singular"), header_card);
    title_label_->setFont(DesignTokens::getFont("heading_lg"));
    title_label_->setProperty("textRole", "heading");
    title_label_->setWordWrap(true);
    info->addWidget(title_label_);

    desc_label_ = new QLabel("", header_card);
    desc_label_->setFont(DesignTokens::getFont("caption_sm"));
    desc_label_->setProperty("textRole", "secondary");
    desc_label_->setWordWrap(true);
    desc_label_->setMaximumWidth(400);
    desc_label_->hide();
    info->addWidget(desc_label_);

    meta_label_ = new QLabel("", header_card);
    meta_label_->setFont(DesignTokens::getFont("caption", 12));
    meta_label_->setProperty("textRole", "muted");
    info->addWidget(meta_label_);

    // Action buttons
    auto *actions = new QHBoxLayout();
    actions->setSpacing(10);

    // Play all
    auto *play_btn = new QPushButton(header_card);
    play_btn->setObjectName("playBtn");
    auto *play_l = new QHBoxLayout(play_btn);
    play_l->setContentsMargins(16, 8, 20, 8);
    play_l->setSpacing(8);
    play_l->addWidget(IconProvider::createIconLabel("play_arrow", 20, DesignTokens::current().text_on_accent, false, play_btn));
    auto *play_t = new QLabel(tr_q("play"), play_btn);
    play_t->setFont(DesignTokens::getFont("body_sm"));
    play_t->setProperty("textRole", "primary");
    play_l->addWidget(play_t);
    play_btn->setLayout(play_l);
    play_btn->setFixedHeight(40);
    play_btn->setCursor(Qt::PointingHandCursor);
    play_btn->setProperty("buttonRole", "primary");
    connect(play_btn, &QPushButton::clicked, this, [this]() {
        emit play_all_requested(tracks_);
    });
    actions->addWidget(play_btn);

    // Shuffle
    auto *shuffle_btn = new QPushButton(header_card);
    shuffle_btn->setObjectName("shuffleBtn");
    auto *shuffle_l = new QHBoxLayout(shuffle_btn);
    shuffle_l->setContentsMargins(16, 8, 20, 8);
    shuffle_l->setSpacing(8);
    shuffle_l->addWidget(IconProvider::createIconLabel("shuffle", 18, DesignTokens::current().text_primary, false, shuffle_btn));
    auto *shuffle_t = new QLabel(tr_q("shuffle"), shuffle_btn);
    shuffle_t->setObjectName("shuffleText");
    shuffle_t->setFont(DesignTokens::getFont("body_sm"));
    shuffle_t->setProperty("textRole", "primary");
    shuffle_l->addWidget(shuffle_t);
    shuffle_btn->setLayout(shuffle_l);
    shuffle_btn->setFixedHeight(40);
    shuffle_btn->setCursor(Qt::PointingHandCursor);
    shuffle_btn->setObjectName("shuffleBtn");
    connect(shuffle_btn, &QPushButton::clicked, this, [this]() {
        emit shuffle_requested(tracks_);
    });
    actions->addWidget(shuffle_btn);

    // Download all
    auto *dl_btn = new QPushButton(header_card);
    dl_btn->setObjectName("dlBtn");
    auto *dl_l = new QHBoxLayout(dl_btn);
    dl_l->setContentsMargins(16, 8, 20, 8);
    dl_l->setSpacing(8);
    auto *dl_icon = IconProvider::createIconLabel("download", 18, DesignTokens::current().accent, false, dl_btn);
    dl_icon->setObjectName("dlIcon");
    dl_l->addWidget(dl_icon);
    auto *dl_t = new QLabel(tr_q("download_all"), dl_btn);
    dl_t->setObjectName("dlText");
    dl_t->setFont(DesignTokens::getFont("body_sm"));
    dl_t->setProperty("textRole", "accent");
    dl_l->addWidget(dl_t);
    dl_btn->setLayout(dl_l);
    dl_btn->setFixedHeight(40);
    dl_btn->setCursor(Qt::PointingHandCursor);
    dl_btn->setObjectName("dlBtn");
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
    edit_btn_ = new QPushButton(header_card);
    edit_btn_->setFixedSize(36, 36);
    edit_btn_->setCursor(Qt::PointingHandCursor);
    edit_btn_->setToolTip(tr_q("edit_playlist"));
    edit_btn_->setIcon(IconProvider::getIcon("edit", DesignTokens::current().text_secondary, 18));
    edit_btn_->setObjectName("editBtn");
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
            desc_label_->setText(new_desc);
            desc_label_->setVisible(!new_desc.isEmpty());
        }
    });
    actions->addWidget(edit_btn_);

    // Delete
    delete_btn_ = new QPushButton(header_card);
    delete_btn_->setFixedSize(36, 36);
    delete_btn_->setCursor(Qt::PointingHandCursor);
    delete_btn_->setToolTip(tr_q("delete_playlist"));
    delete_btn_->setIcon(IconProvider::getIcon("delete", DesignTokens::current().error, 18));
    delete_btn_->setObjectName("deleteBtn");
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

    header->addLayout(info, 1);
    content_layout_->addWidget(header_card);

    // Separator
    auto *sep = new QWidget(scroll_content);
    sep->setFixedHeight(1);
    sep->setProperty("boxRole", "separator");
    content_layout_->addWidget(sep);

    // Tracks container
    tracks_widget_ = new QWidget(scroll_content);
    tracks_widget_->setProperty("bgRole", "transparent");
    tracks_widget_->setAcceptDrops(true);
    tracks_widget_->installEventFilter(this);
    tracks_layout_ = new QVBoxLayout(tracks_widget_);
    tracks_layout_->setContentsMargins(0, 0, 0, 0);
    tracks_layout_->setSpacing(2);

    drop_indicator_ = new QFrame(tracks_widget_);
    drop_indicator_->setFixedHeight(2);
    drop_indicator_->setStyleSheet(QString("background-color: %1; border-radius: 1px;")
        .arg(DesignTokens::current().accent.name()));
    drop_indicator_->hide();

    content_layout_->addWidget(tracks_widget_);

    scroll_area_->setWidget(scroll_content);
    root->addWidget(scroll_area_);
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
        const QString title = title_label_ ? title_label_->text() : QString();
        // Loading state: no metadata yet (owner, privacy, count all empty)
        bool is_loading = meta_label_ && meta_label_->text().isEmpty();
        bool is_error = title.contains("No se pudo", Qt::CaseInsensitive);
        if (is_loading) {
            auto *loading = new LoadingState(LoadingState::ListRows, tracks_widget_);
            loading->setRowCount(4);
            loading->setRowHeight(48);
            tracks_layout_->addWidget(loading);
        } else {
            auto *empty = new EmptyState(tracks_widget_);
            empty->setIcon(is_error ? "error" : "playlist_play");
            empty->setTitle(is_error ? "Error" : "Playlist vacía");
            empty->applyPanelStyle(is_error ? "error" : "empty");
            
            QString message = is_error
                ? (desc_label_ && desc_label_->isVisible() ? desc_label_->text() : "No se pudo cargar esta playlist.")
                : "Esta playlist está vacía.";
            empty->setDescription(message);
            tracks_layout_->addWidget(empty);
        }
    }
    updateGeometry();
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
    // Show loading skeleton immediately
    auto *loading = new LoadingState(LoadingState::ListRows, tracks_widget_);
    loading->setRowCount(4);
    loading->setRowHeight(48);
    tracks_layout_->addWidget(loading);
    updateGeometry();
}

void PlaylistDetailView::update_theme() {
    const auto &c = DesignTokens::current();
    if (cover_label_) {
        cover_label_->setStyleSheet(QString("background-color: %1; border-radius: %2px;").arg(c.bg_elevated.name()).arg(DesignTokens::radius().lg));
    }
    if (drop_indicator_) {
        drop_indicator_->setStyleSheet(QString("background-color: %1; border-radius: 1px;").arg(c.accent.name()));
    }
    for (auto *row : findChildren<PlaylistTrackRow*>()) {
        row->update_theme();
    }
    style()->unpolish(this);
    style()->polish(this);
}
