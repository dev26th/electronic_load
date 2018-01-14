#include "tablemodel.h"
#include "utils.h"

#include <QDateTime>

// =============================================================================================================

QVariant TableModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if(Qt::Horizontal == orientation) {
        if(Qt::DisplayRole == role) {
            switch(section) {
                case 0: return QVariant("Timestamp");
                case 1: return QVariant("Time, s");
                case 2: return QVariant("Current, A");
                case 3: return QVariant("Voltage, V");
                case 4: return QVariant("Energy, A⋅h");
                case 5: return QVariant("Energy, W⋅h");
            }
        }
    }
    else {
        if(Qt::DisplayRole == role) {
            return QVariant(section + 1);
        }
    }

    return QVariant(); // default
}

int TableModel::rowCount(const QModelIndex & parent) const
{
    (void)parent;
    return storage.size();
}

int TableModel::columnCount(const QModelIndex & parent) const
{
    (void)parent;
    return 6;
}

QVariant TableModel::data(const QModelIndex & index, int role) const
{
    static const QString dateTimeFormat("dd.MM.yyyy hh:mm.ss.zzz");

    if(Qt::DisplayRole == role) {
        const Sample& sample = storage.sample(index.row());
        switch(index.column()) {
            case 0: // full time stamp
                return QVariant(QDateTime::fromMSecsSinceEpoch(sample.timestamp).toString(dateTimeFormat));

            case 1: // seconds since begin
                return QVariant(QString("%L1").arg(((double)(sample.timestamp - storage.getBegin())/1000.0), 0, 'f', 3));

            case 2:
                return QVariant(QString("%L1").arg(sample.i, 0, 'f', 3));

            case 3:
                return QVariant(QString("%L1").arg(sample.u, 0, 'f', 3));

            case 4:
                return QVariant(QString("%L1").arg(sample.ah, 0, 'f', 3));

            case 5:
                return QVariant(QString("%L1").arg(sample.wh, 0, 'f', 3));

            default:
                ; // unexpected
        }
    }
    else if(Qt::TextAlignmentRole == role) {
       return QVariant(Qt::AlignRight | Qt::AlignVCenter);
    }

    return QVariant(); // default
}

bool TableModel::insertRows(int row, int count, const QModelIndex & parent)
{
    (void)row; (void)count; (void)parent;

    return true;
}

void TableModel::beforeAppend(const Sample& sample)
{
    (void)sample;

    beginInsertRows(QModelIndex(), storage.size(), storage.size());
}

void TableModel::afterAppend()
{
    endInsertRows();
}

void TableModel::beforeClear()
{
    beginResetModel();
}

void TableModel::afterClear()
{
    endResetModel();
}

void TableModel::beforeDelete(size_t n)
{
    beginRemoveRows(QModelIndex(), 0, n-1);
}

void TableModel::afterDelete()
{
    endRemoveRows();
}
