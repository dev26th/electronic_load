#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "decoder.h"
#include "comm.h"
#include "flasher.h"
#include "samplestorage.h"
#include "curvedata.h"
#include "tablemodel.h"

#include <qwt_color_map.h>

#include <QMainWindow>
#include <QLineEdit>
#include <QtSerialPort/QtSerialPort>
#include <QTimer>
#include <QPointF>
#include <QThread>
#include <QLabel>

#include <stdint.h>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

public:
    static const int MIN_INTERVAL_MS = 250;

private slots:
    void on_portBox_currentIndexChanged(int index);

    void on_actionShowTable_triggered();

    void on_actionShowGraph_triggered();

    void on_actionShowControl_triggered();

    void on_connectButton_clicked();

    void on_serError(QString msg);

    void on_serData(QByteArray d, qint64 timestamp);

    void on_serStateChanged(Comm::State state);

    void on_refreshPortsButton_clicked();

    void on_clearDataButton_clicked();

    void on_actionAbout_triggered();

    void on_actionSaveLog_triggered();

    void on_runButton_clicked();

    void on_limitUpdateButton_clicked();

    void on_actionDeviceConfiguration_triggered();

    void on_energyResetButton_clicked();

    void on_actionUpgradeFirmware_triggered();

    void on_flasherError(QString msg);

signals:
    void portConnect(QString portName);
    void portDisconnect();
    void send(QByteArray data);
    void sample(Sample s);
    void upgradeDevice(QString portName, QByteArray fileContent);
    void cancelUpgradeDevice();

public:
    void closeEvent(QCloseEvent *event);

private:
    void setControlEnabled(bool state);
    void connectSer();
    void disconnectSer();
    void executeNext();
    void configDevice();
    void updateFun();
    void setupTemperatureBox();
    void startUpgrade(const QByteArray& data);

private:
    struct ToExecute {
        enum class Action {
            Send,
            Disconnect,
            StartUpgrade,
        };

        Action action;
        QByteArray data;

        ToExecute() {}

        ToExecute(const ToExecute& o) : action(o.action), data(o.data) {}

        ToExecute(Action action_) : action(action_) {}

        ToExecute(Action action_, QByteArray data_) : action(action_), data(data_) {}
    };

private:
    Ui::MainWindow *ui;

    Comm *comm;
    QThread commThread;
    Flasher *flasher;
    QThread flasherThread;
    bool isConnected;
    QString currentPort;
    QQueue<ToExecute> toExecute;
    int interval;
    uint16_t deviceCurrent;
    uint16_t deviceLastU;
    CmdConfigData deviceConfigData;

    SampleStorage storage;
    CurveData *data;
    TableModel *tableModel;
    QLabel* deviceVersionLabel;
    QLabel* deviceMessageLabel;
};

#endif // MAINWINDOW_H
