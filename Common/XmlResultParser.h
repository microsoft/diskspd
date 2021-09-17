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

class XmlResultParser: public IResultParser
{
public:
    string ParseResults(const Profile& profile, const SystemInformation& system, vector<Results> vResults);
    string ParseProfile(const Profile& profile);

private:
    void _PrintCpuUtilization(const Results& results, const SystemInformation& system);
    void _PrintETW(struct ETWMask ETWMask, struct ETWEventCounters EtwEventCounters);
    void _PrintETWSessionInfo(struct ETWSessionInfo sessionInfo);
    void _PrintLatencyPercentiles(const Results& results);
    void _PrintTargetResults(const TargetResults& results);
    void _PrintTargetLatency(const TargetResults& results);
    void _PrintTargetIops(const IoBucketizer& readBucketizer, const IoBucketizer& writeBucketizer, UINT32 bucketTimeInMs);
    void _PrintOverallIops(const Results& results, UINT32 bucketTimeInMs);
    void _PrintIops(const IoBucketizer& readBucketizer, const IoBucketizer& writeBucketizer, UINT32 bucketTimeInMs);

    void _Print(const char *format, ...);
    void _PrintInc(const char *format, ...);
    void _PrintDec(const char *format, ...);

    string _sResult;
    UINT32 _indent = 0;
};