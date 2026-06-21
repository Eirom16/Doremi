#include "history_view.h"
#include "design_tokens.h"
#include "icon_provider.h"
#include <QHBoxLayout>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QDateTime>
#include <QTimeZone>
#include <QPushButton>
#include <QMessageBox>
#include <QMenu>
#include <QContextMenuEvent>
#include "doremi/src/bridge.rs.h"

HistoryRow::HistoryRow(const Track &track,
                       const std::string &played_at,
                       const std::string &feedback_token,
                       QWidget *parent)
    : QWidget(parent), track_(track), title_(QString::fromStdString(static_cast<std::string>(track.title))), artist_(QString::fromStdString(static_cast<std::string>(track.artist))), feedback_token_(feedback_token)
{
    const auto &c = DesignTokens::current();
    setFixedHeight(64);
    setCursor(Qt::PointingHandCursor);

    auto *layout = new QHBoxLayout(this);
    layout->setContentsMargins(16, 8, 16, 8);
    layout->setSpacing(14);

    auto *thumb_lbl = new QLabel(this);
    thumb_lbl->setFixedSize(48, 48);
    thumb_lbl->setStyleSheet(QString("background-color: %1; border-radius: 6px;")
        .arg(c.bg_elevated.name()));

    QPixmap pm;
    if (!track.thumbnail.empty() && pm.load(QString::fromStdString(static_cast<std::string>(track.thumbnail)))) {
        QPixmap dest(pm.size());
        dest.fill(Qt::transparent);
        QPainter painter(&dest);
        painter.setRenderHint(QPainter::Antialiasing);
        QPainterPath path;
        path.addRoundedRect(pm.rect(), 6, 6);
        painter.setClipPath(path);
        painter.drawPixmap(0, 0, pm);
        thumb_lbl->setPixmap(dest.scaled(48, 48, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation));
    } else {
        QPixmap default_art = IconProvider::getIcon("music_note", c.text_secondary, 24).pixmap(48, 48);
        thumb_lbl->setPixmap(default_art);
        thumb_lbl->setAlignment(Qt::AlignCenter);
    }
    layout->addWidget(thumb_lbl);

    auto *text_container = new QWidget(this);
    auto *text_layout = new QVBoxLayout(text_container);
    text_layout->setContentsMargins(0, 0, 0, 0);
    text_layout->setSpacing(2);

    auto *title_lbl = new QLabel(title_, this);
    title_lbl->setFont(DesignTokens::getFont("body", 13));
    title_lbl->setStyleSheet(QString("color: %1; font-weight: 600;").arg(c.text_primary.name()));
    title_lbl->setMaximumWidth(400);

    auto *artist_lbl = new QLabel(artist_, this);
    artist_lbl->setFont(DesignTokens::getFont("caption", 11));
    artist_lbl->setStyleSheet(QString("color: %1;").arg(c.text_secondary.name()));
    artist_lbl->setMaximumWidth(400);

    text_layout->addWidget(title_lbl);
    text_layout->addWidget(artist_lbl);
    layout->addWidget(text_container, 2);

    // Duration
    if (track.duration_ms > 0) {
        int secs = static_cast<int>(track.duration_ms / 1000);
        QString dur = QString("%1:%2").arg(secs / 60).arg(secs % 60, 2, 10, QChar('0'));
        auto *dur_lbl = new QLabel(dur, this);
        dur_lbl->setFont(DesignTokens::getFont("caption", 11));
        dur_lbl->setStyleSheet(QString("color: %1;").arg(c.text_muted.name()));
        dur_lbl->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        layout->addWidget(dur_lbl);
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
            if (secs_ago < 60) ago = "Ahora";
            else if (secs_ago < 3600) ago = QString("%1 min").arg(secs_ago / 60);
            else if (secs_ago < 86400) ago = QString("%1 h").arg(secs_ago / 3600);
            else ago = QString("%1 d").arg(secs_ago / 86400);
        }
        if (!ago.isEmpty()) {
            auto *ago_lbl = new QLabel(ago, this);
            ago_lbl->setFont(DesignTokens::getFont("caption", 10));
            ago_lbl->setStyleSheet(QString("color: %1;").arg(c.text_muted.name()));
            ago_lbl->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
            ago_lbl->setFixedWidth(50);
            layout->addWidget(ago_lbl);
        }
    }

    auto *delete_btn = new QPushButton(this);
    delete_btn->setFixedSize(28, 28);
    delete_btn->setCursor(Qt::PointingHandCursor);
    delete_btn->setFlat(true);
    delete_btn->setStyleSheet(QString(
        "QPushButton { background: transparent; border: none; border-radius: 4px; }"
        "QPushButton:hover { background: rgba(%1, %2, %3, 0.1); }"
    ).arg(c.text_primary.red()).arg(c.text_primary.green()).arg(c.text_primary.blue()));
    delete_btn->setIcon(IconProvider::getIcon("delete", c.text_muted, 18));
    delete_btn->setToolTip("Eliminar del historial");
    connect(delete_btn, &QPushButton::clicked, this, [this]() {
        auto reply = QMessageBox::question(
            this,
            "Eliminar del historial",
            "¿Quieres eliminar esta reproducción del historial?",
            QMessageBox::Yes | QMessageBox::No,
            QMessageBox::No);
        if (reply == QMessageBox::Yes) {
            emit delete_requested(static_cast<std::string>(track_.id), feedback_token_);
        }
    });
    layout->addWidget(delete_btn);

    setLayout(layout);
    setObjectName("HistoryRow");
    setStyleSheet("QWidget#HistoryRow { background-color: transparent; border-radius: 8px; }");
}

void HistoryRow::mousePressEvent(QMouseEvent *event) {
    QWidget::mousePressEvent(event);
    if (event->button() == Qt::LeftButton) {
        emit play_requested(track_);
    }
}

void HistoryRow::contextMenuEvent(QContextMenuEvent *event) {
    QMenu menu;
    QAction *play = menu.addAction("Reproducir");
    
    bool is_fav = get_track_favorite_state(static_cast<std::string>(track_.id));
    QAction *fav = menu.addAction(is_fav ? "Quitar de favoritos" : "Agregar a favoritos");
    
    QAction *dl = menu.addAction("Descargar");
    menu.addSeparator();
    QAction *next = menu.addAction("Reproducir siguiente");
    QAction *end = menu.addAction("Agregar a la cola");
    menu.addSeparator();
    QAction *remove = menu.addAction("Eliminar del historial");

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
            "Eliminar del historial",
            "¿Quieres eliminar esta reproducción del historial?",
            QMessageBox::Yes | QMessageBox::No,
            QMessageBox::No);
        if (reply == QMessageBox::Yes) {
            emit delete_requested(static_cast<std::string>(track_.id), feedback_token_);
        }
    }
}

void HistoryRow::enterEvent(QEnterEvent *event) {
    QWidget::enterEvent(event);
    const auto &c = DesignTokens::current();
    setStyleSheet(QString("QWidget#HistoryRow { background-color: rgba(%1, %2, %3, 0.06); border-radius: 8px; }")
        .arg(c.text_primary.red()).arg(c.text_primary.green()).arg(c.text_primary.blue()));
}

void HistoryRow::leaveEvent(QEvent *event) {
    QWidget::leaveEvent(event);
    setStyleSheet("QWidget#HistoryRow { background-color: transparent; border-radius: 8px; }");
}

HistoryView::HistoryView(QWidget *parent)
    : QWidget(parent)
{
    setupLayout();
}

void HistoryView::setupLayout() {
    const auto &c = DesignTokens::current();

    auto *main_vbox = new QVBoxLayout(this);
    main_vbox->setContentsMargins(0, 0, 0, 0);
    main_vbox->setSpacing(0);

    content_layout_ = new QVBoxLayout();
    content_layout_->setContentsMargins(24, 24, 24, 24);
    content_layout_->setSpacing(4);
    content_layout_->setAlignment(Qt::AlignTop);

    auto *header_row = new QHBoxLayout();
    auto *title = new QLabel("Historial", this);
    title->setFont(DesignTokens::getFont("display", 22));
    title->setStyleSheet(QString("color: %1; font-weight: bold;").arg(c.text_primary.name()));
    header_row->addWidget(title);
    header_row->addStretch();
    auto *clear_btn = new QPushButton("Limpiar historial", this);
    clear_btn->setFixedHeight(30);
    clear_btn->setCursor(Qt::PointingHandCursor);
    clear_btn->setStyleSheet(QString(
        "QPushButton { background: transparent; border: 1px solid %1; border-radius: 15px; padding: 0 16px; color: %2; font-size: 12px; }"
        "QPushButton:hover { background: rgba(%3, %4, %5, 0.08); }")
        .arg(c.border.name()).arg(c.text_secondary.name())
        .arg(c.text_primary.red()).arg(c.text_primary.green()).arg(c.text_primary.blue()));
    connect(clear_btn, &QPushButton::clicked, this, [this]() {
        auto reply = QMessageBox::question(this, "Limpiar historial",
            "¿Estás seguro de que deseas eliminar todo el historial de reproducción?",
            QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
        if (reply == QMessageBox::Yes) {
            on_clear_history();
        }
    });
    header_row->addWidget(clear_btn);
    content_layout_->addLayout(header_row);

    auto *subtitle = new QLabel("Reproducido recientemente", this);
    subtitle->setFont(DesignTokens::getFont("caption", 12));
    subtitle->setStyleSheet(QString("color: %1; margin-bottom: 12px;").arg(c.text_secondary.name()));
    content_layout_->addWidget(subtitle);

    empty_label_ = new QLabel("Tu historial está vacío\n\nLas canciones que reproduzcas aparecerán aquí", this);
    empty_label_->setAlignment(Qt::AlignCenter);
    empty_label_->setFont(DesignTokens::getFont("body", 13));
    empty_label_->setStyleSheet(QString("color: %1; padding: 60px 0;").arg(c.text_muted.name()));
    empty_label_->setWordWrap(true);
    content_layout_->addWidget(empty_label_);

    main_vbox->addLayout(content_layout_);
    setLayout(main_vbox);
}

void HistoryView::showEvent(QShowEvent *event) {
    QWidget::showEvent(event);
    on_history_requested();
}

QString HistoryView::getGroupLabel(const QString &played_at) const {
    QDateTime dt = QDateTime::fromString(played_at, Qt::ISODate);
    if (!dt.isValid()) {
        dt = QDateTime::fromString(played_at, "yyyy-MM-dd HH:mm:ss");
    }
    if (!dt.isValid()) return "Otros";

    QDate today = QDate::currentDate();
    QDate play_date = dt.date();

    if (play_date == today) return "Hoy";
    if (play_date == today.addDays(-1)) return "Ayer";
    if (play_date >= today.addDays(-7)) return "Esta semana";
    return "Anteriores";
}

void HistoryView::clear_history() {
    while (content_layout_->count() > 2) {
        QLayoutItem *item = content_layout_->takeAt(2);
        if (item->widget()) {
            item->widget()->deleteLater();
        }
        delete item;
    }
}

void HistoryView::set_history(const std::vector<Track> &tracks,
                              const std::vector<std::string> &played_at,
                              const std::vector<std::string> &feedback_tokens) {
    clear_history();

    const auto &c = DesignTokens::current();

    if (tracks.empty()) {
        empty_label_ = new QLabel("Tu historial está vacío\n\nLas canciones que reproduzcas aparecerán aquí", this);
        empty_label_->setAlignment(Qt::AlignCenter);
        empty_label_->setFont(DesignTokens::getFont("body", 13));
        empty_label_->setStyleSheet(QString("color: %1; padding: 60px 0;").arg(c.text_muted.name()));
        empty_label_->setWordWrap(true);
        content_layout_->addWidget(empty_label_);
        return;
    }

    size_t n = std::min({tracks.size(), played_at.size(), feedback_tokens.size()});

    QString last_group;
    for (size_t i = 0; i < n; ++i) {
        const auto &t = tracks[i];
        const auto &pa = played_at[i];
        const auto &ft = feedback_tokens[i];

        QString group = getGroupLabel(QString::fromStdString(pa));
        if (group != last_group) {
            last_group = group;
            auto *group_lbl = new QLabel(group, this);
            group_lbl->setFont(DesignTokens::getFont("heading_sm", 13));
            group_lbl->setStyleSheet(QString("color: %1; font-weight: bold; margin-top: 16px; margin-bottom: 4px;")
                .arg(c.accent.name()));
            content_layout_->addWidget(group_lbl);
        }

        auto *row = new HistoryRow(t, pa, ft, this);
        connect(row, &HistoryRow::play_requested, this, &HistoryView::play_requested);
        connect(row, &HistoryRow::delete_requested, this, [](const std::string &track_id, const std::string &feedback_token) {
            on_delete_history_item(track_id, feedback_token);
        });
        content_layout_->addWidget(row);
    }

    content_layout_->addStretch();
}
