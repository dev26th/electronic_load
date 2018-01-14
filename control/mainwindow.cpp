#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "utils.h"
#include "settings.h"
#include "aboutdialog.h"
#include "configdialog.h"

#include <qwt_plot_curve.h>
#include <qwt_plot_grid.h>

#include <QLineEdit>
#include <QVariant>
#include <QMessageBox>
#include <QFontMetrics>
#include <QList>
#include <QSerialPortInfo>
#include <QAbstractTableModel>
#include <QSpinBox>
#include <QFileDialog>
#include <QDebug>

#include <string>
#include <map>
#include <cstdio>

// =============================================================================================================

#define U_THRESHOLD 5

Q_DECLARE_METATYPE(Cmd)
Q_DECLARE_METATYPE(Sample)
Q_DECLARE_METATYPE(Comm::State)

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    isConnected(false),
    deviceConfigData(Cmd::ReadConfig, CmdState::Error),
    storage(Settings::maxSamples)
{
    ui->setupUi(this);

    qRegisterMetaType<Sample>();
    qRegisterMetaType<Cmd>();
    qRegisterMetaType<Comm::State>();

    ui->temperatureBox->setOrientation(Qt::Horizontal);
    ui->temperatureBox->setFillBrush(Qt::green);
    ui->temperatureBox->setScalePosition(QwtThermo::NoScale);
    setControlEnabled(false);

    QSettings settings("Anatoli Klassen", "Electronic Load Control");

    settings.beginGroup("mainwindow");
    restoreGeometry(settings.value("geometry").toByteArray());
    restoreState(settings.value("state").toByteArray());
    settings.endGroup();

    // serial ports
    currentPort = settings.value("port").toString();
    on_refreshPortsButton_clicked();

    // log enabled
    connect(ui->logDataCheckBox, &QCheckBox::stateChanged, [this](int state) {
      this->storage.setEnabled(Qt::Checked == state);
    } );
    ui->logDataCheckBox->setCheckState((Qt::CheckState)settings.value("logData", Qt::Checked).toInt());

    // graph
    ui->graphPlot->setCanvasBackground(QColor("MidnightBlue"));

    QwtPlotGrid *grid = new QwtPlotGrid();
    grid->enableXMin(true);
    grid->setMajorPen(Qt::white, 0, Qt::DotLine);
    grid->setMinorPen(Qt::gray, 0, Qt::DotLine);
    grid->attach(ui->graphPlot);

    data = new CurveData(storage);

    QwtPlotCurve * curve = new QwtPlotCurve();
    curve->setRenderHint(QwtPlotItem::RenderAntialiased);
    curve->setPen(Qt::yellow);
    curve->setData(data);
    curve->attach(ui->graphPlot);

    // storage
    connect(this, &MainWindow::sample, &storage, &SampleStorage::append);
    connect(&storage, &SampleStorage::afterAppend, [this]() {
        if(!this->ui->graphDock->isHidden()) this->ui->graphPlot->replot();
    } );
    connect(&storage, &SampleStorage::afterClear, [this]() {
        if(!this->ui->graphDock->isHidden()) this->ui->graphPlot->replot();
    } );
    connect(ui->graphDock, &QDockWidget::visibilityChanged, [this]() {
        if(!this->ui->graphDock->isHidden()) this->ui->graphPlot->replot();
    } );

    // table
    tableModel = new TableModel(storage);
    ui->tableView->setModel(tableModel);
    ui->tableView->setColumnWidth(0, 140);
    ui->tableView->setColumnWidth(1, 80);
    connect(&storage, &SampleStorage::beforeAppend, tableModel, &TableModel::beforeAppend);
    connect(&storage, &SampleStorage::afterAppend, tableModel, &TableModel::afterAppend);
    connect(&storage, &SampleStorage::beforeClear, tableModel, &TableModel::beforeClear);
    connect(&storage, &SampleStorage::afterClear, tableModel, &TableModel::afterClear);
    connect(&storage, &SampleStorage::beforeDelete, tableModel, &TableModel::beforeDelete);
    connect(&storage, &SampleStorage::afterDelete, tableModel, &TableModel::afterDelete);

    // comm
    comm = new Comm();
    comm->moveToThread(&commThread);
    connect(&commThread, &QThread::started, comm, &Comm::onStart);
    connect(&commThread, &QThread::finished, comm, &Comm::deleteLater);
    connect(this, &MainWindow::portConnect, comm, &Comm::portConnect);
    connect(this, &MainWindow::portDisconnect, comm, &Comm::portDisconnect);
    connect(this, &MainWindow::send, comm, &Comm::send);
    connect(comm, &Comm::error, this, &MainWindow::on_serError);
    connect(comm, &Comm::data, this, &MainWindow::on_serData);
    connect(comm, &Comm::stateChanged, this, &MainWindow::on_serStateChanged);
    commThread.start();

    // interval
    ui->intervalBox->setValue(settings.value("interval", MIN_INTERVAL_MS).toInt());

    // status bar
    deviceVersionLabel = new QLabel();
    deviceVersionLabel->setToolTip("Device Version");
    deviceVersionLabel->setVisible(false);
    ui->statusBar->addWidget(deviceVersionLabel);
    deviceMessageLabel = new QLabel();
    deviceMessageLabel->setToolTip("Device Status");
    deviceMessageLabel->setVisible(false);
    ui->statusBar->addWidget(deviceMessageLabel);
}

MainWindow::~MainWindow()
{
    commThread.quit();
    commThread.wait();

    delete ui;
}

void MainWindow::on_portBox_currentIndexChanged(int index)
{
    (void)index;

    QVariant data = ui->portBox->currentData();
    currentPort = data.toString();
    if(isConnected) connectSer();
}

void MainWindow::on_actionShowTable_triggered()
{
    ui->tableDock->show();
}

void MainWindow::on_actionShowGraph_triggered()
{
    ui->graphDock->show();
}

void MainWindow::on_actionShowControl_triggered()
{
    ui->controlDock->show();
}

void MainWindow::updateFun()
{
    bool showEnergy = (deviceConfigData.fun == 1);
    ui->energyLabel->setVisible(showEnergy);
    ui->energyBox->setVisible(showEnergy);
    ui->energyResetButton->setVisible(showEnergy);
}

void MainWindow::setControlEnabled(bool state)
{
    ui->funFrame->setEnabled(state);
    ui->uLimitBox->setEnabled(state);
    ui->currentBox->setEnabled(state);
    ui->limitUpdateButton->setEnabled(state);
    ui->energyResetButton->setEnabled(state);
    ui->runButton->setEnabled(state);
    ui->menuService->setEnabled(state);

    if(!state) {
        ui->energyLabel->setVisible(true);
        ui->energyBox->setVisible(true);
        ui->energyResetButton->setVisible(true);

        ui->uLimitBox->setValue(0.0);
        ui->currentBox->setValue(0.0);
        ui->uActualBox->setText("");
        ui->energyBox->setText("");
        ui->temperatureBox->setValue(0);
        ui->wireLabel->setVisible(false);
    }

    // FIXME
    ui->funFrame->setEnabled(false);
    ui->runButton->setVisible(false);
}

void MainWindow::connectSer()
{
    emit portConnect(currentPort);

    ui->connectButton->setText("Disconnect");
    isConnected = true;
}

void MainWindow::disconnectSer()
{
    emit portDisconnect();

    ui->connectButton->setText("Connect");
    isConnected = false;
}

void MainWindow::on_connectButton_clicked()
{
    if(isConnected) disconnectSer();
    else            connectSer();
}

void MainWindow::on_refreshPortsButton_clicked()
{
    QString selPort = currentPort;
    ui->portBox->clear();

    QList<QSerialPortInfo> serPorts = QSerialPortInfo::availablePorts();
    int count = 0;
    for(auto i = serPorts.begin(); i != serPorts.end(); ++i, ++count) {
        QString title = QString("%1: (%2) %3").arg(i->portName()).arg(i->description()).arg(i->manufacturer());
        ui->portBox->addItem(title, QVariant(i->portName()));
        if(selPort == i->portName()) {
            ui->portBox->setCurrentIndex(count);
        }
    }
}

void MainWindow::on_clearDataButton_clicked()
{
    storage.clear();
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    // FIXME ask to save log?

    QSettings settings("Anatoli Klassen", "Electronic Load Control");

    settings.beginGroup("mainwindow");
    settings.setValue("geometry", saveGeometry());
    settings.setValue("state", saveState());
    settings.endGroup();

    settings.setValue("port", currentPort);
    settings.setValue("logData", ui->logDataCheckBox->checkState());
    settings.setValue("interval", ui->intervalBox->value());

    QMainWindow::closeEvent(event);
}

void MainWindow::on_actionAbout_triggered()
{
    AboutDialog * dialog = new AboutDialog(this);
    dialog->exec();
}

static bool saveStorageToCsvFile(const SampleStorage &storage, QFile &file)
{
    if(!file.open(QIODevice::WriteOnly | QIODevice::Text)) return false;

    static const QString dateTimeFormat("dd.MM.yyyy hh:mm.ss.zzz");
    QFileDevice::FileError error;
    QString header = "\"Timestamp\",\"Time, s\",\"Current, A\",\"Voltage, V\",\"Energy, Ah\",\"Energy, Wh\"\n";
    file.write(header.toLatin1());
    error = file.error();
    size_t len = storage.size();
    for(size_t i = 0; (QFileDevice::NoError == error) && (i < len); ++i) {
        const Sample& sample = storage.sample(i);
        QString line = QDateTime::fromMSecsSinceEpoch(sample.timestamp).toString(dateTimeFormat);
        line.append(",").append(QString("%1").arg(((double)(sample.timestamp - storage.getBegin())/1000.0), 0, 'f', 3));
        line.append(",").append(QString("%1").arg(sample.i, 0, 'f', 3));
        line.append(",").append(QString("%1").arg(sample.u, 0, 'f', 3));
        line.append(",").append(QString("%1").arg(sample.ah, 0, 'f', 3));
        line.append(",").append(QString("%1").arg(sample.wh, 0, 'f', 3));
        line.append("\n");
        file.write(line.toLatin1());
        error = file.error();
    }
    if(QFileDevice::NoError == error) {
        file.flush();
        error = file.error();
    }

    file.close();
    return (QFileDevice::NoError == error);
}

void MainWindow::on_actionSaveLog_triggered()
{
    QString fileName = QFileDialog::getSaveFileName(this,
        "Save Log", QString(), "CSV tables (*.csv);;Test files (*.txt);;All files (*)");
    if(fileName.isEmpty()) return;

    QFile file(fileName);
    if(!saveStorageToCsvFile(storage, file)) {
        showError("Cannot write into " + fileName + ": " + file.errorString());
    }
}

void MainWindow::on_runButton_clicked()
{
}

void MainWindow::on_serError(QString msg)
{
    showError(msg);
    this->disconnectSer();
}

static void addError(QString& all, const QString& add)
{
    if(!all.isEmpty()) all += " / ";
    all += add;
}

void MainWindow::setupTemperatureBox()
{
    static QColor cLow(29, 114, 29);
    static QColor cMid(207, 207, 46);
    static QColor cFull(238, 154, 0);
    static QColor cLimit(223, 32, 32);

    double min = 1/(double)deviceConfigData.tempDefect;
    double low = 1/(double)deviceConfigData.tempFanLow;
    double mid = 1/(double)deviceConfigData.tempFanMid;
    double full = 1/(double)deviceConfigData.tempFanFull;
    double limit = 1/(double)deviceConfigData.tempLimit;
    double w = limit - min;

    ui->temperatureBox->setValue(min);
    ui->temperatureBox->setScale(min, limit);

    QwtLinearColorMap* cMap = new QwtLinearColorMap();
    cMap->setMode(QwtLinearColorMap::FixedColors);
    cMap->setColorInterval(cLow, cLimit);
    cMap->colorStops().clear();
    cMap->addColorStop(min, cLow);
    cMap->addColorStop((low-min)/w, cMid);
    cMap->addColorStop((mid-min)/w, cFull);
    cMap->addColorStop((full-min)/w, cLimit);
    ui->temperatureBox->setColorMap(cMap);
}

void MainWindow::on_serData(QByteArray d, qint64 timestamp)
{
    CmdData* cmd = parseCmdData(d);
    if(cmd != nullptr) {
        if(cmd->state == CmdState::Response || cmd->state == CmdState::Event) {
            switch(cmd->cmd) {
                case Cmd::Reboot:
                    if(cmd->state == CmdState::Event) // the device was restarted, re-config it
                        configDevice();
                    break;

                case Cmd::ReadConfig:
                    {
                        CmdConfigData* c = static_cast<CmdConfigData*>(cmd);
                        deviceConfigData = *c;

                        if(deviceConfigData.fun == 0)
                            ui->fun1Button->setChecked(true);
                        else
                            ui->fun2Button->setChecked(true);

                        ui->soundBox->setChecked(deviceConfigData.beepOn);
                        setupTemperatureBox();
                        updateFun();
                    }
                    break;

                case Cmd::ReadSettings:
                    {
                        CmdSettingData* c = static_cast<CmdSettingData*>(cmd);
                        ui->uLimitBox->setValue((double)c->u / 1000);
                        ui->currentBox->setValue((double)c->i / 1000);
                        deviceCurrent = c->i;
                    }
                    break;

                case Cmd::GetVersion:
                    {
                        CmdVersionData* c = static_cast<CmdVersionData*>(cmd);
                        deviceVersionLabel->setText("0x" + QString("%1").arg(c->v, 8, 16, QChar('0')).toUpper());
                        deviceVersionLabel->setVisible(true);
                    }
                    break;

                case Cmd::GetState:
                    {
                        CmdStateData* c = static_cast<CmdStateData*>(cmd);

                        Sample s;
                        s.timestamp = ((timestamp + this->interval/2) / this->interval) * this->interval; // round up to interval borders

                        bool is4Wire = (c->uSense >= c->uCurrent);
                        uint16_t u = (is4Wire ? c->uSense : c->uCurrent);
                        qDebug() << u << deviceLastU << abs((int)u - (int)deviceLastU);
                        if(abs((int)u - (int)deviceLastU) < U_THRESHOLD)
                            u = deviceLastU;
                        else
                            deviceLastU = u;

                        s.u = qFloor((double)u / 10.0 + 0.5) / 100.0; // FIXME good? FIXME very seldom different value as on the device display
                        //s.u = (double)u / 1000; // FIXME good? FIXME very seldom different value as on the device display
                        s.i = (double)deviceCurrent / 1000;
                        s.ah = (double)c->ah / 1000;
                        s.wh = (double)c->wh / 1000;

                        if(c->mode == DeviceMode::Fun1Run || c->mode == DeviceMode::Fun2Run)
                            emit sample(s);

                        QString deviceMessage;
                        if(c->error) {
                            if(c->error & DEVICE_ERROR_POLARITY) addError(deviceMessage, "Polarity error");
                            if(c->error & DEVICE_ERROR_SUPPLY)   addError(deviceMessage, "Supply error");
                            if(c->error & DEVICE_ERROR_OUP)      addError(deviceMessage, "Overvoltage");
                            if(c->error & DEVICE_ERROR_OTP)      addError(deviceMessage, "Overheat");
                            if(c->error & DEVICE_ERROR_ERT)      addError(deviceMessage, "Temperature sensor defect");
                        }
                        else {
                            switch(c->mode) {
                                case DeviceMode::Booting:  deviceMessage = "Booting"; break;
                                case DeviceMode::MenuFun:  deviceMessage = "Menu";    break;
                                case DeviceMode::MenuBeep: deviceMessage = "Menu";    break;
                                case DeviceMode::Fun1:     deviceMessage = "Idle";    break;
                                case DeviceMode::Fun1Run:  deviceMessage = "Run";     break;
                                case DeviceMode::Fun2:     deviceMessage = "Idle";    break;
                                case DeviceMode::Fun2Pre:  deviceMessage = "Run";     break;
                                case DeviceMode::Fun2Run:  deviceMessage = "Run";     break;
                                case DeviceMode::Fun2Warn: deviceMessage = "Stop";    break;
                                case DeviceMode::Fun2Res:  deviceMessage = "Stop";    break;
                            }
                        }
                        deviceMessageLabel->setText(deviceMessage);
                        deviceMessageLabel->setVisible(true);

                        ui->uActualBox->setText(QString("%L1 V").arg(s.u, 0, 'f', 3));
                        ui->energyBox->setText(QString("%L1 A⋅h (%L2 W⋅h)").arg(s.ah, 0, 'f', 3).arg(s.wh, 0, 'f', 3));
                        ui->wireLabel->setVisible(is4Wire);
                        ui->temperatureBox->setValue(1/(double)c->tempRaw);
                    }
                    break;

                default:
                    ;
            }
        }
    }
    executeNext();
}

void MainWindow::on_serStateChanged(Comm::State state)
{
    switch(state) {
        case Comm::State::Connected:
            setControlEnabled(true);
            configDevice();
            deviceLastU = 0xFFFF;
            break;

        case Comm::State::Idle:
            setControlEnabled(false);
            deviceVersionLabel->setVisible(false);
            deviceVersionLabel->clear();
            deviceMessageLabel->setVisible(false);
            deviceMessageLabel->clear();
            break;
    }
}

void MainWindow::configDevice()
{
    this->interval = ui->intervalBox->value();

    toExecute.clear();
    toExecute.enqueue(formCmdData(Cmd::GetVersion));
    toExecute.enqueue(formCmdData(Cmd::ReadConfig));
    toExecute.enqueue(formCmdData(Cmd::ReadSettings));
    toExecute.enqueue(formCmdData(CmdFlowStateData(this->interval)));
    executeNext();
}

void MainWindow::executeNext()
{
    if(!toExecute.isEmpty())
        emit send(toExecute.dequeue());
}

void MainWindow::on_limitUpdateButton_clicked()
{
    CmdSettingData d(Cmd::WriteSettings, CmdState::Request);
    d.u = (uint16_t)(ui->uLimitBox->value() * 1000 + 0.5);
    d.i = (uint16_t)(ui->currentBox->value() * 1000 + 0.5);
    emit send(formCmdData(d));
}

void MainWindow::on_actionDeviceConfiguration_triggered()
{
    CmdConfigData d = deviceConfigData;
    ConfigDialog * dialog = new ConfigDialog(this, d);
    if(1 == dialog->exec()) {
        d.cmd = Cmd::WriteConfig;
        d.state = CmdState::Request;
        emit send(formCmdData(d));
    }
}

void MainWindow::on_energyResetButton_clicked()
{
    emit send(formCmdData(Cmd::ResetState));
}
