#include "album_detail_view.h"
#include "design_tokens.h"
#include "icon_provider.h"
#include <QPainter>
#include <QPainterPath>
#include <QPushButton>
#include <QMenu>
#include "doremi/src/bridge.rs.h"

// ─────────────────────────────────────────────────────────────────────────────
// AlbumTrackRow
// ─────────────────────────────────────────────────────────────────────────────

AlbumTrackRow::AlbumTrackRow(int num, const QString &title, const QString &artist,
                             const QString &duration, Track track,
                             QWidget *parent)
    : QWidget(parent), track_(std::move(track))
{
    const auto &c = DesignTokens::current();
    setFixedHeight(48);
    setCursor(Qt::PointingHandCursor);

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

    // Title + artist
    auto *text = new QWidget(this);
    auto *text_l = new QVBoxLayout(text);
    text_l->setContentsMargins(0, 0, 0, 0);
    text_l->setSpacing(1);

    auto *t_lbl = new QLabel(title, this);
    t_lbl->setFont(DesignTokens::getFont("body", 13));
    t_lbl->setStyleSheet(QString("color: %1; font-weight: 600;").arg(c.text_primary.name()));

    auto *a_lbl = new QLabel(artist, this);
    a_lbl->setFont(DesignTokens::getFont("caption", 11));
    a_lbl->setStyleSheet(QString("color: %1;").arg(c.text_secondary.name()));

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

    // Favorite button
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

    setObjectName("AlbumTrackRow");
    setStyleSheet("QWidget#AlbumTrackRow { background-color: transparent; border-radius: 6px; }");
}

void AlbumTrackRow::mousePressEvent(QMouseEvent *event) {
    QWidget::mousePressEvent(event);
    if (event->button() == Qt::LeftButton)
        emit play_requested(track_);
}

void AlbumTrackRow::contextMenuEvent(QContextMenuEvent *event) {
    QMenu menu;
    QAction *play = menu.addAction("Reproducir");
    
    bool is_fav = get_track_favorite_state(static_cast<std::string>(track_.id));
    QAction *fav = menu.addAction(is_fav ? "Quitar de favoritos" : "Agregar a favoritos");
    
    QAction *dl = menu.addAction("Descargar");
    menu.addSeparator();
    QAction *next = menu.addAction("Reproducir siguiente");
    QAction *end = menu.addAction("Agregar a la cola");

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
    }
}

void AlbumTrackRow::enterEvent(QEnterEvent *event) {
    QWidget::enterEvent(event);
    const auto &c = DesignTokens::current();
    setStyleSheet(QString("QWidget#AlbumTrackRow { background-color: rgba(%1, %2, %3, 0.06); border-radius: 6px; }")
        .arg(c.text_primary.red()).arg(c.text_primary.green()).arg(c.text_primary.blue()));
}

void AlbumTrackRow::leaveEvent(QEvent *event) {
    QWidget::leaveEvent(event);
    setStyleSheet("QWidget#AlbumTrackRow { background-color: transparent; border-radius: 6px; }");
}

// ─────────────────────────────────────────────────────────────────────────────
// AlbumDetailView
// ─────────────────────────────────────────────────────────────────────────────

AlbumDetailView::AlbumDetailView(QWidget *parent)
    : QWidget(parent)
{
    setupLayout();
}

void AlbumDetailView::setupLayout() {
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
    connect(back_btn, &QPushButton::clicked, this, &AlbumDetailView::back_requested);
    content_layout_->addWidget(back_btn, 0, Qt::AlignLeft);

    // Header layout: cover + info
    auto *header = new QHBoxLayout();
    header->setSpacing(24);

    cover_label_ = new QLabel(this);
    cover_label_->setFixedSize(180, 180);
    cover_label_->setStyleSheet(QString("background-color: %1; border-radius: 12px;").arg(c.bg_elevated.name()));
    cover_label_->setAlignment(Qt::AlignCenter);
    header->addWidget(cover_label_);

    auto *info = new QVBoxLayout();
    info->setSpacing(6);

    title_label_ = new QLabel("Álbum", this);
    title_label_->setFont(DesignTokens::getFont("display", 24));
    title_label_->setStyleSheet(QString("color: %1; font-weight: bold;").arg(c.text_primary.name()));
    title_label_->setWordWrap(true);
    info->addWidget(title_label_);

    artist_label_ = new QLabel("Artista", this);
    artist_label_->setFont(DesignTokens::getFont("body", 14));
    artist_label_->setStyleSheet(QString(
        "QLabel { color: %1; background: transparent; }\n"
        "QLabel:hover { color: %2; text-decoration: underline; }"
    ).arg(c.text_secondary.name()).arg(c.accent.name()));
    artist_label_->setCursor(Qt::PointingHandCursor);
    artist_label_->installEventFilter(this);
    info->addWidget(artist_label_);

    meta_label_ = new QLabel("", this);
    meta_label_->setFont(DesignTokens::getFont("caption", 12));
    meta_label_->setStyleSheet(QString("color: %1;").arg(c.text_muted.name()));
    info->addWidget(meta_label_);

    // Play all button
    auto *play_all_btn = new QPushButton(this);
    auto *play_all_layout = new QHBoxLayout(play_all_btn);
    play_all_layout->setContentsMargins(16, 8, 20, 8);
    play_all_layout->setSpacing(8);
    auto *play_icon = IconProvider::createIconLabel("play_arrow", 20, QColor("#FFFFFF"), false, play_all_btn);
    auto *play_text = new QLabel("Reproducir todo", play_all_btn);
    play_text->setFont(DesignTokens::getFont("body", 13));
    play_text->setStyleSheet("color: #FFFFFF; background: transparent; font-weight: bold;");
    play_all_layout->addWidget(play_icon);
    play_all_layout->addWidget(play_text);
    play_all_btn->setLayout(play_all_layout);
    play_all_btn->setFixedHeight(40);
    play_all_btn->setCursor(Qt::PointingHandCursor);
    play_all_btn->setStyleSheet(QString(
        "QPushButton { background: %1; border: none; border-radius: 20px; }"
        "QPushButton:hover { background: %2; }")
        .arg(c.accent.name())
        .arg(c.accent.lighter(115).name()));
    connect(play_all_btn, &QPushButton::clicked, this, [this]() {
        emit play_all_requested(tracks_);
    });

    info->addSpacing(8);
    info->addWidget(play_all_btn, 0, Qt::AlignLeft);

    // Download all button
    auto *dl_all_btn = new QPushButton(this);
    auto *dl_layout = new QHBoxLayout(dl_all_btn);
    dl_layout->setContentsMargins(16, 8, 20, 8);
    dl_layout->setSpacing(8);
    auto *dl_icon = IconProvider::createIconLabel("download", 18, c.accent, false, dl_all_btn);
    auto *dl_text = new QLabel("Descargar todo", dl_all_btn);
    dl_text->setFont(DesignTokens::getFont("body", 13));
    dl_text->setStyleSheet(QString("color: %1; background: transparent; font-weight: 600;").arg(c.accent.name()));
    dl_layout->addWidget(dl_icon);
    dl_layout->addWidget(dl_text);
    dl_all_btn->setLayout(dl_layout);
    dl_all_btn->setFixedHeight(40);
    dl_all_btn->setCursor(Qt::PointingHandCursor);
    dl_all_btn->setStyleSheet(QString(
        "QPushButton { background: transparent; border: 1px solid %1; border-radius: 20px; }"
        "QPushButton:hover { background: rgba(%2, %3, %4, 0.08); }")
        .arg(c.accent.name())
        .arg(c.accent.red()).arg(c.accent.green()).arg(c.accent.blue()));
    connect(dl_all_btn, &QPushButton::clicked, this, [this]() {
        if (!tracks_.empty()) {
            const auto &a = current_album_;
            std::string pid = static_cast<std::string>(a.id);
            std::string pt = static_cast<std::string>(a.title);
            std::string pth = static_cast<std::string>(a.thumbnail);
            emit download_all_requested(tracks_, pid, pt, pth);
        }
    });

    info->addSpacing(8);
    info->addWidget(dl_all_btn, 0, Qt::AlignLeft);
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

void AlbumDetailView::set_album_info(const Album &album) {
    current_album_ = album;
    const auto &c = DesignTokens::current();
    title_label_->setText(QString::fromStdString(static_cast<std::string>(album.title)));
    artist_label_->setText(QString::fromStdString(static_cast<std::string>(album.artist)));
    artist_id_ = static_cast<std::string>(album.artist_id);

    QString meta;
    if (!album.year.empty()) meta += QString::fromStdString(static_cast<std::string>(album.year));
    if (album.track_count > 0) {
        if (!meta.isEmpty()) meta += " · ";
        meta += QString("%1 canciones").arg(album.track_count);
    }
    meta_label_->setText(meta);

    // Load cover
    QPixmap pm;
    if (!album.thumbnail.empty() && pm.load(QString::fromStdString(static_cast<std::string>(album.thumbnail)))) {
        QPixmap dest(pm.size());
        dest.fill(Qt::transparent);
        QPainter painter(&dest);
        painter.setRenderHint(QPainter::Antialiasing);
        QPainterPath path;
        path.addRoundedRect(pm.rect(), 12, 12);
        painter.setClipPath(path);
        painter.drawPixmap(0, 0, pm);
        cover_label_->setPixmap(dest.scaled(180, 180, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation));
    } else {
        auto icon = IconProvider::getIcon("album", c.text_secondary, 64);
        cover_label_->setPixmap(icon.pixmap(64, 64));
    }
}

void AlbumDetailView::set_album_tracks(const std::vector<Track> &tracks) {
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
        auto *row = new AlbumTrackRow(
            static_cast<int>(i) + 1,
            QString::fromStdString(static_cast<std::string>(t.title)),
            QString::fromStdString(static_cast<std::string>(t.artist)),
            dur,
            t,
            tracks_widget_
        );
        connect(row, &AlbumTrackRow::play_requested, this, &AlbumDetailView::play_requested);
        tracks_layout_->addWidget(row);
    }

    if (tracks.empty()) {
        const auto &c = DesignTokens::current();
        auto *empty = new QLabel("No se encontraron canciones en este álbum.", tracks_widget_);
        empty->setAlignment(Qt::AlignCenter);
        empty->setFont(DesignTokens::getFont("caption", 12));
        empty->setStyleSheet(QString("color: %1; padding: 30px;").arg(c.text_muted.name()));
        tracks_layout_->addWidget(empty);
    }
}

void AlbumDetailView::clear() {
    title_label_->setText("Álbum");
    artist_label_->setText("Artista");
    artist_id_.clear();
    meta_label_->setText("");
    cover_label_->clear();
    QLayoutItem *item;
    while ((item = tracks_layout_->takeAt(0)) != nullptr) {
        if (item->widget()) item->widget()->deleteLater();
        delete item;
    }
}

bool AlbumDetailView::eventFilter(QObject *obj, QEvent *event) {
    if (obj == artist_label_) {
        if (event->type() == QEvent::MouseButtonPress) {
            auto *mouse_event = static_cast<QMouseEvent *>(event);
            if (mouse_event->button() == Qt::LeftButton && !artist_id_.empty()) {
                emit artist_requested(artist_id_);
                return true;
            }
        }
    }
    return QWidget::eventFilter(obj, event);
}
