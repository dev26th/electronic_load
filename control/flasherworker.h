#ifndef FLASHERWORKER_H
#define FLASHERWORKER_H

#include <QtSerialPort/QtSerialPort>
#include <QObject>
#include <QByteArray>
#include <QMutex>
#include <QWaitCondition>
#include <QQueue>

class FlasherWorker : public QObject
{
    Q_OBJECT

public:
    FlasherWorker();
    ~FlasherWorker();

public slots:
    void download(const QByteArray& bin);
    void cancel();

signals:
    void result(const QString& res);
    void send(const QByteArray& data);
    void progress(double percent);

public:
    void addChar(char c);

private:
    char getChar();
    void waitForAck();
    void writeToDevice(uint32_t addr, const QByteArray &data);
    QByteArray readFromDevice(uint32_t addr, int len);
    void startDevice(uint32_t addr);
    void incProgress(int points);

private:
    QMutex mutex;
    QQueue<char> queue;
    QWaitCondition cond;
    int pr;
    int total;
    bool canceled;
};

#endif // FLASHERWORKER_H
