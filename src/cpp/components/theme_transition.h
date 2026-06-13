#ifndef DOREMI_THEME_TRANSITION_H
#define DOREMI_THEME_TRANSITION_H

#include <QWidget>
#include <QFrame>
#include <QVBoxLayout>
#include <QLabel>
#include <QProgressBar>
#include <QGraphicsOpacityEffect>
#include <QVariantAnimation>
#include <QPropertyAnimation>
#include <functional>

class ThemeTransitionOverlay : public QWidget {
    Q_OBJECT
public:
    explicit ThemeTransitionOverlay(QWidget *parent = nullptr);
    void start_transition(std::function<void()> on_midpoint);

protected:
    void paintEvent(QPaintEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private:
    void update_styles();

    QFrame *card_;
    QLabel *status_lbl_;
    QProgressBar *progress_bar_;
    QLabel *percent_lbl_;
    QGraphicsOpacityEffect *opacity_effect_;
    QVariantAnimation *progress_anim_;
    QPropertyAnimation *fade_anim_;
    std::function<void()> on_midpoint_callback_;
    bool midpoint_fired_;
};

#endif
