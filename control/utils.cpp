#include "utils.h"
#include <stdint.h>
#include <QMessageBox>

#include <stdio.h>

void showError(QString msg)
{
    QMessageBox msgBox;
    msgBox.setIcon(QMessageBox::Warning);
    msgBox.setWindowTitle("Error");
    msgBox.setText(msg);
    msgBox.exec();
}
