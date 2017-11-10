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

#pragma once

#include <vector>

class IoBucketizer 
{
public:
    IoBucketizer();
    void Initialize(unsigned __int64 bucketDuration, size_t validBuckets);

    size_t GetNumberOfValidBuckets() const;
    unsigned int GetIoBucketCount(size_t bucketNumber) const;
    double GetIoBucketMinDurationUsec(size_t bucketNumber) const;
    double GetIoBucketMaxDurationUsec(size_t bucketNumber) const;
    double GetIoBucketAvgDurationUsec(size_t bucketNumber) const;
    double GetIoBucketDurationStdDevUsec(size_t bucketNumber) const;
    void Add(unsigned __int64 ioCompletionTime, double ioDuration);
    double GetStandardDeviationIOPS() const;
    void Merge(const IoBucketizer& other);
private:
    double _GetMeanIOPS() const;

    struct IoBucket {
        IoBucket() :
            ulCount(0),
            lfMinDuration(0),
            lfMaxDuration(0),
            lfSumDuration(0),
            lfSumSqrDuration(0)
        {
        }
        
        unsigned int ulCount;
        double lfMinDuration;
        double lfMaxDuration;
        double lfSumDuration;
        double lfSumSqrDuration;
    };

    unsigned __int64 _bucketDuration;
    size_t _validBuckets;
    size_t _totalBuckets;
    std::vector<IoBucket> _vBuckets;
};
