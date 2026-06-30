#include "show_detail_view.h"
#include "design_tokens.h"
#include "components/loading_state.h"
#include "components/empty_state.h"
#include <QScrollArea>
#include <QStyle>
#include <QDateTime>
#include <cmath>


ShowDetailView::ShowDetailView(QWidget *parent)
    : QWidget(parent) {
    setupLayout();
}

void ShowDetailView::setupLayout() {
    auto *outer = new QVBoxLayout(this);
    outer->setContentsMargins(0, 0, 0, 0);

    auto *scroll = new QScrollArea;
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);

    auto *container = new QWidget;
    container->setProperty("bgRole", "transparent");
    content_layout_ = new QVBoxLayout(container);
    content_layout_->setContentsMargins(DesignTokens::pagePadding());
    content_layout_->setSpacing(16);

    // Header row: cover + info
    auto *header = new QHBoxLayout;
    header->setSpacing(20);

    cover_label_ = new QLabel;
    cover_label_->setFixedSize(200, 200);
    cover_label_->setStyleSheet(QString("background: %1; border-radius: %2px;").arg(DesignTokens::current().bg_elevated.name()).arg(DesignTokens::radius().md));
    cover_label_->setAlignment(Qt::AlignCenter);
    header->addWidget(cover_label_);

    auto *header_info = new QVBoxLayout;
    header_info->setSpacing(8);

    auto *back_btn = new QPushButton("← Volver");
    back_btn->setObjectName("backBtn");
    back_btn->setCursor(Qt::PointingHandCursor);
    back_btn->setObjectName("backBtn");
    QObject::connect(back_btn, &QPushButton::clicked, this, &ShowDetailView::back_requested);
    header_info->addWidget(back_btn);

    title_label_ = new QLabel;
    title_label_->setProperty("textRole", "heading");
    title_label_->setObjectName("showTitle");
    title_label_->setWordWrap(true);
    header_info->addWidget(title_label_);

    author_label_ = new QLabel;
    author_label_->setProperty("textRole", "secondary");
    header_info->addWidget(author_label_);

    episode_count_label_ = new QLabel;
    episode_count_label_->setProperty("textRole", "muted");
    header_info->addWidget(episode_count_label_);

    description_label_ = new QLabel;
    description_label_->setProperty("textRole", "secondary");
    description_label_->setWordWrap(true);
    header_info->addWidget(description_label_);

    subscribe_btn_ = new QPushButton(this);
    subscribe_btn_->setCursor(Qt::PointingHandCursor);
    subscribe_btn_->setFixedHeight(36);
    QObject::connect(subscribe_btn_, &QPushButton::clicked, this, [this]() {
        if (is_subscribed_) {
            on_remove_favorite_show(static_cast<std::string>(current_show_.id));
            updateSubscriptionButtonState(false);
        } else {
            on_add_favorite_show(current_show_);
            updateSubscriptionButtonState(true);
        }
    });
    header_info->addWidget(subscribe_btn_, 0, Qt::AlignLeft);
    updateSubscriptionButtonState(false);

    header_info->addStretch();
    header->addLayout(header_info, 1);
    content_layout_->addLayout(header);

    // Episodes section
    auto *ep_header = new QLabel("Episodios");
    ep_header->setObjectName("episodesHeader");
    ep_header->setProperty("textRole", "heading");
    content_layout_->addWidget(ep_header);

    episodes_widget_ = new QWidget;
    episodes_widget_->setProperty("bgRole", "transparent");
    episodes_layout_ = new QVBoxLayout(episodes_widget_);
    episodes_layout_->setContentsMargins(0, 0, 0, 0);
    episodes_layout_->setSpacing(2);
    content_layout_->addWidget(episodes_widget_);

    content_layout_->addStretch();

    scroll->setWidget(container);
    outer->addWidget(scroll);
}

void ShowDetailView::set_show_info(const Show &show) {
    const auto &c = DesignTokens::current();
    current_show_ = show;
    
    title_label_->setText(QString::fromStdString(static_cast<std::string>(show.title)));
    author_label_->setText(QString::fromStdString(static_cast<std::string>(show.author)));
    if (!static_cast<std::string>(show.description).empty()) {
        description_label_->setText(QString::fromStdString(static_cast<std::string>(show.description)));
        description_label_->show();
    } else {
        description_label_->hide();
    }
    if (show.episode_count > 0) {
        episode_count_label_->setText(
            QString::number(show.episode_count) + " episodios");
        episode_count_label_->show();
    } else {
        episode_count_label_->hide();
    }
    QString thumb = QString::fromStdString(static_cast<std::string>(show.thumbnail));
    if (!thumb.isEmpty()) {
        QPixmap pix;
        if (pix.loadFromData(QByteArray::fromBase64(thumb.toUtf8()))) {
            cover_label_->setPixmap(pix.scaled(200, 200, Qt::KeepAspectRatio, Qt::SmoothTransformation));
        } else {
            cover_label_->setText(QString::fromUtf8("🎙️"));
            cover_label_->setStyleSheet(QString("font-size: 64px; background: %1; border-radius: %2px;").arg(c.bg_elevated.name()).arg(DesignTokens::radius().md));
        }
    }

    bool is_fav = get_show_favorite_state(static_cast<std::string>(show.id));
    updateSubscriptionButtonState(is_fav);
}

void ShowDetailView::set_episodes(const std::vector<Episode> &episodes) {
    QLayoutItem *child;
    while ((child = episodes_layout_->takeAt(0)) != nullptr) {
        delete child->widget();
        delete child;
    }
    episodes_ = episodes;

    for (const auto &ep : episodes) {
        auto dur_ms = ep.duration_ms;
        int mins = static_cast<int>(dur_ms / 60000);
        int secs = static_cast<int>((dur_ms % 60000) / 1000);
        QString dur_str = QString("%1:%2").arg(mins).arg(secs, 2, 10, QChar('0'));

        auto *row = new EpisodeRow(
            QString::fromStdString(static_cast<std::string>(ep.title)),
            QString::fromStdString(static_cast<std::string>(ep.description)),
            dur_str,
            ep,
            episodes_widget_);
        QObject::connect(row, &EpisodeRow::play_requested,
                         this, &ShowDetailView::play_episode_requested);
        episodes_layout_->addWidget(row);
    }

    if (episodes.empty()) {
        auto *empty = new EmptyState(episodes_widget_);
        empty->setIcon("podcast");
        empty->setTitle(tr_q("empty_podcast_title"));
        empty->setDescription(tr_q("empty_podcast_desc"));
        empty->applyPanelStyle("empty");
        episodes_layout_->addWidget(empty);
    }
    updateGeometry();
}

void ShowDetailView::clear() {
    title_label_->clear();
    author_label_->clear();
    description_label_->clear();
    episode_count_label_->clear();
    cover_label_->clear();
    cover_label_->setStyleSheet(QString("background: %1; border-radius: %2px;").arg(DesignTokens::current().bg_elevated.name()).arg(DesignTokens::radius().md));
    updateSubscriptionButtonState(false);
    QLayoutItem *child;
    while ((child = episodes_layout_->takeAt(0)) != nullptr) {
        delete child->widget();
        delete child;
    }
    episodes_.clear();

    auto *loading = new LoadingState(LoadingState::ListRows, episodes_widget_);
    loading->setRowCount(4);
    loading->setRowHeight(72);
    episodes_layout_->addWidget(loading);
}

void ShowDetailView::updateSubscriptionButtonState(bool subscribed) {
    is_subscribed_ = subscribed;
    if (subscribed) {
        subscribe_btn_->setText(tr_q("unsubscribe"));
        subscribe_btn_->setObjectName("subscribedBtn");
    } else {
        subscribe_btn_->setText(tr_q("subscribe"));
        subscribe_btn_->setProperty("buttonRole", "primary");
    }
}

bool ShowDetailView::eventFilter(QObject *obj, QEvent *event) {
    return QWidget::eventFilter(obj, event);
}

void ShowDetailView::update_theme() {
    updateSubscriptionButtonState(is_subscribed_);
    for (auto *row : findChildren<EpisodeRow*>()) {
        row->update_theme();
    }
}
