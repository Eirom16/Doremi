#include "widgets.h"
#include "design_tokens.h"
#include "icon_provider.h"
#include "components/artwork_loader.h"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QApplication>
#include <QPainter>
#include <QPainterPath>
#include <QPointer>
#include <QMenu>
#include <QAction>
#include <QKeyEvent>
#include <QFocusEvent>
#include "doremi/src/bridge.rs.h"

extern const std::vector<Playlist> &get_context_playlists();

QWidget* create_container_widget(QWidget *parent, const char *object_name) {
    auto *w = new QWidget(parent);
    w->setObjectName(object_name);
    return w;
}

QLabel* create_label(QWidget *parent, const std::string &text, const char *object_name,
                     int font_size, bool bold) {
    auto *label = new QLabel(QString::fromStdString(text), parent);
    label->setObjectName(object_name);
    QFont font = DesignTokens::getFont("body", font_size);
    font.setBold(bold);
    label->setFont(font);
    return label;
}

QPushButton* create_icon_button(QWidget *parent, const std::string &icon_text,
                                const char *object_name, int size) {
    auto *btn = new QPushButton(parent);
    btn->setObjectName(object_name);
    btn->setFixedSize(size, size);
    btn->setCursor(Qt::PointingHandCursor);
    
    // Set icon from ligand text
    const auto &c = DesignTokens::current();
    btn->setIcon(IconProvider::getIcon(QString::fromStdString(icon_text), c.text_primary, size - 8));
    btn->setIconSize(QSize(size - 8, size - 8));
    btn->setStyleSheet("QPushButton { background: transparent; border: none; }");
    return btn;
}

QSlider* create_seek_slider(QWidget *parent) {
    auto *slider = new QSlider(Qt::Horizontal, parent);
    slider->setRange(0, 1000);
    slider->setValue(0);
    
    const auto &c = DesignTokens::current();
    QString style = QString(
        "QSlider::groove:horizontal {\n"
        "    height: 4px;\n"
        "    background: rgba(255, 255, 255, 0.1);\n"
        "    border-radius: 2px;\n"
        "}\n"
        "QSlider::sub-page:horizontal {\n"
        "    background: %1;\n"
        "    border-radius: 2px;\n"
        "}\n"
        "QSlider::handle:horizontal {\n"
        "    background: %2;\n"
        "    width: 10px;\n"
        "    height: 10px;\n"
        "    margin: -3px 0;\n"
        "    border-radius: 5px;\n"
        "}\n"
    )
    .arg(c.accent.name())
    .arg(c.accent_bright.name());
    slider->setStyleSheet(style);
    
    return slider;
}

void set_widget_style(QWidget *widget, const std::string &qss) {
    widget->setStyleSheet(QString::fromStdString(qss));
}

// ── ClickableItem ──

ClickableItem::ClickableItem(const std::string &title, const std::string &subtitle,
                             QWidget *parent)
    : QWidget(parent), title_(title), subtitle_(subtitle)
{
    const auto &c = DesignTokens::current();
    setFixedHeight(52); // Slightly taller for premium look
    setCursor(Qt::PointingHandCursor);
    setContextMenuPolicy(Qt::CustomContextMenu);
    // Keyboard accessibility: reachable via Tab and operable with Enter/Space.
    setFocusPolicy(Qt::StrongFocus);
    setAccessibleName(QString::fromStdString(title));
    if (!subtitle.empty()) {
        setAccessibleDescription(QString::fromStdString(subtitle));
    }

    auto *lay = new QHBoxLayout(this);
    lay->setContentsMargins(12, 6, 12, 6);
    lay->setSpacing(12);

    // Left artwork placeholder
    artwork_label_ = new QLabel(this);
    artwork_label_->setFixedSize(36, 36);
    artwork_label_->setStyleSheet(QString("background-color: %1; border-radius: 4px;").arg(c.bg_elevated.name()));
    artwork_label_->setPixmap(IconProvider::getIcon("music_note", c.text_secondary, 18).pixmap(36, 36));
    artwork_label_->setAlignment(Qt::AlignCenter);
    lay->addWidget(artwork_label_);

    auto *vl = new QVBoxLayout();
    vl->setSpacing(2);
    vl->setContentsMargins(0, 0, 0, 0);
    
    title_label_ = new QLabel(QString::fromStdString(title), this);
    title_label_->setFont(DesignTokens::getFont("body", 13));
    title_label_->setStyleSheet(QString("color: %1; background: transparent;").arg(c.text_primary.name()));
    vl->addWidget(title_label_);

    if (!subtitle.empty()) {
        sub_label_ = new QLabel(QString::fromStdString(subtitle), this);
        sub_label_->setFont(DesignTokens::getFont("caption", 11));
        sub_label_->setStyleSheet(QString("color: %1; background: transparent;").arg(c.text_secondary.name()));
        vl->addWidget(sub_label_);
    }
    lay->addLayout(vl, 1);

    setStyleSheet("ClickableItem { background: transparent; border-radius: 6px; }");

    connect(this, &QWidget::customContextMenuRequested, this, [this](const QPoint &pos) {
        show_context_menu(mapToGlobal(pos));
    });
}

void ClickableItem::set_item_type(const std::string &type) {
    item_type_ = type;
    const auto &c = DesignTokens::current();
    QString iconName = "music_note";
    if (type == "artist") iconName = "person";
    else if (type == "album") iconName = "album";
    else if (type == "playlist") iconName = "queue_music";
    else if (type == "podcast" || type == "show" || type == "episode") iconName = "podcasts";
    
    artwork_label_->setPixmap(IconProvider::getIcon(iconName, c.text_secondary, 18).pixmap(36, 36));
}

void ClickableItem::set_thumbnail(const std::string &thumbnail_url) {
    if (thumbnail_url.empty()) return;
    QPointer<QLabel> label_ptr(artwork_label_);
    ArtworkLoader::load(QString::fromStdString(thumbnail_url), QSize(36, 36), [label_ptr](const QPixmap &pixmap) {
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

void ClickableItem::mousePressEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton) {
        emit clicked();
    }
    QWidget::mousePressEvent(event);
}

void ClickableItem::enterEvent(QEnterEvent *) {
    const auto &c = DesignTokens::current();
    QString hover_bg = QString("rgba(%1, %2, %3, %4)")
        .arg(c.accent_dim.red()).arg(c.accent_dim.green()).arg(c.accent_dim.blue()).arg(c.accent_dim.alpha() / 255.0);
    setStyleSheet(QString("ClickableItem { background: %1; border-radius: 6px; }").arg(hover_bg));
}

void ClickableItem::leaveEvent(QEvent *) {
    if (!hasFocus()) {
        setStyleSheet("ClickableItem { background: transparent; border-radius: 6px; }");
    }
}

void ClickableItem::keyPressEvent(QKeyEvent *event) {
    switch (event->key()) {
    case Qt::Key_Return:
    case Qt::Key_Enter:
    case Qt::Key_Space:
        emit clicked();
        event->accept();
        return;
    case Qt::Key_Menu:
        show_context_menu(mapToGlobal(rect().center()));
        event->accept();
        return;
    default:
        QWidget::keyPressEvent(event);
    }
}

void ClickableItem::focusInEvent(QFocusEvent *) {
    const auto &c = DesignTokens::current();
    setStyleSheet(QString(
        "ClickableItem { background: transparent; border-radius: 6px; border: 1px solid %1; }"
    ).arg(c.accent.name()));
}

void ClickableItem::focusOutEvent(QFocusEvent *) {
    setStyleSheet("ClickableItem { background: transparent; border-radius: 6px; }");
}

void ClickableItem::show_context_menu(const QPoint &global_pos) {
    const auto &c = DesignTokens::current();
    QMenu menu(this);

    // Collection types (artist/album/playlist/show) get a navigation-oriented menu
    if (item_type_ == "artist" || item_type_ == "album" || item_type_ == "playlist" || item_type_ == "show") {
        QString open_label = item_type_ == "artist" ? "Ir al artista"
                           : item_type_ == "album"  ? "Ir al álbum"
                           : item_type_ == "show"   ? "Ir al podcast"
                                                    : "Ir a la playlist";
        QAction *open = new QAction(IconProvider::getIcon("open_in_new", c.text_primary, 16), open_label, &menu);
        menu.addAction(open);

        bool is_fav = false;
        bool has_fav = false;
        if (item_type_ == "artist") {
            is_fav = get_artist_favorite_state(item_id_);
            has_fav = true;
        } else if (item_type_ == "album") {
            is_fav = get_album_favorite_state(item_id_);
            has_fav = true;
        } else if (item_type_ == "show") {
            is_fav = get_show_favorite_state(item_id_);
            has_fav = true;
        }

        QAction *fav = nullptr;
        if (has_fav) {
            menu.addSeparator();
            fav = new QAction(
                IconProvider::getIcon(is_fav ? "favorite" : "favorite_border", is_fav ? c.accent : c.text_secondary, 16),
                is_fav ? "Quitar de favoritos" : "Agregar a favoritos",
                &menu
            );
            menu.addAction(fav);
        }

        QAction *chosen = menu.exec(global_pos);
        if (chosen == open) {
            emit context_action("open", item_id_);
            emit clicked();
        } else if (fav && chosen == fav) {
            emit context_action(is_fav ? "remove_favorite" : "add_favorite", item_id_);
        }
        return;
    }

    bool is_video = item_type_ == "video";
    QAction *play = new QAction(IconProvider::getIcon("play_arrow", c.text_primary, 16), "Reproducir", &menu);
    QAction *play_next = new QAction(IconProvider::getIcon("playlist_play", c.text_primary, 16), "Reproducir después", &menu);
    QAction *add_to_queue = new QAction(IconProvider::getIcon("queue_music", c.text_primary, 16), "Añadir al final de la cola", &menu);
    
    bool is_track_fav = get_track_favorite_state(item_id_);
    QAction *fav = new QAction(
        IconProvider::getIcon(is_track_fav ? "favorite" : "favorite_border", is_track_fav ? c.accent : c.text_secondary, 16),
        is_track_fav ? "Quitar de favoritos" : "Agregar a favoritos",
        &menu
    );
    QAction *download = new QAction(IconProvider::getIcon("download", c.text_primary, 16),
                                    is_video ? "Descargar video" : "Descargar canción", &menu);

    menu.addAction(play);
    menu.addAction(play_next);
    menu.addAction(add_to_queue);
    menu.addSeparator();
    menu.addAction(fav);
    menu.addAction(download);
    menu.addSeparator();

    QMenu *playlist_menu = menu.addMenu(IconProvider::getIcon("playlist_add", c.text_secondary, 16), "Añadir a playlist");
    const auto &playlists = get_context_playlists();
    if (playlists.empty()) {
        QAction *no_playlists = playlist_menu->addAction("(Sin playlists)");
        no_playlists->setEnabled(false);
    } else {
        for (const auto &p : playlists) {
            playlist_menu->addAction(QString::fromStdString(static_cast<std::string>(p.name)));
        }
    }

    QAction *chosen = menu.exec(global_pos);
    if (chosen == play) {
        emit context_action("play", item_id_);
        emit clicked();
    } else if (chosen == play_next) {
        emit context_action("queue_next", item_id_);
    } else if (chosen == add_to_queue) {
        emit context_action("queue_end", item_id_);
    } else if (chosen == fav) {
        emit context_action(is_track_fav ? "remove_favorite" : "add_favorite", item_id_);
    } else if (chosen == download) {
        emit context_action("download", item_id_);
    } else if (chosen && qobject_cast<QWidget*>(chosen->parent()) == playlist_menu) {
        int idx = -1;
        for (const auto &action : playlist_menu->actions()) {
            ++idx;
            if (action == chosen) break;
        }
        if (idx >= 0 && idx < (int)playlists.size()) {
            Track t;
            t.id = item_id_;
            t.title = title_;
            t.artist = subtitle_;
            t.album = "";
            t.duration_ms = 0;
            t.thumbnail = "";
            on_add_to_playlist(t, static_cast<std::string>(playlists[idx].id));
        }
    }
}
