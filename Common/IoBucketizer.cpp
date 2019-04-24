/*

DISKSPD

Copyright(c) Microsoft Corporation
All rights reserved.

MIT License

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED *AS IS*, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

*/

#include <stdexcept>
#include "IoBucketizer.h"

/*
Calculating stddev using an online algorithm:
avg = sum(1..n, a[n]) / n
stddev = sqrt(sum(1..n, (a[n] - avg)^2) / n)
       = sqrt(sum(1..n, a[n]^2 - 2 * a[n] * avg + avg^2) / n)
       = sqrt((sum(1..n, a[n]^2) - 2 * avg * sum(1..n, a[n]) + n * avg^2) / n)
       = sqrt((sum(1..n, a[n]^2) - 2 * (sum(1..n, a[n]) / n) * sum(1..n, a[n]) + n * (sum(1..n], a[n]) / n)^2) / n)
       = sqrt((sum(1..n, a[n]^2) - (2 / n) * sum(1..n, a[n])^2 + (1 / n) * sum(1..n, a[n])^2) / n)
       = sqrt((sum(1..n, a[n]^2) - (1 / n) * sum(1..n, a[n])^2) / n)

So if we track n, sum(a[n]) and sum(a[n]^2) we can calculate the stddev.  This
is used to calculate the stddev of the latencies below.
*/

const unsigned __int64 INVALID_BUCKET_DURATION = 0;

IoBucketizer::IoBucketizer()
    : _bucketDuration(INVALID_BUCKET_DURATION),
      _validBuckets(0),
      _totalBuckets(0)
{}

void IoBucketizer::Initialize(unsigned __int64 bucketDuration, size_t validBuckets)
{
    if (_bucketDuration != INVALID_BUCKET_DURATION)
    {
        throw std::runtime_error("IoBucketizer has already been initialized");
    }
    if (bucketDuration == INVALID_BUCKET_DURATION)
    {
        throw std::invalid_argument("Bucket duration must be a positive integer");
    }

    _bucketDuration = bucketDuration;
    _validBuckets = validBuckets;
    _vBuckets.resize(_validBuckets);
}

void IoBucketizer::Add(unsigned __int64 ioCompletionTime, double ioDuration)
{
    if (_bucketDuration == INVALID_BUCKET_DURATION)
    {
        throw std::runtime_error("IoBucketizer has not been initialized");
    }

    size_t bucketNumber = static_cast<size_t>(ioCompletionTime / _bucketDuration);
    _totalBuckets = bucketNumber + 1;

    if (bucketNumber >= _validBuckets)
    {
        return;
    }
    
    _vBuckets[bucketNumber].lfSumDuration += ioDuration;
    _vBuckets[bucketNumber].lfSumSqrDuration += ioDuration * ioDuration;

    if (_vBuckets[bucketNumber].ulCount == 0 ||
        ioDuration < _vBuckets[bucketNumber].lfMinDuration)
    {
        _vBuckets[bucketNumber].lfMinDuration = ioDuration;
    }
    if (_vBuckets[bucketNumber].ulCount == 0 ||
        ioDuration > _vBuckets[bucketNumber].lfMaxDuration)
    {
        _vBuckets[bucketNumber].lfMaxDuration = ioDuration;
    }

    _vBuckets[bucketNumber].ulCount++;
}

size_t IoBucketizer::GetNumberOfValidBuckets() const 
{
    return (_totalBuckets > _validBuckets ? _validBuckets : _totalBuckets);
}

unsigned int IoBucketizer::GetIoBucketCount(size_t bucketNumber) const 
{
    if (bucketNumber < _validBuckets)
    {
        return _vBuckets[bucketNumber].ulCount;
    }
    
    return 0;
}

double IoBucketizer::GetIoBucketMinDurationUsec(size_t bucketNumber) const
{
    if (bucketNumber < _validBuckets)
    {
        return _vBuckets[bucketNumber].lfMinDuration;
    }
    
    return 0;
}

double IoBucketizer::GetIoBucketMaxDurationUsec(size_t bucketNumber) const
{
    if (bucketNumber < _validBuckets)
    {
        return _vBuckets[bucketNumber].lfMaxDuration;
    }
    
    return 0;
}

double IoBucketizer::GetIoBucketAvgDurationUsec(size_t bucketNumber) const
{
    if (bucketNumber < _validBuckets && _vBuckets[bucketNumber].ulCount != 0)
    {
        return _vBuckets[bucketNumber].lfSumDuration / static_cast<double>(_vBuckets[bucketNumber].ulCount);
    }

    return 0;
}

double IoBucketizer::GetIoBucketDurationStdDevUsec(size_t bucketNumber) const
{
    if (bucketNumber < _validBuckets && _vBuckets[bucketNumber].ulCount != 0)
    {
        double sum_of_squares = _vBuckets[bucketNumber].lfSumSqrDuration;
        double square_of_sum = _vBuckets[bucketNumber].lfSumDuration * _vBuckets[bucketNumber].lfSumDuration;
        double count = static_cast<double>(_vBuckets[bucketNumber].ulCount);
        double square_stddev = (sum_of_squares - (square_of_sum / count)) / count;
        
        return sqrt(square_stddev);
    }

    return 0;
}

double IoBucketizer::_GetMeanIOPS() const 
{ 
    size_t numBuckets = GetNumberOfValidBuckets();
    double sum = 0;

    for (size_t i = 0; i < numBuckets; i++)
    {
        sum += static_cast<double>(_vBuckets[i].ulCount) / numBuckets;
    }

    return sum;
}

double IoBucketizer::GetStandardDeviationIOPS() const
{ 
    size_t numBuckets = GetNumberOfValidBuckets();

    if(numBuckets == 0) 
    {
        return 0.0;
    }

    double mean = _GetMeanIOPS();
    double ssd = 0;

    for (size_t i = 0; i < numBuckets; i++)
    {
        double dev = static_cast<double>(_vBuckets[i].ulCount) - mean;
        double sqdev = dev*dev;
        ssd += sqdev;
    }

    return sqrt(ssd / numBuckets);
}

void IoBucketizer::Merge(const IoBucketizer& other) 
{
    if(other._vBuckets.size() > _vBuckets.size())
    {
        _vBuckets.resize(other._vBuckets.size());
    }
    for(size_t i = 0; i < other._vBuckets.size(); i++) 
    {
        _vBuckets[i].ulCount += other._vBuckets[i].ulCount;
        _vBuckets[i].lfSumDuration += other._vBuckets[i].lfSumDuration;
        _vBuckets[i].lfSumSqrDuration += other._vBuckets[i].lfSumSqrDuration;
        
        if (i >= _validBuckets ||
            other._vBuckets[i].lfMinDuration < _vBuckets[i].lfMinDuration)
        {
            _vBuckets[i].lfMinDuration = other._vBuckets[i].lfMinDuration;
        }
        if (other._vBuckets[i].lfMaxDuration > _vBuckets[i].lfMaxDuration)
        {
            _vBuckets[i].lfMaxDuration = other._vBuckets[i].lfMaxDuration;
        }
    }
    if (other._validBuckets > _validBuckets)
    {
        _validBuckets = other._validBuckets;
    }
    if (other._totalBuckets > _totalBuckets)
    {
        _totalBuckets = other._totalBuckets;
    }
}
