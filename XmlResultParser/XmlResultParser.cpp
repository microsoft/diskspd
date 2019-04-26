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

#include "xmlresultparser.h"

// TODO: refactor to a single function shared with the ResultParser
void XmlResultParser::_Print(const char *format, ...)
{
    assert(nullptr != format);
    va_list listArg;
    va_start(listArg, format);
    char buffer[4096] = {};
    vsprintf_s(buffer, _countof(buffer), format, listArg);
    va_end(listArg);
    _sResult += buffer;
}


void XmlResultParser::_PrintTargetResults(const TargetResults& results)
{
    // TODO: results.readBucketizer;
    // TODO: results.writeBucketizer;

    _Print("<Path>%s</Path>\n", results.sPath.c_str());
    _Print("<BytesCount>%I64u</BytesCount>\n", results.ullBytesCount);
    _Print("<FileSize>%I64u</FileSize>\n", results.ullFileSize);
    _Print("<IOCount>%I64u</IOCount>\n", results.ullIOCount);
    _Print("<ReadBytes>%I64u</ReadBytes>\n", results.ullReadBytesCount);
    _Print("<ReadCount>%I64u</ReadCount>\n", results.ullReadIOCount);
    _Print("<WriteBytes>%I64u</WriteBytes>\n", results.ullWriteBytesCount);
    _Print("<WriteCount>%I64u</WriteCount>\n", results.ullWriteIOCount);
}

void XmlResultParser::_PrintTargetLatency(const TargetResults& results)
{
    if (results.readLatencyHistogram.GetSampleSize() > 0)
    {
        _Print("<AverageReadLatencyMilliseconds>%.3f</AverageReadLatencyMilliseconds>\n", results.readLatencyHistogram.GetAvg() / 1000);
        _Print("<ReadLatencyStdev>%.3f</ReadLatencyStdev>\n", results.readLatencyHistogram.GetStandardDeviation() / 1000);
    }
    if (results.writeLatencyHistogram.GetSampleSize() > 0)
    {
        _Print("<AverageWriteLatencyMilliseconds>%.3f</AverageWriteLatencyMilliseconds>\n", results.writeLatencyHistogram.GetAvg() / 1000);
        _Print("<WriteLatencyStdev>%.3f</WriteLatencyStdev>\n", results.writeLatencyHistogram.GetStandardDeviation() / 1000);
    }
    Histogram<float> totalLatencyHistogram;
    totalLatencyHistogram.Merge(results.readLatencyHistogram);
    totalLatencyHistogram.Merge(results.writeLatencyHistogram);
    if (totalLatencyHistogram.GetSampleSize() > 0)
    {
        _Print("<AverageLatencyMilliseconds>%.3f</AverageLatencyMilliseconds>\n", totalLatencyHistogram.GetAvg() / 1000);
        _Print("<LatencyStdev>%.3f</LatencyStdev>\n", totalLatencyHistogram.GetStandardDeviation() / 1000);
    }
}

void XmlResultParser::_PrintTargetIops(const IoBucketizer& readBucketizer, const IoBucketizer& writeBucketizer, UINT32 bucketTimeInMs)
{
    _Print("<Iops>\n");

    IoBucketizer totalIoBucketizer;
    totalIoBucketizer.Merge(readBucketizer);
    totalIoBucketizer.Merge(writeBucketizer);

    if (readBucketizer.GetNumberOfValidBuckets() > 0)
    {
        _Print("<ReadIopsStdDev>%.3f</ReadIopsStdDev>\n", readBucketizer.GetStandardDeviationIOPS() / (bucketTimeInMs / 1000.0));
    }
    if (writeBucketizer.GetNumberOfValidBuckets() > 0)
    {
        _Print("<WriteIopsStdDev>%.3f</WriteIopsStdDev>\n", writeBucketizer.GetStandardDeviationIOPS() / (bucketTimeInMs / 1000.0));
    }
    if (totalIoBucketizer.GetNumberOfValidBuckets() > 0)
    {
        _Print("<IopsStdDev>%.3f</IopsStdDev>\n", totalIoBucketizer.GetStandardDeviationIOPS() / (bucketTimeInMs / 1000.0));
    }
    _PrintIops(readBucketizer, writeBucketizer, bucketTimeInMs);
    _Print("</Iops>\n");
}

void XmlResultParser::_PrintETWSessionInfo(struct ETWSessionInfo sessionInfo)
{
    _Print("<ETWSessionInfo>\n");
    _Print("<BufferSizeKB>%lu</BufferSizeKB>\n", sessionInfo.ulBufferSize);
    _Print("<MinimimBuffers>%lu</MinimimBuffers>\n", sessionInfo.ulMinimumBuffers);
    _Print("<MaximumBuffers>%lu</MaximumBuffers>\n", sessionInfo.ulMaximumBuffers);
    _Print("<FreeBuffers>%lu</FreeBuffers>", sessionInfo.ulFreeBuffers);
    _Print("<BuffersWritten>%lu</BuffersWritten>\n", sessionInfo.ulBuffersWritten);
    _Print("<FlushTimerSeconds>%lu</FlushTimerSeconds>\n", sessionInfo.ulFlushTimer);
    _Print("<AgeLimitMinutes>%d</AgeLimitMinutes>\n", sessionInfo.lAgeLimit);

    _Print("<AllocatedBuffers>%lu</AllocatedBuffers>\n", sessionInfo.ulNumberOfBuffers);
    _Print("<LostEvents>%lu</LostEvents>\n", sessionInfo.ulEventsLost);
    _Print("<LostLogBuffers>%lu</LostLogBuffers>\n", sessionInfo.ulLogBuffersLost);
    _Print("<LostRealTimeBuffers>%lu</LostRealTimeBuffers>\n", sessionInfo.ulRealTimeBuffersLost);
    _Print("</ETWSessionInfo>\n");
}

void XmlResultParser::_PrintETW(struct ETWMask ETWMask, struct ETWEventCounters EtwEventCounters)
{
    _Print("<ETW>\n");
    if (ETWMask.bDiskIO)
    {
        _Print("<DiskIO>\n");
        _Print("<Read>%I64u</Read>\n", EtwEventCounters.ullIORead);
        _Print("<Write>%I64u</Write>\n", EtwEventCounters.ullIOWrite);
        _Print("</DiskIO>\n");
    }
    if (ETWMask.bImageLoad)
    {
        _Print("<LoadImage>%I64u</LoadImage>\n", EtwEventCounters.ullImageLoad);
    }
    if (ETWMask.bMemoryPageFaults)
    {
        _Print("<MemoryPageFaults>\n");
        _Print("<CopyOnWrite>%I64u</CopyOnWrite>\n", EtwEventCounters.ullMMCopyOnWrite);
        _Print("<DemandZeroFault>%I64u</DemandZeroFault>\n", EtwEventCounters.ullMMDemandZeroFault);
        _Print("<GuardPageFault>%I64u</GuardPageFault>\n", EtwEventCounters.ullMMGuardPageFault);
        _Print("<HardPageFault>%I64u</HardPageFault>\n", EtwEventCounters.ullMMHardPageFault);
        _Print("<TransitionFault>%I64u</TransitionFault>\n", EtwEventCounters.ullMMTransitionFault);
        _Print("</MemoryPageFaults>\n");
    }
    if (ETWMask.bMemoryHardFaults && !ETWMask.bMemoryPageFaults)
    {
        _Print("<HardPageFault>%I64u</HardPageFault>\n", EtwEventCounters.ullMMHardPageFault);
    }
    if (ETWMask.bNetwork)
    {
        _Print("<Network>\n");
        _Print("<Accept>%I64u</Accept>\n", EtwEventCounters.ullNetAccept);
        _Print("<Connect>%I64u</Connect>\n", EtwEventCounters.ullNetConnect);
        _Print("<Disconnect>%I64u</Disconnect>\n", EtwEventCounters.ullNetDisconnect);
        _Print("<Reconnect>%I64u</Reconnect>\n", EtwEventCounters.ullNetReconnect);
        _Print("<Retransmit>%I64u</Retransmit>\n", EtwEventCounters.ullNetRetransmit);
        _Print("<TCPIPSend>%I64u</TCPIPSend>\n", EtwEventCounters.ullNetTcpSend);
        _Print("<TCPIPReceive>%I64u</TCPIPReceive>\n", EtwEventCounters.ullNetTcpReceive);
        _Print("<UDPIPSend>%I64u</UDPIPSend>\n", EtwEventCounters.ullNetUdpSend);
        _Print("<UDPIPReceive>%I64u</UDPIPReceive>\n", EtwEventCounters.ullNetUdpReceive);
        _Print("</Network>\n");
    }
    if (ETWMask.bProcess)
    {
        _Print("<Process>\n");
        _Print("<Start>%I64u</Start>\n", EtwEventCounters.ullProcessStart);
        _Print("<End>%I64u</End>\n", EtwEventCounters.ullProcessEnd);
        _Print("</Process>\n");
    }
    if (ETWMask.bRegistry)
    {
        _Print("<Registry>\n");
        _Print("<NtCreateKey>%I64u</NtCreateKey>\n", EtwEventCounters.ullRegCreate);
        _Print("<NtDeleteKey>%I64u</NtDeleteKey>\n", EtwEventCounters.ullRegDelete);
        _Print("<NtDeleteValueKey>%I64u</NtDeleteValueKey>\n", EtwEventCounters.ullRegDeleteValue);
        _Print("<NtEnumerateKey>%I64u</NtEnumerateKey>\n", EtwEventCounters.ullRegEnumerateKey);
        _Print("<NtEnumerateValueKey>%I64u</NtEnumerateValueKey>\n", EtwEventCounters.ullRegEnumerateValueKey);
        _Print("<NtFlushKey>%I64u</NtFlushKey>\n", EtwEventCounters.ullRegFlush);
        _Print("<NtOpenKey>%I64u</NtOpenKey>\n", EtwEventCounters.ullRegOpen);
        _Print("<NtQueryKey>%I64u</NtQueryKey>\n", EtwEventCounters.ullRegQuery);
        _Print("<NtQueryMultipleValueKey>%I64u</NtQueryMultipleValueKey>\n", EtwEventCounters.ullRegQueryMultipleValue);
        _Print("<NtQueryValueKey>%I64u</NtQueryValueKey>\n", EtwEventCounters.ullRegQueryValue);
        _Print("<NtSetInformationKey>%I64u</NtSetInformationKey>\n", EtwEventCounters.ullRegSetInformation);
        _Print("<NtSetValueKey>%I64u</NtSetValueKey>\n", EtwEventCounters.ullRegSetValue);
        _Print("</Registry>\n");
    }
    if (ETWMask.bThread)
    {
        _Print("<Thread>\n");
        _Print("<Start>%I64u</Start>\n", EtwEventCounters.ullThreadStart);
        _Print("<End>%I64u</End>\n", EtwEventCounters.ullThreadEnd);
        _Print("</Thread>\n");
    }
    _Print("</ETW>\n");
}

void XmlResultParser::_PrintCpuUtilization(const Results& results, const SystemInformation& system)
{
    size_t ulProcCount = results.vSystemProcessorPerfInfo.size();
    size_t ulBaseProc = 0;
    size_t ulActiveProcCount = 0;
    size_t ulNumGroups = system.processorTopology._vProcessorGroupInformation.size();

    _Print("<CpuUtilization>\n");

    double busyTime = 0;
    double totalIdleTime = 0;
    double totalUserTime = 0;
    double totalKrnlTime = 0;

    for (unsigned int ulGroup = 0; ulGroup < ulNumGroups; ulGroup++) {
        const ProcessorGroupInformation *pGroup = &system.processorTopology._vProcessorGroupInformation[ulGroup];

        // System has multiple groups but we only have counters for the first one
        if (ulBaseProc >= ulProcCount) {
            break;
        }
        
        for (unsigned int ulProcessor = 0; ulProcessor < pGroup->_maximumProcessorCount; ulProcessor++) {
            double idleTime;
            double userTime;
            double krnlTime;
            double thisTime;

            if (!pGroup->IsProcessorActive((BYTE)ulProcessor)) {
                continue;
            }

            long long fTime = results.vSystemProcessorPerfInfo[ulBaseProc + ulProcessor].KernelTime.QuadPart +
                              results.vSystemProcessorPerfInfo[ulBaseProc + ulProcessor].UserTime.QuadPart;

            idleTime = 100.0 * results.vSystemProcessorPerfInfo[ulBaseProc + ulProcessor].IdleTime.QuadPart / fTime;
            krnlTime = 100.0 * results.vSystemProcessorPerfInfo[ulBaseProc + ulProcessor].KernelTime.QuadPart / fTime;
            userTime = 100.0 * results.vSystemProcessorPerfInfo[ulBaseProc + ulProcessor].UserTime.QuadPart / fTime;

            thisTime = (krnlTime + userTime) - idleTime;

            _Print("<CPU>\n");
            _Print("<Group>%d</Group>\n", ulGroup);
            _Print("<Id>%d</Id>\n", ulProcessor);
            _Print("<UsagePercent>%.2f</UsagePercent>\n", thisTime);
            _Print("<UserPercent>%.2f</UserPercent>\n", userTime);
            _Print("<KernelPercent>%.2f</KernelPercent>\n", krnlTime - idleTime);
            _Print("<IdlePercent>%.2f</IdlePercent>\n", idleTime);
            _Print("</CPU>\n");

            busyTime += thisTime;
            totalIdleTime += idleTime;
            totalUserTime += userTime;
            totalKrnlTime += krnlTime;

            ulActiveProcCount++;
        }

        ulBaseProc += pGroup->_maximumProcessorCount;
    }

    if (ulActiveProcCount == 0) {
        ulActiveProcCount = 1;
    }

    _Print("<Average>\n");
    _Print("<UsagePercent>%.2f</UsagePercent>\n", busyTime / ulActiveProcCount);
    _Print("<UserPercent>%.2f</UserPercent>\n", totalUserTime / ulActiveProcCount);
    _Print("<KernelPercent>%.2f</KernelPercent>\n", (totalKrnlTime - totalIdleTime) / ulActiveProcCount);
    _Print("<IdlePercent>%.2f</IdlePercent>\n", totalIdleTime / ulActiveProcCount);
    _Print("</Average>\n");

    _Print("</CpuUtilization>\n");
}

// emit the iops time series (this obviates needing perfmon counters, in common cases, and provides file level data)
void XmlResultParser::_PrintIops(const IoBucketizer& readBucketizer, const IoBucketizer& writeBucketizer, UINT32 bucketTimeInMs)
{
    bool done = false;
    for (size_t i = 0; !done; i++)
    {
        done = true;

        double r = 0.0;
        double r_min = 0.0;
        double r_max = 0.0;
        double r_avg = 0.0;
        double r_stddev = 0.0;

        double w = 0.0;
        double w_min = 0.0;
        double w_max = 0.0;
        double w_avg = 0.0;
        double w_stddev = 0.0;

        if (readBucketizer.GetNumberOfValidBuckets() > i)
        {
            r = readBucketizer.GetIoBucketCount(i) / (bucketTimeInMs / 1000.0);
            r_min = readBucketizer.GetIoBucketMinDurationUsec(i) / 1000.0;
            r_max = readBucketizer.GetIoBucketMaxDurationUsec(i) / 1000.0;
            r_avg = readBucketizer.GetIoBucketAvgDurationUsec(i) / 1000.0;
            r_stddev = readBucketizer.GetIoBucketDurationStdDevUsec(i) / 1000.0;
            done = false;
        }
        if (writeBucketizer.GetNumberOfValidBuckets() > i)
        {
            w = writeBucketizer.GetIoBucketCount(i) / (bucketTimeInMs / 1000.0);
            w_min = writeBucketizer.GetIoBucketMinDurationUsec(i) / 1000.0;
            w_max = writeBucketizer.GetIoBucketMaxDurationUsec(i) / 1000.0;
            w_avg = writeBucketizer.GetIoBucketAvgDurationUsec(i) / 1000.0;
            w_stddev = writeBucketizer.GetIoBucketDurationStdDevUsec(i) / 1000.0;
            done = false;
        }
        if (!done)
        {
            _Print("<Bucket SampleMillisecond=\"%lu\" Read=\"%.0f\" Write=\"%.0f\" Total=\"%.0f\" "
                   "ReadMinLatencyMilliseconds=\"%.3f\" ReadMaxLatencyMilliseconds=\"%.3f\" ReadAvgLatencyMilliseconds=\"%.3f\" ReadLatencyStdDev=\"%.3f\" "
                   "WriteMinLatencyMilliseconds=\"%.3f\" WriteMaxLatencyMilliseconds=\"%.3f\" WriteAvgLatencyMilliseconds=\"%.3f\" WriteLatencyStdDev=\"%.3f\"/>\n", 
                   bucketTimeInMs*(i + 1), r, w, r + w,
                   r_min, r_max, r_avg, r_stddev,
                   w_min, w_max, w_avg, w_stddev);
        }
    }
}

void XmlResultParser::_PrintOverallIops(const Results& results, UINT32 bucketTimeInMs)
{
    IoBucketizer readBucketizer;
    IoBucketizer writeBucketizer;

    for (const auto& thread : results.vThreadResults)
    {
        for (const auto& target : thread.vTargetResults)
        {
            readBucketizer.Merge(target.readBucketizer);
            writeBucketizer.Merge(target.writeBucketizer);
        }
    }

    _PrintTargetIops(readBucketizer, writeBucketizer, bucketTimeInMs);
}

void XmlResultParser::_PrintLatencyPercentiles(const Results& results)
{
    Histogram<float> readLatencyHistogram;
    Histogram<float> writeLatencyHistogram;
    Histogram<float> totalLatencyHistogram;

    for (const auto& thread : results.vThreadResults)
    {
        for (const auto& target : thread.vTargetResults)
        {
            readLatencyHistogram.Merge(target.readLatencyHistogram);

            writeLatencyHistogram.Merge(target.writeLatencyHistogram);

            totalLatencyHistogram.Merge(target.writeLatencyHistogram);
            totalLatencyHistogram.Merge(target.readLatencyHistogram);
        }
    }

    _Print("<Latency>\n");
    if (readLatencyHistogram.GetSampleSize() > 0)
    {
        _Print("<AverageReadMilliseconds>%.3f</AverageReadMilliseconds>\n", readLatencyHistogram.GetAvg() / 1000);
        _Print("<ReadLatencyStdev>%.3f</ReadLatencyStdev>\n", readLatencyHistogram.GetStandardDeviation() / 1000);
    }
    if (writeLatencyHistogram.GetSampleSize() > 0)
    {
        _Print("<AverageWriteMilliseconds>%.3f</AverageWriteMilliseconds>\n", writeLatencyHistogram.GetAvg() / 1000);
        _Print("<WriteLatencyStdev>%.3f</WriteLatencyStdev>\n", writeLatencyHistogram.GetStandardDeviation() / 1000);
    }
    if (totalLatencyHistogram.GetSampleSize() > 0)
    {
        _Print("<AverageTotalMilliseconds>%.3f</AverageTotalMilliseconds>\n", totalLatencyHistogram.GetAvg() / 1000);
        _Print("<LatencyStdev>%.3f</LatencyStdev>\n", totalLatencyHistogram.GetStandardDeviation() / 1000);
    }

    _Print("<Bucket>\n");
    _Print("<Percentile>0</Percentile>\n");
    if (readLatencyHistogram.GetSampleSize() > 0)
    {
        _Print("<ReadMilliseconds>%.3f</ReadMilliseconds>\n", readLatencyHistogram.GetMin() / 1000);
    }
    if (writeLatencyHistogram.GetSampleSize() > 0)
    {
        _Print("<WriteMilliseconds>%.3f</WriteMilliseconds>\n", writeLatencyHistogram.GetMin() / 1000);
    }
    if (totalLatencyHistogram.GetSampleSize() > 0)
    {
        _Print("<TotalMilliseconds>%.3f</TotalMilliseconds>\n", totalLatencyHistogram.GetMin() / 1000);
    }
    _Print("</Bucket>\n");

    //  Construct vector of percentiles and decimal precision to squelch trailing zeroes.  This is more
    //  detailed than summary text output, and does not contain the decorated names (15th, etc.)

    vector<pair<int, double>> vPercentiles;
    for (int p = 1; p <= 99; p++)
    {
        vPercentiles.push_back(make_pair(0, p));
    }

    vPercentiles.push_back(make_pair(1, 99.9));
    vPercentiles.push_back(make_pair(2, 99.99));
    vPercentiles.push_back(make_pair(3, 99.999));
    vPercentiles.push_back(make_pair(4, 99.9999));
    vPercentiles.push_back(make_pair(5, 99.99999));
    vPercentiles.push_back(make_pair(6, 99.999999));
    vPercentiles.push_back(make_pair(7, 99.9999999));

    for (auto p : vPercentiles)
    {
        _Print("<Bucket>\n");
        _Print("<Percentile>%.*f</Percentile>\n", p.first, p.second);
        if (readLatencyHistogram.GetSampleSize() > 0)
        {
            _Print("<ReadMilliseconds>%.3f</ReadMilliseconds>\n", readLatencyHistogram.GetPercentile(p.second / 100) / 1000);
        }
        if (writeLatencyHistogram.GetSampleSize() > 0)
        {
            _Print("<WriteMilliseconds>%.3f</WriteMilliseconds>\n", writeLatencyHistogram.GetPercentile(p.second / 100) / 1000);
        }
        if (totalLatencyHistogram.GetSampleSize() > 0)
        {
            _Print("<TotalMilliseconds>%.3f</TotalMilliseconds>\n", totalLatencyHistogram.GetPercentile(p.second / 100) / 1000);
        }
        _Print("</Bucket>\n");
    }

    _Print("<Bucket>\n");
    _Print("<Percentile>100</Percentile>\n"); 
    if (readLatencyHistogram.GetSampleSize() > 0)
    {
        _Print("<ReadMilliseconds>%.3f</ReadMilliseconds>\n", readLatencyHistogram.GetMax() / 1000);
    }
    if (writeLatencyHistogram.GetSampleSize() > 0)
    {
        _Print("<WriteMilliseconds>%.3f</WriteMilliseconds>\n", writeLatencyHistogram.GetMax() / 1000);
    }
    if (totalLatencyHistogram.GetSampleSize() > 0)
    {
        _Print("<TotalMilliseconds>%.3f</TotalMilliseconds>\n", totalLatencyHistogram.GetMax() / 1000);
    }
    _Print("</Bucket>\n");

	_Print("<ReadLatencyHistogramBins>%d</ReadLatencyHistogramBins>\n", readLatencyHistogram.GetBucketCount());
	_Print("<WriteLatencyHistogramBins>%d</WriteLatencyHistogramBins>\n", writeLatencyHistogram.GetBucketCount());

    _Print("</Latency>\n");
}

string XmlResultParser::ParseResults(Profile& profile, const SystemInformation& system, vector<Results> vResults)
{
    _sResult.clear();

    _Print("<Results>\n");
    _sResult += system.GetXml();
    _sResult += profile.GetXml();
    for (size_t iResults = 0; iResults < vResults.size(); iResults++)
    {
        const Results& results = vResults[iResults];
        const TimeSpan& timeSpan = profile.GetTimeSpans()[iResults];

        _Print("<TimeSpan>\n");
        double fTime = PerfTimer::PerfTimeToSeconds(results.ullTimeCount); //test duration
        if (fTime >= 0.0000001)
        {
            // There either is a fixed number of threads for all files to share (GetThreadCount() > 0) or a number of threads per file.
            // In the latter case vThreadResults.size() == number of threads per file * file count
            size_t ulThreadCnt = (timeSpan.GetThreadCount() > 0) ? timeSpan.GetThreadCount() : results.vThreadResults.size();
            unsigned int ulProcCount = system.processorTopology._ulActiveProcCount;

            _Print("<TestTimeSeconds>%.2f</TestTimeSeconds>\n", fTime);
            _Print("<ThreadCount>%u</ThreadCount>\n", ulThreadCnt);
            _Print("<RequestCount>%u</RequestCount>\n", timeSpan.GetRequestCount());
            _Print("<ProcCount>%u</ProcCount>\n", ulProcCount);

            _PrintCpuUtilization(results, system);

            if (timeSpan.GetMeasureLatency())
            {
                _PrintLatencyPercentiles(results);
            }

            if (timeSpan.GetCalculateIopsStdDev())
            {
                _PrintOverallIops(results, timeSpan.GetIoBucketDurationInMilliseconds());
            }

            if (results.fUseETW)
            {
                _PrintETW(results.EtwMask, results.EtwEventCounters);
                _PrintETWSessionInfo(results.EtwSessionInfo);
            }

            for (size_t iThread = 0; iThread < results.vThreadResults.size(); iThread++)
            {
                const ThreadResults& threadResults = results.vThreadResults[iThread];
                _Print("<Thread>\n");
                _Print("<Id>%u</Id>\n", iThread);
                for (const auto& targetResults : threadResults.vTargetResults)
                {
                    _Print("<Target>\n");
                    _PrintTargetResults(targetResults);
                    if (timeSpan.GetMeasureLatency())
                    {
                        _PrintTargetLatency(targetResults);
                    }
                    if (timeSpan.GetCalculateIopsStdDev())
                    {
                        _PrintTargetIops(targetResults.readBucketizer, targetResults.writeBucketizer, timeSpan.GetIoBucketDurationInMilliseconds());
                    }
                    _Print("</Target>\n");
                }
                _Print("</Thread>\n");
            }
        }
        else
        {
            _Print("<Error>The test was interrupted before the measurements began. No results are displayed.</Error>\n");
        }
        _Print("</TimeSpan>\n");
    }
    _Print("</Results>");
    return _sResult;
}
