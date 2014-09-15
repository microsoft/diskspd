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

#include "IoBucketizer.h"

const unsigned __int64 INVALID_BUCKET_DURATION = 0;

IoBucketizer::IoBucketizer()
    : _bucketDuration(INVALID_BUCKET_DURATION),
      _validBuckets(0)
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
    _vBuckets.reserve(_validBuckets);
}

void IoBucketizer::Add(unsigned __int64 ioCompletionTime)
{
    if (_bucketDuration == INVALID_BUCKET_DURATION)
    {
        throw std::runtime_error("IoBucketizer has not been initialized");
    }

    size_t bucketNumber = static_cast<size_t>(ioCompletionTime / _bucketDuration);
    size_t currentSize = _vBuckets.size();
    if (currentSize < bucketNumber + 1) 
    {
        _vBuckets.resize(bucketNumber + 1);
        // Zero the new entries. Note that size is 1-based and bucketNumber is 0-based.
        for (size_t i = currentSize; i <= bucketNumber; i++)
        {
            _vBuckets[i] = 0;
        }
    }
    _vBuckets[bucketNumber]++;
}

size_t IoBucketizer::GetNumberOfValidBuckets() const 
{
    // Buckets beyond this may exist since Add is willing to extend the vector
    // beyond the expected number of valid buckets, but they are not comparable
    // buckets (straggling IOs over the timespan boundary).
    return (_vBuckets.size() > _validBuckets ? _validBuckets : _vBuckets.size());
}

size_t IoBucketizer::GetNumberOfBuckets() const
{
    return _vBuckets.size();
}

unsigned int IoBucketizer::GetIoBucket(size_t bucketNumber) const 
{
    return _vBuckets[bucketNumber];
}

double IoBucketizer::_GetMean() const 
{ 
    size_t numBuckets = GetNumberOfValidBuckets();
    double sum = 0;

    for (size_t i = 0; i < numBuckets; i++)
    {
        sum += static_cast<double>(_vBuckets[i]) / numBuckets;
    }

    return sum;
}

double IoBucketizer::GetStandardDeviation() const
{ 
    size_t numBuckets = GetNumberOfValidBuckets();

    if(numBuckets == 0) 
    {
        return 0.0;
    }

    double mean = _GetMean();
    double ssd = 0;

    for (size_t i = 0; i < numBuckets; i++)
    {
        double dev = static_cast<double>(_vBuckets[i]) - mean;
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
    if (other._validBuckets > _validBuckets)
    {
        _validBuckets = other._validBuckets;
    }
    for(size_t i = 0; i < other._vBuckets.size(); i++) 
    {
        _vBuckets[i] += other.GetIoBucket(i);
    }
}