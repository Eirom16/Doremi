#include "horizontal_carousel.h"
#include "design_tokens.h"
#include "icon_provider.h"
#include "animator.h"
#include <QScrollBar>
#include <QResizeEvent>
#include <QGraphicsOpacityEffect>

HorizontalCarousel::HorizontalCarousel(QWidget *parent)
    : QWidget(parent)
{
    init();
}

void HorizontalCarousel::init() {
    const auto &c = DesignTokens::current();

    auto *mainLayout = new QHBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    m_scrollArea = new QScrollArea(this);
    m_scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_scrollArea->setFrameShape(QFrame::NoFrame);
    m_scrollArea->setWidgetResizable(true);
    m_scrollArea->setStyleSheet(DesignTokens::scrollAreaStyle());

    m_contentWidget = new QWidget(m_scrollArea);
    m_contentWidget->setStyleSheet("background: transparent;");
    
    m_contentLayout = new QHBoxLayout(m_contentWidget);
    m_contentLayout->setContentsMargins(12, 12, 12, 12);
    m_contentLayout->setSpacing(16);
    m_contentLayout->addStretch(1); // Keep stretch at end

    m_contentWidget->setLayout(m_contentLayout);
    m_scrollArea->setWidget(m_contentWidget);
    mainLayout->addWidget(m_scrollArea);
    setLayout(mainLayout);

    // Floating Nav Buttons
    m_leftBtn = new QPushButton(this);
    m_rightBtn = new QPushButton(this);

    m_leftBtn->setFixedSize(36, 36);
    m_rightBtn->setFixedSize(36, 36);
    m_leftBtn->setCursor(Qt::PointingHandCursor);
    m_rightBtn->setCursor(Qt::PointingHandCursor);

    m_leftBtn->setIcon(IconProvider::getIcon("chevron_left", c.text_primary, 20));
    m_rightBtn->setIcon(IconProvider::getIcon("chevron_right", c.text_primary, 20));

    QString btnStyle = QString(
        "QPushButton {\n"
        "    background-color: %1;\n"
        "    border: 1px solid %2;\n"
        "    border-radius: %5px;\n"
        "}\n"
        "QPushButton:hover {\n"
        "    background-color: %3;\n"
        "    border-color: %4;\n"
        "}\n"
    )
    .arg(c.bg_surface.name())
    .arg(c.border.name())
    .arg(c.accent.name())
    .arg(c.accent_bright.name())
    .arg(DesignTokens::radius().xl);

    m_leftBtn->setStyleSheet(btnStyle);
    m_rightBtn->setStyleSheet(btnStyle);

    // Hide buttons initially
    m_leftBtn->hide();
    m_rightBtn->hide();

    // Scroll Animation setup
    m_scrollAnim = new QVariantAnimation(this);
    m_scrollAnim->setDuration(DesignTokens::duration(350));
    m_scrollAnim->setEasingCurve(QEasingCurve::InOutQuad);
    connect(m_scrollAnim, &QVariantAnimation::valueChanged, this, [this](const QVariant &val) {
        setScrollValue(val.toInt());
    });

    connect(m_leftBtn, &QPushButton::clicked, this, [this]() { scroll(-400); });
    connect(m_rightBtn, &QPushButton::clicked, this, [this]() { scroll(400); });

    // Monitor scroll changes to show/hide buttons dynamically
    connect(m_scrollArea->horizontalScrollBar(), &QScrollBar::valueChanged, this, [this]() {
        updateButtonVisibility();
    });
}

void HorizontalCarousel::addWidget(QWidget *widget) {
    widget->setParent(m_contentWidget);
    // Insert before the stretch item
    m_contentLayout->insertWidget(m_contentLayout->count() - 1, widget);
    
    int card_h = widget->sizeHint().height();
    if (card_h <= 0) {
        card_h = widget->maximumHeight();
    }
    if (card_h <= 0 || card_h >= 16777215) { // 16777215 is QWIDGETSIZE_MAX
        card_h = widget->height();
    }
    
    int h = card_h + 24;
    if (h > m_minContentHeight) {
        m_minContentHeight = h;
        setFixedHeight(m_minContentHeight);
        updateGeometry();
    }
    update();
}

QSize HorizontalCarousel::sizeHint() const {
    return QSize(width(), m_minContentHeight);
}

void HorizontalCarousel::clear() {
    m_scrollAnim->stop();
    
    // Clear all widgets in layout except the stretch
    QLayoutItem *child;
    while (m_contentLayout->count() > 1) {
        child = m_contentLayout->takeAt(0);
        if (child->widget()) {
            child->widget()->deleteLater();
        }
        delete child;
    }
    m_scrollArea->horizontalScrollBar()->setValue(0);
    m_minContentHeight = 0;
    setFixedHeight(0);
}

int HorizontalCarousel::scrollValue() const {
    return m_scrollArea->horizontalScrollBar()->value();
}

void HorizontalCarousel::setScrollValue(int val) {
    m_scrollArea->horizontalScrollBar()->setValue(val);
}

void HorizontalCarousel::scroll(int delta) {
    int startVal = scrollValue();
    int endVal = qBound(0, startVal + delta, m_scrollArea->horizontalScrollBar()->maximum());
    
    m_scrollAnim->stop();
    m_scrollAnim->setStartValue(startVal);
    m_scrollAnim->setEndValue(endVal);
    m_scrollAnim->start();
}

void HorizontalCarousel::updateButtonVisibility() {
    if (!m_scrollArea->horizontalScrollBar()) return;
    
    int val = scrollValue();
    int max = m_scrollArea->horizontalScrollBar()->maximum();
    
    m_leftBtn->setVisible(val > 0);
    m_rightBtn->setVisible(val < max);
}

void HorizontalCarousel::enterEvent(QEnterEvent *) {
    updateButtonVisibility();
    if (m_leftBtn->isVisible()) Animator::fadeIn(m_leftBtn, 150);
    if (m_rightBtn->isVisible()) Animator::fadeIn(m_rightBtn, 150);
}

void HorizontalCarousel::leaveEvent(QEvent *) {
    if (m_leftBtn->isVisible()) Animator::fadeOut(m_leftBtn, 150);
    if (m_rightBtn->isVisible()) Animator::fadeOut(m_rightBtn, 150);
}

void HorizontalCarousel::resizeEvent(QResizeEvent *event) {
    QWidget::resizeEvent(event);
    
    // Absolute position floating buttons on center Y, left and right edges
    int btnY = (height() - m_leftBtn->height()) / 2;
    m_leftBtn->move(8, btnY);
    m_rightBtn->move(width() - m_rightBtn->width() - 8, btnY);
    
    updateButtonVisibility();
}
