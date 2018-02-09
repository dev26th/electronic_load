#ifndef FLASHPROGRESSDIALOG_H
#define FLASHPROGRESSDIALOG_H

#include <QDialog>
#include "flasher.h"

namespace Ui {
class FlashProgressDialog;
}

class FlashProgressDialog : public QDialog
{
    Q_OBJECT

public:
    explicit FlashProgressDialog(QWidget *parent = 0);
    ~FlashProgressDialog();

public slots:
    void on_progress(double percent);
    void on_stateChanged(Flasher::State state);

signals:
    void cancel();

private slots:
    void on_buttonBox_rejected();

private:
    Ui::FlashProgressDialog *ui;
};

#endif // FLASHPROGRESSDIALOG_H
