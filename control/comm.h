#ifndef COMM_H
#define COMM_H

#include <QtSerialPort/QtSerialPort>

class Comm : public QObject {
    Q_OBJECT

public:
    enum class State {
        Idle,
        Connected
    };

public:
    Comm();

    static const int RXBUF_SIZE = 250;

public slots:
    void onStart();
    void portConnect(QString portName);
    void portDisconnect();
    void send(QByteArray data);

signals:
    void error(QString msg);
    void data(QByteArray d, qint64 timestamp);
    void stateChanged(Comm::State state);

private slots:
    void on_readyRead();

private:
    enum class SubState {
        Start,
        H,
        L
    };

private:
    void setState(State state);
    void processRead();
    void resetRx();
    void processRx();

private:
    QSerialPort *ser;
    State state;
    SubState subState;
    QByteArray rxBuf;
    qint64 rxTimestamp;
};

#endif // COMM_H
