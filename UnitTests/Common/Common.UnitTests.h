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

    MODULE_SETUP(ModuleSetup);

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
        TEST_METHOD(Test_FinalizeAffinity_CoreAware);
        TEST_METHOD(Test_ValidateBypassIoConflict);
    };

    class TimeSpanUnitTests : public WEX::TestClass<TimeSpanUnitTests>
    {
    public:
        TEST_CLASS(TimeSpanUnitTests);
        TEST_METHOD(Test_TimeSpanGetXmlUseIoRing);
        TEST_METHOD(Test_TimeSpanGetXmlUseIoRingWithBatchSizeAndRegBuffer);
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
        TEST_METHOD(Test_TargetGetXmlBypassIoModePartial);
        TEST_METHOD(Test_TargetGetXmlBypassIoModeFull);
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
        TEST_METHOD(Test_AllocateAndFillBufferForTarget_Aligned);
        TEST_METHOD(Test_AllocateAndFillBufferForTarget_WriteSourceNoSeparation);
        TEST_METHOD(Test_AllocateAndFillBufferForTarget_WriteSourceSeparation);
    };

    class TopologyUnitTests : public WEX::TestClass<TopologyUnitTests>
    {
    public:
        TEST_CLASS(TopologyUnitTests);
        TEST_METHOD(Test_MaskCount);
    };

    class ProcessorTopologyUnitTests : public WEX::TestClass<ProcessorTopologyUnitTests>
    {
    public:
        TEST_CLASS(ProcessorTopologyUnitTests);
        TEST_METHOD(Test_GetLargestCacheLineSize_SingleL3);
        TEST_METHOD(Test_GetLargestCacheLineSize_MultipleL3);
        TEST_METHOD(Test_GetLargestCacheLineSize_NoL3);
        TEST_METHOD(Test_GetLargestCacheLineSize_AllLevels);
        TEST_METHOD(Test_GetLargestCacheLineSize_SpecificLevel);
        TEST_METHOD(Test_GetXml_CacheTopology);
        TEST_METHOD(Test_GetCacheText);
        TEST_METHOD(Test_GetCacheTextMultiGroup);
        TEST_METHOD(Test_CacheTypeName);
        TEST_METHOD(Test_CacheTypeAbbreviation);
        TEST_METHOD(Test_GetText_SectionAll);
        TEST_METHOD(Test_GetText_SectionTopology);
        TEST_METHOD(Test_GetXml_SectionAll);
        TEST_METHOD(Test_GetXml_SectionTopology);
        TEST_METHOD(Test_GetCacheText_BigSystem);
        TEST_METHOD(Test_GetCacheText_SmallHeteroSystem);
        TEST_METHOD(Test_GetCacheText_NoCompaction);
        TEST_METHOD(Test_GetCacheText_MultiMaskPlain);
        TEST_METHOD(Test_GetCacheText_SingleGroupSkipsGroupCase);
        TEST_METHOD(Test_GroupMaskRanges);
        TEST_METHOD(Test_SameGeometry);
    };

    class UtilUnitTests : public WEX::TestClass<UtilUnitTests>
    {
    public:
        TEST_CLASS(UtilUnitTests);
        TEST_METHOD(Test_GetSizeKMGT);
        TEST_METHOD(Test_GetBufferAlignmentSize);
        TEST_METHOD(Test_MaskRanges);
        TEST_METHOD(Test_IntRanges);
        TEST_METHOD(Test_ShrinkContiguousWhitespace);
    };

    class DistributionUnitTests : public WEX::TestClass<DistributionUnitTests>
    {
    public:
        TEST_CLASS(DistributionUnitTests);
        TEST_METHOD(Test_SetPercent);
        TEST_METHOD(Test_SetAbsolute);
        TEST_METHOD(Test_SetPercentFullIO);
        TEST_METHOD(Test_SetPercentTargetFullBeforeIO);
        TEST_METHOD(Test_ValidatePercentValid);
        TEST_METHOD(Test_ValidateAbsoluteValid);
        TEST_METHOD(Test_ValidatePercentIOOverflow);
        TEST_METHOD(Test_ValidatePercentTargetOverflow);
        TEST_METHOD(Test_ValidateAbsoluteRangeTooSmall);
        TEST_METHOD(Test_ValidatePercentTargetCoveredBeforeIO);
        TEST_METHOD(Test_FinalizePercent);
        TEST_METHOD(Test_FinalizePercentDegenerate);
        TEST_METHOD(Test_FinalizeAbsolute);
        TEST_METHOD(Test_FinalizeAbsoluteTrimmed);
        TEST_METHOD(Test_FinalizeTypeSetsAbsolute);
        TEST_METHOD(Test_GetTextPercent);
        TEST_METHOD(Test_GetTextAbsolute);
        TEST_METHOD(Test_GetXml);
        TEST_METHOD(Test_GetXmlWithHoles);
        TEST_METHOD(Test_EmptyDistribution);
    };

    class TextDiffUnitTests : public WEX::TestClass<TextDiffUnitTests>
    {
    public:
        TEST_CLASS(TextDiffUnitTests);
        TEST_METHOD(Test_IdenticalStrings);
        TEST_METHOD(Test_IdenticalStringsWithTrailingNewlines);
        TEST_METHOD(Test_EmptyStrings);
        TEST_METHOD(Test_DifferentFirstLine);
        TEST_METHOD(Test_DifferentMiddleLine);
        TEST_METHOD(Test_ExpectedLonger);
        TEST_METHOD(Test_ActualLonger);
        TEST_METHOD(Test_SingleLineDifference);
        TEST_METHOD(Test_TrailingNewline);
        TEST_METHOD(Test_CaseSensitive);
        TEST_METHOD(Test_VerifyMultilineEqualCharPointers);
    };
}

// TODO: ThreadParameters::GetWriteBuffer
// TODO: Target::GetRandomDataWriteBuffer();

