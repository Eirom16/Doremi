#ifndef DOREMI_TITLE_BAR_H
#define DOREMI_TITLE_BAR_H

#include <QWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QCompleter>
#include <QStringListModel>
#include <vector>

class TitleBar : public QWidget {
    Q_OBJECT
public:
    explicit TitleBar(QWidget *parent = nullptr);
    void set_search_text(const std::string &text);
    void set_search_suggestions(const std::vector<std::string> &suggestions);
    std::string search_text() const;
    void update_theme();
signals:
    void search_submitted(const std::string &query);
    void search_text_changed(const std::string &query);
private:
    QLineEdit *search_input_;
    QCompleter *search_completer_;
    QStringListModel *search_suggestions_model_;
};

#endif
