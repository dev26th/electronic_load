#ifndef CONFIGDIALOG_H
#define CONFIGDIALOG_H

#include "decoder.h"

#include <QDialog>
#include <QAbstractButton>

namespace Ui {
class ConfigDialog;
}

class ConfigDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ConfigDialog(QWidget *parent, CmdConfigData& deviceConfigData_);
    ~ConfigDialog();

private slots:
    void on_buttonBox_accepted();

    void on_buttonBox_clicked(QAbstractButton *button);

signals:
    void send(QByteArray data);

private:
    bool parseInput();

private:
    Ui::ConfigDialog *ui;
    CmdConfigData& deviceConfigData;
};

#endif // CONFIGDIALOG_H
