#include <QFrame>
#include <QLabel>
#include <QPushButton>
#include "home_view.h"
#include "design_tokens.h"
#include "components/song_card.h"
#include "components/album_card.h"
#include "components/artist_card.h"
#include "components/horizontal_carousel.h"
#include "components/skeleton_loader.h"

HomeView::HomeView(QWidget *parent)
    : QWidget(parent)
{
    const auto &c = DesignTokens::current();

    setStyleSheet("background: transparent;");
    content_ = new QVBoxLayout(this);
    content_->setContentsMargins(24, 24, 24, 24);
    content_->setSpacing(28);

    welcome_label_ = new QLabel("¡Bienvenido a Doremi!", this);
    welcome_label_->setObjectName("homeWelcome");
    welcome_label_->setFont(DesignTokens::getFont("display", 24));
    welcome_label_->setStyleSheet(QString("font-weight: bold; color: %1; background: transparent;")
        .arg(c.text_primary.name()));
    content_->addWidget(welcome_label_);

    // Sections are added dynamically via add_section() from Rust.
    // Start with a loading skeleton so the view is never blank on first load.
    set_state("loading", "");

    content_->addStretch(1);
}

void HomeView::set_welcome_message(const std::string &msg) {
    if (welcome_label_) {
        welcome_label_->setText(QString::fromStdString(msg));
    }
}

QWidget *HomeView::add_section_widget(const std::string &title,
                                       const std::vector<HomeCard> &items) {
    const auto &c = DesignTokens::current();

    auto *section = new QWidget(this);
    auto *lay = new QVBoxLayout(section);
    lay->setContentsMargins(0, 0, 0, 0);
    lay->setSpacing(10);

    auto *header = new QLabel(QString::fromStdString(title), section);
    header->setObjectName("sectionHeader");
    header->setFont(DesignTokens::getFont("heading_sm", 16));
    header->setStyleSheet(QString("color: %1; font-weight: bold; background: transparent;")
        .arg(c.text_primary.name()));
    lay->addWidget(header);

    // Modern horizontal carousel for cards
    auto *carousel = new HorizontalCarousel(section);
    
    for (const auto &item : items) {
        const std::string id = static_cast<std::string>(item.id);
        const std::string titleValue = static_cast<std::string>(item.title);
        const std::string subtitleValue = static_cast<std::string>(item.subtitle);
        const std::string thumbnailValue = static_cast<std::string>(item.thumbnail);
        const std::string itemType = static_cast<std::string>(item.item_type);
        const QString itemTitle = QString::fromStdString(titleValue);
        const QString subtitle = QString::fromStdString(subtitleValue);
        const QString thumbnail = QString::fromStdString(thumbnailValue);
        auto playTrack = [this, id, titleValue, subtitleValue, thumbnailValue]() {
            Track track;
            track.id = id;
            track.title = titleValue;
            track.artist = subtitleValue;
            track.thumbnail = thumbnailValue;
            emit play_requested(track);
        };
        auto openTypedItem = [this, id, itemType]() {
            if (id.empty()) return;
            if (itemType == "album") emit album_requested(id);
            else if (itemType == "artist") emit artist_requested(id);
            else if (itemType == "show") emit show_requested(id);
            else emit playlist_requested(id);
        };

        if (itemType == "song") {
            auto *card = new SongCard(itemTitle, subtitle, thumbnail, carousel);
            card->setItemId(id);
            connect(card, &SongCard::playRequested, this, [playTrack](const std::string &) { playTrack(); });
            connect(card, &SongCard::clicked, card, [card]() {
                emit card->playRequested(card->itemId());
            });
            carousel->addWidget(card);
        } else if (itemType == "artist") {
            auto *card = new ArtistCard(itemTitle, thumbnail, carousel);
            card->setItemId(id);
            connect(card, &ArtistCard::clicked, this, [this, id]() {
                emit artist_requested(id);
            });
            carousel->addWidget(card);
        } else {
            auto *card = new AlbumCard(itemTitle, subtitle, thumbnail, carousel);
            card->setItemId(id);
            card->setContentType(QString::fromStdString(itemType));
            if (itemType == "episode") {
                connect(card, &AlbumCard::clicked, this, [playTrack]() { playTrack(); });
                connect(card, &AlbumCard::playRequested, this, [playTrack](const std::string &) { playTrack(); });
            } else {
                connect(card, &AlbumCard::clicked, this, [openTypedItem]() { openTypedItem(); });
                connect(card, &AlbumCard::playRequested, this, [openTypedItem](const std::string &) { openTypedItem(); });
            }
            carousel->addWidget(card);
        }
    }
    
    lay->addWidget(carousel);
    return section;
}

void HomeView::add_section(const std::string &title,
                            const std::vector<HomeCard> &items) {
    if (state_widget_) {
        state_widget_->deleteLater();
        state_widget_ = nullptr;
    }
    // Insert section before the stretch at the end
    content_->insertWidget(content_->count() - 1, add_section_widget(title, items));
}

void HomeView::clear_sections() {
    // Keep only the welcome label (index 0) and stretch (last item)
    while (content_->count() > 2) {
        auto *item = content_->takeAt(1);
        if (item->widget()) {
            item->widget()->deleteLater();
        }
        delete item;
    }
    state_widget_ = nullptr;
}

void HomeView::set_state(const std::string &state, const std::string &message) {
    if (state == "content" || state == "success") {
        if (state_widget_) state_widget_->deleteLater();
        state_widget_ = nullptr;
        return;
    }
    clear_sections();
    const auto &c = DesignTokens::current();
    state_widget_ = new QWidget(this);
    auto *layout = new QVBoxLayout(state_widget_);
    layout->setContentsMargins(0, 16, 0, 16);
    layout->setSpacing(12);

    if (state == "loading") {
        for (int row = 0; row < 2; ++row) {
            auto *line = new QHBoxLayout();
            line->setSpacing(12);
            for (int column = 0; column < 5; ++column) {
                auto *skeleton = new SkeletonLoader(state_widget_);
                skeleton->setFixedSize(160, 210);
                line->addWidget(skeleton);
            }
            line->addStretch();
            layout->addLayout(line);
        }
    } else {
        auto *label = new QLabel(QString::fromStdString(message), state_widget_);
        label->setObjectName("stateMessage");
        label->setAlignment(Qt::AlignCenter);
        label->setWordWrap(true);
        label->setFont(DesignTokens::getFont("body", 13));
        label->setStyleSheet(QString("color: %1; padding: 36px;").arg(c.text_muted.name()));
        layout->addWidget(label);
        if (state == "error") {
            auto *retry = new QPushButton("Reintentar", state_widget_);
            retry->setCursor(Qt::PointingHandCursor);
            retry->setFixedWidth(140);
            retry->setStyleSheet(QString(
                "QPushButton { background: %1; color: white; border: none; border-radius: 8px; padding: 10px; }"
                "QPushButton:hover { background: %2; }")
                .arg(c.accent.name(), c.accent_bright.name()));
            connect(retry, &QPushButton::clicked, this, &HomeView::retry_requested);
            layout->addWidget(retry, 0, Qt::AlignHCenter);
        }
    }
    content_->insertWidget(1, state_widget_);
}

void HomeView::update_theme() {
    const auto &c = DesignTokens::current();
    if (welcome_label_) {
        welcome_label_->setStyleSheet(QString("font-weight: bold; color: %1; background: transparent;")
            .arg(c.text_primary.name()));
    }
    for (auto *label : findChildren<QLabel *>()) {
        if (label->objectName() == "sectionHeader") {
            label->setStyleSheet(QString("color: %1; font-weight: bold; background: transparent;")
                .arg(c.text_primary.name()));
        } else if (label->objectName() == "stateMessage") {
            label->setStyleSheet(QString("color: %1; padding: 36px;").arg(c.text_muted.name()));
        }
    }
}
