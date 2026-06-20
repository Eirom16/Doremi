#pragma once

#include "glass_panel.h"
#include <QLabel>
#include <QPointer>
#include <QPushButton>
#include <QTimer>
#include <QVariantAnimation>
#include <deque>
#include <functional>
#include <vector>

class ToastNotification : public GlassPanel {
    Q_OBJECT
    Q_PROPERTY(qreal progress READ progress WRITE setProgress)
public:
    enum class Type {
        Success,
        Error,
        Info,
        Warning
    };

    static void showToast(QWidget *parent, const QString &message, Type type = Type::Info);
    static void showToast(QWidget *parent,
                          const QString &message,
                          Type type,
                          const QString &actionLabel,
                          std::function<void()> action);
    static void repositionActiveToasts();

    qreal progress() const { return m_progress; }
    void setProgress(qreal p) { m_progress = p; update(); }

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    struct ToastRequest {
        QPointer<QWidget> parent;
        QString message;
        Type type;
        QString actionLabel;
        std::function<void()> action;
    };

    explicit ToastNotification(QWidget *parent,
                               const QString &message,
                               Type type,
                               const QString &actionLabel = QString(),
                               std::function<void()> action = nullptr);
    ~ToastNotification() override;
    
    void startTimeout();
    void refreshTimeout();
    void dismiss();
    QString key() const;
    
    Type m_type;
    QString m_message;
    QString m_actionLabel;
    std::function<void()> m_action;
    qreal m_progress = 1.0;
    QTimer *m_dismissTimer = nullptr;
    QVariantAnimation *m_progressAnim = nullptr;
    QPushButton *m_actionButton = nullptr;
    
    static std::vector<ToastNotification*> s_activeToasts;
    static std::deque<ToastRequest> s_queuedToasts;
    static constexpr int MAX_ACTIVE_TOASTS = 3;
    static constexpr int MAX_QUEUED_TOASTS = 8;
    static QString requestKey(const QString &message, Type type);
    static void updateStackPositions();
    static void showOrQueue(QWidget *parent,
                            const QString &message,
                            Type type,
                            const QString &actionLabel,
                            std::function<void()> action);
    static void pumpQueue();
};
