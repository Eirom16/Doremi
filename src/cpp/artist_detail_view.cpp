#include "artist_detail_view.h"
#include "design_tokens.h"
#include "icon_provider.h"
#include "components/artwork_loader.h"
#include "components/loading_state.h"
#include <QPainter>
#include <QPainterPath>
#include <QPushButton>
#include <QStyle>
#include <QPointer>
#include <QScrollArea>
#include "components/album_card.h"
#include "components/horizontal_carousel.h"
#include "doremi/src/bridge.rs.h"

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

    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    scroll_area_ = new QScrollArea(this);
    scroll_area_->setWidgetResizable(true);
    scroll_area_->setFrameShape(QFrame::NoFrame);
    scroll_area_->setStyleSheet("QScrollArea { background: transparent; border: none; }");

    auto *scroll_content = new QWidget(scroll_area_);
    scroll_content->setObjectName("artistScrollContent");
    content_layout_ = new QVBoxLayout(scroll_content);
    content_layout_->setContentsMargins(DesignTokens::pagePaddingNarrow());
    content_layout_->setSpacing(DesignTokens::spacing().md);
    content_layout_->setAlignment(Qt::AlignTop);

    // Back button
    auto *back_btn = new QPushButton(scroll_content);
    back_btn->setObjectName("backBtn");
    auto *back_layout = new QHBoxLayout(back_btn);
    back_layout->setContentsMargins(8, 4, 12, 4);
    back_layout->setSpacing(6);
    auto *back_icon = IconProvider::createIconLabel("arrow_back", 18, c.text_secondary, false, back_btn);
    auto *back_text = new QLabel("Volver", back_btn);
    back_text->setObjectName("backText");
    back_text->setFont(DesignTokens::getFont("caption", 12));
    back_text->setProperty("textRole", "secondary");
    back_layout->addWidget(back_icon);
    back_layout->addWidget(back_text);
    back_btn->setLayout(back_layout);
    back_btn->setFixedHeight(32);
    back_btn->setCursor(Qt::PointingHandCursor);
    back_btn->setObjectName("backBtn");
    connect(back_btn, &QPushButton::clicked, this, &ArtistDetailView::back_requested);
    content_layout_->addWidget(back_btn, 0, Qt::AlignLeft);

    // Header card: avatar + name info
    auto *header_card = new QWidget(scroll_content);
    header_card->setProperty("boxRole", "card");
    auto *header_layout = new QHBoxLayout(header_card);
    int m = DesignTokens::spacing().lg;
    header_layout->setContentsMargins(m, m, m, m);
    header_layout->setSpacing(20);

    avatar_label_ = new QLabel(header_card);
    avatar_label_->setFixedSize(140, 140);
    avatar_label_->setStyleSheet(QString("background-color: %1; border-radius: %2px;").arg(c.bg_elevated.name()).arg(DesignTokens::radius().pill));
    avatar_label_->setAlignment(Qt::AlignCenter);
    header_layout->addWidget(avatar_label_);

    auto *info = new QVBoxLayout();
    info->setSpacing(6);

    auto *type_lbl = new QLabel("ARTISTA", header_card);
    type_lbl->setObjectName("typeLabel");
    type_lbl->setFont(DesignTokens::getFont("caption", 10));
    type_lbl->setProperty("textRole", "muted");
    info->addWidget(type_lbl);

    name_label_ = new QLabel("Artista", header_card);
    name_label_->setFont(DesignTokens::getFont("heading_lg"));
    name_label_->setProperty("textRole", "heading");
    name_label_->setWordWrap(true);
    info->addWidget(name_label_);

    meta_label_ = new QLabel("", header_card);
    meta_label_->setFont(DesignTokens::getFont("caption", 12));
    meta_label_->setProperty("textRole", "muted");
    info->addWidget(meta_label_);

    desc_label_ = new QLabel("", header_card);
    desc_label_->setFont(DesignTokens::getFont("caption_sm"));
    desc_label_->setProperty("textRole", "secondary");
    desc_label_->setWordWrap(true);
    desc_label_->setMaximumWidth(500);
    desc_label_->hide();
    info->addWidget(desc_label_);

    info->addStretch();
    header_layout->addLayout(info, 1);
    header_card->setLayout(header_layout);
    content_layout_->addWidget(header_card);

    // Separator
    auto *sep = new QWidget(scroll_content);
    sep->setObjectName("separator");
    sep->setFixedHeight(1);
    sep->setProperty("boxRole", "separator");
    content_layout_->addWidget(sep);

    // Tracks section header
    auto *tracks_header = new QLabel("Canciones populares", scroll_content);
    tracks_header->setObjectName("tracksHeader");
    tracks_header->setFont(DesignTokens::getFont("heading_sm", 14));
    tracks_header->setProperty("textRole", "heading");
    content_layout_->addWidget(tracks_header);

    // Tracks container
    tracks_widget_ = new QWidget(scroll_content);
    tracks_widget_->setProperty("boxRole", "card");
    tracks_layout_ = new QVBoxLayout(tracks_widget_);
    tracks_layout_->setContentsMargins(0, 0, 0, 0);
    tracks_layout_->setSpacing(2);
    content_layout_->addWidget(tracks_widget_);

    content_layout_->addStretch(1);

    scroll_content->setLayout(content_layout_);
    scroll_area_->setWidget(scroll_content);
    root->addWidget(scroll_area_, 1);
    setLayout(root);
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

    if (!artist.thumbnail.empty()) {
        QPointer<QLabel> label_ptr(avatar_label_);
        ArtworkLoader::load(QString::fromStdString(static_cast<std::string>(artist.thumbnail)), QSize(140, 140), [label_ptr](const QPixmap &pixmap) {
            if (!label_ptr) return;
            QPixmap dest(pixmap.size());
            dest.fill(Qt::transparent);
            QPainter painter(&dest);
            painter.setRenderHint(QPainter::Antialiasing);
            QPainterPath path;
            int side = qMin(pixmap.width(), pixmap.height());
            path.addEllipse(0, 0, side, side);
            painter.setClipPath(path);
            painter.drawPixmap(0, 0, pixmap);
            label_ptr->setPixmap(dest.scaled(140, 140, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation));
        });
    } else {
        auto icon = IconProvider::getIcon("person", c.text_secondary, 60);
        avatar_label_->setPixmap(icon.pixmap(60, 60));
    }
}

void ArtistDetailView::set_artist_tracks(const std::vector<Track> &tracks,
                                          const std::vector<Album> &albums,
                                          const std::vector<Album> &singles) {
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
        auto *empty = new QLabel("No se encontraron canciones de este artista.", tracks_widget_);
        empty->setAlignment(Qt::AlignCenter);
        empty->setFont(DesignTokens::getFont("caption", 12));
        empty->setProperty("textRole", "muted");
        tracks_layout_->addWidget(empty);
    }

    // Clear dynamic sections
    if (albums_container_) {
        content_layout_->removeWidget(albums_container_);
        albums_container_->deleteLater();
        albums_container_ = nullptr;
    }
    if (singles_container_) {
        content_layout_->removeWidget(singles_container_);
        singles_container_->deleteLater();
        singles_container_ = nullptr;
    }

    // Albums section
    if (!albums.empty()) {
        albums_container_ = new QWidget(this);
        auto *lay = new QVBoxLayout(albums_container_);
        lay->setContentsMargins(0, 0, 0, 0);
        lay->setSpacing(DesignTokens::spacing().sm);
        
        auto *header = new QLabel("Álbumes", albums_container_);
        header->setObjectName("sectionHeader");
        header->setFont(DesignTokens::getFont("heading_sm", 14));
        header->setProperty("textRole", "heading");
        lay->addWidget(header);
        
        auto *carousel = new HorizontalCarousel(albums_container_);
        for (const auto &album : albums) {
            auto *card = new AlbumCard(
                QString::fromStdString(static_cast<std::string>(album.title)),
                QString::fromStdString(static_cast<std::string>(album.artist)),
                QString::fromStdString(static_cast<std::string>(album.thumbnail)),
                carousel
            );
            card->setItemId(static_cast<std::string>(album.id));
            card->setFixedWidth(160);
            connect(card, &AlbumCard::clicked, this, [this, card]() {
                emit album_requested(card->itemId());
            });
            carousel->addWidget(card);
        }
        lay->addWidget(carousel);
        albums_container_->setLayout(lay);
        content_layout_->addWidget(albums_container_);
    }

    // Singles section
    if (!singles.empty()) {
        singles_container_ = new QWidget(this);
        auto *lay = new QVBoxLayout(singles_container_);
        lay->setContentsMargins(0, 0, 0, 0);
        lay->setSpacing(DesignTokens::spacing().sm);
        
        auto *header = new QLabel("Singles y EPs", singles_container_);
        header->setObjectName("sectionHeader");
        header->setFont(DesignTokens::getFont("heading_sm", 14));
        header->setProperty("textRole", "heading");
        lay->addWidget(header);
        
        auto *carousel = new HorizontalCarousel(singles_container_);
        for (const auto &single : singles) {
            auto *card = new AlbumCard(
                QString::fromStdString(static_cast<std::string>(single.title)),
                QString::fromStdString(static_cast<std::string>(single.artist)),
                QString::fromStdString(static_cast<std::string>(single.thumbnail)),
                carousel
            );
            card->setItemId(static_cast<std::string>(single.id));
            card->setFixedWidth(160);
            connect(card, &AlbumCard::clicked, this, [this, card]() {
                emit album_requested(card->itemId());
            });
            carousel->addWidget(card);
        }
        lay->addWidget(carousel);
        singles_container_->setLayout(lay);
        content_layout_->addWidget(singles_container_);
    }
    updateGeometry();
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
    if (albums_container_) {
        content_layout_->removeWidget(albums_container_);
        albums_container_->deleteLater();
        albums_container_ = nullptr;
    }
    if (singles_container_) {
        content_layout_->removeWidget(singles_container_);
        singles_container_->deleteLater();
        singles_container_ = nullptr;
    }
    // Show loading skeleton immediately
    auto *loading = new LoadingState(LoadingState::ListRows, tracks_widget_);
    loading->setRowCount(4);
    loading->setRowHeight(48);
    tracks_layout_->addWidget(loading);
    updateGeometry();
}

void ArtistDetailView::update_theme() {
    const auto &c = DesignTokens::current();
    if (avatar_label_) {
        avatar_label_->setStyleSheet(QString("background-color: %1; border-radius: %2px;").arg(c.bg_elevated.name()).arg(DesignTokens::radius().pill));
    }
    for (auto *row : findChildren<ArtistTrackRow*>()) {
        row->update_theme();
    }
    style()->unpolish(this);
    style()->polish(this);
}
