#include "toast_notification.h"
#include "design_tokens.h"
#include "icon_provider.h"
#include "animator.h"
#include <QHBoxLayout>
#include <QPainter>
#include <QPropertyAnimation>
#include <algorithm>

std::vector<ToastNotification*> ToastNotification::s_activeToasts;

ToastNotification::ToastNotification(QWidget *parent, const QString &message, Type type)
    : GlassPanel(parent), m_type(type), m_message(message)
{
    // Layout setup
    auto *lay = new QHBoxLayout(this);
    lay->setContentsMargins(12, 4, 12, 4);
    lay->setSpacing(12);

    const auto &c = DesignTokens::current();
    QColor typeColor;
    QString iconName;

    switch (m_type) {
        case Type::Success:
            typeColor = c.success;
            iconName = "check_circle";
            break;
        case Type::Error:
            typeColor = c.error;
            iconName = "error";
            break;
        case Type::Warning:
            typeColor = c.warning;
            iconName = "warning";
            break;
        case Type::Info:
        default:
            typeColor = c.accent;
            iconName = "info";
            break;
    }

    // Icon Label
    auto *iconLabel = IconProvider::createIconLabel(iconName, 18, typeColor, true, this);
    lay->addWidget(iconLabel);

    // Text Label
    auto *textLabel = new QLabel(m_message, this);
    textLabel->setFont(DesignTokens::getFont("body", 12));
    textLabel->setStyleSheet("color: " + c.text_primary.name() + "; background: transparent;");
    textLabel->setWordWrap(true);
    lay->addWidget(textLabel, 1);

    setLayout(lay);
}

ToastNotification::~ToastNotification() {
    auto it = std::find(s_activeToasts.begin(), s_activeToasts.end(), this);
    if (it != s_activeToasts.end()) {
        s_activeToasts.erase(it);
    }
}

void ToastNotification::showToast(QWidget *parent, const QString &message, Type type) {
    if (!parent) return;
    
    // Always attach to top-level window
    while (parent->parentWidget()) {
        parent = parent->parentWidget();
    }

    ToastNotification *toast = new ToastNotification(parent, message, type);
    s_activeToasts.insert(s_activeToasts.begin(), toast);

    toast->resize(300, 52);
    
    int parentWidth = parent->width();
    int parentHeight = parent->height();
    int targetX = parentWidth - toast->width() - 20;
    int targetY = parentHeight - 84 - static_cast<int>(s_activeToasts.size()) * (toast->height() + 10);

    toast->move(targetX, targetY + 20); // start slightly lower
    toast->show();
    
    // Trigger smooth entrance
    Animator::fadeIn(toast, 250);
    
    QPropertyAnimation *slide = new QPropertyAnimation(toast, "pos");
    slide->setDuration(250);
    slide->setStartValue(toast->pos());
    slide->setEndValue(QPoint(targetX, targetY + 20 - 20));
    slide->setEasingCurve(QEasingCurve::OutCubic);
    connect(slide, &QPropertyAnimation::finished, slide, &QObject::deleteLater);
    slide->start();

    toast->startTimeout();
    updateStackPositions();
}

void ToastNotification::startTimeout() {
    m_dismissTimer = new QTimer(this);
    m_dismissTimer->setSingleShot(true);
    m_dismissTimer->setInterval(3500);

    m_progressAnim = new QVariantAnimation(this);
    m_progressAnim->setDuration(3500);
    m_progressAnim->setStartValue(1.0);
    m_progressAnim->setEndValue(0.0);
    connect(m_progressAnim, &QVariantAnimation::valueChanged, this, [this](const QVariant &val) {
        setProgress(val.toReal());
    });

    connect(m_dismissTimer, &QTimer::timeout, this, [this]() {
        auto it = std::find(s_activeToasts.begin(), s_activeToasts.end(), this);
        if (it != s_activeToasts.end()) {
            s_activeToasts.erase(it);
            updateStackPositions();
        }
        
        Animator::fadeOut(this, 250);
        QTimer::singleShot(260, this, &QObject::deleteLater);
    });

    m_dismissTimer->start();
    m_progressAnim->start();
}

void ToastNotification::updateStackPositions() {
    if (s_activeToasts.empty()) return;
    
    QWidget *parent = s_activeToasts.front()->parentWidget();
    if (!parent) return;

    int parentWidth = parent->width();
    int parentHeight = parent->height();
    int bottomMargin = 84;
    int rightMargin = 20;
    int toastHeight = 52;
    int spacing = 10;

    for (size_t i = 0; i < s_activeToasts.size(); ++i) {
        ToastNotification *toast = s_activeToasts[i];
        int targetX = parentWidth - toast->width() - rightMargin;
        int targetY = parentHeight - bottomMargin - (static_cast<int>(i) + 1) * (toastHeight + spacing);

        QPropertyAnimation *anim = new QPropertyAnimation(toast, "pos");
        anim->setDuration(250);
        anim->setStartValue(toast->pos());
        anim->setEndValue(QPoint(targetX, targetY));
        anim->setEasingCurve(QEasingCurve::OutCubic);
        connect(anim, &QPropertyAnimation::finished, anim, &QObject::deleteLater);
        anim->start();
    }
}

void ToastNotification::paintEvent(QPaintEvent *event) {
    // Render the base glassmorphism panel
    GlassPanel::paintEvent(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    const auto &c = DesignTokens::current();
    QColor typeColor;

    switch (m_type) {
        case Type::Success: typeColor = c.success; break;
        case Type::Error: typeColor = c.error; break;
        case Type::Warning: typeColor = c.warning; break;
        case Type::Info:
        default:
            typeColor = c.accent;
            break;
    }

    // Draw the progress bar line at the very bottom
    qreal barHeight = 2.0;
    qreal y = height() - barHeight - 2; // leave small space from border
    qreal w = (width() - 4) * m_progress; // margins accounted for
    
    painter.setPen(Qt::NoPen);
    painter.setBrush(typeColor);
    painter.drawRoundedRect(QRectF(2, y, w, barHeight), 1, 1);
}
