#include "curvedata.h"

// =============================================================================================================

CurveData::CurveData(SampleStorage& storage_)
    : storage(storage_)
{
    cleared();

    connect(&storage, &SampleStorage::afterClear, this, &CurveData::cleared);
    connect(&storage, &SampleStorage::afterAppend, this, &CurveData::added);
    // note: SampleStorage::afterDelete is not connected
    //       - min/max value will not be recalculated when samples are deleted via limit
}

CurveData::~CurveData()
{
}

QPointF CurveData::sample(size_t i) const
{
    const Sample& s = storage.sample(i);
    return QPointF((qreal)(s.timestamp - begin)/1000.0, s.u);
}

QRectF CurveData::boundingRect() const
{
    if(storage.size() == 0)
        return QRectF();

    const Sample& first = storage.sample(0);
    const Sample& last = storage.sample(storage.size()-1);

    return QRectF((qreal)(first.timestamp - begin)/1000.0, minValue,
                  (qreal)(last.timestamp - first.timestamp)/1000.0, maxValue - minValue);
}

void CurveData::cleared()
{
    begin = 0;
    minValue = 0.0;
    maxValue = 0.0;
}

void CurveData::added(const Sample& sample)
{
    double value = sample.u;
    if(value == std::numeric_limits<double>::infinity()) value = 0.0;

    if(!begin) {
        begin = sample.timestamp;
        minValue = value;
        maxValue = value;
    }
    else {
        if(value < minValue) minValue = value;
        if(value > maxValue) maxValue = value;
    }
}
