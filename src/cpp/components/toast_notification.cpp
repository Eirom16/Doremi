#include "toast_notification.h"
#include "design_tokens.h"
#include "icon_provider.h"
#include "animator.h"
#include <QHBoxLayout>
#include <QPainter>
#include <QPropertyAnimation>
#include <QTimer>
#include <algorithm>

std::vector<ToastNotification*> ToastNotification::s_activeToasts;
std::deque<ToastNotification::ToastRequest> ToastNotification::s_queuedToasts;

ToastNotification::ToastNotification(QWidget *parent,
                                     const QString &message,
                                     Type type,
                                     const QString &actionLabel,
                                     std::function<void()> action)
    : QFrame(parent),
      m_type(type),
      m_message(message),
      m_actionLabel(actionLabel),
      m_action(std::move(action))
{
    setFocusPolicy(Qt::StrongFocus);
    DesignTokens::applyAccessible(
        this,
        "Notificacion",
        m_message,
        m_message);

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

    if (!m_actionLabel.isEmpty() && m_action) {
        m_actionButton = new QPushButton(m_actionLabel, this);
        m_actionButton->setCursor(Qt::PointingHandCursor);
        m_actionButton->setFocusPolicy(Qt::StrongFocus);
        m_actionButton->setProperty("buttonRole", "primary");
        m_actionButton->setMinimumWidth(76);
        DesignTokens::applyAccessible(
            m_actionButton,
            m_actionLabel,
            QString("Ejecuta la accion de la notificacion: %1").arg(m_message),
            m_actionLabel);
        lay->addWidget(m_actionButton);
        connect(m_actionButton, &QPushButton::clicked, this, [this]() {
            if (m_action) {
                m_action();
            }
            dismiss();
        });
    }

    setLayout(lay);
    setMinimumHeight(64);
}

ToastNotification::~ToastNotification() {
    auto it = std::find(s_activeToasts.begin(), s_activeToasts.end(), this);
    if (it != s_activeToasts.end()) {
        s_activeToasts.erase(it);
    }
    QTimer::singleShot(0, []() {
        ToastNotification::pumpQueue();
    });
}

void ToastNotification::showToast(QWidget *parent, const QString &message, Type type) {
    showOrQueue(parent, message, type, QString(), nullptr);
}

void ToastNotification::showToast(QWidget *parent,
                                  const QString &message,
                                  Type type,
                                  const QString &actionLabel,
                                  std::function<void()> action) {
    showOrQueue(parent, message, type, actionLabel, std::move(action));
}

void ToastNotification::repositionActiveToasts() {
    updateStackPositions();
}

QString ToastNotification::requestKey(const QString &message, Type type) {
    return QString::number(static_cast<int>(type)) + ":" + message.simplified();
}

QString ToastNotification::key() const {
    return requestKey(m_message, m_type);
}

void ToastNotification::showOrQueue(QWidget *parent,
                                    const QString &message,
                                    Type type,
                                    const QString &actionLabel,
                                    std::function<void()> action) {
    if (!parent) return;
    
    // Always attach to top-level window
    while (parent->parentWidget()) {
        parent = parent->parentWidget();
    }

    const QString newKey = requestKey(message, type);
    for (auto *toast : s_activeToasts) {
        if (toast && toast->key() == newKey) {
            toast->refreshTimeout();
            return;
        }
    }
    for (const auto &queued : s_queuedToasts) {
        if (requestKey(queued.message, queued.type) == newKey) {
            return;
        }
    }

    if (static_cast<int>(s_activeToasts.size()) >= MAX_ACTIVE_TOASTS) {
        if (static_cast<int>(s_queuedToasts.size()) >= MAX_QUEUED_TOASTS) {
            s_queuedToasts.pop_front();
        }
        s_queuedToasts.push_back({parent, message, type, actionLabel, std::move(action)});
        return;
    }

    ToastNotification *toast = new ToastNotification(parent, message, type, actionLabel, std::move(action));
    s_activeToasts.insert(s_activeToasts.begin(), toast);

    const int width = actionLabel.isEmpty() ? 360 : 460;
    toast->resize(width, std::max(64, toast->sizeHint().height() + 8));
    
    int parentWidth = parent->width();
    int parentHeight = parent->height();
    int targetX = parentWidth - toast->width() - 20;
    int targetY = parentHeight - 122 - static_cast<int>(s_activeToasts.size()) * (toast->height() + 10);

    toast->move(targetX, targetY + 20); // start slightly lower
    toast->show();
    
    // Trigger smooth entrance
    Animator::fadeIn(toast, 250);
    
    QPropertyAnimation *slide = new QPropertyAnimation(toast, "pos");
    slide->setDuration(DesignTokens::duration(250));
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
    m_progressAnim->setDuration(DesignTokens::duration(3500));
    m_progressAnim->setStartValue(1.0);
    m_progressAnim->setEndValue(0.0);
    connect(m_progressAnim, &QVariantAnimation::valueChanged, this, [this](const QVariant &val) {
        setProgress(val.toReal());
    });

    connect(m_dismissTimer, &QTimer::timeout, this, [this]() {
        dismiss();
    });

    m_dismissTimer->start();
    if (!DesignTokens::reducedMotion()) {
        m_progressAnim->start();
    }
}

void ToastNotification::refreshTimeout() {
    setProgress(1.0);
    if (m_dismissTimer) {
        m_dismissTimer->start(3500);
    }
    if (m_progressAnim) {
        m_progressAnim->stop();
        m_progressAnim->setStartValue(1.0);
        m_progressAnim->setEndValue(0.0);
        m_progressAnim->start();
    }
}

void ToastNotification::dismiss() {
    auto it = std::find(s_activeToasts.begin(), s_activeToasts.end(), this);
    if (it != s_activeToasts.end()) {
        s_activeToasts.erase(it);
        updateStackPositions();
    }

    if (m_dismissTimer) {
        m_dismissTimer->stop();
    }
    if (m_progressAnim) {
        m_progressAnim->stop();
    }

    Animator::fadeOut(this, 250);
    QTimer::singleShot(260, this, &QObject::deleteLater);
}

void ToastNotification::pumpQueue() {
    while (!s_queuedToasts.empty() &&
           static_cast<int>(s_activeToasts.size()) < MAX_ACTIVE_TOASTS) {
        ToastRequest request = std::move(s_queuedToasts.front());
        s_queuedToasts.pop_front();
        if (!request.parent) {
            continue;
        }
        showOrQueue(request.parent,
                    request.message,
                    request.type,
                    request.actionLabel,
                    std::move(request.action));
    }
}

void ToastNotification::updateStackPositions() {
    if (s_activeToasts.empty()) return;
    
    QWidget *parent = s_activeToasts.front()->parentWidget();
    if (!parent) return;

    int parentWidth = parent->width();
    int parentHeight = parent->height();
    int bottomMargin = 122;
    int rightMargin = 20;
    int spacing = 10;

    for (size_t i = 0; i < s_activeToasts.size(); ++i) {
        ToastNotification *toast = s_activeToasts[i];
        int targetX = parentWidth - toast->width() - rightMargin;
        int targetY = parentHeight - bottomMargin - (static_cast<int>(i) + 1) * (toast->height() + spacing);

        QPropertyAnimation *anim = new QPropertyAnimation(toast, "pos");
        anim->setDuration(DesignTokens::duration(250));
        anim->setStartValue(toast->pos());
        anim->setEndValue(QPoint(targetX, targetY));
        anim->setEasingCurve(QEasingCurve::OutCubic);
        connect(anim, &QPropertyAnimation::finished, anim, &QObject::deleteLater);
        anim->start();
    }
}

void ToastNotification::paintEvent(QPaintEvent *event) {
    // Render the base glassmorphism panel
    QFrame::paintEvent(event);

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
