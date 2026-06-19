#include <QFrame>
#include <QPixmap>
#include <QHBoxLayout>
#include <QPainter>
#include <QPainterPath>
#include "trending_view.h"
#include "design_tokens.h"
#include "icon_provider.h"
#include "components/artwork_loader.h"
#include "components/skeleton_loader.h"
#include <QFile>
#include <QPointer>
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

    list_ = new QVBoxLayout();
    list_->setContentsMargins(24, 24, 24, 24);
    list_->setSpacing(8);

    auto *header = new QLabel("Tendencias", this);
    header->setFont(DesignTokens::getFont("display", 24));
    header->setStyleSheet(QString("color: %1; font-weight: bold; background: transparent;").arg(c.text_primary.name()));
    list_->addWidget(header);

    auto *sub = new QLabel("Lo más popular del momento", this);
    sub->setFont(DesignTokens::getFont("body", 13));
    sub->setStyleSheet(QString("color: %1; background: transparent; margin-bottom: 8px;").arg(c.text_secondary.name()));
    list_->addWidget(sub);

    list_->addStretch(1);
    root->addLayout(list_);
    setStyleSheet("background: transparent;");
}

QWidget *TrendingView::make_trending_card(const HomeCard &item) {
    const auto &c = DesignTokens::current();
    const std::string id = static_cast<std::string>(item.id);
    const std::string title = static_cast<std::string>(item.title);
    const std::string subtitle = static_cast<std::string>(item.subtitle);
    const std::string thumbnail = static_cast<std::string>(item.thumbnail);
    const std::string itemType = static_cast<std::string>(item.item_type);

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
    if (!thumbnail.empty() && QFile::exists(QString::fromStdString(thumbnail))) {
        QPixmap px(QString::fromStdString(thumbnail));
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
    QPointer<QLabel> safeThumb(thumb);
    ArtworkLoader::load(QString::fromStdString(thumbnail), QSize(56, 56),
                        [safeThumb](const QPixmap &pixmap) {
        if (safeThumb) safeThumb->setPixmap(getRoundedPixmap(pixmap, 6));
    });
    
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

    connect(play_btn, &QPushButton::clicked, this,
            [this, id, title, subtitle, thumbnail, itemType]() {
        if (itemType == "song") {
            Track track;
            track.id = id;
            track.title = title;
            track.artist = subtitle;
            track.thumbnail = thumbnail;
            emit play_requested(track);
        } else if (itemType == "album") {
            emit album_requested(id);
        } else if (itemType == "artist") {
            emit artist_requested(id);
        } else {
            emit playlist_requested(id);
        }
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

void TrendingView::add_item(const HomeCard &item) {
    if (state_widget_) {
        state_widget_->deleteLater();
        state_widget_ = nullptr;
    }
    // Insert before stretch
    int idx = list_->count() - 1;
    list_->insertWidget(idx, make_trending_card(item));
}

void TrendingView::set_state(const std::string &state, const std::string &message) {
    if (state == "content") {
        if (state_widget_) state_widget_->deleteLater();
        state_widget_ = nullptr;
        return;
    }
    clear_items();
    const auto &c = DesignTokens::current();
    state_widget_ = new QWidget(this);
    auto *layout = new QVBoxLayout(state_widget_);
    layout->setContentsMargins(0, 12, 0, 12);
    layout->setSpacing(10);
    if (state == "loading") {
        for (int index = 0; index < 8; ++index) {
            auto *skeleton = new SkeletonLoader(state_widget_);
            skeleton->setFixedHeight(72);
            layout->addWidget(skeleton);
        }
    } else {
        auto *label = new QLabel(QString::fromStdString(message), state_widget_);
        label->setAlignment(Qt::AlignCenter);
        label->setWordWrap(true);
        label->setFont(DesignTokens::getFont("body", 13));
        label->setStyleSheet(QString("color: %1; padding: 36px;").arg(c.text_muted.name()));
        layout->addWidget(label);
        if (state == "error") {
            auto *retry = new QPushButton("Reintentar", state_widget_);
            retry->setCursor(Qt::PointingHandCursor);
            retry->setFixedWidth(140);
            retry->setStyleSheet(QString(
                "QPushButton { background: %1; color: white; border: none; border-radius: 8px; padding: 10px; }"
                "QPushButton:hover { background: %2; }")
                .arg(c.accent.name(), c.accent_bright.name()));
            connect(retry, &QPushButton::clicked, this, &TrendingView::retry_requested);
            layout->addWidget(retry, 0, Qt::AlignHCenter);
        }
    }
    list_->insertWidget(2, state_widget_);
}
