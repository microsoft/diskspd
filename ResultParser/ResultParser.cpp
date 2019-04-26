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

// ResultParser.cpp : Defines the entry point for the DLL application.
//
#include "ResultParser.h"

#include "common.h"

#include <stdio.h>
#include <stdlib.h>
#include <Winternl.h>   //ntdll.dll

#include <Wmistr.h>     //WNODE_HEADER
#include <Evntrace.h>

#include <assert.h>

// TODO: refactor to a single function shared with the XmlResultParser
void ResultParser::_Print(const char *format, ...)
{
    assert(nullptr != format);
    va_list listArg;
    va_start(listArg, format);
    char buffer[4096] = {};
    vsprintf_s(buffer, _countof(buffer), format, listArg);
    va_end(listArg);
    _sResult += buffer;
}

/*****************************************************************************/
// display file size in a user-friendly form
//

void ResultParser::_DisplayFileSize(UINT64 fsize)
{
    if( fsize > (UINT64)10*1024*1024*1024 )    // > 10GB
    {
        _Print("%uGiB", fsize >> 30);
    }
    else if( fsize > (UINT64)10*1024*1024 )    // > 10MB
    {
        _Print("%uMiB", fsize >> 20);
    }
    else if( fsize > 10*1024 )                // > 10KB
    {
        _Print("%uKiB", fsize >> 10);
    }
    else
    {
        _Print("%I64uB", fsize);
    }
}

/*****************************************************************************/
void ResultParser::_DisplayETWSessionInfo(struct ETWSessionInfo sessionInfo) 
{
    _Print("\n\n");
    _Print("          ETW Buffer Settings & Statistics\n");
    _Print("--------------------------------------------------------\n");
    _Print("(KB)                           Buffers   (Secs)   (Mins)\n");
    _Print("Size  | Min |  Max  |  Free  | Written  | Flush    Age\n");

    _Print("%-5lu %5lu     %-5lu  %-2lu  %8lu  %8lu %8d\n\n",
        sessionInfo.ulBufferSize,
        sessionInfo.ulMinimumBuffers,
        sessionInfo.ulMaximumBuffers,
        sessionInfo.ulFreeBuffers,
        sessionInfo.ulBuffersWritten,
        sessionInfo.ulFlushTimer,
        sessionInfo.lAgeLimit);

    _Print("Allocated Buffers:\t%lu\n",
        sessionInfo.ulNumberOfBuffers);

    _Print("Lost Events:\t\t%lu\n",
        sessionInfo.ulEventsLost);

    _Print("Lost Log Buffers:\t%lu\n",
        sessionInfo.ulLogBuffersLost);

    _Print("Lost Real Time Buffers:\t%lu\n",
        sessionInfo.ulRealTimeBuffersLost);
}


/*****************************************************************************/
void ResultParser::_DisplayETW(struct ETWMask ETWMask, struct ETWEventCounters EtwEventCounters) 
{
    _Print("\n\n\nETW:\n");
    _Print("----\n\n");

    if (ETWMask.bDiskIO)
    {
        _Print("\tDisk I/O\n");

        _Print("\t\tRead: %I64u\n", EtwEventCounters.ullIORead);
        _Print("\t\tWrite: %I64u\n", EtwEventCounters.ullIOWrite);
    }
    if (ETWMask.bImageLoad)
    {
        _Print("\tLoad Image\n");

        _Print("\t\tLoad Image: %I64u\n", EtwEventCounters.ullImageLoad);
    }
    if (ETWMask.bMemoryPageFaults)
    {
        _Print("\tMemory Page Faults\n");

        _Print("\t\tCopy on Write: %I64u\n", EtwEventCounters.ullMMCopyOnWrite);
        _Print("\t\tDemand Zero fault: %I64u\n", EtwEventCounters.ullMMDemandZeroFault);
        _Print("\t\tGuard Page fault: %I64u\n", EtwEventCounters.ullMMGuardPageFault);
        _Print("\t\tHard page fault: %I64u\n", EtwEventCounters.ullMMHardPageFault);
        _Print("\t\tTransition fault: %I64u\n", EtwEventCounters.ullMMTransitionFault);
    }
    if (ETWMask.bMemoryHardFaults && !ETWMask.bMemoryPageFaults )
    {
        _Print("\tMemory Hard Faults\n");
        _Print("\t\tHard page fault: %I64u\n", EtwEventCounters.ullMMHardPageFault);
    }
    if (ETWMask.bNetwork)
    {
        _Print("\tNetwork\n");

        _Print("\t\tAccept: %I64u\n", EtwEventCounters.ullNetAccept);
        _Print("\t\tConnect: %I64u\n", EtwEventCounters.ullNetConnect);
        _Print("\t\tDisconnect: %I64u\n", EtwEventCounters.ullNetDisconnect);
        _Print("\t\tReconnect: %I64u\n", EtwEventCounters.ullNetReconnect);
        _Print("\t\tRetransmit: %I64u\n", EtwEventCounters.ullNetRetransmit);
        _Print("\t\tTCP/IP Send: %I64u\n", EtwEventCounters.ullNetTcpSend);
        _Print("\t\tTCP/IP Receive: %I64u\n", EtwEventCounters.ullNetTcpReceive);
        _Print("\t\tUDP/IP Send: %I64u\n", EtwEventCounters.ullNetUdpSend);
        _Print("\t\tUDP/IP Receive: %I64u\n", EtwEventCounters.ullNetUdpReceive);
    }
    if (ETWMask.bProcess)
    {
        _Print("\tProcess\n");

        _Print("\t\tStart: %I64u\n", EtwEventCounters.ullProcessStart);
        _Print("\t\tEnd: %I64u\n", EtwEventCounters.ullProcessEnd);
    }
    if (ETWMask.bRegistry)
    {
        _Print("\tRegistry\n");

        _Print("\t\tNtCreateKey: %I64u\n", 
            EtwEventCounters.ullRegCreate);

        _Print("\t\tNtDeleteKey: %I64u\n",
            EtwEventCounters.ullRegDelete);

        _Print("\t\tNtDeleteValueKey: %I64u\n",
            EtwEventCounters.ullRegDeleteValue);

        _Print("\t\tNtEnumerateKey: %I64u\n",
            EtwEventCounters.ullRegEnumerateKey);

        _Print("\t\tNtEnumerateValueKey: %I64u\n",
            EtwEventCounters.ullRegEnumerateValueKey);

        _Print("\t\tNtFlushKey: %I64u\n",
            EtwEventCounters.ullRegFlush);

        _Print("\t\tNtOpenKey: %I64u\n",
            EtwEventCounters.ullRegOpen);

        _Print("\t\tNtQueryKey: %I64u\n",
            EtwEventCounters.ullRegQuery);

        _Print("\t\tNtQueryMultipleValueKey: %I64u\n",
            EtwEventCounters.ullRegQueryMultipleValue);

        _Print("\t\tNtQueryValueKey: %I64u\n",
            EtwEventCounters.ullRegQueryValue);

        _Print("\t\tNtSetInformationKey: %I64u\n",
            EtwEventCounters.ullRegSetInformation);

        _Print("\t\tNtSetValueKey: %I64u\n",
            EtwEventCounters.ullRegSetValue);
    }
    if (ETWMask.bThread)
    {
        _Print("\tThread\n");

        _Print("\t\tStart: %I64u\n", EtwEventCounters.ullThreadStart);
        _Print("\t\tEnd: %I64u\n", EtwEventCounters.ullThreadEnd);
    }
}

void ResultParser::_PrintTarget(const Target &target, bool fUseThreadsPerFile, bool fUseRequestsPerFile, bool fCompletionRoutines)
{
    _Print("\tpath: '%s'\n", target.GetPath().c_str());
    _Print("\t\tthink time: %ums\n", target.GetThinkTime());
    _Print("\t\tburst size: %u\n", target.GetBurstSize());
    // TODO: completion routines/ports

    switch (target.GetCacheMode())
    {
        case TargetCacheMode::Cached:
            _Print("\t\tusing software cache\n");
            break;
        case TargetCacheMode::DisableLocalCache:
            _Print("\t\tlocal software cache disabled, remote cache enabled\n");
            break;
        case TargetCacheMode::DisableOSCache:
            _Print("\t\tsoftware cache disabled\n");
            break;
    }

    if (target.GetWriteThroughMode() == WriteThroughMode::On)
    {
        // context-appropriate comment on writethrough
        // if sw cache is disabled, commenting on sw write cache is possibly confusing
        switch (target.GetCacheMode())
        {
        case TargetCacheMode::Cached:
        case TargetCacheMode::DisableLocalCache:
            _Print("\t\thardware and software write caches disabled, writethrough on\n");
            break;
        case TargetCacheMode::DisableOSCache:
            _Print("\t\thardware write cache disabled, writethrough on\n");
            break;
        }
    }
    else
    {
        _Print("\t\tusing hardware write cache, writethrough off\n");
    }

    if (target.GetMemoryMappedIoMode() == MemoryMappedIoMode::On)
    {
        _Print("\t\tmemory mapped I/O enabled");
        switch(target.GetMemoryMappedIoFlushMode())
        {
        case MemoryMappedIoFlushMode::ViewOfFile:
            _Print(", flush mode: FlushViewOfFile");
            break;
        case MemoryMappedIoFlushMode::NonVolatileMemory:
            _Print(", flush mode: FlushNonVolatileMemory");
            break;
        case MemoryMappedIoFlushMode::NonVolatileMemoryNoDrain:
            _Print(", flush mode: FlushNonVolatileMemory with no drain");
            break;
        }
        _Print("\n");
    }

    if (target.GetZeroWriteBuffers())
    {
        _Print("\t\tzeroing write buffers\n");
    }

    if (target.GetRandomDataWriteBufferSize() > 0)
    {
        _Print("\t\twrite buffer size: %I64u\n", target.GetRandomDataWriteBufferSize());
        string sWriteBufferSourcePath = target.GetRandomDataWriteBufferSourcePath();
        if (sWriteBufferSourcePath != "")
        {
            _Print("\t\twrite buffer source: '%s'\n", sWriteBufferSourcePath.c_str());
        }
    }

    if (target.GetUseParallelAsyncIO())
    {
        _Print("\t\tusing parallel async I/O\n");
    }

    if (target.GetWriteRatio() == 0)
    {
        _Print("\t\tperforming read test\n");
    }
    else if (target.GetWriteRatio() == 100)
    {
        _Print("\t\tperforming write test\n");
    }
    else
    {
        _Print("\t\tperforming mix test (read/write ratio: %d/%d)\n", 100 - target.GetWriteRatio(), target.GetWriteRatio());
    }
    _Print("\t\tblock size: %d\n", target.GetBlockSizeInBytes());
    if (target.GetUseRandomAccessPattern())
    {
        _Print("\t\tusing random I/O (alignment: ");
    }
    else
    {
        if (target.GetUseInterlockedSequential())
        {
            _Print("\t\tusing interlocked sequential I/O (stride: ");
        }
        else
        {
            _Print("\t\tusing sequential I/O (stride: ");
        }
    }
    _Print("%I64u)\n", target.GetBlockAlignmentInBytes());

    if (fUseRequestsPerFile)
    {
        _Print("\t\tnumber of outstanding I/O operations: %d\n", target.GetRequestCount());
    }
    
    if (0 != target.GetBaseFileOffsetInBytes())
    {
        _Print("\t\tbase file offset: %I64u\n", target.GetBaseFileOffsetInBytes());
    }

    if (0 != target.GetMaxFileSize())
    {
        _Print("\t\tmax file size: %I64u\n", target.GetMaxFileSize());
    }

    _Print("\t\tthread stride size: %I64u\n", target.GetThreadStrideInBytes());

    if (target.GetSequentialScanHint())
    {
        _Print("\t\tusing FILE_FLAG_SEQUENTIAL_SCAN hint\n");
    }

    if (target.GetRandomAccessHint())
    {
        _Print("\t\tusing FILE_FLAG_RANDOM_ACCESS hint\n");
    }

    if (target.GetTemporaryFileHint())
    {
        _Print("\t\tusing FILE_ATTRIBUTE_TEMPORARY hint\n");
    }

    if (fUseThreadsPerFile)
    {
        _Print("\t\tthreads per file: %d\n", target.GetThreadsPerFile());
    }
    if (target.GetRequestCount() > 1 && fUseThreadsPerFile)
    {
        if (fCompletionRoutines)
        {
            _Print("\t\tusing completion routines (ReadFileEx/WriteFileEx)\n");
        }
        else
        {
            _Print("\t\tusing I/O Completion Ports\n");
        }
    }

    if (target.GetIOPriorityHint() == IoPriorityHintVeryLow)
    {
        _Print("\t\tIO priority: very low\n");
    }
    else if (target.GetIOPriorityHint() == IoPriorityHintLow)
    {
        _Print("\t\tIO priority: low\n");
    }
    else if (target.GetIOPriorityHint() == IoPriorityHintNormal)
    {
        _Print("\t\tIO priority: normal\n");
    }
    else
    {
        _Print("\t\tIO priority: unknown\n");
    }
}

void ResultParser::_PrintTimeSpan(const TimeSpan& timeSpan)
{
    _Print("\tduration: %us\n", timeSpan.GetDuration());
    _Print("\twarm up time: %us\n", timeSpan.GetWarmup());
    _Print("\tcool down time: %us\n", timeSpan.GetCooldown());
    if (timeSpan.GetDisableAffinity())
    {
        _Print("\taffinity disabled\n");
    }
    if (timeSpan.GetMeasureLatency())
    {
        _Print("\tmeasuring latency\n");
    }
    if (timeSpan.GetCalculateIopsStdDev())
    {
        _Print("\tgathering IOPS at intervals of %ums\n", timeSpan.GetIoBucketDurationInMilliseconds());
    }
    _Print("\trandom seed: %u\n", timeSpan.GetRandSeed());

    const auto& vAffinity = timeSpan.GetAffinityAssignments();
    if ( vAffinity.size() > 0)
    {
        _Print("\tadvanced affinity round robin (group/core): ");
        for (unsigned int x = 0; x < vAffinity.size(); ++x)
        {
            _Print("%u/%u", vAffinity[x].wGroup, vAffinity[x].bProc);
            if (x < vAffinity.size() - 1)
            {
                _Print(", ");
            }
        }
        _Print("\n");
    }

    if (timeSpan.GetRandomWriteData())
    {
        _Print("\tgenerating random data for each write IO\n");
        _Print("\t  WARNING: this increases the CPU cost of issuing writes and should only\n");
        _Print("\t           be compared to other results using the -Zr flag\n");
    }

    vector<Target> vTargets(timeSpan.GetTargets());
    for (auto i = vTargets.begin(); i != vTargets.end(); i++)
    {
        _PrintTarget(*i, (timeSpan.GetThreadCount() == 0), (timeSpan.GetThreadCount() == 0 || timeSpan.GetRequestCount() == 0), timeSpan.GetCompletionRoutines());
    }
}

void ResultParser::_PrintProfile(const Profile& profile)
{
    _Print("\nCommand Line: %s\n", profile.GetCmdLine().c_str());
    _Print("\n");
    _Print("Input parameters:\n\n");
    if (profile.GetVerbose())
    {
        _Print("\tusing verbose mode\n");
    }

    const vector<TimeSpan>& vTimeSpans = profile.GetTimeSpans();
    int c = 1;
    for (auto i = vTimeSpans.begin(); i != vTimeSpans.end(); i++)
    {
        _Print("\ttimespan: %3d\n", c++);
        _Print("\t-------------\n");
        _PrintTimeSpan(*i);
        _Print("\n");
    }
}

void ResultParser::_PrintSystemInfo(const SystemInformation& system)
{
    _Print(system.GetText().c_str());
}

void ResultParser::_PrintCpuUtilization(const Results& results, const SystemInformation& system)
{
    size_t ulProcCount = results.vSystemProcessorPerfInfo.size();
    size_t ulBaseProc = 0;
    size_t ulActiveProcCount = 0;
    size_t ulNumGroups = system.processorTopology._vProcessorGroupInformation.size();

    char szFloatBuffer[1024];

    if (ulNumGroups == 1) {
        _Print("\nCPU |  Usage |  User  |  Kernel |  Idle\n");
    }
    else {
        _Print("\nGroup | CPU |  Usage |  User  |  Kernel |  Idle\n");
    }
    _Print("-------------------------------------------\n");

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

            if (ulNumGroups == 1) {
                sprintf_s(szFloatBuffer, sizeof(szFloatBuffer), "%4u| %6.2lf%%| %6.2lf%%|  %6.2lf%%| %6.2lf%%\n",
                    ulProcessor,
                    thisTime,
                    userTime,
                    krnlTime - idleTime,
                    idleTime);
            }
            else {
                sprintf_s(szFloatBuffer, sizeof(szFloatBuffer), "%6u| %4u| %6.2lf%%| %6.2lf%%|  %6.2lf%%| %6.2lf%%\n",
                    ulGroup,
                    ulProcessor,
                    thisTime,
                    userTime,
                    krnlTime - idleTime,
                    idleTime);
            }

            _Print("%s", szFloatBuffer);

            busyTime += thisTime;
            totalIdleTime += idleTime;
            totalUserTime += userTime;
            totalKrnlTime += krnlTime;
            ulActiveProcCount += 1;
        }

        ulBaseProc += pGroup->_maximumProcessorCount;
    }

    if (ulActiveProcCount == 0) {
        ulActiveProcCount = 1;
    }
    
    _Print("-------------------------------------------\n");

    sprintf_s(szFloatBuffer, sizeof(szFloatBuffer), 
        ulNumGroups == 1 ? 
            "avg.| %6.2lf%%| %6.2lf%%|  %6.2lf%%| %6.2lf%%\n" :
            "        avg.| %6.2lf%%| %6.2lf%%|  %6.2lf%%| %6.2lf%%\n",
        busyTime / ulActiveProcCount,
        totalUserTime / ulActiveProcCount,
        (totalKrnlTime - totalIdleTime) / ulActiveProcCount,
        totalIdleTime / ulActiveProcCount);
    _Print("%s", szFloatBuffer);
}

void ResultParser::_PrintSectionFieldNames(const TimeSpan& timeSpan)
{
    _Print("thread |       bytes     |     I/Os     |    MiB/s   |  I/O per s %s%s%s|  file\n",
           timeSpan.GetMeasureLatency()      ? "|  AvgLat  "   : "",
           timeSpan.GetCalculateIopsStdDev() ? "| IopsStdDev " : "",
           timeSpan.GetMeasureLatency()      ? "| LatStdDev "  : "");
}

void ResultParser::_PrintSectionBorderLine(const TimeSpan& timeSpan)
{
    _Print("------------------------------------------------------------------%s%s%s------------\n",
           timeSpan.GetMeasureLatency()      ? "-----------"   : "" ,
           timeSpan.GetCalculateIopsStdDev() ? "-------------" : "",
           timeSpan.GetMeasureLatency()      ? "------------"  : "");
}

void ResultParser::_PrintSection(_SectionEnum section, const TimeSpan& timeSpan, const Results& results)
{
    double fTime = PerfTimer::PerfTimeToSeconds(results.ullTimeCount);
    double fBucketTime = timeSpan.GetIoBucketDurationInMilliseconds() / 1000.0;
    UINT64 ullTotalBytesCount = 0;
    UINT64 ullTotalIOCount = 0;
    Histogram<float> totalLatencyHistogram;
    IoBucketizer totalIoBucketizer;

    _PrintSectionFieldNames(timeSpan);

    _PrintSectionBorderLine(timeSpan);

    for (unsigned int iThread = 0; iThread < results.vThreadResults.size(); ++iThread)
    {
        const ThreadResults& threadResults = results.vThreadResults[iThread];
        for (unsigned int iFile = 0; iFile < threadResults.vTargetResults.size(); iFile++)
        {
            const TargetResults& targetResults = threadResults.vTargetResults[iFile];

            UINT64 ullBytesCount = 0;
            UINT64 ullIOCount = 0;

            Histogram<float> latencyHistogram;
            IoBucketizer ioBucketizer;

            if ((section == _SectionEnum::WRITE) || (section == _SectionEnum::TOTAL))
            {
                ullBytesCount += targetResults.ullWriteBytesCount;
                ullIOCount += targetResults.ullWriteIOCount;

                if (timeSpan.GetMeasureLatency())
                {
                    latencyHistogram.Merge(targetResults.writeLatencyHistogram);
                    totalLatencyHistogram.Merge(targetResults.writeLatencyHistogram);
                }

                if (timeSpan.GetCalculateIopsStdDev())
                {
                    ioBucketizer.Merge(targetResults.writeBucketizer);
                    totalIoBucketizer.Merge(targetResults.writeBucketizer);
                }
            }

            if ((section == _SectionEnum::READ) || (section == _SectionEnum::TOTAL))
            {
                ullBytesCount += targetResults.ullReadBytesCount;
                ullIOCount += targetResults.ullReadIOCount;

                if (timeSpan.GetMeasureLatency())
                {
                    latencyHistogram.Merge(targetResults.readLatencyHistogram);
                    totalLatencyHistogram.Merge(targetResults.readLatencyHistogram);
                }

                if (timeSpan.GetCalculateIopsStdDev())
                {
                    ioBucketizer.Merge(targetResults.readBucketizer);
                    totalIoBucketizer.Merge(targetResults.readBucketizer);
                }
            }

            _Print("%6u | %15llu | %12llu | %10.2f | %10.2f",
                   iThread,
                   ullBytesCount,
                   ullIOCount,
                   (double)ullBytesCount / 1024 / 1024 / fTime,
                   (double)ullIOCount / fTime);

            if (timeSpan.GetMeasureLatency())
            {
                double avgLat = latencyHistogram.GetAvg()/1000;
                _Print(" | %8.3f", avgLat);
            }

            if (timeSpan.GetCalculateIopsStdDev())
            {
                double iopsStdDev = ioBucketizer.GetStandardDeviationIOPS() / fBucketTime;
                _Print(" | %10.2f", iopsStdDev);
            }

            if (timeSpan.GetMeasureLatency())
            {
                if (latencyHistogram.GetSampleSize() > 0)
                {
                    double latStdDev = latencyHistogram.GetStandardDeviation() / 1000;
                    _Print(" |  %8.3f", latStdDev);
                }
                else
                {
                    _Print(" |       N/A");
                }
            }

            _Print(" | %s (", targetResults.sPath.c_str());

            _DisplayFileSize(targetResults.ullFileSize);
            _Print(")\n");

            ullTotalBytesCount += ullBytesCount;
            ullTotalIOCount += ullIOCount;
        }
    }

    _PrintSectionBorderLine(timeSpan);

    double totalAvgLat = 0;

    if (timeSpan.GetMeasureLatency())
    {
        totalAvgLat = totalLatencyHistogram.GetAvg()/1000;
    }

    _Print("total:   %15llu | %12llu | %10.2f | %10.2f",
           ullTotalBytesCount,
           ullTotalIOCount,
           (double)ullTotalBytesCount / 1024 / 1024 / fTime,
           (double)ullTotalIOCount / fTime);

    if (timeSpan.GetMeasureLatency())
    {
        _Print(" | %8.3f", totalAvgLat);
    }

    if (timeSpan.GetCalculateIopsStdDev())
    {
        double iopsStdDev = totalIoBucketizer.GetStandardDeviationIOPS() / fBucketTime;
        _Print(" | %10.2f", iopsStdDev);
    }

    if (timeSpan.GetMeasureLatency())
    {
        if (totalLatencyHistogram.GetSampleSize() > 0)
        {
            double latStdDev = totalLatencyHistogram.GetStandardDeviation() / 1000;
            _Print(" |  %8.3f", latStdDev);
        }
        else
        {
            _Print(" |       N/A");
        }
    }

    _Print("\n");
}

void ResultParser::_PrintLatencyPercentiles(const Results& results)
{
    //Print one chart for each target IF more than one target
    unordered_map<std::string, Histogram<float>> perTargetReadHistogram;
    unordered_map<std::string, Histogram<float>> perTargetWriteHistogram;
    unordered_map<std::string, Histogram<float>> perTargetTotalHistogram;

    for (const auto& thread : results.vThreadResults)
    {
        for (const auto& target : thread.vTargetResults)
        {
            std::string path = target.sPath;

            perTargetReadHistogram[path].Merge(target.readLatencyHistogram);

            perTargetWriteHistogram[path].Merge(target.writeLatencyHistogram);

            perTargetTotalHistogram[path].Merge(target.readLatencyHistogram);
            perTargetTotalHistogram[path].Merge(target.writeLatencyHistogram);
        }
    }

    //Skip if only one target
    if (perTargetTotalHistogram.size() > 1) {
        for (auto i : perTargetTotalHistogram)
        {
            std::string path = i.first;
            _Print("\n%s\n", path.c_str());
            _PrintLatencyChart(perTargetReadHistogram[path],
                perTargetWriteHistogram[path],
                perTargetTotalHistogram[path]);
        }
    }

    //Print one chart for the latencies aggregated across all targets
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

    _Print("\ntotal:\n");
    _PrintLatencyChart(readLatencyHistogram, writeLatencyHistogram, totalLatencyHistogram);
}

void ResultParser::_PrintLatencyChart(const Histogram<float>& readLatencyHistogram,
    const Histogram<float>& writeLatencyHistogram,
    const Histogram<float>& totalLatencyHistogram)
{
    bool fHasReads = readLatencyHistogram.GetSampleSize() > 0;
    bool fHasWrites = writeLatencyHistogram.GetSampleSize() > 0;

    _Print("  %%-ile |  Read (ms) | Write (ms) | Total (ms)\n");
    _Print("----------------------------------------------\n");

    string readMin =
        fHasReads ?
        Util::DoubleToStringHelper(readLatencyHistogram.GetMin()/1000) :
        "N/A";

    string writeMin =
        fHasWrites ?
        Util::DoubleToStringHelper(writeLatencyHistogram.GetMin() / 1000) :
        "N/A";

    _Print("    min | %10s | %10s | %10.3lf\n", 
           readMin.c_str(), writeMin.c_str(), totalLatencyHistogram.GetMin()/1000);

    PercentileDescriptor percentiles[] =
    {
        {       0.25, "25th"    }, 
        {       0.50, "50th"    },
        {       0.75, "75th"    },
        {       0.90, "90th"    },
        {       0.95, "95th"    },
        {       0.99, "99th"    },
        {      0.999, "3-nines" },
        {     0.9999, "4-nines" },
        {    0.99999, "5-nines" },
        {   0.999999, "6-nines" },
        {  0.9999999, "7-nines" },
        { 0.99999999, "8-nines" },
        { 0.999999999, "9-nines" },
    };

    for (auto p : percentiles)
    {
        string readPercentile =
            fHasReads ?
            Util::DoubleToStringHelper(readLatencyHistogram.GetPercentile(p.Percentile) / 1000) :
            "N/A";

        string writePercentile =
            fHasWrites ?
            Util::DoubleToStringHelper(writeLatencyHistogram.GetPercentile(p.Percentile) / 1000) :
            "N/A";

        _Print("%7s | %10s | %10s | %10.3lf\n",
               p.Name.c_str(),
               readPercentile.c_str(),
               writePercentile.c_str(),
               totalLatencyHistogram.GetPercentile(p.Percentile)/1000);
    }

    string readMax = Util::DoubleToStringHelper(readLatencyHistogram.GetMax() / 1000);
    string writeMax = Util::DoubleToStringHelper(writeLatencyHistogram.GetMax() / 1000);

    _Print("    max | %10s | %10s | %10.3lf\n", 
           fHasReads ? readMax.c_str() : "N/A",
           fHasWrites ? writeMax.c_str() : "N/A",
           totalLatencyHistogram.GetMax()/1000);

	_Print("\nRead latency histogram bins:  %d", readLatencyHistogram.GetBucketCount());
	_Print("\nWrite latency histogram bins: %d", writeLatencyHistogram.GetBucketCount());
}

string ResultParser::ParseResults(Profile& profile, const SystemInformation& system, vector<Results> vResults)
{
    _sResult.clear();

    _PrintProfile(profile);
    _PrintSystemInfo(system);

    for (size_t iResult = 0; iResult < vResults.size(); iResult++)
    {
        _Print("\n\nResults for timespan %d:\n", iResult + 1);
        _Print("*******************************************************************************\n");

        const Results& results = vResults[iResult];
        const TimeSpan& timeSpan = profile.GetTimeSpans()[iResult];

        unsigned int ulProcCount = system.processorTopology._ulActiveProcCount;
        double fTime = PerfTimer::PerfTimeToSeconds(results.ullTimeCount); //test duration

        char szFloatBuffer[1024];

        // There either is a fixed number of threads for all files to share (GetThreadCount() > 0) or a number of threads per file.
        // In the latter case vThreadResults.size() == number of threads per file * file count
        size_t ulThreadCnt = (timeSpan.GetThreadCount() > 0) ? timeSpan.GetThreadCount() : results.vThreadResults.size();

        if (fTime < 0.0000001)
        {
            _Print("The test was interrupted before the measurements began. No results are displayed.\n");
        }
        else
        {
            // TODO: parameters.bCreateFile;

            _Print("\n");
            sprintf_s(szFloatBuffer, sizeof(szFloatBuffer), "actual test time:\t%.2lfs\n", fTime);
            _Print("%s", szFloatBuffer);
            _Print("thread count:\t\t%u\n", ulThreadCnt);

            if (timeSpan.GetThreadCount() != 0 && timeSpan.GetRequestCount() != 0) {
                _Print("request count:\t\t%u\n", timeSpan.GetRequestCount());
            }

            _Print("proc count:\t\t%u\n", ulProcCount);
            _PrintCpuUtilization(results, system);

            _Print("\nTotal IO\n");
            _PrintSection(_SectionEnum::TOTAL, timeSpan, results);

            _Print("\nRead IO\n");
            _PrintSection(_SectionEnum::READ, timeSpan, results);

            _Print("\nWrite IO\n");
            _PrintSection(_SectionEnum::WRITE, timeSpan, results);

            if (timeSpan.GetMeasureLatency())
            {
                _Print("\n\n");
                _PrintLatencyPercentiles(results);
            }

            //etw
            if (results.fUseETW)
            {
                _DisplayETW(results.EtwMask, results.EtwEventCounters);
                _DisplayETWSessionInfo(results.EtwSessionInfo);
            }
        }
    }

    if (vResults.size() > 1)
    {
        _Print("\n\nTotals:\n");
        _Print("*******************************************************************************\n\n");
        _Print("type   |       bytes     |     I/Os     |    MiB/s   |  I/O per s\n");
        _Print("-------------------------------------------------------------------------------\n");


        UINT64 cbTotalWritten = 0;
        UINT64 cbTotalRead = 0;
        UINT64 cTotalWriteIO = 0;
        UINT64 cTotalReadIO = 0;
        UINT64 cTotalTicks = 0;
        for (auto pResults = vResults.begin(); pResults != vResults.end(); pResults++)
        {
            double time = PerfTimer::PerfTimeToSeconds(pResults->ullTimeCount); 
            if (time >= 0.0000001)  // skip timespans that were interrupted
            {
                cTotalTicks += pResults->ullTimeCount;
                auto vThreadResults = pResults->vThreadResults;
                for (auto pThreadResults = vThreadResults.begin(); pThreadResults != vThreadResults.end(); pThreadResults++)
                {
                    for (auto pTargetResults = pThreadResults->vTargetResults.begin(); pTargetResults != pThreadResults->vTargetResults.end(); pTargetResults++)
                    {
                        cbTotalRead += pTargetResults->ullReadBytesCount;
                        cbTotalWritten += pTargetResults->ullWriteBytesCount;
                        cTotalReadIO += pTargetResults->ullReadIOCount;
                        cTotalWriteIO += pTargetResults->ullWriteIOCount;
                    }
                }
            }
        }

        double totalTime = PerfTimer::PerfTimeToSeconds(cTotalTicks);

        _Print("write  | %15I64u | %12I64u | %10.2lf | %10.2lf\n",
               cbTotalWritten,
               cTotalWriteIO,
               (double)cbTotalWritten / 1024 / 1024 / totalTime,
               (double)cTotalWriteIO / totalTime);

        _Print("read   | %15I64u | %12I64u | %10.2lf | %10.2lf\n",
               cbTotalRead,
               cTotalReadIO,
               (double)cbTotalRead / 1024 / 1024 / totalTime,
               (double)cTotalReadIO / totalTime);
        _Print("-------------------------------------------------------------------------------\n");
        _Print("total  | %15I64u | %12I64u | %10.2lf | %10.2lf\n\n",
               cbTotalRead + cbTotalWritten,
               cTotalReadIO + cTotalWriteIO,
               (double)(cbTotalRead + cbTotalWritten) / 1024 / 1024 / totalTime,
               (double)(cTotalReadIO + cTotalWriteIO) / totalTime);

        _Print("total test time:\t%.2lfs\n", totalTime);
    }

    return _sResult;
}
