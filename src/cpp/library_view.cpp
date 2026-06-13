#include "library_view.h"
#include "design_tokens.h"
#include "icon_provider.h"
#include <QFrame>
#include <QHBoxLayout>

LibraryView::LibraryView(QWidget *parent)
    : QWidget(parent)
{
    const auto &c = DesignTokens::current();

    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    auto *tab_bar = new QWidget(this);
    tab_bar->setFixedHeight(44);
    
    // Bottom border under tab bar
    tab_bar->setStyleSheet(QString("background-color: transparent; border-bottom: 1px solid %1;")
        .arg(QString("rgba(%1, %2, %3, %4)").arg(c.border.red()).arg(c.border.green()).arg(c.border.blue()).arg(c.border.alpha() / 255.0))
    );
    
    auto *tab_lay = new QHBoxLayout(tab_bar);
    tab_lay->setContentsMargins(24, 0, 24, 0);
    tab_lay->setSpacing(8);
    
    for (const char *name : {"Playlists", "Canciones", "Álbumes", "Artistas"}) {
        auto *btn = new QPushButton(name, tab_bar);
        btn->setCheckable(true);
        btn->setFixedHeight(43); // 1px less to overlap with border-bottom
        btn->setCursor(Qt::PointingHandCursor);
        btn->setFont(DesignTokens::getFont("body", 13));
        
        QString btnStyle = QString(
            "QPushButton {\n"
            "    background: transparent;\n"
            "    border: none;\n"
            "    border-bottom: 2px solid transparent;\n"
            "    color: %1;\n"
            "    padding: 0 12px;\n"
            "}\n"
            "QPushButton:hover {\n"
            "    color: %2;\n"
            "}\n"
            "QPushButton:checked {\n"
            "    color: %3;\n"
            "    border-bottom: 2px solid %3;\n"
            "    font-weight: 500;\n"
            "}\n"
        )
        .arg(c.text_secondary.name())
        .arg(c.text_primary.name())
        .arg(c.accent.name());
        
        btn->setStyleSheet(btnStyle);
        tab_lay->addWidget(btn);
        tab_btns_.push_back(btn);
        connect(btn, &QPushButton::clicked, this, [this, name]() { emit tab_changed(name); });
    }
    tab_lay->addStretch(1);
    root->addWidget(tab_bar);

    auto *scroll = new QScrollArea(this);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setStyleSheet("background: transparent; border: none;");

    auto *inner = new QWidget();
    inner->setStyleSheet("background: transparent;");
    list_ = new QVBoxLayout(inner);
    list_->setContentsMargins(24, 16, 24, 16);
    list_->setSpacing(6);

    auto *placeholder = new QLabel("Tu biblioteca está vacía", inner);
    placeholder->setFont(DesignTokens::getFont("body", 14));
    placeholder->setStyleSheet(QString("color: %1; padding: 24px;").arg(c.text_muted.name()));
    placeholder->setAlignment(Qt::AlignCenter);
    list_->addWidget(placeholder);
    list_->addStretch(1);

    scroll->setWidget(inner);
    root->addWidget(scroll, 1);
    setStyleSheet("background: transparent;");
}

QWidget *LibraryView::make_list_item(const std::string &text, const std::string &sub) {
    auto *ci = new ClickableItem(text, sub, this);
    ci->set_item_id(text);
    
    // Find icon label and set special icon based on active tab
    auto labels = ci->findChildren<QLabel*>();
    for (auto *label : labels) {
        if (label->font().family() == "Material Symbols Rounded") {
            const auto &c = DesignTokens::current();
            QString iconName = "music_note";
            if (active_tab_ == "Playlists") iconName = "queue_music";
            else if (active_tab_ == "Álbumes") iconName = "album";
            else if (active_tab_ == "Artistas") iconName = "person";
            
            label->setPixmap(IconProvider::getIcon(iconName, c.text_secondary, 18).pixmap(36, 36));
        }
    }
    
    connect(ci, &ClickableItem::clicked, this, [this, text, sub]() {
        std::string info = sub.empty() ? text : text + " — " + sub;
        emit play_requested(info);
    });
    connect(ci, &ClickableItem::context_action, this, [this, text, sub](const std::string &action, const std::string &) {
        if (action == "add_favorite") {
            emit remove_favorite_requested(text);
        } else if (action == "download") {
            std::string info = sub.empty() ? text : text + " — " + sub;
            emit download_requested(info);
        }
    });
    return ci;
}

void LibraryView::clear_list() {
    while (list_->count() > 0) {
        auto *item = list_->takeAt(0);
        if (item->widget()) {
            item->widget()->deleteLater();
        }
        delete item;
    }
}

void LibraryView::set_playlists(const std::vector<std::string> &names,
                                 const std::vector<std::string> &counts) {
    active_tab_ = "Playlists";
    clear_list();
    for (size_t i = 0; i < names.size() && i < counts.size(); ++i) {
        list_->addWidget(make_list_item(names[i], counts[i] + " canciones"));
    }
    list_->addStretch(1);
    set_active_tab(active_tab_);
}

void LibraryView::set_songs(const std::vector<std::string> &titles) {
    active_tab_ = "Canciones";
    clear_list();
    for (const auto &t : titles) {
        // Split "Title — Artist" to extract details
        std::string title = t;
        std::string sub;
        auto pos = t.rfind(" — ");
        if (pos != std::string::npos) {
            title = t.substr(0, pos);
            sub = t.substr(pos + 3);
        }
        list_->addWidget(make_list_item(title, sub));
    }
    list_->addStretch(1);
    set_active_tab(active_tab_);
}

void LibraryView::set_albums(const std::vector<std::string> &titles,
                              const std::vector<std::string> &artists) {
    active_tab_ = "Álbumes";
    clear_list();
    for (size_t i = 0; i < titles.size() && i < artists.size(); ++i) {
        list_->addWidget(make_list_item(titles[i], artists[i]));
    }
    list_->addStretch(1);
    set_active_tab(active_tab_);
}

void LibraryView::set_artists(const std::vector<std::string> &names) {
    active_tab_ = "Artistas";
    clear_list();
    for (const auto &n : names) {
        list_->addWidget(make_list_item(n, ""));
    }
    list_->addStretch(1);
    set_active_tab(active_tab_);
}

std::string LibraryView::current_tab() const {
    return active_tab_;
}

void LibraryView::set_active_tab(const std::string &tab) {
    for (auto *btn : tab_btns_) {
        btn->setChecked(btn->text().toStdString() == tab);
    }
}
