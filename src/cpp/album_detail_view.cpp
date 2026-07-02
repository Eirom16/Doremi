#include "album_detail_view.h"
#include "design_tokens.h"
#include "icon_provider.h"
#include "components/artwork_loader.h"
#include <QPainter>
#include <QPainterPath>
#include <QPushButton>
#include <QStyle>
#include <QPointer>
#include "doremi/src/bridge.rs.h"
#include "components/loading_state.h"
#include "components/empty_state.h"


// ─────────────────────────────────────────────────────────────────────────────
// AlbumDetailView
// ─────────────────────────────────────────────────────────────────────────────

AlbumDetailView::AlbumDetailView(QWidget *parent)
    : QWidget(parent)
{
    const auto &c = DesignTokens::current();
    setupLayout();
}

void AlbumDetailView::setupLayout() {
    const auto &s = DesignTokens::spacing();
    const auto &c = DesignTokens::current();

    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    scroll_area_ = new QScrollArea(this);
    scroll_area_->setWidgetResizable(true);
    scroll_area_->setFrameShape(QFrame::NoFrame);
    scroll_area_->setStyleSheet("QScrollArea { background: transparent; border: none; }");

    auto *scroll_content = new QWidget(scroll_area_);
    scroll_content->setObjectName("albumScrollContent");
    content_layout_ = new QVBoxLayout(scroll_content);
    content_layout_->setContentsMargins(DesignTokens::pagePaddingNarrow());
    content_layout_->setSpacing(s.md);
    content_layout_->setAlignment(Qt::AlignTop);

    // Back button
    auto *back_btn = new QPushButton(scroll_content);
    back_btn->setFixedSize(36, 36);
    back_btn->setCursor(Qt::PointingHandCursor);
    back_btn->setIcon(IconProvider::getIcon("arrow_back", DesignTokens::current().text_primary, 24));
    back_btn->setStyleSheet(DesignTokens::iconButtonStyle());
    connect(back_btn, &QPushButton::clicked, this, &AlbumDetailView::back_requested);
    content_layout_->addWidget(back_btn, 0, Qt::AlignLeft);

    // Header card: cover + info
    auto *header_card = new QWidget(scroll_content);
    header_card->setProperty("boxRole", "card");
    auto *header = new QHBoxLayout(header_card);
    int m = s.lg;
    header->setContentsMargins(m, m, m, m);
    header->setSpacing(24);

    cover_label_ = new QLabel(header_card);
    cover_label_->setFixedSize(180, 180);
    cover_label_->setStyleSheet(QString("background-color: %1; border-radius: %2px;").arg(DesignTokens::current().bg_elevated.name()).arg(DesignTokens::radius().lg));
    cover_label_->setAlignment(Qt::AlignCenter);
    header->addWidget(cover_label_);

    auto *info = new QVBoxLayout();
    info->setSpacing(6);

    title_label_ = new QLabel(tr_q("album_singular"), header_card);
    title_label_->setFont(DesignTokens::getFont("heading_lg"));
    title_label_->setProperty("textRole", "heading");
    title_label_->setWordWrap(true);
    info->addWidget(title_label_);

    artist_label_ = new QLabel(tr_q("artist_singular"), header_card);
    artist_label_->setFont(DesignTokens::getFont("body", 14));
    artist_label_->setProperty("textRole", "secondary");
    artist_label_->setCursor(Qt::PointingHandCursor);
    artist_label_->installEventFilter(this);
    info->addWidget(artist_label_);

    meta_label_ = new QLabel("", header_card);
    meta_label_->setFont(DesignTokens::getFont("caption", 12));
    meta_label_->setProperty("textRole", "muted");
    info->addWidget(meta_label_);

    auto *actions_layout = new QHBoxLayout();
    actions_layout->setSpacing(12);

    // Play button
    auto *play_btn = new QPushButton(header_card);
    play_btn->setFixedSize(36, 36);
    play_btn->setCursor(Qt::PointingHandCursor);
    play_btn->setIcon(IconProvider::getIcon("play_arrow", DesignTokens::current().text_primary, 24));
    play_btn->setStyleSheet(DesignTokens::iconButtonStyle());
    connect(play_btn, &QPushButton::clicked, this, [this]() {
        emit play_all_requested(tracks_);
    });

    auto *dl_all_btn = new QPushButton(header_card);
    auto *dl_layout = new QHBoxLayout(dl_all_btn);
    dl_layout->setContentsMargins(16, 0, 16, 0);
    dl_layout->setSpacing(8);
    
    auto *dl_icon = new QLabel;
    dl_icon->setPixmap(IconProvider::getIcon("download", DesignTokens::current().accent, 20).pixmap(20, 20));
    
    auto *dl_text = new QLabel(tr_q("download_all"));
    dl_text->setObjectName("dlText");
    dl_text->setFont(DesignTokens::getFont("body_sm"));
    dl_text->setProperty("textRole", "accent");
    dl_layout->addWidget(dl_icon);
    dl_layout->addWidget(dl_text);
    dl_all_btn->setLayout(dl_layout);
    dl_all_btn->setFixedHeight(40);
    dl_all_btn->setCursor(Qt::PointingHandCursor);
    dl_all_btn->setObjectName("dlAllBtn");
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
    tracks_layout_ = new QVBoxLayout(tracks_widget_);
    tracks_layout_->setContentsMargins(0, 0, 0, 0);
    tracks_layout_->setSpacing(2);
    content_layout_->addWidget(tracks_widget_);

    scroll_area_->setWidget(scroll_content);
    root->addWidget(scroll_area_);
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
    if (!album.thumbnail.empty()) {
        QPointer<QLabel> label_ptr(cover_label_);
        ArtworkLoader::load(QString::fromStdString(static_cast<std::string>(album.thumbnail)), QSize(180, 180), [label_ptr](const QPixmap &pixmap) {
            if (!label_ptr) return;
            QPixmap dest(pixmap.size());
            dest.fill(Qt::transparent);
            QPainter painter(&dest);
            painter.setRenderHint(QPainter::Antialiasing);
            QPainterPath path;
            path.addRoundedRect(pixmap.rect(), 12, 12);
            painter.setClipPath(path);
            painter.drawPixmap(0, 0, pixmap);
            label_ptr->setPixmap(dest.scaled(180, 180, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation));
        });
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
        auto *empty = new EmptyState(tracks_widget_);
        empty->setIcon("album");
        empty->setTitle(tr_q("empty_album_title"));
        empty->setDescription(tr_q("empty_album_desc"));
        empty->applyPanelStyle("empty");
        tracks_layout_->addWidget(empty);
    }
    updateGeometry();
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
    // Show loading skeleton immediately
    auto *loading = new LoadingState(LoadingState::ListRows, tracks_widget_);
    loading->setRowCount(4);
    loading->setRowHeight(48);
    tracks_layout_->addWidget(loading);
    updateGeometry();
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

void AlbumDetailView::update_theme() {
    const auto &c = DesignTokens::current();
    if (cover_label_) {
        cover_label_->setStyleSheet(QString("background-color: %1; border-radius: %2px;").arg(c.bg_elevated.name()).arg(DesignTokens::radius().lg));
    }
    for (auto *row : findChildren<AlbumTrackRow*>()) {
        row->update_theme();
    }
    style()->unpolish(this);
    style()->polish(this);
}
