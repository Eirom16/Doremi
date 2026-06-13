#ifndef DOREMI_SUDO_DIALOG_H
#define DOREMI_SUDO_DIALOG_H

#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QFrame>
#include "components/ripple_button.h"

class SudoPasswordDialog : public QDialog {
    Q_OBJECT
public:
    explicit SudoPasswordDialog(QWidget *parent = nullptr);
    ~SudoPasswordDialog() override;

    QString get_password() const { return password_; }

private slots:
    void on_accept();
    void toggle_password_visibility();

private:
    void center_on_parent();
    void build_ui();

    QFrame *panel_ = nullptr;
    QLabel *title_lbl_ = nullptr;
    QLabel *subtitle_lbl_ = nullptr;
    QLabel *prompt_lbl_ = nullptr;
    QLineEdit *password_input_ = nullptr;
    RippleButton *toggle_visible_btn_ = nullptr;
    QLabel *error_lbl_ = nullptr;
    RippleButton *cancel_btn_ = nullptr;
    RippleButton *confirm_btn_ = nullptr;

    QString password_;
};

#endif // DOREMI_SUDO_DIALOG_H
