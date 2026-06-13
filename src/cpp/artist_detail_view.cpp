#include "artist_detail_view.h"
#include "design_tokens.h"
#include "icon_provider.h"
#include <QScrollBar>
#include <QPainter>
#include <QPainterPath>
#include <QPushButton>
#include "doremi/src/bridge.rs.h"

// ─────────────────────────────────────────────────────────────────────────────
// ArtistTrackRow
// ─────────────────────────────────────────────────────────────────────────────

ArtistTrackRow::ArtistTrackRow(const QString &title, const QString &album,
                               const QString &duration, const std::string &item_id,
                               QWidget *parent)
    : QWidget(parent), item_id_(item_id)
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
    if (event->button() == Qt::LeftButton) emit play_requested(item_id_);
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

    scroll_area_->setWidget(scroll_content_);
    main_vbox->addWidget(scroll_area_);
    setLayout(main_vbox);
}

void ArtistDetailView::set_artist_info(const std::string &name, const std::string &thumbnail,
                                       const std::string &subscriber_count, const std::string &description)
{
    const auto &c = DesignTokens::current();
    name_label_->setText(QString::fromStdString(name));

    if (!subscriber_count.empty()) {
        meta_label_->setText(QString::fromStdString(subscriber_count) + " suscriptores");
    }

    if (!description.empty()) {
        desc_label_->setText(QString::fromStdString(description));
        desc_label_->show();
    } else {
        desc_label_->hide();
    }

    // Load avatar (circular crop)
    QPixmap pm;
    if (!thumbnail.empty() && pm.load(QString::fromStdString(thumbnail))) {
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

void ArtistDetailView::set_artist_tracks(const std::vector<std::string> &titles,
                                         const std::vector<std::string> &albums,
                                         const std::vector<std::string> &durations,
                                         const std::vector<std::string> &item_ids)
{
    QLayoutItem *item;
    while ((item = tracks_layout_->takeAt(0)) != nullptr) {
        if (item->widget()) item->widget()->deleteLater();
        delete item;
    }

    size_t n = std::min({titles.size(), albums.size(), durations.size(), item_ids.size()});
    for (size_t i = 0; i < n; ++i) {
        QString dur;
        int dur_ms = 0;
        try { dur_ms = std::stoi(durations[i]); } catch (...) {}
        if (dur_ms > 0) {
            int secs = dur_ms / 1000;
            dur = QString("%1:%2").arg(secs / 60).arg(secs % 60, 2, 10, QChar('0'));
        } else if (!durations[i].empty()) {
            dur = QString::fromStdString(durations[i]);
        }

        auto *row = new ArtistTrackRow(
            QString::fromStdString(titles[i]),
            QString::fromStdString(albums[i]),
            dur,
            item_ids[i],
            tracks_widget_
        );
        connect(row, &ArtistTrackRow::play_requested, this, &ArtistDetailView::play_requested);
        tracks_layout_->addWidget(row);
    }

    if (n == 0) {
        const auto &c = DesignTokens::current();
        auto *empty = new QLabel("No se encontraron canciones de este artista.", tracks_widget_);
        empty->setAlignment(Qt::AlignCenter);
        empty->setFont(DesignTokens::getFont("caption", 12));
        empty->setStyleSheet(QString("color: %1; padding: 30px;").arg(c.text_muted.name()));
        tracks_layout_->addWidget(empty);
    }
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
}
