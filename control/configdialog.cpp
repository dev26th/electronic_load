#include "configdialog.h"
#include "ui_configdialog.h"

#include "utils.h"

ConfigDialog::ConfigDialog(QWidget *parent, CmdConfigData& deviceConfigData_) :
    QDialog(parent),
    ui(new Ui::ConfigDialog),
    deviceConfigData(deviceConfigData_)
{
    ui->setupUi(this);

    ui->iSetCoefOffsetBox->setText(QString::number(deviceConfigData.iSetCoef.offset));
    ui->iSetCoefMulBox->setText(QString::number(deviceConfigData.iSetCoef.mul));
    ui->iSetCoefDivBox->setText(QString::number(deviceConfigData.iSetCoef.div));

    ui->uCurCoefOffsetBox->setText(QString::number(deviceConfigData.uCurCoef.offset));
    ui->uCurCoefMulBox->setText(QString::number(deviceConfigData.uCurCoef.mul));
    ui->uCurCoefDivBox->setText(QString::number(deviceConfigData.uCurCoef.div));

    ui->uSenseCoefOffsetBox->setText(QString::number(deviceConfigData.uSenseCoef.offset));
    ui->uSenseCoefMulBox->setText(QString::number(deviceConfigData.uSenseCoef.mul));
    ui->uSenseCoefDivBox->setText(QString::number(deviceConfigData.uSenseCoef.div));

    ui->tempFanLowBox->setText(QString::number(deviceConfigData.tempFanLow));
    ui->tempFanMidBox->setText(QString::number(deviceConfigData.tempFanMid));
    ui->tempFanFullBox->setText(QString::number(deviceConfigData.tempFanFull));
    ui->tempLimitBox->setText(QString::number(deviceConfigData.tempLimit));
    ui->tempDefectBox->setText(QString::number(deviceConfigData.tempDefect));
    ui->tempThresholdBox->setText(QString::number(deviceConfigData.tempThreshold));

    ui->uSupMinBox->setText(QString::number(deviceConfigData.uSupMin));
    ui->iSetMinBox->setText(QString::number(deviceConfigData.iSetMin));
    ui->iSetMaxBox->setText(QString::number(deviceConfigData.iSetMax));
    ui->uSetMinBox->setText(QString::number(deviceConfigData.uSetMin));
    ui->uSetMaxBox->setText(QString::number(deviceConfigData.uSetMax));
    ui->uSenseMinBox->setText(QString::number(deviceConfigData.uSenseMin));
    ui->uNegativeBox->setText(QString::number(deviceConfigData.uNegative));
    ui->uCurLimitBox->setText(QString::number(deviceConfigData.uCurLimit));
    ui->powLimitBox->setText(QString::number(deviceConfigData.powLimit));
    ui->ahMaxBox->setText(QString::number(deviceConfigData.ahMax));
    ui->whMaxBox->setText(QString::number(deviceConfigData.whMax));

    ui->funBox->setText(QString::number(deviceConfigData.fun));
    ui->beepOnBox->setText(QString::number(deviceConfigData.beepOn));
    ui->uSetBox->setText(QString::number(deviceConfigData.uSet));
    ui->iSetBox->setText(QString::number(deviceConfigData.iSet));
}

ConfigDialog::~ConfigDialog()
{
    delete ui;
}

static int parseInt(const QString& s)
{
    bool ok = false;
    int res = s.toInt(&ok);
    if(!ok) throw s;
    return res;
}

void ConfigDialog::on_saveButton_clicked()
{
    try {
        deviceConfigData.iSetCoef.offset = parseInt(ui->iSetCoefOffsetBox->text());
        deviceConfigData.iSetCoef.mul = parseInt(ui->iSetCoefMulBox->text());
        deviceConfigData.iSetCoef.div = parseInt(ui->iSetCoefDivBox->text());

        deviceConfigData.uCurCoef.offset = parseInt(ui->uCurCoefOffsetBox->text());
        deviceConfigData.uCurCoef.mul = parseInt(ui->uCurCoefMulBox->text());
        deviceConfigData.uCurCoef.div = parseInt(ui->uCurCoefDivBox->text());

        deviceConfigData.uSenseCoef.offset = parseInt(ui->uSenseCoefOffsetBox->text());
        deviceConfigData.uSenseCoef.mul = parseInt(ui->uSenseCoefMulBox->text());
        deviceConfigData.uSenseCoef.div = parseInt(ui->uSenseCoefDivBox->text());

        deviceConfigData.tempFanLow = parseInt(ui->tempFanLowBox->text());
        deviceConfigData.tempFanMid = parseInt(ui->tempFanMidBox->text());
        deviceConfigData.tempFanFull = parseInt(ui->tempFanFullBox->text());
        deviceConfigData.tempLimit = parseInt(ui->tempLimitBox->text());
        deviceConfigData.tempDefect = parseInt(ui->tempDefectBox->text());
        deviceConfigData.tempThreshold = parseInt(ui->tempThresholdBox->text());

        deviceConfigData.uSupMin = parseInt(ui->uSupMinBox->text());
        deviceConfigData.iSetMin = parseInt(ui->iSetMinBox->text());
        deviceConfigData.iSetMax = parseInt(ui->iSetMaxBox->text());
        deviceConfigData.uSetMin = parseInt(ui->uSetMinBox->text());
        deviceConfigData.uSetMax = parseInt(ui->uSetMaxBox->text());
        deviceConfigData.uSenseMin = parseInt(ui->uSenseMinBox->text());
        deviceConfigData.uNegative = parseInt(ui->uNegativeBox->text());
        deviceConfigData.uCurLimit = parseInt(ui->uCurLimitBox->text());
        deviceConfigData.powLimit = parseInt(ui->powLimitBox->text());
        deviceConfigData.ahMax = parseInt(ui->ahMaxBox->text());
        deviceConfigData.whMax = parseInt(ui->whMaxBox->text());

        deviceConfigData.fun = parseInt(ui->funBox->text());
        deviceConfigData.beepOn = parseInt(ui->beepOnBox->text());
        deviceConfigData.uSet = parseInt(ui->uSetBox->text());
        deviceConfigData.iSet = parseInt(ui->iSetBox->text());

        accept();
    }
    catch(const QString& ex) {
        showError(QString("Cannot parse: %1").arg(ex));
    }
}
