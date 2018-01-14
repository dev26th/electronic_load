#include "samplestorage.h"

// =============================================================================================================

const Sample& SampleStorage::sample(size_t i) const
{
    return samples[i];
}

size_t SampleStorage::size() const
{
    return samples.size();
}

void SampleStorage::append(const Sample & sample)
{
    if(!enabled) return;

    if(!begin) begin = sample.timestamp;

    if(samples.size() >= limit)
        del(samples.size() - limit + 1);

    emit beforeAppend(sample);

    samples.push_back(sample);

    emit afterAppend(sample);
}

void SampleStorage::del(size_t n)
{
    if(n > samples.size())
        n = samples.size();

    emit beforeDelete(n);

    samples.erase(samples.begin(), samples.begin() + n);

    emit afterDelete(n);
}

void SampleStorage::clear()
{
    emit beforeClear();

    samples.clear();
    begin = 0;

    emit afterClear();
}
