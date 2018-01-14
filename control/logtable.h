#ifndef LOGTABLE_H
#define LOGTABLE_H

#include <QTableView>

class LogTable: public QTableView
{
    Q_OBJECT

public:
    explicit LogTable(QWidget *parent = 0) : QTableView(parent) {}

    void setModel(QAbstractItemModel * model) override;

protected slots:
    void rowsAboutToBeInserted(const QModelIndex &parent, int first, int last);

    void rowsInserted(const QModelIndex & parent, int start, int end) override;

private:
    bool autoScroll;
};

#endif // LOGTABLE_H

