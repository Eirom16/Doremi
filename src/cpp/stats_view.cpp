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
    thumb_lbl->setStyleSheet(QString("background-color: %1; border-radius: %2px;")
        .arg(c.bg_elevated.name()).arg(DesignTokens::radius().sm));

    if (!track.thumbnail.empty()) {
        QPointer<QLabel> label_ptr(thumb_lbl);
        ArtworkLoader::load(QString::fromStdString(static_cast<std::string>(track.thumbnail)), QSize(40, 40), [label_ptr](const QPixmap &pixmap) {
            if (!label_ptr) return;
            QPixmap dest(pixmap.size());
            dest.fill(Qt::transparent);
            QPainter painter(&dest);
            painter.setRenderHint(QPainter::Antialiasing);
            QPainterPath path;
            path.addRoundedRect(pixmap.rect(), 4, 4);
            painter.setClipPath(path);
            painter.drawPixmap(0, 0, pixmap);
            label_ptr->setPixmap(dest.scaled(40, 40, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation));
        });
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
    title_lbl->setFont(DesignTokens::getFont("body_sm"));
    title_lbl->setStyleSheet(QString("color: %1; font-weight: bold;").arg(c.text_primary.name()));

    auto *artist_lbl = new QLabel(QString::fromStdString(static_cast<std::string>(track.artist)), this);
    artist_lbl->setFont(DesignTokens::getFont("caption_sm"));
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
    bar->setStyleSheet(QString("background-color: %1; border-radius: %2px;").arg(c.accent.name()).arg(DesignTokens::radius().xs));

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
    setStyleSheet(QString("QWidget#TopTrackRow { background-color: transparent; border-radius: %1px; }").arg(DesignTokens::radius().sm));
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

    main_layout_ = new QVBoxLayout();
    main_layout_->setContentsMargins(DesignTokens::pagePadding());
    main_layout_->setSpacing(24);
    main_layout_->setAlignment(Qt::AlignTop);

    auto *header_layout = new QHBoxLayout();
    header_layout->setSpacing(12);

    auto *title = new QLabel(tr_q("listening_stats"), this);
    title->setObjectName("statsTitle");
    title->setFont(DesignTokens::getFont("heading_lg"));
    title->setStyleSheet(QString("color: %1; font-weight: bold;").arg(c.text_primary.name()));
    header_layout->addWidget(title);
    header_layout->addStretch();

    auto button_style = QString(
        "QPushButton { background: %1; border: 1px solid %2; border-radius: %8px; padding: 7px 12px; color: %3; font-size: 12px; }"
        "QPushButton:hover { background: rgba(%4, %5, %6, 0.08); }"
        "QPushButton:disabled { color: %7; border-color: %2; }")
        .arg(c.bg_surface.name())
        .arg(c.border.name())
        .arg(c.text_primary.name())
        .arg(c.text_primary.red())
        .arg(c.text_primary.green())
        .arg(c.text_primary.blue())
        .arg(c.text_muted.name())
        .arg(DesignTokens::radius().sm);

    auto *export_json = new QPushButton("JSON", this);
    export_json->setObjectName("statsExportBtn");
    export_json->setCursor(Qt::PointingHandCursor);
    export_json->setToolTip(tr_q("export_stats_json"));
    export_json->setStyleSheet(button_style);
    connect(export_json, &QPushButton::clicked, this, &StatsView::exportStatsAsJson);
    header_layout->addWidget(export_json);

    auto *export_csv = new QPushButton("CSV", this);
    export_csv->setObjectName("statsExportBtn");
    export_csv->setCursor(Qt::PointingHandCursor);
    export_csv->setToolTip(tr_q("export_stats_csv"));
    export_csv->setStyleSheet(button_style);
    connect(export_csv, &QPushButton::clicked, this, &StatsView::exportStatsAsCsv);
    header_layout->addWidget(export_csv);

    main_layout_->addLayout(header_layout);

    // Range selector
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
        btn->setStyleSheet(QString(
            "QPushButton { background: transparent; border: 1px solid %1; border-radius: %7px; padding: 0 16px; color: %2; font-size: 12px; }"
            "QPushButton:hover { background: rgba(%3, %4, %5, 0.08); }"
            "QPushButton:checked { background: %1; color: %6; }")
            .arg(c.border.name()).arg(c.text_secondary.name())
            .arg(c.text_primary.red()).arg(c.text_primary.green()).arg(c.text_primary.blue())
            .arg(c.bg_surface.name())
            .arg(DesignTokens::radius().pill));
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
    chart_header->setStyleSheet(QString("color: %1; font-weight: bold; margin-top: 8px;").arg(c.text_secondary.name()));
    main_layout_->addWidget(chart_header);

    auto *chart_panel = new QWidget(this);
    chart_panel->setObjectName("chart_panel");
    chart_panel->setStyleSheet(DesignTokens::panelStyle("surface", 12));
    auto *chart_p_layout = new QVBoxLayout(chart_panel);
    chart_p_layout->setContentsMargins(16, 16, 16, 16);

    bar_chart_ = new BarChart(chart_panel);
    chart_p_layout->addWidget(bar_chart_);
    main_layout_->addWidget(chart_panel);

    auto *top_header = new QLabel(tr_q("stat_top_tracks"), this);
    top_header->setObjectName("statsTopHeader");
    top_header->setFont(DesignTokens::getFont("heading_sm", 14));
    top_header->setStyleSheet(QString("color: %1; font-weight: bold; margin-top: 8px;").arg(c.text_secondary.name()));
    main_layout_->addWidget(top_header);

    top_tracks_widget_ = new QWidget(this);
    top_tracks_widget_->setObjectName("top_tracks_widget");
    top_tracks_widget_->setStyleSheet(DesignTokens::panelStyle("surface", 12));

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
        const auto &c = DesignTokens::current();
        auto *empty = new QLabel(tr_q("stat_not_enough_data"), top_tracks_widget_);
        empty->setObjectName("statsEmptyLabel");
        empty->setAlignment(Qt::AlignCenter);
        empty->setFont(DesignTokens::getFont("caption", 12));
        empty->setStyleSheet(QString("color: %1; padding: 20px;").arg(c.text_muted.name()));
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
    const auto &c = DesignTokens::current();
    if (auto *title = findChild<QLabel*>("statsTitle")) {
        title->setStyleSheet(QString("color: %1; font-weight: bold;").arg(c.text_primary.name()));
    }
    if (auto *ch = findChild<QLabel*>("statsChartHeader")) {
        ch->setStyleSheet(QString("color: %1; font-weight: bold; margin-top: 8px;").arg(c.text_secondary.name()));
    }
    if (auto *th = findChild<QLabel*>("statsTopHeader")) {
        th->setStyleSheet(QString("color: %1; font-weight: bold; margin-top: 8px;").arg(c.text_secondary.name()));
    }
    if (auto *el = findChild<QLabel*>("statsEmptyLabel")) {
        el->setStyleSheet(QString("color: %1; padding: 20px;").arg(c.text_muted.name()));
    }
    // Update export buttons
    auto button_style = QString(
        "QPushButton { background: %1; border: 1px solid %2; border-radius: %8px; padding: 7px 12px; color: %3; font-size: 12px; }"
        "QPushButton:hover { background: rgba(%4, %5, %6, 0.08); }"
        "QPushButton:disabled { color: %7; border-color: %2; }")
        .arg(c.bg_surface.name())
        .arg(c.border.name())
        .arg(c.text_primary.name())
        .arg(c.text_primary.red())
        .arg(c.text_primary.green())
        .arg(c.text_primary.blue())
        .arg(c.text_muted.name())
        .arg(DesignTokens::radius().sm);
    for (auto *btn : findChildren<QPushButton*>("statsExportBtn")) {
        btn->setStyleSheet(button_style);
    }
    // Update range buttons
    for (auto *btn : findChildren<QPushButton*>("statsRangeBtn")) {
        btn->setStyleSheet(QString(
            "QPushButton { background: transparent; border: 1px solid %1; border-radius: %7px; padding: 0 16px; color: %2; font-size: 12px; }"
            "QPushButton:hover { background: rgba(%3, %4, %5, 0.08); }"
            "QPushButton:checked { background: %1; color: %6; }")
            .arg(c.border.name()).arg(c.text_secondary.name())
            .arg(c.text_primary.red()).arg(c.text_primary.green()).arg(c.text_primary.blue())
            .arg(c.bg_surface.name())
            .arg(DesignTokens::radius().pill));
    }
    if (auto *cp = findChild<QWidget*>("chart_panel")) {
        cp->setStyleSheet(DesignTokens::panelStyle("surface", 12));
    }
    if (top_tracks_widget_) {
        top_tracks_widget_->setStyleSheet(DesignTokens::panelStyle("surface", 12));
    }
    if (card_time_) card_time_->update_theme();
    if (card_plays_) card_plays_->update_theme();
    if (card_artists_) card_artists_->update_theme();
}
