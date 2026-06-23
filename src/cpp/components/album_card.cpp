#include "album_card.h"
#include "design_tokens.h"
#include "icon_provider.h"
#include "artwork_loader.h"
#include <QPainter>
#include <QPainterPath>
#include <QMouseEvent>
#include <QFile>
#include <QPointer>
#include <algorithm>

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

AlbumCard::AlbumCard(const QString &title, const QString &artist, const QString &thumbnail, QWidget *parent)
    : QWidget(parent), m_title(title), m_artist(artist), m_thumbnail(thumbnail)
{
    setFixedSize(170, 235);
    setCursor(Qt::PointingHandCursor);
    
    m_hoverAnim = new QVariantAnimation(this);
    m_hoverAnim->setDuration(DesignTokens::duration(150));
    m_hoverAnim->setEasingCurve(QEasingCurve::OutCubic);
    connect(m_hoverAnim, &QVariantAnimation::valueChanged, this, [this](const QVariant &val) {
        setHoverProgress(val.toReal());
    });
    QPointer<AlbumCard> self(this);
    ArtworkLoader::load(m_thumbnail, QSize(146, 146), [self](const QPixmap &pixmap) {
        if (!self) return;
        self->m_artPixmap = getRoundedPixmap(pixmap, 8);
        self->m_artLoaded = true;
        self->update();
    });
}

void AlbumCard::enterEvent(QEnterEvent *) {
    m_hoverAnim->stop();
    m_hoverAnim->setStartValue(m_hoverProgress);
    m_hoverAnim->setEndValue(1.0);
    m_hoverAnim->start();
}

void AlbumCard::leaveEvent(QEvent *) {
    m_hoverAnim->stop();
    m_hoverAnim->setStartValue(m_hoverProgress);
    m_hoverAnim->setEndValue(0.0);
    m_hoverAnim->start();
}

void AlbumCard::mousePressEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton) {
        QRect artRect(12, 12, 146, 146);
        if (m_hoverProgress > 0.5 && artRect.contains(event->pos())) {
            emit playRequested(m_itemId);
        } else {
            emit clicked();
        }
        event->accept();
        return;
    }
    QWidget::mousePressEvent(event);
}

void AlbumCard::paintEvent(QPaintEvent *) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setRenderHint(QPainter::TextAntialiasing);
    painter.setRenderHint(QPainter::SmoothPixmapTransform);
    
    const auto &c = DesignTokens::current();
    const QString type = m_contentType.toLower();
    QString badgeText = "Álbum";
    QString iconName = "album";
    QColor badgeColor = c.accent;
    if (type == "playlist") {
        badgeText = "Playlist";
        iconName = "queue_music";
        badgeColor = QColor("#00A8A8");
    } else if (type == "mix") {
        badgeText = "Mix";
        iconName = "auto_awesome";
        badgeColor = QColor("#FFB000");
    } else if (type == "show") {
        badgeText = "Podcast";
        iconName = "podcasts";
        badgeColor = QColor("#E85D75");
    } else if (type == "episode") {
        badgeText = "Episodio";
        iconName = "graphic_eq";
        badgeColor = QColor("#E85D75");
    }
    
    // 1. Draw card background (fade in on hover)
    if (m_hoverProgress > 0.0) {
        QColor bg = c.accent_dim;
        bg.setAlphaF(bg.alphaF() * m_hoverProgress);
        painter.setBrush(bg);
        painter.setPen(Qt::NoPen);
        painter.drawRoundedRect(rect(), 8, 8);
    }
    
    // 2. Draw artwork
    QRect artRect(12, 12, 146, 146);
    if (!m_artLoaded) {
        if (!m_thumbnail.isEmpty() && QFile::exists(m_thumbnail)) {
            QPixmap original;
            if (original.load(m_thumbnail)) {
                m_artPixmap = getRoundedPixmap(original.scaled(146, 146, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation), 8);
                m_artLoaded = true;
            }
        }
        
        if (!m_artLoaded) {
            QPixmap defaultArt = IconProvider::getIcon(iconName, c.text_secondary, 48).pixmap(146, 146);
            m_artPixmap = getRoundedPixmap(defaultArt, 8);
            m_artLoaded = true;
        }
    }
    
    painter.drawPixmap(artRect.topLeft(), m_artPixmap);

    QRect badgeRect(20, 22, std::min(112, 50 + painter.fontMetrics().horizontalAdvance(badgeText)), 24);
    QColor badgeBg = badgeColor;
    badgeBg.setAlpha(220);
    painter.setPen(Qt::NoPen);
    painter.setBrush(badgeBg);
    painter.drawRoundedRect(badgeRect, 12, 12);
    QIcon badgeIcon = IconProvider::getIcon(iconName, Qt::white, 14);
    painter.drawPixmap(badgeRect.left() + 8, badgeRect.top() + 5, badgeIcon.pixmap(14, 14));
    painter.setFont(DesignTokens::getFont("caption", 9));
    painter.setPen(Qt::white);
    painter.drawText(badgeRect.adjusted(26, 0, -8, 0), Qt::AlignVCenter | Qt::AlignLeft, badgeText);
    
    // 3. Draw play overlay on hover
    if (m_hoverProgress > 0.0) {
        painter.save();
        
        QPainterPath clipPath;
        clipPath.addRoundedRect(artRect, 8, 8);
        painter.setClipPath(clipPath);
        
        painter.setPen(Qt::NoPen);
        painter.setBrush(QColor(0, 0, 0, static_cast<int>(100 * m_hoverProgress))); // up to ~40% opacity
        painter.drawRect(artRect);
        
        QPointF center = artRect.center();
        qreal circleRadius = 22.0 * m_hoverProgress;
        
        if (circleRadius > 0.0) {
            QColor circleColor = badgeColor;
            circleColor.setAlphaF(m_hoverProgress);
            painter.setBrush(circleColor);
            painter.drawEllipse(center, circleRadius, circleRadius);
            
            const QString actionIcon = (type == "album") ? "play_arrow" : "open_in_new";
            QIcon playIcon = IconProvider::getIcon(actionIcon, QColor(255, 255, 255, static_cast<int>(255 * m_hoverProgress)), 22);
            QPixmap playPm = playIcon.pixmap(22, 22);
            painter.drawPixmap(center.x() - 11, center.y() - 11, playPm);
        }
        
        painter.restore();
    }
    
    // 4. Draw Title
    painter.setFont(DesignTokens::getFont("body", 12));
    painter.setPen(c.text_primary);
    QString elidedTitle = painter.fontMetrics().elidedText(m_title, Qt::ElideRight, width() - 24);
    painter.drawText(QRect(12, 172, width() - 24, 20), Qt::AlignLeft | Qt::AlignVCenter, elidedTitle);
    
    // 5. Draw Artist / Subtitle
    painter.setFont(DesignTokens::getFont("caption", 10));
    painter.setPen(c.text_secondary);
    QString elidedArtist = painter.fontMetrics().elidedText(m_artist, Qt::ElideRight, width() - 24);
    painter.drawText(QRect(12, 192, width() - 24, 20), Qt::AlignLeft | Qt::AlignVCenter, elidedArtist);
}
