#ifndef DOREMI_HOME_VIEW_H
#define DOREMI_HOME_VIEW_H

#include <QWidget>
#include <QVBoxLayout>
#include <QScrollArea>
#include <QLabel>
#include <QHBoxLayout>
#include "doremi/src/bridge.rs.h"

class HomeView : public QWidget {
    Q_OBJECT
public:
    explicit HomeView(QWidget *parent = nullptr);
    void set_welcome_message(const std::string &msg);
    void add_section(const std::string &title, const std::vector<HomeCard> &items);
    void clear_sections();
    void set_state(const std::string &state, const std::string &message);
signals:
    void play_requested(Track track);
    void album_requested(const std::string &browse_id);
    void artist_requested(const std::string &browse_id);
    void playlist_requested(const std::string &playlist_id);
    void retry_requested();
    void load_more_requested();
private:
    QWidget *add_section_widget(const std::string &title, const std::vector<HomeCard> &items);
    QVBoxLayout *content_;
    QScrollArea *scroll_ = nullptr;
    QWidget *state_widget_ = nullptr;
};

#endif
