#pragma once

#include <QWidget>
#include <QString>
#include <QVBoxLayout>

class QPushButton;
class QLabel;

class EmptyState : public QWidget {
    Q_OBJECT
public:
    explicit EmptyState(QWidget *parent = nullptr);

    void setIcon(const QString &name, int size = 36);
    void setTitle(const QString &text);
    void setDescription(const QString &text);
    void setMessage(const QString &text);
    void applyPanelStyle(const QString &state = "empty");
    QPushButton *addButton(const QString &text);
    QPushButton *addRetryButton();

private:
    QVBoxLayout *layout_;
    QLabel *icon_ = nullptr;
    QLabel *title_ = nullptr;
    QLabel *description_ = nullptr;
};
