#include "flashprogressdialog.h"
#include "ui_flashprogressdialog.h"

FlashProgressDialog::FlashProgressDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::FlashProgressDialog)
{
    ui->setupUi(this);
}

FlashProgressDialog::~FlashProgressDialog()
{
    delete ui;
}

void FlashProgressDialog::on_progress(double percent)
{
    ui->progressBar->setValue((int)(percent + 0.5));
}

void FlashProgressDialog::on_buttonBox_rejected()
{
    emit cancel();
}

void FlashProgressDialog::on_stateChanged(Flasher::State state)
{
    switch(state) {
        case Flasher::State::Connected:
            ui->stateLabel->setText("Waiting for bootloader...");
            break;

        case Flasher::State::Preparing:
            ui->stateLabel->setText("Preparing...");
            break;

        case Flasher::State::Programming:
            ui->stateLabel->setText("Programming...");
            break;

        case Flasher::State::Verifying:
            ui->stateLabel->setText("Verifying...");
            break;

        case Flasher::State::Resetting:
            ui->stateLabel->setText("Resetting...");
            break;

        case Flasher::State::Ready:
            ui->stateLabel->setText("Done");
            ui->buttonBox->setStandardButtons(QDialogButtonBox::Close);
            break;

        case Flasher::State::Idle:
            ui->stateLabel->setText("Idle");
            break;
    }
}
