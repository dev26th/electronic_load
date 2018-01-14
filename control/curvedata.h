#ifndef CURVEDATA_H
#define CURVEDATA_H

#include "samplestorage.h"

#include <qwt_series_data.h>

#include <QObject>

class CurveData: public QObject, public QwtSeriesData<QPointF>
{
    Q_OBJECT

public:
    CurveData(SampleStorage& storage_);
    ~CurveData();

    QPointF sample(size_t i) const override;
    size_t size() const override { return storage.size(); }
    QRectF boundingRect() const override;

public slots:
    void cleared();
    void added(const Sample& sample);

private:
    SampleStorage& storage;
    qint64 begin;
    double minValue;
    double maxValue;
};


#endif // CURVEDATA_H

