#ifndef DOREMI_TITLE_BAR_H
#define DOREMI_TITLE_BAR_H

#include <QWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>

class TitleBar : public QWidget {
    Q_OBJECT
public:
    explicit TitleBar(QWidget *parent = nullptr);
    void set_search_text(const std::string &text);
    std::string search_text() const;
    void update_theme();
signals:
    void search_submitted(const std::string &query);
private:
    QLineEdit *search_input_;
};

#endif
