#ifndef DOREMI_WELCOME_VIEW_H
#define DOREMI_WELCOME_VIEW_H

#include <QWidget>
#include <QQuickWidget>

class WelcomeView : public QWidget {
    Q_OBJECT
    Q_PROPERTY(QString statusText READ statusText NOTIFY statusTextChanged)
    Q_PROPERTY(bool isLoggingIn READ isLoggingIn NOTIFY isLoggingInChanged)
    Q_PROPERTY(bool isSuccess READ isSuccess NOTIFY isSuccessChanged)

public:
    explicit WelcomeView(QWidget *parent = nullptr);
    void update_theme() {} // Kept for compatibility

    QString statusText() const { return status_text_; }
    bool isLoggingIn() const { return is_logging_in_; }
    bool isSuccess() const { return is_success_; }

    Q_INVOKABLE void requestLogin();

signals:
    void statusTextChanged();
    void isLoggingInChanged();
    void isSuccessChanged();

private slots:
    void handle_login_success(const QString &avatar_url, const QString &user_name);

private:
    QQuickWidget *quick_widget_ = nullptr;
    QString status_text_;
    bool is_logging_in_ = false;
    bool is_success_ = false;
    
    void setStatusText(const QString &text);
    void setIsLoggingIn(bool val);
    void setIsSuccess(bool val);
};

#endif // DOREMI_WELCOME_VIEW_H
