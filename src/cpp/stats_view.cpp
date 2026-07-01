#include "stats_view.h"
#include "design_tokens.h"
#include "icon_provider.h"
#include "components/artwork_loader.h"
#include <QHBoxLayout>
#include <QGridLayout>
#include <QEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QPushButton>
#include <QButtonGroup>
#include <QFile>
#include <QFileDialog>
#include <QStyle>
#include <QJsonArray>
#include <QJsonDocument>
#include <QPointer>
#include <QJsonObject>
#include <QMessageBox>
#include <QTextStream>
#include <algorithm>
#include "doremi/src/bridge.rs.h"

namespace {
QString rs(const rust::String &value) {
    return QString::fromStdString(static_cast<std::string>(value));
}

QString csvCell(QString value) {
    value.replace('"', "\"\"");
    if (value.contains(',') || value.contains('"') || value.contains('\n') || value.contains('\r')) {
        return QString("\"%1\"").arg(value);
    }
    return value;
}
}


StatsView::StatsView(QWidget *parent)
    : QWidget(parent)
{
    setupLayout();
}

void StatsView::setupLayout() {
    auto *main_vbox = new QVBoxLayout(this);
    main_vbox->setContentsMargins(0, 0, 0, 0);
    main_vbox->setSpacing(0);

    main_layout_ = new QVBoxLayout();
    main_layout_->setContentsMargins(DesignTokens::pagePadding());
    main_layout_->setSpacing(24);
    main_layout_->setAlignment(Qt::AlignTop);

    auto *header_layout = new QHBoxLayout();
    header_layout->setSpacing(12);

    auto *title = new QLabel(tr_q("listening_stats"), this);
    title->setObjectName("statsTitle");
    title->setFont(DesignTokens::getFont("heading_lg"));
    title->setProperty("textRole", "heading");
    header_layout->addWidget(title);
    header_layout->addStretch();

    auto *export_json = new QPushButton("JSON", this);
    export_json->setObjectName("statsExportBtn");
    export_json->setCursor(Qt::PointingHandCursor);
    export_json->setToolTip(tr_q("export_stats_json"));
    export_json->setObjectName("statsExportBtn");
    connect(export_json, &QPushButton::clicked, this, &StatsView::exportStatsAsJson);
    header_layout->addWidget(export_json);

    auto *export_csv = new QPushButton("CSV", this);
    export_csv->setObjectName("statsExportBtn");
    export_csv->setCursor(Qt::PointingHandCursor);
    export_csv->setToolTip(tr_q("export_stats_csv"));
    export_csv->setObjectName("statsExportBtn");
    connect(export_csv, &QPushButton::clicked, this, &StatsView::exportStatsAsCsv);
    header_layout->addWidget(export_csv);

    main_layout_->addLayout(header_layout);

    auto *range_layout = new QHBoxLayout();
    range_layout->setSpacing(8);
    auto *range_group = new QButtonGroup(this);
    struct RangeOption { QString label; int days; };
    RangeOption ranges[] = {
        {tr_q("days_7"), 7},
        {tr_q("days_30"), 30},
        {tr_q("year_1"), 365},
        {tr_q("all_time"), -1}
    };
    for (auto &opt : ranges) {
        auto *btn = new QPushButton(opt.label, this);
        btn->setObjectName("statsRangeBtn");
        btn->setCheckable(true);
        btn->setFixedHeight(30);
        btn->setCursor(Qt::PointingHandCursor);
        range_group->addButton(btn, opt.days);
        range_layout->addWidget(btn);
    }
    range_group->button(7)->setChecked(true);
    connect(range_group, &QButtonGroup::idClicked, this, [](int days) {
        on_stats_requested(days);
    });
    range_layout->addStretch();
    main_layout_->addLayout(range_layout);

    auto *cards_layout = new QHBoxLayout();
    cards_layout->setSpacing(16);

    card_time_ = new StatCard(tr_q("stat_time_played"), "schedule", this);
    card_plays_ = new StatCard(tr_q("stat_total_plays"), "play_arrow", this);
    card_artists_ = new StatCard(tr_q("stat_unique_artists"), "person", this);
    cards_layout->addWidget(card_time_);
    cards_layout->addWidget(card_plays_);
    cards_layout->addWidget(card_artists_);
    main_layout_->addLayout(cards_layout);

    auto *chart_header = new QLabel(tr_q("stat_weekly_activity"), this);
    chart_header->setObjectName("statsChartHeader");
    chart_header->setFont(DesignTokens::getFont("heading_sm", 14));
    chart_header->setProperty("textRole", "primary");
    main_layout_->addWidget(chart_header);

    auto *chart_panel = new QWidget(this);
    chart_panel->setObjectName("chart_panel");
    chart_panel->setProperty("boxRole", "card");
    auto *chart_p_layout = new QVBoxLayout(chart_panel);
    chart_p_layout->setContentsMargins(16, 16, 16, 16);

    bar_chart_ = new BarChart(chart_panel);
    chart_p_layout->addWidget(bar_chart_);
    main_layout_->addWidget(chart_panel);

    auto *top_header = new QLabel(tr_q("stat_top_tracks"), this);
    top_header->setObjectName("statsTopHeader");
    top_header->setFont(DesignTokens::getFont("heading_sm", 14));
    top_header->setProperty("textRole", "primary");
    main_layout_->addWidget(top_header);

    top_tracks_widget_ = new QWidget(this);
    top_tracks_widget_->setObjectName("top_tracks_widget");
    top_tracks_widget_->setProperty("boxRole", "card");

    top_tracks_layout_ = new QVBoxLayout(top_tracks_widget_);
    top_tracks_layout_->setContentsMargins(8, 8, 8, 8);
    top_tracks_layout_->setSpacing(4);

    main_layout_->addWidget(top_tracks_widget_);

    main_vbox->addLayout(main_layout_);
    setLayout(main_vbox);
}

void StatsView::showEvent(QShowEvent *event) {
    QWidget::showEvent(event);
    on_stats_requested(7);
}

void StatsView::setStatsData(const StatsData &stats) {
    current_stats_ = stats;
    has_stats_ = true;

    card_time_->setValueText(QString::fromStdString(static_cast<std::string>(stats.total_play_time)));
    card_plays_->setValue(stats.total_plays);
    card_artists_->setValue(stats.unique_artists);

    QVector<int> act;
    for (int v : stats.weekly_activity) act.push_back(v);
    bar_chart_->setData(act);

    std::vector<Track> tracks(stats.top_tracks.begin(), stats.top_tracks.end());
    std::vector<int> plays(stats.top_tracks_plays.begin(), stats.top_tracks_plays.end());
    buildTopTracks(tracks, plays);
}

void StatsView::buildTopTracks(const std::vector<Track> &tracks, const std::vector<int> &plays) {
    QLayoutItem *item;
    while ((item = top_tracks_layout_->takeAt(0)) != nullptr) {
        if (item->widget()) {
            item->widget()->deleteLater();
        }
        delete item;
    }

    if (tracks.empty()) {
        auto *empty = new QLabel(tr_q("stat_not_enough_data"), top_tracks_widget_);
        empty->setObjectName("statsEmptyLabel");
        empty->setAlignment(Qt::AlignCenter);
        empty->setFont(DesignTokens::getFont("caption", 12));
        empty->setProperty("textRole", "muted");
        top_tracks_layout_->addWidget(empty);
        return;
    }

    int max_plays = plays.empty() ? 1 : plays[0];
    int count = static_cast<int>(tracks.size());
    for (int i = 0; i < count; ++i) {
        int track_plays = static_cast<std::size_t>(i) < plays.size() ? plays[i] : 1;
        auto *row = new TopTrackRow(i + 1, tracks[i], track_plays, max_plays, top_tracks_widget_);
        connect(row, &TopTrackRow::clicked, this, &StatsView::play_requested);
        top_tracks_layout_->addWidget(row);
    }
}

void StatsView::exportStatsAsJson() {
    if (!has_stats_) {
        QMessageBox::information(this,
            tr_q("no_data"),
            tr_q("no_stats_to_export"));
        return;
    }

    QString path = QFileDialog::getSaveFileName(
        this,
        tr_q("export_stats"),
        "doremi-stats.json",
        "JSON (*.json)");
    if (path.isEmpty()) return;
    if (!path.endsWith(".json", Qt::CaseInsensitive)) path += ".json";

    if (writeStatsJson(path)) {
        QMessageBox::information(this,
            tr_q("export_completed"),
            tr_q("export_success_msg"));
    } else {
        QMessageBox::warning(this,
            tr_q("export_error"),
            tr_q("export_failed_msg"));
    }
}

void StatsView::exportStatsAsCsv() {
    if (!has_stats_) {
        QMessageBox::information(this,
            tr_q("no_data"),
            tr_q("no_stats_to_export"));
        return;
    }

    QString path = QFileDialog::getSaveFileName(
        this,
        tr_q("export_stats"),
        "doremi-stats.csv",
        "CSV (*.csv)");
    if (path.isEmpty()) return;
    if (!path.endsWith(".csv", Qt::CaseInsensitive)) path += ".csv";

    if (writeStatsCsv(path)) {
        QMessageBox::information(this,
            tr_q("export_completed"),
            tr_q("export_success_msg"));
    } else {
        QMessageBox::warning(this,
            tr_q("export_error"),
            tr_q("export_failed_msg"));
    }
}

bool StatsView::writeStatsJson(const QString &path) const {
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
        return false;
    }

    QJsonArray weekly;
    for (int value : current_stats_.weekly_activity) {
        weekly.append(value);
    }

    QJsonArray top_tracks;
    for (std::size_t i = 0; i < current_stats_.top_tracks.size(); ++i) {
        const auto &track = current_stats_.top_tracks[i];
        QJsonObject item;
        item["rank"] = static_cast<int>(i + 1);
        item["id"] = rs(track.id);
        item["title"] = rs(track.title);
        item["artist"] = rs(track.artist);
        item["album"] = rs(track.album);
        item["duration_ms"] = static_cast<qint64>(track.duration_ms);
        item["thumbnail"] = rs(track.thumbnail);
        item["plays"] = i < current_stats_.top_tracks_plays.size()
            ? current_stats_.top_tracks_plays[i]
            : 0;
        top_tracks.append(item);
    }

    QJsonObject root;
    root["total_play_time"] = rs(current_stats_.total_play_time);
    root["total_plays"] = current_stats_.total_plays;
    root["unique_artists"] = current_stats_.unique_artists;
    root["weekly_activity"] = weekly;
    root["top_tracks"] = top_tracks;

    file.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
    return file.error() == QFileDevice::NoError;
}

bool StatsView::writeStatsCsv(const QString &path) const {
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
        return false;
    }

    QTextStream out(&file);
    out << "metric,value\n";
    out << "total_play_time," << csvCell(rs(current_stats_.total_play_time)) << "\n";
    out << "total_plays," << current_stats_.total_plays << "\n";
    out << "unique_artists," << current_stats_.unique_artists << "\n\n";

    out << "day_offset,plays\n";
    for (std::size_t i = 0; i < current_stats_.weekly_activity.size(); ++i) {
        out << static_cast<int>(i) - 6 << "," << current_stats_.weekly_activity[i] << "\n";
    }

    out << "\nrank,id,title,artist,album,duration_ms,plays,thumbnail\n";
    for (std::size_t i = 0; i < current_stats_.top_tracks.size(); ++i) {
        const auto &track = current_stats_.top_tracks[i];
        int plays = i < current_stats_.top_tracks_plays.size()
            ? current_stats_.top_tracks_plays[i]
            : 0;
        out << (i + 1) << ","
            << csvCell(rs(track.id)) << ","
            << csvCell(rs(track.title)) << ","
            << csvCell(rs(track.artist)) << ","
            << csvCell(rs(track.album)) << ","
            << track.duration_ms << ","
            << plays << ","
            << csvCell(rs(track.thumbnail)) << "\n";
    }

    out.flush();
    return file.error() == QFileDevice::NoError;
}

void StatsView::update_theme() {
    if (card_time_) card_time_->update_theme();
    if (card_plays_) card_plays_->update_theme();
    if (card_artists_) card_artists_->update_theme();
    style()->unpolish(this);
    style()->polish(this);
}
