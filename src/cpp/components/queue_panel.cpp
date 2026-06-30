#include "queue_panel.h"
#include "../design_tokens.h"
#include "../icon_provider.h"
#include "artwork_loader.h"
#include <QHBoxLayout>
#include <QApplication>
#include <QDrag>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMimeData>
#include <QMouseEvent>
#include <QMenu>
#include <QContextMenuEvent>
#include <QPainter>
#include <QPainterPath>
#include <QScrollBar>
#include <QPointer>

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

QueueRow::QueueRow(int index, const Track &track,
                   bool is_current, QWidget *parent)
    : QWidget(parent), index_(index), is_current_(is_current)
{
    const auto &c = DesignTokens::current();
    setFixedHeight(52);
    setCursor(Qt::PointingHandCursor);

    auto *row_layout = new QHBoxLayout(this);
    row_layout->setContentsMargins(12, 6, 12, 6);
    row_layout->setSpacing(12);

    auto *thumb_label = new QLabel(this);
    thumb_label->setFixedSize(38, 38);
    thumb_label->setStyleSheet(QString("background-color: %1; border-radius: %2px;")
        .arg(c.bg_elevated.name()).arg(DesignTokens::radius().sm));

    if (!track.thumbnail.empty()) {
        QPointer<QLabel> label_ptr(thumb_label);
        ArtworkLoader::load(QString::fromStdString(static_cast<std::string>(track.thumbnail)), QSize(38, 38), [label_ptr](const QPixmap &pixmap) {
            if (label_ptr) {
                label_ptr->setPixmap(getRoundedPixmap(pixmap, 4));
            }
        });
    } else {
        QPixmap default_art = IconProvider::getIcon("music_note", c.text_secondary, 18).pixmap(38, 38);
        thumb_label->setPixmap(getRoundedPixmap(default_art, 4));
    }

    auto *text_container = new QWidget(this);
    auto *text_layout = new QVBoxLayout(text_container);
    text_layout->setContentsMargins(0, 0, 0, 0);
    text_layout->setSpacing(2);

    auto *title_lbl = new QLabel(QString::fromStdString(static_cast<std::string>(track.title)), this);
    title_lbl->setFont(DesignTokens::getFont("body_sm"));

    auto *artist_lbl = new QLabel(QString::fromStdString(static_cast<std::string>(track.artist)), this);
    artist_lbl->setFont(DesignTokens::getFont("caption_sm"));

    if (is_current_) {
        title_lbl->setStyleSheet(QString("color: %1; font-weight: bold;").arg(c.accent.name()));
        artist_lbl->setStyleSheet(QString("color: %1;").arg(c.accent_bright.name()));
    } else {
        title_lbl->setStyleSheet(QString("color: %1;").arg(c.text_primary.name()));
        artist_lbl->setStyleSheet(QString("color: %1;").arg(c.text_secondary.name()));
    }

    text_layout->addWidget(title_lbl);
    text_layout->addWidget(artist_lbl);
    text_container->setLayout(text_layout);

    row_layout->addWidget(thumb_label);
    row_layout->addWidget(text_container, 1);

    if (is_current_) {
        auto *playing_icon = IconProvider::createIconLabel("volume_up", 16, c.accent, true, this);
        row_layout->addWidget(playing_icon);
    }

    auto *move_up = new QPushButton(this);
    move_up->setIcon(IconProvider::getIcon("keyboard_arrow_up", c.text_secondary, 16));
    move_up->setToolTip("Mover arriba");
    move_up->setFixedSize(26, 26);
    move_up->setFlat(true);
    connect(move_up, &QPushButton::clicked, this, [this]() {
        if (index_ > 0) emit move_requested(index_, index_ - 1);
    });
    row_layout->addWidget(move_up);

    auto *move_down = new QPushButton(this);
    move_down->setIcon(IconProvider::getIcon("keyboard_arrow_down", c.text_secondary, 16));
    move_down->setToolTip("Mover abajo");
    move_down->setFixedSize(26, 26);
    move_down->setFlat(true);
    connect(move_down, &QPushButton::clicked, this, [this]() {
        emit move_requested(index_, index_ + 1);
    });
    row_layout->addWidget(move_down);

    auto *remove = new QPushButton(this);
    remove->setIcon(IconProvider::getIcon("close", c.text_secondary, 16));
    remove->setToolTip("Quitar de la cola");
    remove->setFixedSize(26, 26);
    remove->setFlat(true);
    connect(remove, &QPushButton::clicked, this, [this]() { emit remove_requested(index_); });
    row_layout->addWidget(remove);

    setLayout(row_layout);

    QString bg_color = is_current_ ?
        QString("rgba(%1, %2, %3, 0.12)").arg(c.accent.red()).arg(c.accent.green()).arg(c.accent.blue()) :
        "transparent";

    setStyleSheet(QString("QWidget#QueueRow { background-color: %1; border-radius: %2px; }")
        .arg(bg_color).arg(DesignTokens::radius().sm));

    setObjectName("QueueRow");
}

void QueueRow::mousePressEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton) {
        drag_start_position_ = event->position().toPoint();
        dragging_ = false;
    }
    QWidget::mousePressEvent(event);
}

void QueueRow::mouseMoveEvent(QMouseEvent *event) {
    if (!(event->buttons() & Qt::LeftButton) ||
        (event->position().toPoint() - drag_start_position_).manhattanLength() < QApplication::startDragDistance()) {
        QWidget::mouseMoveEvent(event);
        return;
    }

    dragging_ = true;
    auto *mime = new QMimeData();
    mime->setData("application/x-doremi-queue-index", QByteArray::number(index_));
    auto *drag = new QDrag(this);
    drag->setMimeData(mime);
    drag->setPixmap(grab());
    drag->setHotSpot(event->position().toPoint());
    drag->exec(Qt::MoveAction);
}

void QueueRow::mouseReleaseEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton && !dragging_) {
        emit clicked(index_);
    }
    dragging_ = false;
    QWidget::mouseReleaseEvent(event);
}

void QueueRow::enterEvent(QEnterEvent *event) {
    QWidget::enterEvent(event);
    is_hovered_ = true;
    const auto &c = DesignTokens::current();

    QString bg_color;
    if (is_current_) {
        bg_color = QString("rgba(%1, %2, %3, 0.20)").arg(c.accent.red()).arg(c.accent.green()).arg(c.accent.blue());
    } else {
        bg_color = QString("rgba(%1, %2, %3, 0.08)").arg(c.text_primary.red()).arg(c.text_primary.green()).arg(c.text_primary.blue());
    }

    setStyleSheet(QString("QWidget#QueueRow { background-color: %1; border-radius: %2px; }").arg(bg_color).arg(DesignTokens::radius().sm));
}

void QueueRow::leaveEvent(QEvent *event) {
    QWidget::leaveEvent(event);
    is_hovered_ = false;
    const auto &c = DesignTokens::current();

    QString bg_color = is_current_ ?
        QString("rgba(%1, %2, %3, 0.12)").arg(c.accent.red()).arg(c.accent.green()).arg(c.accent.blue()) :
        "transparent";

    setStyleSheet(QString("QWidget#QueueRow { background-color: %1; border-radius: %2px; }").arg(bg_color).arg(DesignTokens::radius().sm));
}

void QueueRow::contextMenuEvent(QContextMenuEvent *event) {
    QMenu menu;
    QAction *play_now = menu.addAction("Reproducir ahora");
    menu.addSeparator();
    QAction *move_top = menu.addAction("Mover al inicio");
    QAction *remove = menu.addAction("Quitar de la cola");

    QAction *chosen = menu.exec(event->globalPos());
    if (chosen == play_now) {
        emit clicked(index_);
    } else if (chosen == move_top) {
        emit move_requested(index_, 0);
    } else if (chosen == remove) {
        emit remove_requested(index_);
    }
}

QueuePanel::QueuePanel(QWidget *parent)
    : QScrollArea(parent)
{
    setWidgetResizable(true);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    setFrameShape(QFrame::NoFrame);
    // style handled by base.qss global QScrollArea selector
    viewport()->setAcceptDrops(true);
    viewport()->installEventFilter(this);

    container_ = new QWidget(this);
    container_->setStyleSheet("background: transparent;");

    layout_ = new QVBoxLayout(container_);
    layout_->setContentsMargins(16, 16, 16, 16);
    layout_->setSpacing(6);
    layout_->setAlignment(Qt::AlignTop);

    drop_indicator_ = new QFrame(container_);
    drop_indicator_->setFixedHeight(2);
    drop_indicator_->setStyleSheet(QString("background-color: %1; border-radius: 1px;")
        .arg(DesignTokens::current().accent.name()));
    drop_indicator_->hide();

    setWidget(container_);
}

void QueuePanel::clearLayout() {
    QLayoutItem *item;
    while ((item = layout_->takeAt(0)) != nullptr) {
        if (item->widget()) {
            item->widget()->deleteLater();
        }
        delete item;
    }
}

void QueuePanel::setQueue(const std::vector<Track> &tracks, int current_index) {
    current_index_ = current_index;
    queue_size_ = static_cast<int>(tracks.size());
    hideDropIndicator();
    clearLayout();

    const auto &c = DesignTokens::current();

    if (tracks.empty()) {
        auto *empty_lbl = new QLabel("Cola de reproducción vacía", container_);
        empty_lbl->setAlignment(Qt::AlignCenter);
        empty_lbl->setFont(DesignTokens::getFont("heading_sm", 14));
        empty_lbl->setStyleSheet(QString("color: %1; margin-top: 40px;").arg(c.text_muted.name()));
        layout_->addWidget(empty_lbl);
        return;
    }

    auto *header_widget = new QWidget(container_);
    auto *header_layout = new QHBoxLayout(header_widget);
    header_layout->setContentsMargins(0, 0, 0, 4);
    auto *header = new QLabel(QString("Cola de reproducción (%1 canciones)").arg(tracks.size()), header_widget);
    header->setFont(DesignTokens::getFont("heading_sm", 13));
    header->setStyleSheet(QString("color: %1; font-weight: bold; margin-bottom: 8px;").arg(c.text_secondary.name()));
    auto *clear = new QPushButton("Vaciar", header_widget);
    clear->setToolTip("Vaciar cola");
    clear->setFlat(true);
    clear->setStyleSheet(QString("color: %1;").arg(c.text_secondary.name()));
    connect(clear, &QPushButton::clicked, this, &QueuePanel::clear_requested);
    header_layout->addWidget(header, 1);
    header_layout->addWidget(clear);
    layout_->addWidget(header_widget);

    for (int i = 0; i < static_cast<int>(tracks.size()); ++i) {
        bool is_curr = (i == current_index_);
        auto *row = new QueueRow(i, tracks[i], is_curr, container_);

        connect(row, &QueueRow::clicked, this, &QueuePanel::item_clicked);
        connect(row, &QueueRow::remove_requested, this, &QueuePanel::item_removed);
        connect(row, &QueueRow::move_requested, this, [this, count = static_cast<int>(tracks.size())](int from, int to) {
            if (to >= 0 && to < count) emit item_moved(from, to);
        });

        layout_->addWidget(row);
    }

    layout_->addStretch(1);
}

bool QueuePanel::eventFilter(QObject *watched, QEvent *event) {
    if (watched != viewport()) return QScrollArea::eventFilter(watched, event);

    constexpr auto mime_type = "application/x-doremi-queue-index";
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
                const QPoint container_pos = container_->mapFrom(viewport(), QPoint(0, indicator_y));
                drop_indicator_->setGeometry(
                    layout_->contentsMargins().left(),
                    container_pos.y() - 1,
                    qMax(0, container_->width() - layout_->contentsMargins().left() - layout_->contentsMargins().right()),
                    2);
                drop_indicator_->raise();
                drop_indicator_->show();
                drag_event->acceptProposedAction();
                return true;
            }
        }
    } else if (event->type() == QEvent::Drop) {
        auto *drop_event = static_cast<QDropEvent *>(event);
        if (drop_event->mimeData()->hasFormat(mime_type)) {
            bool ok = false;
            const int source = drop_event->mimeData()->data(mime_type).toInt(&ok);
            const int target = ok ? dropIndexAt(drop_event->position().toPoint(), source, nullptr) : -1;
            hideDropIndicator();
            if (target >= 0 && target != source) {
                emit item_moved(source, target);
            }
            drop_event->acceptProposedAction();
            return true;
        }
    } else if (event->type() == QEvent::DragLeave) {
        hideDropIndicator();
    }

    return QScrollArea::eventFilter(watched, event);
}

int QueuePanel::dropIndexAt(const QPoint &viewport_position, int source_index, int *indicator_y) const {
    if (source_index < 0 || source_index >= queue_size_ || queue_size_ < 2) return -1;

    const QPoint container_position = container_->mapFrom(viewport(), viewport_position);
    int insertion_index = queue_size_;
    int line_y = container_->height() - layout_->contentsMargins().bottom();

    const auto rows = container_->findChildren<QueueRow *>(QString(), Qt::FindDirectChildrenOnly);
    for (auto *row : rows) {
        const int midpoint = row->geometry().center().y();
        if (container_position.y() < midpoint) {
            insertion_index = row->index();
            line_y = row->geometry().top();
            break;
        }
        line_y = row->geometry().bottom() + 1;
    }

    if (indicator_y) {
        *indicator_y = container_->mapTo(viewport(), QPoint(0, line_y)).y();
    }
    if (insertion_index > source_index) --insertion_index;
    return qBound(0, insertion_index, queue_size_ - 1);
}

void QueuePanel::hideDropIndicator() {
    if (drop_indicator_) drop_indicator_->hide();
}
