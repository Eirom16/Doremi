#pragma once

#include <QWidget>
#include <QLabel>
#include <QHBoxLayout>

class OfflineBannerWidget : public QWidget {
    Q_OBJECT
public:
    explicit OfflineBannerWidget(QWidget *parent = nullptr);
    ~OfflineBannerWidget() override = default;

    void showBanner();
    void hideBanner();

protected:
    void changeEvent(QEvent *event) override;

private:
    void applyStyle();

    QWidget *m_container = nullptr;
    QLabel *m_icon = nullptr;
    QLabel *m_label = nullptr;
    QLabel *m_badge = nullptr;
};
