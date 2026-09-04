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

#include "StdAfx.h"
#include "XmlResultParser.UnitTests.h"
#include "Common.h"
#include "xmlresultparser.h"
#include "TextDiff.h"
#include <stdlib.h>
#include <vector>

using namespace WEX::TestExecution;
using namespace WEX::Logging;
using namespace std;

namespace UnitTests
{
    void XmlResultParserUnitTests::Test_ParseResults()
    {
        Profile profile;
        TimeSpan timeSpan;
        Target target;
        XmlResultParser parser;

        Results results;
        results.fUseETW = false;
        double fTime = 120.0;
        results.ullTimeCount = PerfTimer::SecondsToPerfTime(fTime);

        // First group has 1 active cpu
        // 30% user, 45% idle, 25% non-idle kernel (45% + 25% = 70%)
        SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION systemProcessorInfo = { 0 };
        systemProcessorInfo.UserTime.QuadPart = static_cast<LONGLONG>(fTime * 30 * 100000);
        systemProcessorInfo.IdleTime.QuadPart = static_cast<LONGLONG>(fTime * 45 * 100000);
        systemProcessorInfo.KernelTime.QuadPart = static_cast<LONGLONG>(fTime * 70 * 100000);
        results.vSystemProcessorPerfInfo.push_back(systemProcessorInfo);

        // Second group has 2 active
        // 100% idle
        systemProcessorInfo.UserTime.QuadPart = static_cast<LONGLONG>(fTime * 0 * 100000);
        systemProcessorInfo.IdleTime.QuadPart = static_cast<LONGLONG>(fTime * 100 * 100000);
        systemProcessorInfo.KernelTime.QuadPart = static_cast<LONGLONG>(fTime * 100 * 100000);
        results.vSystemProcessorPerfInfo.push_back(systemProcessorInfo);
        results.vSystemProcessorPerfInfo.push_back(systemProcessorInfo);

        // TODO: multiple target cases, full profile/result variations
        target.SetPath("testfile1.dat");
        target.SetCacheMode(TargetCacheMode::DisableOSCache);
        target.SetWriteThroughMode(WriteThroughMode::On);
        target.SetThroughputIOPS(1000);
        target.SetBypassIoMode(BypassIoMode::Full);

        timeSpan.AddTarget(target);
        timeSpan.SetCalculateIopsStdDev(true);
        timeSpan.SetUseIoRing(true);
        timeSpan.SetIoRingBatchSize(75);
        timeSpan.SetIoRingBatchSizeIsPercent(false);
        timeSpan.SetUseRegBuffer(true);

        TargetResults targetResults;
        targetResults.sPath = "testfile1.dat";
        targetResults.ullFileSize = 10 * 1024 * 1024;
        targetResults.ullReadBytesCount = 4 * 1024 * 1024;
        targetResults.ullReadIOCount = 6;
        targetResults.ullWriteBytesCount = 2 * 1024 * 1024;
        targetResults.ullWriteIOCount = 10;
        targetResults.ullBytesCount = targetResults.ullReadBytesCount + targetResults.ullWriteBytesCount;
        targetResults.ullIOCount = targetResults.ullReadIOCount + targetResults.ullWriteIOCount;

        // TODO: Histogram<float> readLatencyHistogram;
        // TODO: Histogram<float> writeLatencyHistogram;
        // TODO: IoBucketizer writeBucketizer;

        targetResults.readBucketizer.Initialize(1000, timeSpan.GetDuration());
        for (size_t i = 0; i < timeSpan.GetDuration(); i++)
        {
            // add an io halfway through the bucket's time interval
            targetResults.readBucketizer.Add(i*1000 + 500, 0);
        }

        ThreadResults threadResults;
        threadResults.vTargetResults.push_back(targetResults);
        threadResults.ullSubmitCount = 10;
        results.vThreadResults.push_back(threadResults);

        vector<Results> vResults;
        vResults.push_back(results);

        // Just throw away the computername, pp and reset the timestamp - for the ut, it's
        // as useful (and simpler) to verify statics as anything else. Reconstruct the
        // processor topo to a fixed example as well. Note that the performance
        // efficiency class must be placed since it is calculated on the fly during
        // the actual GLPIEx enumeration. If we could shim GLPIEx ...
        SystemInformation system;
        system.ResetTime();
        system.sComputerName.clear();
        system.sProcessorName.clear();
        system.sActivePolicyName.clear();
        system.sActivePolicyGuid.clear();

        system.dwPageSize = 4096;

        system.processorTopology._ulProcessorCount = 3;
        system.processorTopology._ubPerformanceEfficiencyClass = 1;
        system.processorTopology._fSMT = true;

        system.processorTopology._vProcessorGroupInformation.clear();
        system.processorTopology._vProcessorGroupInformation.emplace_back((WORD)0, (BYTE)1, (BYTE)1, (KAFFINITY)0x1);
        system.processorTopology._vProcessorGroupInformation.emplace_back((WORD)1, (BYTE)4, (BYTE)2, (KAFFINITY)0x3);

        ProcessorNumaInformation node;
        node._nodeNumber = 0;
        node._vProcessorMasks.emplace_back((WORD)0, (KAFFINITY)0x1);
        node._vProcessorMasks.emplace_back((WORD)1, (KAFFINITY)0x3);
        system.processorTopology._vProcessorNumaInformation.clear();
        system.processorTopology._vProcessorNumaInformation.push_back(node);

        ProcessorSocketInformation socket;
        socket._ulSocketNumber = 0;
        socket._ulProcCount = 0;
        socket._vProcessorMasks.emplace_back((WORD)0, (KAFFINITY)0x1);
        socket._vProcessorMasks.emplace_back((WORD)1, (KAFFINITY)0x3);
        system.processorTopology._vProcessorSocketInformation.clear();
        system.processorTopology._vProcessorSocketInformation.push_back(socket);

        system.processorTopology._vProcessorCoreInformation.clear();
        system.processorTopology._vProcessorCoreInformation.emplace_back((WORD)0, (KAFFINITY)0x1, (BYTE)0);
        system.processorTopology._vProcessorCoreInformation.emplace_back((WORD)1, (KAFFINITY)0x3, (BYTE)1);

        system.processorTopology._vProcessorCacheInformation.clear();
        {
            ProcessorCacheInformation l3(3, 16, 64, 8 * 1024 * 1024, CacheUnified);
            l3._processorMasks.emplace_back((WORD)0, (KAFFINITY)0x1);
            system.processorTopology._vProcessorCacheInformation.push_back(l3);
        }

        // Point timespan at the mock system for deterministic output
        timeSpan.SetSystem(&system);

        // Finalize effective buffer separation before adding to profile
        timeSpan.Finalize();

        // finally, add the timespan to the profile and dump.
        profile.AddTimeSpan(timeSpan);

        string sResults = parser.ParseResults(profile, system, vResults);

        // stringify random text, quoting "'s and adding newline/preserving tabs
        // gc some.txt |% { write-host $("`"{0}\n`"" -f $($_ -replace "`"","\`"" -replace "`t","\t")) }

        const char *pcszExpected = \
            "<Results>\n"
            "  <System>\n"
            "    <ComputerName></ComputerName>\n"
            "    <ProcessorName></ProcessorName>\n"
            "    <Tool>\n"
            "      <Version>" DISKSPD_NUMERIC_VERSION_STRING "</Version>\n"
            "      <VersionDate>" DISKSPD_DATE_VERSION_STRING "</VersionDate>\n"
            "    </Tool>\n"
            "    <RunTime></RunTime>\n"
            "    <PowerScheme Name=\"\" Guid=\"\"/>\n"
            "    <PageSize>4096</PageSize>\n"
            "    <ProcessorTopology Heterogeneous=\"true\">\n"
            "      <Group Group=\"0\" MaximumProcessors=\"1\" ActiveProcessors=\"1\" ActiveProcessorMask=\"0x1\"/>\n"
            "      <Group Group=\"1\" MaximumProcessors=\"4\" ActiveProcessors=\"2\" ActiveProcessorMask=\"0x3\"/>\n"
            "      <Node Node=\"0\">\n"
            "        <Group Group=\"0\" Mask=\"0x1\"/>\n"
            "        <Group Group=\"1\" Mask=\"0x3\"/>\n"
            "      </Node>\n"
            "      <Socket Socket=\"0\">\n"
            "        <Group Group=\"0\" Mask=\"0x1\"/>\n"
            "        <Group Group=\"1\" Mask=\"0x3\"/>\n"
            "      </Socket>\n"
            "      <Core Group=\"0\" Core=\"0\" Mask=\"0x1\" EfficiencyClass=\"0\"/>\n"
            "      <Core Group=\"1\" Core=\"0\" Mask=\"0x3\" EfficiencyClass=\"1\"/>\n"
            "      <Cache Level=\"3\" Associativity=\"16\" LineSize=\"64\" CacheSize=\"8388608\" Type=\"Unified\">\n"
            "        <Group Group=\"0\" Mask=\"0x1\"/>\n"
            "      </Cache>\n"
            "    </ProcessorTopology>\n"
            "  </System>\n"
            "  <Profile>\n"
            "    <Progress>0</Progress>\n"
            "    <ResultFormat>text</ResultFormat>\n"
            "    <Verbose>false</Verbose>\n"
            "    <TimeSpans>\n"
            "      <TimeSpan>\n"
            "        <CompletionRoutines>false</CompletionRoutines>\n"
            "        <MeasureLatency>false</MeasureLatency>\n"
            "        <CalculateIopsStdDev>true</CalculateIopsStdDev>\n"
            "        <Duration>10</Duration>\n"
            "        <Warmup>5</Warmup>\n"
            "        <Cooldown>0</Cooldown>\n"
            "        <ThreadCount>0</ThreadCount>\n"
            "        <RequestCount>0</RequestCount>\n"
            "        <IoBucketDuration>1000</IoBucketDuration>\n"
            "        <RandSeed>0</RandSeed>\n"
            "        <IoRing>\n"
            "          <IoRingBatchSize Percent=\"false\">75</IoRingBatchSize>\n"
            "          <UseRegBuffer>true</UseRegBuffer>\n"
            "        </IoRing>\n"
            "        <DisableAffinity>false</DisableAffinity>\n"
            "        <AffinityTraversal Group=\"Fill\" Efficiency=\"PFirst\">Cpu</AffinityTraversal>\n"
            "        <BufferSeparation>PDECacheLine</BufferSeparation>\n"
            "        <Targets>\n"
            "          <Target>\n"
            "            <Path>testfile1.dat</Path>\n"
            "            <BlockSize>65536</BlockSize>\n"
            "            <BaseFileOffset>0</BaseFileOffset>\n"
            "            <SequentialScan>false</SequentialScan>\n"
            "            <RandomAccess>false</RandomAccess>\n"
            "            <TemporaryFile>false</TemporaryFile>\n"
            "            <UseLargePages>false</UseLargePages>\n"
            "            <DisableOSCache>true</DisableOSCache>\n"
            "            <WriteThrough>true</WriteThrough>\n"
            "            <BypassIO>Full</BypassIO>\n"
            "            <WriteBufferContent>\n"
            "              <Pattern>sequential</Pattern>\n"
            "            </WriteBufferContent>\n"
            "            <ParallelAsyncIO>false</ParallelAsyncIO>\n"
            "            <StrideSize>65536</StrideSize>\n"
            "            <InterlockedSequential>false</InterlockedSequential>\n"
            "            <ThreadStride>0</ThreadStride>\n"
            "            <MaxFileSize>0</MaxFileSize>\n"
            "            <RequestCount>2</RequestCount>\n"
            "            <WriteRatio>0</WriteRatio>\n"
            "            <Throughput unit=\"IOPS\">1000</Throughput>\n"
            "            <ThreadsPerFile>1</ThreadsPerFile>\n"
            "            <IOPriority>3</IOPriority>\n"
            "            <Weight>1</Weight>\n"
            "          </Target>\n"
            "        </Targets>\n"
            "      </TimeSpan>\n"
            "    </TimeSpans>\n"
            "  </Profile>\n"
            "  <TimeSpan>\n"
            "    <TestTimeSeconds>120.00</TestTimeSeconds>\n"
            "    <ThreadCount>1</ThreadCount>\n"
            "    <RequestCount>0</RequestCount>\n"
            "    <ProcCount>3</ProcCount>\n"
            "    <EffectiveBufferSeparation>16777216</EffectiveBufferSeparation>\n"
            "    <EffectiveAffinity>\n"
            "      <Group Group=\"0\" Mask=\"0x1\"/>\n"
            "      <Group Group=\"1\" Mask=\"0x3\"/>\n"
            "    </EffectiveAffinity>\n"
            "    <HeterogeneousAffinityWarning>true</HeterogeneousAffinityWarning>\n"
            "    <CpuUtilization>\n"
            "      <CPU>\n"
            "        <Socket>0</Socket>\n"
            "        <Node>0</Node>\n"
            "        <Group>0</Group>\n"
            "        <Core>0</Core>\n"
            "        <EfficiencyClass>0</EfficiencyClass>\n"
            "        <Id>0</Id>\n"
            "        <UsagePercent>55.00</UsagePercent>\n"
            "        <UserPercent>30.00</UserPercent>\n"
            "        <KernelPercent>25.00</KernelPercent>\n"
            "        <IdlePercent>45.00</IdlePercent>\n"
            "      </CPU>\n"
            "      <CPU>\n"
            "        <Socket>0</Socket>\n"
            "        <Node>0</Node>\n"
            "        <Group>1</Group>\n"
            "        <Core>0</Core>\n"
            "        <EfficiencyClass>1</EfficiencyClass>\n"
            "        <Id>0</Id>\n"
            "        <UsagePercent>0.00</UsagePercent>\n"
            "        <UserPercent>0.00</UserPercent>\n"
            "        <KernelPercent>0.00</KernelPercent>\n"
            "        <IdlePercent>100.00</IdlePercent>\n"
            "      </CPU>\n"
            "      <CPU>\n"
            "        <Socket>0</Socket>\n"
            "        <Node>0</Node>\n"
            "        <Group>1</Group>\n"
            "        <Core>0</Core>\n"
            "        <EfficiencyClass>1</EfficiencyClass>\n"
            "        <Id>1</Id>\n"
            "        <UsagePercent>0.00</UsagePercent>\n"
            "        <UserPercent>0.00</UserPercent>\n"
            "        <KernelPercent>0.00</KernelPercent>\n"
            "        <IdlePercent>100.00</IdlePercent>\n"
            "      </CPU>\n"
            "      <Average>\n"
            "        <UsagePercent>18.33</UsagePercent>\n"
            "        <UserPercent>10.00</UserPercent>\n"
            "        <KernelPercent>8.33</KernelPercent>\n"
            "        <IdlePercent>81.67</IdlePercent>\n"
            "      </Average>\n"
            "    </CpuUtilization>\n"
            "    <Iops>\n"
            "      <ReadIopsStdDev>0.000</ReadIopsStdDev>\n"
            "      <IopsStdDev>0.000</IopsStdDev>\n"
            "      <Bucket SampleMillisecond=\"1000\" Read=\"1\" Write=\"0\" Total=\"1\" ReadMinLatencyMilliseconds=\"0.000\" ReadMaxLatencyMilliseconds=\"0.000\" ReadAvgLatencyMilliseconds=\"0.000\" ReadLatencyStdDev=\"0.000\" WriteMinLatencyMilliseconds=\"0.000\" WriteMaxLatencyMilliseconds=\"0.000\" WriteAvgLatencyMilliseconds=\"0.000\" WriteLatencyStdDev=\"0.000\"/>\n"
            "      <Bucket SampleMillisecond=\"2000\" Read=\"1\" Write=\"0\" Total=\"1\" ReadMinLatencyMilliseconds=\"0.000\" ReadMaxLatencyMilliseconds=\"0.000\" ReadAvgLatencyMilliseconds=\"0.000\" ReadLatencyStdDev=\"0.000\" WriteMinLatencyMilliseconds=\"0.000\" WriteMaxLatencyMilliseconds=\"0.000\" WriteAvgLatencyMilliseconds=\"0.000\" WriteLatencyStdDev=\"0.000\"/>\n"
            "      <Bucket SampleMillisecond=\"3000\" Read=\"1\" Write=\"0\" Total=\"1\" ReadMinLatencyMilliseconds=\"0.000\" ReadMaxLatencyMilliseconds=\"0.000\" ReadAvgLatencyMilliseconds=\"0.000\" ReadLatencyStdDev=\"0.000\" WriteMinLatencyMilliseconds=\"0.000\" WriteMaxLatencyMilliseconds=\"0.000\" WriteAvgLatencyMilliseconds=\"0.000\" WriteLatencyStdDev=\"0.000\"/>\n"
            "      <Bucket SampleMillisecond=\"4000\" Read=\"1\" Write=\"0\" Total=\"1\" ReadMinLatencyMilliseconds=\"0.000\" ReadMaxLatencyMilliseconds=\"0.000\" ReadAvgLatencyMilliseconds=\"0.000\" ReadLatencyStdDev=\"0.000\" WriteMinLatencyMilliseconds=\"0.000\" WriteMaxLatencyMilliseconds=\"0.000\" WriteAvgLatencyMilliseconds=\"0.000\" WriteLatencyStdDev=\"0.000\"/>\n"
            "      <Bucket SampleMillisecond=\"5000\" Read=\"1\" Write=\"0\" Total=\"1\" ReadMinLatencyMilliseconds=\"0.000\" ReadMaxLatencyMilliseconds=\"0.000\" ReadAvgLatencyMilliseconds=\"0.000\" ReadLatencyStdDev=\"0.000\" WriteMinLatencyMilliseconds=\"0.000\" WriteMaxLatencyMilliseconds=\"0.000\" WriteAvgLatencyMilliseconds=\"0.000\" WriteLatencyStdDev=\"0.000\"/>\n"
            "      <Bucket SampleMillisecond=\"6000\" Read=\"1\" Write=\"0\" Total=\"1\" ReadMinLatencyMilliseconds=\"0.000\" ReadMaxLatencyMilliseconds=\"0.000\" ReadAvgLatencyMilliseconds=\"0.000\" ReadLatencyStdDev=\"0.000\" WriteMinLatencyMilliseconds=\"0.000\" WriteMaxLatencyMilliseconds=\"0.000\" WriteAvgLatencyMilliseconds=\"0.000\" WriteLatencyStdDev=\"0.000\"/>\n"
            "      <Bucket SampleMillisecond=\"7000\" Read=\"1\" Write=\"0\" Total=\"1\" ReadMinLatencyMilliseconds=\"0.000\" ReadMaxLatencyMilliseconds=\"0.000\" ReadAvgLatencyMilliseconds=\"0.000\" ReadLatencyStdDev=\"0.000\" WriteMinLatencyMilliseconds=\"0.000\" WriteMaxLatencyMilliseconds=\"0.000\" WriteAvgLatencyMilliseconds=\"0.000\" WriteLatencyStdDev=\"0.000\"/>\n"
            "      <Bucket SampleMillisecond=\"8000\" Read=\"1\" Write=\"0\" Total=\"1\" ReadMinLatencyMilliseconds=\"0.000\" ReadMaxLatencyMilliseconds=\"0.000\" ReadAvgLatencyMilliseconds=\"0.000\" ReadLatencyStdDev=\"0.000\" WriteMinLatencyMilliseconds=\"0.000\" WriteMaxLatencyMilliseconds=\"0.000\" WriteAvgLatencyMilliseconds=\"0.000\" WriteLatencyStdDev=\"0.000\"/>\n"
            "      <Bucket SampleMillisecond=\"9000\" Read=\"1\" Write=\"0\" Total=\"1\" ReadMinLatencyMilliseconds=\"0.000\" ReadMaxLatencyMilliseconds=\"0.000\" ReadAvgLatencyMilliseconds=\"0.000\" ReadLatencyStdDev=\"0.000\" WriteMinLatencyMilliseconds=\"0.000\" WriteMaxLatencyMilliseconds=\"0.000\" WriteAvgLatencyMilliseconds=\"0.000\" WriteLatencyStdDev=\"0.000\"/>\n"
            "      <Bucket SampleMillisecond=\"10000\" Read=\"1\" Write=\"0\" Total=\"1\" ReadMinLatencyMilliseconds=\"0.000\" ReadMaxLatencyMilliseconds=\"0.000\" ReadAvgLatencyMilliseconds=\"0.000\" ReadLatencyStdDev=\"0.000\" WriteMinLatencyMilliseconds=\"0.000\" WriteMaxLatencyMilliseconds=\"0.000\" WriteAvgLatencyMilliseconds=\"0.000\" WriteLatencyStdDev=\"0.000\"/>\n"
            "    </Iops>\n"
            "    <Thread>\n"
            "      <Id>0</Id>\n"
            "      <SubmitCount>10</SubmitCount>\n"
            "      <Target>\n"
            "        <Path>testfile1.dat</Path>\n"
            "        <BytesCount>6291456</BytesCount>\n"
            "        <FileSize>10485760</FileSize>\n"
            "        <IOCount>16</IOCount>\n"
            "        <ReadBytes>4194304</ReadBytes>\n"
            "        <ReadCount>6</ReadCount>\n"
            "        <WriteBytes>2097152</WriteBytes>\n"
            "        <WriteCount>10</WriteCount>\n"
            "        <Iops>\n"
            "          <ReadIopsStdDev>0.000</ReadIopsStdDev>\n"
            "          <IopsStdDev>0.000</IopsStdDev>\n"
            "          <Bucket SampleMillisecond=\"1000\" Read=\"1\" Write=\"0\" Total=\"1\" ReadMinLatencyMilliseconds=\"0.000\" ReadMaxLatencyMilliseconds=\"0.000\" ReadAvgLatencyMilliseconds=\"0.000\" ReadLatencyStdDev=\"0.000\" WriteMinLatencyMilliseconds=\"0.000\" WriteMaxLatencyMilliseconds=\"0.000\" WriteAvgLatencyMilliseconds=\"0.000\" WriteLatencyStdDev=\"0.000\"/>\n"
            "          <Bucket SampleMillisecond=\"2000\" Read=\"1\" Write=\"0\" Total=\"1\" ReadMinLatencyMilliseconds=\"0.000\" ReadMaxLatencyMilliseconds=\"0.000\" ReadAvgLatencyMilliseconds=\"0.000\" ReadLatencyStdDev=\"0.000\" WriteMinLatencyMilliseconds=\"0.000\" WriteMaxLatencyMilliseconds=\"0.000\" WriteAvgLatencyMilliseconds=\"0.000\" WriteLatencyStdDev=\"0.000\"/>\n"
            "          <Bucket SampleMillisecond=\"3000\" Read=\"1\" Write=\"0\" Total=\"1\" ReadMinLatencyMilliseconds=\"0.000\" ReadMaxLatencyMilliseconds=\"0.000\" ReadAvgLatencyMilliseconds=\"0.000\" ReadLatencyStdDev=\"0.000\" WriteMinLatencyMilliseconds=\"0.000\" WriteMaxLatencyMilliseconds=\"0.000\" WriteAvgLatencyMilliseconds=\"0.000\" WriteLatencyStdDev=\"0.000\"/>\n"
            "          <Bucket SampleMillisecond=\"4000\" Read=\"1\" Write=\"0\" Total=\"1\" ReadMinLatencyMilliseconds=\"0.000\" ReadMaxLatencyMilliseconds=\"0.000\" ReadAvgLatencyMilliseconds=\"0.000\" ReadLatencyStdDev=\"0.000\" WriteMinLatencyMilliseconds=\"0.000\" WriteMaxLatencyMilliseconds=\"0.000\" WriteAvgLatencyMilliseconds=\"0.000\" WriteLatencyStdDev=\"0.000\"/>\n"
            "          <Bucket SampleMillisecond=\"5000\" Read=\"1\" Write=\"0\" Total=\"1\" ReadMinLatencyMilliseconds=\"0.000\" ReadMaxLatencyMilliseconds=\"0.000\" ReadAvgLatencyMilliseconds=\"0.000\" ReadLatencyStdDev=\"0.000\" WriteMinLatencyMilliseconds=\"0.000\" WriteMaxLatencyMilliseconds=\"0.000\" WriteAvgLatencyMilliseconds=\"0.000\" WriteLatencyStdDev=\"0.000\"/>\n"
            "          <Bucket SampleMillisecond=\"6000\" Read=\"1\" Write=\"0\" Total=\"1\" ReadMinLatencyMilliseconds=\"0.000\" ReadMaxLatencyMilliseconds=\"0.000\" ReadAvgLatencyMilliseconds=\"0.000\" ReadLatencyStdDev=\"0.000\" WriteMinLatencyMilliseconds=\"0.000\" WriteMaxLatencyMilliseconds=\"0.000\" WriteAvgLatencyMilliseconds=\"0.000\" WriteLatencyStdDev=\"0.000\"/>\n"
            "          <Bucket SampleMillisecond=\"7000\" Read=\"1\" Write=\"0\" Total=\"1\" ReadMinLatencyMilliseconds=\"0.000\" ReadMaxLatencyMilliseconds=\"0.000\" ReadAvgLatencyMilliseconds=\"0.000\" ReadLatencyStdDev=\"0.000\" WriteMinLatencyMilliseconds=\"0.000\" WriteMaxLatencyMilliseconds=\"0.000\" WriteAvgLatencyMilliseconds=\"0.000\" WriteLatencyStdDev=\"0.000\"/>\n"
            "          <Bucket SampleMillisecond=\"8000\" Read=\"1\" Write=\"0\" Total=\"1\" ReadMinLatencyMilliseconds=\"0.000\" ReadMaxLatencyMilliseconds=\"0.000\" ReadAvgLatencyMilliseconds=\"0.000\" ReadLatencyStdDev=\"0.000\" WriteMinLatencyMilliseconds=\"0.000\" WriteMaxLatencyMilliseconds=\"0.000\" WriteAvgLatencyMilliseconds=\"0.000\" WriteLatencyStdDev=\"0.000\"/>\n"
            "          <Bucket SampleMillisecond=\"9000\" Read=\"1\" Write=\"0\" Total=\"1\" ReadMinLatencyMilliseconds=\"0.000\" ReadMaxLatencyMilliseconds=\"0.000\" ReadAvgLatencyMilliseconds=\"0.000\" ReadLatencyStdDev=\"0.000\" WriteMinLatencyMilliseconds=\"0.000\" WriteMaxLatencyMilliseconds=\"0.000\" WriteAvgLatencyMilliseconds=\"0.000\" WriteLatencyStdDev=\"0.000\"/>\n"
            "          <Bucket SampleMillisecond=\"10000\" Read=\"1\" Write=\"0\" Total=\"1\" ReadMinLatencyMilliseconds=\"0.000\" ReadMaxLatencyMilliseconds=\"0.000\" ReadAvgLatencyMilliseconds=\"0.000\" ReadLatencyStdDev=\"0.000\" WriteMinLatencyMilliseconds=\"0.000\" WriteMaxLatencyMilliseconds=\"0.000\" WriteAvgLatencyMilliseconds=\"0.000\" WriteLatencyStdDev=\"0.000\"/>\n"
            "        </Iops>\n"
            "      </Target>\n"
            "    </Thread>\n"
            "  </TimeSpan>\n"
            "</Results>";

        // Compare line by line, reporting the first difference for easier diagnosis
        VERIFY_MULTILINE_EQUAL(pcszExpected, sResults);
    }

    void XmlResultParserUnitTests::Test_ParseProfile()
    {
        Profile profile;
        XmlResultParser parser;
        TimeSpan timeSpan;
        Target target;

        // Profile XML contains only configuration, not finalized data.
        // No system mock or finalization needed.
        timeSpan.AddTarget(target);
        profile.AddTimeSpan(timeSpan);

        string s = parser.ParseProfile(profile);
        const char *pcszExpected = "<Profile>\n"
            "  <Progress>0</Progress>\n"
            "  <ResultFormat>text</ResultFormat>\n"
            "  <Verbose>false</Verbose>\n"
            "  <TimeSpans>\n"
            "    <TimeSpan>\n"
            "      <CompletionRoutines>false</CompletionRoutines>\n"
            "      <MeasureLatency>false</MeasureLatency>\n"
            "      <CalculateIopsStdDev>false</CalculateIopsStdDev>\n"
            "      <Duration>10</Duration>\n"
            "      <Warmup>5</Warmup>\n"
            "      <Cooldown>0</Cooldown>\n"
            "      <ThreadCount>0</ThreadCount>\n"
            "      <RequestCount>0</RequestCount>\n"
            "      <IoBucketDuration>1000</IoBucketDuration>\n"
            "      <RandSeed>0</RandSeed>\n"
            "      <DisableAffinity>false</DisableAffinity>\n"
            "      <AffinityTraversal Group=\"Fill\" Efficiency=\"PFirst\">Cpu</AffinityTraversal>\n"
            "      <BufferSeparation>PDECacheLine</BufferSeparation>\n"
            "      <Targets>\n"
            "        <Target>\n"
            "          <Path></Path>\n"
            "          <BlockSize>65536</BlockSize>\n"
            "          <BaseFileOffset>0</BaseFileOffset>\n"
            "          <SequentialScan>false</SequentialScan>\n"
            "          <RandomAccess>false</RandomAccess>\n"
            "          <TemporaryFile>false</TemporaryFile>\n"
            "          <UseLargePages>false</UseLargePages>\n"
            "          <WriteBufferContent>\n"
            "            <Pattern>sequential</Pattern>\n"
            "          </WriteBufferContent>\n"
            "          <ParallelAsyncIO>false</ParallelAsyncIO>\n"
            "          <StrideSize>65536</StrideSize>\n"
            "          <InterlockedSequential>false</InterlockedSequential>\n"
            "          <ThreadStride>0</ThreadStride>\n"
            "          <MaxFileSize>0</MaxFileSize>\n"
            "          <RequestCount>2</RequestCount>\n"
            "          <WriteRatio>0</WriteRatio>\n"
            "          <Throughput>0</Throughput>\n"
            "          <ThreadsPerFile>1</ThreadsPerFile>\n"
            "          <IOPriority>3</IOPriority>\n"
            "          <Weight>1</Weight>\n"
            "        </Target>\n"
            "      </Targets>\n"
            "    </TimeSpan>\n"
            "  </TimeSpans>\n"
            "</Profile>\n";

        VERIFY_MULTILINE_EQUAL(pcszExpected, s);
    }

    void XmlResultParserUnitTests::Test_ParseTargetProfile()
    {
        Target target;
        string sResults;
        char szExpected[4096];
        int nWritten;

        const char *pcszOutputTemplate = \
            "<Target>\n"
            "  <Path>testfile1.dat</Path>\n"
            "  <BlockSize>65536</BlockSize>\n"
            "  <BaseFileOffset>0</BaseFileOffset>\n"
            "  <SequentialScan>false</SequentialScan>\n"
            "  <RandomAccess>false</RandomAccess>\n"
            "  <TemporaryFile>false</TemporaryFile>\n"
            "  <UseLargePages>false</UseLargePages>\n"
            "  <DisableOSCache>true</DisableOSCache>\n"
            "  <WriteThrough>true</WriteThrough>\n"
            "  <WriteBufferContent>\n"
            "    <Pattern>sequential</Pattern>\n"
            "  </WriteBufferContent>\n"
            "  <ParallelAsyncIO>false</ParallelAsyncIO>\n"
            "  <StrideSize>65536</StrideSize>\n"
            "  <InterlockedSequential>false</InterlockedSequential>\n"
            "  <ThreadStride>0</ThreadStride>\n"
            "  <MaxFileSize>0</MaxFileSize>\n"
            "  <RequestCount>2</RequestCount>\n"
            "  <WriteRatio>0</WriteRatio>\n"
            "  <Throughput%s>%s</Throughput>\n"       // 2 param
            "  <ThreadsPerFile>1</ThreadsPerFile>\n"
            "  <IOPriority>3</IOPriority>\n"
            "  <Weight>1</Weight>\n"
            "</Target>\n";

        target.SetPath("testfile1.dat");
        target.SetCacheMode(TargetCacheMode::DisableOSCache);
        target.SetWriteThroughMode(WriteThroughMode::On);

        // Base case - no limit

        nWritten = sprintf_s(szExpected, sizeof(szExpected),
                             pcszOutputTemplate, "", "0");
        VERIFY_IS_GREATER_THAN(nWritten, 0);
        sResults = target.GetXml(0);
        VERIFY_MULTILINE_EQUAL(szExpected, sResults);

        // IOPS - with units

        target.SetThroughputIOPS(1000);
        nWritten = sprintf_s(szExpected, sizeof(szExpected),
                             pcszOutputTemplate, " unit=\"IOPS\"", "1000");
        VERIFY_IS_GREATER_THAN(nWritten, 0);
        sResults = target.GetXml(0);
        VERIFY_MULTILINE_EQUAL(szExpected, sResults);

        // BPMS - not specified with units in output

        target.SetThroughput(1000);
        nWritten = sprintf_s(szExpected, sizeof(szExpected),
                             pcszOutputTemplate, "", "1000");
        VERIFY_IS_GREATER_THAN(nWritten, 0);
        sResults = target.GetXml(0);
        VERIFY_MULTILINE_EQUAL(szExpected, sResults);

        // Test BypassIO

        const char *pcszOutputTemplateBypassIO = \
            "<Target>\n"
            "  <Path>testfile1.dat</Path>\n"
            "  <BlockSize>65536</BlockSize>\n"
            "  <BaseFileOffset>0</BaseFileOffset>\n"
            "  <SequentialScan>false</SequentialScan>\n"
            "  <RandomAccess>false</RandomAccess>\n"
            "  <TemporaryFile>false</TemporaryFile>\n"
            "  <UseLargePages>false</UseLargePages>\n"
            "  <DisableOSCache>true</DisableOSCache>\n"
            "  <WriteThrough>true</WriteThrough>\n"
            "  <BypassIO>%s</BypassIO>\n"
            "  <WriteBufferContent>\n"
            "    <Pattern>sequential</Pattern>\n"
            "  </WriteBufferContent>\n"
            "  <ParallelAsyncIO>false</ParallelAsyncIO>\n"
            "  <StrideSize>65536</StrideSize>\n"
            "  <InterlockedSequential>false</InterlockedSequential>\n"
            "  <ThreadStride>0</ThreadStride>\n"
            "  <MaxFileSize>0</MaxFileSize>\n"
            "  <RequestCount>2</RequestCount>\n"
            "  <WriteRatio>0</WriteRatio>\n"
            "  <Throughput>0</Throughput>\n"
            "  <ThreadsPerFile>1</ThreadsPerFile>\n"
            "  <IOPriority>3</IOPriority>\n"
            "  <Weight>1</Weight>\n"
            "</Target>\n";

        // Test BypassIO while allowing partial bypass
        target.SetThroughput(0);
        target.SetBypassIoMode(BypassIoMode::Partial);
        nWritten = sprintf_s(szExpected, sizeof(szExpected),
                             pcszOutputTemplateBypassIO, "Partial");
        VERIFY_IS_GREATER_THAN(nWritten, 0);
        sResults = target.GetXml(0);
        VERIFY_MULTILINE_EQUAL(szExpected, sResults);

        // Test full BypassIO

        target.SetBypassIoMode(BypassIoMode::Full);
        nWritten = sprintf_s(szExpected, sizeof(szExpected),
                             pcszOutputTemplateBypassIO, "Full");
        VERIFY_IS_GREATER_THAN(nWritten, 0);
        sResults = target.GetXml(0);
        VERIFY_MULTILINE_EQUAL(szExpected, sResults);
    }

    void XmlResultParserUnitTests::Test_ParseProfileBufferSeparationSystemDefault()
    {
        Profile profile;
        TimeSpan timeSpan;
        Target target;

        timeSpan.SetBufferSeparation(BufferSeparation::SystemDefault);
        timeSpan.AddTarget(target);
        profile.AddTimeSpan(timeSpan);

        string s = profile.GetXml(0);

        // Profile XML should have BufferSeparation but NOT EffectiveBufferSeparation
        VERIFY_IS_TRUE(s.find("<BufferSeparation>SystemDefault</BufferSeparation>") != string::npos);
        VERIFY_IS_TRUE(s.find("EffectiveBufferSeparation") == string::npos);
    }

    void XmlResultParserUnitTests::Test_ParseProfileBufferSeparation()
    {
        Profile profile;
        TimeSpan timeSpan;
        Target target;

        timeSpan.AddTarget(target);
        profile.AddTimeSpan(timeSpan);

        string s = profile.GetXml(0);

        // Profile XML should have BufferSeparation but NOT EffectiveBufferSeparation
        VERIFY_IS_TRUE(s.find("<BufferSeparation>PDECacheLine</BufferSeparation>") != string::npos);
        VERIFY_IS_TRUE(s.find("EffectiveBufferSeparation") == string::npos);
    }

    void XmlResultParserUnitTests::Test_ParseResultsBufferSeparation8KPage128BLine()
    {
        //
        // Verify that the XML result output uses the mock system's 8K/128B
        // values (128MiB = 134217728 effective separation), not the real system.
        //

        Profile profile;
        TimeSpan timeSpan;
        Target target;
        XmlResultParser parser;

        Results results;
        results.fUseETW = false;
        results.ullTimeCount = PerfTimer::SecondsToPerfTime(10.0);

        SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION spi = {};
        results.vSystemProcessorPerfInfo.push_back(spi);

        TargetResults targetResults;
        targetResults.sPath = "testfile.dat";
        targetResults.ullFileSize = 1024 * 1024;

        ThreadResults threadResults;
        threadResults.vTargetResults.push_back(targetResults);
        results.vThreadResults.push_back(threadResults);

        vector<Results> vResults;
        vResults.push_back(results);

        SystemInformation system;
        system.ResetTime();
        system.sComputerName.clear();
        system.sProcessorName.clear();
        system.sActivePolicyName.clear();
        system.sActivePolicyGuid.clear();
        system.dwPageSize = 8192;
        system.processorTopology._ulProcessorCount = 1;
        system.processorTopology._ubPerformanceEfficiencyClass = 0;
        system.processorTopology._fSMT = false;
        system.processorTopology._vProcessorGroupInformation.clear();
        system.processorTopology._vProcessorGroupInformation.emplace_back((WORD)0, (BYTE)1, (BYTE)1, (KAFFINITY)0x1);
        ProcessorNumaInformation node;
        node._nodeNumber = 0;
        node._ulProcCount = 0;
        node._vProcessorMasks.emplace_back((WORD)0, (KAFFINITY)0x1);
        system.processorTopology._vProcessorNumaInformation.clear();
        system.processorTopology._vProcessorNumaInformation.push_back(node);
        ProcessorSocketInformation socket;
        socket._ulSocketNumber = 0;
        socket._ulProcCount = 0;
        socket._vProcessorMasks.emplace_back((WORD)0, (KAFFINITY)0x1);
        system.processorTopology._vProcessorSocketInformation.clear();
        system.processorTopology._vProcessorSocketInformation.push_back(socket);
        system.processorTopology._vProcessorCoreInformation.clear();
        system.processorTopology._vProcessorCoreInformation.emplace_back((WORD)0, (KAFFINITY)0x1, (BYTE)0);
        system.processorTopology._vProcessorCacheInformation.clear();
        ProcessorCacheInformation l3(3, 16, 128, 32 * 1024 * 1024, CacheUnified);
        l3._processorMasks.emplace_back((WORD)0, (KAFFINITY)0x1);
        system.processorTopology._vProcessorCacheInformation.push_back(l3);

        timeSpan.SetSystem(&system);
        timeSpan.Finalize();
        timeSpan.AddTarget(target);
        profile.AddTimeSpan(timeSpan);

        string sResults = parser.ParseResults(profile, system, vResults);

        // Profile section: BufferSeparation present, EffectiveBufferSeparation absent
        VERIFY_IS_TRUE(sResults.find("<BufferSeparation>PDECacheLine</BufferSeparation>") != string::npos);

        // Result timespan section: EffectiveBufferSeparation with 8K/128B value
        VERIFY_IS_TRUE(sResults.find("<EffectiveBufferSeparation>134217728</EffectiveBufferSeparation>") != string::npos);

        // Verify page size in system info is 8K
        VERIFY_IS_TRUE(sResults.find("<PageSize>8192</PageSize>") != string::npos);
    }
}