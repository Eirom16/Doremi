#ifndef DOREMI_HOME_VIEW_H
#define DOREMI_HOME_VIEW_H

#include <QWidget>
#include <QVBoxLayout>
#include <QScrollArea>
#include <QLabel>
#include <QHBoxLayout>

class HomeView : public QWidget {
    Q_OBJECT
public:
    explicit HomeView(QWidget *parent = nullptr);
    void set_welcome_message(const std::string &msg);
    void add_section(const std::string &title, const std::vector<std::string> &item_labels);
    void clear_sections();
private:
    QWidget *add_section_widget(const std::string &title, const std::vector<std::string> &items);
    QVBoxLayout *content_;
};

#endif
