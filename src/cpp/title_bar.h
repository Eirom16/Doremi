#ifndef DOREMI_TITLE_BAR_H
#define DOREMI_TITLE_BAR_H

#include <QWidget>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QTimer>
#include <QListWidget>
#include <vector>
#include <QMenu>
#include "components/ripple_button.h"

class SearchSuggestionsPopup : public QWidget {
    Q_OBJECT
public:
    explicit SearchSuggestionsPopup(QWidget *parent = nullptr);
    void set_suggestions(const std::vector<std::string> &suggestions);
    void update_theme();
signals:
    void suggestion_selected(const std::string &suggestion);
private:
    QVBoxLayout *layout_;
    QListWidget *list_;
};

class TitleBar : public QWidget {
    Q_OBJECT
public:
    explicit TitleBar(QWidget *parent = nullptr);
    void set_search_text(const std::string &text);
    void set_search_suggestions(const std::string &query, const std::vector<std::string> &suggestions);
    std::string search_text() const;
    void focus_search();
    void set_sidebar_offset(int width);
    void update_theme();
    void set_context(const std::string &context);
signals:
    void search_submitted(const std::string &query);
    void search_text_changed(const std::string &query);
    void open_settings_requested();
protected:
    bool eventFilter(QObject *obj, QEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
private:
    QHBoxLayout *layout_ = nullptr;
    QWidget *logo_zone_ = nullptr;
    QLabel *logo_icon_ = nullptr;
    QLabel *logo_label_ = nullptr;
    QLineEdit *search_input_;
    SearchSuggestionsPopup *suggestions_popup_;
    QTimer *debounce_timer_;
    QAction *search_action_ = nullptr;
    std::string current_context_ = "home";

    RippleButton *profile_btn_ = nullptr;
    QMenu *profile_menu_ = nullptr;
    
    void update_popup_geometry();
};

#endif
