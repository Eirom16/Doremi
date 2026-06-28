#include "components/track_row.h"
#include "design_tokens.h"
#include "icon_provider.h"
#include "components/artwork_loader.h"
#include <QMenu>
#include <QDrag>
#include <QMimeData>
#include <QApplication>
#include <QPainter>
#include <QPainterPath>
#include <QMessageBox>
#include <QDateTime>
#include <QPointer>

// ─────────────────────────────────────────────────────────────────────────────
// TrackRow (base)
// ─────────────────────────────────────────────────────────────────────────────

TrackRow::TrackRow(Track track, TrackRowConfig config, QWidget *parent)
    : QWidget(parent), track_(std::move(track)), config_(config)
{
    setFixedHeight(config_.height);
    setCursor(track_.id.empty() ? Qt::ArrowCursor : Qt::PointingHandCursor);

    layout_ = new QHBoxLayout(this);
    layout_->setContentsMargins(16, 4, 16, 4);
    layout_->setSpacing(14);

    setObjectName("TrackRow");
    setStyleSheet(QString("QWidget#TrackRow { background-color: transparent; border-radius: %1px; }")
        .arg(config_.corner_radius));
}

QWidget* TrackRow::createTextContainer(const QString &title, const QString &subtitle) {
    const auto &c = DesignTokens::current();
    bool unavailable = track_.id.empty();

    auto *text = new QWidget(this);
    auto *text_l = new QVBoxLayout(text);
    text_l->setContentsMargins(0, 0, 0, 0);
    text_l->setSpacing(1);

    title_label_ = new QLabel(title, this);
    title_label_->setObjectName("titleLabel");
    title_label_->setFont(DesignTokens::getFont("body_sm"));
    QString title_color = unavailable ? c.text_muted.name() : c.text_primary.name();
    title_label_->setStyleSheet(QString("color: %1; font-weight: 600;").arg(title_color));

    subtitle_label_ = new QLabel(unavailable ? "" : subtitle, this);
    subtitle_label_->setObjectName("subtitleLabel");
    subtitle_label_->setFont(DesignTokens::getFont("caption_sm"));
    subtitle_label_->setStyleSheet(QString("color: %1;").arg(c.text_muted.name()));

    text_l->addWidget(title_label_);
    text_l->addWidget(subtitle_label_);

    return text;
}

void TrackRow::addFavButton() {
    const auto &c = DesignTokens::current();
    bool is_fav = get_track_favorite_state(static_cast<std::string>(track_.id));

    fav_btn_ = new QPushButton(this);
    fav_btn_->setObjectName("favBtn");
    fav_btn_->setFixedSize(28, 28);
    fav_btn_->setCursor(Qt::PointingHandCursor);
    fav_btn_->setIcon(IconProvider::getIcon(
        is_fav ? "favorite" : "favorite_border",
        is_fav ? c.accent : c.text_muted, 16));
    fav_btn_->setStyleSheet("QPushButton { background: transparent; border: none; }");
    connect(fav_btn_, &QPushButton::clicked, this, [this, is_fav]() {
        if (is_fav) {
            on_remove_favorite(static_cast<std::string>(track_.id));
        } else {
            on_add_favorite(track_);
        }
    });
    layout_->addWidget(fav_btn_);
}

void TrackRow::setupCommonWidgets(const QString &title, const QString &subtitle, const QString &duration) {
    const auto &c = DesignTokens::current();
    bool unavailable = track_.id.empty();

    // Title + subtitle
    auto *text = createTextContainer(
        unavailable && subtitle.isEmpty() ? tr_q("not_available") : title,
        subtitle);
    layout_->addWidget(text, 2);

    // Duration
    if (!duration.isEmpty()) {
        duration_label_ = new QLabel(duration, this);
        duration_label_->setObjectName("durationLabel");
        duration_label_->setFont(DesignTokens::getFont("caption_sm"));
        duration_label_->setStyleSheet(QString("color: %1;").arg(c.text_muted.name()));
        duration_label_->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        layout_->addWidget(duration_label_);
    }

    // Fav button (only for available tracks)
    if (config_.show_fav && !unavailable) {
        addFavButton();
    }
}

void TrackRow::mousePressEvent(QMouseEvent *event) {
    QWidget::mousePressEvent(event);
    if (event->button() == Qt::LeftButton && !track_.id.empty()) {
        emit play_requested(track_);
    }
}

void TrackRow::mouseMoveEvent(QMouseEvent *event) {
    QWidget::mouseMoveEvent(event);
}

void TrackRow::mouseReleaseEvent(QMouseEvent *event) {
    QWidget::mouseReleaseEvent(event);
}

void TrackRow::contextMenuEvent(QContextMenuEvent *event) {
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

void TrackRow::enterEvent(QEnterEvent *event) {
    QWidget::enterEvent(event);
    hovered_ = true;
    const auto &c = DesignTokens::current();
    setStyleSheet(QString("QWidget#TrackRow { background-color: rgba(%1, %2, %3, 0.06); border-radius: %4px; }")
        .arg(c.text_primary.red()).arg(c.text_primary.green()).arg(c.text_primary.blue())
        .arg(config_.corner_radius));
}

void TrackRow::leaveEvent(QEvent *event) {
    QWidget::leaveEvent(event);
    hovered_ = false;
    setStyleSheet(QString("QWidget#TrackRow { background-color: transparent; border-radius: %1px; }")
        .arg(config_.corner_radius));
}

void TrackRow::update_theme() {
    const auto &c = DesignTokens::current();
    bool unavailable = track_.id.empty();

    if (title_label_) {
        QString title_color = unavailable ? c.text_muted.name() : c.text_primary.name();
        title_label_->setStyleSheet(QString("color: %1; font-weight: 600;").arg(title_color));
    }
    if (subtitle_label_) {
        subtitle_label_->setStyleSheet(QString("color: %1;").arg(c.text_muted.name()));
    }
    if (duration_label_) {
        duration_label_->setStyleSheet(QString("color: %1;").arg(c.text_muted.name()));
    }
    if (fav_btn_) {
        bool is_fav = get_track_favorite_state(static_cast<std::string>(track_.id));
        fav_btn_->setIcon(IconProvider::getIcon(
            is_fav ? "favorite" : "favorite_border",
            is_fav ? c.accent : c.text_muted, 16));
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// AlbumTrackRow
// ─────────────────────────────────────────────────────────────────────────────

AlbumTrackRow::AlbumTrackRow(int num, const QString &title, const QString &artist,
                              const QString &duration, Track track,
                              QWidget *parent)
    : TrackRow(std::move(track), {}, parent)
{
    setIndex(num - 1);

    // Track number
    const auto &c = DesignTokens::current();
    auto *num_lbl = new QLabel(QString::number(num), this);
    num_lbl->setObjectName("numLabel");
    num_lbl->setFont(DesignTokens::getFont("caption", 12));
    num_lbl->setFixedWidth(24);
    num_lbl->setAlignment(Qt::AlignCenter);
    num_lbl->setStyleSheet(QString("color: %1;").arg(c.text_muted.name()));
    layout_->addWidget(num_lbl);

    setupCommonWidgets(title, artist, duration);
}

// ─────────────────────────────────────────────────────────────────────────────
// PlaylistTrackRow
// ─────────────────────────────────────────────────────────────────────────────

PlaylistTrackRow::PlaylistTrackRow(int num, const QString &title, const QString &artist,
                                    const QString &duration, Track track,
                                    QWidget *parent)
    : TrackRow(std::move(track), {}, parent)
{
    config_.drag_drop = true;
    setIndex(num - 1);

    const auto &c = DesignTokens::current();

    // Track number
    auto *num_lbl = new QLabel(QString::number(num), this);
    num_lbl->setObjectName("numLabel");
    num_lbl->setFont(DesignTokens::getFont("caption", 12));
    num_lbl->setFixedWidth(24);
    num_lbl->setAlignment(Qt::AlignCenter);
    num_lbl->setStyleSheet(QString("color: %1;").arg(c.text_muted.name()));
    layout_->addWidget(num_lbl);

    setupCommonWidgets(title, artist, duration);
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

// ─────────────────────────────────────────────────────────────────────────────
// ArtistTrackRow
// ─────────────────────────────────────────────────────────────────────────────

ArtistTrackRow::ArtistTrackRow(const QString &title, const QString &album,
                                const QString &duration, Track track,
                                QWidget *parent)
    : TrackRow(std::move(track), {}, parent)
{
    config_.height = 52;
    const auto &c = DesignTokens::current();

    // Play icon instead of track number
    auto *play = IconProvider::createIconLabel("play_arrow", 16, c.text_muted, false, this);
    play->setObjectName("playIcon");
    layout_->addWidget(play);

    setupCommonWidgets(title, album, duration);
}

// ─────────────────────────────────────────────────────────────────────────────
// HistoryRow
// ─────────────────────────────────────────────────────────────────────────────

HistoryRow::HistoryRow(const Track &track,
                       const std::string &played_at,
                       const std::string &feedback_token,
                       QWidget *parent)
    : TrackRow(track, {}, parent),
      feedback_token_(feedback_token)
{
    config_.show_fav = false;
    config_.height = 64;
    config_.corner_radius = 8;
    const auto &c = DesignTokens::current();

    // Artwork thumbnail
    thumb_label_ = new QLabel(this);
    thumb_label_->setObjectName("thumbLabel");
    thumb_label_->setFixedSize(48, 48);
    thumb_label_->setStyleSheet(QString("background-color: %1; border-radius: %2px;").arg(c.bg_elevated.name()).arg(DesignTokens::radius().sm));
    loadThumbnail(track);
    layout_->addWidget(thumb_label_);

    QString title = QString::fromStdString(static_cast<std::string>(track.title));
    QString artist = QString::fromStdString(static_cast<std::string>(track.artist));

    // Title + artist
    auto *text = createTextContainer(title, artist);
    layout_->addWidget(text, 2);

    // Duration
    if (track.duration_ms > 0) {
        int secs = static_cast<int>(track.duration_ms / 1000);
        QString dur = QString("%1:%2").arg(secs / 60).arg(secs % 60, 2, 10, QChar('0'));
        duration_label_ = new QLabel(dur, this);
        duration_label_->setObjectName("durationLabel");
        duration_label_->setFont(DesignTokens::getFont("caption_sm"));
        duration_label_->setStyleSheet(QString("color: %1;").arg(c.text_muted.name()));
        duration_label_->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        layout_->addWidget(duration_label_);
    }

    // Time-ago label
    if (!played_at.empty()) {
        QDateTime dt = QDateTime::fromString(QString::fromStdString(played_at), Qt::ISODate);
        if (!dt.isValid()) {
            dt = QDateTime::fromString(QString::fromStdString(played_at), "yyyy-MM-dd HH:mm:ss");
        }
        QString ago;
        if (dt.isValid()) {
            qint64 secs_ago = dt.secsTo(QDateTime::currentDateTime());
            if (secs_ago < 60) ago = tr_q("now");
            else if (secs_ago < 3600) ago = QString("%1 min").arg(secs_ago / 60);
            else if (secs_ago < 86400) ago = QString("%1 h").arg(secs_ago / 3600);
            else ago = QString("%1 d").arg(secs_ago / 86400);
        }
        if (!ago.isEmpty()) {
            time_ago_label_ = new QLabel(ago, this);
            time_ago_label_->setObjectName("agoLabel");
            time_ago_label_->setFont(DesignTokens::getFont("caption", 10));
            time_ago_label_->setStyleSheet(QString("color: %1;").arg(c.text_muted.name()));
            time_ago_label_->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
            time_ago_label_->setFixedWidth(50);
            layout_->addWidget(time_ago_label_);
        }
    }

    // Delete button
    delete_btn_ = new QPushButton(this);
    delete_btn_->setObjectName("deleteBtn");
    delete_btn_->setFixedSize(28, 28);
    delete_btn_->setCursor(Qt::PointingHandCursor);
    delete_btn_->setFlat(true);
    delete_btn_->setStyleSheet(QString(
        "QPushButton { background: transparent; border: none; border-radius: %4px; }"
        "QPushButton:hover { background: rgba(%1, %2, %3, 0.1); }"
    ).arg(c.text_primary.red()).arg(c.text_primary.green()).arg(c.text_primary.blue()).arg(DesignTokens::radius().sm));
    delete_btn_->setIcon(IconProvider::getIcon("delete", c.text_muted, 18));
    delete_btn_->setToolTip(tr_q("remove_from_history"));
    connect(delete_btn_, &QPushButton::clicked, this, [this]() {
        auto reply = QMessageBox::question(
            this,
            tr_q("remove_from_history"),
            tr_q("confirm_remove_from_history"),
            QMessageBox::Yes | QMessageBox::No,
            QMessageBox::No);
        if (reply == QMessageBox::Yes) {
            emit delete_requested(static_cast<std::string>(track_.id), feedback_token_);
        }
    });
    layout_->addWidget(delete_btn_);
}

void HistoryRow::loadThumbnail(const Track &track) {
    if (track.thumbnail.empty()) {
        const auto &c = DesignTokens::current();
        QPixmap default_art = IconProvider::getIcon("music_note", c.text_secondary, 24).pixmap(48, 48);
        thumb_label_->setPixmap(default_art);
        thumb_label_->setAlignment(Qt::AlignCenter);
        return;
    }

    QPointer<QLabel> label_ptr(thumb_label_);
    ArtworkLoader::load(QString::fromStdString(static_cast<std::string>(track.thumbnail)), QSize(48, 48),
        [label_ptr](const QPixmap &pixmap) {
            if (!label_ptr) return;
            QPixmap dest(pixmap.size());
            dest.fill(Qt::transparent);
            QPainter painter(&dest);
            painter.setRenderHint(QPainter::Antialiasing);
            QPainterPath path;
            path.addRoundedRect(pixmap.rect(), 6, 6);
            painter.setClipPath(path);
            painter.drawPixmap(0, 0, pixmap);
            label_ptr->setPixmap(dest.scaled(48, 48, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation));
        });
}

void HistoryRow::contextMenuEvent(QContextMenuEvent *event) {
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
    QAction *remove = menu.addAction(tr_q("remove_from_history"));

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
            tr_q("remove_from_history"),
            tr_q("confirm_remove_from_history"),
            QMessageBox::Yes | QMessageBox::No,
            QMessageBox::No);
        if (reply == QMessageBox::Yes) {
            emit delete_requested(static_cast<std::string>(track_.id), feedback_token_);
        }
    }
}

void HistoryRow::update_theme() {
    const auto &c = DesignTokens::current();

    if (title_label_) {
        title_label_->setStyleSheet(QString("color: %1; font-weight: 600;").arg(c.text_primary.name()));
    }
    if (subtitle_label_) {
        subtitle_label_->setStyleSheet(QString("color: %1;").arg(c.text_secondary.name()));
    }
    if (duration_label_) {
        duration_label_->setStyleSheet(QString("color: %1;").arg(c.text_muted.name()));
    }
    if (time_ago_label_) {
        time_ago_label_->setStyleSheet(QString("color: %1;").arg(c.text_muted.name()));
    }
    if (thumb_label_) {
        thumb_label_->setStyleSheet(QString("background-color: %1; border-radius: %2px;").arg(c.bg_elevated.name()).arg(DesignTokens::radius().sm));
    }
    if (delete_btn_) {
        delete_btn_->setStyleSheet(QString(
            "QPushButton { background: transparent; border: none; border-radius: %4px; }"
            "QPushButton:hover { background: rgba(%1, %2, %3, 0.1); }"
        ).arg(c.text_primary.red()).arg(c.text_primary.green()).arg(c.text_primary.blue()).arg(DesignTokens::radius().sm));
        delete_btn_->setIcon(IconProvider::getIcon("delete", c.text_muted, 18));
    }
}
