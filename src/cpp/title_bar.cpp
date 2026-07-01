#include <QHBoxLayout>
#include <QKeyEvent>
#include "title_bar.h"
#include "design_tokens.h"
#include "icon_provider.h"

TitleBar::TitleBar(QWidget *parent)
    : QWidget(parent)
{
    const auto &c = DesignTokens::current();

    layout_ = new QHBoxLayout(this);
    layout_->setContentsMargins(16, 6, 16, 6);
    layout_->setSpacing(12);

    logo_zone_ = new QWidget(this);
    auto *logo_layout = new QHBoxLayout(logo_zone_);
    logo_layout->setContentsMargins(0, 0, 0, 0);
    logo_layout->setSpacing(6);
    logo_icon_ = IconProvider::createIconLabel("music_note", 18, c.accent, true, logo_zone_);
    logo_label_ = new QLabel("Doremi", logo_zone_);
    logo_label_->setFont(DesignTokens::getFont("heading_sm"));
    logo_label_->setStyleSheet(QString("font-weight: bold; color: %1; background: transparent;").arg(c.accent.name()));
    logo_layout->addWidget(logo_icon_);
    logo_layout->addWidget(logo_label_);
    logo_layout->addStretch(1);
    
    layout_->addWidget(logo_zone_);
    layout_->addSpacing(24);

    search_input_ = new QLineEdit(this);
    search_input_->setMinimumWidth(400);
    search_input_->setFixedHeight(40);
    search_input_->setPlaceholderText("Buscar canciones, artistas, álbumes...");
    search_input_->setFont(DesignTokens::getFont("body_sm"));
    search_input_->setFocusPolicy(Qt::StrongFocus);
    DesignTokens::applyAccessible(
        search_input_,
        "Buscar musica",
        "Busca canciones, artistas, albumes y playlists en Doremi.",
        "Buscar canciones, artistas, albumes o playlists (Ctrl+L / Ctrl+K)");
    search_suggestions_model_ = new QStringListModel(this);
    search_completer_ = new QCompleter(search_suggestions_model_, this);
    search_completer_->setCaseSensitivity(Qt::CaseInsensitive);
    search_completer_->setCompletionMode(QCompleter::PopupCompletion);
    search_completer_->setMaxVisibleItems(8);
    search_input_->setCompleter(search_completer_);
    
    search_action_ = search_input_->addAction(IconProvider::getIcon("search", c.text_secondary, 18), QLineEdit::LeadingPosition);
    
    search_input_->setProperty("inputRole", "search");
    layout_->addWidget(search_input_, 1);

    debounce_timer_ = new QTimer(this);
    debounce_timer_->setSingleShot(true);
    debounce_timer_->setInterval(300);
    connect(debounce_timer_, &QTimer::timeout, this, [this]() {
        emit search_text_changed(search_input_->text().toStdString());
    });

    connect(search_input_, &QLineEdit::returnPressed, this, [this]() {
        debounce_timer_->stop();
        emit search_submitted(search_input_->text().toStdString());
    });
    connect(search_input_, &QLineEdit::textEdited, this, [this](const QString &) {
        debounce_timer_->start();
    });

    setFixedHeight(60);
}

void TitleBar::set_search_text(const std::string &text) {
    search_input_->setText(QString::fromStdString(text));
}

void TitleBar::set_search_suggestions(const std::string &query, const std::vector<std::string> &suggestions) {
    if (query != search_text()) {
        return; // Discard stale suggestions
    }
    QStringList values;
    for (const auto &suggestion : suggestions) {
        values.push_back(QString::fromStdString(suggestion));
    }
    search_suggestions_model_->setStringList(values);
}

std::string TitleBar::search_text() const {
    return search_input_->text().toStdString();
}

void TitleBar::focus_search() {
    if (!search_input_) {
        return;
    }
    search_input_->setFocus(Qt::ShortcutFocusReason);
    search_input_->selectAll();
}

void TitleBar::set_sidebar_offset(int width) {
    if (!logo_zone_) {
        return;
    }
    const int logo_zone_width = qMax(44, width - 32);
    logo_zone_->setFixedWidth(logo_zone_width);
    if (logo_label_) {
        logo_label_->setVisible(width >= 150);
    }
}

void TitleBar::update_theme() {
    const auto &c = DesignTokens::current();
    
    if (logo_icon_) {
        IconProvider::setupIconLabel(logo_icon_, "music_note", 18, c.accent, true);
    }
    if (logo_label_) {
        logo_label_->setStyleSheet(QString("font-weight: bold; color: %1; background: transparent;").arg(c.accent.name()));
    }
    if (search_action_) {
        search_action_->setIcon(IconProvider::getIcon("search", c.text_secondary, 18));
    }
}
