#ifndef TABLEMODEL_H
#define TABLEMODEL_H

#include "samplestorage.h"

#include <QAbstractTableModel>

class TableModel : public QAbstractTableModel
{
public:
    TableModel(SampleStorage& storage_) : storage(storage_) {}

    int rowCount(const QModelIndex & parent = QModelIndex()) const override;
    int columnCount(const QModelIndex & parent = QModelIndex()) const override;
    QVariant data(const QModelIndex & index, int role = Qt::DisplayRole) const override;
    bool insertRows(int row, int count, const QModelIndex & parent = QModelIndex()) override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    void beforeAppend(const Sample &sample);
    void beforeAppendMultiple(const QVector<Sample> &list);
    void afterAppend();
    void afterAppendMultiple();
    void beforeClear();
    void afterClear();
    void beforeDelete(size_t n);
    void afterDelete();

private:
    SampleStorage& storage;
};

#endif // TABLEMODEL_H

