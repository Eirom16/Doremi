#ifndef DOREMI_WIDGETS_H
#define DOREMI_WIDGETS_H

#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QSlider>
#include <QMenu>
#include <QMouseEvent>
#include <string>
#include <vector>

// Helper C++ functions callable from Rust
QWidget* create_container_widget(QWidget *parent, const char *object_name);
QLabel* create_label(QWidget *parent, const std::string &text, const char *object_name,
                     int font_size, bool bold);
QPushButton* create_icon_button(QWidget *parent, const std::string &icon_text,
                                const char *object_name, int size);
QSlider* create_seek_slider(QWidget *parent);
void set_widget_style(QWidget *widget, const std::string &qss);

// Clickable item widget with context menu support
class ClickableItem : public QWidget {
    Q_OBJECT
public:
    explicit ClickableItem(const std::string &title, const std::string &subtitle,
                           QWidget *parent = nullptr);
    void set_item_id(const std::string &id) { item_id_ = id; }
    // Item kind drives the contextual menu: "song", "video", "artist",
    // "album" or "playlist". Defaults to "song".
    void set_item_type(const std::string &type) { item_type_ = type; }
    std::string item_id() const { return item_id_; }
    std::string title() const { return title_; }
    std::string subtitle() const { return subtitle_; }

signals:
    void clicked();
    void context_action(const std::string &action, const std::string &item_id);

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void enterEvent(QEnterEvent *event) override;
    void leaveEvent(QEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void focusInEvent(QFocusEvent *event) override;
    void focusOutEvent(QFocusEvent *event) override;

private:
    QLabel *title_label_;
    QLabel *sub_label_;
    std::string title_;
    std::string subtitle_;
    std::string item_id_;
    std::string item_type_ = "song";
    void show_context_menu(const QPoint &global_pos);
};

#endif // DOREMI_WIDGETS_H
