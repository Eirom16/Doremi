#include <QFrame>
#include <QPixmap>
#include <QHBoxLayout>
#include <QPainter>
#include <QPainterPath>
#include "trending_view.h"
#include "design_tokens.h"
#include "icon_provider.h"
#include <QFile>
#include "doremi/src/bridge.rs.h"

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

TrendingView::TrendingView(QWidget *parent)
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
    list_->setSpacing(8);

    auto *header = new QLabel("Tendencias", inner);
    header->setFont(DesignTokens::getFont("display", 24));
    header->setStyleSheet(QString("color: %1; font-weight: bold; background: transparent;").arg(c.text_primary.name()));
    list_->addWidget(header);

    auto *sub = new QLabel("Lo más popular del momento", inner);
    sub->setFont(DesignTokens::getFont("body", 13));
    sub->setStyleSheet(QString("color: %1; background: transparent; margin-bottom: 8px;").arg(c.text_secondary.name()));
    list_->addWidget(sub);

    list_->addStretch(1);
    scroll->setWidget(inner);
    root->addWidget(scroll);
    setStyleSheet("background: transparent;");
}

QWidget *TrendingView::make_trending_card(const std::string &title,
                                           const std::string &subtitle,
                                           const std::string &thumbnail_path) {
    const auto &c = DesignTokens::current();

    auto *card = new QWidget(this);
    card->setFixedHeight(72);
    
    // Modern hover style with QSS matching our design system tokens
    QString cardStyle = QString(
        "QWidget {\n"
        "    background-color: transparent;\n"
        "    border-radius: 8px;\n"
        "}\n"
        "QWidget:hover {\n"
        "    background-color: %1;\n"
        "}\n"
    )
    .arg(QString("rgba(%1, %2, %3, %4)").arg(c.accent_dim.red()).arg(c.accent_dim.green()).arg(c.accent_dim.blue()).arg(c.accent_dim.alpha() / 255.0));
    card->setStyleSheet(cardStyle);

    auto *lay = new QHBoxLayout(card);
    lay->setContentsMargins(12, 8, 12, 8);
    lay->setSpacing(12);

    auto *thumb = new QLabel(card);
    thumb->setFixedSize(56, 56);
    thumb->setAlignment(Qt::AlignCenter);
    thumb->setStyleSheet(QString("background: %1; border-radius: 6px;").arg(c.bg_elevated.name()));
    
    bool thumbLoaded = false;
    if (!thumbnail_path.empty() && QFile::exists(QString::fromStdString(thumbnail_path))) {
        QPixmap px(QString::fromStdString(thumbnail_path));
        if (!px.isNull()) {
            thumb->setPixmap(getRoundedPixmap(
                px.scaled(56, 56, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation), 6
            ));
            thumbLoaded = true;
        }
    }
    
    if (!thumbLoaded) {
        // Fallback default icon
        QPixmap fallback = IconProvider::getIcon("music_note", c.text_secondary, 22).pixmap(56, 56);
        thumb->setPixmap(getRoundedPixmap(fallback, 6));
    }
    
    lay->addWidget(thumb);

    auto *vl = new QVBoxLayout();
    vl->setSpacing(2);
    vl->setContentsMargins(0, 0, 0, 0);
    
    auto *t = new QLabel(QString::fromStdString(title), card);
    t->setFont(DesignTokens::getFont("body", 13));
    t->setStyleSheet(QString("color: %1; font-weight: bold; background: transparent;").arg(c.text_primary.name()));
    vl->addWidget(t);
    
    auto *s = new QLabel(QString::fromStdString(subtitle), card);
    s->setFont(DesignTokens::getFont("caption", 11));
    s->setStyleSheet(QString("color: %1; background: transparent;").arg(c.text_secondary.name()));
    vl->addWidget(s);
    
    lay->addLayout(vl, 1);

    auto *play_btn = new QPushButton(card);
    play_btn->setFixedSize(36, 36);
    play_btn->setCursor(Qt::PointingHandCursor);
    play_btn->setIcon(IconProvider::getIcon("play_arrow", QColor("#FFFFFF"), 18));
    play_btn->setIconSize(QSize(18, 18));
    
    QString playStyle = QString(
        "QPushButton {\n"
        "    background-color: %1;\n"
        "    border: none;\n"
        "    border-radius: 18px;\n"
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

    std::string id = title + " — " + subtitle;
    Track track_data;
    track_data.id = id;
    track_data.title = title;
    track_data.artist = subtitle;
    connect(play_btn, &QPushButton::clicked, this, [this, track_data]() {
        emit play_requested(track_data);
    });

    return card;
}

void TrendingView::clear_items() {
    // Keep header (0) and subtitle (1) and stretch (last)
    while (list_->count() > 3) {
        auto *item = list_->takeAt(2);
        if (item->widget()) {
            item->widget()->deleteLater();
        }
        delete item;
    }
}

void TrendingView::add_item(const std::string &title, const std::string &subtitle,
                             const std::string &thumbnail_path) {
    // Insert before stretch
    int idx = list_->count() - 1;
    list_->insertWidget(idx, make_trending_card(title, subtitle, thumbnail_path));
}
