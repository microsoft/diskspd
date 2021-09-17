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

        // First group has 1 core
        SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION systemProcessorInfo = {};
        systemProcessorInfo.UserTime.QuadPart = static_cast<LONGLONG>(fTime * 30 * 100000);
        systemProcessorInfo.IdleTime.QuadPart = static_cast<LONGLONG>(fTime * 45 * 100000);
        systemProcessorInfo.KernelTime.QuadPart = static_cast<LONGLONG>(fTime * 70 * 100000);
        results.vSystemProcessorPerfInfo.push_back(systemProcessorInfo);

        // Second group has a maximum of 4 cores with 2 active
        SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION zeroSystemProcessorInfo = { 0 };
        zeroSystemProcessorInfo.UserTime.QuadPart = static_cast<LONGLONG>(fTime * 0 * 100000);
        zeroSystemProcessorInfo.IdleTime.QuadPart = static_cast<LONGLONG>(fTime * 100 * 100000);
        zeroSystemProcessorInfo.KernelTime.QuadPart = static_cast<LONGLONG>(fTime * 100 * 100000);
        results.vSystemProcessorPerfInfo.push_back(zeroSystemProcessorInfo);
        results.vSystemProcessorPerfInfo.push_back(zeroSystemProcessorInfo);
        results.vSystemProcessorPerfInfo.push_back(zeroSystemProcessorInfo);
        results.vSystemProcessorPerfInfo.push_back(zeroSystemProcessorInfo);
        
        // TODO: multiple target cases, full profile/result variations
        target.SetPath("testfile1.dat");
        target.SetCacheMode(TargetCacheMode::DisableOSCache);
        target.SetWriteThroughMode(WriteThroughMode::On);
        target.SetThroughputIOPS(1000);
        
        timeSpan.AddTarget(target);
        timeSpan.SetCalculateIopsStdDev(true);

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
        results.vThreadResults.push_back(threadResults);

        vector<Results> vResults;
        vResults.push_back(results);

        // just throw away the computername and reset the timestamp - for the ut, it's
        // as useful (and simpler) to verify statics as anything else.  Reconstruct
        // processor topo to a fixed example as well.
        SystemInformation system;
        system.ResetTime();
        system.sComputerName.clear();
        system.processorTopology._ulProcCount = 5;
        system.processorTopology._ulActiveProcCount = 3;

        system.processorTopology._vProcessorGroupInformation.clear();
        system.processorTopology._vProcessorGroupInformation.emplace_back((BYTE)1, (BYTE)1, (WORD)0, (KAFFINITY)0x1);
        system.processorTopology._vProcessorGroupInformation.emplace_back((BYTE)4, (BYTE)2, (WORD)1, (KAFFINITY)0x6);

        system.processorTopology._vProcessorNumaInformation.clear();
        system.processorTopology._vProcessorNumaInformation.emplace_back((DWORD)0, (WORD)0, (KAFFINITY)0x1);
        system.processorTopology._vProcessorNumaInformation.emplace_back((DWORD)1, (WORD)1, (KAFFINITY)0x6);

        ProcessorSocketInformation socket;
        socket._vProcessorMasks.emplace_back((WORD)0, (KAFFINITY)0x1);
        socket._vProcessorMasks.emplace_back((WORD)1, (KAFFINITY)0x6);
        system.processorTopology._vProcessorSocketInformation.clear();
        system.processorTopology._vProcessorSocketInformation.push_back(socket);

        system.processorTopology._vProcessorHyperThreadInformation.clear();
        system.processorTopology._vProcessorHyperThreadInformation.emplace_back((WORD)0, (KAFFINITY)0x1);
        system.processorTopology._vProcessorHyperThreadInformation.emplace_back((WORD)1, (KAFFINITY)0x6);

        // finally, add the timespan to the profile and dump.
        profile.AddTimeSpan(timeSpan);

        string sResults = parser.ParseResults(profile, system, vResults);

        // stringify random text, quoting "'s and adding newline/preserving tabs
        // gc some.txt |% { write-host $("`"{0}\n`"" -f $($_ -replace "`"","\`"" -replace "`t","\t")) }

        const char *pcszExpectedOutput = \
            "<Results>\n"
            "  <System>\n"
            "    <ComputerName></ComputerName>\n"
            "    <Tool>\n"
            "      <Version>" DISKSPD_NUMERIC_VERSION_STRING "</Version>\n"
            "      <VersionDate>" DISKSPD_DATE_VERSION_STRING "</VersionDate>\n"
            "    </Tool>\n"
            "    <RunTime></RunTime>\n"
            "    <ProcessorTopology>\n"
            "      <Group Group=\"0\" MaximumProcessors=\"1\" ActiveProcessors=\"1\" ActiveProcessorMask=\"0x1\"/>\n"
            "      <Group Group=\"1\" MaximumProcessors=\"4\" ActiveProcessors=\"2\" ActiveProcessorMask=\"0x6\"/>\n"
            "      <Node Node=\"0\" Group=\"0\" Processors=\"0x1\"/>\n"
            "      <Node Node=\"1\" Group=\"1\" Processors=\"0x6\"/>\n"
            "      <Socket>\n"
            "        <Group Group=\"0\" Processors=\"0x1\"/>\n"
            "        <Group Group=\"1\" Processors=\"0x6\"/>\n"
            "      </Socket>\n"
            "      <HyperThread Group=\"0\" Processors=\"0x1\"/>\n"
            "      <HyperThread Group=\"1\" Processors=\"0x6\"/>\n"
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
            "        <DisableAffinity>false</DisableAffinity>\n"
            "        <Duration>10</Duration>\n"
            "        <Warmup>5</Warmup>\n"
            "        <Cooldown>0</Cooldown>\n"
            "        <ThreadCount>0</ThreadCount>\n"
            "        <RequestCount>0</RequestCount>\n"
            "        <IoBucketDuration>1000</IoBucketDuration>\n"
            "        <RandSeed>0</RandSeed>\n"
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
            "    <CpuUtilization>\n"
            "      <CPU>\n"
            "        <Group>0</Group>\n"
            "        <Id>0</Id>\n"
            "        <UsagePercent>55.00</UsagePercent>\n"
            "        <UserPercent>30.00</UserPercent>\n"
            "        <KernelPercent>25.00</KernelPercent>\n"
            "        <IdlePercent>45.00</IdlePercent>\n"
            "      </CPU>\n"
            "      <CPU>\n"
            "        <Group>1</Group>\n"
            "        <Id>1</Id>\n"
            "        <UsagePercent>0.00</UsagePercent>\n"
            "        <UserPercent>0.00</UserPercent>\n"
            "        <KernelPercent>0.00</KernelPercent>\n"
            "        <IdlePercent>100.00</IdlePercent>\n"
            "      </CPU>\n"
            "      <CPU>\n"
            "        <Group>1</Group>\n"
            "        <Id>2</Id>\n"
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

#if 0
        HANDLE h;
        DWORD written;
        h = CreateFileW(L"g:\\xmlresult-received.txt", GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
        WriteFile(h, sResults.c_str(), (DWORD)sResults.length(), &written, NULL);
        VERIFY_ARE_EQUAL(sResults.length(), written);
        CloseHandle(h);

        h = CreateFileW(L"g:\\xmlresult-expected.txt", GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
        WriteFile(h, pcszExpectedOutput, (DWORD)strlen(pcszExpectedOutput), &written, NULL);
        VERIFY_ARE_EQUAL((DWORD)strlen(pcszExpectedOutput), written);
        CloseHandle(h);

        printf("--\n%s\n", sResults.c_str());
        printf("-------------------------------------------------\n");
        printf("--\n%s\n", pcszExpectedOutput);
#endif

        VERIFY_ARE_EQUAL(0, strcmp(sResults.c_str(), pcszExpectedOutput));
    }

    void XmlResultParserUnitTests::Test_ParseProfile()
    {
        Profile profile;
        XmlResultParser parser;
        TimeSpan timeSpan;
        Target target;

        timeSpan.AddTarget(target);
        profile.AddTimeSpan(timeSpan);

        string s = parser.ParseProfile(profile);
        const char *pcszExpectedOutput = "<Profile>\n"
            "  <Progress>0</Progress>\n"
            "  <ResultFormat>text</ResultFormat>\n"
            "  <Verbose>false</Verbose>\n"
            "  <TimeSpans>\n"
            "    <TimeSpan>\n"
            "      <CompletionRoutines>false</CompletionRoutines>\n"
            "      <MeasureLatency>false</MeasureLatency>\n"
            "      <CalculateIopsStdDev>false</CalculateIopsStdDev>\n"
            "      <DisableAffinity>false</DisableAffinity>\n"
            "      <Duration>10</Duration>\n"
            "      <Warmup>5</Warmup>\n"
            "      <Cooldown>0</Cooldown>\n"
            "      <ThreadCount>0</ThreadCount>\n"
            "      <RequestCount>0</RequestCount>\n"
            "      <IoBucketDuration>1000</IoBucketDuration>\n"
            "      <RandSeed>0</RandSeed>\n"
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

        //VERIFY_ARE_EQUAL(pcszExpectedOutput, s.c_str());
        VERIFY_ARE_EQUAL(strlen(pcszExpectedOutput), s.length());
        VERIFY_IS_TRUE(!strcmp(pcszExpectedOutput, s.c_str()));
    }

    void XmlResultParserUnitTests::Test_ParseTargetProfile()
    {
        Target target;
        string sResults;
        char pszExpectedOutput[4096];
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
 
        nWritten = sprintf_s(pszExpectedOutput, sizeof(pszExpectedOutput),
                             pcszOutputTemplate, "", "0");
        VERIFY_IS_GREATER_THAN(nWritten, 0);
        sResults = target.GetXml(0);
        VERIFY_ARE_EQUAL(sResults, pszExpectedOutput);

        // IOPS - with units

        target.SetThroughputIOPS(1000);
        nWritten = sprintf_s(pszExpectedOutput, sizeof(pszExpectedOutput),
                             pcszOutputTemplate, " unit=\"IOPS\"", "1000");
        VERIFY_IS_GREATER_THAN(nWritten, 0);
        sResults = target.GetXml(0);
        VERIFY_ARE_EQUAL(sResults, pszExpectedOutput);
 
        // BPMS - not specified with units in output

        target.SetThroughput(1000);
        nWritten = sprintf_s(pszExpectedOutput, sizeof(pszExpectedOutput),
                             pcszOutputTemplate, "", "1000");
        VERIFY_IS_GREATER_THAN(nWritten, 0);
        sResults = target.GetXml(0);
        VERIFY_ARE_EQUAL(sResults, pszExpectedOutput);
    }
}