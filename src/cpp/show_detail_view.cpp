#include "show_detail_view.h"
#include "design_tokens.h"
#include <QScrollArea>
#include <QStyle>
#include <QDateTime>
#include <cmath>

EpisodeRow::EpisodeRow(const QString &title, const QString &description,
                       const QString &duration, Episode episode,
                       QWidget *parent)
    : QWidget(parent), episode_(std::move(episode)) {
    setFixedHeight(72);
    setCursor(Qt::PointingHandCursor);

    const auto &c = DesignTokens::current();

    auto *row = new QHBoxLayout(this);
    row->setContentsMargins(12, 8, 12, 8);

    auto *info = new QVBoxLayout;
    auto *title_lbl = new QLabel(title);
    title_lbl->setStyleSheet(QString("font-size: 14px; font-weight: 600; color: %1;").arg(c.text_primary.name()));
    title_lbl->setWordWrap(false);
    title_lbl->setFixedHeight(20);

    auto *desc_lbl = new QLabel(description);
    desc_lbl->setStyleSheet(QString("font-size: 12px; color: %1;").arg(c.text_secondary.name()));
    desc_lbl->setWordWrap(false);
    desc_lbl->setFixedHeight(16);

    auto *duration_lbl = new QLabel(duration);
    duration_lbl->setStyleSheet(QString("font-size: 11px; color: %1;").arg(c.text_muted.name()));

    info->addWidget(title_lbl);
    info->addWidget(desc_lbl);
    info->addWidget(duration_lbl);
    row->addLayout(info, 1);
}

void EpisodeRow::mousePressEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton) {
        emit play_requested(episode_);
    }
    QWidget::mousePressEvent(event);
}

void EpisodeRow::enterEvent(QEnterEvent *event) {
    setStyleSheet(QString("background: %1;").arg(DesignTokens::current().bg_elevated.name()));
    QWidget::enterEvent(event);
}

void EpisodeRow::leaveEvent(QEvent *event) {
    setStyleSheet("");
    QWidget::leaveEvent(event);
}

ShowDetailView::ShowDetailView(QWidget *parent)
    : QWidget(parent) {
    setupLayout();
}

void ShowDetailView::setupLayout() {
    const auto &c = DesignTokens::current();

    auto *outer = new QVBoxLayout(this);
    outer->setContentsMargins(0, 0, 0, 0);

    auto *scroll = new QScrollArea;
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setStyleSheet("QScrollArea { background: transparent; border: none; }");

    auto *container = new QWidget;
    container->setStyleSheet("background: transparent;");
    content_layout_ = new QVBoxLayout(container);
    content_layout_->setContentsMargins(24, 24, 24, 24);
    content_layout_->setSpacing(16);

    // Header row: cover + info
    auto *header = new QHBoxLayout;
    header->setSpacing(20);

    cover_label_ = new QLabel;
    cover_label_->setFixedSize(200, 200);
    cover_label_->setStyleSheet(QString("background: %1; border-radius: 8px;").arg(c.bg_elevated.name()));
    cover_label_->setAlignment(Qt::AlignCenter);
    header->addWidget(cover_label_);

    auto *header_info = new QVBoxLayout;
    header_info->setSpacing(8);

    auto *back_btn = new QPushButton("← Volver");
    back_btn->setCursor(Qt::PointingHandCursor);
    back_btn->setStyleSheet(
        QString("QPushButton { background: transparent; color: %1; font-size: 14px; border: none; padding: 4px 0; }"
                "QPushButton:hover { color: %2; }")
            .arg(c.accent.name())
            .arg(c.accent_bright.name()));
    QObject::connect(back_btn, &QPushButton::clicked, this, &ShowDetailView::back_requested);
    header_info->addWidget(back_btn);

    title_label_ = new QLabel;
    title_label_->setStyleSheet(QString("font-size: 24px; font-weight: 700; color: %1;").arg(c.text_primary.name()));
    title_label_->setWordWrap(true);
    header_info->addWidget(title_label_);

    author_label_ = new QLabel;
    author_label_->setStyleSheet(QString("font-size: 16px; color: %1;").arg(c.text_secondary.name()));
    header_info->addWidget(author_label_);

    episode_count_label_ = new QLabel;
    episode_count_label_->setStyleSheet(QString("font-size: 13px; color: %1;").arg(c.text_muted.name()));
    header_info->addWidget(episode_count_label_);

    description_label_ = new QLabel;
    description_label_->setStyleSheet(QString("font-size: 13px; color: %1;").arg(c.text_secondary.name()));
    description_label_->setWordWrap(true);
    header_info->addWidget(description_label_);

    header_info->addStretch();
    header->addLayout(header_info, 1);
    content_layout_->addLayout(header);

    // Episodes section
    auto *ep_header = new QLabel("Episodios");
    ep_header->setStyleSheet(QString("font-size: 18px; font-weight: 600; color: %1; margin-top: 16px;").arg(c.text_primary.name()));
    content_layout_->addWidget(ep_header);

    episodes_widget_ = new QWidget;
    episodes_widget_->setStyleSheet("background: transparent;");
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
            cover_label_->setStyleSheet(QString("font-size: 64px; background: %1; border-radius: 8px;").arg(c.bg_elevated.name()));
        }
    }
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
}

void ShowDetailView::clear() {
    title_label_->clear();
    author_label_->clear();
    description_label_->clear();
    episode_count_label_->clear();
    cover_label_->clear();
    cover_label_->setStyleSheet(QString("background: %1; border-radius: 8px;").arg(DesignTokens::current().bg_elevated.name()));
    QLayoutItem *child;
    while ((child = episodes_layout_->takeAt(0)) != nullptr) {
        delete child->widget();
        delete child;
    }
    episodes_.clear();
}
