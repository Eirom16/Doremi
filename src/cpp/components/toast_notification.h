#pragma once

#include "glass_panel.h"
#include <QLabel>
#include <QTimer>
#include <QVariantAnimation>
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

    qreal progress() const { return m_progress; }
    void setProgress(qreal p) { m_progress = p; update(); }

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    explicit ToastNotification(QWidget *parent, const QString &message, Type type);
    ~ToastNotification() override;
    
    void startTimeout();
    
    Type m_type;
    QString m_message;
    qreal m_progress = 1.0;
    QTimer *m_dismissTimer = nullptr;
    QVariantAnimation *m_progressAnim = nullptr;
    
    static std::vector<ToastNotification*> s_activeToasts;
    static void updateStackPositions();
};
