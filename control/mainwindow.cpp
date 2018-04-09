#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "utils.h"
#include "settings.h"
#include "aboutdialog.h"
#include "configdialog.h"
#include "flashprogressdialog.h"
#include "crc.h"

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
    connect(this, &MainWindow::sampleMultiple, &storage, &SampleStorage::appendMultiple);
    connect(&storage, &SampleStorage::afterAppend, [this]() {
        if(!this->ui->graphDock->isHidden()) this->ui->graphPlot->replot();
    } );
    connect(&storage, &SampleStorage::afterAppendMultiple, [this]() {
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
    connect(&storage, &SampleStorage::beforeAppendMultiple, tableModel, &TableModel::beforeAppendMultiple);
    connect(&storage, &SampleStorage::afterAppend, tableModel, &TableModel::afterAppend);
    connect(&storage, &SampleStorage::afterAppendMultiple, tableModel, &TableModel::afterAppendMultiple);
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

    // flasher
    flasher = new Flasher();
    flasher->moveToThread(&flasherThread);
    connect(&flasherThread, &QThread::started, flasher, &Flasher::onStart);
    connect(&flasherThread, &QThread::finished, flasher, &Flasher::deleteLater);
    connect(this, &MainWindow::upgradeDevice, flasher, &Flasher::portConnect);
    connect(this, &MainWindow::cancelUpgradeDevice, flasher, &Flasher::portDisconnect);
    connect(flasher, &Flasher::error, this, &MainWindow::on_flasherError);
    flasherThread.start();

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

    clearDeviceInfo();
}

MainWindow::~MainWindow()
{
    commThread.quit();
    flasherThread.quit();

    commThread.wait();
    flasherThread.wait();

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
    ui->actionDeviceConfiguration->setEnabled(state);

    if(state) {
        bool showEnergy = (deviceConfigData.fun == 1);
        ui->energyLabel->setVisible(showEnergy);
        ui->energyBox->setVisible(showEnergy);
        ui->energyResetButton->setVisible(showEnergy);
    }
    else {
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
    clearDeviceInfo();
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
    delete dialog;
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
    disconnectSer();
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

void MainWindow::updateDeviceSettings() {
    setupTemperatureBox();
    ui->uLimitBox->setMinimum((double)deviceConfigData.uSetMin / 1000.0);
    ui->uLimitBox->setMaximum((double)deviceConfigData.uSetMax / 1000.0);
    ui->currentBox->setMinimum((double)deviceConfigData.iSetMin / 1000.0);
    ui->currentBox->setMaximum((double)deviceConfigData.iSetMax / 1000.0);
}

Sample MainWindow::parseSample(CmdStateData *c, qint64 timestamp)
{
    Sample s;
    if(this->interval > 0)
        s.timestamp = ((timestamp + this->interval/2) / this->interval) * this->interval; // round up to interval borders
    else
        s.timestamp = timestamp;

    bool is4Wire = (c->uSense + 100 >= c->uMain);
    uint16_t u = (is4Wire ? c->uSense : c->uMain);
    if(abs((int)u - (int)deviceLastU) < U_THRESHOLD)
        u = deviceLastU;
    else
        deviceLastU = u;

    s.u = qFloor((double)u / 10.0 + 0.5) / 100.0; // FIXME good? or s.u = (double)u / 1000;
    s.i = (double)deviceCurrent / 1000;
    s.ah = (double)c->ah / 1000;
    s.wh = (double)c->wh / 1000;

    return s;
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
                        updateFun();
                        updateDeviceSettings();
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
                        Sample s = parseSample(c, timestamp);
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
                                case DeviceMode::Booting:  deviceMessage = "Booting";     break;
                                case DeviceMode::MenuFun:  deviceMessage = "Menu";        break;
                                case DeviceMode::MenuBeep: deviceMessage = "Menu";        break;
                                case DeviceMode::MenuCalV: deviceMessage = "Menu";        break;
                                case DeviceMode::MenuCalI: deviceMessage = "Menu";        break;
                                case DeviceMode::CalV1:    deviceMessage = "Calibration"; break;
                                case DeviceMode::CalV2:    deviceMessage = "Calibration"; break;
                                case DeviceMode::CalI1r:   deviceMessage = "Calibration"; break;
                                case DeviceMode::CalI1v:   deviceMessage = "Calibration"; break;
                                case DeviceMode::CalI2r:   deviceMessage = "Calibration"; break;
                                case DeviceMode::CalI2v:   deviceMessage = "Calibration"; break;
                                case DeviceMode::Fun1:     deviceMessage = "Idle";        break;
                                case DeviceMode::Fun1Run:  deviceMessage = "Run";         break;
                                case DeviceMode::Fun2:     deviceMessage = "Idle";        break;
                                case DeviceMode::Fun2Pre:  deviceMessage = "Run";         break;
                                case DeviceMode::Fun2Run:  deviceMessage = "Run";         break;
                                case DeviceMode::Fun2Warn: deviceMessage = "Stop";        break;
                                case DeviceMode::Fun2Res:  deviceMessage = "Stop";        break;
                            }
                        }
                        deviceMessageLabel->setText(deviceMessage);
                        deviceMessageLabel->setVisible(true);

                        bool is4Wire = (c->uSense + 100 >= c->uMain);
                        ui->uActualBox->setText(QString("%L1 V").arg(s.u, 0, 'f', 2));
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
    executeNext();
}

void MainWindow::configDevice()
{
    this->interval = ui->intervalBox->value();

    toExecute.clear();
    toExecute.enqueue(ToExecute(ToExecute::Action::Send, formCmdData(Cmd::GetVersion)));
    toExecute.enqueue(ToExecute(ToExecute::Action::Send, formCmdData(Cmd::ReadConfig)));
    toExecute.enqueue(ToExecute(ToExecute::Action::Send, formCmdData(Cmd::ReadSettings)));
    toExecute.enqueue(ToExecute(ToExecute::Action::Send, formCmdData(CmdFlowStateData(this->interval))));
    executeNext();
}

void MainWindow::executeNext()
{
    if(toExecute.isEmpty()) return;

    ToExecute e = toExecute.dequeue();
    switch(e.action) {
        case ToExecute::Action::Send:
            emit send(e.data);
            break;

    case ToExecute::Action::Disconnect:
        disconnectSer();
        break;


    case ToExecute::Action::StartUpgrade:
        startUpgrade(e.data);
        break;
    }
}

void MainWindow::on_limitUpdateButton_clicked()
{
    CmdSettingData d(Cmd::WriteSettings, CmdState::Request);
    d.u = (uint16_t)(ui->uLimitBox->value() * 1000 + 0.5);
    d.i = (uint16_t)(ui->currentBox->value() * 1000 + 0.5);
    toExecute.enqueue(ToExecute(ToExecute::Action::Send, formCmdData(d)));
    toExecute.enqueue(ToExecute(ToExecute::Action::Send, formCmdData(Cmd::ReadSettings)));
    executeNext();
}

void MainWindow::on_uLimitBox_editingFinished()
{
    on_limitUpdateButton_clicked();
}

void MainWindow::on_currentBox_editingFinished()
{
    on_limitUpdateButton_clicked();
}

void MainWindow::on_actionDeviceConfiguration_triggered()
{
    CmdConfigData d = deviceConfigData;
    ConfigDialog * dialog = new ConfigDialog(this, d);
    connect(dialog, &ConfigDialog::send, comm, &Comm::send);
    if(1 == dialog->exec()) {
        toExecute.enqueue(ToExecute(ToExecute::Action::Send, formCmdData(d)));
        toExecute.enqueue(ToExecute(ToExecute::Action::Send, formCmdData(Cmd::ReadConfig)));
        executeNext();
    }
    delete dialog;
}

void MainWindow::on_energyResetButton_clicked()
{
    toExecute.enqueue(ToExecute(ToExecute::Action::Send, formCmdData(Cmd::ResetState)));
    executeNext();
}

// note: length not checked
static uint8_t parse_hex(const QChar& d1, const QChar& d2) {
    uint8_t d[2];
    for(int i = 0; i < 2; ++i) {
        char c = (i==0?d1:d2).toLatin1();
        if(c >= '0' && c <= '9') d[i] = c - '0';
        else if(c >= 'A' && c <= 'F') d[i] = c - 'A' + 10;
        else if(c >= 'a' && c <= 'f') d[i] = c - 'a' + 10;
        else return 0; // error
    }
    return (d[0] << 4) | (d[1]);
}

static void parse_ihex(QByteArray& hexFile, uint8_t erased_pattern, QByteArray& out, uint32_t& begin) {
    begin = UINT32_MAX;
    uint32_t end = 0;
    bool eof_found = false;

    for(int scan = 0; scan < 2; ++scan) { // parse file two times - first to find memory range, second - to fill it
        if(scan == 1) {
            if(!eof_found)
                throw "No EoF recond";

            if(begin >= end)
                throw "No data found in file";

            out.fill(erased_pattern, (end - begin) + 1);
        }

        QTextStream stream(&hexFile);
        uint32_t lba = 0;

        while(!stream.atEnd()) {
            QString line = stream.readLine();
            if(line[0] == '\n' || line[0] == '\r') continue; // skip empty lines
            if(line[0] != ':') // no marker - wrong file format
                throw "Wrong file format - no marker";

            int l = line.length();
            while(l > 0 && (line[l-1] == '\n' || line[l-1] == '\r')) --l; // trim EoL
            if(l < 11) // line too short - wrong file format
                throw "Wrong file format - wrong line length";

            // check sum
            uint8_t chksum = 0;
            for(int i = 1; i < l-1; i += 2) {
                chksum += parse_hex(line[i], line[i+1]);
            }
            if(chksum != 0)
                throw "Wrong file format - checksum mismatch";

            uint8_t reclen = parse_hex(line[1], line[2]);
            if(((uint32_t)reclen + 5)*2 + 1 != (uint32_t)l)
                throw "Wrong file format - record length mismatch";

            uint16_t offset  = ((uint16_t)parse_hex(line[3], line[4]) << 8) | ((uint16_t)parse_hex(line[5], line[6]));
            uint8_t  rectype = parse_hex(line[7], line[8]);

            switch(rectype) {
                case 0: // data
                    if(scan == 0) {
                        uint32_t b = lba + offset;
                        uint32_t e = b + reclen - 1;
                        if(b < begin) begin = b;
                        if(e > end) end = e;
                    }
                    else {
                        for(size_t i = 0; i < reclen; ++i) {
                            uint8_t b = parse_hex(line[(uint)(9 + i*2)], line[(uint)(10 + i*2)]);
                            uint32_t addr = lba + offset + i;
                            if(addr >= begin && addr <= end) {
                                out[addr - begin] = b;
                            }
                        }
                    }
                    break;

                case 1: // EoF
                    eof_found = true;
                    break;

                case 2: // Extended Segment Address, unexpected
                    throw "Unexpected";

                case 3: // Start Segment Address, unexpected
                    throw "Unexpected";

                case 4: // Extended Linear Address
                    if(reclen == 2)
                        lba = ((uint32_t)parse_hex(line[9], line[10]) << 24) | ((uint32_t)parse_hex(line[11], line[12]) << 16);
                    else
                        throw "Wrong file format - wrong LBA length";
                    break;

                case 5: // Start Linear Address - expected, but ignore
                    break;

                default:
                    throw "Wrong file format - unexpected record type";
            }
        }
    }
}

void MainWindow::on_actionUpgradeFirmware_triggered()
{
    QString fileName = QFileDialog::getOpenFileName(this,
        tr("Select firmware file"), "",
        tr("Intel-HEX Files (*.ihex *.ihx *.hex);;Binary Files (*.bin);;All Files (*)"));
    if(fileName.isEmpty()) return;

    QByteArray fileContent;
    QFile inputFile(fileName);
    if(!inputFile.open(QFile::ReadOnly)) {
        showError(QString("Cannot read from file %1:\n%2.").arg(fileName).arg(inputFile.errorString()));
        return;
    }
    else {
        try {
            fileContent = inputFile.readAll();
        }
        catch(char const* msg) {
            showError(QString("Cannot read from file %1:\n%2.").arg(fileName).arg(msg));
            return;
        }
    }

    QString fileNameLc = fileName.toLower();
    if(fileNameLc.endsWith(".ihex") || fileNameLc.endsWith(".ihx") || fileNameLc.endsWith(".hex")) {
        uint32_t begin;
        QByteArray bin;

        try {
            parse_ihex(fileContent, 0xFF, bin, begin);
        }
        catch(char const* msg) {
            showError(QString("Cannot parse file %1 is Intel-HEX:\n%2.").arg(fileName).arg(msg));
            return;
        }

        fileContent = bin;
    }

    if(isConnected) {
        toExecute.clear();
        toExecute.enqueue(ToExecute(ToExecute::Action::Send, formCmdData(CmdBootloaderData(true))));
        toExecute.enqueue(ToExecute(ToExecute::Action::Send, formCmdData(Cmd::Reboot)));
        toExecute.enqueue(ToExecute(ToExecute::Action::Disconnect));
    }

    toExecute.enqueue(ToExecute(ToExecute::Action::StartUpgrade, fileContent));
    executeNext();
}

void MainWindow::startUpgrade(const QByteArray &data)
{
    FlashProgressDialog * dialog = new FlashProgressDialog(this);
    connect(flasher, &Flasher::progress, dialog, &FlashProgressDialog::on_progress);
    connect(flasher, &Flasher::stateChanged, dialog, &FlashProgressDialog::on_stateChanged);

    emit upgradeDevice(currentPort, data);
    dialog->exec();
    emit cancelUpgradeDevice();

    delete dialog;
    executeNext();
}

void MainWindow::clearDeviceInfo()
{
    memset(&deviceConfigData, 0, sizeof(deviceConfigData));
}

void MainWindow::on_flasherError(QString msg)
{
    emit cancelUpgradeDevice();
    showError(msg);
}

void MainWindow::on_actionLoadRawLog_triggered()
{
    QString fileName = QFileDialog::getOpenFileName(this,
        tr("Select RAW-log file"), "",
        tr("Text Files (*.txt *.raw *.log);;All Files (*)"));
    if(fileName.isEmpty()) return;

    QFile inputFile(fileName);
    int skippedLines = 0;
    QVector<Sample> list;
    if(inputFile.open(QIODevice::ReadOnly)) {
        QTextStream in(&inputFile);
        for(;;) {
            QString line = in.readLine();
            if(line.isNull()) break;

            QStringList tokens = line.split(',');
            if(tokens.length() != 2) {
                ++skippedLines;
                continue;
            }

            bool ok;
            qint64 timestamp = tokens[0].toLongLong(&ok);
            if(!ok) {
                ++skippedLines;
                continue;
            }

            if(!tokens[1].startsWith("s") && !tokens[1].startsWith("S")) {
                ++skippedLines;
                continue;
            }

            QByteArray data = QByteArray::fromHex(tokens[1].toUtf8()); // 'S' will be skipped
            char crc = std::accumulate(data.begin(), data.end(), 0, crc8);
            if(crc != 0) {
                ++skippedLines;
                continue;
            }

            QByteArray buf;
            buf.append(data.data(), data.size()-1);

            CmdData* cmd = parseCmdData(buf);
            if(cmd != nullptr && cmd->state == CmdState::Event && cmd->cmd == Cmd::GetState) {
                CmdStateData* c = static_cast<CmdStateData*>(cmd);
                Sample s = parseSample(c, timestamp * 1000);
                if(c->mode == DeviceMode::Fun1Run || c->mode == DeviceMode::Fun2Run)
                    list.push_back(s);
            }
        }
        inputFile.close();

        if(!list.isEmpty())
            emit sampleMultiple(list);

        if(skippedLines > 0)
            showError(QString("Read successfully, but %1 lines skipped.").arg(skippedLines));
    }
    else {
        showError(QString("Cannot opene file %1").arg(fileName));
    }
}
