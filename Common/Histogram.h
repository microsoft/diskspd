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

#include <map>
#include <unordered_map>
#include <string>
#include <sstream>
#include <limits>
#include <cmath>

#pragma push_macro("min")
#pragma push_macro("max")
#undef min
#undef max

template<typename T>
class Histogram
{
    private:

    unsigned _samples;

#define USE_HASH_TABLE
#ifdef USE_HASH_TABLE
    std::unordered_map<T,unsigned> _data;

	using SortedDataCache = std::map<T, unsigned>;
	using SortedDataCachePtr = std::shared_ptr<SortedDataCache>;

	mutable SortedDataCachePtr _sortedDataCache;

	SortedDataCachePtr _GetSortedData() const
    {
		//	Initialize the sorted data cache if this is the first time through, or it was reset by Touch().
		if (!_sortedDataCache)
		{
			_sortedDataCache = std::make_shared<SortedDataCache>(_data.begin(), _data.end());
		}

        return _sortedDataCache;
    }
#else
    std::map<T,unsigned> _data;

    std::map<T,unsigned> _GetSortedData() const
    {
        return _data; 
    }
#endif

	void Touch()
	{
		if (_sortedDataCache)
		{
			_sortedDataCache.reset();
		}
	}

	public:

    Histogram()
        : _samples(0), _sortedDataCache(nullptr)
    {}

    void Clear()
    {
        _data.clear();
        _samples = 0;

		Touch();
    }

    void Add(T v)
    { 
        _data[ v ]++;
        _samples++;

		Touch();
	}

    void Merge(const Histogram<T> &other)
    {
        for (auto i : other._data)
        {
            _data[ i.first ] += i.second;
        }

        _samples += other._samples;
	
		Touch();
	}

    T GetMin() const
    { 
        T min(std::numeric_limits<T>::max());

        for (auto i : _data)
        {
            if (i.first < min)
            {
                min = i.first;
            }
        }

        return min;
    }

    T GetMax() const
    {
        T max(std::numeric_limits<T>::min());

        for (auto i : _data)
        {
            if (i.first > max) 
            {
                max = i.first;
            }
        }

        return max;
    }

    unsigned GetSampleSize() const 
    {
        return _samples;
    }

	unsigned GetBucketCount() const
	{
		return _data.size();
	}

    T GetPercentile(double p) const 
    {
        // ISSUE-REVIEW
        // What do the 0th and 100th percentile really mean?
        if ((p < 0) || (p > 1))
        {
            throw std::invalid_argument("Percentile must be >= 0 and <= 1");
        }

        const double target = GetSampleSize() * p;

        unsigned cur = 0;
        for (auto i : *_GetSortedData()) 
        {
            cur += i.second;
            if (cur >= target)
            {
                return i.first;
            }
        }

        // We can get here if no IOs are issued, simply return 0 since
        // we don't want to throw an exception and crash
        return 0;
    }
    
    T GetPercentile(int p) const 
    {
        return GetPercentile(static_cast<double>(p)/100);
    }

    T GetMedian() const 
    { 
        return GetPercentile(0.5); 
    }

    double GetStdDev() const { return GetStandardDeviation(); }
    double GetAvg() const { return GetMean(); }

    double GetMean() const 
    { 
        double sum(0);
        unsigned samples = GetSampleSize();

        for (auto i : _data)
        {
            double bucket_val =
                static_cast<double>(i.first) * i.second / samples;

            if (sum + bucket_val < 0)
            {
                throw std::overflow_error("while trying to accumulate sum");
            }

            sum += bucket_val;
        }

        return sum;
    }

    double GetStandardDeviation() const
    { 
        double mean(GetMean());
        double ssd(0);

        for (auto i : _data)
        {
            double dev = static_cast<double>(i.first) - mean;
            double sqdev = dev*dev;
            ssd += i.second * sqdev;
        }

        return sqrt(ssd / GetSampleSize());
    }

    std::string GetHistogramCsv(const unsigned bins) const
    {
        return GetHistogramCsv(bins, GetMin(), GetMax());
    }

    std::string GetHistogramCsv(const unsigned bins, const T LOW, const T HIGH) const
    {
        // ISSUE-REVIEW
        // Currently bins are defined as strictly less-than
        // their upper limit, with the exception of the last
        // bin.  Otherwise where would I put the max value?
        const double binSize = static_cast<double>((HIGH - LOW) / bins);
        double limit = static_cast<double>(LOW);

        std::ostringstream os;
        os.precision(std::numeric_limits<T>::digits10);

        auto sortedData = _GetSortedData();

        auto pos = sortedData.begin(); 

        unsigned cumulative = 0;

        for (unsigned bin = 1; bin <= bins; ++bin)
        {
            unsigned count = 0;
            limit += binSize;

            while (pos != sortedData.end() && 
                    (pos->first < limit || bin == bins))
            {
                count += pos->second;
                ++pos;
            }

            cumulative += count;

            os << limit << "," << count << "," << cumulative << std::endl;
        }

        return os.str();
    }

    std::string GetRawCsv() const
    {
        std::ostringstream os;
        os.precision(std::numeric_limits<T>::digits10);

        for (auto i : *_GetSortedData()) 
        {
            os << i.first << "," << i.second << std::endl;
        }

        return os.str();
    }

    std::string GetRaw() const
    {
        std::ostringstream os;

        for (auto i : *_GetSortedData()) 
        {
            os << i.second << " " << i.first << std::endl;
        }

        return os.str();
    }
};

#pragma pop_macro("min")
#pragma pop_macro("max")
