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
        targetResults.readBucketizer.Initialize(1000, timeSpan.GetDuration());
        for (size_t i = 0; i < timeSpan.GetDuration(); i++)
        {
            // add an io halfway through the bucket's time interval
            targetResults.readBucketizer.Add(i * 1000 + 500, 100);
        }

        ThreadResults threadResults;
        threadResults.vTargetResults.push_back(targetResults);
        results.vThreadResults.push_back(threadResults);

        vector<Results> vResults;
        vResults.push_back(results);

        // just throw away the computername - for the ut, it's as useful (and simpler)
        // to verify a static null as anything else.
        SystemInformation system;
        system.sComputerName.clear();
        system.ResetTime();

        // and power plan
        system.sActivePolicyName.clear();
        system.sActivePolicyGuid.clear();

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

        // finally, add the timespan to the profile and dump.
        profile.AddTimeSpan(timeSpan);

        string sResults = parser.ParseResults(profile, system, vResults);

        // stringify random text, quoting "'s and adding newline/preserving tabs
        // gc some.txt |% { write-host $("`"{0}\n`"" -f $($_ -replace "`"","\`"" -replace "`t","\t")) }

        const char *pcszExpectedOutput = "\n"
            "Command Line: \n"
            "\n"
            "Input parameters:\n"
            "\n"
            "\ttimespan:   1\n"
            "\t-------------\n"
            "\tduration: 10s\n"
            "\twarm up time: 5s\n"
            "\tcool down time: 0s\n"
            "\tgathering IOPS at intervals of 1000ms\n"
            "\trandom seed: 0\n"
            "\n"
            "System information:\n\n"
            "\tcomputer name: \n"
            "\tstart time: \n"
            "\n"
            "\tcpu count:\t\t1\n"
            "\tcore count:\t\t1\n"
            "\tgroup count:\t\t1\n"
            "\tnode count:\t\t1\n"
            "\tsocket count:\t\t1\n"
            "\theterogeneous cores:\tn\n"
            "\n"
            "\tactive power scheme:\t\n"
            "\n"
            "Results for timespan 1:\n"
            "*******************************************************************************\n"
            "\n"
            "actual test time:\t120.00s\n"
            "thread count:\t\t1\n"
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
            "total:           2097152 |           10 |       0.02 |       0.08 |       0.00\n";
        VERIFY_ARE_EQUAL(sResults, pcszExpectedOutput);
    }

    void ResultParserUnitTests::Test_ParseProfile()
    {
        Profile profile;
        ResultParser parser;
        TimeSpan timeSpan;
        Target target;

        timeSpan.AddTarget(target);
        profile.AddTimeSpan(timeSpan);

        string s = parser.ParseProfile(profile);
        const char *pszExpectedResult = "\nCommand Line: \n"
            "\n"
            "Input parameters:\n"
            "\n"
            "\ttimespan:   1\n"
            "\t-------------\n"
            "\tduration: 10s\n"
            "\twarm up time: 5s\n"
            "\tcool down time: 0s\n"
            "\trandom seed: 0\n"
            "\tpath: ''\n"
            "\t\tthink time: 0ms\n"
            "\t\tburst size: 0\n"
            "\t\tusing software cache\n"
            "\t\tusing hardware write cache, writethrough off\n"
            "\t\tperforming read test\n"
            "\t\tblock size: 64KiB\n"
            "\t\tusing sequential I/O (stride: 64KiB)\n"
            "\t\tnumber of outstanding I/O operations per thread: 2\n"
            "\t\tthreads per file: 1\n"
            "\t\tusing I/O Completion Ports\n"
            "\t\tIO priority: normal\n\n";

        VERIFY_ARE_EQUAL(strlen(pszExpectedResult), s.length());
        VERIFY_IS_TRUE(!strcmp(pszExpectedResult, s.c_str()));
    }

    void ResultParserUnitTests::Test_PrintTarget()
    {
        ResultParser parser;
        Target target;

        parser._PrintTarget(target, false, true, false);
        const char *pszExpectedResult = "\tpath: ''\n" \
            "\t\tthink time: 0ms\n"
            "\t\tburst size: 0\n"
            "\t\tusing software cache\n"
            "\t\tusing hardware write cache, writethrough off\n"
            "\t\tperforming read test\n"
            "\t\tblock size: 64KiB\n"
            "\t\tusing sequential I/O (stride: 64KiB)\n"
            "\t\tnumber of outstanding I/O operations per thread: 2\n"
            "\t\tIO priority: normal\n";
        VERIFY_ARE_EQUAL(parser._sResult, pszExpectedResult);

        parser._sResult.clear();
        target.SetThreadStrideInBytes(100 * 1024);
        parser._PrintTarget(target, false, true, false);
        pszExpectedResult = "\tpath: ''\n" \
            "\t\tthink time: 0ms\n"
            "\t\tburst size: 0\n"
            "\t\tusing software cache\n"
            "\t\tusing hardware write cache, writethrough off\n"
            "\t\tperforming read test\n"
            "\t\tblock size: 64KiB\n"
            "\t\tusing sequential I/O (stride: 64KiB)\n"
            "\t\tnumber of outstanding I/O operations per thread: 2\n"
            "\t\tthread stride size: 100KiB\n"
            "\t\tIO priority: normal\n";
        VERIFY_ARE_EQUAL(parser._sResult, pszExpectedResult);
        target.SetThreadStrideInBytes(0);

        parser._sResult.clear();
        target.SetMaxFileSize(2000 * 1024);
        parser._PrintTarget(target, false, true, false);
        pszExpectedResult = "\tpath: ''\n" \
            "\t\tthink time: 0ms\n"
            "\t\tburst size: 0\n"
            "\t\tusing software cache\n"
            "\t\tusing hardware write cache, writethrough off\n"
            "\t\tperforming read test\n"
            "\t\tblock size: 64KiB\n"
            "\t\tusing sequential I/O (stride: 64KiB)\n"
            "\t\tnumber of outstanding I/O operations per thread: 2\n"
            "\t\tmax file size: 1.95MiB\n"
            "\t\tIO priority: normal\n";
        VERIFY_ARE_EQUAL(parser._sResult, pszExpectedResult);
        target.SetMaxFileSize(0);

        parser._sResult.clear();
        target.SetBaseFileOffsetInBytes(2 * 1024 * 1024);
        parser._PrintTarget(target, false, true, false);
        pszExpectedResult = "\tpath: ''\n" \
            "\t\tthink time: 0ms\n"
            "\t\tburst size: 0\n"
            "\t\tusing software cache\n"
            "\t\tusing hardware write cache, writethrough off\n"
            "\t\tperforming read test\n"
            "\t\tblock size: 64KiB\n"
            "\t\tusing sequential I/O (stride: 64KiB)\n"
            "\t\tnumber of outstanding I/O operations per thread: 2\n"
            "\t\tbase file offset: 2MiB\n"
            "\t\tIO priority: normal\n";
        VERIFY_ARE_EQUAL(parser._sResult, pszExpectedResult);
        target.SetBaseFileOffsetInBytes(0);

        parser._sResult.clear();
        target.SetThroughput(1000);
        parser._PrintTarget(target, false, true, false);
        pszExpectedResult = "\tpath: ''\n" \
            "\t\tthink time: 0ms\n"
            "\t\tburst size: 0\n"
            "\t\tusing software cache\n"
            "\t\tusing hardware write cache, writethrough off\n"
            "\t\tperforming read test\n"
            "\t\tblock size: 64KiB\n"
            "\t\tusing sequential I/O (stride: 64KiB)\n"
            "\t\tnumber of outstanding I/O operations per thread: 2\n"
            "\t\tIO priority: normal\n"
            "\t\tthroughput rate-limited to 1000 B/ms\n";
        VERIFY_ARE_EQUAL(parser._sResult, pszExpectedResult);
        target.SetThroughput(0);

        parser._sResult.clear();
        target.SetThroughputIOPS(1000);
        parser._PrintTarget(target, false, true, false);
        pszExpectedResult = "\tpath: ''\n" \
            "\t\tthink time: 0ms\n"
            "\t\tburst size: 0\n"
            "\t\tusing software cache\n"
            "\t\tusing hardware write cache, writethrough off\n"
            "\t\tperforming read test\n"
            "\t\tblock size: 64KiB\n"
            "\t\tusing sequential I/O (stride: 64KiB)\n"
            "\t\tnumber of outstanding I/O operations per thread: 2\n"
            "\t\tIO priority: normal\n"
            "\t\tthroughput rate-limited to 1000 IOPS\n";
        VERIFY_ARE_EQUAL(parser._sResult, pszExpectedResult);
        target.SetThroughputIOPS(0);

        parser._sResult.clear();
        target.SetWriteRatio(30);
        parser._PrintTarget(target, false, true, false);
        pszExpectedResult = "\tpath: ''\n" \
            "\t\tthink time: 0ms\n"
            "\t\tburst size: 0\n"
            "\t\tusing software cache\n"
            "\t\tusing hardware write cache, writethrough off\n"
            "\t\tperforming mix test (read/write ratio: 70/30)\n"
            "\t\tblock size: 64KiB\n"
            "\t\tusing sequential I/O (stride: 64KiB)\n"
            "\t\tnumber of outstanding I/O operations per thread: 2\n"
            "\t\tIO priority: normal\n";
        VERIFY_ARE_EQUAL(parser._sResult, pszExpectedResult);
        target.SetWriteRatio(0);

        parser._sResult.clear();
        target.SetRandomDataWriteBufferSize(12341234);
        parser._PrintTarget(target, false, true, false);
        pszExpectedResult = "\tpath: ''\n" \
            "\t\tthink time: 0ms\n"
            "\t\tburst size: 0\n"
            "\t\tusing software cache\n"
            "\t\tusing hardware write cache, writethrough off\n"
            "\t\twrite buffer size: 11.77MiB\n"
            "\t\twrite buffer source: random fill\n"
            "\t\tperforming read test\n"
            "\t\tblock size: 64KiB\n"
            "\t\tusing sequential I/O (stride: 64KiB)\n"
            "\t\tnumber of outstanding I/O operations per thread: 2\n"
            "\t\tIO priority: normal\n";
        VERIFY_ARE_EQUAL(parser._sResult, pszExpectedResult);

        parser._sResult.clear();
        target.SetRandomDataWriteBufferSourcePath("x:\\foo\\bar.dat");
        target.SetRandomRatio(100);
        parser._PrintTarget(target, false, true, false);
        pszExpectedResult = "\tpath: ''\n" \
            "\t\tthink time: 0ms\n"
            "\t\tburst size: 0\n"
            "\t\tusing software cache\n"
            "\t\tusing hardware write cache, writethrough off\n"
            "\t\twrite buffer size: 11.77MiB\n"
            "\t\twrite buffer source: 'x:\\foo\\bar.dat'\n"
            "\t\tperforming read test\n"
            "\t\tblock size: 64KiB\n"
            "\t\tusing random I/O (alignment: 64KiB)\n"
            "\t\tnumber of outstanding I/O operations per thread: 2\n"
            "\t\tIO priority: normal\n";
        VERIFY_ARE_EQUAL(parser._sResult, pszExpectedResult);
        target.SetRandomDataWriteBufferSize(0);
        target.SetRandomDataWriteBufferSourcePath("");

        parser._sResult.clear();
        target.SetCacheMode(TargetCacheMode::DisableOSCache);
        parser._PrintTarget(target, false, true, false);
        pszExpectedResult = "\tpath: ''\n" \
            "\t\tthink time: 0ms\n"
            "\t\tburst size: 0\n"
            "\t\tsoftware cache disabled\n"
            "\t\tusing hardware write cache, writethrough off\n"
            "\t\tperforming read test\n"
            "\t\tblock size: 64KiB\n"
            "\t\tusing random I/O (alignment: 64KiB)\n"
            "\t\tnumber of outstanding I/O operations per thread: 2\n"
            "\t\tIO priority: normal\n";
        VERIFY_ARE_EQUAL(parser._sResult, pszExpectedResult);

        parser._sResult.clear();
        target.SetCacheMode(TargetCacheMode::DisableOSCache);
        target.SetWriteThroughMode(WriteThroughMode::On);
        parser._PrintTarget(target, false, true, false);
        pszExpectedResult = "\tpath: ''\n" \
            "\t\tthink time: 0ms\n"
            "\t\tburst size: 0\n"
            "\t\tsoftware cache disabled\n"
            "\t\thardware write cache disabled, writethrough on\n"
            "\t\tperforming read test\n"
            "\t\tblock size: 64KiB\n"
            "\t\tusing random I/O (alignment: 64KiB)\n"
            "\t\tnumber of outstanding I/O operations per thread: 2\n"
            "\t\tIO priority: normal\n";
        VERIFY_ARE_EQUAL(parser._sResult, pszExpectedResult);

        parser._sResult.clear();
        target.SetCacheMode(TargetCacheMode::Cached);
        target.SetWriteThroughMode(WriteThroughMode::On);
        parser._PrintTarget(target, false, true, false);
        pszExpectedResult = "\tpath: ''\n" \
            "\t\tthink time: 0ms\n"
            "\t\tburst size: 0\n"
            "\t\tusing software cache\n"
            "\t\thardware and software write caches disabled, writethrough on\n"
            "\t\tperforming read test\n"
            "\t\tblock size: 64KiB\n"
            "\t\tusing random I/O (alignment: 64KiB)\n"
            "\t\tnumber of outstanding I/O operations per thread: 2\n"
            "\t\tIO priority: normal\n";
        VERIFY_ARE_EQUAL(parser._sResult, pszExpectedResult);
        target.SetWriteThroughMode(WriteThroughMode::Undefined);

        parser._sResult.clear();
        target.SetCacheMode(TargetCacheMode::DisableLocalCache);
        parser._PrintTarget(target, false, true, false);
        pszExpectedResult = "\tpath: ''\n" \
            "\t\tthink time: 0ms\n"
            "\t\tburst size: 0\n"
            "\t\tlocal software cache disabled, remote cache enabled\n"
            "\t\tusing hardware write cache, writethrough off\n"
            "\t\tperforming read test\n"
            "\t\tblock size: 64KiB\n"
            "\t\tusing random I/O (alignment: 64KiB)\n"
            "\t\tnumber of outstanding I/O operations per thread: 2\n"
            "\t\tIO priority: normal\n";
        VERIFY_ARE_EQUAL(parser._sResult, pszExpectedResult);

        parser._sResult.clear();
        target.SetCacheMode(TargetCacheMode::Cached);
        target.SetMemoryMappedIoMode(MemoryMappedIoMode::On);
        parser._PrintTarget(target, false, true, false);
        pszExpectedResult = "\tpath: ''\n" \
            "\t\tthink time: 0ms\n"
            "\t\tburst size: 0\n"
            "\t\tusing software cache\n"
            "\t\tusing hardware write cache, writethrough off\n"
            "\t\tmemory mapped I/O enabled\n"
            "\t\tperforming read test\n"
            "\t\tblock size: 64KiB\n"
            "\t\tusing random I/O (alignment: 64KiB)\n"
            "\t\tnumber of outstanding I/O operations per thread: 2\n"
            "\t\tIO priority: normal\n";
        VERIFY_ARE_EQUAL(parser._sResult, pszExpectedResult);

        parser._sResult.clear();
        target.SetMemoryMappedIoFlushMode(MemoryMappedIoFlushMode::ViewOfFile);
        parser._PrintTarget(target, false, true, false);
        pszExpectedResult = "\tpath: ''\n" \
            "\t\tthink time: 0ms\n"
            "\t\tburst size: 0\n"
            "\t\tusing software cache\n"
            "\t\tusing hardware write cache, writethrough off\n"
            "\t\tmemory mapped I/O enabled, flush mode: FlushViewOfFile\n"
            "\t\tperforming read test\n"
            "\t\tblock size: 64KiB\n"
            "\t\tusing random I/O (alignment: 64KiB)\n"
            "\t\tnumber of outstanding I/O operations per thread: 2\n"
            "\t\tIO priority: normal\n";
        VERIFY_ARE_EQUAL(parser._sResult, pszExpectedResult);

        parser._sResult.clear();
        target.SetMemoryMappedIoFlushMode(MemoryMappedIoFlushMode::NonVolatileMemory);
        parser._PrintTarget(target, false, true, false);
        pszExpectedResult = "\tpath: ''\n" \
            "\t\tthink time: 0ms\n"
            "\t\tburst size: 0\n"
            "\t\tusing software cache\n"
            "\t\tusing hardware write cache, writethrough off\n"
            "\t\tmemory mapped I/O enabled, flush mode: FlushNonVolatileMemory\n"
            "\t\tperforming read test\n"
            "\t\tblock size: 64KiB\n"
            "\t\tusing random I/O (alignment: 64KiB)\n"
            "\t\tnumber of outstanding I/O operations per thread: 2\n"
            "\t\tIO priority: normal\n";
        VERIFY_ARE_EQUAL(parser._sResult, pszExpectedResult);

        parser._sResult.clear();
        target.SetMemoryMappedIoFlushMode(MemoryMappedIoFlushMode::NonVolatileMemoryNoDrain);
        parser._PrintTarget(target, false, true, false);
        pszExpectedResult = "\tpath: ''\n" \
            "\t\tthink time: 0ms\n"
            "\t\tburst size: 0\n"
            "\t\tusing software cache\n"
            "\t\tusing hardware write cache, writethrough off\n"
            "\t\tmemory mapped I/O enabled, flush mode: FlushNonVolatileMemory with no drain\n"
            "\t\tperforming read test\n"
            "\t\tblock size: 64KiB\n"
            "\t\tusing random I/O (alignment: 64KiB)\n"
            "\t\tnumber of outstanding I/O operations per thread: 2\n"
            "\t\tIO priority: normal\n";
        VERIFY_ARE_EQUAL(parser._sResult, pszExpectedResult);

        parser._sResult.clear();
        target.SetMemoryMappedIoMode(MemoryMappedIoMode::Off);
        target.SetMemoryMappedIoFlushMode(MemoryMappedIoFlushMode::Undefined);
        target.SetCacheMode(TargetCacheMode::DisableLocalCache);
        target.SetTemporaryFileHint(true);
        parser._PrintTarget(target, false, true, false);
        pszExpectedResult = "\tpath: ''\n" \
            "\t\tthink time: 0ms\n"
            "\t\tburst size: 0\n"
            "\t\tlocal software cache disabled, remote cache enabled\n"
            "\t\tusing hardware write cache, writethrough off\n"
            "\t\tperforming read test\n"
            "\t\tblock size: 64KiB\n"
            "\t\tusing random I/O (alignment: 64KiB)\n"
            "\t\tnumber of outstanding I/O operations per thread: 2\n"
            "\t\tusing FILE_ATTRIBUTE_TEMPORARY hint\n"
            "\t\tIO priority: normal\n";
        VERIFY_ARE_EQUAL(parser._sResult, pszExpectedResult);

        parser._sResult.clear();
        target.SetRandomAccessHint(true);
        parser._PrintTarget(target, false, true, false);
        pszExpectedResult = "\tpath: ''\n" \
            "\t\tthink time: 0ms\n"
            "\t\tburst size: 0\n"
            "\t\tlocal software cache disabled, remote cache enabled\n"
            "\t\tusing hardware write cache, writethrough off\n"
            "\t\tperforming read test\n"
            "\t\tblock size: 64KiB\n"
            "\t\tusing random I/O (alignment: 64KiB)\n"
            "\t\tnumber of outstanding I/O operations per thread: 2\n"
            "\t\tusing FILE_FLAG_RANDOM_ACCESS hint\n"
            "\t\tusing FILE_ATTRIBUTE_TEMPORARY hint\n"
            "\t\tIO priority: normal\n";
        VERIFY_ARE_EQUAL(parser._sResult, pszExpectedResult);

        parser._sResult.clear();
        target.SetRandomAccessHint(false);
        target.SetTemporaryFileHint(false);
        target.SetSequentialScanHint(true);
        parser._PrintTarget(target, false, true, false);
        pszExpectedResult = "\tpath: ''\n" \
            "\t\tthink time: 0ms\n"
            "\t\tburst size: 0\n"
            "\t\tlocal software cache disabled, remote cache enabled\n"
            "\t\tusing hardware write cache, writethrough off\n"
            "\t\tperforming read test\n"
            "\t\tblock size: 64KiB\n"
            "\t\tusing random I/O (alignment: 64KiB)\n"
            "\t\tnumber of outstanding I/O operations per thread: 2\n"
            "\t\tusing FILE_FLAG_SEQUENTIAL_SCAN hint\n"
            "\t\tIO priority: normal\n";
        VERIFY_ARE_EQUAL(parser._sResult, pszExpectedResult);
    }

    void ResultParserUnitTests::Test_PrintTargetDistributionPct()
    {
        ResultParser parser;
        Target target;

        vector<DistributionRange> v;

        // these match the CmdLineParser UTs

        // -rdpct10/10:10/10:0/10, though we need to produce the tail here
        v.emplace_back(0, 10, make_pair(0, 10));
        v.emplace_back(10, 10, make_pair(10, 10));
        v.emplace_back(20, 0, make_pair(20, 10));   // zero IO% length hole
        v.emplace_back(20, 80, make_pair(30, 70));
        target.SetDistributionRange(v, DistributionType::Percent);

        parser._PrintTarget(target, false, true, false);
        const char *pszExpectedResult = "\tpath: ''\n" \
            "\t\tthink time: 0ms\n"
            "\t\tburst size: 0\n"
            "\t\tusing software cache\n"
            "\t\tusing hardware write cache, writethrough off\n"
            "\t\tperforming read test\n"
            "\t\tblock size: 64KiB\n"
            "\t\tusing sequential I/O (stride: 64KiB)\n"
            "\t\tnumber of outstanding I/O operations per thread: 2\n"
            "\t\tIO priority: normal\n"
            "\t\tIO Distribution:\n"
            "\t\t    10% of IO => [ 0% -  10%) of target\n"
            "\t\t    10% of IO => [10% -  20%) of target\n"
            "\t\t     0% of IO => [20% -  30%) of target\n"
            "\t\t    80% of IO => [30% - 100%) of target\n";
        VERIFY_ARE_EQUAL(parser._sResult, pszExpectedResult);
        v.clear();
        parser._sResult.clear();
    }

    void ResultParserUnitTests::Test_PrintTargetDistributionAbs()
    {
        ResultParser parser;
        Target target;

        vector<DistributionRange> v;

        // these match the CmdLineParser UTs

        // -rdabs10/1G:10/1G:0/100G, again producing tail
        v.emplace_back(0,10, make_pair(0, 1*GB));
        v.emplace_back(10,10, make_pair(1*GB, 1*GB));
        v.emplace_back(20,0, make_pair(2*GB, 100*GB));
        v.emplace_back(20,80, make_pair(102*GB, 0));
        target.SetDistributionRange(v, DistributionType::Absolute);

        parser._PrintTarget(target, false, true, false);
        const char *pszExpectedResult = "\tpath: ''\n" \
            "\t\tthink time: 0ms\n"
            "\t\tburst size: 0\n"
            "\t\tusing software cache\n"
            "\t\tusing hardware write cache, writethrough off\n"
            "\t\tperforming read test\n"
            "\t\tblock size: 64KiB\n"
            "\t\tusing sequential I/O (stride: 64KiB)\n"
            "\t\tnumber of outstanding I/O operations per thread: 2\n"
            "\t\tIO priority: normal\n"
            "\t\tIO Distribution:\n"
            "\t\t    10% of IO => [     0    -      1GiB)\n"
            "\t\t    10% of IO => [     1GiB -      2GiB)\n"
            "\t\t     0% of IO => [     2GiB -    102GiB)\n"
            "\t\t    80% of IO => [   102GiB -       end)\n";
        VERIFY_ARE_EQUAL(parser._sResult, pszExpectedResult);
        v.clear();
        parser._sResult.clear();
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
            targetResults.vDistributionRange = tts._vDistributionRange;

            threadResults.vTargetResults.push_back(targetResults);
            results.vThreadResults.push_back(threadResults);

            parser._PrintEffectiveDistributions(results);

            const char* pcszResults = "\nEffective IO Distributions\n" \
                "--------------------------\n"
                "target: testfile.dat [thread: 0]\n"
                "    10% of IO => [     0    -      8KiB)\n"
                "    10% of IO => [     8KiB -     20KiB)\n"
                "    80% of IO => [    28KiB -    100KiB)\n";
            VERIFY_ARE_EQUAL(pcszResults, parser._sResult);

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
            targetResults.vDistributionRange = tts._vDistributionRange;

            threadResults.vTargetResults.push_back(targetResults);
            results.vThreadResults.push_back(threadResults);
            results.vThreadResults.push_back(threadResults);

            parser._PrintEffectiveDistributions(results);

            const char* pcszResults = "\nEffective IO Distributions\n" \
                "--------------------------\n"
                "target: testfile.dat [thread: 0 1]\n"
                "    10% of IO => [     0    -      8KiB)\n"
                "    10% of IO => [     8KiB -     20KiB)\n"
                "    80% of IO => [    28KiB -    100KiB)\n";
            VERIFY_ARE_EQUAL(pcszResults, parser._sResult);

            parser._sResult.clear();
        }

        // now repeat, for a third thread - ellision

        {
            ThreadTargetState tts(&tp, 0, 100*KB);

            Results results;
            ThreadResults threadResults;
            TargetResults targetResults;

            targetResults.sPath = "testfile.dat";
            targetResults.vDistributionRange = tts._vDistributionRange;

            threadResults.vTargetResults.push_back(targetResults);
            results.vThreadResults.push_back(threadResults);
            results.vThreadResults.push_back(threadResults);
            results.vThreadResults.push_back(threadResults);

            parser._PrintEffectiveDistributions(results);

            const char* pcszResults = "\nEffective IO Distributions\n" \
                "--------------------------\n"
                "target: testfile.dat [thread: 0 - 2]\n"
                "    10% of IO => [     0    -      8KiB)\n"
                "    10% of IO => [     8KiB -     20KiB)\n"
                "    80% of IO => [    28KiB -    100KiB)\n";
            VERIFY_ARE_EQUAL(pcszResults, parser._sResult);

            parser._sResult.clear();
        }

        // now repeat, moving the third thread to a different target

        {
            ThreadTargetState tts(&tp, 0, 100*KB);

            Results results;
            ThreadResults threadResults;
            TargetResults targetResults;

            targetResults.vDistributionRange = tts._vDistributionRange;

            targetResults.sPath = "testfile.dat";
            threadResults.vTargetResults.push_back(targetResults);
            results.vThreadResults.push_back(threadResults);
            results.vThreadResults.push_back(threadResults);

            targetResults.sPath = "testfile2.dat";
            threadResults.vTargetResults.clear();
            threadResults.vTargetResults.push_back(targetResults);
            results.vThreadResults.push_back(threadResults);

            parser._PrintEffectiveDistributions(results);

            const char* pcszResults = "\nEffective IO Distributions\n" \
                "--------------------------\n"
                "target: testfile.dat [thread: 0 1]\n"
                "target: testfile2.dat [thread: 2]\n"
                "    10% of IO => [     0    -      8KiB)\n"
                "    10% of IO => [     8KiB -     20KiB)\n"
                "    80% of IO => [    28KiB -    100KiB)\n";
            VERIFY_ARE_EQUAL(pcszResults, parser._sResult);

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

            targetResults.vDistributionRange = tts._vDistributionRange;

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

            const char* pcszResults = "\nEffective IO Distributions\n" \
                "--------------------------\n"
                "target: testfile.dat [thread: 0 - 2 4]\n"
                "target: testfile2.dat [thread: 3]\n"
                "    10% of IO => [     0    -      8KiB)\n"
                "    10% of IO => [     8KiB -     20KiB)\n"
                "    80% of IO => [    28KiB -    100KiB)\n";
            VERIFY_ARE_EQUAL(pcszResults, parser._sResult);

            parser._sResult.clear();
        }

        // two distinct distributions which share the same IO%

        {
            ThreadTargetState tts1(&tp, 0, 100*KB);
            ThreadTargetState tts2(&tp, 0, 1*MB);

            Results results;
            ThreadResults threadResults;
            TargetResults targetResults;

            targetResults.vDistributionRange = tts1._vDistributionRange;
            targetResults.sPath = "testfile.dat";
            threadResults.vTargetResults.push_back(targetResults);
            results.vThreadResults.push_back(threadResults);

            threadResults.vTargetResults.clear();

            targetResults.vDistributionRange = tts2._vDistributionRange;
            targetResults.sPath = "testfile2.dat";
            threadResults.vTargetResults.push_back(targetResults);
            results.vThreadResults.push_back(threadResults);

            parser._PrintEffectiveDistributions(results);

            const char* pcszResults = "\nEffective IO Distributions\n" \
                "--------------------------\n"
                "target: testfile.dat [thread: 0]\n"
                "    10% of IO => [     0    -      8KiB)\n"
                "    10% of IO => [     8KiB -     20KiB)\n"
                "    80% of IO => [    28KiB -    100KiB)\n"
                "target: testfile2.dat [thread: 1]\n"
                "    10% of IO => [     0    -    100KiB)\n"
                "    10% of IO => [   100KiB -    204KiB)\n"
                "    80% of IO => [   304KiB -      1MiB)\n";
            VERIFY_ARE_EQUAL(pcszResults, parser._sResult);

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
            targetResults.vDistributionRange = tts._vDistributionRange;

            threadResults.vTargetResults.push_back(targetResults);
            results.vThreadResults.push_back(threadResults);

            parser._PrintEffectiveDistributions(results);

            const char* pcszResults = "\nEffective IO Distributions\n" \
                "--------------------------\n"
                "target: testfile.dat [thread: 0]\n"
                "    10% of IO => [     0    -      1GiB)\n"
                "    10% of IO => [     1GiB -      2GiB)\n"
                "    80% of IO => [   102GiB -    200GiB)\n";
            VERIFY_ARE_EQUAL(pcszResults, parser._sResult);

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
            targetResults.vDistributionRange = tts._vDistributionRange;

            threadResults.vTargetResults.push_back(targetResults);
            results.vThreadResults.push_back(threadResults);

            parser._PrintEffectiveDistributions(results);

            const char* pcszResults = "\nEffective IO Distributions\n" \
                "--------------------------\n"
                "target: testfile.dat [thread: 0]\n"
                "    16.7% of IO => [     0    -     50KiB)\n"
                "    33.3% of IO => [    50KiB -     60KiB)\n"
                "    50.0% of IO => [    60KiB -    100KiB)\n";
            VERIFY_ARE_EQUAL(pcszResults, parser._sResult);

            tp.vTargets.clear();
            v.clear();
            parser._sResult.clear();
        }
    }
}