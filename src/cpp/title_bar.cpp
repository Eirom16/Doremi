#include <QHBoxLayout>
#include <QKeyEvent>
#include <QEvent>
#include <QApplication>
#include <QGuiApplication>
#include <QScreen>
#include <QMenu>
#include <QAction>
#include <QDesktopServices>
#include <QUrl>
#include "title_bar.h"
#include "design_tokens.h"
#include "icon_provider.h"
#include "widgets.h"

SearchSuggestionsPopup::SearchSuggestionsPopup(QWidget *parent)
    : QWidget(parent)
{
    setWindowFlags(Qt::Popup | Qt::FramelessWindowHint | Qt::NoDropShadowWindowHint);
    setAttribute(Qt::WA_ShowWithoutActivating);
    setAttribute(Qt::WA_TranslucentBackground);
    setFocusPolicy(Qt::NoFocus);
    hide();

    layout_ = new QVBoxLayout(this);
    layout_->setContentsMargins(0, 0, 0, 0);
    layout_->setSpacing(0);

    list_ = new QListWidget(this);
    list_->setFocusPolicy(Qt::NoFocus);
    list_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    list_->setCursor(Qt::PointingHandCursor);
    layout_->addWidget(list_);

    connect(list_, &QListWidget::itemClicked, this, [this](QListWidgetItem *item) {
        emit suggestion_selected(item->data(Qt::UserRole).toString().toStdString());
        hide();
    });
    
    update_theme();
}

void SearchSuggestionsPopup::set_suggestions(const std::vector<std::string> &suggestions) {
    list_->clear();
    const auto &c = DesignTokens::current();
    for (const auto &s : suggestions) {
        auto *item = new QListWidgetItem();
        item->setData(Qt::UserRole, QString::fromStdString(s));
        item->setText("  " + QString::fromStdString(s));
        item->setIcon(IconProvider::getIcon("search", c.text_secondary, 18));
        item->setSizeHint(QSize(list_->width(), 44));
        list_->addItem(item);
    }
    int h = (suggestions.size() * 44) + 2;
    if (h > 400) h = 400;
    setFixedHeight(h);
    if (suggestions.empty() || (suggestions.size() == 1 && suggestions[0].rfind("GLOBAL:", 0) == 0)) {
        hide();
    } else {
        if (!isVisible()) show();
        raise();
    }
}

void SearchSuggestionsPopup::update_theme() {
    const auto &c = DesignTokens::current();
    setStyleSheet(QString(
        "SearchSuggestionsPopup { background: transparent; }"
        "QListWidget {"
        "  background: %1;"
        "  border: 1px solid %2;"
        "  border-radius: 8px;"
        "  outline: none;"
        "  color: %3;"
        "  font-size: 14px;"
        "}"
        "QListWidget::item {"
        "  border-radius: 4px;"
        "  padding: 4px;"
        "}"
        "QListWidget::item:hover {"
        "  background: %4;"
        "}"
        "QListWidget::item:selected {"
        "  background: %4;"
        "}"
    ).arg(c.surface.name(), c.border.name(), c.text_primary.name(), c.surface_raised.name()));
}

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
    search_input_->setPlaceholderText("Buscar...");
    search_input_->setFont(DesignTokens::getFont("body_sm"));
    search_input_->setFocusPolicy(Qt::StrongFocus);
    search_input_->setProperty("inputRole", "search");
    search_input_->installEventFilter(this);
    
    search_action_ = search_input_->addAction(IconProvider::getIcon("search", c.text_secondary, 18), QLineEdit::LeadingPosition);
    layout_->addWidget(search_input_, 1);
    layout_->addSpacing(24);

    profile_btn_ = new RippleButton(this);
    profile_btn_->setFixedSize(40, 40);
    profile_btn_->setCursor(Qt::PointingHandCursor);
    profile_btn_->setToolTip("Perfil");
    layout_->addWidget(profile_btn_);

    profile_menu_ = new QMenu(this);
    
    auto *settings_action = profile_menu_->addAction("Configuración");
    connect(settings_action, &QAction::triggered, this, &TitleBar::open_settings_requested);
    
    auto *help_action = profile_menu_->addAction("Ayuda");
    connect(help_action, &QAction::triggered, this, []() {
        QDesktopServices::openUrl(QUrl("https://github.com/Doremi/Doremi"));
    });

    connect(profile_btn_, &RippleButton::clicked, this, [this]() {
        profile_menu_->exec(profile_btn_->mapToGlobal(QPoint(0, profile_btn_->height())));
    });

    suggestions_popup_ = new SearchSuggestionsPopup(parent ? parent : this);
    connect(suggestions_popup_, &SearchSuggestionsPopup::suggestion_selected, this, [this](const std::string &suggestion) {
        search_input_->setText(QString::fromStdString(suggestion));
        emit search_submitted(suggestion);
    });

    debounce_timer_ = new QTimer(this);
    debounce_timer_->setSingleShot(true);
    debounce_timer_->setInterval(100);
    connect(debounce_timer_, &QTimer::timeout, this, [this]() {
        emit search_text_changed(search_input_->text().toStdString());
    });

    connect(search_input_, &QLineEdit::returnPressed, this, [this]() {
        debounce_timer_->stop();
        suggestions_popup_->hide();
        emit search_submitted(search_input_->text().toStdString());
    });
    connect(search_input_, &QLineEdit::textEdited, this, [this](const QString &text) {
        if (text.isEmpty()) {
            suggestions_popup_->hide();
        } else {
            debounce_timer_->start();
        }
    });

    setFixedHeight(60);
    set_context("home");
}

void TitleBar::set_context(const std::string &context) {
    current_context_ = context;
    if (context == "library") {
        search_input_->setPlaceholderText(tr_q("search_library_placeholder"));
    } else if (context == "downloads") {
        search_input_->setPlaceholderText("Buscar en descargas...");
    } else if (context == "settings") {
        search_input_->setPlaceholderText("Buscar ajustes...");
    } else {
        search_input_->setPlaceholderText("Buscar canciones, artistas, álbumes...");
    }
}

bool TitleBar::eventFilter(QObject *obj, QEvent *event) {
    if (obj == search_input_) {
        if (event->type() == QEvent::FocusOut) {
            // Hide popup when search loses focus, but slight delay to allow click on popup
            QTimer::singleShot(150, this, [this]() {
                if (!suggestions_popup_->isActiveWindow()) {
                    suggestions_popup_->hide();
                }
            });
        } else if (event->type() == QEvent::KeyPress) {
            auto *keyEvent = static_cast<QKeyEvent *>(event);
            if (keyEvent->key() == Qt::Key_Escape) {
                suggestions_popup_->hide();
            }
        }
    }
    return QWidget::eventFilter(obj, event);
}

void TitleBar::resizeEvent(QResizeEvent *event) {
    QWidget::resizeEvent(event);
    update_popup_geometry();
}

void TitleBar::update_popup_geometry() {
    if (suggestions_popup_ && search_input_) {
        QWidget *popup_parent = suggestions_popup_->parentWidget();
        if (popup_parent) {
            QPoint pos = popup_parent->mapFromGlobal(search_input_->mapToGlobal(QPoint(0, search_input_->height() + 4)));
            suggestions_popup_->setFixedWidth(search_input_->width());
            suggestions_popup_->move(pos);
            suggestions_popup_->raise();
        }
    }
}

void TitleBar::set_search_text(const std::string &text) {
    search_input_->setText(QString::fromStdString(text));
}

void TitleBar::set_search_suggestions(const std::string &query, const std::vector<std::string> &suggestions) {
    if (query != search_text()) return;
    update_popup_geometry();
    suggestions_popup_->set_suggestions(suggestions);
}

std::string TitleBar::search_text() const {
    return search_input_->text().toStdString();
}

void TitleBar::focus_search() {
    if (!search_input_) return;
    search_input_->setFocus(Qt::ShortcutFocusReason);
    search_input_->selectAll();
}

void TitleBar::set_sidebar_offset(int width) {
    if (!logo_zone_) return;
    const int logo_zone_width = qMax(44, width - 32);
    logo_zone_->setFixedWidth(logo_zone_width);
    if (logo_label_) {
        logo_label_->setVisible(width >= 150);
    }
}

void TitleBar::update_theme() {
    const auto &c = DesignTokens::current();
    if (logo_icon_) IconProvider::setupIconLabel(logo_icon_, "music_note", 18, c.accent, true);
    if (logo_label_) logo_label_->setStyleSheet(QString("font-weight: bold; color: %1; background: transparent;").arg(c.accent.name()));
    if (search_action_) search_action_->setIcon(IconProvider::getIcon("search", c.text_secondary, 18));
    if (suggestions_popup_) suggestions_popup_->update_theme();
    if (profile_btn_) {
        profile_btn_->setIcon(IconProvider::getIcon("person", c.text_primary, 24));
        profile_btn_->setStyleSheet(QString("background: %1; border-radius: 20px;").arg(c.surface.name()));
    }
    if (profile_menu_) {
        profile_menu_->setStyleSheet(QString(
            "QMenu { background-color: %1; color: %2; border: 1px solid %3; border-radius: 8px; padding: 4px; }"
            "QMenu::item { padding: 8px 24px; border-radius: 4px; }"
            "QMenu::item:selected { background-color: %4; }"
        ).arg(c.surface.name(), c.text_primary.name(), c.border.name(), c.surface_raised.name()));
    }
}
