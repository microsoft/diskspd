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
#include <functional>

#include <stdio.h>
#include <stdlib.h>
#include <Winternl.h>   //ntdll.dll

#include <Wmistr.h>     //WNODE_HEADER
#include <Evntrace.h>

#include <assert.h>

// TODO: refactor to a single function shared with the XmlResultParser
// Note: not thread safe (avoid 4K on the stack)

static char printBuffer[4096] = {};

void ResultParser::_Print(const char *format, ...)
{
    assert(nullptr != format);
    va_list listArg;
    va_start(listArg, format);
    vsprintf_s(printBuffer, _countof(printBuffer), format, listArg);
    va_end(listArg);
    _sResult += printBuffer;
}

/*****************************************************************************/
// display file size in a user-friendly form
//

void ResultParser::_PrintFileSize(UINT64 fsize, UINT32 align)
{
    string s = Util::GetSizeKMGT(fsize);

    if (align > s.size())
    {
        _sResult.append(align - s.size(), ' ');
    }
    _sResult += s;
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

class DistributionRef {
public:

    DistributionRef(
        const string &TargetPath,
        UINT32 Thread
    )
    {
        set<UINT32> s;
        s.insert(Thread);

        _mTargetThreads.emplace(make_pair(TargetPath, std::move(s)));
    }

    //
    // Map a target to the set of threads referencing it with a given distribution
    //

    map<string, set<UINT32>> _mTargetThreads;
};

namespace std
{
    template<>
    struct less<const Distribution *>
    {
        // map by pointer, compare with the distribution ranges
        bool operator()(const Distribution * const &lhs, const Distribution * const &rhs) const
        {
            return lhs->GetRanges() < rhs->GetRanges();
        }
    };
}

void ResultParser::_PrintEffectiveDistributions(const Results& results)
{
    //
    // Effective distributions can be distinct per target if they vary in size.
    // While not possible at the command line, more complex configurations can
    // in general specify a distribution per target per thread.
    //
    // This deduplicates the effective distributions so that we report each
    // with the target/thread list which used the (equivalent) distribution
    // to access the target.
    //

    bool header = false;
    UINT32 threadNo = 0;
    map<const Distribution *, DistributionRef> m;

    for (auto& thResult : results.vThreadResults)
    {
        for (auto& tgtResult : thResult.vTargetResults)
        {
            if (tgtResult.distribution.HasRanges())
            {
                auto it = m.find(&tgtResult.distribution);
                if (it == m.end())
                {
                    m.emplace(make_pair(&tgtResult.distribution,
                                        DistributionRef(tgtResult.sPath, threadNo)));
                }
                else
                {
                    it->second._mTargetThreads[tgtResult.sPath].insert(threadNo);
                }
            }
        }

        ++threadNo;
    }

    for (auto& r : m)
    {
        if (!header)
        {
            header = true;
            _Print("\nEffective IO Distributions\n--------------------------\n");
        }
        for (auto& tgt : r.second._mTargetThreads)
        {
            _Print("target: %s [thread:", tgt.first.c_str());

            UINT32 lastTh = MAXUINT, runLen = 0;

            for (auto& th : tgt.second)
            {
                if (lastTh != MAXUINT)
                {
                    // accumulate run?
                    if (lastTh + 1 == th) {
                        lastTh = th;
                        ++runLen;
                        continue;
                    }

                    // end of run - indicate ellision of actual runs
                    if (runLen > 1)
                    {
                        _Print(" -");
                    }
                    _Print(" %u", lastTh);
                }

                // start new run (may be singular)
                _Print(" %u", th);
                lastTh = th;
                runLen = 0;
            }

            // terminate final run
            if (runLen > 1)
            {
                _Print(" -");
            }
            // don't show last thread twice if it terminated run
            if (runLen)
            {
                _Print(" %u", lastTh);
            }

            _Print("]\n");
        }
        _sResult += r.first->GetText(0);
    }
}

void ResultParser::_PrintProfile(const Profile& profile)
{
    _Print("\nCommand Line: %s\n", profile.GetCmdLine().c_str());
    _Print("\n");
    if (g_ExperimentFlags)
    {
        _Print("Experiment Flags: 0x%x (%u)\n", g_ExperimentFlags, g_ExperimentFlags);
        _Print("\n");
    }
    _Print("Input parameters:\n\n");
    if (profile.GetVerbose())
    {
        _Print("  using verbose mode\n");
    }

    const vector<TimeSpan>& vTimeSpans = profile.GetTimeSpans();
    int c = 1;
    for (auto i = vTimeSpans.begin(); i != vTimeSpans.end(); i++)
    {
        _Print("  timespan: %3d\n", c++);
        _Print("  -------------\n");
        _sResult += i->GetText(4);  // timespan properties at indent 4
        _Print("\n");
    }
}

void ResultParser::_PrintSystemInfo(const SystemInformation& system)
{
    _Print(system.GetText().c_str());
}

string ResultParser::ParseSystemInformation(const SystemInformation& system)
{
    _sResult.clear();
    _PrintSystemInfo(system);
    return _sResult;
}

void ResultParser::_PrintCompactAffinity(const vector<AffinityAssignment>& v, bool fMultiGroup)
{
    auto vCompact = AffinityGroupMask::Compact(v);
    for (size_t i = 0; i < vCompact.size(); i++)
    {
        if (i > 0)
        {
            _Print(" ");
        }
        if (fMultiGroup)
        {
            _Print("%u/", vCompact[i].wGroup);
        }
        _Print("%s", Util::MaskRanges(vCompact[i].mask).c_str());
    }
}

void ResultParser::_PrintEffectiveAffinity(const TimeSpan& timeSpan, const SystemInformation& system)
{
    assert(timeSpan.IsFinalized());

    if (timeSpan.GetDisableAffinity())
    {
        return;
    }

    const auto& vEffective = timeSpan.GetEffectiveAffinityAssignments();
    if (vEffective.empty())
    {
        return;
    }

    bool fMultiGroup = system.processorTopology._vProcessorGroupInformation.size() > 1;

    _Print(fMultiGroup ? "\neffective affinity (group/cpu): " : "\neffective affinity (cpu): ");
    _PrintCompactAffinity(vEffective, fMultiGroup);
    _Print("\n");
}

void ResultParser::_PrintHeterogeneousAffinityWarning(const TimeSpan& timeSpan, const SystemInformation& system)
{
    assert(timeSpan.IsFinalized());

    if (system.processorTopology._ubPerformanceEfficiencyClass == 0)
    {
        return;
    }

    const auto& vEffective = timeSpan.GetEffectiveAffinityAssignments();
    if (vEffective.empty())
    {
        return;
    }

    BYTE maxEffClass = system.processorTopology._ubPerformanceEfficiencyClass;

    vector<AffinityAssignment> vPerf, vEff;
    for (const auto& a : vEffective)
    {
        if (a.bEfficiencyClass == maxEffClass)
        {
            vPerf.emplace_back(a.wGroup, a.bProc);
        }
        else
        {
            vEff.emplace_back(a.wGroup, a.bProc);
        }
    }

    if (vPerf.empty() || vEff.empty())
    {
        return;
    }

    //
    // Provide a diagnostic comment on mixed core assignments. This is not a warning since
    // it may be intentional, descriptive if valuable for confirmation by the analyst.
    //

    bool fMultiGroup = system.processorTopology._vProcessorGroupInformation.size() > 1;

    _Print("\nNOTE: thread assignment spans different core types (P-core and E-core).\n");
    _Print("      Performance results may reflect mixed core capabilities.\n");

    _Print("      P-core cpus: ");
    _PrintCompactAffinity(vPerf, fMultiGroup);
    _Print("\n");

    _Print("      E-core cpus: ");
    _PrintCompactAffinity(vEff, fMultiGroup);
    _Print("\n");
}

void ResultParser::_PrintCpuUtilization(const Results& results, const SystemInformation& system)
{
    const auto& topo = system.processorTopology;
    size_t procCount = results.vSystemProcessorPerfInfo.size();
    size_t baseProc = 0;
    BYTE efficiencyClass = 0;
    WORD processorCore = 0;

    bool fMultiSocket = topo._vProcessorSocketInformation.size() > 1;
    bool fMultiNode = topo._vProcessorNumaInformation.size() > 1;
    bool fMultiGroup = topo._vProcessorGroupInformation.size() > 1;

    //
    // Columns dynamically expand based on whether the system has multiple of the following,
    // in hierarchical order, followed by CPU #:
    //
    //      Socket NUMA Group Core Class
    //
    // Note that core & cpu number are group-relative, not absolute (or NUMA or socket relative)
    //

    _Print("\n");
    if (fMultiSocket)    { _Print("Socket | "); }
    if (fMultiNode)      { _Print("Node | "); }
    if (fMultiGroup) { _Print("Group | "); }
    if (topo._fSMT)      { _Print("Core | "); }
    if (topo._ubPerformanceEfficiencyClass) { _Print("Class | "); }
    _Print("CPU |  Usage |  User  | Kernel |  Idle\n");
    if (fMultiSocket)    { _Print("---------"); }
    if (fMultiNode)      { _Print("-------"); }
    if (fMultiGroup) { _Print("--------"); }
    if (topo._fSMT)      { _Print("-------"); }
    if (topo._ubPerformanceEfficiencyClass) { _Print("--------"); }
    _Print("----------------------------------------\n");

    double busyTime = 0;
    double totalIdleTime = 0;
    double totalUserTime = 0;
    double totalKrnlTime = 0;

    for (const auto& group : topo._vProcessorGroupInformation) {

        // Sanity assert - results are sized to the sum of active processors
        assert(baseProc + group._activeProcessorCount <= procCount);

        for (BYTE processor = 0; processor < group._activeProcessorCount; processor++) {

            long long fTime = results.vSystemProcessorPerfInfo[baseProc + processor].KernelTime.QuadPart +
                              results.vSystemProcessorPerfInfo[baseProc + processor].UserTime.QuadPart;

            double idleTime = 100.0 * results.vSystemProcessorPerfInfo[baseProc + processor].IdleTime.QuadPart / fTime;
            double krnlTime = 100.0 * results.vSystemProcessorPerfInfo[baseProc + processor].KernelTime.QuadPart / fTime;
            double userTime = 100.0 * results.vSystemProcessorPerfInfo[baseProc + processor].UserTime.QuadPart / fTime;
            double usedTime = (krnlTime - idleTime) + userTime;

            if (fMultiSocket) {
                _Print("%7u| ", topo.GetSocketOfProcessor(group._groupNumber, processor));
            }
            if (fMultiNode) {
                _Print("%5u| ", topo.GetNumaOfProcessor(group._groupNumber, processor));
            }
            if (fMultiGroup) {
                _Print("%6u| ", group._groupNumber);
            }
            processorCore = topo.GetCoreOfProcessor(group._groupNumber, processor, efficiencyClass);
            if (topo._fSMT){
                _Print("%5u| ", processorCore);
            }
            if (topo._ubPerformanceEfficiencyClass) {
                _Print("%5u%c| ",
                    efficiencyClass,
                    efficiencyClass == topo._ubPerformanceEfficiencyClass ? 'P' : ' ');
            }

            _Print("%4u| %6.2lf%%| %6.2lf%%| %6.2lf%%| %6.2lf%%\n",
                processor,
                usedTime,
                userTime,
                krnlTime - idleTime,
                idleTime);

            busyTime += usedTime;
            totalIdleTime += idleTime;
            totalUserTime += userTime;
            totalKrnlTime += krnlTime;
        }

        baseProc += group._activeProcessorCount;
    }

    assert(baseProc == procCount);

    if (fMultiSocket)    { _Print("---------"); }
    if (fMultiNode)      { _Print("-------"); }
    if (fMultiGroup) { _Print("--------"); }
    if (topo._fSMT)      { _Print("-------"); }
    if (topo._ubPerformanceEfficiencyClass) { _Print("--------"); }
    _Print("----------------------------------------\n");

    if (fMultiSocket)    { _Print("         "); }
    if (fMultiNode)      { _Print("       "); }
    if (fMultiGroup) { _Print("        "); }
    if (topo._fSMT)      { _Print("       "); }
    if (topo._ubPerformanceEfficiencyClass) { _Print("        "); }

    _Print("avg.| %6.2lf%%| %6.2lf%%| %6.2lf%%| %6.2lf%%\n",
        busyTime / procCount,
        totalUserTime / procCount,
        (totalKrnlTime - totalIdleTime) / procCount,
        totalIdleTime / procCount);
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
                _Print(" | %8.3f", latencyHistogram.GetAvg()/1000);
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

            _PrintFileSize(targetResults.ullFileSize);
            _Print(")\n");

            ullTotalBytesCount += ullBytesCount;
            ullTotalIOCount += ullIOCount;
        }
    }

    _PrintSectionBorderLine(timeSpan);

    _Print("total:   %15llu | %12llu | %10.2f | %10.2f",
           ullTotalBytesCount,
           ullTotalIOCount,
           (double)ullTotalBytesCount / 1024 / 1024 / fTime,
           (double)ullTotalIOCount / fTime);

    if (timeSpan.GetMeasureLatency())
    {
        _Print(" | %8.3f", totalLatencyHistogram.GetAvg()/1000);
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
            _Print("\nLatency distribution: %s\n", path.c_str());
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

    _Print("\nTotal latency distribution:\n");
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
}

string ResultParser::ParseProfile(const Profile& profile)
{
    _sResult.clear();
    _PrintProfile(profile);
    return _sResult;
}

void ResultParser::_PrintWaitStats(const Results &results, const TimeSpan& timeSpan)
{
    // Output format hardcodes 8 bucket values per line; assert if changed.
    static_assert(c_nCompletionBuckets == 8, "update _PrintWaitStats format strings for new bucket count");

    bool fLookasideActive = timeSpan.GetMeasureLatency() || timeSpan.GetCalculateIopsStdDev();

    // Determine if any thread had throttled targets
    bool fAnyThrottled = false;
    for (const auto& threadResults : results.vThreadResults)
    {
        if (threadResults.WaitStats.fThrottled)
        {
            fAnyThrottled = true;
            break;
        }
    }

    // Table 1: Regular waits with WaitCompletion counts
    if (fAnyThrottled)
    {
        _Print("Wait Statistics - Completion Wait\n");
        _Print("thread |         wait | throttle wait  -  sleep | 0 - 7+ complete per wait\n");
        _Print("--------------------------------------------------------------------------\n");
    }
    else
    {
        _Print("Wait Statistics - Completion Wait\n");
        _Print("thread |         wait | 0 - 7+ complete per wait\n");
        _Print("------------------------------------------------\n");
    }

    for (unsigned int iThread = 0; iThread < results.vThreadResults.size(); ++iThread)
    {
        const WAIT_STATS& ws = results.vThreadResults[iThread].WaitStats;
        if (ws.fThrottled)
        {
            _Print(
                "%6u | %12llu | %13llu  - %6llu | %llu %llu %llu %llu %llu %llu %llu %llu\n",
                iThread, ws.Wait,
                ws.ThrottleWait, ws.ThrottleSleep,
                ws.WaitCompletion[0], ws.WaitCompletion[1], ws.WaitCompletion[2], ws.WaitCompletion[3],
                ws.WaitCompletion[4], ws.WaitCompletion[5], ws.WaitCompletion[6], ws.WaitCompletion[7]);
        }
        else
        {
            _Print(
                fAnyThrottled
                    ? "%6u | %12llu |               ---       | %llu %llu %llu %llu %llu %llu %llu %llu\n"
                    : "%6u | %12llu | %llu %llu %llu %llu %llu %llu %llu %llu\n",
                iThread, ws.Wait,
                ws.WaitCompletion[0], ws.WaitCompletion[1], ws.WaitCompletion[2], ws.WaitCompletion[3],
                ws.WaitCompletion[4], ws.WaitCompletion[5], ws.WaitCompletion[6], ws.WaitCompletion[7]);
        }
    }

    // Table 2: Lookaside waits (only when latency measurement active)
    if (fLookasideActive)
    {
        _Print("\nWait Statistics - Lookaside\n");
        _Print("thread |    lookaside | 0 - 7+ complete per lookaside\n");
        _Print("-----------------------------------------------------\n");
        for (unsigned int iThread = 0; iThread < results.vThreadResults.size(); ++iThread)
        {
            const WAIT_STATS& ws = results.vThreadResults[iThread].WaitStats;
            _Print(
                "%6u | %12llu | %llu %llu %llu %llu %llu %llu %llu %llu\n",
                iThread, ws.Lookaside,
                ws.LookasideCompletion[0], ws.LookasideCompletion[1], ws.LookasideCompletion[2], ws.LookasideCompletion[3],
                ws.LookasideCompletion[4], ws.LookasideCompletion[5], ws.LookasideCompletion[6], ws.LookasideCompletion[7]);
        }
    }
}

void ResultParser::_PrintIoRingStatsFieldNames()
{
    _Print("thread |  Submits/s \n");
}

void ResultParser::_PrintIoRingStatsBorderLine()
{
    _Print("--------------------\n");
}

//
// Print IoRing statistics table (submits per second per thread)
//
void ResultParser::_PrintIoRingStats(const TimeSpan& timeSpan, const Results& results)
{
    double fTime = PerfTimer::PerfTimeToSeconds(results.ullTimeCount);
    UINT64 ullTotalSubmitCount = 0;

    // Skip if IoRing is not enabled
    if (!timeSpan.GetUseIoRing())
    {
        return;
    }

    _Print("\n\nIoRing Statistics\n");
    _PrintIoRingStatsFieldNames();
    _PrintIoRingStatsBorderLine();

    for (unsigned int iThread = 0; iThread < results.vThreadResults.size(); ++iThread)
    {
        const ThreadResults& threadResults = results.vThreadResults[iThread];

        _Print("%6u", iThread);

        _Print(" | %11.2f", (double)threadResults.ullSubmitCount / fTime);

        ullTotalSubmitCount += threadResults.ullSubmitCount;

        _Print("\n");
    }
    _PrintIoRingStatsBorderLine();
    _Print("total:");

    _Print(" | %11.2f", (double)ullTotalSubmitCount / fTime);

    _Print("\n");
}

string ResultParser::ParseResults(const Profile& profile, const SystemInformation& system, vector<Results> vResults)
{
    _sResult.clear();

    _PrintProfile(profile);
    _PrintSystemInfo(system);

    for (size_t iResult = 0; iResult < vResults.size(); iResult++)
    {
        _Print("\nResults for timespan %d:\n", iResult + 1);
        _Print("*******************************************************************************\n");

        const Results& results = vResults[iResult];
        const TimeSpan& timeSpan = profile.GetTimeSpans()[iResult];

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

            _PrintEffectiveAffinity(timeSpan, system);
            _PrintHeterogeneousAffinityWarning(timeSpan, system);
            _PrintCpuUtilization(results, system);
            _PrintEffectiveDistributions(results);

            _Print("\nTotal IO\n");
            _PrintSection(_SectionEnum::TOTAL, timeSpan, results);

            _Print("\nRead IO\n");
            _PrintSection(_SectionEnum::READ, timeSpan, results);

            _Print("\nWrite IO\n");
            _PrintSection(_SectionEnum::WRITE, timeSpan, results);

            _PrintIoRingStats(timeSpan, results);

            if (timeSpan.GetMeasureLatency())
            {
                _PrintLatencyPercentiles(results);
            }

            //etw
            if (results.fUseETW)
            {
                _DisplayETW(results.EtwMask, results.EtwEventCounters);
                _DisplayETWSessionInfo(results.EtwSessionInfo);
            }

            // wait stats
            if (profile.GetVerboseStats())
            {
                _Print("\n");
                _PrintWaitStats(results, timeSpan);
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
