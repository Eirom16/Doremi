#ifndef DOREMI_WELCOME_VIEW_H
#define DOREMI_WELCOME_VIEW_H

#include <QWidget>
#include <QVBoxLayout>
#include <QLabel>
#include "components/ripple_button.h"
#include "components/glass_panel.h"

class WelcomeView : public QWidget {
    Q_OBJECT
public:
    explicit WelcomeView(QWidget *parent = nullptr);
    void update_theme();

private slots:
    void on_login_clicked();
    void handle_login_success(const QString &avatar_url, const QString &user_name);

private:
    QLabel *logo_ = nullptr;
    QLabel *title_ = nullptr;
    QLabel *subtitle_ = nullptr;
    GlassPanel *card_ = nullptr;
    QLabel *welcome_text_ = nullptr;
    QLabel *desc_text_ = nullptr;
    RippleButton *login_btn_ = nullptr;
    QLabel *status_label_ = nullptr;
    QLabel *progress_ = nullptr;
};

#endif // DOREMI_WELCOME_VIEW_H
