#include "song_card.h"
#include "design_tokens.h"
#include "icon_provider.h"
#include "artwork_loader.h"
#include <QPainter>
#include <QPainterPath>
#include <QMouseEvent>
#include <QFile>
#include <QPointer>

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

SongCard::SongCard(const QString &title, const QString &artist, const QString &thumbnail, QWidget *parent)
    : QWidget(parent), m_title(title), m_artist(artist), m_thumbnail(thumbnail)
{
    setFixedSize(160, 220);
    setCursor(Qt::PointingHandCursor);
    
    m_hoverAnim = new QVariantAnimation(this);
    m_hoverAnim->setDuration(DesignTokens::duration(150));
    m_hoverAnim->setEasingCurve(QEasingCurve::OutCubic);
    connect(m_hoverAnim, &QVariantAnimation::valueChanged, this, [this](const QVariant &val) {
        setHoverProgress(val.toReal());
    });
    QPointer<SongCard> self(this);
    ArtworkLoader::load(m_thumbnail, QSize(136, 136), [self](const QPixmap &pixmap) {
        if (!self) return;
        self->m_artPixmap = getRoundedPixmap(pixmap, 6);
        self->m_artLoaded = true;
        self->update();
    });
}

void SongCard::enterEvent(QEnterEvent *) {
    m_hoverAnim->stop();
    m_hoverAnim->setStartValue(m_hoverProgress);
    m_hoverAnim->setEndValue(1.0);
    m_hoverAnim->start();
}

void SongCard::leaveEvent(QEvent *) {
    m_hoverAnim->stop();
    m_hoverAnim->setStartValue(m_hoverProgress);
    m_hoverAnim->setEndValue(0.0);
    m_hoverAnim->start();
}

void SongCard::mousePressEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton) {
        QRect artRect(12, 12, 136, 136);
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

void SongCard::paintEvent(QPaintEvent *) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setRenderHint(QPainter::TextAntialiasing);
    painter.setRenderHint(QPainter::SmoothPixmapTransform);
    
    const auto &c = DesignTokens::current();
    
    // 1. Draw card background (fade in on hover)
    if (m_hoverProgress > 0.0) {
        QColor bg = c.accent_dim;
        bg.setAlphaF(bg.alphaF() * m_hoverProgress);
        painter.setBrush(bg);
        painter.setPen(Qt::NoPen);
        painter.drawRoundedRect(rect(), 8, 8);
    }
    
    // 2. Draw artwork
    QRect artRect(12, 12, 136, 136);
    if (!m_artLoaded) {
        if (!m_thumbnail.isEmpty() && QFile::exists(m_thumbnail)) {
            QPixmap original;
            if (original.load(m_thumbnail)) {
                m_artPixmap = getRoundedPixmap(original.scaled(136, 136, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation), 6);
                m_artLoaded = true;
            }
        }
        
        if (!m_artLoaded) {
            QPixmap defaultArt = IconProvider::getIcon("music_note", c.text_secondary, 44).pixmap(136, 136);
            m_artPixmap = getRoundedPixmap(defaultArt, 6);
            m_artLoaded = true;
        }
    }
    
    painter.drawPixmap(artRect.topLeft(), m_artPixmap);
    
    // 3. Draw play overlay on hover
    if (m_hoverProgress > 0.0) {
        painter.save();
        
        QPainterPath clipPath;
        clipPath.addRoundedRect(artRect, 6, 6);
        painter.setClipPath(clipPath);
        
        painter.setPen(Qt::NoPen);
        painter.setBrush(QColor(0, 0, 0, static_cast<int>(100 * m_hoverProgress))); // up to ~40% opacity
        painter.drawRect(artRect);
        
        QPointF center = artRect.center();
        qreal circleRadius = 20.0 * m_hoverProgress;
        
        if (circleRadius > 0.0) {
            QColor circleColor = c.accent;
            circleColor.setAlphaF(m_hoverProgress);
            painter.setBrush(circleColor);
            painter.drawEllipse(center, circleRadius, circleRadius);
            
            QIcon playIcon = IconProvider::getIcon("play_arrow", QColor(255, 255, 255, static_cast<int>(255 * m_hoverProgress)), 20);
            QPixmap playPm = playIcon.pixmap(20, 20);
            painter.drawPixmap(center.x() - 10, center.y() - 10, playPm);
        }
        
        painter.restore();
    }
    
    // 4. Draw Title
    painter.setFont(DesignTokens::getFont("body", 12));
    painter.setPen(c.text_primary);
    QString elidedTitle = painter.fontMetrics().elidedText(m_title, Qt::ElideRight, width() - 24);
    painter.drawText(QRect(12, 160, width() - 24, 20), Qt::AlignLeft | Qt::AlignVCenter, elidedTitle);
    
    // 5. Draw Artist
    painter.setFont(DesignTokens::getFont("caption", 10));
    painter.setPen(c.text_secondary);
    QString elidedArtist = painter.fontMetrics().elidedText(m_artist, Qt::ElideRight, width() - 24);
    painter.drawText(QRect(12, 180, width() - 24, 20), Qt::AlignLeft | Qt::AlignVCenter, elidedArtist);
}
