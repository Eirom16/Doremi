#ifndef DOREMI_CREATE_PLAYLIST_DIALOG_H
#define DOREMI_CREATE_PLAYLIST_DIALOG_H

#include <QDialog>
#include <QLineEdit>
#include <QTextEdit>
#include <QComboBox>
#include <QString>

class CreatePlaylistDialog : public QDialog {
    Q_OBJECT
public:
    explicit CreatePlaylistDialog(QWidget *parent = nullptr);
    QString playlistName() const;
    QString description() const;
    QString privacy() const;

private:
    QLineEdit *name_edit_;
    QTextEdit *desc_edit_;
    QComboBox *privacy_combo_;
};

#endif