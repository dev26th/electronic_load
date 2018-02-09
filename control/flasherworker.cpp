#include "flasherworker.h"

#include <QMutexLocker>

static const uint32_t ADDR_WRITE_ROUTINE = 0x000000A0;
static const uint32_t ADDR_FLASH = 0x00008000;
static const unsigned long RX_TIMEOUT_MS = 2000;
static const int PROGRESS_POINTS_WRITE = 1;
static const int PROGRESS_POINTS_READ  = 5;

enum class BlCmd : uint8_t {
    Get   = 0x00,
    Read  = 0x11,
    Erase = 0x43,
    Write = 0x31,
    Speed = 0x03,
    Go    = 0x21,

    Synch = 0x7F,
    Ack   = 0x79,
    Nack  = 0x1F,
    Busy  = 0xAA,
};

// converted from E_W_ROUTINEs_32K_ver_1.2.s19, which is atteched to the UM0560
// © 2017 STMicroelectronics – All rights reserved
static const unsigned char E_W_ROUTINEs_32K_ver_1_3[] = {
  0x5f, 0x3f, 0x90, 0x3f, 0x96, 0x72, 0x09, 0x00, 0x8e, 0x16, 0xcd, 0x60,
  0x65, 0xb6, 0x90, 0xe7, 0x00, 0x5c, 0x4c, 0xb7, 0x90, 0xa1, 0x21, 0x26,
  0xf1, 0xa6, 0x20, 0xb7, 0x88, 0x5f, 0x3f, 0x90, 0xe6, 0x00, 0xa1, 0x20,
  0x26, 0x07, 0x3f, 0x8a, 0xae, 0x40, 0x00, 0x20, 0x0c, 0x3f, 0x8a, 0xae,
  0x00, 0x80, 0x42, 0x58, 0x58, 0x58, 0x1c, 0x80, 0x00, 0x90, 0x5f, 0xcd,
  0x60, 0x65, 0x9e, 0xb7, 0x8b, 0x9f, 0xb7, 0x8c, 0xa6, 0x20, 0xc7, 0x50,
  0x5b, 0x43, 0xc7, 0x50, 0x5c, 0x4f, 0x92, 0xbd, 0x00, 0x8a, 0x5c, 0x9f,
  0xb7, 0x8c, 0x4f, 0x92, 0xbd, 0x00, 0x8a, 0x5c, 0x9f, 0xb7, 0x8c, 0x4f,
  0x92, 0xbd, 0x00, 0x8a, 0x5c, 0x9f, 0xb7, 0x8c, 0x4f, 0x92, 0xbd, 0x00,
  0x8a, 0x72, 0x00, 0x50, 0x5f, 0x07, 0x72, 0x05, 0x50, 0x5f, 0xfb, 0x20,
  0x04, 0x72, 0x10, 0x00, 0x96, 0x90, 0xa3, 0x00, 0x07, 0x27, 0x0a, 0x90,
  0x5c, 0x1d, 0x00, 0x03, 0x1c, 0x00, 0x80, 0x20, 0xae, 0xb6, 0x90, 0xb1,
  0x88, 0x27, 0x1c, 0x5f, 0x3c, 0x90, 0xb6, 0x90, 0x97, 0xcc, 0x00, 0xc0,
  0x9d, 0x9d, 0x9d, 0x9d, 0x9d, 0x9d, 0x9d, 0x9d, 0x9d, 0x9d, 0x9d, 0x9d,
  0x9d, 0x9d, 0x9d, 0x9d, 0x9d, 0x9d, 0x9d, 0x81, 0xcd, 0x60, 0x65, 0x5f,
  0x3f, 0x97, 0x72, 0x0d, 0x00, 0x8e, 0x18, 0x72, 0x00, 0x00, 0x94, 0x0b,
  0xa6, 0x01, 0xc7, 0x50, 0x5b, 0x43, 0xc7, 0x50, 0x5c, 0x20, 0x08, 0x35,
  0x81, 0x50, 0x5b, 0x35, 0x7e, 0x50, 0x5c, 0x3f, 0x94, 0xf6, 0x92, 0xa7,
  0x00, 0x8a, 0x72, 0x0c, 0x00, 0x8e, 0x13, 0x72, 0x00, 0x50, 0x5f, 0x07,
  0x72, 0x05, 0x50, 0x5f, 0xfb, 0x20, 0x04, 0x72, 0x10, 0x00, 0x97, 0xcd,
  0x60, 0x65, 0x9f, 0xb1, 0x88, 0x27, 0x03, 0x5c, 0x20, 0xdb, 0x72, 0x0d,
  0x00, 0x8e, 0x10, 0x72, 0x00, 0x50, 0x5f, 0x07, 0x72, 0x05, 0x50, 0x5f,
  0xfb, 0x20, 0x24, 0x72, 0x10, 0x00, 0x97, 0x20, 0x1e, 0x9d, 0x9d, 0x9d,
  0x9d, 0x9d, 0x9d, 0x9d, 0x9d, 0x9d, 0x9d, 0x9d, 0x9d, 0x9d, 0x9d, 0x9d,
  0x9d, 0x9d, 0x9d, 0x9d, 0x9d, 0x9d, 0x9d, 0x9d, 0x9d, 0x9d, 0x9d, 0x9d,
  0x9d, 0x9d, 0x9d, 0x81
};

FlasherWorker::FlasherWorker()
{
}

FlasherWorker::~FlasherWorker()
{
}

void FlasherWorker::download(const QByteArray& bin)
{
    try {
        QByteArray routine((const char*)E_W_ROUTINEs_32K_ver_1_3, sizeof(E_W_ROUTINEs_32K_ver_1_3));

        pr = 0;
        total = routine.size() * PROGRESS_POINTS_WRITE + bin.size() * (PROGRESS_POINTS_WRITE + PROGRESS_POINTS_READ);
        canceled = false;

        writeToDevice(ADDR_WRITE_ROUTINE, routine);

        emit result("1");
        writeToDevice(ADDR_FLASH, bin);

        emit result("2");
        QByteArray r = readFromDevice(ADDR_FLASH, bin.size());

        //qDebug() << "GOT" << r.toHex();
        if(bin == r) {
            emit result("3");
            startDevice(ADDR_FLASH);
        }
        else {
            throw "Validation failed";
        }

        emit result("");
    }
    catch(char const* msg) {
        qDebug() << msg;
        emit result(msg);
    }
}

void FlasherWorker::cancel()
{
    canceled = true;
}

void FlasherWorker::addChar(char c)
{
    QMutexLocker locker(&mutex);
    queue.enqueue(c);
    cond.wakeAll();
}

char FlasherWorker::getChar()
{
    QMutexLocker locker(&mutex);
    if(queue.isEmpty()) {
        if(!cond.wait(&mutex, RX_TIMEOUT_MS))
            throw "Rx timeout";
    }
    char c = queue.dequeue();

    return c;
}

static char xorChecksum(char checksum, char b) {
    return checksum ^ b;
}

void FlasherWorker::waitForAck()
{
    if(canceled)
        throw "Canceled";

    if(getChar() != (uint8_t)BlCmd::Ack)
        throw "Not ack";
}

void FlasherWorker::incProgress(int points)
{
    pr += points;
    emit progress((double)pr / total * 100);
}

void FlasherWorker::writeToDevice(uint32_t addr, const QByteArray& data)
{
    //qDebug() << "writeToDevice" << addr << data.size();
    QElapsedTimer elapsed;
    elapsed.start();
    for(int i = 0; i < data.size(); i += 128, addr += 128) {
        {
            QByteArray buf;
            buf.append((char)BlCmd::Write);

            char checksum = std::accumulate(buf.begin(), buf.end(), 0xFF, xorChecksum);
            buf.append(checksum);

            emit send(buf);
            waitForAck();
        }

        {
            QByteArray buf;
            buf.append((char)(addr >> 24));
            buf.append((char)(addr >> 16));
            buf.append((char)(addr >> 8));
            buf.append((char)(addr));

            char checksum = std::accumulate(buf.begin(), buf.end(), 0, xorChecksum);
            buf.append(checksum);

            emit send(buf);
            waitForAck();
        }

        {
            QByteArray buf;
            int chunkLen = std::min(128, data.size() - i);
            buf.append((char)(chunkLen - 1));
            buf.append(data.mid(i, chunkLen));

            char checksum = std::accumulate(buf.begin(), buf.end(), 0, xorChecksum);
            buf.append(checksum);

            emit send(buf);
            waitForAck();

            incProgress(chunkLen * PROGRESS_POINTS_WRITE);
        }
    }
    //qDebug() << "elapsed" << elapsed.elapsed();
}

QByteArray FlasherWorker::readFromDevice(uint32_t addr, int len)
{
    //qDebug() << "readFromDevice" << addr << len;
    QElapsedTimer elapsed;
    elapsed.start();

    QByteArray res;

    for(; len > 0; len -= 256, addr += 256) {
        int chunkLen = std::min(256, len);

        {
            QByteArray buf;
            buf.append((char)BlCmd::Read);

            char checksum = std::accumulate(buf.begin(), buf.end(), 0xFF, xorChecksum);
            buf.append(checksum);

            emit send(buf);
            waitForAck();
        }

        {
            QByteArray buf;
            buf.append((char)(addr >> 24));
            buf.append((char)(addr >> 16));
            buf.append((char)(addr >> 8));
            buf.append((char)(addr));

            char checksum = std::accumulate(buf.begin(), buf.end(), 0, xorChecksum);
            buf.append(checksum);

            emit send(buf);
            waitForAck();
        }

        {
            QByteArray buf;
            buf.append((char)(chunkLen - 1));

            char checksum = std::accumulate(buf.begin(), buf.end(), 0xFF, xorChecksum);
            buf.append(checksum);

            emit send(buf);
            waitForAck();
        }

        for(int l = chunkLen; l > 0; --l)
            res.append(getChar());

        incProgress(chunkLen * PROGRESS_POINTS_READ);
    }
    //qDebug() << "readDone" << res.size() << elapsed.elapsed();
    return res;
}

void FlasherWorker::startDevice(uint32_t addr)
{
    {
        QByteArray buf;
        buf.append((char)BlCmd::Go);

        char checksum = std::accumulate(buf.begin(), buf.end(), 0xFF, xorChecksum);
        buf.append(checksum);

        emit send(buf);
        waitForAck();
    }

    {
        QByteArray buf;
        buf.append((char)(addr >> 24));
        buf.append((char)(addr >> 16));
        buf.append((char)(addr >> 8));
        buf.append((char)(addr));

        char checksum = std::accumulate(buf.begin(), buf.end(), 0, xorChecksum);
        buf.append(checksum);

        emit send(buf);
        waitForAck();
    }
}
