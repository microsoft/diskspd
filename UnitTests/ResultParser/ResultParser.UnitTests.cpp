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
#include "ResultParser.UnitTests.h"
#include "Common.h"
#include "resultparser.h"
#include "TextDiff.h"
#include <stdlib.h>
#include <vector>

using namespace WEX::TestExecution;
using namespace WEX::Logging;
using namespace std;

namespace UnitTests
{
    void ResultParserUnitTests::Test_ParseResults()
    {
        Profile profile;
        TimeSpan timeSpan;
        ResultParser parser;

        Results results;
        results.fUseETW = false;
        double fTime = 120.0;
        results.ullTimeCount = PerfTimer::SecondsToPerfTime(fTime);

        SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION systemProcessorInfo = {};
        systemProcessorInfo.UserTime.QuadPart = static_cast<LONGLONG>(fTime * 30 * 100000);
        systemProcessorInfo.IdleTime.QuadPart = static_cast<LONGLONG>(fTime * 45 * 100000);
        systemProcessorInfo.KernelTime.QuadPart = static_cast<LONGLONG>(fTime * 70 * 100000);
        results.vSystemProcessorPerfInfo.push_back(systemProcessorInfo);

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

        timeSpan.SetCalculateIopsStdDev(true);
        timeSpan.SetUseIoRing(true);
        timeSpan.SetUseRegBuffer(true);

        targetResults.readBucketizer.Initialize(1000, timeSpan.GetDuration());
        for (size_t i = 0; i < timeSpan.GetDuration(); i++)
        {
            // add an io halfway through the bucket's time interval
            targetResults.readBucketizer.Add(i * 1000 + 500, 100);
        }

        ThreadResults threadResults;
        threadResults.ullSubmitCount = 1200;
        threadResults.vTargetResults.push_back(targetResults);
        results.vThreadResults.push_back(threadResults);

        vector<Results> vResults;
        vResults.push_back(results);

        // just throw away the computername - for the ut, it's as useful (and simpler)
        // to verify a static null as anything else.
        SystemInformation system;
        system.sComputerName.clear();
        system.sProcessorName.clear();
        system.ResetTime();

        // and power plan
        system.sActivePolicyName.clear();
        system.sActivePolicyGuid.clear();

        system.dwPageSize = 4096;

        system.processorTopology._ulProcessorCount = 1;
        system.processorTopology._ubPerformanceEfficiencyClass = 0;
        system.processorTopology._fSMT = false;

        system.processorTopology._vProcessorGroupInformation.clear();
        system.processorTopology._vProcessorGroupInformation.emplace_back((WORD)0, (BYTE)1, (BYTE)1, (KAFFINITY)0x1);

        ProcessorNumaInformation node;
        node._nodeNumber = 0;
        node._vProcessorMasks.emplace_back((WORD)0, (KAFFINITY)0x1);
        system.processorTopology._vProcessorNumaInformation.clear();
        system.processorTopology._vProcessorNumaInformation.push_back(node);

        ProcessorSocketInformation socket;
        socket._vProcessorMasks.emplace_back((WORD)0, (KAFFINITY)0x1);
        system.processorTopology._vProcessorSocketInformation.clear();
        system.processorTopology._vProcessorSocketInformation.push_back(socket);

        system.processorTopology._vProcessorCoreInformation.clear();
        system.processorTopology._vProcessorCoreInformation.emplace_back((WORD)0, (KAFFINITY)0x1, (BYTE)0);

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

        const char *pcszExpected = "\n"
            "Command Line: \n"
            "\n"
            "Input parameters:\n"
            "\n"
            "  timespan:   1\n"
            "  -------------\n"
            "    duration: 10s\n"
            "    warm up time: 5s\n"
            "    cool down time: 0s\n"
            "    gathering IOPS at intervals of 1000ms\n"
            "    random seed: 0\n"
            "    using IoRing (batch size: 25%) with registered buffers\n"
            "    affinity assignment: cpu order, fill groups, P-cores first\n"
            "    buffer separation: thread optimized (16MiB)\n"
            "\n"
            "System information:\n\n"
            "  computer name:        \n"
            "  processor name:       \n"
            "  start time:           \n"
            "  active power scheme:  \n"
            "  page size:            4KiB\n"
            "\n"
            "  cpu count:            1\n"
            "  core count:           1\n"
            "  group count:          1\n"
            "  node count:           1\n"
            "  socket count:         1\n"
            "  heterogeneous cores:  n\n"
            "\n"
            "  cache information:\n\n"
            "    Cache |   Size   | Line  | Assoc  | CPU\n"
            "    -------------------------------------------------------\n"
            "    L3    |     8MiB |   64B | 16-way | 0\n"
            "\n"
            "Results for timespan 1:\n"
            "*******************************************************************************\n"
            "\n"
            "actual test time:\t120.00s\n"
            "thread count:\t\t1\n"
            "\n"
            "effective affinity (cpu): 0\n"
            "\n"
            "CPU |  Usage |  User  | Kernel |  Idle\n"
            "----------------------------------------\n"
            "   0|  55.00%|  30.00%|  25.00%|  45.00%\n"
            "----------------------------------------\n"
            "avg.|  55.00%|  30.00%|  25.00%|  45.00%\n"
            "\n"
            "Total IO\n"
            "thread |       bytes     |     I/Os     |    MiB/s   |  I/O per s | IopsStdDev |  file\n"
            "-------------------------------------------------------------------------------------------\n"
            "     0 |         6291456 |           16 |       0.05 |       0.13 |       0.00 | testfile1.dat (10MiB)\n"
            "-------------------------------------------------------------------------------------------\n"
            "total:           6291456 |           16 |       0.05 |       0.13 |       0.00\n"
            "\n"
            "Read IO\n"
            "thread |       bytes     |     I/Os     |    MiB/s   |  I/O per s | IopsStdDev |  file\n"
            "-------------------------------------------------------------------------------------------\n"
            "     0 |         4194304 |            6 |       0.03 |       0.05 |       0.00 | testfile1.dat (10MiB)\n"
            "-------------------------------------------------------------------------------------------\n"
            "total:           4194304 |            6 |       0.03 |       0.05 |       0.00\n"
            "\n"
            "Write IO\n"
            "thread |       bytes     |     I/Os     |    MiB/s   |  I/O per s | IopsStdDev |  file\n"
            "-------------------------------------------------------------------------------------------\n"
            "     0 |         2097152 |           10 |       0.02 |       0.08 |       0.00 | testfile1.dat (10MiB)\n"
            "-------------------------------------------------------------------------------------------\n"
            "total:           2097152 |           10 |       0.02 |       0.08 |       0.00\n"
            "\n"
            "\n"
            "IoRing Statistics\n"
            "thread |  Submits/s \n"
            "--------------------\n"
            "     0 |       10.00\n"
            "--------------------\n"
            "total: |       10.00\n";
        VERIFY_MULTILINE_EQUAL(pcszExpected, sResults);
    }

    void ResultParserUnitTests::Test_ParseProfile()
    {
        Profile profile;
        ResultParser parser;
        TimeSpan timeSpan;
        Target target;

        // Mock system for deterministic buffer separation output
        SystemInformation mockSystem;
        mockSystem.ResetTime();
        mockSystem.sComputerName.clear();
        mockSystem.sProcessorName.clear();
        mockSystem.sActivePolicyName.clear();
        mockSystem.sActivePolicyGuid.clear();
        mockSystem.dwPageSize = 4096;
        mockSystem.processorTopology._vProcessorCacheInformation.clear();
        ProcessorCacheInformation l3(3, 16, 64, 8 * 1024 * 1024, CacheUnified);
        l3._processorMasks.emplace_back((WORD)0, (KAFFINITY)0x1);
        mockSystem.processorTopology._vProcessorCacheInformation.push_back(l3);

        timeSpan.SetSystem(&mockSystem);
        timeSpan.Finalize();
        timeSpan.AddTarget(target);
        profile.AddTimeSpan(timeSpan);

        string s = parser.ParseProfile(profile);
        const char *pcszExpected = "\nCommand Line: \n"
            "\n"
            "Input parameters:\n"
            "\n"
            "  timespan:   1\n"
            "  -------------\n"
            "    duration: 10s\n"
            "    warm up time: 5s\n"
            "    cool down time: 0s\n"
            "    random seed: 0\n"
            "    affinity assignment: cpu order, fill groups, P-cores first\n"
            "    buffer separation: thread optimized (16MiB)\n"
            "    path: ''\n"
            "      think time: 0ms\n"
            "      burst size: 0\n"
            "      using software cache\n"
            "      using hardware write cache, writethrough off\n"
            "      performing read test\n"
            "      block size: 64KiB\n"
            "      using sequential I/O (stride: 64KiB)\n"
            "      number of outstanding I/O operations per thread: 2\n"
            "      threads per file: 1\n"
            "      using I/O Completion Ports\n"
            "      IO priority: normal\n\n";

        VERIFY_MULTILINE_EQUAL(pcszExpected, s);
    }

    void ResultParserUnitTests::Test_PrintTarget()
    {
        Target target;

        string sResult = target.GetText(4, false, true, false);
        const char *pcszExpected = "    path: ''\n" \
            "      think time: 0ms\n"
            "      burst size: 0\n"
            "      using software cache\n"
            "      using hardware write cache, writethrough off\n"
            "      performing read test\n"
            "      block size: 64KiB\n"
            "      using sequential I/O (stride: 64KiB)\n"
            "      number of outstanding I/O operations per thread: 2\n"
            "      IO priority: normal\n";
        VERIFY_MULTILINE_EQUAL(pcszExpected, sResult);

        target.SetThreadStrideInBytes(100 * 1024);
        sResult = target.GetText(4, false, true, false);
        pcszExpected = "    path: ''\n" \
            "      think time: 0ms\n"
            "      burst size: 0\n"
            "      using software cache\n"
            "      using hardware write cache, writethrough off\n"
            "      performing read test\n"
            "      block size: 64KiB\n"
            "      using sequential I/O (stride: 64KiB)\n"
            "      number of outstanding I/O operations per thread: 2\n"
            "      thread stride size: 100KiB\n"
            "      IO priority: normal\n";
        VERIFY_MULTILINE_EQUAL(pcszExpected, sResult);
        target.SetThreadStrideInBytes(0);

        target.SetMaxFileSize(2000 * 1024);
        sResult = target.GetText(4, false, true, false);
        pcszExpected = "    path: ''\n" \
            "      think time: 0ms\n"
            "      burst size: 0\n"
            "      using software cache\n"
            "      using hardware write cache, writethrough off\n"
            "      performing read test\n"
            "      block size: 64KiB\n"
            "      using sequential I/O (stride: 64KiB)\n"
            "      number of outstanding I/O operations per thread: 2\n"
            "      max file size: 1.95MiB\n"
            "      IO priority: normal\n";
        VERIFY_MULTILINE_EQUAL(pcszExpected, sResult);
        target.SetMaxFileSize(0);

        target.SetBaseFileOffsetInBytes(2 * 1024 * 1024);
        sResult = target.GetText(4, false, true, false);
        pcszExpected = "    path: ''\n" \
            "      think time: 0ms\n"
            "      burst size: 0\n"
            "      using software cache\n"
            "      using hardware write cache, writethrough off\n"
            "      performing read test\n"
            "      block size: 64KiB\n"
            "      using sequential I/O (stride: 64KiB)\n"
            "      number of outstanding I/O operations per thread: 2\n"
            "      base file offset: 2MiB\n"
            "      IO priority: normal\n";
        VERIFY_MULTILINE_EQUAL(pcszExpected, sResult);
        target.SetBaseFileOffsetInBytes(0);

        target.SetThroughput(1000);
        sResult = target.GetText(4, false, true, false);
        pcszExpected = "    path: ''\n" \
            "      think time: 0ms\n"
            "      burst size: 0\n"
            "      using software cache\n"
            "      using hardware write cache, writethrough off\n"
            "      performing read test\n"
            "      block size: 64KiB\n"
            "      using sequential I/O (stride: 64KiB)\n"
            "      number of outstanding I/O operations per thread: 2\n"
            "      IO priority: normal\n"
            "      throughput rate-limited to 1000 B/ms\n";
        VERIFY_MULTILINE_EQUAL(pcszExpected, sResult);
        target.SetThroughput(0);

        target.SetThroughputIOPS(1000);
        sResult = target.GetText(4, false, true, false);
        pcszExpected = "    path: ''\n" \
            "      think time: 0ms\n"
            "      burst size: 0\n"
            "      using software cache\n"
            "      using hardware write cache, writethrough off\n"
            "      performing read test\n"
            "      block size: 64KiB\n"
            "      using sequential I/O (stride: 64KiB)\n"
            "      number of outstanding I/O operations per thread: 2\n"
            "      IO priority: normal\n"
            "      throughput rate-limited to 1000 IOPS\n";
        VERIFY_MULTILINE_EQUAL(pcszExpected, sResult);
        target.SetThroughputIOPS(0);

        target.SetWriteRatio(30);
        sResult = target.GetText(4, false, true, false);
        pcszExpected = "    path: ''\n" \
            "      think time: 0ms\n"
            "      burst size: 0\n"
            "      using software cache\n"
            "      using hardware write cache, writethrough off\n"
            "      performing mix test (read/write ratio: 70/30)\n"
            "      block size: 64KiB\n"
            "      using sequential I/O (stride: 64KiB)\n"
            "      number of outstanding I/O operations per thread: 2\n"
            "      IO priority: normal\n";
        VERIFY_MULTILINE_EQUAL(pcszExpected, sResult);
        target.SetWriteRatio(0);

        target.SetRandomDataWriteBufferSize(12341234);
        sResult = target.GetText(4, false, true, false);
        pcszExpected = "    path: ''\n" \
            "      think time: 0ms\n"
            "      burst size: 0\n"
            "      using software cache\n"
            "      using hardware write cache, writethrough off\n"
            "      write buffer size: 11.77MiB\n"
            "      write buffer source: random fill\n"
            "      performing read test\n"
            "      block size: 64KiB\n"
            "      using sequential I/O (stride: 64KiB)\n"
            "      number of outstanding I/O operations per thread: 2\n"
            "      IO priority: normal\n";
        VERIFY_MULTILINE_EQUAL(pcszExpected, sResult);

        target.SetRandomDataWriteBufferSourcePath("x:\\foo\\bar.dat");
        target.SetRandomRatio(100);
        sResult = target.GetText(4, false, true, false);
        pcszExpected = "    path: ''\n" \
            "      think time: 0ms\n"
            "      burst size: 0\n"
            "      using software cache\n"
            "      using hardware write cache, writethrough off\n"
            "      write buffer size: 11.77MiB\n"
            "      write buffer source: 'x:\\foo\\bar.dat'\n"
            "      performing read test\n"
            "      block size: 64KiB\n"
            "      using random I/O (alignment: 64KiB)\n"
            "      number of outstanding I/O operations per thread: 2\n"
            "      IO priority: normal\n";
        VERIFY_MULTILINE_EQUAL(pcszExpected, sResult);
        target.SetRandomDataWriteBufferSize(0);
        target.SetRandomDataWriteBufferSourcePath("");

        target.SetCacheMode(TargetCacheMode::DisableOSCache);
        sResult = target.GetText(4, false, true, false);
        pcszExpected = "    path: ''\n" \
            "      think time: 0ms\n"
            "      burst size: 0\n"
            "      software cache disabled\n"
            "      using hardware write cache, writethrough off\n"
            "      performing read test\n"
            "      block size: 64KiB\n"
            "      using random I/O (alignment: 64KiB)\n"
            "      number of outstanding I/O operations per thread: 2\n"
            "      IO priority: normal\n";
        VERIFY_MULTILINE_EQUAL(pcszExpected, sResult);

        target.SetCacheMode(TargetCacheMode::DisableOSCache);
        target.SetWriteThroughMode(WriteThroughMode::On);
        sResult = target.GetText(4, false, true, false);
        pcszExpected = "    path: ''\n" \
            "      think time: 0ms\n"
            "      burst size: 0\n"
            "      software cache disabled\n"
            "      hardware write cache disabled, writethrough on\n"
            "      performing read test\n"
            "      block size: 64KiB\n"
            "      using random I/O (alignment: 64KiB)\n"
            "      number of outstanding I/O operations per thread: 2\n"
            "      IO priority: normal\n";
        VERIFY_MULTILINE_EQUAL(pcszExpected, sResult);

        target.SetCacheMode(TargetCacheMode::Cached);
        target.SetWriteThroughMode(WriteThroughMode::On);
        sResult = target.GetText(4, false, true, false);
        pcszExpected = "    path: ''\n" \
            "      think time: 0ms\n"
            "      burst size: 0\n"
            "      using software cache\n"
            "      hardware and software write caches disabled, writethrough on\n"
            "      performing read test\n"
            "      block size: 64KiB\n"
            "      using random I/O (alignment: 64KiB)\n"
            "      number of outstanding I/O operations per thread: 2\n"
            "      IO priority: normal\n";
        VERIFY_MULTILINE_EQUAL(pcszExpected, sResult);
        target.SetWriteThroughMode(WriteThroughMode::Undefined);

        target.SetCacheMode(TargetCacheMode::DisableLocalCache);
        sResult = target.GetText(4, false, true, false);
        pcszExpected = "    path: ''\n" \
            "      think time: 0ms\n"
            "      burst size: 0\n"
            "      local software cache disabled, remote cache enabled\n"
            "      using hardware write cache, writethrough off\n"
            "      performing read test\n"
            "      block size: 64KiB\n"
            "      using random I/O (alignment: 64KiB)\n"
            "      number of outstanding I/O operations per thread: 2\n"
            "      IO priority: normal\n";
        VERIFY_MULTILINE_EQUAL(pcszExpected, sResult);

        target.SetCacheMode(TargetCacheMode::Cached);
        target.SetMemoryMappedIoMode(MemoryMappedIoMode::On);
        sResult = target.GetText(4, false, true, false);
        pcszExpected = "    path: ''\n" \
            "      think time: 0ms\n"
            "      burst size: 0\n"
            "      using software cache\n"
            "      using hardware write cache, writethrough off\n"
            "      memory mapped I/O enabled\n"
            "      performing read test\n"
            "      block size: 64KiB\n"
            "      using random I/O (alignment: 64KiB)\n"
            "      number of outstanding I/O operations per thread: 2\n"
            "      IO priority: normal\n";
        VERIFY_MULTILINE_EQUAL(pcszExpected, sResult);

        target.SetMemoryMappedIoFlushMode(MemoryMappedIoFlushMode::ViewOfFile);
        sResult = target.GetText(4, false, true, false);
        pcszExpected = "    path: ''\n" \
            "      think time: 0ms\n"
            "      burst size: 0\n"
            "      using software cache\n"
            "      using hardware write cache, writethrough off\n"
            "      memory mapped I/O enabled, flush mode: FlushViewOfFile\n"
            "      performing read test\n"
            "      block size: 64KiB\n"
            "      using random I/O (alignment: 64KiB)\n"
            "      number of outstanding I/O operations per thread: 2\n"
            "      IO priority: normal\n";
        VERIFY_MULTILINE_EQUAL(pcszExpected, sResult);

        target.SetMemoryMappedIoFlushMode(MemoryMappedIoFlushMode::NonVolatileMemory);
        sResult = target.GetText(4, false, true, false);
        pcszExpected = "    path: ''\n" \
            "      think time: 0ms\n"
            "      burst size: 0\n"
            "      using software cache\n"
            "      using hardware write cache, writethrough off\n"
            "      memory mapped I/O enabled, flush mode: FlushNonVolatileMemory\n"
            "      performing read test\n"
            "      block size: 64KiB\n"
            "      using random I/O (alignment: 64KiB)\n"
            "      number of outstanding I/O operations per thread: 2\n"
            "      IO priority: normal\n";
        VERIFY_MULTILINE_EQUAL(pcszExpected, sResult);

        target.SetMemoryMappedIoFlushMode(MemoryMappedIoFlushMode::NonVolatileMemoryNoDrain);
        sResult = target.GetText(4, false, true, false);
        pcszExpected = "    path: ''\n" \
            "      think time: 0ms\n"
            "      burst size: 0\n"
            "      using software cache\n"
            "      using hardware write cache, writethrough off\n"
            "      memory mapped I/O enabled, flush mode: FlushNonVolatileMemory with no drain\n"
            "      performing read test\n"
            "      block size: 64KiB\n"
            "      using random I/O (alignment: 64KiB)\n"
            "      number of outstanding I/O operations per thread: 2\n"
            "      IO priority: normal\n";
        VERIFY_MULTILINE_EQUAL(pcszExpected, sResult);

        target.SetMemoryMappedIoMode(MemoryMappedIoMode::Off);
        target.SetMemoryMappedIoFlushMode(MemoryMappedIoFlushMode::Undefined);
        target.SetCacheMode(TargetCacheMode::DisableOSCache);
        target.SetBypassIoMode(BypassIoMode::Partial);
        sResult = target.GetText(4, false, true, false);
        pcszExpected = "    path: ''\n" \
            "      think time: 0ms\n"
            "      burst size: 0\n"
            "      software cache disabled\n"
            "      using hardware write cache, writethrough off\n"
            "      using BypassIO (allow partial)\n"
            "      performing read test\n"
            "      block size: 64KiB\n"
            "      using random I/O (alignment: 64KiB)\n"
            "      number of outstanding I/O operations per thread: 2\n"
            "      IO priority: normal\n";
        VERIFY_MULTILINE_EQUAL(pcszExpected, sResult);

        target.SetBypassIoMode(BypassIoMode::Full);
        sResult = target.GetText(4, false, true, false);
        pcszExpected = "    path: ''\n" \
            "      think time: 0ms\n"
            "      burst size: 0\n"
            "      software cache disabled\n"
            "      using hardware write cache, writethrough off\n"
            "      using BypassIO (full bypass)\n"
            "      performing read test\n"
            "      block size: 64KiB\n"
            "      using random I/O (alignment: 64KiB)\n"
            "      number of outstanding I/O operations per thread: 2\n"
            "      IO priority: normal\n";
        VERIFY_MULTILINE_EQUAL(pcszExpected, sResult);

        target.SetBypassIoMode(BypassIoMode::Undefined);
        target.SetCacheMode(TargetCacheMode::DisableLocalCache);
        target.SetTemporaryFileHint(true);
        sResult = target.GetText(4, false, true, false);
        pcszExpected = "    path: ''\n" \
            "      think time: 0ms\n"
            "      burst size: 0\n"
            "      local software cache disabled, remote cache enabled\n"
            "      using hardware write cache, writethrough off\n"
            "      performing read test\n"
            "      block size: 64KiB\n"
            "      using random I/O (alignment: 64KiB)\n"
            "      number of outstanding I/O operations per thread: 2\n"
            "      using FILE_ATTRIBUTE_TEMPORARY hint\n"
            "      IO priority: normal\n";
        VERIFY_MULTILINE_EQUAL(pcszExpected, sResult);

        target.SetRandomAccessHint(true);
        sResult = target.GetText(4, false, true, false);
        pcszExpected = "    path: ''\n" \
            "      think time: 0ms\n"
            "      burst size: 0\n"
            "      local software cache disabled, remote cache enabled\n"
            "      using hardware write cache, writethrough off\n"
            "      performing read test\n"
            "      block size: 64KiB\n"
            "      using random I/O (alignment: 64KiB)\n"
            "      number of outstanding I/O operations per thread: 2\n"
            "      using FILE_FLAG_RANDOM_ACCESS hint\n"
            "      using FILE_ATTRIBUTE_TEMPORARY hint\n"
            "      IO priority: normal\n";
        VERIFY_MULTILINE_EQUAL(pcszExpected, sResult);

        target.SetRandomAccessHint(false);
        target.SetTemporaryFileHint(false);
        target.SetSequentialScanHint(true);
        sResult = target.GetText(4, false, true, false);
        pcszExpected = "    path: ''\n" \
            "      think time: 0ms\n"
            "      burst size: 0\n"
            "      local software cache disabled, remote cache enabled\n"
            "      using hardware write cache, writethrough off\n"
            "      performing read test\n"
            "      block size: 64KiB\n"
            "      using random I/O (alignment: 64KiB)\n"
            "      number of outstanding I/O operations per thread: 2\n"
            "      using FILE_FLAG_SEQUENTIAL_SCAN hint\n"
            "      IO priority: normal\n";
        VERIFY_MULTILINE_EQUAL(pcszExpected, sResult);
    }

    void ResultParserUnitTests::Test_PrintTargetDistributionPct()
    {
        Target target;

        vector<DistributionRange> v;

        // these match the CmdLineParser UTs

        // -rdpct10/10:10/10:0/10, though we need to produce the tail here
        v.emplace_back(0, 10, make_pair(0, 10));
        v.emplace_back(10, 10, make_pair(10, 10));
        v.emplace_back(20, 0, make_pair(20, 10));   // zero IO% length hole
        v.emplace_back(20, 80, make_pair(30, 70));
        target.SetDistributionRange(v, DistributionType::Percent);

        string sResult = target.GetText(4, false, true, false);
        const char *pcszExpected = "    path: ''\n" \
            "      think time: 0ms\n"
            "      burst size: 0\n"
            "      using software cache\n"
            "      using hardware write cache, writethrough off\n"
            "      performing read test\n"
            "      block size: 64KiB\n"
            "      using sequential I/O (stride: 64KiB)\n"
            "      number of outstanding I/O operations per thread: 2\n"
            "      IO priority: normal\n"
            "      IO Distribution:\n"
            "          10% of IO => [ 0% -  10%) of target\n"
            "          10% of IO => [10% -  20%) of target\n"
            "           0% of IO => [20% -  30%) of target\n"
            "          80% of IO => [30% - 100%) of target\n";
        VERIFY_MULTILINE_EQUAL(pcszExpected, sResult);
        v.clear();
    }

    void ResultParserUnitTests::Test_PrintTargetDistributionAbs()
    {
        Target target;

        vector<DistributionRange> v;

        // these match the CmdLineParser UTs

        // -rdabs10/1G:10/1G:0/100G, again producing tail
        v.emplace_back(0,10, make_pair(0, 1*GB));
        v.emplace_back(10,10, make_pair(1*GB, 1*GB));
        v.emplace_back(20,0, make_pair(2*GB, 100*GB));
        v.emplace_back(20,80, make_pair(102*GB, 0));
        target.SetDistributionRange(v, DistributionType::Absolute);

        string sResult = target.GetText(4, false, true, false);
        const char *pcszExpected = "    path: ''\n" \
            "      think time: 0ms\n"
            "      burst size: 0\n"
            "      using software cache\n"
            "      using hardware write cache, writethrough off\n"
            "      performing read test\n"
            "      block size: 64KiB\n"
            "      using sequential I/O (stride: 64KiB)\n"
            "      number of outstanding I/O operations per thread: 2\n"
            "      IO priority: normal\n"
            "      IO Distribution:\n"
            "          10% of IO => [     0    -      1GiB)\n"
            "          10% of IO => [     1GiB -      2GiB)\n"
            "           0% of IO => [     2GiB -    102GiB)\n"
            "          80% of IO => [   102GiB -       end)\n";
        VERIFY_MULTILINE_EQUAL(pcszExpected, sResult);
        v.clear();
    }

    void ResultParserUnitTests::Test_PrintEffectiveDistributionPct()
    {
        // the first matches the corresponding IORequestGenerator effdist UT
        ResultParser parser;

        Target target;
        target.SetBlockAlignmentInBytes(4*KB);
        target.SetBlockSizeInBytes(4*KB);

        Random r;
        ThreadParameters tp;
        tp.pRand = &r;

        vector<DistributionRange> v;

        // -rdpct10/10:10/10:0/10 + tail
        // this is the same distribution in the cmdlineparser UT
        v.emplace_back(0, 10, make_pair(0, 10));
        v.emplace_back(10, 10, make_pair(10, 10));
        v.emplace_back(20, 0, make_pair(20, 10));   // zero IO% length hole
        v.emplace_back(20, 80, make_pair(30, 70));
        target.SetDistributionRange(v, DistributionType::Percent);
        tp.vTargets.push_back(target);

        {
            ThreadTargetState tts(&tp, 0, 100*KB);

            Results results;
            ThreadResults threadResults;
            TargetResults targetResults;

            targetResults.sPath = "testfile.dat";
            targetResults.distribution = tts._distribution;

            threadResults.vTargetResults.push_back(targetResults);
            results.vThreadResults.push_back(threadResults);

            parser._PrintEffectiveDistributions(results);

            const char* pcszExpected = "\nEffective IO Distributions\n" \
                "--------------------------\n"
                "target: testfile.dat [thread: 0]\n"
                "    10% of IO => [     0    -      8KiB)\n"
                "    10% of IO => [     8KiB -     20KiB)\n"
                "    80% of IO => [    28KiB -    100KiB)\n";
            VERIFY_MULTILINE_EQUAL(pcszExpected, parser._sResult);

            parser._sResult.clear();
        }

        //
        // Tests of distribution deduplication.
        //

        // now repeat, duplicating the thread result for a second thread

        {
            ThreadTargetState tts(&tp, 0, 100*KB);

            Results results;
            ThreadResults threadResults;
            TargetResults targetResults;

            targetResults.sPath = "testfile.dat";
            targetResults.distribution = tts._distribution;

            threadResults.vTargetResults.push_back(targetResults);
            results.vThreadResults.push_back(threadResults);
            results.vThreadResults.push_back(threadResults);

            parser._PrintEffectiveDistributions(results);

            const char* pcszExpected = "\nEffective IO Distributions\n" \
                "--------------------------\n"
                "target: testfile.dat [thread: 0 1]\n"
                "    10% of IO => [     0    -      8KiB)\n"
                "    10% of IO => [     8KiB -     20KiB)\n"
                "    80% of IO => [    28KiB -    100KiB)\n";
            VERIFY_MULTILINE_EQUAL(pcszExpected, parser._sResult);

            parser._sResult.clear();
        }

        // now repeat, for a third thread - ellision

        {
            ThreadTargetState tts(&tp, 0, 100*KB);

            Results results;
            ThreadResults threadResults;
            TargetResults targetResults;

            targetResults.sPath = "testfile.dat";
            targetResults.distribution = tts._distribution;

            threadResults.vTargetResults.push_back(targetResults);
            results.vThreadResults.push_back(threadResults);
            results.vThreadResults.push_back(threadResults);
            results.vThreadResults.push_back(threadResults);

            parser._PrintEffectiveDistributions(results);

            const char* pcszExpected = "\nEffective IO Distributions\n" \
                "--------------------------\n"
                "target: testfile.dat [thread: 0 - 2]\n"
                "    10% of IO => [     0    -      8KiB)\n"
                "    10% of IO => [     8KiB -     20KiB)\n"
                "    80% of IO => [    28KiB -    100KiB)\n";
            VERIFY_MULTILINE_EQUAL(pcszExpected, parser._sResult);

            parser._sResult.clear();
        }

        // now repeat, moving the third thread to a different target

        {
            ThreadTargetState tts(&tp, 0, 100*KB);

            Results results;
            ThreadResults threadResults;
            TargetResults targetResults;

            targetResults.distribution = tts._distribution;

            targetResults.sPath = "testfile.dat";
            threadResults.vTargetResults.push_back(targetResults);
            results.vThreadResults.push_back(threadResults);
            results.vThreadResults.push_back(threadResults);

            targetResults.sPath = "testfile2.dat";
            threadResults.vTargetResults.clear();
            threadResults.vTargetResults.push_back(targetResults);
            results.vThreadResults.push_back(threadResults);

            parser._PrintEffectiveDistributions(results);

            const char* pcszExpected = "\nEffective IO Distributions\n" \
                "--------------------------\n"
                "target: testfile.dat [thread: 0 1]\n"
                "target: testfile2.dat [thread: 2]\n"
                "    10% of IO => [     0    -      8KiB)\n"
                "    10% of IO => [     8KiB -     20KiB)\n"
                "    80% of IO => [    28KiB -    100KiB)\n";
            VERIFY_MULTILINE_EQUAL(pcszExpected, parser._sResult);

            parser._sResult.clear();
        }

        // now repeat, four threads on the first target, three contiguous and one not
        // the thread on the second target is used to create the gap - ellision will occur
        // and the fourth thread will stand alone

        {
            ThreadTargetState tts(&tp, 0, 100*KB);

            Results results;
            ThreadResults threadResults;
            TargetResults targetResults;

            targetResults.distribution = tts._distribution;

            targetResults.sPath = "testfile.dat";
            threadResults.vTargetResults.push_back(targetResults);
            results.vThreadResults.push_back(threadResults);
            results.vThreadResults.push_back(threadResults);
            results.vThreadResults.push_back(threadResults);

            threadResults.vTargetResults.clear();
            targetResults.sPath = "testfile2.dat";
            threadResults.vTargetResults.push_back(targetResults);
            results.vThreadResults.push_back(threadResults);

            threadResults.vTargetResults.clear();
            targetResults.sPath = "testfile.dat";
            threadResults.vTargetResults.push_back(targetResults);
            results.vThreadResults.push_back(threadResults);

            parser._PrintEffectiveDistributions(results);

            const char* pcszExpected = "\nEffective IO Distributions\n" \
                "--------------------------\n"
                "target: testfile.dat [thread: 0 - 2 4]\n"
                "target: testfile2.dat [thread: 3]\n"
                "    10% of IO => [     0    -      8KiB)\n"
                "    10% of IO => [     8KiB -     20KiB)\n"
                "    80% of IO => [    28KiB -    100KiB)\n";
            VERIFY_MULTILINE_EQUAL(pcszExpected, parser._sResult);

            parser._sResult.clear();
        }

        // two distinct distributions which share the same IO%

        {
            ThreadTargetState tts1(&tp, 0, 100*KB);
            ThreadTargetState tts2(&tp, 0, 1*MB);

            Results results;
            ThreadResults threadResults;
            TargetResults targetResults;

            targetResults.distribution = tts1._distribution;
            targetResults.sPath = "testfile.dat";
            threadResults.vTargetResults.push_back(targetResults);
            results.vThreadResults.push_back(threadResults);

            threadResults.vTargetResults.clear();

            targetResults.distribution = tts2._distribution;
            targetResults.sPath = "testfile2.dat";
            threadResults.vTargetResults.push_back(targetResults);
            results.vThreadResults.push_back(threadResults);

            parser._PrintEffectiveDistributions(results);

            const char* pcszExpected = "\nEffective IO Distributions\n" \
                "--------------------------\n"
                "target: testfile.dat [thread: 0]\n"
                "    10% of IO => [     0    -      8KiB)\n"
                "    10% of IO => [     8KiB -     20KiB)\n"
                "    80% of IO => [    28KiB -    100KiB)\n"
                "target: testfile2.dat [thread: 1]\n"
                "    10% of IO => [     0    -    100KiB)\n"
                "    10% of IO => [   100KiB -    204KiB)\n"
                "    80% of IO => [   304KiB -      1MiB)\n";
            VERIFY_MULTILINE_EQUAL(pcszExpected, parser._sResult);

            parser._sResult.clear();
        }

        tp.vTargets.clear();
        v.clear();
    }

    void ResultParserUnitTests::Test_PrintEffectiveDistributionAbs()
    {
        // the first matches the corresponding IORequestGenerator effdist UT
        ResultParser parser;

        Target target;
        target.SetBlockAlignmentInBytes(4*KB);
        target.SetBlockSizeInBytes(4*KB);

        Random r;
        ThreadParameters tp;

        vector<DistributionRange> v;

        // -rdabs10/1G:10/1G:0/100G, again producing tail - with autoscale (0)
        // this is the same distribution in the cmdlineparser UT
        // aligned tail range
        v.emplace_back(0,10, make_pair(0, 1*GB));
        v.emplace_back(10,10, make_pair(1*GB, 1*GB));
        v.emplace_back(20,0, make_pair(2*GB, 100*GB));
        v.emplace_back(20,80, make_pair(102*GB, 0));
        target.SetDistributionRange(v, DistributionType::Absolute);
        tp.vTargets.push_back(target);

        {
            ThreadTargetState tts(&tp, 0, 200*GB);

            Results results;
            ThreadResults threadResults;
            TargetResults targetResults;

            targetResults.sPath = "testfile.dat";
            targetResults.distribution = tts._distribution;

            threadResults.vTargetResults.push_back(targetResults);
            results.vThreadResults.push_back(threadResults);

            parser._PrintEffectiveDistributions(results);

            const char* pcszExpected = "\nEffective IO Distributions\n" \
                "--------------------------\n"
                "target: testfile.dat [thread: 0]\n"
                "    10% of IO => [     0    -      1GiB)\n"
                "    10% of IO => [     1GiB -      2GiB)\n"
                "    80% of IO => [   102GiB -    200GiB)\n";
            VERIFY_MULTILINE_EQUAL(pcszExpected, parser._sResult);

            tp.vTargets.clear();
            v.clear();
            parser._sResult.clear();
        }

        // -rdabs10/50k:20/10k:30/1G on 100KiB target - autoscale tail, but trimmed on last spec'd range
        // this results in logical truncation since the covered ranges are only 60% of IO%, a case which
        // is specific to absolute distributions.
        v.emplace_back(0, 10, make_pair(0, 50*KB));
        v.emplace_back(10, 20, make_pair(50*KB, 10*KB));
        v.emplace_back(30, 30, make_pair(60*KB, 1*GB));
        v.emplace_back(60, 40, make_pair(1*GB + 60*KB, 0));
        target.SetDistributionRange(v, DistributionType::Absolute);
        tp.vTargets.push_back(target);

        {
            ThreadTargetState tts(&tp, 0, 100*KB);

            Results results;
            ThreadResults threadResults;
            TargetResults targetResults;

            targetResults.sPath = "testfile.dat";
            targetResults.distribution = tts._distribution;

            threadResults.vTargetResults.push_back(targetResults);
            results.vThreadResults.push_back(threadResults);

            parser._PrintEffectiveDistributions(results);

            const char* pcszExpected = "\nEffective IO Distributions\n" \
                "--------------------------\n"
                "target: testfile.dat [thread: 0]\n"
                "    16.7% of IO => [     0    -     50KiB)\n"
                "    33.3% of IO => [    50KiB -     60KiB)\n"
                "    50.0% of IO => [    60KiB -    100KiB)\n";
            VERIFY_MULTILINE_EQUAL(pcszExpected, parser._sResult);

            tp.vTargets.clear();
            v.clear();
            parser._sResult.clear();
        }
    }

    void ResultParserUnitTests::Test_PrintProfileBufferSeparationSystemDefault()
    {
        Profile profile;
        ResultParser parser;
        TimeSpan timeSpan;
        Target target;

        SystemInformation mockSystem;
        mockSystem.ResetTime();
        mockSystem.sComputerName.clear();
        mockSystem.sProcessorName.clear();
        mockSystem.sActivePolicyName.clear();
        mockSystem.sActivePolicyGuid.clear();
        mockSystem.dwPageSize = 4096;
        mockSystem.processorTopology._vProcessorCacheInformation.clear();

        timeSpan.SetBufferSeparation(BufferSeparation::SystemDefault);
        timeSpan.SetSystem(&mockSystem);
        timeSpan.Finalize();
        timeSpan.AddTarget(target);
        profile.AddTimeSpan(timeSpan);

        string s = parser.ParseProfile(profile);
        const char *pcszExpected = "\nCommand Line: \n"
            "\n"
            "Input parameters:\n"
            "\n"
            "  timespan:   1\n"
            "  -------------\n"
            "    duration: 10s\n"
            "    warm up time: 5s\n"
            "    cool down time: 0s\n"
            "    random seed: 0\n"
            "    affinity assignment: cpu order, fill groups, P-cores first\n"
            "    buffer separation: system default\n"
            "    path: ''\n"
            "      think time: 0ms\n"
            "      burst size: 0\n"
            "      using software cache\n"
            "      using hardware write cache, writethrough off\n"
            "      performing read test\n"
            "      block size: 64KiB\n"
            "      using sequential I/O (stride: 64KiB)\n"
            "      number of outstanding I/O operations per thread: 2\n"
            "      threads per file: 1\n"
            "      using I/O Completion Ports\n"
            "      IO priority: normal\n\n";

        VERIFY_MULTILINE_EQUAL(pcszExpected, s);
    }

    void ResultParserUnitTests::Test_PrintProfileBufferSeparation8KPage128BLine()
    {
        Profile profile;
        ResultParser parser;
        TimeSpan timeSpan;
        Target target;

        // 8K pages, 128B cache line => 128MiB effective separation
        // This verifies the mock system is actually being used (not the real system)
        SystemInformation mockSystem;
        mockSystem.ResetTime();
        mockSystem.sComputerName.clear();
        mockSystem.sProcessorName.clear();
        mockSystem.sActivePolicyName.clear();
        mockSystem.sActivePolicyGuid.clear();
        mockSystem.dwPageSize = 8192;
        mockSystem.processorTopology._vProcessorCacheInformation.clear();
        ProcessorCacheInformation l3(3, 16, 128, 32 * 1024 * 1024, CacheUnified);
        l3._processorMasks.emplace_back((WORD)0, (KAFFINITY)0x1);
        mockSystem.processorTopology._vProcessorCacheInformation.push_back(l3);

        timeSpan.SetSystem(&mockSystem);
        timeSpan.Finalize();
        timeSpan.AddTarget(target);
        profile.AddTimeSpan(timeSpan);

        string s = parser.ParseProfile(profile);
        const char *pcszExpected = "\nCommand Line: \n"
            "\n"
            "Input parameters:\n"
            "\n"
            "  timespan:   1\n"
            "  -------------\n"
            "    duration: 10s\n"
            "    warm up time: 5s\n"
            "    cool down time: 0s\n"
            "    random seed: 0\n"
            "    affinity assignment: cpu order, fill groups, P-cores first\n"
            "    buffer separation: thread optimized (128MiB)\n"
            "    path: ''\n"
            "      think time: 0ms\n"
            "      burst size: 0\n"
            "      using software cache\n"
            "      using hardware write cache, writethrough off\n"
            "      performing read test\n"
            "      block size: 64KiB\n"
            "      using sequential I/O (stride: 64KiB)\n"
            "      number of outstanding I/O operations per thread: 2\n"
            "      threads per file: 1\n"
            "      using I/O Completion Ports\n"
            "      IO priority: normal\n\n";

        VERIFY_MULTILINE_EQUAL(pcszExpected, s);
    }

    void ResultParserUnitTests::Test_PrintResultsBufferSeparation8KPage128BLine()
    {
        //
        // Verify that the text result output uses the mock system's 8K/128B
        // values (128MiB effective separation), not the real system.
        //

        Profile profile;
        TimeSpan timeSpan;
        ResultParser parser;
        Target target;

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
        system.sComputerName.clear();
        system.sProcessorName.clear();
        system.ResetTime();
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

        // Verify the 8K/128B effective separation appears in text output
        VERIFY_IS_TRUE(sResults.find("buffer separation: thread optimized (128MiB)") != string::npos);
    }

    void ResultParserUnitTests::Test_PrintWaitStatsNoThrottleNoLookaside()
    {
        ResultParser parser;
        TimeSpan timeSpan;

        Results results;
        ThreadResults tr;
        tr.WaitStats = {};
        tr.WaitStats.Wait = 100;
        tr.WaitStats.fThrottled = false;
        tr.WaitStats.WaitCompletion[0] = 10;
        tr.WaitStats.WaitCompletion[1] = 20;
        tr.WaitStats.WaitCompletion[2] = 30;
        tr.WaitStats.WaitCompletion[3] = 40;
        results.vThreadResults.push_back(tr);

        parser._PrintWaitStats(results, timeSpan);

        const char *pcszExpected =
            "Wait Statistics - Completion Wait\n"
            "thread |         wait | 0 - 7+ complete per wait\n"
            "------------------------------------------------\n"
            "     0 |          100 | 10 20 30 40 0 0 0 0\n";

        VERIFY_MULTILINE_EQUAL(pcszExpected, parser._sResult);
    }

    void ResultParserUnitTests::Test_PrintWaitStatsWithThrottle()
    {
        ResultParser parser;
        TimeSpan timeSpan;

        Results results;

        // Thread 0: throttled
        ThreadResults tr0;
        tr0.WaitStats = {};
        tr0.WaitStats.Wait = 100;
        tr0.WaitStats.ThrottleWait = 50;
        tr0.WaitStats.ThrottleSleep = 5;
        tr0.WaitStats.fThrottled = true;
        tr0.WaitStats.WaitCompletion[1] = 90;
        results.vThreadResults.push_back(tr0);

        // Thread 1: not throttled
        ThreadResults tr1;
        tr1.WaitStats = {};
        tr1.WaitStats.Wait = 200;
        tr1.WaitStats.fThrottled = false;
        tr1.WaitStats.WaitCompletion[2] = 180;
        results.vThreadResults.push_back(tr1);

        parser._PrintWaitStats(results, timeSpan);

        const char *pcszExpected =
            "Wait Statistics - Completion Wait\n"
            "thread |         wait | throttle wait  -  sleep | 0 - 7+ complete per wait\n"
            "--------------------------------------------------------------------------\n"
            "     0 |          100 |            50  -      5 | 0 90 0 0 0 0 0 0\n"
            "     1 |          200 |               ---       | 0 0 180 0 0 0 0 0\n";

        VERIFY_MULTILINE_EQUAL(pcszExpected, parser._sResult);
    }

    void ResultParserUnitTests::Test_PrintWaitStatsWithLookaside()
    {
        ResultParser parser;
        TimeSpan timeSpan;
        timeSpan.SetMeasureLatency(true);

        Results results;
        ThreadResults tr;
        tr.WaitStats = {};
        tr.WaitStats.Wait = 100;
        tr.WaitStats.Lookaside = 500;
        tr.WaitStats.fThrottled = false;
        tr.WaitStats.WaitCompletion[1] = 90;
        tr.WaitStats.LookasideCompletion[0] = 11;
        tr.WaitStats.LookasideCompletion[1] = 22;
        tr.WaitStats.LookasideCompletion[2] = 33;
        results.vThreadResults.push_back(tr);

        parser._PrintWaitStats(results, timeSpan);

        const char *pcszExpected =
            "Wait Statistics - Completion Wait\n"
            "thread |         wait | 0 - 7+ complete per wait\n"
            "------------------------------------------------\n"
            "     0 |          100 | 0 90 0 0 0 0 0 0\n"
            "\n"
            "Wait Statistics - Lookaside\n"
            "thread |    lookaside | 0 - 7+ complete per lookaside\n"
            "-----------------------------------------------------\n"
            "     0 |          500 | 11 22 33 0 0 0 0 0\n";

        VERIFY_MULTILINE_EQUAL(pcszExpected, parser._sResult);
    }

    void ResultParserUnitTests::Test_PrintAffinityPolicy()
    {
        SystemInformation mockSystem;
        mockSystem.processorTopology._vProcessorGroupInformation.clear();
        mockSystem.processorTopology._vProcessorGroupInformation.emplace_back((WORD)0, (BYTE)4, (BYTE)4, (KAFFINITY)0xF);
        mockSystem.processorTopology._vProcessorCoreInformation.clear();
        mockSystem.processorTopology._vProcessorCoreInformation.emplace_back((WORD)0, (KAFFINITY)0x3, (BYTE)0);
        mockSystem.processorTopology._vProcessorCoreInformation.emplace_back((WORD)0, (KAFFINITY)0xC, (BYTE)0);
        mockSystem.processorTopology._ubPerformanceEfficiencyClass = 0;
        mockSystem.processorTopology._fSMT = true;

        // Default (Cpu, Fill, Unspecified resolves to PFirst)
        {
            TimeSpan ts;
            ts.SetSystem(&mockSystem);
            ts.Finalize();
            string s = ts.GetText(0);
            VERIFY_IS_TRUE(s.find("affinity assignment: cpu order, fill groups, P-cores first\n") != string::npos);
        }

        // PFirst explicitly set - same display as default
        {
            TimeSpan ts;
            ts.SetSystem(&mockSystem);
            ts.SetAffinityEfficiencyOrder(AffinityEfficiencyOrder::PFirst);
            ts.Finalize();
            string s = ts.GetText(0);
            VERIFY_IS_TRUE(s.find("affinity assignment: cpu order, fill groups, P-cores first\n") != string::npos);
        }

        // EFirst (default Cpu traversal)
        {
            TimeSpan ts;
            ts.SetSystem(&mockSystem);
            ts.SetAffinityEfficiencyOrder(AffinityEfficiencyOrder::EFirst);
            ts.Finalize();
            string s = ts.GetText(0);
            VERIFY_IS_TRUE(s.find("affinity assignment: cpu order, fill groups, E-cores first\n") != string::npos);
        }

        // FillPFirst (default Cpu traversal)
        {
            TimeSpan ts;
            ts.SetSystem(&mockSystem);
            ts.SetAffinityEfficiencyOrder(AffinityEfficiencyOrder::FillPFirst);
            ts.Finalize();
            string s = ts.GetText(0);
            VERIFY_IS_TRUE(s.find("affinity assignment: cpu order, fill groups, fill P-cores first\n") != string::npos);
        }

        // FillEFirst (default Cpu traversal)
        {
            TimeSpan ts;
            ts.SetSystem(&mockSystem);
            ts.SetAffinityEfficiencyOrder(AffinityEfficiencyOrder::FillEFirst);
            ts.Finalize();
            string s = ts.GetText(0);
            VERIFY_IS_TRUE(s.find("affinity assignment: cpu order, fill groups, fill E-cores first\n") != string::npos);
        }

        // Cpu + FillEFirst (explicit Cpu, same as default)
        {
            TimeSpan ts;
            ts.SetSystem(&mockSystem);
            ts.SetAffinityTraversal(AffinityTraversal::Cpu);
            ts.SetAffinityEfficiencyOrder(AffinityEfficiencyOrder::FillEFirst);
            ts.Finalize();
            string s = ts.GetText(0);
            VERIFY_IS_TRUE(s.find("affinity assignment: cpu order, fill groups, fill E-cores first\n") != string::npos);
        }

        // CoreAware + Span + EFirst
        {
            TimeSpan ts;
            ts.SetSystem(&mockSystem);
            ts.SetAffinityTraversal(AffinityTraversal::CoreAware);
            ts.SetAffinityGroupSpan(AffinityGroupSpan::Span);
            ts.SetAffinityEfficiencyOrder(AffinityEfficiencyOrder::EFirst);
            ts.Finalize();
            string s = ts.GetText(0);
            VERIFY_IS_TRUE(s.find("affinity assignment: core-aware, span groups, E-cores first\n") != string::npos);
        }

        // Disabled - no affinity assignment line at all
        {
            TimeSpan ts;
            ts.SetSystem(&mockSystem);
            ts.SetDisableAffinity(true);
            ts.Finalize();
            string s = ts.GetText(0);
            VERIFY_IS_TRUE(s.find("affinity disabled\n") != string::npos);
            VERIFY_IS_TRUE(s.find("affinity assignment") == string::npos);
        }
    }
}