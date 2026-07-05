#ifndef DOREMI_HOME_VIEW_H
#define DOREMI_HOME_VIEW_H

#include <QWidget>
#include <QQuickWidget>
#include <QVariantList>
#include <QVariantMap>
#include "doremi/src/bridge.rs.h"

class HomeView : public QWidget {
    Q_OBJECT
    Q_PROPERTY(QVariantList sections READ sections NOTIFY sectionsChanged)
    Q_PROPERTY(QString viewState READ viewState NOTIFY viewStateChanged)
    Q_PROPERTY(QString welcomeMessage READ welcomeMessage NOTIFY welcomeMessageChanged)
    
public:
    explicit HomeView(QWidget *parent = nullptr);
    void update_theme() {}
    void set_welcome_message(const std::string &msg);
    void add_section(const std::string &title, const std::vector<HomeCard> &items);
    void clear_sections();
    void set_state(const std::string &state, const std::string &message);
    
    QVariantList sections() const { return sections_; }
    QString viewState() const { return view_state_; }
    QString welcomeMessage() const { return welcome_message_; }
    
    // Q_INVOKABLE methods for QML
    Q_INVOKABLE void requestPlay(const QString &id, const QString &type, const QString &title, const QString &artist, const QString &thumbnail);
    Q_INVOKABLE void requestNavigate(const QString &id, const QString &type, const QString &title = "", const QString &subtitle = "", const QString &thumbnail = "");
    Q_INVOKABLE void requestRetry();
signals:
    void play_requested(Track track);
    void album_requested(const std::string &browse_id);
    void artist_requested(const std::string &browse_id);
    void playlist_requested(const std::string &playlist_id,
                            const std::string &title,
                            const std::string &subtitle,
                            const std::string &thumbnail);
    void show_requested(const std::string &browse_id);
    void retry_requested();
    void load_more_requested();
    
    void sectionsChanged();
    void viewStateChanged();
    void welcomeMessageChanged();

private:
    QQuickWidget *quick_widget_ = nullptr;
    QVariantList sections_;
    QString view_state_;
    QString welcome_message_;
};

#endif
