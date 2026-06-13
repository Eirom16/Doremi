#include "queue_panel.h"
#include "../design_tokens.h"
#include "../icon_provider.h"
#include <QHBoxLayout>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QScrollBar>

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

QueueRow::QueueRow(int index, const QString &title, const QString &artist,
                   const QString &thumbnail, bool is_current, QWidget *parent)
    : QWidget(parent), index_(index), is_current_(is_current)
{
    const auto &c = DesignTokens::current();
    setFixedHeight(52);
    setCursor(Qt::PointingHandCursor);

    auto *row_layout = new QHBoxLayout(this);
    row_layout->setContentsMargins(12, 6, 12, 6);
    row_layout->setSpacing(12);

    // 1. Thumbnail
    auto *thumb_label = new QLabel(this);
    thumb_label->setFixedSize(38, 38);
    thumb_label->setStyleSheet(QString("background-color: %1; border-radius: 4px;")
        .arg(c.bg_elevated.name()));

    QPixmap pm;
    if (!thumbnail.isEmpty() && pm.load(thumbnail)) {
        thumb_label->setPixmap(getRoundedPixmap(
            pm.scaled(38, 38, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation), 4
        ));
    } else {
        QPixmap default_art = IconProvider::getIcon("music_note", c.text_secondary, 18).pixmap(38, 38);
        thumb_label->setPixmap(getRoundedPixmap(default_art, 4));
    }

    // 2. Title & Artist
    auto *text_container = new QWidget(this);
    auto *text_layout = new QVBoxLayout(text_container);
    text_layout->setContentsMargins(0, 0, 0, 0);
    text_layout->setSpacing(2);

    auto *title_lbl = new QLabel(title, this);
    title_lbl->setFont(DesignTokens::getFont("body", 13));
    
    auto *artist_lbl = new QLabel(artist, this);
    artist_lbl->setFont(DesignTokens::getFont("caption", 11));

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

    // 3. Indicator if current
    if (is_current_) {
        auto *playing_icon = IconProvider::createIconLabel("volume_up", 16, c.accent, true, this);
        row_layout->addWidget(playing_icon);
    }

    setLayout(row_layout);

    // Initial background styling
    QString bg_color = is_current_ ? 
        QString("rgba(%1, %2, %3, 0.12)").arg(c.accent.red()).arg(c.accent.green()).arg(c.accent.blue()) : 
        "transparent";
    
    setStyleSheet(QString("QWidget#QueueRow { background-color: %1; border-radius: 6px; }")
        .arg(bg_color));
    
    setObjectName("QueueRow");
}

void QueueRow::mousePressEvent(QMouseEvent *event) {
    QWidget::mousePressEvent(event);
    if (event->button() == Qt::LeftButton) {
        emit clicked(index_);
    }
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
    
    setStyleSheet(QString("QWidget#QueueRow { background-color: %1; border-radius: 6px; }").arg(bg_color));
}

void QueueRow::leaveEvent(QEvent *event) {
    QWidget::leaveEvent(event);
    is_hovered_ = false;
    const auto &c = DesignTokens::current();
    
    QString bg_color = is_current_ ? 
        QString("rgba(%1, %2, %3, 0.12)").arg(c.accent.red()).arg(c.accent.green()).arg(c.accent.blue()) : 
        "transparent";
    
    setStyleSheet(QString("QWidget#QueueRow { background-color: %1; border-radius: 6px; }").arg(bg_color));
}

// ─────────────────────────────────────────────────────────────────────────────

QueuePanel::QueuePanel(QWidget *parent)
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

void QueuePanel::clearLayout() {
    QLayoutItem *item;
    while ((item = layout_->takeAt(0)) != nullptr) {
        if (item->widget()) {
            item->widget()->deleteLater();
        }
        delete item;
    }
}

void QueuePanel::setQueue(const QStringList &titles, const QStringList &artists,
                          const QStringList &thumbnails, int current_index)
{
    current_index_ = current_index;
    clearLayout();

    const auto &c = DesignTokens::current();

    if (titles.isEmpty()) {
        auto *empty_lbl = new QLabel("Cola de reproducción vacía", container_);
        empty_lbl->setAlignment(Qt::AlignCenter);
        empty_lbl->setFont(DesignTokens::getFont("heading_sm", 14));
        empty_lbl->setStyleSheet(QString("color: %1; margin-top: 40px;").arg(c.text_muted.name()));
        layout_->addWidget(empty_lbl);
        return;
    }

    // Header label
    auto *header = new QLabel(QString("Cola de reproducción (%1 canciones)").arg(titles.size()), container_);
    header->setFont(DesignTokens::getFont("heading_sm", 13));
    header->setStyleSheet(QString("color: %1; font-weight: bold; margin-bottom: 8px;").arg(c.text_secondary.name()));
    layout_->addWidget(header);

    for (int i = 0; i < titles.size(); ++i) {
        bool is_curr = (i == current_index_);
        auto *row = new QueueRow(i, titles[i], artists[i], thumbnails[i], is_curr, container_);
        
        connect(row, &QueueRow::clicked, this, &QueuePanel::item_clicked);
        
        layout_->addWidget(row);
    }

    // Pushes all elements to top
    layout_->addStretch(1);
}
