#ifndef FLASHER_H
#define FLASHER_H

#include <QtSerialPort/QtSerialPort>
#include "flasherworker.h"

class Flasher : public QObject {
    Q_OBJECT

public:
    enum class State {
        Idle,
        Connected,
        Preparing,
        Programming,
        Verifying,
        Resetting,
        Ready,
    };

public:
    Flasher();
    ~Flasher();

public slots:
    void onStart();
    void portConnect(QString portName, QByteArray fileContent);
    void portDisconnect();

signals:
    void error(QString msg);
    void stateChanged(Flasher::State state);
    void download(const QByteArray& bin);
    void progress(double percent);
    void cancel();

private slots:
    void send(const QByteArray& data);
    void on_readyRead();
    void on_workerResult(const QString& res);

protected:
    void timerEvent(QTimerEvent *event) override;

private:
    void setState(State state);
    void processRead();
    void startTheTimer();
    void stopTheTimer();
    void restartTheTimer();

private:
    QThread workerThread;
    FlasherWorker* worker;

    QSerialPort *ser;
    State state;
    int timerId;
    QQueue<QByteArray> toSend;
    QByteArray fileContent;

    QByteArray readBuf;
    uint32_t readAddr;
    uint32_t readLen;
    uint32_t chunkLen;
};

#endif // FLASHER_H
