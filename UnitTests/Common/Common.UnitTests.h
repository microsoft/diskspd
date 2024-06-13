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
        MODULE_PROPERTY(L"Feature", L"Common")
    END_MODULE()

    class PerfTimerUnitTests : public WEX::TestClass<PerfTimerUnitTests>
    {
    public:
        TEST_CLASS(PerfTimerUnitTests);
        TEST_METHOD(Test_Freq);
        TEST_METHOD(Test_GetTime);
        TEST_METHOD(Test_PerfTimeToSeconds);
        TEST_METHOD(Test_PerfTimeToMilliseconds);
        TEST_METHOD(Test_PerfTimeToMicroseconds);
        TEST_METHOD(Test_SecondsToPerfTime);
        TEST_METHOD(Test_MillisecondsToPerfTime);
        TEST_METHOD(Test_MicrosecondsToPerfTime);
    };

    class HistogramUnitTests : public WEX::TestClass<HistogramUnitTests>
    {
    public:
        TEST_CLASS(HistogramUnitTests);
        TEST_METHOD(Test_Empty);
        TEST_METHOD(Test_Add);
        TEST_METHOD(Test_Clear);
        TEST_METHOD(Test_MinMax);
        TEST_METHOD(Test_GetPercentile);
        TEST_METHOD(Test_GetMean);
        TEST_METHOD(Test_Merge);
    };

    class IoBucketizerUnitTests :  public WEX::TestClass<IoBucketizerUnitTests>
    {
    public:
        TEST_CLASS(IoBucketizerUnitTests);
        TEST_METHOD(Test_Empty);
        TEST_METHOD(Test_Add);
        TEST_METHOD(Test_Merge);
        TEST_METHOD(Test_GetStandardDeviation);
    };

    class ProfileUnitTests : public WEX::TestClass<ProfileUnitTests>
    {
    public:
        TEST_CLASS(ProfileUnitTests);
        TEST_METHOD(Test_GetXmlEmptyProfile);
        TEST_METHOD(Test_GetXmlPrecreateFilesUseMaxSize);
        TEST_METHOD(Test_GetXmlPrecreateFilesOnlyFilesWithConstantSizes);
        TEST_METHOD(Test_GetXmlPrecreateFilesOnlyFilesWithConstantOrZeroSizes);
        TEST_METHOD(Test_MarkFilesAsCreated);
        TEST_METHOD(Test_Validate);
        TEST_METHOD(Test_ValidateSystem);
    };

    class TargetUnitTests : public WEX::TestClass<TargetUnitTests>
    {
    public:
        TEST_CLASS(TargetUnitTests);
        TEST_METHOD(TestGetSetRandomDataWriteBufferSize);
        TEST_METHOD(TestGetSetRandomDataWriteBufferSourcePath);
        TEST_METHOD(Test_TargetGetXmlWriteBufferContentSequential);
        TEST_METHOD(Test_TargetGetXmlWriteBufferContentZero);
        TEST_METHOD(Test_TargetGetXmlWriteBufferContentRandomNoFilePath);
        TEST_METHOD(Test_TargetGetXmlWriteBufferContentRandomWithFilePath);
        TEST_METHOD(Test_TargetGetXmlDisableAllCache);
        TEST_METHOD(Test_TargetGetXmlDisableLocalCache);
        TEST_METHOD(Test_TargetGetXmlDisableOSCache);
        TEST_METHOD(Test_TargetGetXmlBufferedWriteThrough);
        TEST_METHOD(Test_TargetGetXmlMemoryMappedIo);
        TEST_METHOD(Test_TargetGetXmlMemoryMappedIoFlushModeViewOfFile);
        TEST_METHOD(Test_TargetGetXmlMemoryMappedIoFlushModeNonVolatileMemory);
        TEST_METHOD(Test_TargetGetXmlMemoryMappedIoFlushModeNonVolatileMemoryNoDrain);
        TEST_METHOD(Test_TargetGetXmlRandomAccessHint);
        TEST_METHOD(Test_TargetGetXmlSequentialScanHint);
        TEST_METHOD(Test_TargetGetXmlCombinedAccessHint);
        TEST_METHOD(Test_AllocateAndFillRandomDataWriteBuffer);
        TEST_METHOD(Test_AllocateAndFillRandomDataWriteBufferFromFile);
    };

    class ThreadParametersUnitTests : public WEX::TestClass<ThreadParametersUnitTests>
    {
    public:
        TEST_CLASS(ThreadParametersUnitTests);
        TEST_METHOD(Test_AllocateAndFillBufferForTarget);
    };

    class TopologyUnitTests : public WEX::TestClass<TopologyUnitTests>
    {
    public:
        TEST_CLASS(TopologyUnitTests);
        TEST_METHOD(Test_MaskCount);
    };
}

// TODO: ThreadParameters::GetWriteBuffer
// TODO: Target::GetRandomDataWriteBuffer();
