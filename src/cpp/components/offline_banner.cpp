#include "offline_banner.h"
#include "design_tokens.h"
#include "icon_provider.h"
#include "animator.h"
#include <QEvent>
#include <QTimer>

OfflineBannerWidget::OfflineBannerWidget(QWidget *parent)
    : QWidget(parent)
{
    // Layout setup
    auto *mainLayout = new QHBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    m_container = new QWidget(this);
    m_container->setObjectName("OfflineBannerContainer");
    
    auto *layout = new QHBoxLayout(m_container);
    layout->setContentsMargins(18, 0, 18, 0);
    layout->setSpacing(12);

    const auto &c = DesignTokens::current();

    // Icon Label (using IconProvider)
    m_icon = IconProvider::createIconLabel("wifi_off", 18, c.warning, true, m_container);
    layout->addWidget(m_icon);

    m_label = new QLabel("Sin conexión: reproduciendo descargas locales", m_container);
    m_label->setFont(DesignTokens::getFont("body_sm"));
    m_label->setStyleSheet(QString("color: %1; font-weight: 500; border: none; background: transparent;").arg(c.text_on_accent.name()));
    layout->addWidget(m_label);

    layout->addStretch();

    // Add badge
    m_badge = new QLabel("MODO OFFLINE", m_container);
    m_badge->setFont(DesignTokens::getFont("micro"));
    m_badge->setAlignment(Qt::AlignCenter);
    m_badge->setFixedSize(96, 20);
    layout->addWidget(m_badge);

    m_container->setLayout(layout);
    mainLayout->addWidget(m_container);
    setLayout(mainLayout);

    setFixedHeight(0);
    setVisible(false);
    applyStyle();
}

void OfflineBannerWidget::applyStyle() {
    const auto &c = DesignTokens::current();
    QColor warn_c = c.warning;
    int r = warn_c.red();
    int g = warn_c.green();
    int b = warn_c.blue();

    // Glassmorphic warning border, bg and floating margin
    m_container->setStyleSheet(QString(
        "QWidget#OfflineBannerContainer {"
        "  background-color: rgba(%1,%2,%3,0.08);"
        "  border: 1px solid rgba(%1,%2,%3,0.25);"
        "  border-radius: %4px;"
        "  margin: 4px 18px 8px 18px;"
        "}"
    ).arg(r).arg(g).arg(b).arg(DesignTokens::radius().lg));

    m_label->setStyleSheet(QString("color: %1; font-weight: 500; border: none; background: transparent;").arg(c.text_on_accent.name()));

    m_badge->setStyleSheet(QString(
        "QLabel {"
        "  background-color: rgba(%1,%2,%3,0.15);"
        "  color: %4;"
        "  border: 1px solid rgba(%1,%2,%3,0.4);"
        "  border-radius: %5px;"
        "  font-weight: 700;"
        "}"
    ).arg(r).arg(g).arg(b).arg(c.warning.name()).arg(DesignTokens::radius().sm));
    
    if (m_icon) {
        IconProvider::setupIconLabel(m_icon, "wifi_off", 18, c.warning, true);
    }
}

void OfflineBannerWidget::update_theme() {
    applyStyle();
}

void OfflineBannerWidget::changeEvent(QEvent *event) {
    if (event->type() == QEvent::PaletteChange || event->type() == QEvent::StyleChange) {
        applyStyle();
    }
    QWidget::changeEvent(event);
}

void OfflineBannerWidget::showBanner() {
    if (isVisible() && maximumHeight() > 0) {
        return;
    }
    setVisible(true);
    setMinimumHeight(0);
    Animator::animateHeight(this, 0, 54, 300);
    Animator::fadeIn(this, 300);
}

void OfflineBannerWidget::hideBanner() {
    if (!isVisible() || maximumHeight() == 0) {
        return;
    }
    Animator::animateHeight(this, height(), 0, 200);
    Animator::fadeOut(this, 200);
    QTimer::singleShot(200, this, [this]() {
        setVisible(false);
        setFixedHeight(0);
    });
}
