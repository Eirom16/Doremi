#include "related_tracks_widget.h"
#include "../design_tokens.h"
#include "../icon_provider.h"
#include <QHBoxLayout>
#include <QMouseEvent>
#include <QMenu>
#include <QPainter>
#include <QPainterPath>

static QPixmap getRoundedPixmap(const QPixmap &src, int radius) {
    if (src.isNull()) return src;
    QPixmap dest(src.size());
    dest.fill(Qt::transparent);
    QPainter painter(&dest);
    painter.setRenderHint(QPainter::Antialiasing);
    QPainterPath path;
    path.addRoundedRect(src.rect(), radius, radius);
    painter.setClipPath(path);
    painter.drawPixmap(0, 0, src);
    return dest;
}

RelatedTrackRow::RelatedTrackRow(const Track &track, QWidget *parent)
    : QWidget(parent), track_(track)
{
    const auto &c = DesignTokens::current();
    setFixedHeight(52);
    setCursor(Qt::PointingHandCursor);

    auto *row_layout = new QHBoxLayout(this);
    row_layout->setContentsMargins(12, 6, 12, 6);
    row_layout->setSpacing(12);

    auto *thumb_label = new QLabel(this);
    thumb_label->setFixedSize(38, 38);
    thumb_label->setStyleSheet(QString("background-color: %1; border-radius: 4px;")
        .arg(c.bg_elevated.name()));

    QPixmap pm;
    if (!track.thumbnail.empty() && pm.load(QString::fromStdString(static_cast<std::string>(track.thumbnail)))) {
        thumb_label->setPixmap(getRoundedPixmap(
            pm.scaled(38, 38, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation), 4
        ));
    } else {
        QPixmap default_art = IconProvider::getIcon("music_note", c.text_secondary, 18).pixmap(38, 38);
        thumb_label->setPixmap(getRoundedPixmap(default_art, 4));
    }

    auto *text_container = new QWidget(this);
    auto *text_layout = new QVBoxLayout(text_container);
    text_layout->setContentsMargins(0, 0, 0, 0);
    text_layout->setSpacing(2);

    auto *title_lbl = new QLabel(QString::fromStdString(static_cast<std::string>(track.title)), this);
    title_lbl->setFont(DesignTokens::getFont("body", 13));
    title_lbl->setStyleSheet(QString("color: %1;").arg(c.text_primary.name()));

    auto *artist_lbl = new QLabel(QString::fromStdString(static_cast<std::string>(track.artist)), this);
    artist_lbl->setFont(DesignTokens::getFont("caption", 11));
    artist_lbl->setStyleSheet(QString("color: %1;").arg(c.text_secondary.name()));

    text_layout->addWidget(title_lbl);
    text_layout->addWidget(artist_lbl);
    text_container->setLayout(text_layout);

    row_layout->addWidget(thumb_label);
    row_layout->addWidget(text_container, 1);

    setLayout(row_layout);
    setStyleSheet("QWidget { background: transparent; border-radius: 6px; }");
}

void RelatedTrackRow::mousePressEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton) {
        pressed_ = true;
    }
    QWidget::mousePressEvent(event);
}

void RelatedTrackRow::mouseReleaseEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton && pressed_) {
        pressed_ = false;
        emit clicked(track_);
    }
    QWidget::mouseReleaseEvent(event);
}

void RelatedTrackRow::contextMenuEvent(QContextMenuEvent *event) {
    QMenu menu;
    QAction *play_now = menu.addAction("Reproducir ahora");
    QAction *add_queue = menu.addAction("Agregar a la cola");

    QAction *chosen = menu.exec(event->globalPos());
    if (chosen == play_now) {
        emit clicked(track_);
    } else if (chosen == add_queue) {
        emit add_to_queue_requested(track_);
    }
}

void RelatedTrackRow::enterEvent(QEnterEvent *event) {
    QWidget::enterEvent(event);
    const auto &c = DesignTokens::current();
    QString bg = QString("rgba(%1, %2, %3, 0.08)")
        .arg(c.text_primary.red()).arg(c.text_primary.green()).arg(c.text_primary.blue());
    setStyleSheet(QString("QWidget { background-color: %1; border-radius: 6px; }").arg(bg));
}

void RelatedTrackRow::leaveEvent(QEvent *event) {
    QWidget::leaveEvent(event);
    setStyleSheet("QWidget { background: transparent; border-radius: 6px; }");
}

RelatedTracksWidget::RelatedTracksWidget(QWidget *parent)
    : QScrollArea(parent)
{
    setWidgetResizable(true);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    setFrameShape(QFrame::NoFrame);
    setStyleSheet("background: transparent;");

    container_ = new QWidget(this);
    container_->setStyleSheet("background: transparent;");

    layout_ = new QVBoxLayout(container_);
    layout_->setContentsMargins(16, 16, 16, 16);
    layout_->setSpacing(6);
    layout_->setAlignment(Qt::AlignTop);

    setWidget(container_);
}

void RelatedTracksWidget::setTracks(const std::vector<Track> &tracks) {
    clearLayout();

    if (tracks.empty()) {
        auto *empty_lbl = new QLabel("No hay canciones relacionadas", container_);
        empty_lbl->setAlignment(Qt::AlignCenter);
        empty_lbl->setStyleSheet(QString("color: %1; padding: 32px;")
            .arg(DesignTokens::current().text_muted.name()));
        layout_->addWidget(empty_lbl);
        return;
    }

    for (const auto &track : tracks) {
        auto *row = new RelatedTrackRow(track, container_);
        connect(row, &RelatedTrackRow::clicked, this, &RelatedTracksWidget::play_requested);
        connect(row, &RelatedTrackRow::add_to_queue_requested, this, &RelatedTracksWidget::add_to_queue_requested);
        layout_->addWidget(row);
    }
}

void RelatedTracksWidget::clearLayout() {
    QLayoutItem *item;
    while ((item = layout_->takeAt(0)) != nullptr) {
        if (item->widget()) {
            item->widget()->deleteLater();
        }
        delete item;
    }
}
