#include "widgets.h"
#include "design_tokens.h"
#include "icon_provider.h"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QApplication>
#include <QMenu>
#include <QAction>

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

    auto *lay = new QHBoxLayout(this);
    lay->setContentsMargins(12, 6, 12, 6);
    lay->setSpacing(12);

    // Left artwork placeholder
    auto *artwork = new QLabel(this);
    artwork->setFixedSize(36, 36);
    artwork->setStyleSheet(QString("background-color: %1; border-radius: 4px;").arg(c.bg_elevated.name()));
    artwork->setPixmap(IconProvider::getIcon("music_note", c.text_secondary, 18).pixmap(36, 36));
    artwork->setAlignment(Qt::AlignCenter);
    lay->addWidget(artwork);

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
    setStyleSheet("ClickableItem { background: transparent; border-radius: 6px; }");
}

void ClickableItem::show_context_menu(const QPoint &global_pos) {
    const auto &c = DesignTokens::current();
    QMenu menu(this);

    QAction *play = new QAction(IconProvider::getIcon("play_arrow", c.text_primary, 16), "Reproducir", &menu);
    QAction *fav = new QAction(IconProvider::getIcon("favorite", c.accent, 16), "Añadir a favoritos", &menu);
    QAction *download = new QAction(IconProvider::getIcon("download", c.text_primary, 16), "Descargar canción", &menu);

    menu.addAction(play);
    menu.addAction(fav);
    menu.addAction(download);
    menu.addSeparator();

    QMenu *playlist_menu = menu.addMenu(IconProvider::getIcon("playlist_add", c.text_secondary, 16), "Añadir a playlist");
    QAction *no_playlists = playlist_menu->addAction("(Sin playlists)");
    no_playlists->setEnabled(false);

    QAction *chosen = menu.exec(global_pos);
    if (chosen == play) {
        emit context_action("play", item_id_);
        emit clicked();
    } else if (chosen == fav) {
        emit context_action("add_favorite", item_id_);
    } else if (chosen == download) {
        emit context_action("download", item_id_);
    }
}
