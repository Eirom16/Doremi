#include <QHBoxLayout>
#include <QKeyEvent>
#include "title_bar.h"
#include "design_tokens.h"
#include "icon_provider.h"

TitleBar::TitleBar(QWidget *parent)
    : QWidget(parent)
{
    const auto &c = DesignTokens::current();

    auto *layout = new QHBoxLayout(this);
    layout->setContentsMargins(16, 6, 16, 6);
    layout->setSpacing(12);

    auto *logo = new QLabel("Doremi", this);
    logo->setFont(DesignTokens::getFont("heading_sm", 16));
    logo->setStyleSheet(QString("font-weight: bold; color: %1; background: transparent;").arg(c.accent.name()));
    
    auto *logo_icon = IconProvider::createIconLabel("music_note", 18, c.accent, true, this);
    
    auto *logo_layout = new QHBoxLayout();
    logo_layout->setContentsMargins(0, 0, 0, 0);
    logo_layout->setSpacing(6);
    logo_layout->addWidget(logo_icon);
    logo_layout->addWidget(logo);
    
    layout->addLayout(logo_layout);
    layout->addSpacing(24);

    search_input_ = new QLineEdit(this);
    search_input_->setPlaceholderText("Buscar canciones, artistas, álbumes...");
    search_input_->setFont(DesignTokens::getFont("body", 13));
    search_suggestions_model_ = new QStringListModel(this);
    search_completer_ = new QCompleter(search_suggestions_model_, this);
    search_completer_->setCaseSensitivity(Qt::CaseInsensitive);
    search_completer_->setCompletionMode(QCompleter::PopupCompletion);
    search_completer_->setMaxVisibleItems(8);
    search_input_->setCompleter(search_completer_);
    
    search_input_->addAction(IconProvider::getIcon("search", c.text_secondary, 18), QLineEdit::LeadingPosition);
    
    QString search_style = QString(
        "QLineEdit {\n"
        "    background-color: %1;\n"
        "    border: 1px solid %2;\n"
        "    border-radius: 8px;\n"
        "    padding: 6px 12px 6px 36px;\n"
        "    color: %3;\n"
        "}\n"
        "QLineEdit:hover {\n"
        "    border-color: %4;\n"
        "}\n"
        "QLineEdit:focus {\n"
        "    border-color: %5;\n"
        "    background-color: %6;\n"
        "}\n"
    )
    .arg(c.bg_base.name())
    .arg(c.border.name())
    .arg(c.text_primary.name())
    .arg(c.text_secondary.name())
    .arg(c.accent.name())
    .arg(c.bg_elevated.name());
    
    search_input_->setStyleSheet(search_style);
    layout->addWidget(search_input_, 1);

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

    setStyleSheet(QString("background-color: %1; border-bottom: 1px solid %2;")
        .arg(c.bg_surface.name())
        .arg(QString("rgba(%1, %2, %3, %4)").arg(c.border.red()).arg(c.border.green()).arg(c.border.blue()).arg(c.border.alpha() / 255.0))
    );
    setFixedHeight(48);
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

void TitleBar::update_theme() {
    const auto &c = DesignTokens::current();
    
    setStyleSheet(QString("background-color: %1; border-bottom: 1px solid %2;")
        .arg(c.bg_surface.name())
        .arg(QString("rgba(%1, %2, %3, %4)").arg(c.border.red()).arg(c.border.green()).arg(c.border.blue()).arg(c.border.alpha() / 255.0))
    );
    
    auto *logo = findChild<QLabel*>();
    if (logo && logo->text() == "Doremi") {
        logo->setStyleSheet(QString("font-weight: bold; color: %1; background: transparent;").arg(c.accent.name()));
    }
    
    if (search_input_) {
        QString search_style = QString(
            "QLineEdit {\n"
            "    background-color: %1;\n"
            "    border: 1px solid %2;\n"
            "    border-radius: 8px;\n"
            "    padding: 6px 12px 6px 36px;\n"
            "    color: %3;\n"
            "}\n"
            "QLineEdit:hover {\n"
            "    border-color: %4;\n"
            "}\n"
            "QLineEdit:focus {\n"
            "    border-color: %5;\n"
            "    background-color: %6;\n"
            "}\n"
        )
        .arg(c.bg_base.name())
        .arg(c.border.name())
        .arg(c.text_primary.name())
        .arg(c.text_secondary.name())
        .arg(c.accent.name())
        .arg(c.bg_elevated.name());
        
        search_input_->setStyleSheet(search_style);
    }
}
