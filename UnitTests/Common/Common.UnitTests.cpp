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
#include "Common.UnitTests.h"
#include "Common.h"
#include "TextDiff.h"
#include <stdlib.h>

using namespace WEX::TestExecution;
using namespace WEX::Logging;

namespace UnitTests
{
    bool ModuleSetup()
    {
        Diagnostics::Initialize();
        return true;
    }

    void PerfTimerUnitTests::Test_Freq()
    {
        VERIFY_IS_TRUE(PerfTimer::TIMER_FREQ > 0);
    }

    void PerfTimerUnitTests::Test_GetTime()
    {
        VERIFY_IS_TRUE(PerfTimer::GetTime() > 0);
    }

    void PerfTimerUnitTests::Test_PerfTimeToSeconds()
    {
        double d = PerfTimer::PerfTimeToSeconds(PerfTimer::TIMER_FREQ);
        printf("tos %f %a ==? %f %a\n", d, d, 1.0, 1.0);
        VERIFY_IS_TRUE(d == 1.0);
    }

    void PerfTimerUnitTests::Test_PerfTimeToMilliseconds()
    {
        double d = PerfTimer::PerfTimeToMilliseconds(PerfTimer::TIMER_FREQ);
        printf("toms %f %a ==? %f %a\n", d, d, 1000.0, 1000.0);
        VERIFY_IS_TRUE(d == 1000.0);
    }

    void PerfTimerUnitTests::Test_PerfTimeToMicroseconds()
    {
        double d = PerfTimer::PerfTimeToMicroseconds(PerfTimer::TIMER_FREQ);
        printf("tous %f %a ==? %f %a\n", d, d, 1000000.0, 1000000.0);
        VERIFY_IS_TRUE(d == 1000000.0);
    }

    void PerfTimerUnitTests::Test_SecondsToPerfTime()
    {
        UINT64 u = PerfTimer::SecondsToPerfTime(1.0);
        VERIFY_IS_TRUE(u == PerfTimer::TIMER_FREQ);
    }

    void PerfTimerUnitTests::Test_MillisecondsToPerfTime()
    {
        UINT64 u = PerfTimer::MillisecondsToPerfTime(1000.0);
        VERIFY_IS_TRUE(u == PerfTimer::TIMER_FREQ);
    }

    void PerfTimerUnitTests::Test_MicrosecondsToPerfTime()
    {
        UINT64 u = PerfTimer::MicrosecondsToPerfTime(1000000.0);
        VERIFY_IS_TRUE(u == PerfTimer::TIMER_FREQ);
    }

    void HistogramUnitTests::Test_Empty()
    {
        Histogram<int> h;
        VERIFY_ARE_EQUAL(h.GetSampleSize(), (unsigned)0);
    }

    void HistogramUnitTests::Test_Add()
    {
        Histogram<int> h;
        h.Add(42);
        VERIFY_ARE_EQUAL(h.GetSampleSize(), (unsigned)1);
        VERIFY_ARE_EQUAL(h.GetSampleBuckets(), (unsigned)1);

        h.Add(42);
        VERIFY_ARE_EQUAL(h.GetSampleSize(), (unsigned)2);
        VERIFY_ARE_EQUAL(h.GetSampleBuckets(), (unsigned)1);

        h.Add(0);
        VERIFY_ARE_EQUAL(h.GetSampleSize(), (unsigned)3);
        VERIFY_ARE_EQUAL(h.GetSampleBuckets(), (unsigned)2);

        // seal/reset count
        (void) h.GetMin();
        VERIFY_ARE_EQUAL(h.GetSampleSize(), (unsigned)3);
        VERIFY_ARE_EQUAL(h.GetSampleBuckets(), (unsigned)2);

        h.Add(0);
        VERIFY_ARE_EQUAL(h.GetSampleSize(), (unsigned)1);
        VERIFY_ARE_EQUAL(h.GetSampleBuckets(), (unsigned)1);

        (void) h.GetMin();
        VERIFY_ARE_EQUAL(h.GetSampleSize(), (unsigned)1);
        VERIFY_ARE_EQUAL(h.GetSampleBuckets(), (unsigned)1);
    }

    void HistogramUnitTests::Test_Clear()
    {
        Histogram<int> h;
        h.Add(42);
        h.Clear();
        VERIFY_ARE_EQUAL(h.GetSampleSize(), (unsigned)0);
    }

    void HistogramUnitTests::Test_MinMax()
    {
        // use unsigned here for the sake of a compact empty "min"
        // signed would be ~0 as negative int
        Histogram<unsigned> h;
        h.Add(1);
        h.Add(3);
        VERIFY_ARE_EQUAL(h.GetMin(), (unsigned)1);
        VERIFY_ARE_EQUAL(h.GetMax(), (unsigned)3);
        VERIFY_ARE_EQUAL(h.GetSampleSize(), (unsigned)2);

        // seal/reset
        h.Add(2);
        VERIFY_ARE_EQUAL(h.GetMin(), (unsigned)2);
        VERIFY_ARE_EQUAL(h.GetMax(), (unsigned)2);
        VERIFY_ARE_EQUAL(h.GetSampleSize(), (unsigned)1);

        // empty case
        h.Clear();
        VERIFY_ARE_EQUAL(h.GetMin(), (unsigned)0);
        VERIFY_ARE_EQUAL(h.GetMax(), (unsigned)0);
    }

    void HistogramUnitTests::Test_GetPercentile()
    {
        Histogram<int> h;
        h.Add(1);
        h.Add(2);
        h.Add(3);
        VERIFY_ARE_EQUAL(h.GetSampleSize(), (unsigned)3);
        VERIFY_ARE_EQUAL(h.GetPercentile(0.0), 1);
        VERIFY_ARE_EQUAL(h.GetPercentile(0.5), 2);
        VERIFY_ARE_EQUAL(h.GetPercentile(1.0), 3);
        VERIFY_ARE_EQUAL(h.GetSampleSize(), (unsigned)3);

        // single sample buckets
        for (int i = 1; i < 100; i++)
        {
            h.Add(i);
        }
        // double query at same val, forward, back and again
        // stresses iterator save correctness
        VERIFY_ARE_EQUAL(h.GetSampleSize(), (unsigned)99);
        VERIFY_ARE_EQUAL(h.GetPercentile(0.0), 1);
        VERIFY_ARE_EQUAL(h.GetPercentile(0.5), 50);
        VERIFY_ARE_EQUAL(h.GetPercentile(0.5), 50);
        VERIFY_ARE_EQUAL(h.GetPercentile(0.6), 60);
        VERIFY_ARE_EQUAL(h.GetPercentile(0.1), 10);
        VERIFY_ARE_EQUAL(h.GetPercentile(0.5), 50);
        VERIFY_ARE_EQUAL(h.GetPercentile(0.5), 50);
        VERIFY_ARE_EQUAL(h.GetPercentile(0.6), 60);
        VERIFY_ARE_EQUAL(h.GetPercentile(1.0), 99);
        VERIFY_ARE_EQUAL(h.GetSampleSize(), (unsigned)99);

        // multiple sample buckets - all same (2)
        for (int i = 1; i < 100; i++)
        {
            h.Add(i);
            h.Add(i);
        }
        // double query at same val, forward, back and again
        // stresses iterator save correctness
        VERIFY_ARE_EQUAL(h.GetSampleSize(), (unsigned)198);
        VERIFY_ARE_EQUAL(h.GetPercentile(0.0), 1);
        VERIFY_ARE_EQUAL(h.GetPercentile(0.5), 50);
        VERIFY_ARE_EQUAL(h.GetPercentile(0.5), 50);
        VERIFY_ARE_EQUAL(h.GetPercentile(0.6), 60);
        VERIFY_ARE_EQUAL(h.GetPercentile(0.1), 10);
        VERIFY_ARE_EQUAL(h.GetPercentile(0.5), 50);
        VERIFY_ARE_EQUAL(h.GetPercentile(0.5), 50);
        VERIFY_ARE_EQUAL(h.GetPercentile(0.6), 60);
        VERIFY_ARE_EQUAL(h.GetPercentile(1.0), 99);
        VERIFY_ARE_EQUAL(h.GetSampleSize(), (unsigned)198);

        // multiple sample buckets - extra weights on low end shift things lower
        for (int i = 1; i < 100; i++)
        {
            h.Add(i);

            if (i < 50)
            {
                h.Add(i);
            }
        }
        // double query at same val, forward, back and again
        // stresses iterator save correctness
        VERIFY_ARE_EQUAL(h.GetSampleSize(), (unsigned)148);
        VERIFY_ARE_EQUAL(h.GetPercentile(0.0), 1);
        VERIFY_ARE_EQUAL(h.GetPercentile(0.5), 37);
        VERIFY_ARE_EQUAL(h.GetPercentile(0.5), 37);
        VERIFY_ARE_EQUAL(h.GetPercentile(0.6), 45);
        VERIFY_ARE_EQUAL(h.GetPercentile(0.1), 8);
        VERIFY_ARE_EQUAL(h.GetPercentile(0.5), 37);
        VERIFY_ARE_EQUAL(h.GetPercentile(0.5), 37);
        VERIFY_ARE_EQUAL(h.GetPercentile(0.6), 45);
        VERIFY_ARE_EQUAL(h.GetPercentile(1.0), 99);
        VERIFY_ARE_EQUAL(h.GetSampleSize(), (unsigned)148);
    }

    void HistogramUnitTests::Test_GetMean()
    {
        Histogram<int> h;
        h.Add(2);
        h.Add(4);
        VERIFY_ARE_EQUAL(h.GetMean(), 3);
    }

    void HistogramUnitTests::Test_Merge()
    {
        Histogram<int> h1;
        h1.Add(1);

        Histogram<int> h2;
        h2.Add(2);

        h1.Merge(h2);

        VERIFY_ARE_EQUAL(h1.GetSampleSize(), (unsigned)2);
    }

    void IoBucketizerUnitTests::Test_Empty()
    {
        IoBucketizer b;
        VERIFY_ARE_EQUAL(b.GetNumberOfValidBuckets(), (size_t)0);
    }

    void IoBucketizerUnitTests::Test_Add()
    {
        IoBucketizer b;
        b.Initialize(10, 4);

        b.Add(5, 1);
        b.Add(8, 2);
        VERIFY_ARE_EQUAL(b.GetNumberOfValidBuckets(), (size_t)1);

        b.Add(15, 3);
        VERIFY_ARE_EQUAL(b.GetNumberOfValidBuckets(), (size_t)2);

        b.Add(18, 5);
        VERIFY_ARE_EQUAL(b.GetNumberOfValidBuckets(), (size_t)2);

        b.Add(45, 4);
        VERIFY_ARE_EQUAL(b.GetNumberOfValidBuckets(), (size_t)4);

        VERIFY_ARE_EQUAL(b.GetIoBucketCount(0), (unsigned int)2);
        VERIFY_ARE_EQUAL(b.GetIoBucketMinDurationUsec(0), 1L);
        VERIFY_ARE_EQUAL(b.GetIoBucketMaxDurationUsec(0), 2L);
        VERIFY_ARE_EQUAL(b.GetIoBucketAvgDurationUsec(0), 1.5L);
        VERIFY_ARE_EQUAL(b.GetIoBucketDurationStdDevUsec(0), 0.5L);
        VERIFY_ARE_EQUAL(b.GetIoBucketCount(1), (unsigned int)2);
        VERIFY_ARE_EQUAL(b.GetIoBucketMinDurationUsec(1), 3L);
        VERIFY_ARE_EQUAL(b.GetIoBucketMaxDurationUsec(1), 5L);
        VERIFY_ARE_EQUAL(b.GetIoBucketAvgDurationUsec(1), 4L);
        VERIFY_ARE_EQUAL(b.GetIoBucketDurationStdDevUsec(1), 1L);
        VERIFY_ARE_EQUAL(b.GetIoBucketCount(2), (unsigned int)0);
        VERIFY_ARE_EQUAL(b.GetIoBucketMinDurationUsec(2), 0);
        VERIFY_ARE_EQUAL(b.GetIoBucketMaxDurationUsec(2), 0);
        VERIFY_ARE_EQUAL(b.GetIoBucketAvgDurationUsec(2), 0);
        VERIFY_ARE_EQUAL(b.GetIoBucketDurationStdDevUsec(2), 0);
        VERIFY_ARE_EQUAL(b.GetIoBucketCount(3), (unsigned int)0);
        VERIFY_ARE_EQUAL(b.GetIoBucketMinDurationUsec(3), 0);
        VERIFY_ARE_EQUAL(b.GetIoBucketMaxDurationUsec(3), 0);
        VERIFY_ARE_EQUAL(b.GetIoBucketAvgDurationUsec(3), 0);
        VERIFY_ARE_EQUAL(b.GetIoBucketDurationStdDevUsec(3), 0);
    }

    void IoBucketizerUnitTests::Test_Merge()
    {
        IoBucketizer b1;
        IoBucketizer b2;
        b1.Initialize(10, 3);
        b2.Initialize(10, 3);

        // b1 buckets: 2,0,1
        b1.Add(0, 0);
        b1.Add(1, 0);
        b1.Add(20, 0);
        b1.Add(30, 0);

        VERIFY_ARE_EQUAL(b1.GetNumberOfValidBuckets(), (size_t)3);
        VERIFY_ARE_EQUAL(b1.GetIoBucketCount(0), (unsigned int)2);
        VERIFY_ARE_EQUAL(b1.GetIoBucketCount(1), (unsigned int)0);
        VERIFY_ARE_EQUAL(b1.GetIoBucketCount(2), (unsigned int)1);

        // b2 buckets: 1,3
        b2.Add(0, 0);
        b2.Add(10, 0);
        b2.Add(11, 0);
        b2.Add(12, 0);

        VERIFY_ARE_EQUAL(b2.GetNumberOfValidBuckets(), (size_t)2);
        VERIFY_ARE_EQUAL(b2.GetIoBucketCount(0), (unsigned int)1);
        VERIFY_ARE_EQUAL(b2.GetIoBucketCount(1), (unsigned int)3);

        b1.Merge(b2);

        // Merged buckets: 3,3,1
        VERIFY_ARE_EQUAL(b1.GetNumberOfValidBuckets(), (size_t)3);
        VERIFY_ARE_EQUAL(b1.GetIoBucketCount(0), (unsigned int)3);
        VERIFY_ARE_EQUAL(b1.GetIoBucketCount(1), (unsigned int)3);
        VERIFY_ARE_EQUAL(b1.GetIoBucketCount(2), (unsigned int)1);

        // Source unchanged.
        VERIFY_ARE_EQUAL(b2.GetNumberOfValidBuckets(), (size_t)2);
        VERIFY_ARE_EQUAL(b2.GetIoBucketCount(0), (unsigned int)1);
        VERIFY_ARE_EQUAL(b2.GetIoBucketCount(1), (unsigned int)3);

        // Merge into empty bucketizer
        IoBucketizer b3;

        // Its empty.
        VERIFY_ARE_EQUAL(b3.GetNumberOfValidBuckets(), (size_t)0);

        b3.Merge(b1);

        // Merged buckets: 3,3,1
        VERIFY_ARE_EQUAL(b3.GetNumberOfValidBuckets(), (size_t)3);
        VERIFY_ARE_EQUAL(b3.GetIoBucketCount(0), (unsigned int)3);
        VERIFY_ARE_EQUAL(b3.GetIoBucketCount(1), (unsigned int)3);
        VERIFY_ARE_EQUAL(b3.GetIoBucketCount(2), (unsigned int)1);
    }

    void IoBucketizerUnitTests::Test_GetStandardDeviation()
    {
        IoBucketizer b;
        b.Initialize(10, 2);

        // b buckets: 1,2
        b.Add(0, 0);
        b.Add(10, 0);
        b.Add(11, 0);
        b.Add(20, 0);

        // Standard deviation from valid buckets (the first two) is STDDEV(1,2) = 0.5
        VERIFY_ARE_EQUAL(b.GetStandardDeviationIOPS(), 0.5L);
    }

    void ProfileUnitTests::Test_GetXmlEmptyProfile()
    {
        Profile profile;
        string sXml = profile.GetXml(0);
        //printf("'%s'\n", sXml.c_str());
        const char *pcszExpected = "<Profile>\n"
                               "  <Progress>0</Progress>\n"
                               "  <ResultFormat>text</ResultFormat>\n"
                               "  <Verbose>false</Verbose>\n"
                               "  <TimeSpans>\n"
                               "  </TimeSpans>\n"
                               "</Profile>\n";
        VERIFY_MULTILINE_EQUAL(pcszExpected, sXml);
    }

    void ProfileUnitTests::Test_GetXmlPrecreateFilesUseMaxSize()
    {
        Profile profile;
        profile.SetPrecreateFiles(PrecreateFiles::UseMaxSize);
        string sXml = profile.GetXml(0);
        //printf("'%s'\n", sXml.c_str());
        const char *pcszExpected = "<Profile>\n"
                               "  <Progress>0</Progress>\n"
                               "  <ResultFormat>text</ResultFormat>\n"
                               "  <Verbose>false</Verbose>\n"
                               "  <PrecreateFiles>UseMaxSize</PrecreateFiles>\n"
                               "  <TimeSpans>\n"
                               "  </TimeSpans>\n"
                               "</Profile>\n";
        VERIFY_MULTILINE_EQUAL(pcszExpected, sXml);
    }

    void ProfileUnitTests::Test_GetXmlPrecreateFilesOnlyFilesWithConstantSizes()
    {
        Profile profile;
        profile.SetPrecreateFiles(PrecreateFiles::OnlyFilesWithConstantSizes);
        string sXml = profile.GetXml(0);
        //printf("'%s'\n", sXml.c_str());
        const char *pcszExpected = "<Profile>\n"
                               "  <Progress>0</Progress>\n"
                               "  <ResultFormat>text</ResultFormat>\n"
                               "  <Verbose>false</Verbose>\n"
                               "  <PrecreateFiles>CreateOnlyFilesWithConstantSizes</PrecreateFiles>\n"
                               "  <TimeSpans>\n"
                               "  </TimeSpans>\n"
                               "</Profile>\n";
        VERIFY_MULTILINE_EQUAL(pcszExpected, sXml);
    }

    void ProfileUnitTests::Test_GetXmlPrecreateFilesOnlyFilesWithConstantOrZeroSizes()
    {
        Profile profile;
        profile.SetPrecreateFiles(PrecreateFiles::OnlyFilesWithConstantOrZeroSizes);
        string sXml = profile.GetXml(0);
        //printf("'%s'\n", sXml.c_str());
        const char *pcszExpected = "<Profile>\n"
                               "  <Progress>0</Progress>\n"
                               "  <ResultFormat>text</ResultFormat>\n"
                               "  <Verbose>false</Verbose>\n"
                               "  <PrecreateFiles>CreateOnlyFilesWithConstantOrZeroSizes</PrecreateFiles>\n"
                               "  <TimeSpans>\n"
                               "  </TimeSpans>\n"
                               "</Profile>\n";
        VERIFY_MULTILINE_EQUAL(pcszExpected, sXml);
    }

    void ProfileUnitTests::Test_MarkFilesAsCreated()
    {
        Target target1;
        target1.SetPath("file1.txt");

        Target target2;
        target2.SetPath("file2.txt");

        Target target3;
        target3.SetPath("file1.txt");

        Target target4;
        target4.SetPath("file3.txt");

        Target target5;
        target5.SetPath("file2.txt");

        Target target6;
        target6.SetPath("file2.txt");

        TimeSpan timeSpan1;
        timeSpan1.AddTarget(target1);
        timeSpan1.AddTarget(target2);

        TimeSpan timeSpan2;
        timeSpan2.AddTarget(target3);
        timeSpan2.AddTarget(target4);
        timeSpan2.AddTarget(target5);
        timeSpan2.AddTarget(target6);

        Profile profile;
        profile.AddTimeSpan(timeSpan1);
        profile.AddTimeSpan(timeSpan2);

        vector<string> vFiles;
        vFiles.push_back("file1.txt");
        vFiles.push_back("file2.txt");

        VERIFY_IS_FALSE(profile._vTimeSpans[0]._vTargets[0]._fPrecreated);
        VERIFY_IS_FALSE(profile._vTimeSpans[0]._vTargets[1]._fPrecreated);
        VERIFY_IS_FALSE(profile._vTimeSpans[1]._vTargets[0]._fPrecreated);
        VERIFY_IS_FALSE(profile._vTimeSpans[1]._vTargets[1]._fPrecreated);
        VERIFY_IS_FALSE(profile._vTimeSpans[1]._vTargets[2]._fPrecreated);
        VERIFY_IS_FALSE(profile._vTimeSpans[1]._vTargets[3]._fPrecreated);

        profile.MarkFilesAsPrecreated(vFiles);
        VERIFY_IS_TRUE(profile._vTimeSpans[0]._vTargets[0]._fPrecreated);
        VERIFY_IS_TRUE(profile._vTimeSpans[0]._vTargets[1]._fPrecreated);
        VERIFY_IS_TRUE(profile._vTimeSpans[1]._vTargets[0]._fPrecreated);
        VERIFY_IS_FALSE(profile._vTimeSpans[1]._vTargets[1]._fPrecreated);
        VERIFY_IS_TRUE(profile._vTimeSpans[1]._vTargets[2]._fPrecreated);
        VERIFY_IS_TRUE(profile._vTimeSpans[1]._vTargets[3]._fPrecreated);
    }

    void ProfileUnitTests::Test_Validate()
    {
        TimeSpan timeSpan;
        Target target;

        target.SetBaseFileOffsetInBytes(1000);
        target.SetBlockAlignmentInBytes(500);
        target.SetBlockSizeInBytes(1000);
        target.SetThreadStrideInBytes(5000);
        timeSpan.AddTarget(target);

        Profile profile;
        profile.AddTimeSpan(timeSpan);

        // thread stride errors if only one thread used (default)
        // both the single spec assumption and full should behave the same
        VERIFY_IS_FALSE(profile.Validate(true));
        VERIFY_IS_FALSE(profile.Validate(false));

        profile._vTimeSpans[0].SetThreadCount(2);
        VERIFY_IS_TRUE(profile.Validate(true));
        VERIFY_IS_TRUE(profile.Validate(false));

        // now turning on interlocked sequential, fail since thread stride is set
        profile._vTimeSpans[0]._vTargets[0].SetUseInterlockedSequential(true);
        VERIFY_IS_FALSE(profile.Validate(true));
        VERIFY_IS_FALSE(profile.Validate(false));

        profile._vTimeSpans[0]._vTargets[0].SetThreadStrideInBytes(0);
        VERIFY_IS_TRUE(profile.Validate(true));
        VERIFY_IS_TRUE(profile.Validate(false));
    }

    void ProfileUnitTests::Test_ValidateSystem()
    {
        // processor topology validation for affinity assignments
        // 2 group, 2 procs/group
        SystemInformation system;
        system.processorTopology._vProcessorGroupInformation.clear();
        system.processorTopology._vProcessorGroupInformation.emplace_back((WORD)0, (BYTE)2, (BYTE)2, (KAFFINITY)0x3);
        system.processorTopology._vProcessorGroupInformation.emplace_back((WORD)1, (BYTE)2, (BYTE)2, (KAFFINITY)0x3);

        TimeSpan timeSpan;
        Profile profile;

        // assign to each proc
        profile.ClearTimeSpans();
        timeSpan.ClearAffinityGroupMasks();
        timeSpan.AddAffinityGroupMaskCpu(0, 0);
        timeSpan.AddAffinityGroupMaskCpu(0, 1);
        timeSpan.AddAffinityGroupMaskCpu(1, 0);
        timeSpan.AddAffinityGroupMaskCpu(1, 1);
        profile.AddTimeSpan(timeSpan);
        VERIFY_IS_TRUE(profile.Validate(true, &system));

        // shrink active mask
        system.processorTopology._vProcessorGroupInformation.clear();
        system.processorTopology._vProcessorGroupInformation.emplace_back((WORD)0, (BYTE)2, (BYTE)2, (KAFFINITY)0x1);
        system.processorTopology._vProcessorGroupInformation.emplace_back((WORD)1, (BYTE)2, (BYTE)2, (KAFFINITY)0x1);

        // fail assignment to inactive procs
        VERIFY_IS_FALSE(profile.Validate(true, &system));

        // shrink procs, still fail
        system.processorTopology._vProcessorGroupInformation.clear();
        system.processorTopology._vProcessorGroupInformation.emplace_back((WORD)0, (BYTE)1, (BYTE)1, (KAFFINITY)0x1);
        system.processorTopology._vProcessorGroupInformation.emplace_back((WORD)1, (BYTE)1, (BYTE)1, (KAFFINITY)0x1);

        // now fail
        VERIFY_IS_FALSE(profile.Validate(true, &system));

        // assign to low procs, and succeed
        profile.ClearTimeSpans();
        timeSpan.ClearAffinityGroupMasks();
        timeSpan.AddAffinityGroupMaskCpu(0, 0);
        timeSpan.AddAffinityGroupMaskCpu(1, 0);
        profile.AddTimeSpan(timeSpan);
        VERIFY_IS_TRUE(profile.Validate(true, &system));

        // shrink groups
        system.processorTopology._vProcessorGroupInformation.clear();
        system.processorTopology._vProcessorGroupInformation.emplace_back((WORD)0, (BYTE)1, (BYTE)1, (KAFFINITY)0x1);

        // now fail
        VERIFY_IS_FALSE(profile.Validate(true, &system));

        // assign to low proc, and succeed
        profile.ClearTimeSpans();
        timeSpan.ClearAffinityGroupMasks();
        timeSpan.AddAffinityGroupMaskCpu(0, 0);
        profile.AddTimeSpan(timeSpan);
        VERIFY_IS_TRUE(profile.Validate(true, &system));

        // assign to invalid group
        profile.ClearTimeSpans();
        timeSpan.ClearAffinityGroupMasks();
        timeSpan.AddAffinityGroupMaskCpu(1, 0);
        profile.AddTimeSpan(timeSpan);
        VERIFY_IS_FALSE(profile.Validate(true, &system));
    }

    void ProfileUnitTests::Test_FinalizeAffinity_CoreAware()
    {
        //
        // Shared system topology configurations used across multiple test cases.
        // These are const - the same SystemInformation is reused for all cases
        // that share a topology.
        //

        // 4-core HT, single group, homogeneous
        // Cores: 0/1, 2/3, 4/5, 6/7 (all eff=0)
        SystemInformation sys4coreHT;
        sys4coreHT.processorTopology._vProcessorGroupInformation.clear();
        sys4coreHT.processorTopology._vProcessorGroupInformation.emplace_back((WORD)0, (BYTE)8, (BYTE)8, (KAFFINITY)0xFF);
        sys4coreHT.processorTopology._vProcessorCoreInformation.clear();
        sys4coreHT.processorTopology._vProcessorCoreInformation.emplace_back((WORD)0, (KAFFINITY)0x3, (BYTE)0);
        sys4coreHT.processorTopology._vProcessorCoreInformation.emplace_back((WORD)0, (KAFFINITY)0xC, (BYTE)0);
        sys4coreHT.processorTopology._vProcessorCoreInformation.emplace_back((WORD)0, (KAFFINITY)0x30, (BYTE)0);
        sys4coreHT.processorTopology._vProcessorCoreInformation.emplace_back((WORD)0, (KAFFINITY)0xC0, (BYTE)0);
        sys4coreHT.processorTopology._ubPerformanceEfficiencyClass = 0;
        sys4coreHT.processorTopology._fSMT = true;

        // 2-core HT, single group, homogeneous
        // Cores: 0/1, 2/3 (all eff=0)
        SystemInformation sys2coreHT;
        sys2coreHT.processorTopology._vProcessorGroupInformation.clear();
        sys2coreHT.processorTopology._vProcessorGroupInformation.emplace_back((WORD)0, (BYTE)4, (BYTE)4, (KAFFINITY)0xF);
        sys2coreHT.processorTopology._vProcessorCoreInformation.clear();
        sys2coreHT.processorTopology._vProcessorCoreInformation.emplace_back((WORD)0, (KAFFINITY)0x3, (BYTE)0);
        sys2coreHT.processorTopology._vProcessorCoreInformation.emplace_back((WORD)0, (KAFFINITY)0xC, (BYTE)0);
        sys2coreHT.processorTopology._ubPerformanceEfficiencyClass = 0;
        sys2coreHT.processorTopology._fSMT = true;

        // 2-group HT, 2 cores per group, homogeneous
        // Group 0: cores 0/1, 2/3. Group 1: cores 0/1, 2/3. (all eff=0)
        SystemInformation sys2groupHT;
        sys2groupHT.processorTopology._vProcessorGroupInformation.clear();
        sys2groupHT.processorTopology._vProcessorGroupInformation.emplace_back((WORD)0, (BYTE)4, (BYTE)4, (KAFFINITY)0xF);
        sys2groupHT.processorTopology._vProcessorGroupInformation.emplace_back((WORD)1, (BYTE)4, (BYTE)4, (KAFFINITY)0xF);
        sys2groupHT.processorTopology._vProcessorCoreInformation.clear();
        sys2groupHT.processorTopology._vProcessorCoreInformation.emplace_back((WORD)0, (KAFFINITY)0x3, (BYTE)0);
        sys2groupHT.processorTopology._vProcessorCoreInformation.emplace_back((WORD)0, (KAFFINITY)0xC, (BYTE)0);
        sys2groupHT.processorTopology._vProcessorCoreInformation.emplace_back((WORD)1, (KAFFINITY)0x3, (BYTE)0);
        sys2groupHT.processorTopology._vProcessorCoreInformation.emplace_back((WORD)1, (KAFFINITY)0xC, (BYTE)0);
        sys2groupHT.processorTopology._ubPerformanceEfficiencyClass = 0;
        sys2groupHT.processorTopology._fSMT = true;

        // Heterogeneous: 2 P-cores (HT, eff=1) + 2 E-cores (no HT, eff=0), single group
        // P: cores 0/1 (0x3), 2/3 (0xC). E: core 4 (0x10), core 5 (0x20).
        SystemInformation sysHetero;
        sysHetero.processorTopology._vProcessorGroupInformation.clear();
        sysHetero.processorTopology._vProcessorGroupInformation.emplace_back((WORD)0, (BYTE)6, (BYTE)6, (KAFFINITY)0x3F);
        sysHetero.processorTopology._vProcessorCoreInformation.clear();
        sysHetero.processorTopology._vProcessorCoreInformation.emplace_back((WORD)0, (KAFFINITY)0x3, (BYTE)1);
        sysHetero.processorTopology._vProcessorCoreInformation.emplace_back((WORD)0, (KAFFINITY)0xC, (BYTE)1);
        sysHetero.processorTopology._vProcessorCoreInformation.emplace_back((WORD)0, (KAFFINITY)0x10, (BYTE)0);
        sysHetero.processorTopology._vProcessorCoreInformation.emplace_back((WORD)0, (KAFFINITY)0x20, (BYTE)0);
        sysHetero.processorTopology._ubPerformanceEfficiencyClass = 1;
        sysHetero.processorTopology._fSMT = true;

        // Heterogeneous ARM-style: E-cores first, P-cores last (opposite of Intel)
        // E: core 0 (cpu 0, eff=0), core 1 (cpu 1, eff=0). P: cores 2/3 (0xC, eff=1), 4/5 (0x30, eff=1).
        SystemInformation sysHeteroARM;
        sysHeteroARM.processorTopology._vProcessorGroupInformation.clear();
        sysHeteroARM.processorTopology._vProcessorGroupInformation.emplace_back((WORD)0, (BYTE)6, (BYTE)6, (KAFFINITY)0x3F);
        sysHeteroARM.processorTopology._vProcessorCoreInformation.clear();
        sysHeteroARM.processorTopology._vProcessorCoreInformation.emplace_back((WORD)0, (KAFFINITY)0x1, (BYTE)0);
        sysHeteroARM.processorTopology._vProcessorCoreInformation.emplace_back((WORD)0, (KAFFINITY)0x2, (BYTE)0);
        sysHeteroARM.processorTopology._vProcessorCoreInformation.emplace_back((WORD)0, (KAFFINITY)0xC, (BYTE)1);
        sysHeteroARM.processorTopology._vProcessorCoreInformation.emplace_back((WORD)0, (KAFFINITY)0x30, (BYTE)1);
        sysHeteroARM.processorTopology._ubPerformanceEfficiencyClass = 1;
        sysHeteroARM.processorTopology._fSMT = true;

        // Case 1: 4-core HT, full mask, CoreAware
        // Expect: 0,2,4,6,1,3,5,7
        {
            TimeSpan ts;
            ts.SetSystem(&sys4coreHT);
            ts.SetAffinityTraversal(AffinityTraversal::CoreAware);
            ts.AddAffinityGroupMask((WORD)0, (KAFFINITY)0xFF);
            ts.Finalize();

            const auto& v = ts.GetEffectiveAffinityAssignments();
            VERIFY_ARE_EQUAL(v.size(), (size_t)8);
            VERIFY_ARE_EQUAL(v[0].bProc, (BYTE)0);
            VERIFY_ARE_EQUAL(v[1].bProc, (BYTE)2);
            VERIFY_ARE_EQUAL(v[2].bProc, (BYTE)4);
            VERIFY_ARE_EQUAL(v[3].bProc, (BYTE)6);
            VERIFY_ARE_EQUAL(v[4].bProc, (BYTE)1);
            VERIFY_ARE_EQUAL(v[5].bProc, (BYTE)3);
            VERIFY_ARE_EQUAL(v[6].bProc, (BYTE)5);
            VERIFY_ARE_EQUAL(v[7].bProc, (BYTE)7);
        }

        // Case 2: Partial mask on HT system - CPUs 3,4,5,6 on 4-core HT
        // Core 1: 2/3 - only cpu 3 in mask (pass 0)
        // Core 2: 4/5 - both in mask (pass 0: 4, pass 1: 5)
        // Core 3: 6/7 - only cpu 6 in mask (pass 0)
        // Expect: 3,4,6,5
        {
            TimeSpan ts;
            ts.SetSystem(&sys4coreHT);
            ts.SetAffinityTraversal(AffinityTraversal::CoreAware);
            ts.AddAffinityGroupMask((WORD)0, (KAFFINITY)0x78);
            ts.Finalize();

            const auto& v = ts.GetEffectiveAffinityAssignments();
            VERIFY_ARE_EQUAL(v.size(), (size_t)4);
            VERIFY_ARE_EQUAL(v[0].bProc, (BYTE)3);
            VERIFY_ARE_EQUAL(v[1].bProc, (BYTE)4);
            VERIFY_ARE_EQUAL(v[2].bProc, (BYTE)6);
            VERIFY_ARE_EQUAL(v[3].bProc, (BYTE)5);
        }

        // Case 3: Non-HT system - no reordering needed
        // 4 cores, 1 cpu each (0,1,2,3). Expect: 0,1,2,3
        {
            SystemInformation system;
            system.processorTopology._vProcessorGroupInformation.clear();
            system.processorTopology._vProcessorGroupInformation.emplace_back((WORD)0, (BYTE)4, (BYTE)4, (KAFFINITY)0xF);
            system.processorTopology._vProcessorCoreInformation.clear();
            system.processorTopology._vProcessorCoreInformation.emplace_back((WORD)0, (KAFFINITY)0x1, (BYTE)0);
            system.processorTopology._vProcessorCoreInformation.emplace_back((WORD)0, (KAFFINITY)0x2, (BYTE)0);
            system.processorTopology._vProcessorCoreInformation.emplace_back((WORD)0, (KAFFINITY)0x4, (BYTE)0);
            system.processorTopology._vProcessorCoreInformation.emplace_back((WORD)0, (KAFFINITY)0x8, (BYTE)0);
            system.processorTopology._ubPerformanceEfficiencyClass = 0;
            system.processorTopology._fSMT = false;

            TimeSpan ts;
            ts.SetSystem(&system);
            ts.SetAffinityTraversal(AffinityTraversal::CoreAware);
            ts.AddAffinityGroupMask((WORD)0, (KAFFINITY)0xF);
            ts.Finalize();

            const auto& v = ts.GetEffectiveAffinityAssignments();
            VERIFY_ARE_EQUAL(v.size(), (size_t)4);
            VERIFY_ARE_EQUAL(v[0].bProc, (BYTE)0);
            VERIFY_ARE_EQUAL(v[1].bProc, (BYTE)1);
            VERIFY_ARE_EQUAL(v[2].bProc, (BYTE)2);
            VERIFY_ARE_EQUAL(v[3].bProc, (BYTE)3);
        }

        // Case 4: Heterogeneous P+E with HT on P-cores only
        // P-cores: core 0 (cpus 0,1) eff=1, core 1 (cpus 2,3) eff=1
        // E-cores: core 2 (cpu 4) eff=0, core 3 (cpu 5) eff=0
        // CoreAware: pass 0 P-cores (0,2), pass 0 E-cores (4,5), pass 1 P-cores (1,3)
        {
            TimeSpan ts;
            ts.SetSystem(&sysHetero);
            ts.SetAffinityTraversal(AffinityTraversal::CoreAware);
            ts.AddAffinityGroupMask((WORD)0, (KAFFINITY)0x3F);
            ts.Finalize();

            const auto& v = ts.GetEffectiveAffinityAssignments();
            VERIFY_ARE_EQUAL(v.size(), (size_t)6);
            VERIFY_ARE_EQUAL(v[0].bProc, (BYTE)0);
            VERIFY_ARE_EQUAL(v[1].bProc, (BYTE)2);
            VERIFY_ARE_EQUAL(v[2].bProc, (BYTE)4);
            VERIFY_ARE_EQUAL(v[3].bProc, (BYTE)5);
            VERIFY_ARE_EQUAL(v[4].bProc, (BYTE)1);
            VERIFY_ARE_EQUAL(v[5].bProc, (BYTE)3);
        }

        // Case 5: 2-group HT, CoreAware + Span
        // Group 0: core 0 (0/1), core 1 (2/3). Group 1: core 0 (0/1), core 1 (2/3).
        // SystemWide: pass 0 all groups (g0/0, g0/2, g1/0, g1/2), pass 1 (g0/1, g0/3, g1/1, g1/3)
        {
            TimeSpan ts;
            ts.SetSystem(&sys2groupHT);
           ts.SetAffinityTraversal(AffinityTraversal::CoreAware);
            ts.SetAffinityTraversal(AffinityTraversal::CoreAware);
            ts.SetAffinityGroupSpan(AffinityGroupSpan::Span);
            ts.AddAffinityGroupMask((WORD)0, (KAFFINITY)0xF);
            ts.AddAffinityGroupMask((WORD)1, (KAFFINITY)0xF);
            ts.Finalize();

            const auto& v = ts.GetEffectiveAffinityAssignments();
            VERIFY_ARE_EQUAL(v.size(), (size_t)8);
            // pass 0: g0/0, g0/2, g1/0, g1/2
            VERIFY_ARE_EQUAL(v[0].wGroup, (WORD)0); VERIFY_ARE_EQUAL(v[0].bProc, (BYTE)0);
            VERIFY_ARE_EQUAL(v[1].wGroup, (WORD)0); VERIFY_ARE_EQUAL(v[1].bProc, (BYTE)2);
            VERIFY_ARE_EQUAL(v[2].wGroup, (WORD)1); VERIFY_ARE_EQUAL(v[2].bProc, (BYTE)0);
            VERIFY_ARE_EQUAL(v[3].wGroup, (WORD)1); VERIFY_ARE_EQUAL(v[3].bProc, (BYTE)2);
            // pass 1: g0/1, g0/3, g1/1, g1/3
            VERIFY_ARE_EQUAL(v[4].wGroup, (WORD)0); VERIFY_ARE_EQUAL(v[4].bProc, (BYTE)1);
            VERIFY_ARE_EQUAL(v[5].wGroup, (WORD)0); VERIFY_ARE_EQUAL(v[5].bProc, (BYTE)3);
            VERIFY_ARE_EQUAL(v[6].wGroup, (WORD)1); VERIFY_ARE_EQUAL(v[6].bProc, (BYTE)1);
            VERIFY_ARE_EQUAL(v[7].wGroup, (WORD)1); VERIFY_ARE_EQUAL(v[7].bProc, (BYTE)3);
        }

        // Case 6: 2-group HT, default CoreAware (group-contained)
        // Same topology as case 5. Expect group 0 fully consumed then group 1.
        // g0: 0,2,1,3 then g1: 0,2,1,3
        {
            TimeSpan ts;
            ts.SetSystem(&sys2groupHT);
            ts.SetAffinityTraversal(AffinityTraversal::CoreAware);
            ts.AddAffinityGroupMask((WORD)0, (KAFFINITY)0xF);
            ts.AddAffinityGroupMask((WORD)1, (KAFFINITY)0xF);
            ts.Finalize();

            const auto& v = ts.GetEffectiveAffinityAssignments();
            VERIFY_ARE_EQUAL(v.size(), (size_t)8);
            // group 0 fully: pass 0 then pass 1
            VERIFY_ARE_EQUAL(v[0].wGroup, (WORD)0); VERIFY_ARE_EQUAL(v[0].bProc, (BYTE)0);
            VERIFY_ARE_EQUAL(v[1].wGroup, (WORD)0); VERIFY_ARE_EQUAL(v[1].bProc, (BYTE)2);
            VERIFY_ARE_EQUAL(v[2].wGroup, (WORD)0); VERIFY_ARE_EQUAL(v[2].bProc, (BYTE)1);
            VERIFY_ARE_EQUAL(v[3].wGroup, (WORD)0); VERIFY_ARE_EQUAL(v[3].bProc, (BYTE)3);
            // group 1 fully
            VERIFY_ARE_EQUAL(v[4].wGroup, (WORD)1); VERIFY_ARE_EQUAL(v[4].bProc, (BYTE)0);
            VERIFY_ARE_EQUAL(v[5].wGroup, (WORD)1); VERIFY_ARE_EQUAL(v[5].bProc, (BYTE)2);
            VERIFY_ARE_EQUAL(v[6].wGroup, (WORD)1); VERIFY_ARE_EQUAL(v[6].bProc, (BYTE)1);
            VERIFY_ARE_EQUAL(v[7].wGroup, (WORD)1); VERIFY_ARE_EQUAL(v[7].bProc, (BYTE)3);
        }

        // Case 7: Cpu mode - no reordering, direct mask expansion
        {
            TimeSpan ts;
            ts.SetSystem(&sys4coreHT);
           ts.SetAffinityTraversal(AffinityTraversal::Cpu);
            ts.AddAffinityGroupMask((WORD)0, (KAFFINITY)0xFF);
            ts.Finalize();

            const auto& v = ts.GetEffectiveAffinityAssignments();
            VERIFY_ARE_EQUAL(v.size(), (size_t)8);
            for (BYTE i = 0; i < 8; i++)
            {
                VERIFY_ARE_EQUAL(v[i].bProc, i);
            }
        }

        // Case 8: Heterogeneous with partial mask - only E-cores specified
        // P-cores: core 0 (0/1 eff=1), core 1 (2/3 eff=1)
        // E-cores: core 2 (cpu 4 eff=0), core 3 (cpu 5 eff=0)
        // Mask: 0x30 (cpus 4,5 only). All pass 0, eff class 0.
        // Expect: 4, 5 (no reordering since all are unique-core pass 0)
        {
            TimeSpan ts;
            ts.SetSystem(&sysHetero);
            ts.SetAffinityTraversal(AffinityTraversal::CoreAware);
            ts.AddAffinityGroupMask((WORD)0, (KAFFINITY)0x30);
            ts.Finalize();

            const auto& v = ts.GetEffectiveAffinityAssignments();
            VERIFY_ARE_EQUAL(v.size(), (size_t)2);
            VERIFY_ARE_EQUAL(v[0].bProc, (BYTE)4);
            VERIFY_ARE_EQUAL(v[1].bProc, (BYTE)5);
        }

        // Case 9: Whole-group (zero mask) on HT system, CoreAware
        // Should expand to full active mask, then reorder core-aware.
        {
            TimeSpan ts;
            ts.SetSystem(&sys2coreHT);
            ts.SetAffinityTraversal(AffinityTraversal::CoreAware);
            ts.AddAffinityGroupMask((WORD)0, (KAFFINITY)0);
            ts.Finalize();

            const auto& v = ts.GetEffectiveAffinityAssignments();
            VERIFY_ARE_EQUAL(v.size(), (size_t)4);
            VERIFY_ARE_EQUAL(v[0].bProc, (BYTE)0);
            VERIFY_ARE_EQUAL(v[1].bProc, (BYTE)2);
            VERIFY_ARE_EQUAL(v[2].bProc, (BYTE)1);
            VERIFY_ARE_EQUAL(v[3].bProc, (BYTE)3);
        }

        // Case 10: Tail partial mask - CPUs 5,6,7 on 4-core HT (cores 0/1, 2/3, 4/5, 6/7)
        // Core 2: only cpu 5 in mask (sibling, but it's first in core -> pass 0)
        // Core 3: both 6 and 7 in mask (pass 0: 6, pass 1: 7)
        // Expect: 5, 6, 7
        {
            TimeSpan ts;
            ts.SetSystem(&sys4coreHT);
            ts.SetAffinityTraversal(AffinityTraversal::CoreAware);
            ts.AddAffinityGroupMask((WORD)0, (KAFFINITY)0xE0);
            ts.Finalize();

            const auto& v = ts.GetEffectiveAffinityAssignments();
            VERIFY_ARE_EQUAL(v.size(), (size_t)3);
            VERIFY_ARE_EQUAL(v[0].bProc, (BYTE)5);
            VERIFY_ARE_EQUAL(v[1].bProc, (BYTE)6);
            VERIFY_ARE_EQUAL(v[2].bProc, (BYTE)7);
        }

        // Case 11: Mixed order pigeon-hole - explicit CPUs 0,2,1,5 on 4-core HT
        // Cores: 0/1, 2/3, 4/5, 6/7.
        // Expanded order: 0, 2, 1, 5 (from two separate mask entries since 0,2 merges but 1 < 2 starts new, 5 merges with 1? No:
        //   0 -> {g=0, 0x1}, 2 > 0 merge -> {g=0, 0x5}, 1 < 2 new -> {g=0, 0x2}, 5 > 1 merge -> {g=0, 0x22}
        //   Expansion: mask 0x5 -> cpus 0,2; mask 0x22 -> cpus 1,5
        //   Expanded: 0, 2, 1, 5
        // Classification (in expanded order):
        //   0: core 0, occupancy[0]=0 -> pass 0
        //   2: core 1, occupancy[1]=0 -> pass 0
        //   1: core 0, occupancy[0]=1 -> pass 1 (core 0 already has cpu 0)
        //   5: core 2, occupancy[2]=0 -> pass 0
        // Sort (group -> pass -> eff -> order):
        //   pass 0: 0 (order 0), 2 (order 1), 5 (order 3)
        //   pass 1: 1 (order 2)
        // Result: 0, 2, 5, 1
        {
            TimeSpan ts;
            ts.SetSystem(&sys4coreHT);
            ts.SetAffinityTraversal(AffinityTraversal::CoreAware);
            ts.AddAffinityGroupMaskCpu((WORD)0, (BYTE)0);
            ts.AddAffinityGroupMaskCpu((WORD)0, (BYTE)2);
            ts.AddAffinityGroupMaskCpu((WORD)0, (BYTE)1);
            ts.AddAffinityGroupMaskCpu((WORD)0, (BYTE)5);
            ts.Finalize();

            const auto& v = ts.GetEffectiveAffinityAssignments();
            VERIFY_ARE_EQUAL(v.size(), (size_t)4);
            VERIFY_ARE_EQUAL(v[0].bProc, (BYTE)0);
            VERIFY_ARE_EQUAL(v[1].bProc, (BYTE)2);
            VERIFY_ARE_EQUAL(v[2].bProc, (BYTE)5);
            VERIFY_ARE_EQUAL(v[3].bProc, (BYTE)1);
        }

        // Case 12: All siblings, no primaries - mask has only cpu 1,3 (HT siblings of cores 0,1)
        // Both are pass 0 (first in their respective core within the mask)
        // Expect: 1, 3 (no reordering, both unique-core pass 0)
        {
            TimeSpan ts;
            ts.SetSystem(&sys4coreHT);
            ts.SetAffinityTraversal(AffinityTraversal::CoreAware);
            ts.AddAffinityGroupMask((WORD)0, (KAFFINITY)0xA);
            ts.Finalize();

            const auto& v = ts.GetEffectiveAffinityAssignments();
            VERIFY_ARE_EQUAL(v.size(), (size_t)2);
            VERIFY_ARE_EQUAL(v[0].bProc, (BYTE)1);
            VERIFY_ARE_EQUAL(v[1].bProc, (BYTE)3);
        }

        // Case 13: Double-dip same core via separate mask entries
        // Two mask entries both covering core 0: first {g=0, 0x1} (cpu 0), second {g=0, 0x2} (cpu 1)
        // Expanded: 0, 1. Core 0: occupancy 0 -> pass 0, occupancy 1 -> pass 1.
        // Result: 0, 1 (pass 0 then pass 1, same group/eff, order preserved)
        {
            TimeSpan ts;
            ts.SetSystem(&sys2coreHT);
            ts.SetAffinityTraversal(AffinityTraversal::CoreAware);
            ts.AddAffinityGroupMask((WORD)0, (KAFFINITY)0x1);
            ts.AddAffinityGroupMask((WORD)0, (KAFFINITY)0x2);
            ts.Finalize();

            const auto& v = ts.GetEffectiveAffinityAssignments();
            VERIFY_ARE_EQUAL(v.size(), (size_t)2);
            VERIFY_ARE_EQUAL(v[0].bProc, (BYTE)0);
            VERIFY_ARE_EQUAL(v[1].bProc, (BYTE)1);
        }

        // Case 14: Pigeon-hole stress - cpus 1,0,3,2 (reversed pairs) on 2-core HT
        // Cores: 0/1, 2/3. Expanded: 1, 0, 3, 2 (from non-mergeable mask entries)
        // Core 0: cpu 1 (pass 0), cpu 0 (pass 1)
        // Core 1: cpu 3 (pass 0), cpu 2 (pass 1)
        // Sort: pass 0 (1 order=0, 3 order=2), pass 1 (0 order=1, 2 order=3)
        // Result: 1, 3, 0, 2
        {
            TimeSpan ts;
            ts.SetSystem(&sys2coreHT);
            ts.SetAffinityTraversal(AffinityTraversal::CoreAware);
            ts.AddAffinityGroupMask((WORD)0, (KAFFINITY)0x2);
            ts.AddAffinityGroupMask((WORD)0, (KAFFINITY)0x1);
            ts.AddAffinityGroupMask((WORD)0, (KAFFINITY)0x8);
            ts.AddAffinityGroupMask((WORD)0, (KAFFINITY)0x4);
            ts.Finalize();

            const auto& v = ts.GetEffectiveAffinityAssignments();
            VERIFY_ARE_EQUAL(v.size(), (size_t)4);
            VERIFY_ARE_EQUAL(v[0].bProc, (BYTE)1);
            VERIFY_ARE_EQUAL(v[1].bProc, (BYTE)3);
            VERIFY_ARE_EQUAL(v[2].bProc, (BYTE)0);
            VERIFY_ARE_EQUAL(v[3].bProc, (BYTE)2);
        }

        // Case 15: Repeated CPU - same CPU specified multiple times via separate masks.
        // Two mask entries both specifying cpu 0, plus one for cpu 2.
        // 2-core HT: cores 0/1, 2/3.
        // Expanded: 0, 0, 2. Core 0: occupancy 0->pass 0, 1->pass 1. Core 1: occupancy 0->pass 0.
        // Sort: pass 0 (cpu 0 order=0, cpu 2 order=2), pass 1 (cpu 0 order=1)
        // Result: 0, 2, 0
        {
            TimeSpan ts;
            ts.SetSystem(&sys2coreHT);
            ts.SetAffinityTraversal(AffinityTraversal::CoreAware);
            ts.AddAffinityGroupMask((WORD)0, (KAFFINITY)0x1);
            ts.AddAffinityGroupMask((WORD)0, (KAFFINITY)0x1);
            ts.AddAffinityGroupMask((WORD)0, (KAFFINITY)0x4);
            ts.Finalize();

            const auto& v = ts.GetEffectiveAffinityAssignments();
            VERIFY_ARE_EQUAL(v.size(), (size_t)3);
            VERIFY_ARE_EQUAL(v[0].bProc, (BYTE)0);
            VERIFY_ARE_EQUAL(v[1].bProc, (BYTE)2);
            VERIFY_ARE_EQUAL(v[2].bProc, (BYTE)0);
        }

        // Case 16: Triple repetition of same CPU - stresses occupancy beyond HT depth.
        // Three mask entries all specifying cpu 0 on a 2-core HT system.
        // Expanded: 0, 0, 0. Core 0: pass 0, pass 1, pass 2.
        // Result: 0, 0, 0 (all same group/eff, sorted by pass then order)
        {
            TimeSpan ts;
            ts.SetSystem(&sys2coreHT);
            ts.SetAffinityTraversal(AffinityTraversal::CoreAware);
            ts.AddAffinityGroupMask((WORD)0, (KAFFINITY)0x1);
            ts.AddAffinityGroupMask((WORD)0, (KAFFINITY)0x1);
            ts.AddAffinityGroupMask((WORD)0, (KAFFINITY)0x1);
            ts.Finalize();

            const auto& v = ts.GetEffectiveAffinityAssignments();
            VERIFY_ARE_EQUAL(v.size(), (size_t)3);
            VERIFY_ARE_EQUAL(v[0].bProc, (BYTE)0);
            VERIFY_ARE_EQUAL(v[1].bProc, (BYTE)0);
            VERIFY_ARE_EQUAL(v[2].bProc, (BYTE)0);
        }

        // Case 17: Disabled affinity - effective vector should be empty
        {
            TimeSpan ts;
            ts.SetSystem(&sys2coreHT);
            ts.SetAffinityTraversal(AffinityTraversal::CoreAware);
            ts.SetDisableAffinity(true);
            ts.AddAffinityGroupMask((WORD)0, (KAFFINITY)0xF);
            ts.Finalize();

            const auto& v = ts.GetEffectiveAffinityAssignments();
            VERIFY_IS_TRUE(v.empty());
        }

        // Case 18: Default (no masks specified) - synthesizes from all groups, CoreAware
        // 2-core HT system. Expect: 0,2,1,3 (core-aware ordering of full system)
        {
            TimeSpan ts;
            ts.SetSystem(&sys2coreHT);
            ts.SetAffinityTraversal(AffinityTraversal::CoreAware);
            ts.Finalize();

            const auto& v = ts.GetEffectiveAffinityAssignments();
            VERIFY_ARE_EQUAL(v.size(), (size_t)4);
            VERIFY_ARE_EQUAL(v[0].bProc, (BYTE)0);
            VERIFY_ARE_EQUAL(v[1].bProc, (BYTE)2);
            VERIFY_ARE_EQUAL(v[2].bProc, (BYTE)1);
            VERIFY_ARE_EQUAL(v[3].bProc, (BYTE)3);
        }

        // Case 19: P-only mask on heterogeneous system
        // P-cores: 0/1 (eff=1), 2/3 (eff=1). E-cores: 4 (eff=0), 5 (eff=0).
        // Mask: 0xF (cpus 0-3, all P-cores). All same eff class, pass 0 then pass 1.
        // Expect: 0,2,1,3
        {
            TimeSpan ts;
            ts.SetSystem(&sysHetero);
            ts.SetAffinityTraversal(AffinityTraversal::CoreAware);
            ts.AddAffinityGroupMask((WORD)0, (KAFFINITY)0xF);
            ts.Finalize();

            const auto& v = ts.GetEffectiveAffinityAssignments();
            VERIFY_ARE_EQUAL(v.size(), (size_t)4);
            VERIFY_ARE_EQUAL(v[0].bProc, (BYTE)0);
            VERIFY_ARE_EQUAL(v[1].bProc, (BYTE)2);
            VERIFY_ARE_EQUAL(v[2].bProc, (BYTE)1);
            VERIFY_ARE_EQUAL(v[3].bProc, (BYTE)3);
        }

        // Case 20: Multi-group heterogeneous, CoreAware + Span
        // Group 0: 2 P-cores HT (0/1, 2/3 eff=1). Group 1: 2 E-cores no HT (0, 1 eff=0).
        // SystemWide: pass 0 P (g0/0, g0/2), pass 0 E (g1/0, g1/1), pass 1 P (g0/1, g0/3)
        {
            SystemInformation system;
            system.processorTopology._vProcessorGroupInformation.clear();
            system.processorTopology._vProcessorGroupInformation.emplace_back((WORD)0, (BYTE)4, (BYTE)4, (KAFFINITY)0xF);
            system.processorTopology._vProcessorGroupInformation.emplace_back((WORD)1, (BYTE)2, (BYTE)2, (KAFFINITY)0x3);
            system.processorTopology._vProcessorCoreInformation.clear();
            system.processorTopology._vProcessorCoreInformation.emplace_back((WORD)0, (KAFFINITY)0x3, (BYTE)1);
            system.processorTopology._vProcessorCoreInformation.emplace_back((WORD)0, (KAFFINITY)0xC, (BYTE)1);
            system.processorTopology._vProcessorCoreInformation.emplace_back((WORD)1, (KAFFINITY)0x1, (BYTE)0);
            system.processorTopology._vProcessorCoreInformation.emplace_back((WORD)1, (KAFFINITY)0x2, (BYTE)0);
            system.processorTopology._ubPerformanceEfficiencyClass = 1;
            system.processorTopology._fSMT = true;

            TimeSpan ts;
            ts.SetSystem(&system);
            ts.SetAffinityTraversal(AffinityTraversal::CoreAware);
            ts.SetAffinityTraversal(AffinityTraversal::CoreAware);
            ts.SetAffinityGroupSpan(AffinityGroupSpan::Span);
            ts.AddAffinityGroupMask((WORD)0, (KAFFINITY)0xF);
            ts.AddAffinityGroupMask((WORD)1, (KAFFINITY)0x3);
            ts.Finalize();

            const auto& v = ts.GetEffectiveAffinityAssignments();
            VERIFY_ARE_EQUAL(v.size(), (size_t)6);
            // pass 0, P-cores (eff=1): g0/0, g0/2
            VERIFY_ARE_EQUAL(v[0].wGroup, (WORD)0); VERIFY_ARE_EQUAL(v[0].bProc, (BYTE)0);
            VERIFY_ARE_EQUAL(v[1].wGroup, (WORD)0); VERIFY_ARE_EQUAL(v[1].bProc, (BYTE)2);
            // pass 0, E-cores (eff=0): g1/0, g1/1
            VERIFY_ARE_EQUAL(v[2].wGroup, (WORD)1); VERIFY_ARE_EQUAL(v[2].bProc, (BYTE)0);
            VERIFY_ARE_EQUAL(v[3].wGroup, (WORD)1); VERIFY_ARE_EQUAL(v[3].bProc, (BYTE)1);
            // pass 1, P-cores: g0/1, g0/3
            VERIFY_ARE_EQUAL(v[4].wGroup, (WORD)0); VERIFY_ARE_EQUAL(v[4].bProc, (BYTE)1);
            VERIFY_ARE_EQUAL(v[5].wGroup, (WORD)0); VERIFY_ARE_EQUAL(v[5].bProc, (BYTE)3);
        }

        // Case 21: Single CPU mask - just one bit set on an HT system
        // Cores: 0/1, 2/3. Mask: 0x4 (cpu 2 only). Expect: just cpu 2.
        {
            TimeSpan ts;
            ts.SetSystem(&sys2coreHT);
            ts.SetAffinityTraversal(AffinityTraversal::CoreAware);
            ts.AddAffinityGroupMask((WORD)0, (KAFFINITY)0x4);
            ts.Finalize();

            const auto& v = ts.GetEffectiveAffinityAssignments();
            VERIFY_ARE_EQUAL(v.size(), (size_t)1);
            VERIFY_ARE_EQUAL(v[0].bProc, (BYTE)2);
        }

        // Case 22: Interleaved groups in manual spec - g0 cpu, g1 cpu, g0 cpu, g1 cpu
        // 2-group HT: group 0 cores 0/1, 2/3; group 1 cores 0/1, 2/3.
        // Masks: {g0, 0x1}, {g1, 0x1}, {g0, 0x4}, {g1, 0x4}
        // Expanded: g0/0, g1/0, g0/2, g1/2.
        // All pass 0 (each is first in its respective core).
        // CoreAware sort: group 0 first (g0/0, g0/2), then group 1 (g1/0, g1/2).
        {
            TimeSpan ts;
            ts.SetSystem(&sys2groupHT);
            ts.SetAffinityTraversal(AffinityTraversal::CoreAware);
            ts.AddAffinityGroupMask((WORD)0, (KAFFINITY)0x1);
            ts.AddAffinityGroupMask((WORD)1, (KAFFINITY)0x1);
            ts.AddAffinityGroupMask((WORD)0, (KAFFINITY)0x4);
            ts.AddAffinityGroupMask((WORD)1, (KAFFINITY)0x4);
            ts.Finalize();

            const auto& v = ts.GetEffectiveAffinityAssignments();
            VERIFY_ARE_EQUAL(v.size(), (size_t)4);
            VERIFY_ARE_EQUAL(v[0].wGroup, (WORD)0); VERIFY_ARE_EQUAL(v[0].bProc, (BYTE)0);
            VERIFY_ARE_EQUAL(v[1].wGroup, (WORD)0); VERIFY_ARE_EQUAL(v[1].bProc, (BYTE)2);
            VERIFY_ARE_EQUAL(v[2].wGroup, (WORD)1); VERIFY_ARE_EQUAL(v[2].bProc, (BYTE)0);
            VERIFY_ARE_EQUAL(v[3].wGroup, (WORD)1); VERIFY_ARE_EQUAL(v[3].bProc, (BYTE)2);
        }

        // Case 23: Mask that straddles a core boundary - cpus 1,2 on 4-core HT
        // Cores: 0/1, 2/3. Cpu 1 is in core 0, cpu 2 is in core 1.
        // Both are pass 0 (each is first representative of a different core).
        // Expect: 1, 2 (no reordering needed, both unique-core pass 0)
        {
            TimeSpan ts;
            ts.SetSystem(&sys2coreHT);
            ts.SetAffinityTraversal(AffinityTraversal::CoreAware);
            ts.AddAffinityGroupMask((WORD)0, (KAFFINITY)0x6);
            ts.Finalize();

            const auto& v = ts.GetEffectiveAffinityAssignments();
            VERIFY_ARE_EQUAL(v.size(), (size_t)2);
            VERIFY_ARE_EQUAL(v[0].bProc, (BYTE)1);
            VERIFY_ARE_EQUAL(v[1].bProc, (BYTE)2);
        }

        // Case 24: Large system simulation - 8 cores, 4 HT + 4 non-HT (mixed SMT width)
        // Cores 0-3: HT (0/1, 2/3, 4/5, 6/7) eff=0. Cores 4-7: no HT (8,9,10,11) eff=0.
        // Full mask 0xFFF. All same eff class.
        // Pass 0: 0,2,4,6,8,9,10,11 (one per core). Pass 1: 1,3,5,7 (HT siblings).
        {
            SystemInformation system;
            system.processorTopology._vProcessorGroupInformation.clear();
            system.processorTopology._vProcessorGroupInformation.emplace_back((WORD)0, (BYTE)12, (BYTE)12, (KAFFINITY)0xFFF);
            system.processorTopology._vProcessorCoreInformation.clear();
            system.processorTopology._vProcessorCoreInformation.emplace_back((WORD)0, (KAFFINITY)0x3, (BYTE)0);
            system.processorTopology._vProcessorCoreInformation.emplace_back((WORD)0, (KAFFINITY)0xC, (BYTE)0);
            system.processorTopology._vProcessorCoreInformation.emplace_back((WORD)0, (KAFFINITY)0x30, (BYTE)0);
            system.processorTopology._vProcessorCoreInformation.emplace_back((WORD)0, (KAFFINITY)0xC0, (BYTE)0);
            system.processorTopology._vProcessorCoreInformation.emplace_back((WORD)0, (KAFFINITY)0x100, (BYTE)0);
            system.processorTopology._vProcessorCoreInformation.emplace_back((WORD)0, (KAFFINITY)0x200, (BYTE)0);
            system.processorTopology._vProcessorCoreInformation.emplace_back((WORD)0, (KAFFINITY)0x400, (BYTE)0);
            system.processorTopology._vProcessorCoreInformation.emplace_back((WORD)0, (KAFFINITY)0x800, (BYTE)0);
            system.processorTopology._ubPerformanceEfficiencyClass = 0;
            system.processorTopology._fSMT = true;

            TimeSpan ts;
            ts.SetSystem(&system);
            ts.SetAffinityTraversal(AffinityTraversal::CoreAware);
            ts.AddAffinityGroupMask((WORD)0, (KAFFINITY)0xFFF);
            ts.Finalize();

            const auto& v = ts.GetEffectiveAffinityAssignments();
            VERIFY_ARE_EQUAL(v.size(), (size_t)12);
            // pass 0: one per core
            VERIFY_ARE_EQUAL(v[0].bProc, (BYTE)0);
            VERIFY_ARE_EQUAL(v[1].bProc, (BYTE)2);
            VERIFY_ARE_EQUAL(v[2].bProc, (BYTE)4);
            VERIFY_ARE_EQUAL(v[3].bProc, (BYTE)6);
            VERIFY_ARE_EQUAL(v[4].bProc, (BYTE)8);
            VERIFY_ARE_EQUAL(v[5].bProc, (BYTE)9);
            VERIFY_ARE_EQUAL(v[6].bProc, (BYTE)10);
            VERIFY_ARE_EQUAL(v[7].bProc, (BYTE)11);
            // pass 1: HT siblings (only from the 4 HT cores)
            VERIFY_ARE_EQUAL(v[8].bProc, (BYTE)1);
            VERIFY_ARE_EQUAL(v[9].bProc, (BYTE)3);
            VERIFY_ARE_EQUAL(v[10].bProc, (BYTE)5);
            VERIFY_ARE_EQUAL(v[11].bProc, (BYTE)7);
        }

        // Case 25: EFirst on heterogeneous system
        // sysHetero: P cores 0/1, 2/3 (eff=1), E cores 4, 5 (eff=0)
        // EFirst: pass 0 E-cores (4,5), pass 0 P-cores (0,2), pass 1 P-cores (1,3)
        {
            TimeSpan ts;
            ts.SetSystem(&sysHetero);
            ts.SetAffinityTraversal(AffinityTraversal::CoreAware);
            ts.SetAffinityEfficiencyOrder(AffinityEfficiencyOrder::EFirst);
            ts.AddAffinityGroupMask((WORD)0, (KAFFINITY)0x3F);
            ts.Finalize();

            const auto& v = ts.GetEffectiveAffinityAssignments();
            VERIFY_ARE_EQUAL(v.size(), (size_t)6);
            VERIFY_ARE_EQUAL(v[0].bProc, (BYTE)4);
            VERIFY_ARE_EQUAL(v[1].bProc, (BYTE)5);
            VERIFY_ARE_EQUAL(v[2].bProc, (BYTE)0);
            VERIFY_ARE_EQUAL(v[3].bProc, (BYTE)2);
            VERIFY_ARE_EQUAL(v[4].bProc, (BYTE)1);
            VERIFY_ARE_EQUAL(v[5].bProc, (BYTE)3);
        }

        // Case 26: FillPFirst on heterogeneous system
        // FillPFirst: all P passes (0,2,1,3) before any E (4,5)
        {
            TimeSpan ts;
            ts.SetSystem(&sysHetero);
            ts.SetAffinityTraversal(AffinityTraversal::CoreAware);
            ts.SetAffinityEfficiencyOrder(AffinityEfficiencyOrder::FillPFirst);
            ts.AddAffinityGroupMask((WORD)0, (KAFFINITY)0x3F);
            ts.Finalize();

            const auto& v = ts.GetEffectiveAffinityAssignments();
            VERIFY_ARE_EQUAL(v.size(), (size_t)6);
            VERIFY_ARE_EQUAL(v[0].bProc, (BYTE)0);
            VERIFY_ARE_EQUAL(v[1].bProc, (BYTE)2);
            VERIFY_ARE_EQUAL(v[2].bProc, (BYTE)1);
            VERIFY_ARE_EQUAL(v[3].bProc, (BYTE)3);
            VERIFY_ARE_EQUAL(v[4].bProc, (BYTE)4);
            VERIFY_ARE_EQUAL(v[5].bProc, (BYTE)5);
        }

        // Case 27: FillEFirst on heterogeneous system
        // FillEFirst: all E passes (4,5) before any P (0,2,1,3)
        {
            TimeSpan ts;
            ts.SetSystem(&sysHetero);
            ts.SetAffinityTraversal(AffinityTraversal::CoreAware);
            ts.SetAffinityEfficiencyOrder(AffinityEfficiencyOrder::FillEFirst);
            ts.AddAffinityGroupMask((WORD)0, (KAFFINITY)0x3F);
            ts.Finalize();

            const auto& v = ts.GetEffectiveAffinityAssignments();
            VERIFY_ARE_EQUAL(v.size(), (size_t)6);
            VERIFY_ARE_EQUAL(v[0].bProc, (BYTE)4);
            VERIFY_ARE_EQUAL(v[1].bProc, (BYTE)5);
            VERIFY_ARE_EQUAL(v[2].bProc, (BYTE)0);
            VERIFY_ARE_EQUAL(v[3].bProc, (BYTE)2);
            VERIFY_ARE_EQUAL(v[4].bProc, (BYTE)1);
            VERIFY_ARE_EQUAL(v[5].bProc, (BYTE)3);
        }

        // Case 28: FillPFirst on homogeneous system - no effect (all same eff class)
        {
            TimeSpan ts;
            ts.SetSystem(&sys4coreHT);
            ts.SetAffinityTraversal(AffinityTraversal::CoreAware);
            ts.SetAffinityEfficiencyOrder(AffinityEfficiencyOrder::FillPFirst);
            ts.AddAffinityGroupMask((WORD)0, (KAFFINITY)0xFF);
            ts.Finalize();

            const auto& v = ts.GetEffectiveAffinityAssignments();
            VERIFY_ARE_EQUAL(v.size(), (size_t)8);
            // Same as default CoreAware: 0,2,4,6,1,3,5,7
            VERIFY_ARE_EQUAL(v[0].bProc, (BYTE)0);
            VERIFY_ARE_EQUAL(v[1].bProc, (BYTE)2);
            VERIFY_ARE_EQUAL(v[2].bProc, (BYTE)4);
            VERIFY_ARE_EQUAL(v[3].bProc, (BYTE)6);
            VERIFY_ARE_EQUAL(v[4].bProc, (BYTE)1);
            VERIFY_ARE_EQUAL(v[5].bProc, (BYTE)3);
            VERIFY_ARE_EQUAL(v[6].bProc, (BYTE)5);
            VERIFY_ARE_EQUAL(v[7].bProc, (BYTE)7);
        }

        // Case 29: FillEFirst + CoreAware + Span on multi-group heterogeneous
        // sys2groupHetero (from case 20): g0 P-cores HT, g1 E-cores no HT
        // FillEFirst+SystemWide: E all passes (g1/0, g1/1), then P all passes (g0/0, g0/2, g0/1, g0/3)
        {
            SystemInformation sys2groupHetero;
            sys2groupHetero.processorTopology._vProcessorGroupInformation.clear();
            sys2groupHetero.processorTopology._vProcessorGroupInformation.emplace_back((WORD)0, (BYTE)4, (BYTE)4, (KAFFINITY)0xF);
            sys2groupHetero.processorTopology._vProcessorGroupInformation.emplace_back((WORD)1, (BYTE)2, (BYTE)2, (KAFFINITY)0x3);
            sys2groupHetero.processorTopology._vProcessorCoreInformation.clear();
            sys2groupHetero.processorTopology._vProcessorCoreInformation.emplace_back((WORD)0, (KAFFINITY)0x3, (BYTE)1);
            sys2groupHetero.processorTopology._vProcessorCoreInformation.emplace_back((WORD)0, (KAFFINITY)0xC, (BYTE)1);
            sys2groupHetero.processorTopology._vProcessorCoreInformation.emplace_back((WORD)1, (KAFFINITY)0x1, (BYTE)0);
            sys2groupHetero.processorTopology._vProcessorCoreInformation.emplace_back((WORD)1, (KAFFINITY)0x2, (BYTE)0);
            sys2groupHetero.processorTopology._ubPerformanceEfficiencyClass = 1;
            sys2groupHetero.processorTopology._fSMT = true;

            TimeSpan ts;
            ts.SetSystem(&sys2groupHetero);
            ts.SetAffinityTraversal(AffinityTraversal::CoreAware);
            ts.SetAffinityTraversal(AffinityTraversal::CoreAware);
            ts.SetAffinityGroupSpan(AffinityGroupSpan::Span);
            ts.SetAffinityEfficiencyOrder(AffinityEfficiencyOrder::FillEFirst);
            ts.AddAffinityGroupMask((WORD)0, (KAFFINITY)0xF);
            ts.AddAffinityGroupMask((WORD)1, (KAFFINITY)0x3);
            ts.Finalize();

            const auto& v = ts.GetEffectiveAffinityAssignments();
            VERIFY_ARE_EQUAL(v.size(), (size_t)6);
            // E-cores all passes first
            VERIFY_ARE_EQUAL(v[0].wGroup, (WORD)1); VERIFY_ARE_EQUAL(v[0].bProc, (BYTE)0);
            VERIFY_ARE_EQUAL(v[1].wGroup, (WORD)1); VERIFY_ARE_EQUAL(v[1].bProc, (BYTE)1);
            // P-cores all passes
            VERIFY_ARE_EQUAL(v[2].wGroup, (WORD)0); VERIFY_ARE_EQUAL(v[2].bProc, (BYTE)0);
            VERIFY_ARE_EQUAL(v[3].wGroup, (WORD)0); VERIFY_ARE_EQUAL(v[3].bProc, (BYTE)2);
            VERIFY_ARE_EQUAL(v[4].wGroup, (WORD)0); VERIFY_ARE_EQUAL(v[4].bProc, (BYTE)1);
            VERIFY_ARE_EQUAL(v[5].wGroup, (WORD)0); VERIFY_ARE_EQUAL(v[5].bProc, (BYTE)3);
        }

        // Case 30: FillPFirst + CoreAware + Span on multi-group heterogeneous
        // Same topology as case 29. Symmetric counterpart.
        // FillPFirst+SystemWide: P all passes (g0/0, g0/2, g0/1, g0/3), then E all passes (g1/0, g1/1)
        {
            SystemInformation sys2groupHetero;
            sys2groupHetero.processorTopology._vProcessorGroupInformation.clear();
            sys2groupHetero.processorTopology._vProcessorGroupInformation.emplace_back((WORD)0, (BYTE)4, (BYTE)4, (KAFFINITY)0xF);
            sys2groupHetero.processorTopology._vProcessorGroupInformation.emplace_back((WORD)1, (BYTE)2, (BYTE)2, (KAFFINITY)0x3);
            sys2groupHetero.processorTopology._vProcessorCoreInformation.clear();
            sys2groupHetero.processorTopology._vProcessorCoreInformation.emplace_back((WORD)0, (KAFFINITY)0x3, (BYTE)1);
            sys2groupHetero.processorTopology._vProcessorCoreInformation.emplace_back((WORD)0, (KAFFINITY)0xC, (BYTE)1);
            sys2groupHetero.processorTopology._vProcessorCoreInformation.emplace_back((WORD)1, (KAFFINITY)0x1, (BYTE)0);
            sys2groupHetero.processorTopology._vProcessorCoreInformation.emplace_back((WORD)1, (KAFFINITY)0x2, (BYTE)0);
            sys2groupHetero.processorTopology._ubPerformanceEfficiencyClass = 1;
            sys2groupHetero.processorTopology._fSMT = true;

            TimeSpan ts;
            ts.SetSystem(&sys2groupHetero);
            ts.SetAffinityTraversal(AffinityTraversal::CoreAware);
            ts.SetAffinityTraversal(AffinityTraversal::CoreAware);
            ts.SetAffinityGroupSpan(AffinityGroupSpan::Span);
            ts.SetAffinityEfficiencyOrder(AffinityEfficiencyOrder::FillPFirst);
            ts.AddAffinityGroupMask((WORD)0, (KAFFINITY)0xF);
            ts.AddAffinityGroupMask((WORD)1, (KAFFINITY)0x3);
            ts.Finalize();

            const auto& v = ts.GetEffectiveAffinityAssignments();
            VERIFY_ARE_EQUAL(v.size(), (size_t)6);
            // P-cores all passes first
            VERIFY_ARE_EQUAL(v[0].wGroup, (WORD)0); VERIFY_ARE_EQUAL(v[0].bProc, (BYTE)0);
            VERIFY_ARE_EQUAL(v[1].wGroup, (WORD)0); VERIFY_ARE_EQUAL(v[1].bProc, (BYTE)2);
            VERIFY_ARE_EQUAL(v[2].wGroup, (WORD)0); VERIFY_ARE_EQUAL(v[2].bProc, (BYTE)1);
            VERIFY_ARE_EQUAL(v[3].wGroup, (WORD)0); VERIFY_ARE_EQUAL(v[3].bProc, (BYTE)3);
            // E-cores all passes
            VERIFY_ARE_EQUAL(v[4].wGroup, (WORD)1); VERIFY_ARE_EQUAL(v[4].bProc, (BYTE)0);
            VERIFY_ARE_EQUAL(v[5].wGroup, (WORD)1); VERIFY_ARE_EQUAL(v[5].bProc, (BYTE)1);
        }

        // Case 31: Cpu + EFirst on heterogeneous system - stable partition, E-cores first
        // sysHetero: P 0/1, 2/3 (eff=1), E 4, 5 (eff=0). Mask 0x3F.
        // Expanded: 0,1,2,3,4,5. Stable partition E-first: 4,5,0,1,2,3
        {
            TimeSpan ts;
            ts.SetSystem(&sysHetero);
           ts.SetAffinityTraversal(AffinityTraversal::Cpu);
            ts.SetAffinityEfficiencyOrder(AffinityEfficiencyOrder::EFirst);
            ts.AddAffinityGroupMask((WORD)0, (KAFFINITY)0x3F);
            ts.Finalize();

            const auto& v = ts.GetEffectiveAffinityAssignments();
            VERIFY_ARE_EQUAL(v.size(), (size_t)6);
            VERIFY_ARE_EQUAL(v[0].bProc, (BYTE)4);
            VERIFY_ARE_EQUAL(v[1].bProc, (BYTE)5);
            VERIFY_ARE_EQUAL(v[2].bProc, (BYTE)0);
            VERIFY_ARE_EQUAL(v[3].bProc, (BYTE)1);
            VERIFY_ARE_EQUAL(v[4].bProc, (BYTE)2);
            VERIFY_ARE_EQUAL(v[5].bProc, (BYTE)3);
        }

        // Case 32: Cpu + FillPFirst on heterogeneous
        // P/AllP are equivalent in Cpu mode (no passes); both partition P-cores to front.
        // Expanded: 0,1,2,3,4,5. P-first partition: 0,1,2,3,4,5 (already P-first in LSB order)
        {
            TimeSpan ts;
            ts.SetSystem(&sysHetero);
           ts.SetAffinityTraversal(AffinityTraversal::Cpu);
            ts.SetAffinityEfficiencyOrder(AffinityEfficiencyOrder::FillPFirst);
            ts.AddAffinityGroupMask((WORD)0, (KAFFINITY)0x3F);
            ts.Finalize();

            const auto& v = ts.GetEffectiveAffinityAssignments();
            VERIFY_ARE_EQUAL(v.size(), (size_t)6);
            VERIFY_ARE_EQUAL(v[0].bProc, (BYTE)0);
            VERIFY_ARE_EQUAL(v[1].bProc, (BYTE)1);
            VERIFY_ARE_EQUAL(v[2].bProc, (BYTE)2);
            VERIFY_ARE_EQUAL(v[3].bProc, (BYTE)3);
            VERIFY_ARE_EQUAL(v[4].bProc, (BYTE)4);
            VERIFY_ARE_EQUAL(v[5].bProc, (BYTE)5);
        }

        // Case 33: Cpu + EFirst on homogeneous - no effect
        {
            TimeSpan ts;
            ts.SetSystem(&sys4coreHT);
           ts.SetAffinityTraversal(AffinityTraversal::Cpu);
            ts.SetAffinityEfficiencyOrder(AffinityEfficiencyOrder::EFirst);
            ts.AddAffinityGroupMask((WORD)0, (KAFFINITY)0xFF);
            ts.Finalize();

            const auto& v = ts.GetEffectiveAffinityAssignments();
            VERIFY_ARE_EQUAL(v.size(), (size_t)8);
            // No reordering: 0,1,2,3,4,5,6,7
            for (BYTE i = 0; i < 8; i++)
            {
                VERIFY_ARE_EQUAL(v[i].bProc, i);
            }
        }

        // Case 34: ARM-style default CoreAware (Unspecified, resolves to PFirst)
        // E: cpus 0,1 (eff=0). P: cpus 2/3, 4/5 (eff=1, HT).
        // Unspecified resolves to PFirst: pass 0 P (2,4), pass 0 E (0,1), pass 1 P (3,5)
        {
            TimeSpan ts;
            ts.SetSystem(&sysHeteroARM);
            ts.SetAffinityTraversal(AffinityTraversal::CoreAware);
            ts.AddAffinityGroupMask((WORD)0, (KAFFINITY)0x3F);
            ts.Finalize();

            const auto& v = ts.GetEffectiveAffinityAssignments();
            VERIFY_ARE_EQUAL(v.size(), (size_t)6);
            VERIFY_ARE_EQUAL(v[0].bProc, (BYTE)2);
            VERIFY_ARE_EQUAL(v[1].bProc, (BYTE)4);
            VERIFY_ARE_EQUAL(v[2].bProc, (BYTE)0);
            VERIFY_ARE_EQUAL(v[3].bProc, (BYTE)1);
            VERIFY_ARE_EQUAL(v[4].bProc, (BYTE)3);
            VERIFY_ARE_EQUAL(v[5].bProc, (BYTE)5);
        }

        // Case 35: ARM-style EFirst
        // EFirst: pass 0 E (0,1), pass 0 P (2,4), pass 1 P (3,5)
        {
            TimeSpan ts;
            ts.SetSystem(&sysHeteroARM);
            ts.SetAffinityTraversal(AffinityTraversal::CoreAware);
            ts.SetAffinityEfficiencyOrder(AffinityEfficiencyOrder::EFirst);
            ts.AddAffinityGroupMask((WORD)0, (KAFFINITY)0x3F);
            ts.Finalize();

            const auto& v = ts.GetEffectiveAffinityAssignments();
            VERIFY_ARE_EQUAL(v.size(), (size_t)6);
            VERIFY_ARE_EQUAL(v[0].bProc, (BYTE)0);
            VERIFY_ARE_EQUAL(v[1].bProc, (BYTE)1);
            VERIFY_ARE_EQUAL(v[2].bProc, (BYTE)2);
            VERIFY_ARE_EQUAL(v[3].bProc, (BYTE)4);
            VERIFY_ARE_EQUAL(v[4].bProc, (BYTE)3);
            VERIFY_ARE_EQUAL(v[5].bProc, (BYTE)5);
        }

        // Case 36: ARM-style FillPFirst
        // FillPFirst: P all passes (2,4,3,5), then E (0,1)
        {
            TimeSpan ts;
            ts.SetSystem(&sysHeteroARM);
            ts.SetAffinityTraversal(AffinityTraversal::CoreAware);
            ts.SetAffinityEfficiencyOrder(AffinityEfficiencyOrder::FillPFirst);
            ts.AddAffinityGroupMask((WORD)0, (KAFFINITY)0x3F);
            ts.Finalize();

            const auto& v = ts.GetEffectiveAffinityAssignments();
            VERIFY_ARE_EQUAL(v.size(), (size_t)6);
            VERIFY_ARE_EQUAL(v[0].bProc, (BYTE)2);
            VERIFY_ARE_EQUAL(v[1].bProc, (BYTE)4);
            VERIFY_ARE_EQUAL(v[2].bProc, (BYTE)3);
            VERIFY_ARE_EQUAL(v[3].bProc, (BYTE)5);
            VERIFY_ARE_EQUAL(v[4].bProc, (BYTE)0);
            VERIFY_ARE_EQUAL(v[5].bProc, (BYTE)1);
        }

        // Case 37: ARM-style FillEFirst
        // FillEFirst: E all passes (0,1), then P all passes (2,4,3,5)
        {
            TimeSpan ts;
            ts.SetSystem(&sysHeteroARM);
            ts.SetAffinityTraversal(AffinityTraversal::CoreAware);
            ts.SetAffinityEfficiencyOrder(AffinityEfficiencyOrder::FillEFirst);
            ts.AddAffinityGroupMask((WORD)0, (KAFFINITY)0x3F);
            ts.Finalize();

            const auto& v = ts.GetEffectiveAffinityAssignments();
            VERIFY_ARE_EQUAL(v.size(), (size_t)6);
            VERIFY_ARE_EQUAL(v[0].bProc, (BYTE)0);
            VERIFY_ARE_EQUAL(v[1].bProc, (BYTE)1);
            VERIFY_ARE_EQUAL(v[2].bProc, (BYTE)2);
            VERIFY_ARE_EQUAL(v[3].bProc, (BYTE)4);
            VERIFY_ARE_EQUAL(v[4].bProc, (BYTE)3);
            VERIFY_ARE_EQUAL(v[5].bProc, (BYTE)5);
        }

        // Case 38: ARM-style Cpu + EFirst (stable partition)
        // Expanded: 0,1,2,3,4,5. E-first partition: 0,1,2,3,4,5 (already E-first in LSB order)
        {
            TimeSpan ts;
            ts.SetSystem(&sysHeteroARM);
            ts.SetAffinityTraversal(AffinityTraversal::Cpu);
            ts.SetAffinityEfficiencyOrder(AffinityEfficiencyOrder::EFirst);
            ts.AddAffinityGroupMask((WORD)0, (KAFFINITY)0x3F);
            ts.Finalize();

            const auto& v = ts.GetEffectiveAffinityAssignments();
            VERIFY_ARE_EQUAL(v.size(), (size_t)6);
            VERIFY_ARE_EQUAL(v[0].bProc, (BYTE)0);
            VERIFY_ARE_EQUAL(v[1].bProc, (BYTE)1);
            VERIFY_ARE_EQUAL(v[2].bProc, (BYTE)2);
            VERIFY_ARE_EQUAL(v[3].bProc, (BYTE)3);
            VERIFY_ARE_EQUAL(v[4].bProc, (BYTE)4);
            VERIFY_ARE_EQUAL(v[5].bProc, (BYTE)5);
        }

        // Case 39: ARM-style Cpu + FillPFirst (stable partition, P-cores moved to front)
        // Expanded: 0,1,2,3,4,5. P-first partition: 2,3,4,5,0,1
        {
            TimeSpan ts;
            ts.SetSystem(&sysHeteroARM);
           ts.SetAffinityTraversal(AffinityTraversal::Cpu);
            ts.SetAffinityEfficiencyOrder(AffinityEfficiencyOrder::FillPFirst);
            ts.AddAffinityGroupMask((WORD)0, (KAFFINITY)0x3F);
            ts.Finalize();

            const auto& v = ts.GetEffectiveAffinityAssignments();
            VERIFY_ARE_EQUAL(v.size(), (size_t)6);
            VERIFY_ARE_EQUAL(v[0].bProc, (BYTE)2);
            VERIFY_ARE_EQUAL(v[1].bProc, (BYTE)3);
            VERIFY_ARE_EQUAL(v[2].bProc, (BYTE)4);
            VERIFY_ARE_EQUAL(v[3].bProc, (BYTE)5);
            VERIFY_ARE_EQUAL(v[4].bProc, (BYTE)0);
            VERIFY_ARE_EQUAL(v[5].bProc, (BYTE)1);
        }

        // Case 40: ARM-style Cpu + default (Unspecified resolves to PFirst + Fill)
        // P-cores sorted first within group: 2,3,4,5 (P) then 0,1 (E)
        {
            TimeSpan ts;
            ts.SetSystem(&sysHeteroARM);
            ts.SetAffinityTraversal(AffinityTraversal::Cpu);
            ts.AddAffinityGroupMask((WORD)0, (KAFFINITY)0x3F);
            ts.Finalize();

            const auto& v = ts.GetEffectiveAffinityAssignments();
            VERIFY_ARE_EQUAL(v.size(), (size_t)6);
            VERIFY_ARE_EQUAL(v[0].bProc, (BYTE)2);
            VERIFY_ARE_EQUAL(v[1].bProc, (BYTE)3);
            VERIFY_ARE_EQUAL(v[2].bProc, (BYTE)4);
            VERIFY_ARE_EQUAL(v[3].bProc, (BYTE)5);
            VERIFY_ARE_EQUAL(v[4].bProc, (BYTE)0);
            VERIFY_ARE_EQUAL(v[5].bProc, (BYTE)1);
        }

        // Case 41: ARM-style Cpu + Unordered (-aup) - no reordering, direct CPU order
        // E-cores first in ARM numbering preserved as-is: 0,1,2,3,4,5
        {
            TimeSpan ts;
            ts.SetSystem(&sysHeteroARM);
            ts.SetAffinityTraversal(AffinityTraversal::Cpu);
            ts.SetAffinityEfficiencyOrder(AffinityEfficiencyOrder::Unordered);
            ts.AddAffinityGroupMask((WORD)0, (KAFFINITY)0x3F);
            ts.Finalize();

            const auto& v = ts.GetEffectiveAffinityAssignments();
            VERIFY_ARE_EQUAL(v.size(), (size_t)6);
            VERIFY_ARE_EQUAL(v[0].bProc, (BYTE)0);
            VERIFY_ARE_EQUAL(v[1].bProc, (BYTE)1);
            VERIFY_ARE_EQUAL(v[2].bProc, (BYTE)2);
            VERIFY_ARE_EQUAL(v[3].bProc, (BYTE)3);
            VERIFY_ARE_EQUAL(v[4].bProc, (BYTE)4);
            VERIFY_ARE_EQUAL(v[5].bProc, (BYTE)5);
        }

        // Case 42: CoreAware + Unordered - pigeon-hole without efficiency preference
        // On sysHetero (P cores 0/1, 2/3; E cores 4, 5):
        // Pass 0: 0, 2, 4, 5 (first CPU from each core, in spec order)
        // Pass 1: 1, 3 (second CPU from HT P-cores)
        // No efficiency sorting within passes
        {
            TimeSpan ts;
            ts.SetSystem(&sysHetero);
            ts.SetAffinityTraversal(AffinityTraversal::CoreAware);
            ts.SetAffinityEfficiencyOrder(AffinityEfficiencyOrder::Unordered);
            ts.AddAffinityGroupMask((WORD)0, (KAFFINITY)0x3F);
            ts.Finalize();

            const auto& v = ts.GetEffectiveAffinityAssignments();
            VERIFY_ARE_EQUAL(v.size(), (size_t)6);
            // pass 0: cores in spec order (0, 2, 4, 5)
            VERIFY_ARE_EQUAL(v[0].bProc, (BYTE)0);
            VERIFY_ARE_EQUAL(v[1].bProc, (BYTE)2);
            VERIFY_ARE_EQUAL(v[2].bProc, (BYTE)4);
            VERIFY_ARE_EQUAL(v[3].bProc, (BYTE)5);
            // pass 1: HT siblings (1, 3)
            VERIFY_ARE_EQUAL(v[4].bProc, (BYTE)1);
            VERIFY_ARE_EQUAL(v[5].bProc, (BYTE)3);
        }
    }

    void ProfileUnitTests::Test_ValidateBypassIoConflict()
    {
        // BypassIO cross-target validation only applies to XML profiles
        // (fSingleSpec = false). Each target needs unbuffered IO + read-only
        // to pass per-target BypassIO checks.

        // Case 1: same path, same Partial mode -> passes
        {
            Profile profile;
            TimeSpan timeSpan;
            Target t1, t2;
            t1.SetPath("testfile.dat");
            t1.SetCacheMode(TargetCacheMode::DisableOSCache);
            t1.SetBypassIoMode(BypassIoMode::Partial);
            t2.SetPath("testfile.dat");
            t2.SetCacheMode(TargetCacheMode::DisableOSCache);
            t2.SetBypassIoMode(BypassIoMode::Partial);
            timeSpan.AddTarget(t1);
            timeSpan.AddTarget(t2);
            profile.AddTimeSpan(timeSpan);
            VERIFY_IS_TRUE(profile.Validate(false));
        }

        // Case 2: same path, same Full mode -> passes
        {
            Profile profile;
            TimeSpan timeSpan;
            Target t1, t2;
            t1.SetPath("testfile.dat");
            t1.SetCacheMode(TargetCacheMode::DisableOSCache);
            t1.SetBypassIoMode(BypassIoMode::Full);
            t2.SetPath("testfile.dat");
            t2.SetCacheMode(TargetCacheMode::DisableOSCache);
            t2.SetBypassIoMode(BypassIoMode::Full);
            timeSpan.AddTarget(t1);
            timeSpan.AddTarget(t2);
            profile.AddTimeSpan(timeSpan);
            VERIFY_IS_TRUE(profile.Validate(false));
        }

        // Case 3: same path, Partial vs Full -> fails
        {
            Profile profile;
            TimeSpan timeSpan;
            Target t1, t2;
            t1.SetPath("testfile.dat");
            t1.SetCacheMode(TargetCacheMode::DisableOSCache);
            t1.SetBypassIoMode(BypassIoMode::Partial);
            t2.SetPath("testfile.dat");
            t2.SetCacheMode(TargetCacheMode::DisableOSCache);
            t2.SetBypassIoMode(BypassIoMode::Full);
            timeSpan.AddTarget(t1);
            timeSpan.AddTarget(t2);
            profile.AddTimeSpan(timeSpan);
            VERIFY_IS_FALSE(profile.Validate(false));
        }

        // Case 4: same path, one with BypassIO and one without -> fails
        {
            Profile profile;
            TimeSpan timeSpan;
            Target t1, t2;
            t1.SetPath("testfile.dat");
            t1.SetCacheMode(TargetCacheMode::DisableOSCache);
            t1.SetBypassIoMode(BypassIoMode::Partial);
            t2.SetPath("testfile.dat");
            t2.SetCacheMode(TargetCacheMode::DisableOSCache);
            // t2 has BypassIoMode::Undefined (default)
            timeSpan.AddTarget(t1);
            timeSpan.AddTarget(t2);
            profile.AddTimeSpan(timeSpan);
            VERIFY_IS_FALSE(profile.Validate(false));
        }

        // Case 5: different paths with different BypassIO modes -> passes
        {
            Profile profile;
            TimeSpan timeSpan;
            Target t1, t2;
            t1.SetPath("file1.dat");
            t1.SetCacheMode(TargetCacheMode::DisableOSCache);
            t1.SetBypassIoMode(BypassIoMode::Partial);
            t2.SetPath("file2.dat");
            t2.SetCacheMode(TargetCacheMode::DisableOSCache);
            t2.SetBypassIoMode(BypassIoMode::Full);
            timeSpan.AddTarget(t1);
            timeSpan.AddTarget(t2);
            profile.AddTimeSpan(timeSpan);
            VERIFY_IS_TRUE(profile.Validate(false));
        }
    }

    void TimeSpanUnitTests::Test_TimeSpanGetXmlUseIoRing()
    {
        TimeSpan timeSpan;
        timeSpan.SetUseIoRing(true);
        string sXml = timeSpan.GetXml(0);
        const char *pcszExpected =
            "<TimeSpan>\n"
            "  <CompletionRoutines>false</CompletionRoutines>\n"
            "  <MeasureLatency>false</MeasureLatency>\n"
            "  <CalculateIopsStdDev>false</CalculateIopsStdDev>\n"
            "  <Duration>10</Duration>\n"
            "  <Warmup>5</Warmup>\n"
            "  <Cooldown>0</Cooldown>\n"
            "  <ThreadCount>0</ThreadCount>\n"
            "  <RequestCount>0</RequestCount>\n"
            "  <IoBucketDuration>1000</IoBucketDuration>\n"
            "  <RandSeed>0</RandSeed>\n"
            "  <IoRing>\n"
            "    <IoRingBatchSize>25</IoRingBatchSize>\n"
            "    <UseRegBuffer>false</UseRegBuffer>\n"
            "  </IoRing>\n"
            "  <DisableAffinity>false</DisableAffinity>\n"
            "  <AffinityTraversal Group=\"Fill\" Efficiency=\"PFirst\">Cpu</AffinityTraversal>\n"
            "  <BufferSeparation>PDECacheLine</BufferSeparation>\n"
            "  <Targets>\n"
            "  </Targets>\n"
            "</TimeSpan>\n";
        VERIFY_MULTILINE_EQUAL(pcszExpected, sXml);
    }

    void TimeSpanUnitTests::Test_TimeSpanGetXmlUseIoRingWithBatchSizeAndRegBuffer()
    {
        TimeSpan timeSpan;
        timeSpan.SetUseIoRing(true);
        timeSpan.SetIoRingBatchSize(75);
        timeSpan.SetUseRegBuffer(true);
        string sXml = timeSpan.GetXml(0);
        const char *pcszExpected =
            "<TimeSpan>\n"
            "  <CompletionRoutines>false</CompletionRoutines>\n"
            "  <MeasureLatency>false</MeasureLatency>\n"
            "  <CalculateIopsStdDev>false</CalculateIopsStdDev>\n"
            "  <Duration>10</Duration>\n"
            "  <Warmup>5</Warmup>\n"
            "  <Cooldown>0</Cooldown>\n"
            "  <ThreadCount>0</ThreadCount>\n"
            "  <RequestCount>0</RequestCount>\n"
            "  <IoBucketDuration>1000</IoBucketDuration>\n"
            "  <RandSeed>0</RandSeed>\n"
            "  <IoRing>\n"
            "    <IoRingBatchSize>75</IoRingBatchSize>\n"
            "    <UseRegBuffer>true</UseRegBuffer>\n"
            "  </IoRing>\n"
            "  <DisableAffinity>false</DisableAffinity>\n"
            "  <AffinityTraversal Group=\"Fill\" Efficiency=\"PFirst\">Cpu</AffinityTraversal>\n"
            "  <BufferSeparation>PDECacheLine</BufferSeparation>\n"
            "  <Targets>\n"
            "  </Targets>\n"
            "</TimeSpan>\n";
        VERIFY_MULTILINE_EQUAL(pcszExpected, sXml);
    }

    void TargetUnitTests::TestGetSetRandomDataWriteBufferSize()
    {
        Target t;
        VERIFY_ARE_EQUAL(t.GetRandomDataWriteBufferSize(), 0);
        t.SetRandomDataWriteBufferSize(1234);
        VERIFY_ARE_EQUAL(t.GetRandomDataWriteBufferSize(), 1234);
    }

    void TargetUnitTests::TestGetSetRandomDataWriteBufferSourcePath()
    {
        Target t;
        VERIFY_ARE_EQUAL(t.GetRandomDataWriteBufferSourcePath(), "");
        t.SetRandomDataWriteBufferSourcePath("x:\\foo\\bar.dat");
        VERIFY_ARE_EQUAL(t.GetRandomDataWriteBufferSourcePath(), "x:\\foo\\bar.dat");
    }

    void TargetUnitTests::Test_TargetGetXmlWriteBufferContentSequential()
    {
        Target target;
        string sXml = target.GetXml(0);
        const char *pcszExpected = "<Target>\n"
                               "  <Path></Path>\n"
                               "  <BlockSize>65536</BlockSize>\n"
                               "  <BaseFileOffset>0</BaseFileOffset>\n"
                               "  <SequentialScan>false</SequentialScan>\n"
                               "  <RandomAccess>false</RandomAccess>\n"
                               "  <TemporaryFile>false</TemporaryFile>\n"
                               "  <UseLargePages>false</UseLargePages>\n"
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
        VERIFY_MULTILINE_EQUAL(pcszExpected, sXml);
    }

    void TargetUnitTests::Test_TargetGetXmlWriteBufferContentZero()
    {
        Target target;
        target.SetZeroWriteBuffers(true);
        string sXml = target.GetXml(0);
        const char *pcszExpected = "<Target>\n"
                               "  <Path></Path>\n"
                               "  <BlockSize>65536</BlockSize>\n"
                               "  <BaseFileOffset>0</BaseFileOffset>\n"
                               "  <SequentialScan>false</SequentialScan>\n"
                               "  <RandomAccess>false</RandomAccess>\n"
                               "  <TemporaryFile>false</TemporaryFile>\n"
                               "  <UseLargePages>false</UseLargePages>\n"
                               "  <WriteBufferContent>\n"
                               "    <Pattern>zero</Pattern>\n"
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
        VERIFY_MULTILINE_EQUAL(pcszExpected, sXml);
    }

    void TargetUnitTests::Test_TargetGetXmlWriteBufferContentRandomNoFilePath()
    {
        Target target;
        target.SetRandomDataWriteBufferSize(224433);
        string sXml = target.GetXml(0);
        const char *pcszExpected = "<Target>\n"
                               "  <Path></Path>\n"
                               "  <BlockSize>65536</BlockSize>\n"
                               "  <BaseFileOffset>0</BaseFileOffset>\n"
                               "  <SequentialScan>false</SequentialScan>\n"
                               "  <RandomAccess>false</RandomAccess>\n"
                               "  <TemporaryFile>false</TemporaryFile>\n"
                               "  <UseLargePages>false</UseLargePages>\n"
                               "  <WriteBufferContent>\n"
                               "    <Pattern>random</Pattern>\n"
                               "    <RandomDataSource>\n"
                               "      <SizeInBytes>224433</SizeInBytes>\n"
                               "    </RandomDataSource>\n"
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
        VERIFY_MULTILINE_EQUAL(pcszExpected, sXml);
    }

    void TargetUnitTests::Test_TargetGetXmlWriteBufferContentRandomWithFilePath()
    {
        Target target;
        target.SetRandomDataWriteBufferSize(224433);
        target.SetRandomDataWriteBufferSourcePath("x:\\foo\\bar.baz");
        string sXml = target.GetXml(0);
        const char *pcszExpected = "<Target>\n"
                               "  <Path></Path>\n"
                               "  <BlockSize>65536</BlockSize>\n"
                               "  <BaseFileOffset>0</BaseFileOffset>\n"
                               "  <SequentialScan>false</SequentialScan>\n"
                               "  <RandomAccess>false</RandomAccess>\n"
                               "  <TemporaryFile>false</TemporaryFile>\n"
                               "  <UseLargePages>false</UseLargePages>\n"
                               "  <WriteBufferContent>\n"
                               "    <Pattern>random</Pattern>\n"
                               "    <RandomDataSource>\n"
                               "      <SizeInBytes>224433</SizeInBytes>\n"
                               "      <FilePath>x:\\foo\\bar.baz</FilePath>\n"
                               "    </RandomDataSource>\n"
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
        VERIFY_MULTILINE_EQUAL(pcszExpected, sXml);
    }

    void TargetUnitTests::Test_TargetGetXmlDisableAllCache()
    {
        Target target;
        target.SetCacheMode(TargetCacheMode::DisableOSCache);
        target.SetWriteThroughMode(WriteThroughMode::On);
        string sXml = target.GetXml(0);
        const char *pcszExpected = "<Target>\n"
            "  <Path></Path>\n"
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
            "  <Throughput>0</Throughput>\n"
            "  <ThreadsPerFile>1</ThreadsPerFile>\n"
            "  <IOPriority>3</IOPriority>\n"
            "  <Weight>1</Weight>\n"
            "</Target>\n";
        VERIFY_MULTILINE_EQUAL(pcszExpected, sXml);
    }

    void TargetUnitTests::Test_TargetGetXmlDisableLocalCache()
    {
        Target target;
        target.SetCacheMode(TargetCacheMode::DisableLocalCache);
        string sXml = target.GetXml(0);
        const char *pcszExpected = "<Target>\n"
            "  <Path></Path>\n"
            "  <BlockSize>65536</BlockSize>\n"
            "  <BaseFileOffset>0</BaseFileOffset>\n"
            "  <SequentialScan>false</SequentialScan>\n"
            "  <RandomAccess>false</RandomAccess>\n"
            "  <TemporaryFile>false</TemporaryFile>\n"
            "  <UseLargePages>false</UseLargePages>\n"
            "  <DisableLocalCache>true</DisableLocalCache>\n"
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
        VERIFY_MULTILINE_EQUAL(pcszExpected, sXml);
    }

    void TargetUnitTests::Test_TargetGetXmlDisableOSCache()
    {
        Target target;
        target.SetCacheMode(TargetCacheMode::DisableOSCache);
        string sXml = target.GetXml(0);
        const char *pcszExpected = "<Target>\n"
            "  <Path></Path>\n"
            "  <BlockSize>65536</BlockSize>\n"
            "  <BaseFileOffset>0</BaseFileOffset>\n"
            "  <SequentialScan>false</SequentialScan>\n"
            "  <RandomAccess>false</RandomAccess>\n"
            "  <TemporaryFile>false</TemporaryFile>\n"
            "  <UseLargePages>false</UseLargePages>\n"
            "  <DisableOSCache>true</DisableOSCache>\n"
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
        VERIFY_MULTILINE_EQUAL(pcszExpected, sXml);
    }

    void TargetUnitTests::Test_TargetGetXmlBufferedWriteThrough()
    {
        Target target;
        target.SetWriteThroughMode(WriteThroughMode::On);
        string sXml = target.GetXml(0);
        const char *pcszExpected = "<Target>\n"
            "  <Path></Path>\n"
            "  <BlockSize>65536</BlockSize>\n"
            "  <BaseFileOffset>0</BaseFileOffset>\n"
            "  <SequentialScan>false</SequentialScan>\n"
            "  <RandomAccess>false</RandomAccess>\n"
            "  <TemporaryFile>false</TemporaryFile>\n"
            "  <UseLargePages>false</UseLargePages>\n"
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
            "  <Throughput>0</Throughput>\n"
            "  <ThreadsPerFile>1</ThreadsPerFile>\n"
            "  <IOPriority>3</IOPriority>\n"
            "  <Weight>1</Weight>\n"
            "</Target>\n";
        VERIFY_MULTILINE_EQUAL(pcszExpected, sXml);
    }

    void TargetUnitTests::Test_TargetGetXmlMemoryMappedIo()
    {
        Target target;
        target.SetMemoryMappedIoMode(MemoryMappedIoMode::On);
        string sXml = target.GetXml(0);
        const char *pcszExpected = "<Target>\n"
            "  <Path></Path>\n"
            "  <BlockSize>65536</BlockSize>\n"
            "  <BaseFileOffset>0</BaseFileOffset>\n"
            "  <SequentialScan>false</SequentialScan>\n"
            "  <RandomAccess>false</RandomAccess>\n"
            "  <TemporaryFile>false</TemporaryFile>\n"
            "  <UseLargePages>false</UseLargePages>\n"
            "  <MemoryMappedIo>true</MemoryMappedIo>\n"
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
        VERIFY_MULTILINE_EQUAL(pcszExpected, sXml);
    }

    void TargetUnitTests::Test_TargetGetXmlMemoryMappedIoFlushModeViewOfFile()
    {
        Target target;
        target.SetMemoryMappedIoMode(MemoryMappedIoMode::On);
        target.SetMemoryMappedIoFlushMode(MemoryMappedIoFlushMode::ViewOfFile);
        string sXml = target.GetXml(0);
        const char *pcszExpected = "<Target>\n"
            "  <Path></Path>\n"
            "  <BlockSize>65536</BlockSize>\n"
            "  <BaseFileOffset>0</BaseFileOffset>\n"
            "  <SequentialScan>false</SequentialScan>\n"
            "  <RandomAccess>false</RandomAccess>\n"
            "  <TemporaryFile>false</TemporaryFile>\n"
            "  <UseLargePages>false</UseLargePages>\n"
            "  <MemoryMappedIo>true</MemoryMappedIo>\n"
            "  <FlushType>ViewOfFile</FlushType>\n"
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
        VERIFY_MULTILINE_EQUAL(pcszExpected, sXml);
    }

    void TargetUnitTests::Test_TargetGetXmlMemoryMappedIoFlushModeNonVolatileMemory()
    {
        Target target;
        target.SetMemoryMappedIoMode(MemoryMappedIoMode::On);
        target.SetMemoryMappedIoFlushMode(MemoryMappedIoFlushMode::NonVolatileMemory);
        string sXml = target.GetXml(0);
        const char *pcszExpected = "<Target>\n"
            "  <Path></Path>\n"
            "  <BlockSize>65536</BlockSize>\n"
            "  <BaseFileOffset>0</BaseFileOffset>\n"
            "  <SequentialScan>false</SequentialScan>\n"
            "  <RandomAccess>false</RandomAccess>\n"
            "  <TemporaryFile>false</TemporaryFile>\n"
            "  <UseLargePages>false</UseLargePages>\n"
            "  <MemoryMappedIo>true</MemoryMappedIo>\n"
            "  <FlushType>NonVolatileMemory</FlushType>\n"
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
        VERIFY_MULTILINE_EQUAL(pcszExpected, sXml);
    }

    void TargetUnitTests::Test_TargetGetXmlMemoryMappedIoFlushModeNonVolatileMemoryNoDrain()
    {
        Target target;
        target.SetMemoryMappedIoMode(MemoryMappedIoMode::On);
        target.SetMemoryMappedIoFlushMode(MemoryMappedIoFlushMode::NonVolatileMemoryNoDrain);
        string sXml = target.GetXml(0);
        const char *pcszExpected = "<Target>\n"
            "  <Path></Path>\n"
            "  <BlockSize>65536</BlockSize>\n"
            "  <BaseFileOffset>0</BaseFileOffset>\n"
            "  <SequentialScan>false</SequentialScan>\n"
            "  <RandomAccess>false</RandomAccess>\n"
            "  <TemporaryFile>false</TemporaryFile>\n"
            "  <UseLargePages>false</UseLargePages>\n"
            "  <MemoryMappedIo>true</MemoryMappedIo>\n"
            "  <FlushType>NonVolatileMemoryNoDrain</FlushType>\n"
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
        VERIFY_MULTILINE_EQUAL(pcszExpected, sXml);
    }

    void TargetUnitTests::Test_TargetGetXmlBypassIoModePartial()
    {
        Target target;
        target.SetCacheMode(TargetCacheMode::DisableOSCache);
        target.SetBypassIoMode(BypassIoMode::Partial);
        string sXml = target.GetXml(0);
        const char *pcszExpected =
            "<Target>\n"
            "  <Path></Path>\n"
            "  <BlockSize>65536</BlockSize>\n"
            "  <BaseFileOffset>0</BaseFileOffset>\n"
            "  <SequentialScan>false</SequentialScan>\n"
            "  <RandomAccess>false</RandomAccess>\n"
            "  <TemporaryFile>false</TemporaryFile>\n"
            "  <UseLargePages>false</UseLargePages>\n"
            "  <DisableOSCache>true</DisableOSCache>\n"
            "  <BypassIO>Partial</BypassIO>\n"
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
        VERIFY_MULTILINE_EQUAL(pcszExpected, sXml);
    }

    void TargetUnitTests::Test_TargetGetXmlBypassIoModeFull()
    {
        Target target;
        target.SetCacheMode(TargetCacheMode::DisableOSCache);
        target.SetBypassIoMode(BypassIoMode::Full);
        string sXml = target.GetXml(0);
        const char *pcszExpected =
            "<Target>\n"
            "  <Path></Path>\n"
            "  <BlockSize>65536</BlockSize>\n"
            "  <BaseFileOffset>0</BaseFileOffset>\n"
            "  <SequentialScan>false</SequentialScan>\n"
            "  <RandomAccess>false</RandomAccess>\n"
            "  <TemporaryFile>false</TemporaryFile>\n"
            "  <UseLargePages>false</UseLargePages>\n"
            "  <DisableOSCache>true</DisableOSCache>\n"
            "  <BypassIO>Full</BypassIO>\n"
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
        VERIFY_MULTILINE_EQUAL(pcszExpected, sXml);
    }

    void TargetUnitTests::Test_TargetGetXmlRandomAccessHint()
    {
        Target target;
        target.SetRandomAccessHint(true);
        string sXml = target.GetXml(0);
        const char *pcszExpected = "<Target>\n"
            "  <Path></Path>\n"
            "  <BlockSize>65536</BlockSize>\n"
            "  <BaseFileOffset>0</BaseFileOffset>\n"
            "  <SequentialScan>false</SequentialScan>\n"
            "  <RandomAccess>true</RandomAccess>\n"
            "  <TemporaryFile>false</TemporaryFile>\n"
            "  <UseLargePages>false</UseLargePages>\n"
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
        VERIFY_MULTILINE_EQUAL(pcszExpected, sXml);
    }

    void TargetUnitTests::Test_TargetGetXmlSequentialScanHint()
    {
        Target target;
        target.SetSequentialScanHint(true);
        string sXml = target.GetXml(0);
        const char *pcszExpected = "<Target>\n"
            "  <Path></Path>\n"
            "  <BlockSize>65536</BlockSize>\n"
            "  <BaseFileOffset>0</BaseFileOffset>\n"
            "  <SequentialScan>true</SequentialScan>\n"
            "  <RandomAccess>false</RandomAccess>\n"
            "  <TemporaryFile>false</TemporaryFile>\n"
            "  <UseLargePages>false</UseLargePages>\n"
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
        VERIFY_MULTILINE_EQUAL(pcszExpected, sXml);
    }

    void TargetUnitTests::Test_TargetGetXmlCombinedAccessHint()
    {
        Target target;
        target.SetSequentialScanHint(true);
        target.SetTemporaryFileHint(true);
        string sXml = target.GetXml(0);
        const char *pcszExpected = "<Target>\n"
            "  <Path></Path>\n"
            "  <BlockSize>65536</BlockSize>\n"
            "  <BaseFileOffset>0</BaseFileOffset>\n"
            "  <SequentialScan>true</SequentialScan>\n"
            "  <RandomAccess>false</RandomAccess>\n"
            "  <TemporaryFile>true</TemporaryFile>\n"
            "  <UseLargePages>false</UseLargePages>\n"
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
        VERIFY_MULTILINE_EQUAL(pcszExpected, sXml);
    }

    void TargetUnitTests::Test_AllocateAndFillRandomDataWriteBuffer()
    {
        Random r;
        Target t;
        VERIFY_IS_FALSE(t.AllocateAndFillRandomDataWriteBuffer(&r));
        VERIFY_ARE_EQUAL(t._pRandomDataWriteBuffer, nullptr);

        size_t cb = 12345;
        t.SetRandomDataWriteBufferSize(cb);
        VERIFY_IS_TRUE(t.AllocateAndFillRandomDataWriteBuffer(&r));
        VERIFY_IS_TRUE(t._pRandomDataWriteBuffer != nullptr);
        // see if the test crashes if we try to write to every byte of the buffer

        for (size_t i = 0; i < cb; i++)
        {
            t._pRandomDataWriteBuffer[i] = (i % 256);
        }

        for (size_t i = 0; i < cb; i++)
        {
            if (t._pRandomDataWriteBuffer[i] != (i % 256))
            {
                // don't call VERIFY_ARE_EQUAL on each item because it prints to the screen and makes the test take
                // too long
                VERIFY_IS_TRUE(false);
            }
        }
    }

    void TargetUnitTests::Test_AllocateAndFillRandomDataWriteBufferFromFile()
    {
        char szTempDirPath[MAX_PATH] = {};
        DWORD cch = GetTempPathA(_countof(szTempDirPath), szTempDirPath);
        VERIFY_IS_TRUE(cch != 0);
        string sTempFilePath(szTempDirPath);
        sTempFilePath += "diskspd-random-data-file.dat";
        DeleteFileA(sTempFilePath.c_str());

        printf("path: '%s'\n", sTempFilePath.c_str());
        FILE *pFile;
        fopen_s(&pFile, sTempFilePath.c_str(), "wb");
        VERIFY_IS_TRUE(pFile != nullptr);
        char buffer[256];
        for (int i = 0; i < 256; i++)
        {
            buffer[i] = static_cast<char>(0xFF - i);
        }
        VERIFY_ARE_EQUAL(fwrite(buffer, sizeof(buffer), 1, pFile), (size_t)1);
        fclose(pFile);
        pFile = nullptr;

        Random r;
        Target t;
        size_t cbBuffer = 1024 * 1024;
        t.SetRandomDataWriteBufferSize(cbBuffer);
        t.SetRandomDataWriteBufferSourcePath(sTempFilePath.c_str());
        VERIFY_IS_TRUE(t.AllocateAndFillRandomDataWriteBuffer(&r));
        VERIFY_IS_TRUE(t._pRandomDataWriteBuffer != nullptr);
        for (size_t i = 0; i < cbBuffer; i++)
        {
            if (t._pRandomDataWriteBuffer[i] != (0xFF - (i % 256)))
            {
                // don't call VERIFY_ARE_EQUAL on each item because it prints to the screen and makes the test take
                // too long
                VERIFY_IS_TRUE(false);
            }
        }

        DeleteFileA(sTempFilePath.c_str());
    }

    void ThreadParametersUnitTests::Test_AllocateAndFillBufferForTarget()
    {
        TimeSpan ts;
        ts.SetBufferSeparation(BufferSeparation::SystemDefault);
        ts.Finalize();

        Target t;
        Random r;
        t.SetBlockSizeInBytes(12345);
        t.SetRequestCount(12);
        ThreadParameters tp;
        tp.pTimeSpan = &ts;
        tp.pRand = &r;
        VERIFY_IS_TRUE(tp.AllocateAndFillBufferForTarget(t));

        // see if the test crashes if we try to write to every byte of the buffer
        size_t cb = t.GetBlockSizeInBytes() * t.GetRequestCount();
        for (size_t i = 0; i < cb; i++)
        {
            tp.vpDataBuffers[0][i] = (i % 256);
        }

        for (size_t i = 0; i < cb; i++)
        {
            if (tp.vpDataBuffers[0][i] != (i % 256))
            {
                // don't call VERIFY_ARE_EQUAL on each item because it prints to the screen and makes the test take
                // too long
                VERIFY_IS_TRUE(false);
            }
        }
    }

    void ThreadParametersUnitTests::Test_AllocateAndFillBufferForTarget_WriteSourceNoSeparation()
    {
        // When buffer separation is SystemDefault, the write source buffer
        // should NOT be replaced -- the thread shares the original pointer.
        TimeSpan ts;
        ts.SetBufferSeparation(BufferSeparation::SystemDefault);
        ts.Finalize();
        VERIFY_ARE_EQUAL(ts.GetEffectiveBufferSeparation(), (DWORD)0);

        Target t;
        Random r;
        t.SetBlockSizeInBytes(4096);
        t.SetRequestCount(2);
        t.SetWriteRatio(100);
        t.SetRandomDataWriteBufferSize(64 * 1024);
        VERIFY_IS_TRUE(t.AllocateAndFillRandomDataWriteBuffer(&r));
        BYTE *pOriginalBuffer = t._pRandomDataWriteBuffer;
        VERIFY_IS_TRUE(pOriginalBuffer != nullptr);
        VERIFY_IS_FALSE(t._fOwnsRandomDataWriteBuffer);

        ThreadParameters tp;
        tp.pTimeSpan = &ts;
        tp.pRand = &r;
        VERIFY_IS_TRUE(tp.AllocateAndFillBufferForTarget(t));

        // Write source buffer should be unchanged (shared, not separated)
        VERIFY_ARE_EQUAL(t._pRandomDataWriteBuffer, pOriginalBuffer);
        VERIFY_IS_FALSE(t._fOwnsRandomDataWriteBuffer);

        // Clean up -- since ownership was not taken, free explicitly
        VirtualFree(pOriginalBuffer, 0, MEM_RELEASE);
        t._pRandomDataWriteBuffer = nullptr;
    }

    void ThreadParametersUnitTests::Test_AllocateAndFillBufferForTarget_Aligned()
    {
        // Use buffer separation with known alignment.
        // Requires VirtualAlloc2 -- skip if unavailable.
        if (!ResolveVirtualAlloc2())
        {
            Log::Comment(L"VirtualAlloc2 not available, skipping aligned allocation test");
            return;
        }

        SystemInformation mockSystem;
        mockSystem.dwPageSize = 4096;
        mockSystem.processorTopology._vProcessorCacheInformation.emplace_back(
            (BYTE)3, (BYTE)12, (WORD)64, (DWORD)(8 * 1024 * 1024), CacheUnified);

        TimeSpan ts;
        ts.SetSystem(&mockSystem);
        ts.SetAffinityTraversal(AffinityTraversal::CoreAware);
        ts.SetBufferSeparation(BufferSeparation::PDECacheLine);
        ts.Finalize();
        VERIFY_IS_TRUE(ts.GetEffectiveBufferSeparation() > 0);

        Target t;
        Random r;
        t.SetBlockSizeInBytes(4096);
        t.SetRequestCount(4);
        ThreadParameters tp;
        tp.pTimeSpan = &ts;
        tp.pRand = &r;
        VERIFY_IS_TRUE(tp.AllocateAndFillBufferForTarget(t));
        VERIFY_IS_TRUE(tp.vpDataBuffers[0] != nullptr);

        // Verify the buffer is usable
        size_t cb = t.GetBlockSizeInBytes() * t.GetRequestCount();
        for (size_t i = 0; i < cb; i++)
        {
            tp.vpDataBuffers[0][i] = (i % 256);
        }
        for (size_t i = 0; i < cb; i++)
        {
            if (tp.vpDataBuffers[0][i] != (i % 256))
            {
                VERIFY_IS_TRUE(false);
            }
        }
    }

    void ThreadParametersUnitTests::Test_AllocateAndFillBufferForTarget_WriteSourceSeparation()
    {
        // Verify that buffer separation creates a per-thread copy of the
        // write source buffer when alignment is active.
        if (!ResolveVirtualAlloc2())
        {
            Log::Comment(L"VirtualAlloc2 not available, skipping write source separation test");
            return;
        }

        SystemInformation mockSystem;
        mockSystem.dwPageSize = 4096;
        mockSystem.processorTopology._vProcessorCacheInformation.emplace_back(
            (BYTE)3, (BYTE)12, (WORD)64, (DWORD)(8 * 1024 * 1024), CacheUnified);

        TimeSpan ts;
        ts.SetSystem(&mockSystem);
        ts.SetAffinityTraversal(AffinityTraversal::CoreAware);
        ts.SetBufferSeparation(BufferSeparation::PDECacheLine);
        ts.Finalize();

        Target t;
        Random r;
        t.SetBlockSizeInBytes(4096);
        t.SetRequestCount(2);
        t.SetWriteRatio(100);
        t.SetRandomDataWriteBufferSize(64 * 1024);
        VERIFY_IS_TRUE(t.AllocateAndFillRandomDataWriteBuffer(&r));
        BYTE *pOriginalBuffer = t._pRandomDataWriteBuffer;
        VERIFY_IS_TRUE(pOriginalBuffer != nullptr);

        ThreadParameters tp;
        tp.pTimeSpan = &ts;
        tp.pRand = &r;
        VERIFY_IS_TRUE(tp.AllocateAndFillBufferForTarget(t));

        // The write source buffer should have been replaced with a separated copy
        VERIFY_IS_TRUE(t._pRandomDataWriteBuffer != nullptr);
        VERIFY_ARE_NOT_EQUAL(t._pRandomDataWriteBuffer, pOriginalBuffer);
        VERIFY_IS_TRUE(t._fOwnsRandomDataWriteBuffer);

        // Clean up the original shared buffer (not owned by t since we didn't delegate)
        VirtualFree(pOriginalBuffer, 0, MEM_RELEASE);
    }

    void TopologyUnitTests::Test_MaskCount()
    {
        ULONG kaff_bits = sizeof(KAFFINITY) * 8;

        // a complete enumeration could be interesting, but a nibble is enough to test the algorithm.
        // take the given mask and its width (ordinal distance to the upper 1), shift it through the
        // range of KAFFINITY to verify the popcnt is correct at all positions
        //
        // note that unique masks have msb/lsb set for all combinations of the interior bits. we don't
        // test "10" (0x2) as a mask, because it's not unique - its the same as the first shift-up of
        // "1" (0x1), etc.

        struct {
            KAFFINITY mask;
            ULONG width;
            ULONG bits;
        } tests[] = {      // msb ... lsb
            { 0x1, 1, 1 }, //    1
            { 0x3, 2, 2 }, //   11
            { 0x5, 3, 2 }, //  101
            { 0x7, 3, 3 }, //  111
            { 0x9, 4, 2 }, // 1001
            { 0xb, 4, 3 }, // 1011
            { 0xd, 4, 3 }, // 1101
            { 0xf, 4, 4 }  // 1111
        };

        for (const auto &test : tests)
        {
            KAFFINITY mask = test.mask;
            for (ULONG i = 0; i < kaff_bits - test.width; i++)
            {
                VERIFY_ARE_EQUAL(ProcessorTopology::MaskCount(mask), test.bits);
                mask <<= 1;
            }
        }

        // ... and a few explicit true/false
        VERIFY_ARE_NOT_EQUAL(ProcessorTopology::MaskCount(0x0), (ULONG)1);
        VERIFY_ARE_NOT_EQUAL(ProcessorTopology::MaskCount(0x3), (ULONG)0);
        VERIFY_ARE_NOT_EQUAL(ProcessorTopology::MaskCount(0x5), (ULONG)3);

        VERIFY_ARE_EQUAL(ProcessorTopology::MaskCount(0x0), (ULONG)0);
        VERIFY_ARE_EQUAL(ProcessorTopology::MaskCount(0x3), (ULONG)2);
        VERIFY_ARE_EQUAL(ProcessorTopology::MaskCount(0x5), (ULONG)2);

        VERIFY_ARE_EQUAL(ProcessorTopology::MaskCount(0xffff), (ULONG)16);
        VERIFY_ARE_EQUAL(ProcessorTopology::MaskCount(0xfeef), (ULONG)14);
        VERIFY_ARE_EQUAL(ProcessorTopology::MaskCount(0xfeef00ff), (ULONG)22);
        VERIFY_ARE_EQUAL(ProcessorTopology::MaskCount(0xfe0000ff), (ULONG)15);
        VERIFY_ARE_EQUAL(ProcessorTopology::MaskCount(0x7e0000ff), (ULONG)14);
    }

    //
    // ProcessorTopologyUnitTests
    //

    // Helper to create a minimal SystemInformation with only cache information.
    static SystemInformation CreateTestSystemWithCaches(
        const vector<ProcessorCacheInformation>& caches)
    {
        SystemInformation system;
        system.ResetTime();
        system.sComputerName.clear();
        system.sActivePolicyName.clear();
        system.sActivePolicyGuid.clear();

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
        for (const auto& cache : caches)
        {
            system.processorTopology._vProcessorCacheInformation.push_back(cache);
        }

        return system;
    }

    void ProcessorTopologyUnitTests::Test_GetLargestCacheLineSize_SingleL3()
    {
        ProcessorCacheInformation cache(3, 16, 64, 16 * 1024 * 1024, CacheUnified);
        cache._processorMasks.emplace_back((WORD)0, (KAFFINITY)0xf);

        auto system = CreateTestSystemWithCaches({ cache });
        VERIFY_ARE_EQUAL((WORD)64, system.processorTopology.GetLargestCacheLineSize(3));
    }

    void ProcessorTopologyUnitTests::Test_GetLargestCacheLineSize_MultipleL3()
    {
        ProcessorCacheInformation cache1(3, 16, 64, 16 * 1024 * 1024, CacheUnified);
        cache1._processorMasks.emplace_back((WORD)0, (KAFFINITY)0xf);

        ProcessorCacheInformation cache2(3, 12, 128, 32 * 1024 * 1024, CacheUnified);
        cache2._processorMasks.emplace_back((WORD)0, (KAFFINITY)0xf0);

        auto system = CreateTestSystemWithCaches({ cache1, cache2 });
        VERIFY_ARE_EQUAL((WORD)128, system.processorTopology.GetLargestCacheLineSize(3));
    }

    void ProcessorTopologyUnitTests::Test_GetLargestCacheLineSize_NoL3()
    {
        ProcessorCacheInformation l1d(1, 12, 64, 32 * 1024, CacheData);
        l1d._processorMasks.emplace_back((WORD)0, (KAFFINITY)0x1);

        ProcessorCacheInformation l2(2, 8, 64, 256 * 1024, CacheUnified);
        l2._processorMasks.emplace_back((WORD)0, (KAFFINITY)0x1);

        auto system = CreateTestSystemWithCaches({ l1d, l2 });
        VERIFY_ARE_EQUAL((WORD)0, system.processorTopology.GetLargestCacheLineSize(3));
    }

    void ProcessorTopologyUnitTests::Test_GetLargestCacheLineSize_AllLevels()
    {
        // L1d has 64B line, L2 has 128B line - level 0 should return 128
        ProcessorCacheInformation l1d(1, 12, 64, 32 * 1024, CacheData);
        l1d._processorMasks.emplace_back((WORD)0, (KAFFINITY)0x1);

        ProcessorCacheInformation l2(2, 8, 128, 256 * 1024, CacheUnified);
        l2._processorMasks.emplace_back((WORD)0, (KAFFINITY)0x1);

        auto system = CreateTestSystemWithCaches({ l1d, l2 });
        VERIFY_ARE_EQUAL((WORD)128, system.processorTopology.GetLargestCacheLineSize(0));
        VERIFY_ARE_EQUAL((WORD)128, system.processorTopology.GetLargestCacheLineSize());
    }

    void ProcessorTopologyUnitTests::Test_GetLargestCacheLineSize_SpecificLevel()
    {
        // L1d: 64B line, L2: 128B line, L3: 64B line
        // Requesting L1 should return 64 even though L2 has 128
        ProcessorCacheInformation l1d(1, 12, 64, 32 * 1024, CacheData);
        l1d._processorMasks.emplace_back((WORD)0, (KAFFINITY)0x1);

        ProcessorCacheInformation l2(2, 8, 128, 256 * 1024, CacheUnified);
        l2._processorMasks.emplace_back((WORD)0, (KAFFINITY)0x1);

        ProcessorCacheInformation l3(3, 16, 64, 8 * 1024 * 1024, CacheUnified);
        l3._processorMasks.emplace_back((WORD)0, (KAFFINITY)0x1);

        auto system = CreateTestSystemWithCaches({ l1d, l2, l3 });
        VERIFY_ARE_EQUAL((WORD)64, system.processorTopology.GetLargestCacheLineSize(1));
        VERIFY_ARE_EQUAL((WORD)128, system.processorTopology.GetLargestCacheLineSize(2));
        VERIFY_ARE_EQUAL((WORD)64, system.processorTopology.GetLargestCacheLineSize(3));
        VERIFY_ARE_EQUAL((WORD)128, system.processorTopology.GetLargestCacheLineSize(0));
    }

    void ProcessorTopologyUnitTests::Test_GetXml_CacheTopology()
    {
        ProcessorCacheInformation l1d(1, 12, 64, 32 * 1024, CacheData);
        l1d._processorMasks.emplace_back((WORD)0, (KAFFINITY)0x1);

        ProcessorCacheInformation l3(3, 16, 64, 16 * 1024 * 1024, CacheUnified);
        l3._processorMasks.emplace_back((WORD)0, (KAFFINITY)0x1);

        auto system = CreateTestSystemWithCaches({ l1d, l3 });
        string sXml = system.processorTopology.GetXml(0, ProcessorTopology::Section::Cache);

        const char *pcszExpected =
            "<Cache Level=\"1\" Associativity=\"12\" LineSize=\"64\" CacheSize=\"32768\" Type=\"Data\">\n"
            "  <Group Group=\"0\" Mask=\"0x1\"/>\n"
            "</Cache>\n"
            "<Cache Level=\"3\" Associativity=\"16\" LineSize=\"64\" CacheSize=\"16777216\" Type=\"Unified\">\n"
            "  <Group Group=\"0\" Mask=\"0x1\"/>\n"
            "</Cache>\n";

        VERIFY_MULTILINE_EQUAL(pcszExpected, sXml);
    }

    void ProcessorTopologyUnitTests::Test_GetCacheText()
    {
        //
        // Single-group 4-CPU system with L1d/L1i/L2/L3.
        // Verifies single-group: column header is "CPU", no group prefix.
        // L1d entries are all single-cpu with same geometry -> merge with "(per cpu)".
        // L1i has a non-standard mask (0x5) that doesn't match any core -> plain.
        // L2 masks don't match any core (cores are single-cpu) -> plain.
        // L3 covers all CPUs but isn't a group (single-group) -> plain.
        //

        ProcessorCacheInformation l1d0(1, 12, 64, 32 * 1024, CacheData);
        l1d0._processorMasks.emplace_back((WORD)0, (KAFFINITY)0x1);
        ProcessorCacheInformation l1d1(1, 12, 64, 32 * 1024, CacheData);
        l1d1._processorMasks.emplace_back((WORD)0, (KAFFINITY)0x2);
        ProcessorCacheInformation l1d2(1, 12, 64, 32 * 1024, CacheData);
        l1d2._processorMasks.emplace_back((WORD)0, (KAFFINITY)0x4);
        ProcessorCacheInformation l1d3(1, 12, 64, 32 * 1024, CacheData);
        l1d3._processorMasks.emplace_back((WORD)0, (KAFFINITY)0x8);
        ProcessorCacheInformation l1i(1, 12, 64, 32 * 1024, CacheInstruction);
        l1i._processorMasks.emplace_back((WORD)0, (KAFFINITY)0x5); // CPUs 0,2
        ProcessorCacheInformation l2a(2, 8, 64, 256 * 1024, CacheUnified);
        l2a._processorMasks.emplace_back((WORD)0, (KAFFINITY)0x3);
        ProcessorCacheInformation l2b(2, 8, 64, 256 * 1024, CacheUnified);
        l2b._processorMasks.emplace_back((WORD)0, (KAFFINITY)0xc);
        ProcessorCacheInformation l3(3, 0xFF, 64, 6 * 1024 * 1024, CacheUnified);
        l3._processorMasks.emplace_back((WORD)0, (KAFFINITY)0xf);

        auto system = CreateTestSystemWithCaches({ l1d0, l1d1, l1d2, l1d3, l1i, l2a, l2b, l3 });

        // Expand to 4 CPUs
        system.processorTopology._ulProcessorCount = 4;
        system.processorTopology._vProcessorGroupInformation.clear();
        system.processorTopology._vProcessorGroupInformation.emplace_back((WORD)0, (BYTE)4, (BYTE)4, (KAFFINITY)0xf);
        system.processorTopology._vProcessorCoreInformation.clear();
        for (BYTE i = 0; i < 4; i++)
        {
            system.processorTopology._vProcessorCoreInformation.emplace_back((WORD)0, (KAFFINITY)1 << i, (BYTE)0);
        }

        string sText = system.processorTopology.GetText(0, ProcessorTopology::Section::Cache);

        const char *pcszExpected =
            "\ncache information:\n\n"
            "  Cache |   Size   | Line  | Assoc  | CPU\n"
            "  -------------------------------------------------------\n"
            "  L1d   |    32KiB |   64B | 12-way | 0-3 (per cpu)\n"
            "  L1i   |    32KiB |   64B | 12-way | 0,2\n"
            "  L2    |   256KiB |   64B |  8-way | 0-1\n"
            "  L2    |   256KiB |   64B |  8-way | 2-3\n"
            "  L3    |     6MiB |   64B |   full | 0-3\n";

        VERIFY_MULTILINE_EQUAL(pcszExpected, sText);
    }

    void ProcessorTopologyUnitTests::Test_GetCacheTextMultiGroup()
    {
        //
        // 2-group system. Verifies multi-group: column header is "Group/CPU".
        //

        ProcessorCacheInformation l2a(2, 8, 64, 256 * 1024, CacheUnified);
        l2a._processorMasks.emplace_back((WORD)0, (KAFFINITY)0x3);
        ProcessorCacheInformation l2b(2, 8, 64, 256 * 1024, CacheUnified);
        l2b._processorMasks.emplace_back((WORD)1, (KAFFINITY)0xc);
        ProcessorCacheInformation l3(3, 0xFF, 64, 16 * 1024 * 1024, CacheUnified);
        l3._processorMasks.emplace_back((WORD)0, (KAFFINITY)0xf);
        l3._processorMasks.emplace_back((WORD)1, (KAFFINITY)0xf);

        auto system = CreateTestSystemWithCaches({ l2a, l2b, l3 });

        // Expand to 2-group 8 CPUs
        system.processorTopology._ulProcessorCount = 8;
        system.processorTopology._vProcessorGroupInformation.clear();
        system.processorTopology._vProcessorGroupInformation.emplace_back((WORD)0, (BYTE)4, (BYTE)4, (KAFFINITY)0xf);
        system.processorTopology._vProcessorGroupInformation.emplace_back((WORD)1, (BYTE)4, (BYTE)4, (KAFFINITY)0xf);

        string sText = system.processorTopology.GetText(0, ProcessorTopology::Section::Cache);

        const char *pcszExpected =
            "\ncache information:\n\n"
            "  Cache |   Size   | Line  | Assoc  | Group/CPU\n"
            "  -----------------------------------------------------------\n"
            "  L2    |   256KiB |   64B |  8-way | 0/0-1\n"
            "  L2    |   256KiB |   64B |  8-way | 1/2-3\n"
            "  L3    |    16MiB |   64B |   full | 0/0-3 1/0-3\n";

        VERIFY_MULTILINE_EQUAL(pcszExpected, sText);
    }

    void ProcessorTopologyUnitTests::Test_CacheTypeName()
    {
        VERIFY_ARE_EQUAL(strcmp(ProcessorCacheInformation::TypeName(CacheUnified), "Unified"), 0);
        VERIFY_ARE_EQUAL(strcmp(ProcessorCacheInformation::TypeName(CacheInstruction), "Instruction"), 0);
        VERIFY_ARE_EQUAL(strcmp(ProcessorCacheInformation::TypeName(CacheData), "Data"), 0);
        VERIFY_ARE_EQUAL(strcmp(ProcessorCacheInformation::TypeName(CacheTrace), "Trace"), 0);
    }

    void ProcessorTopologyUnitTests::Test_CacheTypeAbbreviation()
    {
        VERIFY_ARE_EQUAL(strcmp(ProcessorCacheInformation::TypeAbbreviation(CacheUnified), ""), 0);
        VERIFY_ARE_EQUAL(strcmp(ProcessorCacheInformation::TypeAbbreviation(CacheInstruction), "i"), 0);
        VERIFY_ARE_EQUAL(strcmp(ProcessorCacheInformation::TypeAbbreviation(CacheData), "d"), 0);
        VERIFY_ARE_EQUAL(strcmp(ProcessorCacheInformation::TypeAbbreviation(CacheTrace), "t"), 0);
    }

    void ProcessorTopologyUnitTests::Test_GetText_SectionAll()
    {
        //
        // 2-core, 1-group system with L1d per core and shared L3.
        // Section::All should emit topology counts AND cache table.
        //

        ProcessorCacheInformation l1d0(1, 12, 64, 32 * 1024, CacheData);
        l1d0._processorMasks.emplace_back((WORD)0, (KAFFINITY)0x1);
        ProcessorCacheInformation l1d1(1, 12, 64, 32 * 1024, CacheData);
        l1d1._processorMasks.emplace_back((WORD)0, (KAFFINITY)0x2);
        ProcessorCacheInformation l3(3, 0xFF, 64, 8 * 1024 * 1024, CacheUnified);
        l3._processorMasks.emplace_back((WORD)0, (KAFFINITY)0x3);

        auto system = CreateTestSystemWithCaches({ l1d0, l1d1, l3 });
        system.processorTopology._ulProcessorCount = 2;
        system.processorTopology._vProcessorGroupInformation.clear();
        system.processorTopology._vProcessorGroupInformation.emplace_back((WORD)0, (BYTE)2, (BYTE)2, (KAFFINITY)0x3);
        system.processorTopology._vProcessorCoreInformation.clear();
        system.processorTopology._vProcessorCoreInformation.emplace_back((WORD)0, (KAFFINITY)0x1, (BYTE)0);
        system.processorTopology._vProcessorCoreInformation.emplace_back((WORD)0, (KAFFINITY)0x2, (BYTE)0);

        string sText = system.processorTopology.GetText(0, ProcessorTopology::Section::All);

        const char *pcszExpected =
            "cpu count:            2\n"
            "core count:           2\n"
            "group count:          1\n"
            "node count:           1\n"
            "socket count:         1\n"
            "heterogeneous cores:  n\n"
            "\n"
            "cache information:\n\n"
            "  Cache |   Size   | Line  | Assoc  | CPU\n"
            "  -------------------------------------------------------\n"
            "  L1d   |    32KiB |   64B | 12-way | 0-1 (per cpu)\n"
            "  L3    |     8MiB |   64B |   full | 0-1\n";

        VERIFY_MULTILINE_EQUAL(pcszExpected, sText);
    }

    void ProcessorTopologyUnitTests::Test_GetText_SectionTopology()
    {
        //
        // Same system as SectionAll but with Section::Topology.
        // Should emit topology counts but NO cache table.
        //

        ProcessorCacheInformation l3(3, 0xFF, 64, 8 * 1024 * 1024, CacheUnified);
        l3._processorMasks.emplace_back((WORD)0, (KAFFINITY)0x3);

        auto system = CreateTestSystemWithCaches({ l3 });
        system.processorTopology._ulProcessorCount = 2;
        system.processorTopology._vProcessorGroupInformation.clear();
        system.processorTopology._vProcessorGroupInformation.emplace_back((WORD)0, (BYTE)2, (BYTE)2, (KAFFINITY)0x3);
        system.processorTopology._vProcessorCoreInformation.clear();
        system.processorTopology._vProcessorCoreInformation.emplace_back((WORD)0, (KAFFINITY)0x1, (BYTE)0);
        system.processorTopology._vProcessorCoreInformation.emplace_back((WORD)0, (KAFFINITY)0x2, (BYTE)0);

        string sText = system.processorTopology.GetText(0, ProcessorTopology::Section::Topology);

        const char *pcszExpected =
            "cpu count:            2\n"
            "core count:           2\n"
            "group count:          1\n"
            "node count:           1\n"
            "socket count:         1\n"
            "heterogeneous cores:  n\n";

        VERIFY_MULTILINE_EQUAL(pcszExpected, sText);
    }

    void ProcessorTopologyUnitTests::Test_GetXml_SectionAll()
    {
        //
        // 2-core, 1-group system with one L3 cache.
        // Section::All should emit full <ProcessorTopology> including <Cache>.
        //

        ProcessorCacheInformation l3(3, 16, 64, 8 * 1024 * 1024, CacheUnified);
        l3._processorMasks.emplace_back((WORD)0, (KAFFINITY)0x3);

        auto system = CreateTestSystemWithCaches({ l3 });
        system.processorTopology._ulProcessorCount = 2;
        system.processorTopology._vProcessorGroupInformation.clear();
        system.processorTopology._vProcessorGroupInformation.emplace_back((WORD)0, (BYTE)2, (BYTE)2, (KAFFINITY)0x3);
        system.processorTopology._vProcessorCoreInformation.clear();
        system.processorTopology._vProcessorCoreInformation.emplace_back((WORD)0, (KAFFINITY)0x1, (BYTE)0);
        system.processorTopology._vProcessorCoreInformation.emplace_back((WORD)0, (KAFFINITY)0x2, (BYTE)0);

        string sXml = system.processorTopology.GetXml(0, ProcessorTopology::Section::All);

        const char *pcszExpected =
            "<ProcessorTopology Heterogeneous=\"false\">\n"
            "  <Group Group=\"0\" MaximumProcessors=\"2\" ActiveProcessors=\"2\" ActiveProcessorMask=\"0x3\"/>\n"
            "  <Node Node=\"0\">\n"
            "    <Group Group=\"0\" Mask=\"0x1\"/>\n"
            "  </Node>\n"
            "  <Socket Socket=\"0\">\n"
            "    <Group Group=\"0\" Mask=\"0x1\"/>\n"
            "  </Socket>\n"
            "  <Core Group=\"0\" Core=\"0\" Mask=\"0x1\" EfficiencyClass=\"0\"/>\n"
            "  <Core Group=\"0\" Core=\"0\" Mask=\"0x2\" EfficiencyClass=\"0\"/>\n"
            "  <Cache Level=\"3\" Associativity=\"16\" LineSize=\"64\" CacheSize=\"8388608\" Type=\"Unified\">\n"
            "    <Group Group=\"0\" Mask=\"0x3\"/>\n"
            "  </Cache>\n"
            "</ProcessorTopology>\n";

        VERIFY_MULTILINE_EQUAL(pcszExpected, sXml);
    }

    void ProcessorTopologyUnitTests::Test_GetXml_SectionTopology()
    {
        //
        // Same system as SectionAll but with Section::Topology.
        // Should emit <ProcessorTopology> but NO <Cache> elements.
        //

        ProcessorCacheInformation l3(3, 16, 64, 8 * 1024 * 1024, CacheUnified);
        l3._processorMasks.emplace_back((WORD)0, (KAFFINITY)0x3);

        auto system = CreateTestSystemWithCaches({ l3 });
        system.processorTopology._ulProcessorCount = 2;
        system.processorTopology._vProcessorGroupInformation.clear();
        system.processorTopology._vProcessorGroupInformation.emplace_back((WORD)0, (BYTE)2, (BYTE)2, (KAFFINITY)0x3);
        system.processorTopology._vProcessorCoreInformation.clear();
        system.processorTopology._vProcessorCoreInformation.emplace_back((WORD)0, (KAFFINITY)0x1, (BYTE)0);
        system.processorTopology._vProcessorCoreInformation.emplace_back((WORD)0, (KAFFINITY)0x2, (BYTE)0);

        string sXml = system.processorTopology.GetXml(0, ProcessorTopology::Section::Topology);

        const char *pcszExpected =
            "<ProcessorTopology Heterogeneous=\"false\">\n"
            "  <Group Group=\"0\" MaximumProcessors=\"2\" ActiveProcessors=\"2\" ActiveProcessorMask=\"0x3\"/>\n"
            "  <Node Node=\"0\">\n"
            "    <Group Group=\"0\" Mask=\"0x1\"/>\n"
            "  </Node>\n"
            "  <Socket Socket=\"0\">\n"
            "    <Group Group=\"0\" Mask=\"0x1\"/>\n"
            "  </Socket>\n"
            "  <Core Group=\"0\" Core=\"0\" Mask=\"0x1\" EfficiencyClass=\"0\"/>\n"
            "  <Core Group=\"0\" Core=\"0\" Mask=\"0x2\" EfficiencyClass=\"0\"/>\n"
            "</ProcessorTopology>\n";

        VERIFY_MULTILINE_EQUAL(pcszExpected, sXml);
    }

    void ProcessorTopologyUnitTests::Test_GetCacheText_BigSystem()
    {
        //
        // 2-group system, 3 cores/group, 2 CPUs/core (12 total).
        // L1d/L1i/L2 per core -> "(per core)"
        // L3 per group -> "(per group)"
        //

        vector<ProcessorCacheInformation> caches;
        KAFFINITY coreMasks[] = { 0x3, 0xC, 0x30 };

        // Per-core caches for both groups, interleaved as the system would return them
        for (WORD g = 0; g < 2; g++)
        {
            for (int c = 0; c < 3; c++)
            {
                ProcessorCacheInformation l1d(1, 8, 64, 32 * 1024, CacheData);
                l1d._processorMasks.emplace_back(g, coreMasks[c]);
                caches.push_back(l1d);

                ProcessorCacheInformation l1i(1, 8, 64, 32 * 1024, CacheInstruction);
                l1i._processorMasks.emplace_back(g, coreMasks[c]);
                caches.push_back(l1i);

                ProcessorCacheInformation l2(2, 16, 64, 1024 * 1024, CacheUnified);
                l2._processorMasks.emplace_back(g, coreMasks[c]);
                caches.push_back(l2);
            }

            ProcessorCacheInformation l3(3, 11, 64, 28 * 1024 * 1024, CacheUnified);
            l3._processorMasks.emplace_back(g, (KAFFINITY)0x3F);
            caches.push_back(l3);
        }

        auto system = CreateTestSystemWithCaches(caches);

        system.processorTopology._ulProcessorCount = 12;
        system.processorTopology._vProcessorGroupInformation.clear();
        system.processorTopology._vProcessorGroupInformation.emplace_back((WORD)0, (BYTE)6, (BYTE)6, (KAFFINITY)0x3F);
        system.processorTopology._vProcessorGroupInformation.emplace_back((WORD)1, (BYTE)6, (BYTE)6, (KAFFINITY)0x3F);

        system.processorTopology._vProcessorCoreInformation.clear();
        for (WORD g = 0; g < 2; g++)
        {
            for (int c = 0; c < 3; c++)
            {
                system.processorTopology._vProcessorCoreInformation.emplace_back(g, coreMasks[c], (BYTE)0);
            }
        }

        string sText = system.processorTopology.GetText(0, ProcessorTopology::Section::Cache);

        const char *pcszExpected =
            "\ncache information:\n\n"
            "  Cache |   Size   | Line  | Assoc  | Group/CPU\n"
            "  -----------------------------------------------------------\n"
            "  L1d   |    32KiB |   64B |  8-way | 0/0-5 1/0-5 (per core)\n"
            "  L1i   |    32KiB |   64B |  8-way | 0/0-5 1/0-5 (per core)\n"
            "  L2    |     1MiB |   64B | 16-way | 0/0-5 1/0-5 (per core)\n"
            "  L3    |    28MiB |   64B | 11-way | 0-1 (per group)\n";

        VERIFY_MULTILINE_EQUAL(pcszExpected, sText);
    }

    void ProcessorTopologyUnitTests::Test_GetCacheText_SmallHeteroSystem()
    {
        //
        // Single-group, 6 CPUs: 1 HT core (CPUs 0-1) + 4 single-cpu cores (CPUs 2-5).
        // L1d(48K)/L1i(32K)/L2(1.25M) per core 0-1 -> "(per core)" has no match (only one HT core)
        //   -> plain output since matchCount == 0
        // L1d(32K)/L1i(64K) per cpu 2-5 -> "(per cpu)"
        // L2(2M) covering 2-5 -> plain (doesn't match any core)
        // L3(12M) all -> plain
        //

        vector<ProcessorCacheInformation> caches;

        // HT core (CPUs 0-1)
        ProcessorCacheInformation l1d_ht(1, 12, 64, 48 * 1024, CacheData);
        l1d_ht._processorMasks.emplace_back((WORD)0, (KAFFINITY)0x3);
        caches.push_back(l1d_ht);

        ProcessorCacheInformation l1i_ht(1, 8, 64, 32 * 1024, CacheInstruction);
        l1i_ht._processorMasks.emplace_back((WORD)0, (KAFFINITY)0x3);
        caches.push_back(l1i_ht);

        ProcessorCacheInformation l2_ht(2, 10, 64, 1280 * 1024, CacheUnified);
        l2_ht._processorMasks.emplace_back((WORD)0, (KAFFINITY)0x3);
        caches.push_back(l2_ht);

        // L3 shared by all
        ProcessorCacheInformation l3(3, 12, 64, 12 * 1024 * 1024, CacheUnified);
        l3._processorMasks.emplace_back((WORD)0, (KAFFINITY)0x3F);
        caches.push_back(l3);

        // Single-cpu caches for CPUs 2-5
        for (int cpu = 2; cpu < 6; cpu++)
        {
            ProcessorCacheInformation l1d(1, 8, 64, 32 * 1024, CacheData);
            l1d._processorMasks.emplace_back((WORD)0, (KAFFINITY)1 << cpu);
            caches.push_back(l1d);

            ProcessorCacheInformation l1i(1, 8, 64, 64 * 1024, CacheInstruction);
            l1i._processorMasks.emplace_back((WORD)0, (KAFFINITY)1 << cpu);
            caches.push_back(l1i);
        }

        // L2 shared across CPUs 2-5 (not a core match)
        ProcessorCacheInformation l2_shared(2, 16, 64, 2 * 1024 * 1024, CacheUnified);
        l2_shared._processorMasks.emplace_back((WORD)0, (KAFFINITY)0x3C);
        caches.push_back(l2_shared);

        auto system = CreateTestSystemWithCaches(caches);

        system.processorTopology._ulProcessorCount = 6;
        system.processorTopology._vProcessorGroupInformation.clear();
        system.processorTopology._vProcessorGroupInformation.emplace_back((WORD)0, (BYTE)6, (BYTE)6, (KAFFINITY)0x3F);

        system.processorTopology._vProcessorCoreInformation.clear();
        system.processorTopology._vProcessorCoreInformation.emplace_back((WORD)0, (KAFFINITY)0x3, (BYTE)0);  // HT core
        system.processorTopology._vProcessorCoreInformation.emplace_back((WORD)0, (KAFFINITY)0x4, (BYTE)0);
        system.processorTopology._vProcessorCoreInformation.emplace_back((WORD)0, (KAFFINITY)0x8, (BYTE)0);
        system.processorTopology._vProcessorCoreInformation.emplace_back((WORD)0, (KAFFINITY)0x10, (BYTE)0);
        system.processorTopology._vProcessorCoreInformation.emplace_back((WORD)0, (KAFFINITY)0x20, (BYTE)0);

        string sText = system.processorTopology.GetText(0, ProcessorTopology::Section::Cache);

        const char *pcszExpected =
            "\ncache information:\n\n"
            "  Cache |   Size   | Line  | Assoc  | CPU\n"
            "  -------------------------------------------------------\n"
            "  L1d   |    48KiB |   64B | 12-way | 0-1\n"
            "  L1i   |    32KiB |   64B |  8-way | 0-1\n"
            "  L2    |  1.25MiB |   64B | 10-way | 0-1\n"
            "  L3    |    12MiB |   64B | 12-way | 0-5\n"
            "  L1d   |    32KiB |   64B |  8-way | 2-5 (per cpu)\n"
            "  L1i   |    64KiB |   64B |  8-way | 2-5 (per cpu)\n"
            "  L2    |     2MiB |   64B | 16-way | 2-5\n";

        VERIFY_MULTILINE_EQUAL(pcszExpected, sText);
    }

    void ProcessorTopologyUnitTests::Test_GetCacheText_NoCompaction()
    {
        //
        // Each cache has a unique geometry. No compaction possible.
        //

        ProcessorCacheInformation l1d(1, 8, 64, 32 * 1024, CacheData);
        l1d._processorMasks.emplace_back((WORD)0, (KAFFINITY)0x1);
        ProcessorCacheInformation l2(2, 16, 64, 256 * 1024, CacheUnified);
        l2._processorMasks.emplace_back((WORD)0, (KAFFINITY)0x1);
        ProcessorCacheInformation l3(3, 0xFF, 64, 8 * 1024 * 1024, CacheUnified);
        l3._processorMasks.emplace_back((WORD)0, (KAFFINITY)0x1);

        auto system = CreateTestSystemWithCaches({ l1d, l2, l3 });

        string sText = system.processorTopology.GetText(0, ProcessorTopology::Section::Cache);

        const char *pcszExpected =
            "\ncache information:\n\n"
            "  Cache |   Size   | Line  | Assoc  | CPU\n"
            "  -------------------------------------------------------\n"
            "  L1d   |    32KiB |   64B |  8-way | 0\n"
            "  L2    |   256KiB |   64B | 16-way | 0\n"
            "  L3    |     8MiB |   64B |   full | 0\n";

        VERIFY_MULTILINE_EQUAL(pcszExpected, sText);
    }

    void ProcessorTopologyUnitTests::Test_GetCacheText_MultiMaskPlain()
    {
        //
        // Cache with multiple (group, mask) pairs -> always plain case.
        //

        ProcessorCacheInformation l3(3, 16, 64, 16 * 1024 * 1024, CacheUnified);
        l3._processorMasks.emplace_back((WORD)0, (KAFFINITY)0xF);
        l3._processorMasks.emplace_back((WORD)1, (KAFFINITY)0xF);

        auto system = CreateTestSystemWithCaches({ l3 });

        system.processorTopology._ulProcessorCount = 8;
        system.processorTopology._vProcessorGroupInformation.clear();
        system.processorTopology._vProcessorGroupInformation.emplace_back((WORD)0, (BYTE)4, (BYTE)4, (KAFFINITY)0xF);
        system.processorTopology._vProcessorGroupInformation.emplace_back((WORD)1, (BYTE)4, (BYTE)4, (KAFFINITY)0xF);

        string sText = system.processorTopology.GetText(0, ProcessorTopology::Section::Cache);

        const char *pcszExpected =
            "\ncache information:\n\n"
            "  Cache |   Size   | Line  | Assoc  | Group/CPU\n"
            "  -----------------------------------------------------------\n"
            "  L3    |    16MiB |   64B | 16-way | 0/0-3 1/0-3\n";

        VERIFY_MULTILINE_EQUAL(pcszExpected, sText);
    }

    void ProcessorTopologyUnitTests::Test_GetCacheText_SingleGroupSkipsGroupCase()
    {
        //
        // Single-group system where a cache covers all CPUs (matches group mask).
        // Group case is skipped for single-group systems -> falls to Other (plain).
        //

        ProcessorCacheInformation l3(3, 12, 64, 8 * 1024 * 1024, CacheUnified);
        l3._processorMasks.emplace_back((WORD)0, (KAFFINITY)0xF);

        auto system = CreateTestSystemWithCaches({ l3 });

        system.processorTopology._ulProcessorCount = 4;
        system.processorTopology._vProcessorGroupInformation.clear();
        system.processorTopology._vProcessorGroupInformation.emplace_back((WORD)0, (BYTE)4, (BYTE)4, (KAFFINITY)0xF);
        system.processorTopology._vProcessorCoreInformation.clear();
        for (BYTE i = 0; i < 4; i++)
        {
            system.processorTopology._vProcessorCoreInformation.emplace_back((WORD)0, (KAFFINITY)1 << i, (BYTE)0);
        }

        string sText = system.processorTopology.GetText(0, ProcessorTopology::Section::Cache);

        const char *pcszExpected =
            "\ncache information:\n\n"
            "  Cache |   Size   | Line  | Assoc  | CPU\n"
            "  -------------------------------------------------------\n"
            "  L3    |     8MiB |   64B | 12-way | 0-3\n";

        VERIFY_MULTILINE_EQUAL(pcszExpected, sText);
    }

    void ProcessorTopologyUnitTests::Test_GroupMaskRanges()
    {
        // Single group, no prefix
        vector<pair<WORD, KAFFINITY>> single = { {(WORD)0, (KAFFINITY)0xF} };
        VERIFY_ARE_EQUAL(ProcessorTopology::GroupMaskRanges(single, false), string("0-3"));

        // Multi group, with prefix and space separator
        vector<pair<WORD, KAFFINITY>> multi = { {(WORD)0, (KAFFINITY)0x3F}, {(WORD)1, (KAFFINITY)0x3F} };
        VERIFY_ARE_EQUAL(ProcessorTopology::GroupMaskRanges(multi, true), string("0/0-5 1/0-5"));

        // Single group entry with sparse mask
        vector<pair<WORD, KAFFINITY>> sparse = { {(WORD)0, (KAFFINITY)0x5} };
        VERIFY_ARE_EQUAL(ProcessorTopology::GroupMaskRanges(sparse, false), string("0,2"));
        VERIFY_ARE_EQUAL(ProcessorTopology::GroupMaskRanges(sparse, true), string("0/0,2"));
    }

    void ProcessorTopologyUnitTests::Test_SameGeometry()
    {
        ProcessorCacheInformation a(1, 8, 64, 32 * 1024, CacheData);
        ProcessorCacheInformation b(1, 8, 64, 32 * 1024, CacheData);
        ProcessorCacheInformation c(2, 8, 64, 32 * 1024, CacheData);      // different level
        ProcessorCacheInformation d(1, 16, 64, 32 * 1024, CacheData);     // different assoc
        ProcessorCacheInformation e(1, 8, 128, 32 * 1024, CacheData);     // different line
        ProcessorCacheInformation f(1, 8, 64, 64 * 1024, CacheData);      // different size
        ProcessorCacheInformation g(1, 8, 64, 32 * 1024, CacheInstruction); // different type

        VERIFY_IS_TRUE(a.SameGeometry(b));
        VERIFY_IS_FALSE(a.SameGeometry(c));
        VERIFY_IS_FALSE(a.SameGeometry(d));
        VERIFY_IS_FALSE(a.SameGeometry(e));
        VERIFY_IS_FALSE(a.SameGeometry(f));
        VERIFY_IS_FALSE(a.SameGeometry(g));
    }

    //
    // UtilUnitTests
    //

    void UtilUnitTests::Test_GetSizeKMGT()
    {
        // Even KiB multiples
        VERIFY_ARE_EQUAL(Util::GetSizeKMGT(1024), string("1KiB"));
        VERIFY_ARE_EQUAL(Util::GetSizeKMGT(32 * 1024), string("32KiB"));
        VERIFY_ARE_EQUAL(Util::GetSizeKMGT(256 * 1024), string("256KiB"));

        // Even MiB multiples
        VERIFY_ARE_EQUAL(Util::GetSizeKMGT(1024 * 1024), string("1MiB"));
        VERIFY_ARE_EQUAL(Util::GetSizeKMGT(16 * 1024 * 1024), string("16MiB"));

        // Even GiB multiples
        VERIFY_ARE_EQUAL(Util::GetSizeKMGT((UINT64)1 * 1024 * 1024 * 1024), string("1GiB"));

        // Even TiB multiples
        VERIFY_ARE_EQUAL(Util::GetSizeKMGT((UINT64)1 * 1024 * 1024 * 1024 * 1024), string("1TiB"));

        // Even PiB multiples
        VERIFY_ARE_EQUAL(Util::GetSizeKMGT((UINT64)1024 * 1024 * 1024 * 1024 * 1024), string("1PiB"));
        VERIFY_ARE_EQUAL(Util::GetSizeKMGT((UINT64)4 * 1024 * 1024 * 1024 * 1024 * 1024), string("4PiB"));

        // Fractional TiB
        VERIFY_ARE_EQUAL(Util::GetSizeKMGT((UINT64)1024 * 1024 * 1024 * 1024 + (UINT64)512 * 1024 * 1024 * 1024), string("1.5TiB"));

        // Fractional PiB
        VERIFY_ARE_EQUAL(Util::GetSizeKMGT((UINT64)1024 * 1024 * 1024 * 1024 * 1024 + (UINT64)512 * 1024 * 1024 * 1024 * 1024), string("1.5PiB"));

        // Fractional sizes - trailing zeros stripped
        VERIFY_ARE_EQUAL(Util::GetSizeKMGT(1280), string("1.25KiB"));   // 1.25 - no trailing zero
        VERIFY_ARE_EQUAL(Util::GetSizeKMGT(1536), string("1.5KiB"));    // 1.50 -> 1.5
        VERIFY_ARE_EQUAL(Util::GetSizeKMGT(1792), string("1.75KiB"));   // 1.75 - no trailing zero
        VERIFY_ARE_EQUAL(Util::GetSizeKMGT(3 * 512), string("1.5KiB")); // 1536 again

        // Fractional MiB
        VERIFY_ARE_EQUAL(Util::GetSizeKMGT(1024 * 1024 + 512 * 1024), string("1.5MiB"));
        VERIFY_ARE_EQUAL(Util::GetSizeKMGT(2000 * 1024), string("1.95MiB"));

        // Sub-KiB uses B suffix
        VERIFY_ARE_EQUAL(Util::GetSizeKMGT(500), string("500B"));
        VERIFY_ARE_EQUAL(Util::GetSizeKMGT(64), string("64B"));
        VERIFY_ARE_EQUAL(Util::GetSizeKMGT(0), string("0B"));
    }

    //
    // UtilUnitTests - GetBufferAlignmentSize
    //

    void UtilUnitTests::Test_GetBufferAlignmentSize()
    {
        // SystemDefault always returns 0
        VERIFY_ARE_EQUAL((DWORD)0, Util::GetBufferAlignmentSize(BufferSeparation::SystemDefault, 4096, 64));
        VERIFY_ARE_EQUAL((DWORD)0, Util::GetBufferAlignmentSize(BufferSeparation::SystemDefault, 4096, 128));

        // PDECacheLine with 4K page, 64B cache line:
        //   ptesPerPage = 4096/8 = 512 (MMPTE is 8 bytes on all current platforms)
        //   ptePageCoverage = 512 * 4096 = 2MiB
        //   pdePerCacheLine = 64/8 = 8
        //   alignment = 8 * 2MiB = 16MiB
        VERIFY_ARE_EQUAL((DWORD)(16 * 1024 * 1024),
            Util::GetBufferAlignmentSize(BufferSeparation::PDECacheLine, 4096, 64));

        // PDECacheLine with 4K page, 128B cache line:
        //   pdePerCacheLine = 128/8 = 16
        //   alignment = 16 * 2MiB = 32MiB
        VERIFY_ARE_EQUAL((DWORD)(32 * 1024 * 1024),
            Util::GetBufferAlignmentSize(BufferSeparation::PDECacheLine, 4096, 128));

        // Cache line size 0 falls back to 64B default
        VERIFY_ARE_EQUAL((DWORD)(16 * 1024 * 1024),
            Util::GetBufferAlignmentSize(BufferSeparation::PDECacheLine, 4096, 0));

        // PDECacheLine with 8K page, 128B cache line:
        //   ptesPerPage = 8192/8 = 1024
        //   ptePageCoverage = 1024 * 8192 = 8MiB
        //   pdePerCacheLine = 128/8 = 16
        //   alignment = 16 * 8MiB = 128MiB
        VERIFY_ARE_EQUAL((DWORD)(128 * 1024 * 1024),
            Util::GetBufferAlignmentSize(BufferSeparation::PDECacheLine, 8192, 128));

        // SystemDefault with 8K page, 128B cache line still returns 0
        VERIFY_ARE_EQUAL((DWORD)0,
            Util::GetBufferAlignmentSize(BufferSeparation::SystemDefault, 8192, 128));
    }

    void UtilUnitTests::Test_MaskRanges()
    {
        VERIFY_ARE_EQUAL(Util::MaskRanges(0x0), string(""));
        VERIFY_ARE_EQUAL(Util::MaskRanges(0x1), string("0"));
        VERIFY_ARE_EQUAL(Util::MaskRanges(0x2), string("1"));
        VERIFY_ARE_EQUAL(Util::MaskRanges(0x3), string("0-1"));
        VERIFY_ARE_EQUAL(Util::MaskRanges(0x5), string("0,2"));
        VERIFY_ARE_EQUAL(Util::MaskRanges(0xF), string("0-3"));
        VERIFY_ARE_EQUAL(Util::MaskRanges(0xFF), string("0-7"));
        VERIFY_ARE_EQUAL(Util::MaskRanges(0x37), string("0-2,4-5"));
        VERIFY_ARE_EQUAL(Util::MaskRanges(0x8001), string("0,15"));
    }

    void UtilUnitTests::Test_IntRanges()
    {
        VERIFY_ARE_EQUAL(Util::IntRanges({}), string(""));
        VERIFY_ARE_EQUAL(Util::IntRanges({0}), string("0"));
        VERIFY_ARE_EQUAL(Util::IntRanges({0, 1}), string("0-1"));
        VERIFY_ARE_EQUAL(Util::IntRanges({0, 1, 2}), string("0-2"));
        VERIFY_ARE_EQUAL(Util::IntRanges({0, 2, 5}), string("0,2,5"));
        VERIFY_ARE_EQUAL(Util::IntRanges({0, 1, 3, 4}), string("0-1,3-4"));
    }

    //
    // UtilUnitTests - ShrinkContiguousWhitespace
    //

    void UtilUnitTests::Test_ShrinkContiguousWhitespace()
    {
        // empty string - no-op
        {
            string s = "";
            Util::ShrinkContiguousWhitespace(s);
            VERIFY_ARE_EQUAL(s, string(""));
        }

        // no whitespace - unchanged
        {
            string s = "hello";
            Util::ShrinkContiguousWhitespace(s);
            VERIFY_ARE_EQUAL(s, string("hello"));
        }

        // single character - unchanged
        {
            string s = "x";
            Util::ShrinkContiguousWhitespace(s);
            VERIFY_ARE_EQUAL(s, string("x"));
        }

        // leading whitespace only
        {
            string s = "   hello";
            Util::ShrinkContiguousWhitespace(s);
            VERIFY_ARE_EQUAL(s, string("hello"));
        }

        // trailing whitespace only
        {
            string s = "hello   ";
            Util::ShrinkContiguousWhitespace(s);
            VERIFY_ARE_EQUAL(s, string("hello"));
        }

        // mid-string whitespace only (single run)
        {
            string s = "hello   world";
            Util::ShrinkContiguousWhitespace(s);
            VERIFY_ARE_EQUAL(s, string("hello world"));
        }

        // leading + trailing
        {
            string s = "  hello  ";
            Util::ShrinkContiguousWhitespace(s);
            VERIFY_ARE_EQUAL(s, string("hello"));
        }

        // leading + mid-string
        {
            string s = "  hello   world";
            Util::ShrinkContiguousWhitespace(s);
            VERIFY_ARE_EQUAL(s, string("hello world"));
        }

        // trailing + mid-string
        {
            string s = "hello   world  ";
            Util::ShrinkContiguousWhitespace(s);
            VERIFY_ARE_EQUAL(s, string("hello world"));
        }

        // leading + mid-string + trailing
        {
            string s = "  hello   world  ";
            Util::ShrinkContiguousWhitespace(s);
            VERIFY_ARE_EQUAL(s, string("hello world"));
        }

        // tabs treated as whitespace
        {
            string s = "\thello\t\tworld\t";
            Util::ShrinkContiguousWhitespace(s);
            VERIFY_ARE_EQUAL(s, string("hello world"));
        }

        // mixed spaces and tabs
        {
            string s = " \t hello \t world \t ";
            Util::ShrinkContiguousWhitespace(s);
            VERIFY_ARE_EQUAL(s, string("hello world"));
        }

        // all whitespace collapses to empty
        {
            string s = "     ";
            Util::ShrinkContiguousWhitespace(s);
            VERIFY_ARE_EQUAL(s, string(""));
        }

        // single space collapses to empty
        {
            string s = " ";
            Util::ShrinkContiguousWhitespace(s);
            VERIFY_ARE_EQUAL(s, string(""));
        }

        // multiple words - only runs collapse, single spaces preserved
        {
            string s = "a b c";
            Util::ShrinkContiguousWhitespace(s);
            VERIFY_ARE_EQUAL(s, string("a b c"));
        }

        // realistic CPUID brand string with internal padding
        {
            string s = "  Intel(R) Core(TM) i9-10900 CPU @ 2.80GHz      ";
            Util::ShrinkContiguousWhitespace(s);
            VERIFY_ARE_EQUAL(s, string("Intel(R) Core(TM) i9-10900 CPU @ 2.80GHz"));
        }
    }

    //
    // DistributionUnitTests
    //

    void DistributionUnitTests::Test_SetPercent()
    {
        Distribution dist;

        // Set a percent distribution with IO% < 100 to verify tail element placement
        vector<DistributionRange> v;
        v.emplace_back(0, 10, make_pair((UINT64)0, (UINT64)10));
        v.emplace_back(10, 10, make_pair((UINT64)10, (UINT64)10));

        dist.Set(v, DistributionType::Percent);

        VERIFY_ARE_EQUAL(dist.GetType(), DistributionType::Percent);

        // Should have 3 entries: the 2 we set plus the tail element
        const auto& ranges = dist.GetRanges();
        VERIFY_ARE_EQUAL(ranges.size(), (size_t)3);

        // Tail element: IO 20-100 => Target 20-100
        VERIFY_ARE_EQUAL(ranges[2]._src, (UINT32)20);
        VERIFY_ARE_EQUAL(ranges[2]._span, (UINT32)80);
        VERIFY_ARE_EQUAL(ranges[2]._dst.first, (UINT64)20);
        VERIFY_ARE_EQUAL(ranges[2]._dst.second, (UINT64)80);
    }

    void DistributionUnitTests::Test_SetAbsolute()
    {
        Distribution dist;

        // Set an absolute distribution with IO% < 100
        // Tail element should have zero length (resolved at Finalize time)
        vector<DistributionRange> v;
        v.emplace_back(0, 10, make_pair((UINT64)0, (UINT64)(1024 * 1024)));
        v.emplace_back(10, 10, make_pair((UINT64)(1024 * 1024), (UINT64)(1024 * 1024)));

        dist.Set(v, DistributionType::Absolute);

        const auto& ranges = dist.GetRanges();
        VERIFY_ARE_EQUAL(ranges.size(), (size_t)3);

        // Tail element for absolute: zero length (to be resolved by Finalize)
        VERIFY_ARE_EQUAL(ranges[2]._src, (UINT32)20);
        VERIFY_ARE_EQUAL(ranges[2]._span, (UINT32)80);
        VERIFY_ARE_EQUAL(ranges[2]._dst.second, (UINT64)0);
    }

    void DistributionUnitTests::Test_SetPercentFullIO()
    {
        Distribution dist;

        // IO% == 100: should NOT add a tail element
        vector<DistributionRange> v;
        v.emplace_back(0, 50, make_pair((UINT64)0, (UINT64)50));
        v.emplace_back(50, 50, make_pair((UINT64)50, (UINT64)50));

        dist.Set(v, DistributionType::Percent);

        VERIFY_ARE_EQUAL(dist.GetRanges().size(), (size_t)2);
    }

    void DistributionUnitTests::Test_SetPercentTargetFullBeforeIO()
    {
        Distribution dist;

        // Target% reaches 100 but IO% < 100 (30% IO)
        // Tail should have zero target length (caught by validation)
        vector<DistributionRange> v;
        v.emplace_back(0, 10, make_pair((UINT64)0, (UINT64)50));
        v.emplace_back(10, 20, make_pair((UINT64)50, (UINT64)50));

        dist.Set(v, DistributionType::Percent);

        const auto& r = dist.GetRanges();
        VERIFY_ARE_EQUAL(r.size(), (size_t)3);

        // First two unchanged
        VERIFY_ARE_EQUAL(r[0]._src, (UINT32)0);
        VERIFY_ARE_EQUAL(r[0]._span, (UINT32)10);
        VERIFY_ARE_EQUAL(r[0]._dst.first, (UINT64)0);
        VERIFY_ARE_EQUAL(r[0]._dst.second, (UINT64)50);

        VERIFY_ARE_EQUAL(r[1]._src, (UINT32)10);
        VERIFY_ARE_EQUAL(r[1]._span, (UINT32)20);
        VERIFY_ARE_EQUAL(r[1]._dst.first, (UINT64)50);
        VERIFY_ARE_EQUAL(r[1]._dst.second, (UINT64)50);

        // Tail: IO 30-100, target at 100 with zero length
        VERIFY_ARE_EQUAL(r[2]._src, (UINT32)30);
        VERIFY_ARE_EQUAL(r[2]._span, (UINT32)70);
        VERIFY_ARE_EQUAL(r[2]._dst.first, (UINT64)100);
        VERIFY_ARE_EQUAL(r[2]._dst.second, (UINT64)0);
    }

    void DistributionUnitTests::Test_ValidatePercentValid()
    {
        Distribution dist;

        vector<DistributionRange> v;
        v.emplace_back(0, 10, make_pair((UINT64)0, (UINT64)10));
        v.emplace_back(10, 10, make_pair((UINT64)10, (UINT64)10));
        dist.Set(v, DistributionType::Percent);

        VERIFY_IS_TRUE(dist.Validate(4096));
    }

    void DistributionUnitTests::Test_ValidateAbsoluteValid()
    {
        Distribution dist;

        vector<DistributionRange> v;
        v.emplace_back(0, 50, make_pair((UINT64)0, (UINT64)(1024 * 1024)));
        v.emplace_back(50, 50, make_pair((UINT64)(1024 * 1024), (UINT64)0));
        dist.Set(v, DistributionType::Absolute);

        VERIFY_IS_TRUE(dist.Validate(4096));
    }

    void DistributionUnitTests::Test_ValidatePercentIOOverflow()
    {
        Distribution dist;

        // IO% > 100 is invalid
        vector<DistributionRange> v;
        v.emplace_back(0, 60, make_pair((UINT64)0, (UINT64)50));
        v.emplace_back(60, 60, make_pair((UINT64)50, (UINT64)50));
        dist.Set(v, DistributionType::Percent);

        VERIFY_IS_FALSE(dist.Validate(4096));
    }

    void DistributionUnitTests::Test_ValidatePercentTargetOverflow()
    {
        Distribution dist;

        // Target% > 100 is invalid
        vector<DistributionRange> v;
        v.emplace_back(0, 10, make_pair((UINT64)0, (UINT64)60));
        v.emplace_back(10, 10, make_pair((UINT64)60, (UINT64)60));
        dist.Set(v, DistributionType::Percent);

        VERIFY_IS_FALSE(dist.Validate(4096));
    }

    void DistributionUnitTests::Test_ValidateAbsoluteRangeTooSmall()
    {
        Distribution dist;

        // Absolute range smaller than block size is invalid
        vector<DistributionRange> v;
        v.emplace_back(0, 50, make_pair((UINT64)0, (UINT64)2048));  // 2K < 4K block
        v.emplace_back(50, 50, make_pair((UINT64)2048, (UINT64)0));
        dist.Set(v, DistributionType::Absolute);

        VERIFY_IS_FALSE(dist.Validate(4096));
    }

    void DistributionUnitTests::Test_ValidatePercentTargetCoveredBeforeIO()
    {
        Distribution dist;

        // Target% reaches 100 but IO% is only 30 (10+20).
        // This should fail: "the target is covered with 70% IO left to distribute"
        vector<DistributionRange> v;
        v.emplace_back(0, 10, make_pair((UINT64)0, (UINT64)50));
        v.emplace_back(10, 20, make_pair((UINT64)50, (UINT64)50));
        dist.Set(v, DistributionType::Percent);

        VERIFY_IS_FALSE(dist.Validate(4096));
    }

    void DistributionUnitTests::Test_FinalizePercent()
    {
        Distribution dist;

        // -rdpct10/10:10/10:0/10 + tail
        // 100KB target, 4KB block/alignment
        vector<DistributionRange> v;
        v.emplace_back(0, 10, make_pair((UINT64)0, (UINT64)10));
        v.emplace_back(10, 10, make_pair((UINT64)10, (UINT64)10));
        v.emplace_back(20, 0, make_pair((UINT64)20, (UINT64)10));   // zero IO% hole
        v.emplace_back(20, 80, make_pair((UINT64)30, (UINT64)70));
        dist.Set(v, DistributionType::Percent);

        // relTargetSizeAligned = 100KB, relTargetSize = 100KB
        dist.Finalize(100 * 1024, 100 * 1024, 4 * 1024, 4 * 1024);

        const auto& r = dist.GetRanges();
        VERIFY_ARE_EQUAL(dist.GetIOSpan(), (UINT32)100);
        VERIFY_ARE_EQUAL(r.size(), (size_t)3);

        // Range 0: 10% IO, [0, 8KB)
        VERIFY_ARE_EQUAL(r[0]._span, (UINT32)10);
        VERIFY_ARE_EQUAL(r[0]._dst.first, (UINT64)0);
        VERIFY_ARE_EQUAL(r[0]._dst.second, (UINT64)(8 * 1024));

        // Range 1: 10% IO, [8KB, 20KB) - note 12KB, not 8KB due to hole absorption
        VERIFY_ARE_EQUAL(r[1]._span, (UINT32)10);
        VERIFY_ARE_EQUAL(r[1]._dst.first, (UINT64)(8 * 1024));
        VERIFY_ARE_EQUAL(r[1]._dst.second, (UINT64)(12 * 1024));

        // Range 2: 80% IO, [28KB, 100KB) - hole removed
        VERIFY_ARE_EQUAL(r[2]._span, (UINT32)80);
        VERIFY_ARE_EQUAL(r[2]._dst.first, (UINT64)(28 * 1024));
        VERIFY_ARE_EQUAL(r[2]._dst.second, (UINT64)(72 * 1024));
    }

    void DistributionUnitTests::Test_FinalizePercentDegenerate()
    {
        Distribution dist;

        // -rdpct10/1:10/1 with 100KB target, 4KB alignment
        // Both ranges are degenerate (1% of 96KB < 4KB alignment)
        vector<DistributionRange> v;
        v.emplace_back(0, 10, make_pair((UINT64)0, (UINT64)1));
        v.emplace_back(10, 10, make_pair((UINT64)1, (UINT64)1));
        dist.Set(v, DistributionType::Percent);

        dist.Finalize(100 * 1024, 100 * 1024, 4 * 1024, 4 * 1024);

        const auto& r = dist.GetRanges();
        VERIFY_ARE_EQUAL(dist.GetIOSpan(), (UINT32)100);

        // Both degenerate ranges combine with the tail; we should get 2 ranges
        VERIFY_ARE_EQUAL(r.size(), (size_t)2);

        // First range is the combined degenerate at offset 0
        VERIFY_ARE_EQUAL(r[0]._dst.first, (UINT64)0);

        // Second range covers the remainder
        VERIFY_ARE_EQUAL(r[1]._dst.first + r[1]._dst.second, (UINT64)(100 * 1024));
    }

    void DistributionUnitTests::Test_FinalizeAbsolute()
    {
        Distribution dist;

        // -rdabs10/1G:10/1G + tail, target is 10GB
        vector<DistributionRange> v;
        v.emplace_back(0, 10, make_pair((UINT64)0, (UINT64)(1024ull * 1024 * 1024)));
        v.emplace_back(10, 10, make_pair((UINT64)(1024ull * 1024 * 1024), (UINT64)(1024ull * 1024 * 1024)));
        dist.Set(v, DistributionType::Absolute);

        UINT64 targetSize = 10ull * 1024 * 1024 * 1024;
        dist.Finalize(targetSize, targetSize, 4 * 1024, 4 * 1024);

        const auto& r = dist.GetRanges();
        VERIFY_ARE_EQUAL(dist.GetIOSpan(), (UINT32)100);
        VERIFY_ARE_EQUAL(r.size(), (size_t)3);

        // First two ranges preserved as-is
        VERIFY_ARE_EQUAL(r[0]._span, (UINT32)10);
        VERIFY_ARE_EQUAL(r[0]._dst.second, (UINT64)(1024ull * 1024 * 1024));
        VERIFY_ARE_EQUAL(r[1]._span, (UINT32)10);
        VERIFY_ARE_EQUAL(r[1]._dst.second, (UINT64)(1024ull * 1024 * 1024));

        // Tail expanded to rest of target
        VERIFY_ARE_EQUAL(r[2]._span, (UINT32)80);
        VERIFY_ARE_EQUAL(r[2]._dst.first, (UINT64)(2ull * 1024 * 1024 * 1024));
    }

    void DistributionUnitTests::Test_FinalizeAbsoluteTrimmed()
    {
        Distribution dist;

        // -rdabs10/1G:10/1G + tail, but target is only 1.5GB
        // The tail (at 2GB) is beyond the target, so it's discarded
        // IO% trims to 20 (only first two ranges are usable)
        vector<DistributionRange> v;
        v.emplace_back(0, 10, make_pair((UINT64)0, (UINT64)(1024ull * 1024 * 1024)));
        v.emplace_back(10, 10, make_pair((UINT64)(1024ull * 1024 * 1024), (UINT64)(1024ull * 1024 * 1024)));
        dist.Set(v, DistributionType::Absolute);

        UINT64 targetSize = (UINT64)(1.5 * 1024 * 1024 * 1024);
        dist.Finalize(targetSize, targetSize, 4 * 1024, 4 * 1024);

        const auto& r = dist.GetRanges();

        // IO% trimmed: only 10+10 = 20 of IO is usable
        VERIFY_ARE_EQUAL(dist.GetIOSpan(), (UINT32)20);
        VERIFY_ARE_EQUAL(r.size(), (size_t)2);

        // First range preserved
        VERIFY_ARE_EQUAL(r[0]._span, (UINT32)10);
        VERIFY_ARE_EQUAL(r[0]._dst.second, (UINT64)(1024ull * 1024 * 1024));

        // Second range trimmed to remaining target
        VERIFY_ARE_EQUAL(r[1]._span, (UINT32)10);
        VERIFY_ARE_EQUAL(r[1]._dst.first, (UINT64)(1024ull * 1024 * 1024));
    }

    void DistributionUnitTests::Test_FinalizeTypeSetsAbsolute()
    {
        Distribution dist;

        vector<DistributionRange> v;
        v.emplace_back(0, 50, make_pair((UINT64)0, (UINT64)50));
        v.emplace_back(50, 50, make_pair((UINT64)50, (UINT64)50));
        dist.Set(v, DistributionType::Percent);

        VERIFY_ARE_EQUAL(dist.GetType(), DistributionType::Percent);

        dist.Finalize(100 * 1024, 100 * 1024, 4 * 1024, 4 * 1024);

        // After finalization, type is always Absolute
        VERIFY_ARE_EQUAL(dist.GetType(), DistributionType::Absolute);
    }

    void DistributionUnitTests::Test_GetTextPercent()
    {
        Distribution dist;

        vector<DistributionRange> v;
        v.emplace_back(0, 10, make_pair((UINT64)0, (UINT64)10));
        v.emplace_back(10, 10, make_pair((UINT64)10, (UINT64)10));
        v.emplace_back(20, 0, make_pair((UINT64)20, (UINT64)10));
        v.emplace_back(20, 80, make_pair((UINT64)30, (UINT64)70));
        dist.Set(v, DistributionType::Percent);

        string sText = dist.GetText(6);

        string pcszExpected =
            "          10% of IO => [ 0% -  10%) of target\n"
            "          10% of IO => [10% -  20%) of target\n"
            "           0% of IO => [20% -  30%) of target\n"
            "          80% of IO => [30% - 100%) of target\n";

        VERIFY_MULTILINE_EQUAL(pcszExpected, sText);
    }

    void DistributionUnitTests::Test_GetTextAbsolute()
    {
        Distribution dist;

        vector<DistributionRange> v;
        v.emplace_back(0, 10, make_pair((UINT64)0, (UINT64)(1024 * 1024 * 1024)));
        v.emplace_back(10, 10, make_pair((UINT64)(1024 * 1024 * 1024), (UINT64)(1024 * 1024 * 1024)));
        v.emplace_back(20, 0, make_pair((UINT64)(2ull * 1024 * 1024 * 1024), (UINT64)(100ull * 1024 * 1024 * 1024)));
        v.emplace_back(20, 80, make_pair((UINT64)(102ull * 1024 * 1024 * 1024), (UINT64)0));
        dist.Set(v, DistributionType::Absolute);

        string sText = dist.GetText(6);

        string pcszExpected =
            "          10% of IO => [     0    -      1GiB)\n"
            "          10% of IO => [     1GiB -      2GiB)\n"
            "           0% of IO => [     2GiB -    102GiB)\n"
            "          80% of IO => [   102GiB -       end)\n";

        VERIFY_MULTILINE_EQUAL(pcszExpected, sText);
    }

    void DistributionUnitTests::Test_GetXml()
    {
        Distribution dist;

        vector<DistributionRange> v;
        v.emplace_back(0, 10, make_pair((UINT64)0, (UINT64)10));
        v.emplace_back(10, 10, make_pair((UINT64)10, (UINT64)10));
        dist.Set(v, DistributionType::Percent);

        string sXml = dist.GetXml(0);

        string pcszExpected =
            "<Distribution>\n"
            "  <Percent>\n"
            "    <Range IO=\"10\">10</Range>\n"
            "    <Range IO=\"10\">10</Range>\n"
            "    <Range IO=\"80\">80</Range>\n"
            "  </Percent>\n"
            "</Distribution>\n";

        VERIFY_MULTILINE_EQUAL(pcszExpected, sXml);
    }

    void DistributionUnitTests::Test_GetXmlWithHoles()
    {
        Distribution dist;

        // Simulate a finalized distribution with a gap between ranges
        // Range 0: [0, 4KB), Range 1: [8KB, 16KB) - hole at [4KB, 8KB)
        vector<DistributionRange> v;
        v.emplace_back(0, 50, make_pair((UINT64)0, (UINT64)4096));
        v.emplace_back(50, 50, make_pair((UINT64)8192, (UINT64)8192));
        dist.Set(v, DistributionType::Absolute);

        string sXml = dist.GetXml(0, true);

        string pcszExpected =
            "<Distribution>\n"
            "  <Absolute>\n"
            "    <Range IO=\"50\">4096</Range>\n"
            "    <Range IO=\"0\">4096</Range>\n"
            "    <Range IO=\"50\">8192</Range>\n"
            "  </Absolute>\n"
            "</Distribution>\n";

        VERIFY_MULTILINE_EQUAL(pcszExpected, sXml);
    }

    void DistributionUnitTests::Test_EmptyDistribution()
    {
        Distribution dist;

        // Default constructed distribution
        VERIFY_IS_TRUE(dist.IsEmpty());
        VERIFY_IS_FALSE(dist.HasRanges());
        VERIFY_ARE_EQUAL(dist.GetType(), DistributionType::None);
        VERIFY_ARE_EQUAL(dist.GetIOSpan(), (UINT32)100);

        // GetText and GetXml return empty for empty distribution
        VERIFY_ARE_EQUAL(dist.GetText(0), string());
        VERIFY_ARE_EQUAL(dist.GetXml(0), string());
    }

    void TextDiffUnitTests::Test_IdenticalStrings()
    {
        auto result = TextDiff::FindFirstDifference("line1\nline2\nline3", "line1\nline2\nline3");
        VERIFY_IS_TRUE(result.equal);
        VERIFY_ARE_EQUAL((size_t)0, result.lineNumber);
    }

    void TextDiffUnitTests::Test_IdenticalStringsWithTrailingNewlines()
    {
        auto result = TextDiff::FindFirstDifference("line1\nline2\n", "line1\nline2\n");
        VERIFY_IS_TRUE(result.equal);
        VERIFY_ARE_EQUAL((size_t)0, result.lineNumber);
    }

    void TextDiffUnitTests::Test_EmptyStrings()
    {
        auto result = TextDiff::FindFirstDifference("", "");
        VERIFY_IS_TRUE(result.equal);
    }

    void TextDiffUnitTests::Test_DifferentFirstLine()
    {
        auto result = TextDiff::FindFirstDifference("expected\nline2", "actual\nline2");
        VERIFY_IS_TRUE(!result.equal);
        VERIFY_ARE_EQUAL((size_t)1, result.lineNumber);
        VERIFY_ARE_EQUAL(string("expected"), result.expectedLine);
        VERIFY_ARE_EQUAL(string("actual"), result.actualLine);
    }

    void TextDiffUnitTests::Test_DifferentMiddleLine()
    {
        auto result = TextDiff::FindFirstDifference("line1\nexpected\nline3", "line1\nactual\nline3");
        VERIFY_IS_TRUE(!result.equal);
        VERIFY_ARE_EQUAL((size_t)2, result.lineNumber);
        VERIFY_ARE_EQUAL(string("expected"), result.expectedLine);
        VERIFY_ARE_EQUAL(string("actual"), result.actualLine);
    }

    void TextDiffUnitTests::Test_ExpectedLonger()
    {
        auto result = TextDiff::FindFirstDifference("line1\nline2\nline3", "line1\nline2");
        VERIFY_IS_TRUE(!result.equal);
        VERIFY_ARE_EQUAL((size_t)3, result.lineNumber);
        VERIFY_ARE_EQUAL(string("line3"), result.expectedLine);
        VERIFY_ARE_EQUAL(string("<end of text>"), result.actualLine);
    }

    void TextDiffUnitTests::Test_ActualLonger()
    {
        auto result = TextDiff::FindFirstDifference("line1\nline2", "line1\nline2\nline3");
        VERIFY_IS_TRUE(!result.equal);
        VERIFY_ARE_EQUAL((size_t)3, result.lineNumber);
        VERIFY_ARE_EQUAL(string("<end of text>"), result.expectedLine);
        VERIFY_ARE_EQUAL(string("line3"), result.actualLine);
    }

    void TextDiffUnitTests::Test_SingleLineDifference()
    {
        auto result = TextDiff::FindFirstDifference("hello", "world");
        VERIFY_IS_TRUE(!result.equal);
        VERIFY_ARE_EQUAL((size_t)1, result.lineNumber);
        VERIFY_ARE_EQUAL(string("hello"), result.expectedLine);
        VERIFY_ARE_EQUAL(string("world"), result.actualLine);
    }

    void TextDiffUnitTests::Test_TrailingNewline()
    {
        auto result = TextDiff::FindFirstDifference("line1\nline2", "line1\nline2\n");
        VERIFY_IS_TRUE(!result.equal);
    }

    void TextDiffUnitTests::Test_CaseSensitive()
    {
        auto result = TextDiff::FindFirstDifference("Hello\nWorld", "hello\nworld");
        VERIFY_IS_TRUE(!result.equal);
        VERIFY_ARE_EQUAL((size_t)1, result.lineNumber);
        VERIFY_ARE_EQUAL(string("Hello"), result.expectedLine);
        VERIFY_ARE_EQUAL(string("hello"), result.actualLine);
    }

    void TextDiffUnitTests::Test_VerifyMultilineEqualCharPointers()
    {
        // Verify that VERIFY_MULTILINE_EQUAL compares string content, not
        // pointer addresses, when both arguments are const char*.
        // Construct two distinct buffers with identical content to guarantee
        // different pointer addresses.

        const char* pcszA = "line1\nline2\nline3";
        char buf[32];
        strcpy_s(buf, pcszA);

        // buf and pcszA have the same content but different addresses
        VERIFY_ARE_NOT_EQUAL((const void*)pcszA, (const void*)buf);
        VERIFY_MULTILINE_EQUAL(pcszA, buf);
    }
}

