#include <QFrame>
#include "search_view.h"
#include "design_tokens.h"
#include "icon_provider.h"

SearchView::SearchView(QWidget *parent)
    : QWidget(parent)
{
    const auto &c = DesignTokens::current();

    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(24, 24, 24, 24);
    root->setSpacing(16);

    header_ = new QLabel("Resultados", this);
    header_->setFont(DesignTokens::getFont("heading_lg", 20));
    header_->setStyleSheet(QString("color: %1; font-weight: bold; background: transparent;").arg(c.text_primary.name()));
    root->addWidget(header_);

    filters_ = new QHBoxLayout();
    filters_->setSpacing(8);
    for (const char *name : {"Canciones", "Videos", "Álbumes", "Artistas", "Playlists"}) {
        auto *btn = new QPushButton(name, this);
        btn->setCheckable(true);
        btn->setFixedHeight(32);
        btn->setCursor(Qt::PointingHandCursor);
        btn->setFont(DesignTokens::getFont("body", 12));
        
        QString btnStyle = QString(
            "QPushButton {\n"
            "    background-color: %1;\n"
            "    border: 1px solid %2;\n"
            "    border-radius: 16px;\n"
            "    padding: 0 16px;\n"
            "    color: %3;\n"
            "}\n"
            "QPushButton:hover {\n"
            "    background-color: %4;\n"
            "    color: %5;\n"
            "}\n"
            "QPushButton:checked {\n"
            "    background-color: %6;\n"
            "    border-color: %6;\n"
            "    color: #FFFFFF;\n"
            "    font-weight: 500;\n"
            "}\n"
        )
        .arg(c.bg_surface.name())
        .arg(c.border.name())
        .arg(c.text_secondary.name())
        .arg(QString("rgba(%1, %2, %3, %4)").arg(c.accent_dim.red()).arg(c.accent_dim.green()).arg(c.accent_dim.blue()).arg(c.accent_dim.alpha() / 255.0))
        .arg(c.text_primary.name())
        .arg(c.accent.name());

        btn->setStyleSheet(btnStyle);
        filters_->addWidget(btn);
        filter_btns_.push_back(btn);
        connect(btn, &QPushButton::clicked, this, [this, name]() { emit filter_changed(name); });
    }
    filters_->addStretch(1);
    root->addLayout(filters_);

    auto *scroll = new QScrollArea(this);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setStyleSheet("background: transparent; border: none;");

    auto *inner = new QWidget();
    inner->setStyleSheet("background: transparent;");
    results_ = new QVBoxLayout(inner);
    results_->setSpacing(6);
    results_->setContentsMargins(0, 4, 0, 4);

    auto *placeholder = new QLabel("Escribe algo para buscar", inner);
    placeholder->setFont(DesignTokens::getFont("body", 14));
    placeholder->setStyleSheet(QString("color: %1; padding: 24px;").arg(c.text_muted.name()));
    placeholder->setAlignment(Qt::AlignCenter);
    results_->addWidget(placeholder);
    results_->addStretch(1);

    scroll->setWidget(inner);
    root->addWidget(scroll, 1);
    setStyleSheet("background: transparent;");
}

void SearchView::set_query(const std::string &query) {
    header_->setText("Resultados para \"" + QString::fromStdString(query) + "\"");
}

static void clear_layout(QVBoxLayout *lay) {
    while (lay->count() > 0) {
        auto *item = lay->takeAt(0);
        if (item->widget()) {
            item->widget()->deleteLater();
        }
        delete item;
    }
}

void SearchView::show_results(const std::vector<Track> &songs,
                              const std::vector<Artist> &artists,
                              const std::vector<Album> &albums) {
    showing_recent_ = false;
    clear_layout(results_);

    const auto &c = DesignTokens::current();

    bool has_any = !songs.empty() || !artists.empty() || !albums.empty();
    if (!has_any) {
        auto *empty = new QLabel("Sin resultados", this);
        empty->setFont(DesignTokens::getFont("body", 14));
        empty->setStyleSheet(QString("color: %1; padding: 24px;").arg(c.text_muted.name()));
        empty->setAlignment(Qt::AlignCenter);
        results_->addWidget(empty);
        results_->addStretch(1);
        return;
    }

    if (!songs.empty()) {
        auto *sec_header = new QLabel("Canciones", this);
        sec_header->setFont(DesignTokens::getFont("micro", 11));
        sec_header->setStyleSheet(QString("color: %1; text-transform: uppercase; padding: 12px 12px 6px;")
            .arg(c.accent.name()));
        results_->addWidget(sec_header);
        for (const auto &track : songs) {
            auto *ci = new ClickableItem(static_cast<std::string>(track.title), static_cast<std::string>(track.artist), this);
            ci->set_item_id(static_cast<std::string>(track.id));
            results_->addWidget(ci);
            
            connect(ci, &ClickableItem::clicked, this, [this, track]() {
                emit play_requested(track);
            });
            connect(ci, &ClickableItem::context_action, this, [this, track](const std::string &action, const std::string &) {
                if (action == "add_favorite") {
                    emit add_favorite_requested(track);
                } else if (action == "download") {
                    emit download_requested(track);
                } else if (action == "queue_next") {
                    emit add_to_queue_next_requested(track);
                } else if (action == "queue_end") {
                    emit add_to_queue_end_requested(track);
                }
            });
        }
    }

    if (!artists.empty()) {
        auto *sec_header = new QLabel("Artistas", this);
        sec_header->setFont(DesignTokens::getFont("micro", 11));
        sec_header->setStyleSheet(QString("color: %1; text-transform: uppercase; padding: 12px 12px 6px;")
            .arg(c.accent.name()));
        results_->addWidget(sec_header);
        for (const auto &artist : artists) {
            auto *ci = new ClickableItem(static_cast<std::string>(artist.name), "", this);
            ci->set_item_id(static_cast<std::string>(artist.id));
            results_->addWidget(ci);
        }
    }

    if (!albums.empty()) {
        auto *sec_header = new QLabel("Álbumes", this);
        sec_header->setFont(DesignTokens::getFont("micro", 11));
        sec_header->setStyleSheet(QString("color: %1; text-transform: uppercase; padding: 12px 12px 6px;")
            .arg(c.accent.name()));
        results_->addWidget(sec_header);
        for (const auto &album : albums) {
            auto *ci = new ClickableItem(static_cast<std::string>(album.title), static_cast<std::string>(album.artist), this);
            ci->set_item_id(static_cast<std::string>(album.id));
            results_->addWidget(ci);
        }
    }

    results_->addStretch(1);
}

void SearchView::show_recent_searches(const std::vector<std::string> &queries) {
    showing_recent_ = true;
    clear_layout(results_);

    const auto &c = DesignTokens::current();

    if (queries.empty()) {
        auto *empty = new QLabel("Sin búsquedas recientes", this);
        empty->setFont(DesignTokens::getFont("body", 14));
        empty->setStyleSheet(QString("color: %1; padding: 24px;").arg(c.text_muted.name()));
        empty->setAlignment(Qt::AlignCenter);
        results_->addWidget(empty);
        results_->addStretch(1);
        return;
    }

    auto *sec_header = new QLabel("Búsquedas recientes", this);
    sec_header->setFont(DesignTokens::getFont("micro", 11));
    sec_header->setStyleSheet(QString("color: %1; text-transform: uppercase; padding: 12px 12px 6px;")
        .arg(c.accent.name()));
    results_->addWidget(sec_header);

    for (const auto &q : queries) {
        auto *btn = new QPushButton(this);
        btn->setFixedHeight(40);
        btn->setCursor(Qt::PointingHandCursor);
        
        auto *btn_layout = new QHBoxLayout(btn);
        btn_layout->setContentsMargins(12, 0, 12, 0);
        btn_layout->setSpacing(8);
        
        auto *history_icon = IconProvider::createIconLabel("history", 16, c.text_secondary, true, btn);
        auto *text_label = new QLabel(QString::fromStdString(q), btn);
        text_label->setFont(DesignTokens::getFont("body", 13));
        text_label->setStyleSheet("color: inherit; background: transparent;");
        
        btn_layout->addWidget(history_icon);
        btn_layout->addWidget(text_label);
        btn_layout->addStretch();
        btn->setLayout(btn_layout);
 
        QString btnStyle = QString(
            "QPushButton {\n"
            "    background: transparent;\n"
            "    border: none;\n"
            "    border-radius: 6px;\n"
            "    color: %1;\n"
            "}\n"
            "QPushButton:hover {\n"
            "    background-color: %2;\n"
            "    color: %3;\n"
            "}\n"
        )
        .arg(c.text_secondary.name())
        .arg(QString("rgba(%1, %2, %3, %4)").arg(c.accent_dim.red()).arg(c.accent_dim.green()).arg(c.accent_dim.blue()).arg(c.accent_dim.alpha() / 255.0))
        .arg(c.text_primary.name());

        btn->setStyleSheet(btnStyle);
        results_->addWidget(btn);
        connect(btn, &QPushButton::clicked, this, [this, q]() {
            emit search_requested(q);
        });
    }
    results_->addStretch(1);
}

void SearchView::set_results(const std::vector<Track> &songs,
                             const std::vector<Artist> &artists,
                             const std::vector<Album> &albums) {
    show_results(songs, artists, albums);
}

void SearchView::set_recent_searches(const std::vector<std::string> &queries) {
    show_recent_searches(queries);
}

void SearchView::set_active_filter(const std::string &filter) {
    for (auto *btn : filter_btns_) {
        btn->setChecked(btn->text().toStdString() == filter);
    }
}
