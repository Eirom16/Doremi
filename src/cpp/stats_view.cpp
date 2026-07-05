#include "stats_view.h"
#include <QQmlContext>
#include <QVBoxLayout>
#include <QFileDialog>
#include <QMessageBox>
#include <QFile>
#include <QTextStream>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
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
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    quick_widget_ = new QQuickWidget(this);
    quick_widget_->setResizeMode(QQuickWidget::SizeRootObjectToView);
    quick_widget_->setAttribute(Qt::WA_TranslucentBackground);
    quick_widget_->setClearColor(Qt::transparent);

    quick_widget_->rootContext()->setContextProperty("StatsCtrl", this);
    quick_widget_->setSource(QUrl("qrc:/qml/StatsView.qml"));

    layout->addWidget(quick_widget_);
}

void StatsView::showEvent(QShowEvent *event) {
    QWidget::showEvent(event);
    on_stats_requested(7);
}

void StatsView::requestStats(int days) {
    on_stats_requested(days);
}

void StatsView::setStatsData(const StatsData &stats) {
    current_stats_ = stats;

    QVariantMap sum;
    sum["totalPlayTime"] = rs(stats.total_play_time);
    sum["totalPlays"] = stats.total_plays;
    sum["uniqueArtists"] = stats.unique_artists;
    summary_ = sum;

    QVariantList act;
    for (int v : stats.weekly_activity) {
        act.append(v);
    }
    daily_plays_ = act;

    QVariantList top;
    for (std::size_t i = 0; i < stats.top_tracks.size(); ++i) {
        const auto &track = stats.top_tracks[i];
        QVariantMap t;
        t["id"] = rs(track.id);
        t["title"] = rs(track.title);
        t["artist"] = rs(track.artist);
        t["album"] = rs(track.album);
        t["thumbnail"] = rs(track.thumbnail);
        t["durationMs"] = static_cast<qint64>(track.duration_ms);
        t["plays"] = i < stats.top_tracks_plays.size() ? stats.top_tracks_plays[i] : 0;
        top.append(t);
    }
    top_tracks_ = top;

    emit statsChanged();
}

void StatsView::requestPlay(const QString &trackId) {
    for (const auto &track : current_stats_.top_tracks) {
        if (rs(track.id) == trackId) {
            emit play_requested(track);
            break;
        }
    }
}

void StatsView::exportStatsAsJson() {
    if (current_stats_.top_tracks.empty() && current_stats_.total_plays == 0) {
        QMessageBox::information(this,
            "Sin Datos",
            "No hay estadísticas para exportar en este periodo.");
        return;
    }

    QString path = QFileDialog::getSaveFileName(
        this,
        "Exportar Estadísticas",
        "doremi-stats.json",
        "JSON (*.json)");
    if (path.isEmpty()) return;
    if (!path.endsWith(".json", Qt::CaseInsensitive)) path += ".json";

    if (writeStatsJson(path)) {
        QMessageBox::information(this, "Exportado", "Las estadísticas se han exportado exitosamente.");
    } else {
        QMessageBox::warning(this, "Error", "Hubo un error al exportar el archivo.");
    }
}

void StatsView::exportStatsAsCsv() {
    if (current_stats_.top_tracks.empty() && current_stats_.total_plays == 0) {
        QMessageBox::information(this,
            "Sin Datos",
            "No hay estadísticas para exportar en este periodo.");
        return;
    }

    QString path = QFileDialog::getSaveFileName(
        this,
        "Exportar Estadísticas",
        "doremi-stats.csv",
        "CSV (*.csv)");
    if (path.isEmpty()) return;
    if (!path.endsWith(".csv", Qt::CaseInsensitive)) path += ".csv";

    if (writeStatsCsv(path)) {
        QMessageBox::information(this, "Exportado", "Las estadísticas se han exportado exitosamente.");
    } else {
        QMessageBox::warning(this, "Error", "Hubo un error al exportar el archivo.");
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
