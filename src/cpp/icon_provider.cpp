#include "icon_provider.h"
#include "design_tokens.h"
#include <QPixmap>
#include <QPainter>

QFont IconProvider::getFont(int size, bool filled) {
    QFont font = DesignTokens::getFont("icon", size);
    // In Qt6, we can set font variations using font.setVariableAxis
    // For Material Symbols, the "FILL" axis accepts 0.0 (outlined) or 1.0 (filled).
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    // Try to set variable axis for font weight / fill
    // "FILL" is the axis name
    font.setVariableAxis("FILL", filled ? 1.0f : 0.0f);
#endif
    return font;
}

QIcon IconProvider::getIcon(const QString &name, const QColor &color, int size, bool filled) {
    // Generate a high DPI pixmap (e.g. scale up by 2x for retina screens)
    int scale = 2;
    QPixmap pixmap(size * scale, size * scale);
    pixmap.fill(Qt::transparent);

    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setRenderHint(QPainter::TextAntialiasing);

    QFont font = getFont(size * scale, filled);
    painter.setFont(font);
    painter.setPen(color);
    
    painter.drawText(pixmap.rect(), Qt::AlignCenter, name);
    painter.end();

    // Set high device pixel ratio so Qt scales it down beautifully
    pixmap.setDevicePixelRatio(scale);
    return QIcon(pixmap);
}

QLabel *IconProvider::createIconLabel(const QString &name, int size, const QColor &color, bool filled, QWidget *parent) {
    QLabel *label = new QLabel(parent);
    setupIconLabel(label, name, size, color, filled);
    return label;
}

void IconProvider::setupIconLabel(QLabel *label, const QString &name, int size, const QColor &color, bool filled) {
    label->setText(name);
    label->setFont(getFont(size, filled));
    label->setAlignment(Qt::AlignCenter);
    
    // Prevent clipping of glyphs
    int widget_size = qMax(size + 8, 28);
    label->setFixedSize(widget_size, widget_size);
    
    // Use rgba color format to preserve alpha
    QString color_str = QString("rgba(%1, %2, %3, %4)")
        .arg(color.red()).arg(color.green()).arg(color.blue()).arg(color.alpha() / 255.0);
        
    // Set style
    QString style = QString("color: %1; background: transparent; font-family: 'Material Symbols Rounded'; font-size: %2px;")
        .arg(color_str)
        .arg(size);
    label->setStyleSheet(style);
}
