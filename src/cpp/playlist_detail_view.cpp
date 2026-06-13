#include "playlist_detail_view.h"
#include "design_tokens.h"
#include "icon_provider.h"
#include <QScrollBar>
#include <QPainter>
#include <QPainterPath>
#include <QPushButton>
#include "doremi/src/bridge.rs.h"

// ─────────────────────────────────────────────────────────────────────────────
// PlaylistTrackRow
// ─────────────────────────────────────────────────────────────────────────────

PlaylistTrackRow::PlaylistTrackRow(int num, const QString &title, const QString &artist,
                                   const QString &duration, const std::string &item_id,
                                   QWidget *parent)
    : QWidget(parent), item_id_(item_id)
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

    // Title + Artist
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

    setObjectName("PlaylistTrackRow");
    setStyleSheet("QWidget#PlaylistTrackRow { background-color: transparent; border-radius: 6px; }");
}

void PlaylistTrackRow::mousePressEvent(QMouseEvent *event) {
    QWidget::mousePressEvent(event);
    if (event->button() == Qt::LeftButton) emit play_requested(item_id_);
}

void PlaylistTrackRow::enterEvent(QEnterEvent *event) {
    QWidget::enterEvent(event);
    const auto &c = DesignTokens::current();
    setStyleSheet(QString("QWidget#PlaylistTrackRow { background-color: rgba(%1, %2, %3, 0.06); border-radius: 6px; }")
        .arg(c.text_primary.red()).arg(c.text_primary.green()).arg(c.text_primary.blue()));
}

void PlaylistTrackRow::leaveEvent(QEvent *event) {
    QWidget::leaveEvent(event);
    setStyleSheet("QWidget#PlaylistTrackRow { background-color: transparent; border-radius: 6px; }");
}

// ─────────────────────────────────────────────────────────────────────────────
// PlaylistDetailView
// ─────────────────────────────────────────────────────────────────────────────

PlaylistDetailView::PlaylistDetailView(QWidget *parent)
    : QWidget(parent)
{
    setupLayout();
}

void PlaylistDetailView::setupLayout() {
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
    connect(back_btn, &QPushButton::clicked, this, &PlaylistDetailView::back_requested);
    content_layout_->addWidget(back_btn, 0, Qt::AlignLeft);

    // Header: cover + info
    auto *header = new QHBoxLayout();
    header->setSpacing(24);

    cover_label_ = new QLabel(scroll_content_);
    cover_label_->setFixedSize(160, 160);
    cover_label_->setStyleSheet(QString("background-color: %1; border-radius: 12px;").arg(c.bg_elevated.name()));
    cover_label_->setAlignment(Qt::AlignCenter);
    header->addWidget(cover_label_);

    auto *info = new QVBoxLayout();
    info->setSpacing(6);

    auto *type_lbl = new QLabel("PLAYLIST", scroll_content_);
    type_lbl->setFont(DesignTokens::getFont("caption", 10));
    type_lbl->setStyleSheet(QString("color: %1; font-weight: bold; letter-spacing: 2px;").arg(c.text_muted.name()));
    info->addWidget(type_lbl);

    title_label_ = new QLabel("Playlist", scroll_content_);
    title_label_->setFont(DesignTokens::getFont("display", 24));
    title_label_->setStyleSheet(QString("color: %1; font-weight: bold;").arg(c.text_primary.name()));
    title_label_->setWordWrap(true);
    info->addWidget(title_label_);

    desc_label_ = new QLabel("", scroll_content_);
    desc_label_->setFont(DesignTokens::getFont("caption", 11));
    desc_label_->setStyleSheet(QString("color: %1;").arg(c.text_secondary.name()));
    desc_label_->setWordWrap(true);
    desc_label_->setMaximumWidth(400);
    desc_label_->hide();
    info->addWidget(desc_label_);

    meta_label_ = new QLabel("", scroll_content_);
    meta_label_->setFont(DesignTokens::getFont("caption", 12));
    meta_label_->setStyleSheet(QString("color: %1;").arg(c.text_muted.name()));
    info->addWidget(meta_label_);

    // Action buttons
    auto *actions = new QHBoxLayout();
    actions->setSpacing(10);

    // Play all
    auto *play_btn = new QPushButton(scroll_content_);
    auto *play_l = new QHBoxLayout(play_btn);
    play_l->setContentsMargins(16, 8, 20, 8);
    play_l->setSpacing(8);
    play_l->addWidget(IconProvider::createIconLabel("play_arrow", 20, QColor("#FFFFFF"), false, play_btn));
    auto *play_t = new QLabel("Reproducir", play_btn);
    play_t->setFont(DesignTokens::getFont("body", 13));
    play_t->setStyleSheet("color: #FFFFFF; background: transparent; font-weight: bold;");
    play_l->addWidget(play_t);
    play_btn->setLayout(play_l);
    play_btn->setFixedHeight(40);
    play_btn->setCursor(Qt::PointingHandCursor);
    play_btn->setStyleSheet(QString(
        "QPushButton { background: %1; border: none; border-radius: 20px; }"
        "QPushButton:hover { background: %2; }")
        .arg(c.accent.name()).arg(c.accent.lighter(115).name()));
    connect(play_btn, &QPushButton::clicked, this, &PlaylistDetailView::play_all_requested);
    actions->addWidget(play_btn);

    // Shuffle
    auto *shuffle_btn = new QPushButton(scroll_content_);
    auto *shuffle_l = new QHBoxLayout(shuffle_btn);
    shuffle_l->setContentsMargins(16, 8, 20, 8);
    shuffle_l->setSpacing(8);
    shuffle_l->addWidget(IconProvider::createIconLabel("shuffle", 18, c.text_primary, false, shuffle_btn));
    auto *shuffle_t = new QLabel("Aleatorio", shuffle_btn);
    shuffle_t->setFont(DesignTokens::getFont("body", 13));
    shuffle_t->setStyleSheet(QString("color: %1; background: transparent; font-weight: 600;").arg(c.text_primary.name()));
    shuffle_l->addWidget(shuffle_t);
    shuffle_btn->setLayout(shuffle_l);
    shuffle_btn->setFixedHeight(40);
    shuffle_btn->setCursor(Qt::PointingHandCursor);
    shuffle_btn->setStyleSheet(QString(
        "QPushButton { background: transparent; border: 1px solid %1; border-radius: 20px; }"
        "QPushButton:hover { background: rgba(%2, %3, %4, 0.08); }")
        .arg(c.border.name())
        .arg(c.text_primary.red()).arg(c.text_primary.green()).arg(c.text_primary.blue()));
    connect(shuffle_btn, &QPushButton::clicked, this, &PlaylistDetailView::shuffle_requested);
    actions->addWidget(shuffle_btn);

    actions->addStretch();

    info->addSpacing(8);
    info->addLayout(actions);
    info->addStretch();

    header->addLayout(info, 1);
    content_layout_->addLayout(header);

    // Separator
    auto *sep = new QWidget(scroll_content_);
    sep->setFixedHeight(1);
    sep->setStyleSheet(QString("background-color: %1;").arg(c.border.name()));
    content_layout_->addSpacing(8);
    content_layout_->addWidget(sep);
    content_layout_->addSpacing(4);

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

void PlaylistDetailView::set_playlist_info(const std::string &name, const std::string &description,
                                           const std::string &thumbnail, int32_t track_count)
{
    const auto &c = DesignTokens::current();
    title_label_->setText(QString::fromStdString(name));

    if (!description.empty()) {
        desc_label_->setText(QString::fromStdString(description));
        desc_label_->show();
    } else {
        desc_label_->hide();
    }

    meta_label_->setText(track_count > 0 ? QString("%1 canciones").arg(track_count) : "");

    QPixmap pm;
    if (!thumbnail.empty() && pm.load(QString::fromStdString(thumbnail))) {
        QPixmap dest(pm.size());
        dest.fill(Qt::transparent);
        QPainter painter(&dest);
        painter.setRenderHint(QPainter::Antialiasing);
        QPainterPath path;
        path.addRoundedRect(pm.rect(), 12, 12);
        painter.setClipPath(path);
        painter.drawPixmap(0, 0, pm);
        cover_label_->setPixmap(dest.scaled(160, 160, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation));
    } else {
        auto icon = IconProvider::getIcon("queue_music", c.text_secondary, 60);
        cover_label_->setPixmap(icon.pixmap(60, 60));
    }
}

void PlaylistDetailView::set_playlist_tracks(const std::vector<std::string> &titles,
                                             const std::vector<std::string> &artists,
                                             const std::vector<std::string> &durations,
                                             const std::vector<std::string> &item_ids)
{
    QLayoutItem *item;
    while ((item = tracks_layout_->takeAt(0)) != nullptr) {
        if (item->widget()) item->widget()->deleteLater();
        delete item;
    }

    size_t n = std::min({titles.size(), artists.size(), durations.size(), item_ids.size()});
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

        auto *row = new PlaylistTrackRow(
            static_cast<int>(i) + 1,
            QString::fromStdString(titles[i]),
            QString::fromStdString(artists[i]),
            dur,
            item_ids[i],
            tracks_widget_
        );
        connect(row, &PlaylistTrackRow::play_requested, this, &PlaylistDetailView::play_requested);
        tracks_layout_->addWidget(row);
    }

    if (n == 0) {
        const auto &c = DesignTokens::current();
        auto *empty = new QLabel("Esta playlist está vacía.", tracks_widget_);
        empty->setAlignment(Qt::AlignCenter);
        empty->setFont(DesignTokens::getFont("caption", 12));
        empty->setStyleSheet(QString("color: %1; padding: 30px;").arg(c.text_muted.name()));
        tracks_layout_->addWidget(empty);
    }
}

void PlaylistDetailView::clear() {
    title_label_->setText("Playlist");
    desc_label_->hide();
    meta_label_->setText("");
    cover_label_->clear();
    QLayoutItem *item;
    while ((item = tracks_layout_->takeAt(0)) != nullptr) {
        if (item->widget()) item->widget()->deleteLater();
        delete item;
    }
}
