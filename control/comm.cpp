#include "comm.h"
#include <limits>
#include <algorithm>

#include <QDebug>

Comm::Comm()
  : state(State::Idle)
{
}

void Comm::onStart()
{
    setState(State::Idle);
    ser = new QSerialPort(this);
    connect(ser, &QSerialPort::readyRead, this, &Comm::on_readyRead);
}

void Comm::portConnect(QString portName)
{
    if(ser->isOpen()) portDisconnect();

    ser->setPortName(portName);
    ser->setBaudRate(115200);

    bool openSuccess = ser->open(QIODevice::ReadWrite);
    if(!openSuccess)
        emit error("Cannot connect to the port");
    else
        setState(State::Connected);
}

void Comm::setState(State state)
{
    if(this->state != state) {
        this->state = state;
        resetRx();
        emit stateChanged(state);
    }
}

void Comm::portDisconnect()
{
    ser->close();
    setState(State::Idle);
}

static char crc8(char crc, char b) {
    crc ^= b;
    for(uint8_t i = 0; i < 8; ++i)
        crc = crc & 0x80 ? (crc << 1) ^ 0x07 : crc << 1;
    return crc;
}

void Comm::send(QByteArray data)
{
    if(!ser->isOpen()) return;

    ser->clear();

    QByteArray buf;
    buf.append('S');
    buf.append(data.toHex().toUpper());

    char crc = std::accumulate(data.begin(), data.end(), 0, crc8);
    QByteArray crcBuf;
    crcBuf.append(crc);
    buf.append(crcBuf.toHex().toUpper());

    buf.append('\r');
    (void)ser->write(buf);
    qDebug() << "<=" << data.toHex();
}

void Comm::on_readyRead()
{
    while(!ser->atEnd()) {
        processRead();
    }
}

void Comm::resetRx()
{
    subState = SubState::Start;
    rxBuf.clear();
}

void Comm::processRx()
{
    char crc = std::accumulate(rxBuf.begin(), rxBuf.end(), 0, crc8);
    if(crc == 0) {
        qDebug() << "=>" << rxBuf.toHex();

        QByteArray buf;
        buf.append(rxBuf.data(), rxBuf.size()-1);
        emit data(buf, rxTimestamp);
    }
}

void Comm::processRead()
{
    char c;
    if(!ser->getChar(&c)) return;

    switch(state) {
        case State::Idle:
            break;

        case State::Connected:
            if(c == 'S') {
                resetRx();
                rxTimestamp = QDateTime::currentMSecsSinceEpoch();
                subState = SubState::H;
            }
            else {
                uint8_t v;

                switch(subState) {
                    case SubState::H:
                    case SubState::L:
                        if(c >= '0' && c <= '9') {
                            v = c - '0';
                        }
                        else if(c >= 'A' && c <= 'F') {
                            v = c - 'A' + 10;
                        }
                        else if(c == '\n') { // ignore
                            break;
                        }
                        else if(c == '\r') { // stop
                            if(subState == SubState::L) {  // unexpected, reset
                            }
                            else {
                                processRx();
                            }
                            resetRx();

                            break;
                        }
                        else {    // unexpected symbol, reset
                            resetRx();
                            break;
                        }

                        if(subState == SubState::H) {
                            rxBuf.append(v << 4);
                            subState = SubState::L;
                        }
                        else {
                            rxBuf[rxBuf.size()-1] = (rxBuf.at(rxBuf.size()-1) | (char)v);
                            subState = SubState::H;
                            if(rxBuf.size() >= RXBUF_SIZE) resetRx();
                        }

                        break;

                    case SubState::Start:
                        break; // ignore all
                }
            }

            break;
    }
}
