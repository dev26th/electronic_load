#include "logtable.h"

void LogTable::setModel(QAbstractItemModel * model)
{
    QAbstractItemModel * oldModel = this->model();
    if(oldModel == model)
        return;

    if(oldModel)
        disconnect(oldModel, &QAbstractItemModel::rowsAboutToBeInserted, this, &LogTable::rowsAboutToBeInserted);

    QTableView::setModel(model);

    QAbstractItemModel * newModel = this->model();
    if(newModel)
        connect(newModel, &QAbstractItemModel::rowsAboutToBeInserted, this, &LogTable::rowsAboutToBeInserted);
}

void LogTable::rowsAboutToBeInserted(const QModelIndex &parent, int first, int last)
{
    (void)parent; (void)last;

    int lastVisibleRow = rowAt(height() - rowHeight(first - 1));
    autoScroll = (lastVisibleRow == (int)(first - 1)); // if last row was visible before insert visible
}

void LogTable::rowsInserted(const QModelIndex & parent, int start, int end)
{
    (void)parent; (void)start; (void)end;

    if(autoScroll)
        scrollToBottom();
}
