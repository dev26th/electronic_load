#ifndef SAMPLESTORAGE_H
#define SAMPLESTORAGE_H

#include "sample.h"

#include <QObject>

#include <deque>

class SampleStorage : public QObject
{
    Q_OBJECT

public:
    SampleStorage(size_t limit_) : limit(limit_), enabled(false), begin(0) {}

    const Sample &sample(size_t i) const;
    size_t size() const;
    void clear();
    void del(size_t n); // delete first n samples
    bool isEnabled() const { return enabled; }
    qint64 getBegin() const { return begin; }

public slots:
    void append(const Sample &sample);
    void setEnabled(bool enabled) { this->enabled = enabled; }

signals:
    void beforeAppend(const Sample &sample);
    void afterAppend(const Sample &sample);
    void beforeClear();
    void afterClear();
    void beforeDelete(size_t n); // before delteing of first n samples
    void afterDelete(size_t n);  // after delteing of first n samples

private:
    size_t limit;
    std::deque<Sample> samples;
    bool enabled;
    qint64 begin;
};

#endif // SAMPLESTORAGE_H
