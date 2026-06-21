#include "artist_card.h"
#include "design_tokens.h"
#include "icon_provider.h"
#include "artwork_loader.h"
#include <QPainter>
#include <QPainterPath>
#include <QMouseEvent>
#include <QFile>
#include <QPointer>

static QPixmap getCircularPixmap(const QPixmap &src) {
    if (src.isNull()) return src;
    QPixmap dest(src.size());
    dest.fill(Qt::transparent);
    QPainter painter(&dest);
    painter.setRenderHint(QPainter::Antialiasing);
    QPainterPath path;
    path.addEllipse(src.rect());
    painter.setClipPath(path);
    painter.drawPixmap(0, 0, src);
    return dest;
}

ArtistCard::ArtistCard(const QString &name, const QString &thumbnail, QWidget *parent)
    : QWidget(parent), m_name(name), m_thumbnail(thumbnail)
{
    setFixedSize(160, 200);
    setCursor(Qt::PointingHandCursor);
    
    m_hoverAnim = new QVariantAnimation(this);
    m_hoverAnim->setDuration(DesignTokens::duration(150));
    m_hoverAnim->setEasingCurve(QEasingCurve::OutCubic);
    connect(m_hoverAnim, &QVariantAnimation::valueChanged, this, [this](const QVariant &val) {
        setHoverProgress(val.toReal());
    });
    QPointer<ArtistCard> self(this);
    ArtworkLoader::load(m_thumbnail, QSize(120, 120), [self](const QPixmap &pixmap) {
        if (!self) return;
        self->m_artPixmap = getCircularPixmap(pixmap);
        self->m_artLoaded = true;
        self->update();
    });
}

void ArtistCard::enterEvent(QEnterEvent *) {
    m_hoverAnim->stop();
    m_hoverAnim->setStartValue(m_hoverProgress);
    m_hoverAnim->setEndValue(1.0);
    m_hoverAnim->start();
}

void ArtistCard::leaveEvent(QEvent *) {
    m_hoverAnim->stop();
    m_hoverAnim->setStartValue(m_hoverProgress);
    m_hoverAnim->setEndValue(0.0);
    m_hoverAnim->start();
}

void ArtistCard::mousePressEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton) {
        emit clicked();
        event->accept();
        return;
    }
    QWidget::mousePressEvent(event);
}

void ArtistCard::paintEvent(QPaintEvent *) {
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
    
    // 2. Draw circular artwork
    int artSize = 120;
    QRect artRect((width() - artSize) / 2, 16, artSize, artSize);
    
    if (!m_artLoaded) {
        if (!m_thumbnail.isEmpty() && QFile::exists(m_thumbnail)) {
            QPixmap original;
            if (original.load(m_thumbnail)) {
                m_artPixmap = getCircularPixmap(original.scaled(artSize, artSize, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation));
                m_artLoaded = true;
            }
        }
        
        if (!m_artLoaded) {
            QPixmap defaultArt = IconProvider::getIcon("person", c.text_secondary, 48).pixmap(artSize, artSize);
            m_artPixmap = getCircularPixmap(defaultArt);
            m_artLoaded = true;
        }
    }
    
    // Slight scale effect on artwork when hovered
    qreal scale = 1.0 + 0.03 * m_hoverProgress;
    painter.save();
    painter.translate(artRect.center());
    painter.scale(scale, scale);
    painter.drawPixmap(-artSize / 2.0, -artSize / 2.0, m_artPixmap);
    painter.restore();
    
    // 3. Draw Artist Name
    painter.setFont(DesignTokens::getFont("body", 12));
    painter.setPen(c.text_primary);
    QString elidedName = painter.fontMetrics().elidedText(m_name, Qt::ElideRight, width() - 24);
    painter.drawText(QRect(12, 148, width() - 24, 20), Qt::AlignCenter, elidedName);
    
    // 4. Draw subtitle "Artista"
    painter.setFont(DesignTokens::getFont("caption", 10));
    painter.setPen(c.text_muted);
    painter.drawText(QRect(12, 168, width() - 24, 20), Qt::AlignCenter, "Artista");
}
