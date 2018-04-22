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
        system.processorTopology._ulProcCount = 1;
        system.processorTopology._ulActiveProcCount = 1;
        system.processorTopology._vProcessorGroupInformation[0]._maximumProcessorCount = 1;
        system.processorTopology._vProcessorGroupInformation[0]._activeProcessorCount = 1;

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
            "\n"
            "\n"
            "Results for timespan 1:\n"
            "*******************************************************************************\n"
            "\n"
            "actual test time:\t120.00s\n"
            "thread count:\t\t1\n"
            "proc count:\t\t1\n"
            "\n"
            "CPU |  Usage |  User  |  Kernel |  Idle\n"
            "-------------------------------------------\n"
            "   0|  55.00%|  30.00%|   25.00%|  45.00%\n"
            "-------------------------------------------\n"
            "avg.|  55.00%|  30.00%|   25.00%|  45.00%\n"
            "\n"
            "Total IO\n"
            "thread |       bytes     |     I/Os     |    MiB/s   |  I/O per s | IopsStdDev |  file\n"
            "-------------------------------------------------------------------------------------------\n"
            "     0 |         6291456 |           16 |       0.05 |       0.13 |       0.00 | testfile1.dat (10240KiB)\n"
            "-------------------------------------------------------------------------------------------\n"
            "total:           6291456 |           16 |       0.05 |       0.13 |       0.00\n"
            "\n"
            "Read IO\n"
            "thread |       bytes     |     I/Os     |    MiB/s   |  I/O per s | IopsStdDev |  file\n"
            "-------------------------------------------------------------------------------------------\n"
            "     0 |         4194304 |            6 |       0.03 |       0.05 |       0.00 | testfile1.dat (10240KiB)\n"
            "-------------------------------------------------------------------------------------------\n"
            "total:           4194304 |            6 |       0.03 |       0.05 |       0.00\n"
            "\n"
            "Write IO\n"
            "thread |       bytes     |     I/Os     |    MiB/s   |  I/O per s | IopsStdDev |  file\n"
            "-------------------------------------------------------------------------------------------\n"
            "     0 |         2097152 |           10 |       0.02 |       0.08 |       0.00 | testfile1.dat (10240KiB)\n"
            "-------------------------------------------------------------------------------------------\n"
            "total:           2097152 |           10 |       0.02 |       0.08 |       0.00\n";

        VERIFY_ARE_EQUAL(sResults.length(), strlen(pcszExpectedOutput));
        VERIFY_ARE_EQUAL(memcmp(sResults.c_str(), pcszExpectedOutput, sResults.length()), 0);
        VERIFY_IS_TRUE(strcmp(sResults.c_str(), pcszExpectedOutput) == 0);
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
            "\t\tblock size: 65536\n"
            "\t\tusing sequential I/O (stride: 65536)\n"
            "\t\tnumber of outstanding I/O operations: 2\n"
            "\t\tthread stride size: 0\n"
            "\t\tIO priority: normal\n";
        VERIFY_IS_TRUE(parser._sResult == pszExpectedResult);

        parser._sResult = "";
        target.SetWriteRatio(30);
        parser._PrintTarget(target, false, true, false);
        pszExpectedResult = "\tpath: ''\n" \
            "\t\tthink time: 0ms\n"
            "\t\tburst size: 0\n"
            "\t\tusing software cache\n"
            "\t\tusing hardware write cache, writethrough off\n"
            "\t\tperforming mix test (read/write ratio: 70/30)\n"
            "\t\tblock size: 65536\n"
            "\t\tusing sequential I/O (stride: 65536)\n"
            "\t\tnumber of outstanding I/O operations: 2\n"
            "\t\tthread stride size: 0\n"
            "\t\tIO priority: normal\n";
        VERIFY_IS_TRUE(parser._sResult == pszExpectedResult);
        target.SetWriteRatio(0);

        parser._sResult = "";
        target.SetRandomDataWriteBufferSize(12341234);
        parser._PrintTarget(target, false, true, false);
        pszExpectedResult = "\tpath: ''\n" \
            "\t\tthink time: 0ms\n"
            "\t\tburst size: 0\n"
            "\t\tusing software cache\n"
            "\t\tusing hardware write cache, writethrough off\n"
            "\t\twrite buffer size: 12341234\n"
            "\t\tperforming read test\n"
            "\t\tblock size: 65536\n"
            "\t\tusing sequential I/O (stride: 65536)\n"
            "\t\tnumber of outstanding I/O operations: 2\n"
            "\t\tthread stride size: 0\n"
            "\t\tIO priority: normal\n";
        VERIFY_IS_TRUE(parser._sResult == pszExpectedResult);

        parser._sResult = "";
        target.SetRandomDataWriteBufferSourcePath("x:\\foo\\bar.dat");
        target.SetUseRandomAccessPattern(true);
        parser._PrintTarget(target, false, true, false);
        pszExpectedResult = "\tpath: ''\n" \
            "\t\tthink time: 0ms\n"
            "\t\tburst size: 0\n"
            "\t\tusing software cache\n"
            "\t\tusing hardware write cache, writethrough off\n"
            "\t\twrite buffer size: 12341234\n"
            "\t\twrite buffer source: 'x:\\foo\\bar.dat'\n"
            "\t\tperforming read test\n"
            "\t\tblock size: 65536\n"
            "\t\tusing random I/O (alignment: 65536)\n"
            "\t\tnumber of outstanding I/O operations: 2\n"
            "\t\tthread stride size: 0\n"
            "\t\tIO priority: normal\n";
        VERIFY_IS_TRUE(parser._sResult == pszExpectedResult);

        parser._sResult = "";
        target.SetCacheMode(TargetCacheMode::DisableOSCache);
        parser._PrintTarget(target, false, true, false);
        pszExpectedResult = "\tpath: ''\n" \
            "\t\tthink time: 0ms\n"
            "\t\tburst size: 0\n"
            "\t\tsoftware cache disabled\n"
            "\t\tusing hardware write cache, writethrough off\n"
            "\t\twrite buffer size: 12341234\n"
            "\t\twrite buffer source: 'x:\\foo\\bar.dat'\n"
            "\t\tperforming read test\n"
            "\t\tblock size: 65536\n"
            "\t\tusing random I/O (alignment: 65536)\n"
            "\t\tnumber of outstanding I/O operations: 2\n"
            "\t\tthread stride size: 0\n"
            "\t\tIO priority: normal\n";
        VERIFY_IS_TRUE(parser._sResult == pszExpectedResult);

        parser._sResult = "";
        target.SetCacheMode(TargetCacheMode::DisableOSCache);
        target.SetWriteThroughMode(WriteThroughMode::On);
        parser._PrintTarget(target, false, true, false);
        pszExpectedResult = "\tpath: ''\n" \
            "\t\tthink time: 0ms\n"
            "\t\tburst size: 0\n"
            "\t\tsoftware cache disabled\n"
            "\t\thardware write cache disabled, writethrough on\n"
            "\t\twrite buffer size: 12341234\n"
            "\t\twrite buffer source: 'x:\\foo\\bar.dat'\n"
            "\t\tperforming read test\n"
            "\t\tblock size: 65536\n"
            "\t\tusing random I/O (alignment: 65536)\n"
            "\t\tnumber of outstanding I/O operations: 2\n"
            "\t\tthread stride size: 0\n"
            "\t\tIO priority: normal\n";
        VERIFY_IS_TRUE(parser._sResult == pszExpectedResult);

        parser._sResult = "";
        target.SetCacheMode(TargetCacheMode::Cached);
        target.SetWriteThroughMode(WriteThroughMode::On);
        parser._PrintTarget(target, false, true, false);
        pszExpectedResult = "\tpath: ''\n" \
            "\t\tthink time: 0ms\n"
            "\t\tburst size: 0\n"
            "\t\tusing software cache\n"
            "\t\thardware and software write caches disabled, writethrough on\n"
            "\t\twrite buffer size: 12341234\n"
            "\t\twrite buffer source: 'x:\\foo\\bar.dat'\n"
            "\t\tperforming read test\n"
            "\t\tblock size: 65536\n"
            "\t\tusing random I/O (alignment: 65536)\n"
            "\t\tnumber of outstanding I/O operations: 2\n"
            "\t\tthread stride size: 0\n"
            "\t\tIO priority: normal\n";
        VERIFY_IS_TRUE(parser._sResult == pszExpectedResult);
        target.SetWriteThroughMode(WriteThroughMode::Undefined);

        parser._sResult = "";
        target.SetCacheMode(TargetCacheMode::DisableLocalCache);
        parser._PrintTarget(target, false, true, false);
        pszExpectedResult = "\tpath: ''\n" \
            "\t\tthink time: 0ms\n"
            "\t\tburst size: 0\n"
            "\t\tlocal software cache disabled, remote cache enabled\n"
            "\t\tusing hardware write cache, writethrough off\n"
            "\t\twrite buffer size: 12341234\n"
            "\t\twrite buffer source: 'x:\\foo\\bar.dat'\n"
            "\t\tperforming read test\n"
            "\t\tblock size: 65536\n"
            "\t\tusing random I/O (alignment: 65536)\n"
            "\t\tnumber of outstanding I/O operations: 2\n"
            "\t\tthread stride size: 0\n"
            "\t\tIO priority: normal\n";
        VERIFY_IS_TRUE(parser._sResult == pszExpectedResult);

        parser._sResult = "";
        target.SetTemporaryFileHint(true);
        parser._PrintTarget(target, false, true, false);
        pszExpectedResult = "\tpath: ''\n" \
            "\t\tthink time: 0ms\n"
            "\t\tburst size: 0\n"
            "\t\tlocal software cache disabled, remote cache enabled\n"
            "\t\tusing hardware write cache, writethrough off\n"
            "\t\twrite buffer size: 12341234\n"
            "\t\twrite buffer source: 'x:\\foo\\bar.dat'\n"
            "\t\tperforming read test\n"
            "\t\tblock size: 65536\n"
            "\t\tusing random I/O (alignment: 65536)\n"
            "\t\tnumber of outstanding I/O operations: 2\n"
            "\t\tthread stride size: 0\n"
            "\t\tusing FILE_ATTRIBUTE_TEMPORARY hint\n"
            "\t\tIO priority: normal\n";
        VERIFY_IS_TRUE(parser._sResult == pszExpectedResult);

        parser._sResult = "";
        target.SetRandomAccessHint(true);
        parser._PrintTarget(target, false, true, false);
        pszExpectedResult = "\tpath: ''\n" \
            "\t\tthink time: 0ms\n"
            "\t\tburst size: 0\n"
            "\t\tlocal software cache disabled, remote cache enabled\n"
            "\t\tusing hardware write cache, writethrough off\n"
            "\t\twrite buffer size: 12341234\n"
            "\t\twrite buffer source: 'x:\\foo\\bar.dat'\n"
            "\t\tperforming read test\n"
            "\t\tblock size: 65536\n"
            "\t\tusing random I/O (alignment: 65536)\n"
            "\t\tnumber of outstanding I/O operations: 2\n"
            "\t\tthread stride size: 0\n"
            "\t\tusing FILE_FLAG_RANDOM_ACCESS hint\n"
            "\t\tusing FILE_ATTRIBUTE_TEMPORARY hint\n"
            "\t\tIO priority: normal\n";
        VERIFY_IS_TRUE(parser._sResult == pszExpectedResult);

        parser._sResult = "";
        target.SetRandomAccessHint(false);
        target.SetTemporaryFileHint(false);
        target.SetSequentialScanHint(true);
        parser._PrintTarget(target, false, true, false);
        pszExpectedResult = "\tpath: ''\n" \
            "\t\tthink time: 0ms\n"
            "\t\tburst size: 0\n"
            "\t\tlocal software cache disabled, remote cache enabled\n"
            "\t\tusing hardware write cache, writethrough off\n"
            "\t\twrite buffer size: 12341234\n"
            "\t\twrite buffer source: 'x:\\foo\\bar.dat'\n"
            "\t\tperforming read test\n"
            "\t\tblock size: 65536\n"
            "\t\tusing random I/O (alignment: 65536)\n"
            "\t\tnumber of outstanding I/O operations: 2\n"
            "\t\tthread stride size: 0\n"
            "\t\tusing FILE_FLAG_SEQUENTIAL_SCAN hint\n"
            "\t\tIO priority: normal\n";
        VERIFY_IS_TRUE(parser._sResult == pszExpectedResult);
    }
}
