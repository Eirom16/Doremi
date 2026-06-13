#include <QFrame>
#include <QPixmap>
#include <QPainter>
#include <QPainterPath>
#include <QFile>
#include "doremi/src/bridge.rs.h"
#include "downloads_view.h"
#include "design_tokens.h"
#include "icon_provider.h"

static QPixmap getRoundedPixmap(const QPixmap &src, int radius) {
    if (src.isNull()) return src;
    QPixmap dest(src.size());
    dest.fill(Qt::transparent);
    QPainter painter(&dest);
    painter.setRenderHint(QPainter::Antialiasing);
    QPainterPath path;
    path.addRoundedRect(src.rect(), radius, radius);
    painter.setClipPath(path);
    painter.drawPixmap(0, 0, src);
    return dest;
}

DownloadsView::DownloadsView(QWidget *parent)
    : QWidget(parent)
{
    const auto &c = DesignTokens::current();

    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    auto *scroll = new QScrollArea(this);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setStyleSheet("background: transparent; border: none;");

    auto *inner = new QWidget();
    inner->setStyleSheet("background: transparent;");
    list_ = new QVBoxLayout(inner);
    list_->setContentsMargins(24, 24, 24, 24);
    list_->setSpacing(6);

    auto *header = new QLabel("Descargas", inner);
    header->setFont(DesignTokens::getFont("display", 24));
    header->setStyleSheet(QString("color: %1; font-weight: bold; background: transparent;").arg(c.text_primary.name()));
    list_->addWidget(header);

    status_label_ = new QLabel("Sin descargas", inner);
    status_label_->setFont(DesignTokens::getFont("body", 14));
    status_label_->setStyleSheet(QString("color: %1; padding: 24px 0; background: transparent;").arg(c.text_muted.name()));
    list_->addWidget(status_label_);

    list_->addStretch(1);
    scroll->setWidget(inner);
    root->addWidget(scroll);
    setStyleSheet("background: transparent;");
}

QWidget *DownloadsView::make_download_row(const std::string &title,
                                           const std::string &artist,
                                           const std::string &thumbnail_path) {
    const auto &c = DesignTokens::current();

    auto *row = new QWidget(this);
    row->setFixedHeight(64);
    
    QString rowStyle = QString(
        "QWidget {\n"
        "    background-color: transparent;\n"
        "    border-radius: 8px;\n"
        "}\n"
        "QWidget:hover {\n"
        "    background-color: %1;\n"
        "}\n"
    )
    .arg(QString("rgba(%1, %2, %3, %4)").arg(c.accent_dim.red()).arg(c.accent_dim.green()).arg(c.accent_dim.blue()).arg(c.accent_dim.alpha() / 255.0));
    row->setStyleSheet(rowStyle);

    auto *lay = new QHBoxLayout(row);
    lay->setContentsMargins(12, 6, 12, 6);
    lay->setSpacing(12);

    auto *thumb = new QLabel(row);
    thumb->setFixedSize(48, 48);
    thumb->setAlignment(Qt::AlignCenter);
    thumb->setStyleSheet(QString("background: %1; border-radius: 4px;").arg(c.bg_elevated.name()));
    
    bool thumbLoaded = false;
    if (!thumbnail_path.empty() && QFile::exists(QString::fromStdString(thumbnail_path))) {
        QPixmap px(QString::fromStdString(thumbnail_path));
        if (!px.isNull()) {
            thumb->setPixmap(getRoundedPixmap(
                px.scaled(48, 48, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation), 4
            ));
            thumbLoaded = true;
        }
    }
    
    if (!thumbLoaded) {
        QPixmap fallback = IconProvider::getIcon("music_note", c.text_secondary, 20).pixmap(48, 48);
        thumb->setPixmap(getRoundedPixmap(fallback, 4));
    }
    
    lay->addWidget(thumb);

    auto *vl = new QVBoxLayout();
    vl->setSpacing(2);
    vl->setContentsMargins(0, 0, 0, 0);
    
    auto *t = new QLabel(QString::fromStdString(title), row);
    t->setFont(DesignTokens::getFont("body", 13));
    t->setStyleSheet(QString("color: %1; font-weight: bold; background: transparent;").arg(c.text_primary.name()));
    vl->addWidget(t);
    
    auto *a = new QLabel(QString::fromStdString(artist), row);
    a->setFont(DesignTokens::getFont("caption", 11));
    a->setStyleSheet(QString("color: %1; background: transparent;").arg(c.text_secondary.name()));
    vl->addWidget(a);
    
    lay->addLayout(vl, 1);

    auto *play_btn = new QPushButton(row);
    play_btn->setFixedSize(32, 32);
    play_btn->setCursor(Qt::PointingHandCursor);
    play_btn->setIcon(IconProvider::getIcon("play_arrow", QColor("#FFFFFF"), 16));
    play_btn->setIconSize(QSize(16, 16));
    
    QString playStyle = QString(
        "QPushButton {\n"
        "    background-color: %1;\n"
        "    border: none;\n"
        "    border-radius: 16px;\n"
        "}\n"
        "QPushButton:hover {\n"
        "    background-color: %2;\n"
        "}\n"
        "QPushButton:pressed {\n"
        "    background-color: %3;\n"
        "}\n"
    )
    .arg(c.accent.name())
    .arg(c.accent_bright.name())
    .arg(c.accent.darker(115).name());
    
    play_btn->setStyleSheet(playStyle);
    lay->addWidget(play_btn);

    Track track_data;
    track_data.id = title + " — " + artist;
    track_data.title = title;
    track_data.artist = artist;
    connect(play_btn, &QPushButton::clicked, this, [this, track_data]() {
        emit play_requested(track_data);
    });

    return row;
}

void DownloadsView::set_downloads(const std::vector<std::string> &titles,
                                   const std::vector<std::string> &artists,
                                   const std::vector<std::string> &thumbnails) {
    clear_downloads();
    size_t n = std::min({titles.size(), artists.size(), thumbnails.size()});
    if (n == 0) {
        status_label_->setText("Sin descargas");
        status_label_->show();
        return;
    }
    status_label_->hide();
    for (size_t i = 0; i < n; ++i) {
        int idx = list_->count() - 1;
        list_->insertWidget(idx, make_download_row(titles[i], artists[i], thumbnails[i]));
    }
}

void DownloadsView::clear_downloads() {
    // Keep header (0) and status_label_ (1) and stretch (last)
    while (list_->count() > 3) {
        auto *item = list_->takeAt(2);
        if (item->widget()) {
            item->widget()->deleteLater();
        }
        delete item;
    }
}
