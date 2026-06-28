#pragma once

#include <QWidget>
#include <QSize>

class LoadingState : public QWidget {
    Q_OBJECT
public:
    enum Mode { ListRows, GridCards };

    explicit LoadingState(Mode mode, QWidget *parent = nullptr);

    void setRowCount(int count);
    void setRowHeight(int height);
    void setGridRows(int rows);
    void setGridColumns(int cols);
    void setCardSize(QSize size);

private:
    void rebuild();
    Mode mode_;
    int row_count_ = 8;
    int row_height_ = 72;
    int grid_rows_ = 2;
    int grid_cols_ = 5;
    QSize card_size_{160, 210};
};
