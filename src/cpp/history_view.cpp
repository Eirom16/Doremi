#include "history_view.h"
#include <QtGlobal>
#include "design_tokens.h"
#include "icon_provider.h"
#include "components/artwork_loader.h"
#include <QHBoxLayout>
#include <QMouseEvent>
#include <QDateTime>
#include <QTimeZone>
#include <QPushButton>
#include <QMessageBox>
#include "doremi/src/bridge.rs.h"

HistoryView::HistoryView(QWidget *parent)
    : QWidget(parent)
{
    setupLayout();
}

void HistoryView::setupLayout() {
    const auto &c = DesignTokens::current();

    auto *main_vbox = new QVBoxLayout(this);
    main_vbox->setContentsMargins(0, 0, 0, 0);
    main_vbox->setSpacing(0);

    content_layout_ = new QVBoxLayout();
    content_layout_->setContentsMargins(DesignTokens::pagePadding());
    content_layout_->setSpacing(4);
    content_layout_->setAlignment(Qt::AlignTop);

    auto *header_layout = new QHBoxLayout();
    auto *title = new QLabel(tr_q("history"), this);
    title->setObjectName("historyTitle");
    title->setFont(DesignTokens::getFont("heading_lg"));
    title->setStyleSheet(QString("color: %1; font-weight: bold;").arg(c.text_primary.name()));

    auto *clear_btn = new QPushButton(tr_q("clear_history"), this);
    clear_btn->setObjectName("historyClearBtn");
    clear_btn->setCursor(Qt::PointingHandCursor);
    clear_btn->setStyleSheet(QString(
        "QPushButton { background: transparent; border: 1px solid %1; border-radius: %6px; padding: 0 16px; color: %2; font-size: 12px; }"
        "QPushButton:hover { background: rgba(%3, %4, %5, 0.08); }")
        .arg(c.border.name()).arg(c.text_secondary.name())
        .arg(c.text_primary.red()).arg(c.text_primary.green()).arg(c.text_primary.blue())
        .arg(DesignTokens::radius().pill));
    connect(clear_btn, &QPushButton::clicked, this, [this]() {
        auto reply = QMessageBox::question(this, tr_q("clear_history"),
            tr_q("confirm_clear_history"),
            QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
        if (reply == QMessageBox::Yes) {
            on_clear_history();
        }
    });

    header_layout->addWidget(title);
    header_layout->addStretch();
    header_layout->addWidget(clear_btn);
    content_layout_->addLayout(header_layout);

    auto *subtitle = new QLabel(tr_q("recently_played"), this);
    subtitle->setObjectName("historySubtitle");
    subtitle->setFont(DesignTokens::getFont("caption", 12));
    subtitle->setStyleSheet(QString("color: %1; margin-bottom: 12px;").arg(c.text_secondary.name()));
    content_layout_->addWidget(subtitle);

    empty_label_ = new EmptyState(this);
    empty_label_->setObjectName("historyEmptyLabel");
    empty_label_->setIcon("history");
    empty_label_->setTitle(tr_q("history_empty_desc"));
    empty_label_->applyPanelStyle("empty");
    empty_label_->hide();
    content_layout_->addWidget(empty_label_);

    main_vbox->addLayout(content_layout_);
    setLayout(main_vbox);
}

void HistoryView::showEvent(QShowEvent *event) {
    QWidget::showEvent(event);
    if (!qEnvironmentVariableIsSet("DOREMI_UI_TEST")) {
        on_history_requested();
    }
}

QString HistoryView::getGroupLabel(const QString &played_at) const {
    QDateTime dt = QDateTime::fromString(played_at, Qt::ISODate);
    if (!dt.isValid()) {
        dt = QDateTime::fromString(played_at, "yyyy-MM-dd HH:mm:ss");
    }
    if (!dt.isValid()) return tr_q("other");

    QDate today = QDate::currentDate();
    QDate play_date = dt.date();

    if (play_date == today) return tr_q("today");
    if (play_date == today.addDays(-1)) return tr_q("yesterday");
    if (play_date >= today.addDays(-7)) return tr_q("this_week");
    return tr_q("older");
}

void HistoryView::clear_history() {
    while (content_layout_->count() > 2) {
        QLayoutItem *item = content_layout_->takeAt(2);
        if (item->widget()) {
            item->widget()->deleteLater();
        }
        delete item;
    }
}

void HistoryView::set_history(const std::vector<Track> &tracks,
                              const std::vector<std::string> &played_at,
                              const std::vector<std::string> &feedback_tokens) {
    clear_history();

    const auto &c = DesignTokens::current();

    if (tracks.empty()) {
        empty_label_ = new EmptyState(this);
        empty_label_->setObjectName("historyEmptyLabel");
        empty_label_->setIcon("history");
        empty_label_->setTitle(tr_q("history_empty_desc"));
        empty_label_->applyPanelStyle("empty");
        content_layout_->addWidget(empty_label_);
        return;
    }

    size_t n = std::min({tracks.size(), played_at.size(), feedback_tokens.size()});

    QString last_group;
    for (size_t i = 0; i < n; ++i) {
        const auto &t = tracks[i];
        const auto &pa = played_at[i];
        const auto &ft = feedback_tokens[i];

        QString group = getGroupLabel(QString::fromStdString(pa));
        if (group != last_group) {
            last_group = group;
            auto *group_lbl = new QLabel(group, this);
            group_lbl->setFont(DesignTokens::getFont("heading_sm", 13));
            group_lbl->setStyleSheet(QString("color: %1; font-weight: bold; margin-top: 16px; margin-bottom: 4px;")
                .arg(c.accent.name()));
            content_layout_->addWidget(group_lbl);
        }

        auto *row = new HistoryRow(t, pa, ft, this);
        connect(row, &HistoryRow::play_requested, this, &HistoryView::play_requested);
        connect(row, &HistoryRow::delete_requested, this, [](const std::string &track_id, const std::string &feedback_token) {
            on_delete_history_item(track_id, feedback_token);
        });
        content_layout_->addWidget(row);
    }

    content_layout_->addStretch();
}

void HistoryView::update_theme() {
    const auto &c = DesignTokens::current();
    if (auto *title = findChild<QLabel*>("historyTitle")) {
        title->setStyleSheet(QString("color: %1; font-weight: bold;").arg(c.text_primary.name()));
    }
    if (auto *clear_btn = findChild<QPushButton*>("historyClearBtn")) {
        clear_btn->setStyleSheet(QString(
            "QPushButton { background: transparent; border: 1px solid %1; border-radius: %6px; padding: 0 16px; color: %2; font-size: 12px; }"
            "QPushButton:hover { background: rgba(%3, %4, %5, 0.08); }")
            .arg(c.border.name()).arg(c.text_secondary.name())
            .arg(c.text_primary.red()).arg(c.text_primary.green()).arg(c.text_primary.blue())
            .arg(DesignTokens::radius().pill));
    }
    if (auto *subtitle = findChild<QLabel*>("historySubtitle")) {
        subtitle->setStyleSheet(QString("color: %1; margin-bottom: 12px;").arg(c.text_secondary.name()));
    }
    if (auto *empty = findChild<EmptyState*>("historyEmptyLabel")) {
        empty->applyPanelStyle("empty");
    }
    for (auto *row : findChildren<HistoryRow*>()) {
        row->update_theme();
    }
}
