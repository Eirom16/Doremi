#include <QFrame>
#include <QPixmap>
#include <QPainter>
#include <QPainterPath>
#include <QFile>
#include "doremi/src/bridge.rs.h"
#include "downloads_view.h"
#include "design_tokens.h"
#include "icon_provider.h"
#include "widgets.h"
#include <QMenu>
#include <QAction>
#include <QMessageBox>
#include <QButtonGroup>
#include <QPointer>
#include "components/artwork_loader.h"

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
    : QWidget(parent), active_tab_("songs")
{
    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    list_ = new QVBoxLayout();
    list_->setContentsMargins(DesignTokens::pagePadding());
    list_->setSpacing(6);

    auto *header = new QLabel(tr_q("downloads"), this);
    header->setFont(DesignTokens::getFont("heading_lg"));
    header->setProperty("textRole", "heading");
    list_->addWidget(header);

    auto *tab_bar = new QWidget(this);
    tab_bar->setFixedHeight(44);
    tab_bar->setObjectName("downloadsTabBar");
    
    auto *tab_lay = new QHBoxLayout(tab_bar);
    tab_lay->setContentsMargins(0, 0, 0, 0);
    tab_lay->setSpacing(8);
    
    struct TabSpec {
        const char *key;
        const char *translation_key;
    };
    const TabSpec tabs[] = {
        {"songs", "songs"},
        {"albums", "albums"},
        {"playlists", "playlists"},
        {"shows", "shows"}
    };

    QButtonGroup *group = new QButtonGroup(this);
    group->setExclusive(true);

    for (const auto &tab : tabs) {
        auto *btn = new QPushButton(tr_q(tab.translation_key), tab_bar);
        btn->setProperty("tabKey", QString(tab.key));
        btn->setCheckable(true);
        btn->setFixedHeight(43);
        btn->setCursor(Qt::PointingHandCursor);
        btn->setFont(DesignTokens::getFont("body_sm"));
        btn->setObjectName("downloadsTabBtn");
        tab_lay->addWidget(btn);
        group->addButton(btn);
        tab_btns_.push_back(btn);

        if (std::string(tab.key) == active_tab_) {
            btn->setChecked(true);
        }

        connect(btn, &QPushButton::toggled, this, [this, btn](bool checked) {
            if (checked) {
                active_tab_ = btn->property("tabKey").toString().toStdString();
                update_view();
            }
        });
    }
    tab_lay->addStretch(1);
    list_->addWidget(tab_bar);

    status_label_ = new EmptyState(this);
    status_label_->setIcon("download");
    status_label_->setTitle(tr_q("no_downloads"));
    status_label_->applyPanelStyle("empty");
    list_->addWidget(status_label_);

    rows_layout_ = new QVBoxLayout();
    rows_layout_->setContentsMargins(0, 0, 0, 0);
    rows_layout_->setSpacing(6);
    list_->addLayout(rows_layout_);

    list_->addStretch(1);
    root->addLayout(list_);
    setProperty("bgRole", "transparent");
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

    QWidget *row = new QWidget(this);
    row->setObjectName("DownloadRow");
    row->setFixedHeight(is_active || is_failed ? 88 : 64);

    auto *lay = new QVBoxLayout(row);
    lay->setContentsMargins(12, 6, 12, 6);
    lay->setSpacing(4);

    auto *top = new QHBoxLayout();
    top->setSpacing(12);

    auto *thumb = new QLabel(row);
    thumb->setObjectName("downloadRowThumb");
    thumb->setFixedSize(36, 36);
    thumb->setAlignment(Qt::AlignCenter);
    thumb->setStyleSheet(QString("background: %1; border-radius: %2px;").arg(c.bg_elevated.name()).arg(DesignTokens::radius().sm));

    // Try local file first, fall back to ArtworkLoader for remote URLs
    bool thumbLoaded = false;
    if (!thumbnail_path.empty()) {
        QString path = QString::fromStdString(thumbnail_path);
        if (QFile::exists(path)) {
            QPixmap px(path);
            if (!px.isNull()) {
                thumb->setPixmap(getRoundedPixmap(
                    px.scaled(36, 36, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation), 4
                ));
                thumbLoaded = true;
            }
        }
        if (!thumbLoaded) {
            QPointer<QLabel> label_ptr(thumb);
            ArtworkLoader::load(path, QSize(36, 36),
                [label_ptr, c](const QPixmap &pixmap) {
                    if (!label_ptr) return;
                    QPixmap dest(pixmap.size());
                    dest.fill(Qt::transparent);
                    QPainter painter(&dest);
                    painter.setRenderHint(QPainter::Antialiasing);
                    QPainterPath path;
                    path.addRoundedRect(pixmap.rect(), 4, 4);
                    painter.setClipPath(path);
                    painter.drawPixmap(0, 0, pixmap);
                    label_ptr->setPixmap(dest.scaled(36, 36, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation));
                });
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
    t->setObjectName("downloadRowTitle");
    t->setFont(DesignTokens::getFont("body_sm"));
    if (is_cancelled) {
        t->setProperty("textRole", "muted");
        QFont f = t->font();
        f.setStrikeOut(true);
        t->setFont(f);
    } else {
        t->setProperty("textRole", "primary");
    }
    vl->addWidget(t);

    auto *a = new QLabel(QString::fromStdString(artist), row);
    a->setObjectName("downloadRowArtist");
    a->setFont(DesignTokens::getFont("caption_sm"));
    a->setProperty("textRole", "secondary");
    vl->addWidget(a);

    if (is_active) {
        auto *status_text = new QLabel(row);
        status_text->setObjectName("status_label");
        QString statusMsg;
        if (status == "queued") statusMsg = tr_q("queued_status");
        else if (status == "resolving") statusMsg = tr_q("resolving_status");
        else statusMsg = tr_q("downloading_status").arg(static_cast<int>(progress));
        status_text->setText(statusMsg);
        status_text->setFont(DesignTokens::getFont("caption", 10));
        status_text->setProperty("textRole", "muted");
        vl->addWidget(status_text);
    } else if (is_failed) {
        auto *err_text = new QLabel(tr_q("download_error"), row);
        err_text->setObjectName("downloadRowError");
        err_text->setFont(DesignTokens::getFont("caption", 10));
        err_text->setObjectName("downloadRowError");
        vl->addWidget(err_text);
    }

    top->addLayout(vl, 1);

    if (is_active) {
        auto *cancel_btn = new QPushButton(row);
        cancel_btn->setObjectName("downloadRowCancel");
        cancel_btn->setFixedSize(28, 28);
        cancel_btn->setCursor(Qt::PointingHandCursor);
        cancel_btn->setIcon(IconProvider::getIcon("close", c.text_secondary, 14));
        cancel_btn->setIconSize(QSize(14, 14));
        std::string vid = video_id;
        connect(cancel_btn, &QPushButton::clicked, this, [vid]() {
            on_download_cancel_requested(vid);
        });
        top->addWidget(cancel_btn);
    } else if (is_completed) {
        auto *play_btn = new QPushButton(row);
        play_btn->setObjectName("downloadRowPlay");
        play_btn->setFixedSize(28, 28);
        play_btn->setCursor(Qt::PointingHandCursor);
        play_btn->setIcon(IconProvider::getIcon("play_arrow", c.text_on_accent, 14));
        play_btn->setIconSize(QSize(14, 14));
        play_btn->setProperty("buttonRole", "icon");
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
        retry_btn->setObjectName("downloadRowRetry");
        retry_btn->setFixedSize(28, 28);
        retry_btn->setCursor(Qt::PointingHandCursor);
        retry_btn->setIcon(IconProvider::getIcon("refresh", c.accent, 14));
        retry_btn->setIconSize(QSize(14, 14));
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
            play = menu.addAction(tr_q("play"));
        }

        bool is_fav = get_track_favorite_state(video_id);
        QAction *fav = menu.addAction(is_fav 
            ? tr_q("remove_favorite")
            : tr_q("add_favorite"));

        QAction *next = nullptr;
        QAction *end = nullptr;
        if (is_completed) {
            next = menu.addAction(tr_q("play_next"));
            end = menu.addAction(tr_q("add_to_queue"));
        }

        QAction *cancel = nullptr;
        if (is_active) {
            cancel = menu.addAction(tr_q("cancel_download"));
        }

        QAction *delete_db = menu.addAction(tr_q("remove_from_list"));
        QAction *delete_both = nullptr;
        if (is_completed) {
            delete_both = menu.addAction(tr_q("delete_download_file"));
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
                tr_q("confirm_delete_download_title"),
                tr_q("confirm_delete_download_list_desc"),
                QMessageBox::Yes | QMessageBox::No,
                QMessageBox::No);
            if (reply == QMessageBox::Yes) {
                on_delete_download(video_id, false);
            }
        } else if (chosen == delete_both) {
            auto reply = QMessageBox::question(
                this,
                tr_q("delete_download_file_title"),
                tr_q("delete_download_file_desc"),
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
    row->setObjectName("BatchRow");
    row->setFixedHeight(80);

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
    title_lbl->setFont(DesignTokens::getFont("body_sm"));
    title_lbl->setProperty("textRole", "primary");
    vl->addWidget(title_lbl);

    auto *count_lbl = new QLabel(
        QString("Descargando lote… %1/%2 (%3%)")
            .arg(completed).arg(total).arg(static_cast<int>(percent)), row);
    count_lbl->setObjectName("batch_count");
    count_lbl->setFont(DesignTokens::getFont("caption_sm"));
    count_lbl->setProperty("textRole", "secondary");
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
        "QProgressBar { background: %1; border: none; border-radius: %4px; }"
        "QProgressBar::chunk { background: qlineargradient(x1:0, y1:0, x2:1, y2:0, "
        "    stop:0 %2, stop:1 %3); border-radius: %4px; }"
    ).arg(c.bg_overlay.name(), c.accent.name(), c.accent_bright.name(), QString::number(DesignTokens::radius().xs)));
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

void DownloadsView::set_downloads(const std::vector<DownloadItem> &items) {
    all_downloads_ = items;
    update_view();
}

void DownloadsView::update_view() {
    clear_downloads();

    if (active_tab_ == "songs") {
        if (all_downloads_.empty()) {
            status_label_->setTitle(tr_q("no_downloads"));
            status_label_->show();
            return;
        }
        status_label_->hide();

        for (const auto &item : all_downloads_) {
            std::string parent_id = static_cast<std::string>(item.parent_playlist_id);
            if (parent_id.rfind("show_", 0) == 0 || parent_id.rfind("podcast_", 0) == 0) {
                continue;
            }

            auto *row = make_download_row(
                static_cast<std::string>(item.video_id),
                static_cast<std::string>(item.title),
                static_cast<std::string>(item.artist),
                static_cast<std::string>(item.thumbnail_url),
                item.progress,
                static_cast<std::string>(item.status)
            );
            if (!item.video_id.empty()) {
                row_map_[static_cast<std::string>(item.video_id)] = row;
            }
            rows_layout_->addWidget(row);
        }
    } else {
        struct GroupData {
            std::string id;
            std::string title;
            std::string thumbnail;
            std::string artist;
            int count = 0;
        };
        std::vector<GroupData> groups;
        std::map<std::string, size_t> group_index;

        for (const auto &item : all_downloads_) {
            if (static_cast<std::string>(item.status) != "completed") {
                continue;
            }

            std::string parent_id = static_cast<std::string>(item.parent_playlist_id);
            if (parent_id.empty()) {
                continue;
            }

            bool matches = false;
            if (active_tab_ == "albums") {
                matches = (parent_id.rfind("VL", 0) != 0 && parent_id.rfind("PL", 0) != 0 &&
                           parent_id.rfind("show_", 0) != 0 && parent_id.rfind("podcast_", 0) != 0);
            } else if (active_tab_ == "playlists") {
                matches = (parent_id.rfind("VL", 0) == 0 || parent_id.rfind("PL", 0) == 0);
            } else if (active_tab_ == "shows") {
                matches = (parent_id.rfind("show_", 0) == 0 || parent_id.rfind("podcast_", 0) == 0);
            }

            if (matches) {
                if (group_index.find(parent_id) == group_index.end()) {
                    GroupData gd;
                    gd.id = parent_id;
                    gd.title = static_cast<std::string>(item.parent_playlist_title);
                    if (gd.title.empty()) {
                        gd.title = static_cast<std::string>(item.album);
                    }
                    if (gd.title.empty()) {
                        gd.title = "Colección Desconocida";
                    }
                    gd.thumbnail = static_cast<std::string>(item.parent_playlist_thumbnail_url);
                    if (gd.thumbnail.empty()) {
                        gd.thumbnail = static_cast<std::string>(item.thumbnail_url);
                    }
                    gd.artist = static_cast<std::string>(item.artist);
                    gd.count = 1;
                    group_index[parent_id] = groups.size();
                    groups.push_back(gd);
                } else {
                    groups[group_index[parent_id]].count++;
                }
            }
        }

        if (groups.empty()) {
            status_label_->setTitle(tr_q("no_downloads_category"));
            status_label_->show();
            return;
        }
        status_label_->hide();

        for (const auto &g : groups) {
            std::string sub_text;
            if (active_tab_ == "albums") {
                sub_text = "Álbum • " + std::to_string(g.count) + " canciones";
            } else if (active_tab_ == "playlists") {
                sub_text = "Lista de reproducción • " + std::to_string(g.count) + " canciones";
            } else if (active_tab_ == "shows") {
                sub_text = "Podcast • " + std::to_string(g.count) + " episodios";
            }

            auto *ci = new ClickableItem(g.title, sub_text, this);
            ci->set_item_id(g.id);
            ci->set_item_type(active_tab_ == "albums" ? "album" :
                              active_tab_ == "playlists" ? "playlist" : "show");
            ci->set_thumbnail(g.thumbnail);

            connect(ci, &ClickableItem::clicked, this, [this, g]() {
                if (active_tab_ == "albums") {
                    emit album_requested(g.id);
                } else if (active_tab_ == "playlists") {
                    emit playlist_requested(g.id);
                } else if (active_tab_ == "shows") {
                    emit show_requested(g.id);
                }
            });

            rows_layout_->addWidget(ci);
        }
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
        rows_layout_->insertWidget(0, batch_row);
        batch_row_map_[parent_id] = batch_row;
    }
}

void DownloadsView::clear_downloads() {
    row_map_.clear();
    for (auto it = batch_row_map_.begin(); it != batch_row_map_.end(); ++it) {
        it.value()->deleteLater();
    }
    batch_row_map_.clear();

    QLayoutItem *child;
    while ((child = rows_layout_->takeAt(0)) != nullptr) {
        if (child->widget()) {
            child->widget()->deleteLater();
        }
        delete child;
    }
}

void DownloadsView::update_theme() {
    if (status_label_) {
        status_label_->applyPanelStyle("empty");
    }
    for (auto *lbl : findChildren<QLabel*>("batch_title")) {
        lbl->setProperty("textRole", "primary");
    }
    for (auto *lbl : findChildren<QLabel*>("batch_count")) {
        lbl->setProperty("textRole", "secondary");
    }
    for (auto *lbl : findChildren<QLabel*>("downloadRowTitle")) {
        if (lbl->font().strikeOut()) {
            lbl->setProperty("textRole", "muted");
        } else {
            lbl->setProperty("textRole", "primary");
        }
    }
    for (auto *lbl : findChildren<QLabel*>("downloadRowArtist")) {
        lbl->setProperty("textRole", "secondary");
    }
    for (auto *lbl : findChildren<QLabel*>("status_label")) {
        lbl->setProperty("textRole", "muted");
    }
    for (auto *lbl : findChildren<QLabel*>("downloadRowError")) {
        lbl->setObjectName("downloadRowError");
    }
}
