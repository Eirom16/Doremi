#ifndef DOREMI_STAT_CARD_H
#define DOREMI_STAT_CARD_H

#include <QWidget>
#include <QLabel>
#include <QVariantAnimation>

class StatCard : public QWidget {
    Q_OBJECT
    Q_PROPERTY(int displayValue READ displayValue WRITE setDisplayValue)
public:
    explicit StatCard(const QString &title, const QString &icon_name, QWidget *parent = nullptr);
    void setValue(int target_value, const QString &prefix = "", const QString &suffix = "");
    void setValueText(const QString &text); // for non-numeric stats (e.g. "12h 34m")
    void update_theme();

    int displayValue() const { return display_value_; }
    void setDisplayValue(int val);

protected:
    void paintEvent(QPaintEvent *event) override;
    void enterEvent(QEnterEvent *event) override;
    void leaveEvent(QEvent *event) override;

private:
    void updateLabels();

    QString title_;
    QString icon_name_;
    int target_value_ = 0;
    int display_value_ = 0;
    QString prefix_;
    QString suffix_;
    QString static_text_;
    bool is_numeric_ = true;
    bool is_hovered_ = false;

    QVariantAnimation *count_anim_ = nullptr;
    QLabel *title_lbl_;
    QLabel *value_lbl_;
    QLabel *icon_lbl_;
};

#endif
