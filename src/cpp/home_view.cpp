#include "home_view.h"
#include <QQmlContext>
#include <QVBoxLayout>
#include <QVariantMap>

HomeView::HomeView(QWidget *parent)
    : QWidget(parent)
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    quick_widget_ = new QQuickWidget(this);
    quick_widget_->setResizeMode(QQuickWidget::SizeRootObjectToView);
    quick_widget_->rootContext()->setContextProperty("HomeCtrl", this);
    
    // Translucent background so main_window background shows
    quick_widget_->setAttribute(Qt::WA_TranslucentBackground);
    quick_widget_->setClearColor(Qt::transparent);
    
    quick_widget_->setSource(QUrl("qrc:/qml/HomeView.qml"));
    
    layout->addWidget(quick_widget_);
    
    view_state_ = "loading";
    welcome_message_ = "¡Bienvenido a Doremi!";
}

void HomeView::set_welcome_message(const std::string &msg) {
    if (welcome_message_.toStdString() != msg) {
        welcome_message_ = QString::fromStdString(msg);
        emit welcomeMessageChanged();
    }
}

void HomeView::add_section(const std::string &title, const std::vector<HomeCard> &items) {
    QVariantList itemsList;
    for (const auto &item : items) {
        QVariantMap map;
        map["id"] = QString::fromStdString(static_cast<std::string>(item.id));
        map["title"] = QString::fromStdString(static_cast<std::string>(item.title));
        map["subtitle"] = QString::fromStdString(static_cast<std::string>(item.subtitle));
        map["thumbnail"] = QString::fromStdString(static_cast<std::string>(item.thumbnail));
        map["itemType"] = QString::fromStdString(static_cast<std::string>(item.item_type));
        itemsList.append(map);
    }
    
    QVariantMap sectionMap;
    sectionMap["title"] = QString::fromStdString(title);
    sectionMap["items"] = itemsList;
    
    sections_.append(sectionMap);
    emit sectionsChanged();
}

void HomeView::clear_sections() {
    sections_.clear();
    emit sectionsChanged();
}

void HomeView::set_state(const std::string &state, const std::string &message) {
    QString newState = QString::fromStdString(state);
    if (view_state_ != newState) {
        view_state_ = newState;
        emit viewStateChanged();
    }
}

void HomeView::requestPlay(const QString &id, const QString &type, const QString &title, const QString &artist, const QString &thumbnail) {
    if (type == "episode") {
        // Episodes can just be played like a song
        Track track;
        track.id = id.toStdString();
        track.title = title.toStdString();
        track.artist = artist.toStdString();
        track.thumbnail = thumbnail.toStdString();
        emit play_requested(track);
    } else {
        Track track;
        track.id = id.toStdString();
        track.title = title.toStdString();
        track.artist = artist.toStdString();
        track.thumbnail = thumbnail.toStdString();
        emit play_requested(track);
    }
}

void HomeView::requestNavigate(const QString &id, const QString &type, const QString &title, const QString &subtitle, const QString &thumbnail) {
    if (id.isEmpty()) return;
    if (type == "album") emit album_requested(id.toStdString());
    else if (type == "artist") emit artist_requested(id.toStdString());
    else if (type == "show") emit show_requested(id.toStdString());
    else emit playlist_requested(id.toStdString(), title.toStdString(), subtitle.toStdString(), thumbnail.toStdString());
}

void HomeView::requestRetry() {
    emit retry_requested();
}
