#ifndef SAMPLE_H
#define SAMPLE_H

#include "decoder.h"

#include <QString>

struct Sample {
    qint64 timestamp;
    double u;
    double i;
    double ah;
    double wh;
};

#endif // SAMPLE_H
