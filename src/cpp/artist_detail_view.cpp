#include "artist_detail_view.h"
#include "design_tokens.h"
#include "icon_provider.h"
#include <QScrollBar>
#include <QPainter>
#include <QPainterPath>
#include <QPushButton>
#include <QMenu>
#include "components/album_card.h"
#include "doremi/src/bridge.rs.h"

// ─────────────────────────────────────────────────────────────────────────────
// ArtistTrackRow
// ─────────────────────────────────────────────────────────────────────────────

ArtistTrackRow::ArtistTrackRow(const QString &title, const QString &album,
                               const QString &duration, Track track,
                               QWidget *parent)
    : QWidget(parent), track_(std::move(track))
{
    const auto &c = DesignTokens::current();
    setFixedHeight(52);
    setCursor(Qt::PointingHandCursor);

    auto *layout = new QHBoxLayout(this);
    layout->setContentsMargins(16, 4, 16, 4);
    layout->setSpacing(14);

    // Play icon
    auto *play = IconProvider::createIconLabel("play_arrow", 16, c.text_muted, false, this);
    layout->addWidget(play);

    // Title
    auto *t_lbl = new QLabel(title, this);
    t_lbl->setFont(DesignTokens::getFont("body", 13));
    t_lbl->setStyleSheet(QString("color: %1; font-weight: 600;").arg(c.text_primary.name()));
    layout->addWidget(t_lbl, 2);

    // Album
    if (!album.isEmpty()) {
        auto *a_lbl = new QLabel(album, this);
        a_lbl->setFont(DesignTokens::getFont("caption", 11));
        a_lbl->setStyleSheet(QString("color: %1;").arg(c.text_secondary.name()));
        a_lbl->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        layout->addWidget(a_lbl, 1);
    }

    // Duration
    if (!duration.isEmpty()) {
        auto *dur = new QLabel(duration, this);
        dur->setFont(DesignTokens::getFont("caption", 11));
        dur->setStyleSheet(QString("color: %1;").arg(c.text_muted.name()));
        dur->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        layout->addWidget(dur);
    }

    setObjectName("ArtistTrackRow");
    setStyleSheet("QWidget#ArtistTrackRow { background-color: transparent; border-radius: 6px; }");
}

void ArtistTrackRow::mousePressEvent(QMouseEvent *event) {
    QWidget::mousePressEvent(event);
    if (event->button() == Qt::LeftButton) emit play_requested(track_);
}

void ArtistTrackRow::contextMenuEvent(QContextMenuEvent *event) {
    QMenu menu;
    QAction *play = menu.addAction("Reproducir");
    QAction *fav = menu.addAction("Agregar a favoritos");
    QAction *dl = menu.addAction("Descargar");
    menu.addSeparator();
    QAction *next = menu.addAction("Reproducir siguiente");
    QAction *end = menu.addAction("Agregar a la cola");

    QAction *chosen = menu.exec(event->globalPos());
    if (chosen == play) {
        emit play_requested(track_);
    } else if (chosen == fav) {
        on_add_favorite(track_);
    } else if (chosen == dl) {
        on_download_requested(track_);
    } else if (chosen == next) {
        on_add_to_queue_next(track_);
    } else if (chosen == end) {
        on_add_to_queue_end(track_);
    }
}

void ArtistTrackRow::enterEvent(QEnterEvent *event) {
    QWidget::enterEvent(event);
    const auto &c = DesignTokens::current();
    setStyleSheet(QString("QWidget#ArtistTrackRow { background-color: rgba(%1, %2, %3, 0.06); border-radius: 6px; }")
        .arg(c.text_primary.red()).arg(c.text_primary.green()).arg(c.text_primary.blue()));
}

void ArtistTrackRow::leaveEvent(QEvent *event) {
    QWidget::leaveEvent(event);
    setStyleSheet("QWidget#ArtistTrackRow { background-color: transparent; border-radius: 6px; }");
}

// ─────────────────────────────────────────────────────────────────────────────
// ArtistDetailView
// ─────────────────────────────────────────────────────────────────────────────

ArtistDetailView::ArtistDetailView(QWidget *parent)
    : QWidget(parent)
{
    setupLayout();
}

void ArtistDetailView::setupLayout() {
    const auto &c = DesignTokens::current();

    auto *main_vbox = new QVBoxLayout(this);
    main_vbox->setContentsMargins(0, 0, 0, 0);
    main_vbox->setSpacing(0);

    scroll_area_ = new QScrollArea(this);
    scroll_area_->setWidgetResizable(true);
    scroll_area_->setFrameShape(QFrame::NoFrame);
    scroll_area_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scroll_area_->setStyleSheet("background: transparent;");

    scroll_content_ = new QWidget(scroll_area_);
    scroll_content_->setStyleSheet("background: transparent;");

    content_layout_ = new QVBoxLayout(scroll_content_);
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
    connect(back_btn, &QPushButton::clicked, this, &ArtistDetailView::back_requested);
    content_layout_->addWidget(back_btn, 0, Qt::AlignLeft);

    // Header: avatar + name
    auto *header = new QHBoxLayout();
    header->setSpacing(20);

    avatar_label_ = new QLabel(scroll_content_);
    avatar_label_->setFixedSize(140, 140);
    avatar_label_->setStyleSheet(QString("background-color: %1; border-radius: 70px;").arg(c.bg_elevated.name()));
    avatar_label_->setAlignment(Qt::AlignCenter);
    header->addWidget(avatar_label_);

    auto *info = new QVBoxLayout();
    info->setSpacing(6);

    auto *type_lbl = new QLabel("ARTISTA", scroll_content_);
    type_lbl->setFont(DesignTokens::getFont("caption", 10));
    type_lbl->setStyleSheet(QString("color: %1; font-weight: bold; letter-spacing: 2px;").arg(c.text_muted.name()));
    info->addWidget(type_lbl);

    name_label_ = new QLabel("Artista", scroll_content_);
    name_label_->setFont(DesignTokens::getFont("display", 28));
    name_label_->setStyleSheet(QString("color: %1; font-weight: bold;").arg(c.text_primary.name()));
    name_label_->setWordWrap(true);
    info->addWidget(name_label_);

    meta_label_ = new QLabel("", scroll_content_);
    meta_label_->setFont(DesignTokens::getFont("caption", 12));
    meta_label_->setStyleSheet(QString("color: %1;").arg(c.text_muted.name()));
    info->addWidget(meta_label_);

    desc_label_ = new QLabel("", scroll_content_);
    desc_label_->setFont(DesignTokens::getFont("caption", 11));
    desc_label_->setStyleSheet(QString("color: %1;").arg(c.text_secondary.name()));
    desc_label_->setWordWrap(true);
    desc_label_->setMaximumWidth(500);
    desc_label_->hide();
    info->addWidget(desc_label_);

    info->addStretch();
    header->addLayout(info, 1);
    content_layout_->addLayout(header);

    // Separator
    auto *sep = new QWidget(scroll_content_);
    sep->setFixedHeight(1);
    sep->setStyleSheet(QString("background-color: %1;").arg(c.border.name()));
    content_layout_->addSpacing(12);
    content_layout_->addWidget(sep);
    content_layout_->addSpacing(4);

    // Tracks section header
    auto *tracks_header = new QLabel("Canciones populares", scroll_content_);
    tracks_header->setFont(DesignTokens::getFont("heading_sm", 14));
    tracks_header->setStyleSheet(QString("color: %1; font-weight: bold;").arg(c.text_primary.name()));
    content_layout_->addWidget(tracks_header);

    // Tracks container
    tracks_widget_ = new QWidget(scroll_content_);
    tracks_widget_->setStyleSheet("background: transparent;");
    tracks_layout_ = new QVBoxLayout(tracks_widget_);
    tracks_layout_->setContentsMargins(0, 0, 0, 0);
    tracks_layout_->setSpacing(2);
    content_layout_->addWidget(tracks_widget_);

    // Albums section
    content_layout_->addSpacing(16);
    auto *albums_header = new QLabel("Álbumes", scroll_content_);
    albums_header->setFont(DesignTokens::getFont("heading_sm", 14));
    albums_header->setStyleSheet(QString("color: %1; font-weight: bold;").arg(c.text_primary.name()));
    content_layout_->addWidget(albums_header);

    albums_widget_ = new QWidget(scroll_content_);
    albums_widget_->setStyleSheet("background: transparent;");
    albums_layout_ = new QVBoxLayout(albums_widget_);
    albums_layout_->setContentsMargins(0, 0, 0, 0);
    albums_layout_->setSpacing(2);
    content_layout_->addWidget(albums_widget_);

    scroll_area_->setWidget(scroll_content_);
    main_vbox->addWidget(scroll_area_);
    setLayout(main_vbox);
}

void ArtistDetailView::set_artist_info(const Artist &artist) {
    const auto &c = DesignTokens::current();
    name_label_->setText(QString::fromStdString(static_cast<std::string>(artist.name)));

    if (!artist.subscribers.empty()) {
        meta_label_->setText(QString::fromStdString(static_cast<std::string>(artist.subscribers)) + " suscriptores");
    }

    if (!artist.description.empty()) {
        desc_label_->setText(QString::fromStdString(static_cast<std::string>(artist.description)));
        desc_label_->show();
    } else {
        desc_label_->hide();
    }

    QPixmap pm;
    if (!artist.thumbnail.empty() && pm.load(QString::fromStdString(static_cast<std::string>(artist.thumbnail)))) {
        QPixmap dest(pm.size());
        dest.fill(Qt::transparent);
        QPainter painter(&dest);
        painter.setRenderHint(QPainter::Antialiasing);
        QPainterPath path;
        int side = qMin(pm.width(), pm.height());
        path.addEllipse(0, 0, side, side);
        painter.setClipPath(path);
        painter.drawPixmap(0, 0, pm);
        avatar_label_->setPixmap(dest.scaled(140, 140, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation));
    } else {
        auto icon = IconProvider::getIcon("person", c.text_secondary, 60);
        avatar_label_->setPixmap(icon.pixmap(60, 60));
    }
}

void ArtistDetailView::set_artist_tracks(const std::vector<Track> &tracks,
                                          const std::vector<Album> &albums) {
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

        auto *row = new ArtistTrackRow(
            QString::fromStdString(static_cast<std::string>(t.title)),
            QString::fromStdString(static_cast<std::string>(t.album)),
            dur,
            t,
            tracks_widget_
        );
        connect(row, &ArtistTrackRow::play_requested, this, &ArtistDetailView::play_requested);
        tracks_layout_->addWidget(row);
    }

    if (tracks.empty()) {
        const auto &c = DesignTokens::current();
        auto *empty = new QLabel("No se encontraron canciones de este artista.", tracks_widget_);
        empty->setAlignment(Qt::AlignCenter);
        empty->setFont(DesignTokens::getFont("caption", 12));
        empty->setStyleSheet(QString("color: %1; padding: 30px;").arg(c.text_muted.name()));
        tracks_layout_->addWidget(empty);
    }

    // Albums section
    albums_ = albums;
    while ((item = albums_layout_->takeAt(0)) != nullptr) {
        if (item->widget()) item->widget()->deleteLater();
        delete item;
    }

    if (albums.empty()) {
        albums_widget_->hide();
        return;
    }
    albums_widget_->show();

    auto *cards_layout = new QHBoxLayout();
    cards_layout->setSpacing(12);
    cards_layout->setAlignment(Qt::AlignLeft);

    for (const auto &album : albums) {
        auto *card = new AlbumCard(
            QString::fromStdString(static_cast<std::string>(album.title)),
            QString::fromStdString(static_cast<std::string>(album.artist)),
            QString::fromStdString(static_cast<std::string>(album.thumbnail)),
            albums_widget_
        );
        card->setItemId(static_cast<std::string>(album.id));
        card->setFixedWidth(160);
        connect(card, &AlbumCard::clicked, this, [this, card]() {
            emit album_requested(card->itemId());
        });
        cards_layout->addWidget(card);
    }
    albums_layout_->addLayout(cards_layout);
    albums_layout_->addStretch();
}

void ArtistDetailView::clear() {
    name_label_->setText("Artista");
    meta_label_->setText("");
    desc_label_->hide();
    avatar_label_->clear();
    QLayoutItem *item;
    while ((item = tracks_layout_->takeAt(0)) != nullptr) {
        if (item->widget()) item->widget()->deleteLater();
        delete item;
    }
    while ((item = albums_layout_->takeAt(0)) != nullptr) {
        if (item->widget()) item->widget()->deleteLater();
        delete item;
    }
    albums_.clear();
}
