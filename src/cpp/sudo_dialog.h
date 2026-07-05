#ifndef DOREMI_SUDO_DIALOG_H
#define DOREMI_SUDO_DIALOG_H

#include <QDialog>
#include <QQuickWidget>
#include <QString>

class SudoPasswordDialog : public QDialog {
    Q_OBJECT
public:
    explicit SudoPasswordDialog(QWidget *parent = nullptr);
    ~SudoPasswordDialog() override;

    QString get_password() const { return password_; }

    Q_INVOKABLE void requestAccept(const QString &password);
    Q_INVOKABLE void requestCancel();

signals:
    void errorOccurred(const QString &message);
    void validatingChanged(bool isValidating);

private:
    void center_on_parent();
    QQuickWidget *quick_widget_ = nullptr;
    QString password_;
};

#endif // DOREMI_SUDO_DIALOG_H
