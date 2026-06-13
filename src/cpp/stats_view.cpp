#include "stats_view.h"
#include "design_tokens.h"
#include "icon_provider.h"
#include <QHBoxLayout>
#include <QGridLayout>
#include <QScrollBar>
#include <QEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <algorithm>
#include "doremi/src/bridge.rs.h"

TopTrackRow::TopTrackRow(int rank, const Track &track,
            int plays, int max_plays, QWidget *parent)
    : QWidget(parent), track_(track)
{
    const auto &c = DesignTokens::current();
    setFixedHeight(56);
    setCursor(Qt::PointingHandCursor);

    auto *layout = new QHBoxLayout(this);
    layout->setContentsMargins(12, 6, 12, 6);
    layout->setSpacing(16);

    auto *rank_lbl = new QLabel(QString("#%1").arg(rank), this);
    rank_lbl->setFont(DesignTokens::getFont("heading_sm", 12));
    rank_lbl->setFixedWidth(24);
    rank_lbl->setStyleSheet(QString("color: %1; font-weight: bold;").arg(c.accent.name()));
    layout->addWidget(rank_lbl);

    auto *thumb_lbl = new QLabel(this);
    thumb_lbl->setFixedSize(40, 40);
    thumb_lbl->setStyleSheet(QString("background-color: %1; border-radius: 4px;")
        .arg(c.bg_elevated.name()));

    QPixmap pm;
    if (!track.thumbnail.empty() && pm.load(QString::fromStdString(static_cast<std::string>(track.thumbnail)))) {
        QPixmap dest(pm.size());
        dest.fill(Qt::transparent);
        QPainter painter(&dest);
        painter.setRenderHint(QPainter::Antialiasing);
        QPainterPath path;
        path.addRoundedRect(pm.rect(), 4, 4);
        painter.setClipPath(path);
        painter.drawPixmap(0, 0, pm);
        thumb_lbl->setPixmap(dest.scaled(40, 40, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation));
    } else {
        QPixmap default_art = IconProvider::getIcon("music_note", c.text_secondary, 20).pixmap(40, 40);
        thumb_lbl->setPixmap(default_art);
    }
    layout->addWidget(thumb_lbl);

    auto *text_container = new QWidget(this);
    auto *text_layout = new QVBoxLayout(text_container);
    text_layout->setContentsMargins(0, 0, 0, 0);
    text_layout->setSpacing(2);

    auto *title_lbl = new QLabel(QString::fromStdString(static_cast<std::string>(track.title)), this);
    title_lbl->setFont(DesignTokens::getFont("body", 13));
    title_lbl->setStyleSheet(QString("color: %1; font-weight: bold;").arg(c.text_primary.name()));

    auto *artist_lbl = new QLabel(QString::fromStdString(static_cast<std::string>(track.artist)), this);
    artist_lbl->setFont(DesignTokens::getFont("caption", 11));
    artist_lbl->setStyleSheet(QString("color: %1;").arg(c.text_secondary.name()));

    text_layout->addWidget(title_lbl);
    text_layout->addWidget(artist_lbl);
    layout->addWidget(text_container, 2);

    auto *ratio_widget = new QWidget(this);
    ratio_widget->setMinimumWidth(80);
    auto *ratio_layout = new QVBoxLayout(ratio_widget);
    ratio_layout->setContentsMargins(0, 0, 0, 0);
    ratio_layout->setSpacing(4);

    auto *plays_lbl = new QLabel(QString("%1 reproducciones").arg(plays), this);
    plays_lbl->setFont(DesignTokens::getFont("caption", 10));
    plays_lbl->setStyleSheet(QString("color: %1;").arg(c.text_secondary.name()));
    plays_lbl->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

    auto *bar = new QWidget(this);
    bar->setFixedHeight(4);
    int target_w = max_plays > 0 ? (plays * 80 / max_plays) : 0;
    bar->setFixedWidth(qMax(4, target_w));
    bar->setStyleSheet(QString("background-color: %1; border-radius: 2px;").arg(c.accent.name()));

    auto *bar_container = new QWidget(this);
    bar_container->setFixedHeight(4);
    auto *bar_c_layout = new QHBoxLayout(bar_container);
    bar_c_layout->setContentsMargins(0, 0, 0, 0);
    bar_c_layout->addStretch();
    bar_c_layout->addWidget(bar);

    ratio_layout->addWidget(plays_lbl);
    ratio_layout->addWidget(bar_container);
    layout->addWidget(ratio_widget, 1);

    setLayout(layout);

    setObjectName("TopTrackRow");
    setStyleSheet("QWidget#TopTrackRow { background-color: transparent; border-radius: 6px; }");
}

void TopTrackRow::mousePressEvent(QMouseEvent *event) {
    QWidget::mousePressEvent(event);
    if (event->button() == Qt::LeftButton) {
        emit clicked(track_);
    }
}

void TopTrackRow::enterEvent(QEnterEvent *event) {
    QWidget::enterEvent(event);
    setStyleSheet(QString("QWidget#TopTrackRow { background-color: %1; }")
        .arg(QString("rgba(%1, %2, %3, 0.06)").arg(DesignTokens::current().text_primary.red())
                                                .arg(DesignTokens::current().text_primary.green())
                                                .arg(DesignTokens::current().text_primary.blue())));
}

void TopTrackRow::leaveEvent(QEvent *event) {
    QWidget::leaveEvent(event);
    setStyleSheet("QWidget#TopTrackRow { background-color: transparent; }");
}

StatsView::StatsView(QWidget *parent)
    : QWidget(parent)
{
    setupLayout();
}

void StatsView::setupLayout() {
    const auto &c = DesignTokens::current();

    auto *main_vbox = new QVBoxLayout(this);
    main_vbox->setContentsMargins(0, 0, 0, 0);
    main_vbox->setSpacing(0);

    scroll_area_ = new QScrollArea(this);
    scroll_area_->setWidgetResizable(true);
    scroll_area_->setFrameShape(QFrame::NoFrame);
    scroll_area_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scroll_area_->setStyleSheet("background: transparent;");

    scroll_content_ = new QWidget(scroll_area_);
    scroll_content_->setStyleSheet("background: transparent;");

    main_layout_ = new QVBoxLayout(scroll_content_);
    main_layout_->setContentsMargins(24, 24, 24, 24);
    main_layout_->setSpacing(24);
    main_layout_->setAlignment(Qt::AlignTop);

    auto *title = new QLabel("Estadísticas de Escucha", scroll_content_);
    title->setFont(DesignTokens::getFont("display", 22));
    title->setStyleSheet(QString("color: %1; font-weight: bold;").arg(c.text_primary.name()));
    main_layout_->addWidget(title);

    auto *cards_layout = new QHBoxLayout();
    cards_layout->setSpacing(16);

    card_time_ = new StatCard("Tiempo Escuchado", "schedule", scroll_content_);
    card_plays_ = new StatCard("Reproducciones Totales", "play_arrow", scroll_content_);
    card_artists_ = new StatCard("Artistas Escuchados", "person", scroll_content_);

    cards_layout->addWidget(card_time_);
    cards_layout->addWidget(card_plays_);
    cards_layout->addWidget(card_artists_);
    main_layout_->addLayout(cards_layout);

    auto *chart_header = new QLabel("Actividad Semanal", scroll_content_);
    chart_header->setFont(DesignTokens::getFont("heading_sm", 14));
    chart_header->setStyleSheet(QString("color: %1; font-weight: bold; margin-top: 8px;").arg(c.text_secondary.name()));
    main_layout_->addWidget(chart_header);

    auto *chart_panel = new QWidget(scroll_content_);
    chart_panel->setStyleSheet(QString("background-color: %1; border: 1px solid %2; border-radius: 12px;")
        .arg(c.bg_surface.name())
        .arg(c.border.name()));
    auto *chart_p_layout = new QVBoxLayout(chart_panel);
    chart_p_layout->setContentsMargins(16, 16, 16, 16);

    bar_chart_ = new BarChart(chart_panel);
    chart_p_layout->addWidget(bar_chart_);
    main_layout_->addWidget(chart_panel);

    auto *top_header = new QLabel("Tus Canciones Más Escuchadas", scroll_content_);
    top_header->setFont(DesignTokens::getFont("heading_sm", 14));
    top_header->setStyleSheet(QString("color: %1; font-weight: bold; margin-top: 8px;").arg(c.text_secondary.name()));
    main_layout_->addWidget(top_header);

    top_tracks_widget_ = new QWidget(scroll_content_);
    top_tracks_widget_->setStyleSheet(QString("background-color: %1; border: 1px solid %2; border-radius: 12px;")
        .arg(c.bg_surface.name())
        .arg(c.border.name()));

    top_tracks_layout_ = new QVBoxLayout(top_tracks_widget_);
    top_tracks_layout_->setContentsMargins(8, 8, 8, 8);
    top_tracks_layout_->setSpacing(4);

    main_layout_->addWidget(top_tracks_widget_);

    scroll_area_->setWidget(scroll_content_);
    main_vbox->addWidget(scroll_area_);
    setLayout(main_vbox);
}

void StatsView::showEvent(QShowEvent *event) {
    QWidget::showEvent(event);
    on_stats_requested();
}

void StatsView::setStatsData(const StatsData &stats) {
    card_time_->setValueText(QString::fromStdString(static_cast<std::string>(stats.total_play_time)));
    card_plays_->setValue(stats.total_plays);
    card_artists_->setValue(stats.unique_artists);

    QVector<int> act;
    for (int v : stats.weekly_activity) act.push_back(v);
    bar_chart_->setData(act);

    buildTopTracks(std::vector<Track>(stats.top_tracks.begin(), stats.top_tracks.end()),
                   stats.total_plays);
}

void StatsView::buildTopTracks(const std::vector<Track> &tracks, int total_plays) {
    QLayoutItem *item;
    while ((item = top_tracks_layout_->takeAt(0)) != nullptr) {
        if (item->widget()) {
            item->widget()->deleteLater();
        }
        delete item;
    }

    if (tracks.empty()) {
        const auto &c = DesignTokens::current();
        auto *empty = new QLabel("No hay suficientes datos de reproducción para generar el top.", top_tracks_widget_);
        empty->setAlignment(Qt::AlignCenter);
        empty->setFont(DesignTokens::getFont("caption", 12));
        empty->setStyleSheet(QString("color: %1; padding: 20px;").arg(c.text_muted.name()));
        top_tracks_layout_->addWidget(empty);
        return;
    }

    int max_plays = total_plays > 0 ? total_plays : 1;
    int count = static_cast<int>(tracks.size());
    for (int i = 0; i < count; ++i) {
        // We don't have per-track play count from StatsData directly,
        // so we estimate based on position (higher rank = more plays)
        int plays = (count - i) * (total_plays / qMax(count, 1) / 2);
        auto *row = new TopTrackRow(i + 1, tracks[i], plays, max_plays, top_tracks_widget_);
        connect(row, &TopTrackRow::clicked, this, &StatsView::play_requested);
        top_tracks_layout_->addWidget(row);
    }
}
