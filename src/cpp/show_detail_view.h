#ifndef DOREMI_SHOW_DETAIL_VIEW_H
#define DOREMI_SHOW_DETAIL_VIEW_H

#include <QWidget>
#include <QVBoxLayout>
#include <QLabel>
#include <QHBoxLayout>
#include <QPushButton>
#include <QScrollArea>
#include <QMouseEvent>
#include <QContextMenuEvent>
#include <QMenu>
#include <vector>
#include <string>
#include "components/track_row.h"
#include "doremi/src/bridge.rs.h"


class ShowDetailView : public QWidget {
    Q_OBJECT
public:
    explicit ShowDetailView(QWidget *parent = nullptr);

    void set_show_info(const Show &show);
    void set_episodes(const std::vector<Episode> &episodes);
    void clear();
    void update_theme();

signals:
    void back_requested();
    void play_episode_requested(Episode episode);

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;

private:
    void setupLayout();
    void updateSubscriptionButtonState(bool subscribed);

    QVBoxLayout *content_layout_;

    QLabel *cover_label_;
    QLabel *title_label_;
    QLabel *author_label_;
    QLabel *description_label_;
    QLabel *episode_count_label_;
    QPushButton *subscribe_btn_;

    QWidget *episodes_widget_;
    QVBoxLayout *episodes_layout_;
    std::vector<Episode> episodes_;
    Show current_show_;
    bool is_subscribed_;
};

#endif
