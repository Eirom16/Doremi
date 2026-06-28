#include "loading_state.h"
#include "skeleton_loader.h"
#include <QVBoxLayout>
#include <QHBoxLayout>

LoadingState::LoadingState(Mode mode, QWidget *parent)
    : QWidget(parent)
    , mode_(mode)
{
    rebuild();
}

void LoadingState::setRowCount(int count) {
    row_count_ = count;
    rebuild();
}

void LoadingState::setRowHeight(int height) {
    row_height_ = height;
    rebuild();
}

void LoadingState::setGridRows(int rows) {
    grid_rows_ = rows;
    rebuild();
}

void LoadingState::setGridColumns(int cols) {
    grid_cols_ = cols;
    rebuild();
}

void LoadingState::setCardSize(QSize size) {
    card_size_ = size;
    rebuild();
}

void LoadingState::rebuild() {
    while (auto *child = findChild<QWidget *>(QString(), Qt::FindDirectChildrenOnly))
        delete child;

    auto *outer = new QVBoxLayout(this);
    outer->setContentsMargins(0, 0, 0, 0);
    outer->setSpacing(0);

    if (mode_ == ListRows) {
        auto *list = new QVBoxLayout();
        list->setSpacing(10);
        for (int i = 0; i < row_count_; ++i) {
            auto *sk = new SkeletonLoader(this);
            sk->setFixedHeight(row_height_);
            list->addWidget(sk);
        }
        list->addStretch();
        outer->addLayout(list);
    } else {
        for (int r = 0; r < grid_rows_; ++r) {
            auto *row = new QHBoxLayout();
            row->setSpacing(12);
            for (int c = 0; c < grid_cols_; ++c) {
                auto *sk = new SkeletonLoader(this);
                sk->setFixedSize(card_size_);
                row->addWidget(sk);
            }
            row->addStretch();
            outer->addLayout(row);
        }
        outer->addStretch();
    }
}
