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

// Plain min/max macros from common headers will interfere with std::numeric_limits

#pragma push_macro("min")
#pragma push_macro("max")
#undef min
#undef max

template<typename T>
class Histogram
{
private:

    mutable std::unordered_map<T, unsigned> _data;
    mutable unsigned _samples;

    mutable std::map<T, unsigned> _sdata;
    mutable unsigned _ssamples;

    // Save most recent percentile/iterator/nth-distance query. If the next is strictly >= it allows
    // an efficient forward iteration through an ascending set of queries.

    mutable double _lastptile;
    mutable unsigned _lastptilen;
    mutable decltype(_sdata.cbegin()) _lastptilepos;

    // A histogram starts writable/unsealed and automatically seals after the first read operation.
    // Subsequent writes which add data restart from empty.

    void _SealData() const
    {
        if (!_data.empty())
        {
            _sdata.clear();

            _sdata = std::map<T, unsigned>(_data.cbegin(), _data.cend());
            _ssamples = _samples;

            // invalid ptile > 1; first ptile query will initialize
            _lastptile = 1.1;

            _data.clear();
            _samples = 0;
        }
    }

public:

    Histogram()
        : _samples(0),
        _ssamples(0)
    {}

    void Clear()
    {
        _data.clear();
        _samples = 0;

        _sdata.clear();
        _ssamples = 0;

    }

    void Add(T v)
    {
        _data[v]++;
        _samples++;
    }

    void Merge(const Histogram<T> &other)
    {
        for (auto i : other._data)
        {
            _data[ i.first ] += i.second;
        }

        _samples += other._samples;
    }

    T GetMin() const
    {
        _SealData();

        // Default low if empty
        if (!_ssamples)
        {
            return std::numeric_limits<T>::min();
        }

        return _sdata.cbegin()->first;
    }

    T GetMax() const
    {
        _SealData();

        // Default low if empty
        if (!_ssamples)
        {
            return std::numeric_limits<T>::min();
        }

        return _sdata.crbegin()->first;
    }

    unsigned GetSampleBuckets() const
    {
        return (unsigned) (_samples ? _data.size() : _sdata.size());
    }

    unsigned GetSampleSize() const
    {
        return _samples ? _samples : _ssamples;
    }

    T GetPercentile(double p) const
    {
        if ((p < 0.0) || (p > 1.0))
        {
            throw std::invalid_argument("Percentile must be >= 0 and <= 1");
        }

        _SealData();

        // Default low if empty
        if (!_ssamples)
        {
            return std::numeric_limits<T>::min();
        }

        const double target = p * _ssamples;

        // Default to beginning; n is the number of samples iterated over so far
        unsigned n = 0;
        auto pos = _sdata.cbegin();

        // Resume from last?
        if (p >= _lastptile)
        {
            n = _lastptilen;
            pos = _lastptilepos;
        }

        while (pos != _sdata.cend())
        {
            if (n + pos->second >= target)
            {
                // Save position. Note the pre-incremented distance through the histogram
                // must be saved in case next is still in the same bucket.
                _lastptile = p;
                _lastptilen = n;
                _lastptilepos = pos;

                return pos->first;
            }

            n += pos->second;
            ++pos;
        }

        throw std::overflow_error("overran end trying to find percentile");
    }

    T GetPercentile(int p) const
    {
        return GetPercentile(static_cast<double>(p) / 100);
    }

    T GetMedian() const
    {
        return GetPercentile(0.5);
    }

    double GetStdDev()  const { return GetStandardDeviation(); }
    double GetAvg()     const { return GetMean(); }

    double GetMean() const
    {
        _SealData();

        // Default low if empty
        if (!_ssamples)
        {
            return std::numeric_limits<T>::min();
        }

        double sum(0);

        for (const auto i : _sdata)
        {
            double bucket_val =
                static_cast<double>(i.first) * i.second / _ssamples;

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

        for (const auto i : _sdata)
        {
            double dev = static_cast<double>(i.first) - mean;
            double sqdev = dev * dev;
            ssd += i.second * sqdev;
        }

        return sqrt(ssd / _ssamples);
    }

    std::string GetHistogramCsv(const unsigned bins) const
    {
        return GetHistogramCsv(bins, GetMin(), GetMax());
    }

    std::string GetHistogramCsv(const unsigned bins, const T LOW, const T HIGH) const
    {
        _SealData();

        // ISSUE-REVIEW
        // Currently bins are defined as strictly less-than
        // their upper limit, with the exception of the last
        // bin.  Otherwise where would I put the max value?
        const double binSize = static_cast<double>((HIGH - LOW) / bins);
        double limit = static_cast<double>(LOW);

        std::ostringstream os;
        os.precision(std::numeric_limits<T>::digits10);

        auto pos = _sdata.cbegin();
        unsigned cumulative = 0;

        for (unsigned bin = 1; bin <= bins; ++bin)
        {
            unsigned count = 0;
            limit += binSize;

            while (pos != _sdata.end() &&
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
        _SealData();

        std::ostringstream os;
        os.precision(std::numeric_limits<T>::digits10);

        for (const auto i : _sdata)
        {
            os << i.first << "," << i.second << std::endl;
        }

        return os.str();
    }

    std::string GetRaw() const
    {
        _SealData();

        std::ostringstream os;

        for (const auto i : _sdata)
        {
            os << i.second << " " << i.first << std::endl;
        }

        return os.str();
    }
};

#pragma pop_macro("min")
#pragma pop_macro("max")
