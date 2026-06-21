#include <QFrame>
#include <QPixmap>
#include <QPainter>
#include <QPainterPath>
#include <QFile>
#include "doremi/src/bridge.rs.h"
#include "downloads_view.h"
#include "design_tokens.h"
#include "icon_provider.h"
#include <QMenu>
#include <QAction>
#include <QMessageBox>

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

DownloadsView::DownloadsView(QWidget *parent)
    : QWidget(parent)
{
    const auto &c = DesignTokens::current();

    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    list_ = new QVBoxLayout();
    list_->setContentsMargins(24, 24, 24, 24);
    list_->setSpacing(6);

    auto *header = new QLabel("Descargas", this);
    header->setFont(DesignTokens::getFont("display", 24));
    header->setStyleSheet(QString("color: %1; font-weight: bold; background: transparent;").arg(c.text_primary.name()));
    list_->addWidget(header);

    status_label_ = new QLabel("Sin descargas", this);
    status_label_->setFont(DesignTokens::getFont("body", 14));
    status_label_->setStyleSheet(QString("color: %1; padding: 24px 0; background: transparent;").arg(c.text_muted.name()));
    list_->addWidget(status_label_);

    list_->addStretch(1);
    root->addLayout(list_);
    setStyleSheet("background: transparent;");
}

QWidget *DownloadsView::make_download_row(const std::string &video_id, const std::string &title,
                                           const std::string &artist,
                                           const std::string &thumbnail_path,
                                           double progress, const std::string &status) {
    const auto &c = DesignTokens::current();
    bool is_active = (status == "queued" || status == "resolving" || status == "downloading");
    bool is_failed = (status == "failed");
    bool is_completed = (status == "completed");
    bool is_cancelled = (status == "cancelled");

    auto *row = new QWidget(this);
    row->setFixedHeight(is_active || is_failed ? 88 : 64);

    QString rowStyle = QString(
        "QWidget {\n"
        "    background-color: transparent;\n"
        "    border-radius: 8px;\n"
        "}\n"
        "QWidget:hover {\n"
        "    background-color: %1;\n"
        "}\n"
    )
    .arg(QString("rgba(%1, %2, %3, %4)").arg(c.accent_dim.red()).arg(c.accent_dim.green()).arg(c.accent_dim.blue()).arg(c.accent_dim.alpha() / 255.0));
    row->setStyleSheet(rowStyle);

    auto *lay = new QVBoxLayout(row);
    lay->setContentsMargins(12, 6, 12, 6);
    lay->setSpacing(4);

    auto *top = new QHBoxLayout();
    top->setSpacing(12);

    auto *thumb = new QLabel(row);
    thumb->setFixedSize(36, 36);
    thumb->setAlignment(Qt::AlignCenter);
    thumb->setStyleSheet(QString("background: %1; border-radius: 4px;").arg(c.bg_elevated.name()));

    bool thumbLoaded = false;
    if (!thumbnail_path.empty() && QFile::exists(QString::fromStdString(thumbnail_path))) {
        QPixmap px(QString::fromStdString(thumbnail_path));
        if (!px.isNull()) {
            thumb->setPixmap(getRoundedPixmap(
                px.scaled(36, 36, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation), 4
            ));
            thumbLoaded = true;
        }
    }
    if (!thumbLoaded) {
        QPixmap fallback = IconProvider::getIcon("music_note", c.text_secondary, 16).pixmap(36, 36);
        thumb->setPixmap(getRoundedPixmap(fallback, 4));
    }
    top->addWidget(thumb);

    auto *vl = new QVBoxLayout();
    vl->setSpacing(1);
    vl->setContentsMargins(0, 0, 0, 0);

    auto *t = new QLabel(QString::fromStdString(title), row);
    t->setFont(DesignTokens::getFont("body", 13));
    QString titleColor = is_cancelled ? c.text_muted.name() : c.text_primary.name();
    t->setStyleSheet(QString("color: %1; font-weight: bold; background: transparent;").arg(titleColor));
    if (is_cancelled) {
        QFont f = t->font();
        f.setStrikeOut(true);
        t->setFont(f);
    }
    vl->addWidget(t);

    auto *a = new QLabel(QString::fromStdString(artist), row);
    a->setFont(DesignTokens::getFont("caption", 11));
    a->setStyleSheet(QString("color: %1; background: transparent;").arg(c.text_secondary.name()));
    vl->addWidget(a);

    if (is_active) {
        auto *status_text = new QLabel(row);
        status_text->setObjectName("status_label");
        QString statusMsg;
        if (status == "queued") statusMsg = "En cola…";
        else if (status == "resolving") statusMsg = "Resolviendo…";
        else statusMsg = QString("Descargando… %1%").arg(static_cast<int>(progress));
        status_text->setText(statusMsg);
        status_text->setFont(DesignTokens::getFont("caption", 10));
        status_text->setStyleSheet(QString("color: %1; background: transparent;").arg(c.text_muted.name()));
        vl->addWidget(status_text);
    } else if (is_failed) {
        auto *err_text = new QLabel("Error en la descarga", row);
        err_text->setFont(DesignTokens::getFont("caption", 10));
        err_text->setStyleSheet(QString("color: %1; background: transparent;").arg(c.error.name()));
        vl->addWidget(err_text);
    }

    top->addLayout(vl, 1);

    if (is_active) {
        auto *cancel_btn = new QPushButton(row);
        cancel_btn->setFixedSize(28, 28);
        cancel_btn->setCursor(Qt::PointingHandCursor);
        cancel_btn->setIcon(IconProvider::getIcon("close", c.text_secondary, 14));
        cancel_btn->setIconSize(QSize(14, 14));
        cancel_btn->setStyleSheet(QString(
            "QPushButton { background: transparent; border: none; border-radius: 14px; }"
            "QPushButton:hover { background: %1; }"
        ).arg(c.bg_elevated.name()));
        std::string vid = video_id;
        connect(cancel_btn, &QPushButton::clicked, this, [vid]() {
            on_download_cancel_requested(vid);
        });
        top->addWidget(cancel_btn);
    } else if (is_completed) {
        auto *play_btn = new QPushButton(row);
        play_btn->setFixedSize(28, 28);
        play_btn->setCursor(Qt::PointingHandCursor);
        play_btn->setIcon(IconProvider::getIcon("play_arrow", QColor("#FFFFFF"), 14));
        play_btn->setIconSize(QSize(14, 14));
        play_btn->setStyleSheet(QString(
            "QPushButton { background-color: %1; border: none; border-radius: 14px; }"
            "QPushButton:hover { background-color: %2; }"
        ).arg(c.accent.name()).arg(c.accent_bright.name()));
        Track track_data;
        track_data.id = rust::String(video_id);
        track_data.title = rust::String(title);
        track_data.artist = rust::String(artist);
        track_data.thumbnail = rust::String(thumbnail_path);
        connect(play_btn, &QPushButton::clicked, this, [this, track_data]() {
            emit play_requested(track_data);
        });
        top->addWidget(play_btn);
    } else if (is_failed) {
        auto *retry_btn = new QPushButton(row);
        retry_btn->setFixedSize(28, 28);
        retry_btn->setCursor(Qt::PointingHandCursor);
        retry_btn->setIcon(IconProvider::getIcon("refresh", c.accent, 14));
        retry_btn->setIconSize(QSize(14, 14));
        retry_btn->setStyleSheet(QString(
            "QPushButton { background: transparent; border: none; border-radius: 14px; }"
            "QPushButton:hover { background: %1; }"
        ).arg(c.bg_elevated.name()));
        Track track_data;
        track_data.id = rust::String(video_id);
        track_data.title = rust::String(title);
        track_data.artist = rust::String(artist);
        connect(retry_btn, &QPushButton::clicked, this, [this, track_data]() {
            on_download_requested(track_data);
        });
        top->addWidget(retry_btn);
    }

    lay->addLayout(top);

    if (is_active) {
        auto *progress_bar = new QProgressBar(row);
        progress_bar->setObjectName("progress_bar");
        progress_bar->setFixedHeight(4);
        progress_bar->setTextVisible(false);
        progress_bar->setRange(0, 100);
        progress_bar->setValue(static_cast<int>(progress));
        progress_bar->setStyleSheet(QString(
            "QProgressBar { background: %1; border: none; border-radius: 2px; }"
            "QProgressBar::chunk { background: %2; border-radius: 2px; }"
        ).arg(c.bg_elevated.name()).arg(c.accent.name()));
        lay->addWidget(progress_bar);
    }

    row->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(row, &QWidget::customContextMenuRequested, this, [this, video_id, title, artist, status, thumbnail_path](const QPoint &pos) {
        auto *sender_widget = qobject_cast<QWidget*>(sender());
        if (!sender_widget) return;
        
        QMenu menu;
        bool is_completed = (status == "completed");
        bool is_active = (status == "queued" || status == "resolving" || status == "downloading");

        QAction *play = nullptr;
        if (is_completed) {
            play = menu.addAction("Reproducir");
        }

        bool is_fav = get_track_favorite_state(video_id);
        QAction *fav = menu.addAction(is_fav ? "Quitar de favoritos" : "Agregar a favoritos");

        QAction *next = nullptr;
        QAction *end = nullptr;
        if (is_completed) {
            next = menu.addAction("Reproducir siguiente");
            end = menu.addAction("Agregar a la cola");
        }

        QAction *cancel = nullptr;
        if (is_active) {
            cancel = menu.addAction("Cancelar descarga");
        }

        QAction *delete_db = menu.addAction("Eliminar de la lista");
        QAction *delete_both = nullptr;
        if (is_completed) {
            delete_both = menu.addAction("Eliminar descarga (borrar archivo)");
        }

        QAction *chosen = menu.exec(sender_widget->mapToGlobal(pos));
        if (!chosen) return;

        Track track_data;
        track_data.id = rust::String(video_id);
        track_data.title = rust::String(title);
        track_data.artist = rust::String(artist);
        track_data.thumbnail = rust::String(thumbnail_path);

        if (chosen == play) {
            emit play_requested(track_data);
        } else if (chosen == fav) {
            if (is_fav) {
                on_remove_favorite(video_id);
            } else {
                on_add_favorite(track_data);
            }
        } else if (chosen == next) {
            on_add_to_queue_next(track_data);
        } else if (chosen == end) {
            on_add_to_queue_end(track_data);
        } else if (chosen == cancel) {
            on_download_cancel_requested(video_id);
        } else if (chosen == delete_db) {
            auto reply = QMessageBox::question(
                this,
                "Eliminar descarga",
                "¿Quieres eliminar esta descarga de la lista?",
                QMessageBox::Yes | QMessageBox::No,
                QMessageBox::No);
            if (reply == QMessageBox::Yes) {
                on_delete_download(video_id, false);
            }
        } else if (chosen == delete_both) {
            auto reply = QMessageBox::question(
                this,
                "Eliminar archivo descargado",
                "¿Quieres eliminar esta descarga de la lista y borrar el archivo local?",
                QMessageBox::Yes | QMessageBox::No,
                QMessageBox::No);
            if (reply == QMessageBox::Yes) {
                on_delete_download(video_id, true);
            }
        }
    });

    return row;
}

void DownloadsView::update_row(QWidget *row, double percent, const std::string &status) {
    bool is_active = (status == "queued" || status == "resolving" || status == "downloading");
    row->setFixedHeight(is_active ? 88 : 64);

    auto *status_label = row->findChild<QLabel*>("status_label");
    if (status_label) {
        QString msg;
        if (status == "queued") msg = "En cola…";
        else if (status == "resolving") msg = "Resolviendo…";
        else msg = QString("Descargando… %1%").arg(static_cast<int>(percent));
        status_label->setText(msg);
    }

    auto *progress_bar = row->findChild<QProgressBar*>("progress_bar");
    if (progress_bar) {
        progress_bar->setValue(static_cast<int>(percent));
    }
}

QWidget *DownloadsView::make_batch_row(const std::string &/*parent_id*/, const std::string &parent_title,
                                        int total, int completed, double percent) {
    const auto &c = DesignTokens::current();

    auto *row = new QWidget(this);
    row->setFixedHeight(80);

    QString rowStyle = QString(
        "QWidget {\n"
        "    background-color: %1;\n"
        "    border-radius: 10px;\n"
        "    border: 1px solid %2;\n"
        "}\n"
        "QWidget:hover {\n"
        "    background-color: %3;\n"
        "}\n"
    )
    .arg(c.bg_elevated.name())
    .arg(c.border.name())
    .arg(QString("rgba(%1, %2, %3, %4)").arg(c.accent_dim.red()).arg(c.accent_dim.green()).arg(c.accent_dim.blue()).arg(c.accent_dim.alpha() / 255.0));
    row->setStyleSheet(rowStyle);

    auto *lay = new QVBoxLayout(row);
    lay->setContentsMargins(16, 10, 16, 10);
    lay->setSpacing(4);

    auto *top = new QHBoxLayout();
    top->setSpacing(10);

    auto *icon_lbl = new QLabel(row);
    icon_lbl->setFixedSize(20, 20);
    icon_lbl->setPixmap(IconProvider::getIcon("download", c.accent, 16).pixmap(20, 20));
    icon_lbl->setStyleSheet("background: transparent;");
    top->addWidget(icon_lbl);

    auto *vl = new QVBoxLayout();
    vl->setSpacing(2);
    vl->setContentsMargins(0, 0, 0, 0);

    auto *title_lbl = new QLabel(QString::fromStdString(parent_title), row);
    title_lbl->setObjectName("batch_title");
    title_lbl->setFont(DesignTokens::getFont("body", 13));
    title_lbl->setStyleSheet(QString("color: %1; font-weight: bold; background: transparent;").arg(c.text_primary.name()));
    vl->addWidget(title_lbl);

    auto *count_lbl = new QLabel(
        QString("Descargando lote… %1/%2 (%3%)")
            .arg(completed).arg(total).arg(static_cast<int>(percent)), row);
    count_lbl->setObjectName("batch_count");
    count_lbl->setFont(DesignTokens::getFont("caption", 11));
    count_lbl->setStyleSheet(QString("color: %1; background: transparent;").arg(c.text_secondary.name()));
    vl->addWidget(count_lbl);

    top->addLayout(vl, 1);
    lay->addLayout(top);

    auto *progress_bar = new QProgressBar(row);
    progress_bar->setObjectName("batch_progress");
    progress_bar->setFixedHeight(6);
    progress_bar->setTextVisible(false);
    progress_bar->setRange(0, 100);
    progress_bar->setValue(static_cast<int>(percent));
    progress_bar->setStyleSheet(QString(
        "QProgressBar { background: %1; border: none; border-radius: 3px; }"
        "QProgressBar::chunk { background: qlineargradient(x1:0, y1:0, x2:1, y2:0, "
        "    stop:0 %2, stop:1 %3); border-radius: 3px; }"
    ).arg(c.bg_overlay.name()).arg(c.accent.name()).arg(c.accent_bright.name()));
    lay->addWidget(progress_bar);

    return row;
}

void DownloadsView::update_batch_row(QWidget *row, int total, int completed, double percent) {
    auto *count_lbl = row->findChild<QLabel*>("batch_count");
    if (count_lbl) {
        count_lbl->setText(
            QString("Descargando lote… %1/%2 (%3%)")
                .arg(completed).arg(total).arg(static_cast<int>(percent)));
    }
    auto *progress_bar = row->findChild<QProgressBar*>("batch_progress");
    if (progress_bar) {
        progress_bar->setValue(static_cast<int>(percent));
    }
}

void DownloadsView::set_downloads(const std::vector<std::string> &titles,
                                   const std::vector<std::string> &artists,
                                   const std::vector<std::string> &thumbnails,
                                   const std::vector<std::string> &video_ids,
                                   const std::vector<std::string> &statuses,
                                   const std::vector<double> &progresses) {
    clear_downloads();
    size_t n = std::min({titles.size(), artists.size(), thumbnails.size(),
                         video_ids.size(), statuses.size(), progresses.size()});
    if (n == 0) {
        status_label_->setText("Sin descargas");
        status_label_->show();
        return;
    }
    status_label_->hide();
    for (size_t i = 0; i < n; ++i) {
        int idx = list_->count() - 1;
        auto *row = make_download_row(video_ids[i], titles[i], artists[i], thumbnails[i],
                                       progresses[i], statuses[i]);
        if (!video_ids[i].empty()) {
            row_map_[video_ids[i]] = row;
        }
        list_->insertWidget(idx, row);
    }
}

void DownloadsView::set_progress(const std::string &video_id, double percent, const std::string &status) {
    if (auto *row = row_map_.value(video_id, nullptr)) {
        update_row(row, percent, status);
        return;
    }
}

void DownloadsView::set_batch_progress(const std::string &parent_id, int total, int completed, double percent) {
    if (auto *row = batch_row_map_.value(parent_id, nullptr)) {
        update_batch_row(row, total, completed, percent);
    } else {
        status_label_->hide();
        auto *batch_row = make_batch_row(parent_id, "Lote", total, completed, percent);
        list_->insertWidget(2, batch_row);
        batch_row_map_[parent_id] = batch_row;
    }
}

void DownloadsView::clear_downloads() {
    row_map_.clear();
    for (auto it = batch_row_map_.begin(); it != batch_row_map_.end(); ++it) {
        it.value()->deleteLater();
    }
    batch_row_map_.clear();
    while (list_->count() > 3) {
        auto *item = list_->takeAt(2);
        if (item->widget()) {
            item->widget()->deleteLater();
        }
        delete item;
    }
}
