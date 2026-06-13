#include <QScrollArea>
#include <QFrame>
#include <QLabel>
#include "home_view.h"
#include "design_tokens.h"
#include "components/song_card.h"
#include "components/horizontal_carousel.h"

HomeView::HomeView(QWidget *parent)
    : QWidget(parent)
{
    const auto &c = DesignTokens::current();

    auto *scroll = new QScrollArea(this);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setStyleSheet("background: transparent; border: none;");

    auto *inner = new QWidget();
    inner->setStyleSheet("background: transparent;");
    content_ = new QVBoxLayout(inner);
    content_->setContentsMargins(24, 24, 24, 24);
    content_->setSpacing(28);

    auto *welcome = new QLabel("¡Bienvenido a Doremi!", inner);
    welcome->setFont(DesignTokens::getFont("display", 24));
    welcome->setStyleSheet(QString("font-weight: bold; color: %1; background: transparent;")
        .arg(c.text_primary.name()));
    content_->addWidget(welcome);

    // Sections are added dynamically via add_section() from Rust
    content_->addStretch(1);
    scroll->setWidget(inner);

    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->addWidget(scroll);
    setStyleSheet("background: transparent;");
}

void HomeView::set_welcome_message(const std::string &msg) {
    auto *label = qobject_cast<QLabel *>(content_->itemAt(0)->widget());
    if (label) {
        label->setText(QString::fromStdString(msg));
    }
}

QWidget *HomeView::add_section_widget(const std::string &title,
                                       const std::vector<std::string> &items) {
    const auto &c = DesignTokens::current();

    auto *section = new QWidget(this);
    auto *lay = new QVBoxLayout(section);
    lay->setContentsMargins(0, 0, 0, 0);
    lay->setSpacing(10);

    auto *header = new QLabel(QString::fromStdString(title), section);
    header->setFont(DesignTokens::getFont("heading_sm", 16));
    header->setStyleSheet(QString("color: %1; font-weight: bold; background: transparent;")
        .arg(c.text_primary.name()));
    lay->addWidget(header);

    // Modern horizontal carousel for cards
    auto *carousel = new HorizontalCarousel(section);
    
    for (const auto &item : items) {
        QString itemStr = QString::fromStdString(item);
        QStringList parts = itemStr.split(" — ");
        QString titleStr = parts.size() > 0 ? parts[0] : "";
        QString artistStr = parts.size() > 1 ? parts[1] : "";
        
        auto *card = new SongCard(titleStr, artistStr, "", carousel);
        
        // Connect clicks to perform generic play trigger if needed,
        // but for now, it's just visual cards that can be selected.
        
        carousel->addWidget(card);
    }
    
    lay->addWidget(carousel);
    return section;
}

void HomeView::add_section(const std::string &title,
                            const std::vector<std::string> &items) {
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
}
