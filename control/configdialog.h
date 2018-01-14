#ifndef CONFIGDIALOG_H
#define CONFIGDIALOG_H

#include "decoder.h"

#include <QDialog>

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
    void on_saveButton_clicked();

private:
    Ui::ConfigDialog *ui;
    CmdConfigData& deviceConfigData;
};

#endif // CONFIGDIALOG_H
