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
#include "Common.h"

namespace UnitTests
{
    class ResultParserUnitTests;
}

class ResultParser : public IResultParser
{
public:
    string ParseResults(const Profile& profile, const SystemInformation& system, vector<Results> vResults);
    string ParseProfile(const Profile& profile);

private:
    void _DisplayFileSize(UINT64 fsize, UINT32 align = 0);
    void _DisplayETWSessionInfo(struct ETWSessionInfo sessionInfo);
    void _DisplayETW(struct ETWMask ETWMask, struct ETWEventCounters EtwEventCounters);
    void _Print(const char *format, ...);
    void _PrintProfile(const Profile& profile);
    void _PrintSystemInfo(const SystemInformation& system);
    void _PrintCpuUtilization(const Results& results, const SystemInformation& system);
    enum class _SectionEnum {TOTAL, READ, WRITE};
    void _PrintSectionFieldNames(const TimeSpan& timeSpan);
    void _PrintSectionBorderLine(const TimeSpan& timeSpan);
    void _PrintSection(_SectionEnum, const TimeSpan&, const Results&);
    void _PrintLatencyPercentiles(const Results&);
    void _PrintLatencyChart(const Histogram<float>& readLatencyHistogram,
        const Histogram<float>& writeLatencyHistogram,
        const Histogram<float>& totalLatencyHistogram);
    void _PrintTimeSpan(const TimeSpan &timeSpan);
    void _PrintTarget(const Target &target, bool fUseThreadsPerFile, bool fUseRequestsPerFile, bool fCompletionRoutines);
    void _PrintDistribution(DistributionType dT, const vector<DistributionRange>& v, char* spc);
    void _PrintEffectiveDistributions(const Results& results);

    string _sResult;

    friend class UnitTests::ResultParserUnitTests;
};