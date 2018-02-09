#include "flasher.h"

#include <limits>
#include <algorithm>

#include <QDebug>

Q_DECLARE_METATYPE(Flasher::State)

static const int SYNCH_INTERVAL_MS = 250;

enum class BlCmd : uint8_t {
    Synch = 0x7F,
    Ack   = 0x79,
};

Flasher::Flasher()
  : worker(nullptr), state(State::Idle), timerId(0)
{
    qRegisterMetaType<Flasher::State>();
}

Flasher::~Flasher() {
    workerThread.quit();
    workerThread.wait();
}

void Flasher::onStart()
{
    setState(State::Idle);
    ser = new QSerialPort(this);
    connect(ser, &QSerialPort::readyRead, this, &Flasher::on_readyRead);

    worker = new FlasherWorker();
    worker->moveToThread(&workerThread);
    connect(&workerThread, &QThread::finished, worker, &QObject::deleteLater);
    connect(this, &Flasher::download, worker, &FlasherWorker::download);
    connect(this, &Flasher::cancel, worker, &FlasherWorker::cancel);
    connect(worker, &FlasherWorker::send, this, &Flasher::send);
    connect(worker, &FlasherWorker::result, this, &Flasher::on_workerResult);
    connect(worker, &FlasherWorker::progress, this, &Flasher::progress);
    workerThread.start();
}

void Flasher::startTheTimer()
{
    if(timerId) killTimer(timerId);
    timerId = startTimer(SYNCH_INTERVAL_MS, Qt::PreciseTimer);
}

void Flasher::stopTheTimer()
{
    if(timerId) {
        killTimer(timerId);
        timerId = 0;
    }
}

void Flasher::portConnect(QString portName, QByteArray fileContent)
{
    this->fileContent = fileContent;

    if(ser->isOpen()) portDisconnect();

    ser->setPortName(portName);
    ser->setBaudRate(115200);

    bool openSuccess = ser->open(QIODevice::ReadWrite);
    if(!openSuccess) {
        emit error("Cannot connect to the port");
    }
    else {
        setState(State::Connected);
        startTheTimer();
    }
}

void Flasher::timerEvent(QTimerEvent *event)
{
    if(!ser->isOpen()) return;

    (void)event;

    switch(state) {
        case State::Connected:
            send(QByteArray(1, (uint8_t)BlCmd::Synch));
            break;

        default:
            ;
    }
}

void Flasher::on_workerResult(const QString& state)
{
    if("1" == state) {
        setState(State::Programming);
    }
    else if("2" == state) {
        setState(State::Verifying);
    }
    else if("3" == state) {
        setState(State::Resetting);
    }
    else {
        setState(State::Ready);
    }
}

void Flasher::setState(State state)
{
    if(this->state != state) {
        this->state = state;
        emit stateChanged(state);
    }
}

void Flasher::portDisconnect()
{
    ser->close();
    setState(State::Idle);
    emit cancel();
}

void Flasher::send(const QByteArray& data)
{
    if(!ser->isOpen()) return;

    (void)ser->write(data);
    //qDebug() << "<=" << data.toHex();
}

void Flasher::on_readyRead()
{
    char c;
    while(ser->getChar(&c)) {
        //qDebug() << "=>" << QByteArray(1, c).toHex();
        switch(state) {
            case State::Connected:
                if(c == (uint8_t)BlCmd::Ack) {
                    stopTheTimer();
                    send(QByteArray(1, c)); // echo
                    setState(State::Preparing);
                    emit download(fileContent);
                }
                break;

            case State::Preparing:
            case State::Programming:
            case State::Verifying:
            case State::Resetting:
                send(QByteArray(1, c)); // echo
                worker->addChar(c);
                break;

            default:
                ;
        }
    }
}
