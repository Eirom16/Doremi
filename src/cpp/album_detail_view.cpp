#include "album_detail_view.h"
#include "design_tokens.h"
#include "icon_provider.h"
#include "components/artwork_loader.h"
#include <QPainter>
#include <QPainterPath>
#include <QPushButton>
#include <QPointer>
#include "doremi/src/bridge.rs.h"

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
    content_layout_->setContentsMargins(DesignTokens::pagePaddingNarrow());
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
        "QPushButton { background: transparent; border: none; border-radius: %4px; }"
        "QPushButton:hover { background: rgba(%1, %2, %3, 0.08); }")
        .arg(c.text_primary.red()).arg(c.text_primary.green()).arg(c.text_primary.blue()).arg(DesignTokens::radius().sm));
    connect(back_btn, &QPushButton::clicked, this, &AlbumDetailView::back_requested);
    content_layout_->addWidget(back_btn, 0, Qt::AlignLeft);

    // Header layout: cover + info
    auto *header = new QHBoxLayout();
    header->setSpacing(24);

    cover_label_ = new QLabel(this);
    cover_label_->setFixedSize(180, 180);
    cover_label_->setStyleSheet(QString("background-color: %1; border-radius: %2px;").arg(c.bg_elevated.name()).arg(DesignTokens::radius().lg));
    cover_label_->setAlignment(Qt::AlignCenter);
    header->addWidget(cover_label_);

    auto *info = new QVBoxLayout();
    info->setSpacing(6);

    title_label_ = new QLabel(tr_q("album_singular"), this);
    title_label_->setFont(DesignTokens::getFont("heading_lg"));
    title_label_->setStyleSheet(QString("color: %1; font-weight: bold;").arg(c.text_primary.name()));
    title_label_->setWordWrap(true);
    info->addWidget(title_label_);

    artist_label_ = new QLabel(tr_q("artist_singular"), this);
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
    play_all_btn->setObjectName("playAllBtn");
    auto *play_all_layout = new QHBoxLayout(play_all_btn);
    play_all_layout->setContentsMargins(16, 8, 20, 8);
    play_all_layout->setSpacing(8);
    auto *play_icon = IconProvider::createIconLabel("play_arrow", 20, c.text_on_accent, false, play_all_btn);
    auto *play_text = new QLabel(tr_q("play_all"), play_all_btn);
    play_text->setFont(DesignTokens::getFont("body_sm"));
    play_text->setStyleSheet(QString("color: %1; background: transparent; font-weight: bold;").arg(c.text_on_accent.name()));
    play_all_layout->addWidget(play_icon);
    play_all_layout->addWidget(play_text);
    play_all_btn->setLayout(play_all_layout);
    play_all_btn->setFixedHeight(40);
    play_all_btn->setCursor(Qt::PointingHandCursor);
    play_all_btn->setStyleSheet(QString(
        "QPushButton { background: %1; border: none; border-radius: %3px; }"
        "QPushButton:hover { background: %2; }")
        .arg(c.accent.name())
        .arg(c.accent.lighter(115).name())
        .arg(DesignTokens::radius().pill));
    connect(play_all_btn, &QPushButton::clicked, this, [this]() {
        emit play_all_requested(tracks_);
    });

    info->addSpacing(8);
    info->addWidget(play_all_btn, 0, Qt::AlignLeft);

    // Download all button
    auto *dl_all_btn = new QPushButton(this);
    dl_all_btn->setObjectName("dlAllBtn");
    auto *dl_layout = new QHBoxLayout(dl_all_btn);
    dl_layout->setContentsMargins(16, 8, 20, 8);
    dl_layout->setSpacing(8);
    auto *dl_icon = IconProvider::createIconLabel("download", 18, c.accent, false, dl_all_btn);
    dl_icon->setObjectName("dlIcon");
    auto *dl_text = new QLabel(tr_q("download_all"), dl_all_btn);
    dl_text->setObjectName("dlText");
    dl_text->setFont(DesignTokens::getFont("body_sm"));
    dl_text->setStyleSheet(QString("color: %1; background: transparent; font-weight: 600;").arg(c.accent.name()));
    dl_layout->addWidget(dl_icon);
    dl_layout->addWidget(dl_text);
    dl_all_btn->setLayout(dl_layout);
    dl_all_btn->setFixedHeight(40);
    dl_all_btn->setCursor(Qt::PointingHandCursor);
    dl_all_btn->setStyleSheet(QString(
        "QPushButton { background: transparent; border: 1px solid %1; border-radius: %5px; }"
        "QPushButton:hover { background: rgba(%2, %3, %4, 0.08); }")
        .arg(c.accent.name())
        .arg(c.accent.red()).arg(c.accent.green()).arg(c.accent.blue())
        .arg(DesignTokens::radius().pill));
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

void AlbumDetailView::update_theme() {
    const auto &c = DesignTokens::current();
    if (cover_label_) {
        cover_label_->setStyleSheet(QString("background-color: %1; border-radius: %2px;").arg(c.bg_elevated.name()).arg(DesignTokens::radius().lg));
    }
    if (title_label_) {
        title_label_->setStyleSheet(QString("color: %1; font-weight: bold;").arg(c.text_primary.name()));
    }
    if (artist_label_) {
        artist_label_->setStyleSheet(QString(
            "QLabel { color: %1; background: transparent; }\n"
            "QLabel:hover { color: %2; text-decoration: underline; }"
        ).arg(c.text_secondary.name()).arg(c.accent.name()));
    }
    if (meta_label_) {
        meta_label_->setStyleSheet(QString("color: %1;").arg(c.text_muted.name()));
    }
    if (auto *back_btn = findChild<QPushButton*>("backBtn")) {
        back_btn->setStyleSheet(QString(
            "QPushButton { background: transparent; border: none; border-radius: %4px; }\n"
            "QPushButton:hover { background: rgba(%1, %2, %3, 0.08); }")
            .arg(c.text_primary.red()).arg(c.text_primary.green()).arg(c.text_primary.blue()).arg(DesignTokens::radius().sm));
    }
    if (auto *back_text = findChild<QLabel*>("backText")) {
        back_text->setStyleSheet(QString("color: %1; background: transparent;").arg(c.text_secondary.name()));
    }
    if (auto *play_all_btn = findChild<QPushButton*>("playAllBtn")) {
        play_all_btn->setStyleSheet(QString(
            "QPushButton { background: %1; border: none; border-radius: %3px; }\n"
            "QPushButton:hover { background: %2; }")
            .arg(c.accent.name())
            .arg(c.accent.lighter(115).name())
            .arg(DesignTokens::radius().pill));
    }
    if (auto *dl_all_btn = findChild<QPushButton*>("dlAllBtn")) {
        dl_all_btn->setStyleSheet(QString(
            "QPushButton { background: transparent; border: 1px solid %1; border-radius: %5px; }\n"
            "QPushButton:hover { background: rgba(%2, %3, %4, 0.08); }")
            .arg(c.accent.name())
            .arg(c.accent.red()).arg(c.accent.green()).arg(c.accent.blue())
            .arg(DesignTokens::radius().pill));
    }
    if (auto *dl_text = findChild<QLabel*>("dlText")) {
        dl_text->setStyleSheet(QString("color: %1; background: transparent; font-weight: 600;").arg(c.accent.name()));
    }
    if (auto *dl_icon = findChild<QLabel*>("dlIcon")) {
        IconProvider::setupIconLabel(dl_icon, "download", 18, c.accent, false);
    }
    for (auto *row : findChildren<AlbumTrackRow*>()) {
        row->update_theme();
    }
}


