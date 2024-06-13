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
#include "WexTestClass.h"

namespace UnitTests
{
    BEGIN_MODULE()
        MODULE_PROPERTY(L"Feature", L"CmdLineParser")
    END_MODULE()

    MODULE_SETUP(ModuleSetup);
    MODULE_CLEANUP(ModuleCleanup);

    class CmdLineParserUnitTests : public WEX::TestClass<CmdLineParserUnitTests>
    {
    private:
        void VerifyParseCmdLineDisableAllCache(Profile &profile);
        void VerifyParseCmdLineMappedIO(Profile &profile, MemoryMappedIoFlushMode FlushMode);
        void VerifyParseCmdLineAccessHints(Profile &profile, bool RandomAccess, bool SequentialScan, bool TemporaryFile);

    public:
        TEST_CLASS(CmdLineParserUnitTests)

        TEST_CLASS_SETUP(ClassSetup);
        TEST_CLASS_CLEANUP(ClassCleanup);

        TEST_METHOD_SETUP(MethodSetup);
        TEST_METHOD_CLEANUP(MethodCleanup);

        TEST_METHOD(Test_GetSizeInBytes);
        TEST_METHOD(TestGetRandomDataWriteBufferData);
        TEST_METHOD(TestParseCmdLine);
        TEST_METHOD(TestParseCmdLineAssignAffinity);
        TEST_METHOD(TestParseCmdLineBlockSize);
        TEST_METHOD(TestParseCmdLineBufferedWriteThrough);
        TEST_METHOD(TestParseCmdLineBurstSizeAndThinkTime);
        TEST_METHOD(TestParseCmdLineConflictingCacheModes);
        TEST_METHOD(TestParseCmdLineCreateFileAndMaxFileSize);
        TEST_METHOD(TestParseCmdLineDisableAffinity);
        TEST_METHOD(TestParseCmdLineDisableAffinityConflict);
        TEST_METHOD(TestParseCmdLineDisableAllCacheMode1);
        TEST_METHOD(TestParseCmdLineDisableAllCacheMode2);
        TEST_METHOD(TestParseCmdLineDisableLocalCache);
        TEST_METHOD(TestParseCmdLineDisableOSCache);
        TEST_METHOD(TestParseCmdLineDurationAndProgress);
        TEST_METHOD(TestParseCmdLineEtwDISK_IO);
        TEST_METHOD(TestParseCmdLineEtwIMAGE_LOAD);
        TEST_METHOD(TestParseCmdLineEtwMEMORY_HARD_FAULTS);
        TEST_METHOD(TestParseCmdLineEtwMEMORY_PAGE_FAULTS);
        TEST_METHOD(TestParseCmdLineEtwNETWORK);
        TEST_METHOD(TestParseCmdLineEtwPROCESS);
        TEST_METHOD(TestParseCmdLineEtwREGISTRY);
        TEST_METHOD(TestParseCmdLineEtwTHREAD);
        TEST_METHOD(TestParseCmdLineEtwUseCycleCount);
        TEST_METHOD(TestParseCmdLineEtwUsePagedMemory);
        TEST_METHOD(TestParseCmdLineEtwUsePerfTimer);
        TEST_METHOD(TestParseCmdLineEtwUseSystemTimer);
        TEST_METHOD(TestParseCmdLineGroupAffinity);
        TEST_METHOD(TestParseCmdLineHintFlag);
        TEST_METHOD(TestParseCmdLineBaseMaxTarget);
        TEST_METHOD(TestParseCmdLineInterlockedSequential);
        TEST_METHOD(TestParseCmdLineInterlockedSequentialWithStride);
        TEST_METHOD(TestParseCmdLineIOPriority);
        TEST_METHOD(TestParseCmdLineMappedIO);
        TEST_METHOD(TestParseCmdLineMeasureLatency);
        TEST_METHOD(TestParseCmdLineOverlappedCountAndBaseOffset);
        TEST_METHOD(TestParseCmdLineRandomIOAlignment);
        TEST_METHOD(TestParseCmdLineRandomSequentialMixed);
        TEST_METHOD(TestParseCmdLineRandomWriteBuffers);
        TEST_METHOD(TestParseCmdLineRandSeed);
        TEST_METHOD(TestParseCmdLineRandSeedGetTickCount);
        TEST_METHOD(TestParseCmdLineResultOutput);
        TEST_METHOD(TestParseCmdLineStrideSize);
        TEST_METHOD(TestParseCmdLineTargetDistribution);
        TEST_METHOD(TestParseCmdLineTargetPosition);
        TEST_METHOD(TestParseCmdLineThreadsPerFileAndThreadStride);
        TEST_METHOD(TestParseCmdLineThroughput);
        TEST_METHOD(TestParseCmdLineTotalThreadCountAndThroughput);
        TEST_METHOD(TestParseCmdLineTotalThreadCountAndTotalRequestCount);
        TEST_METHOD(TestParseCmdLineUseCompletionRoutines);
        TEST_METHOD(TestParseCmdLineUseLargePages);
        TEST_METHOD(TestParseCmdLineUseParallelAsyncIO);
        TEST_METHOD(TestParseCmdLineVerbose);
        TEST_METHOD(TestParseCmdLineWarmupAndCooldown);
        TEST_METHOD(TestParseCmdLineWriteBufferContentRandomNoFilePath);
        TEST_METHOD(TestParseCmdLineWriteBufferContentRandomWithFilePath);
        TEST_METHOD(TestParseCmdLineZeroWriteBuffers);
    };
}
