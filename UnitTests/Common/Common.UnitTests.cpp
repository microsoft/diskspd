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
#include <stdlib.h>

using namespace WEX::TestExecution;
using namespace WEX::Logging;

namespace UnitTests
{
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
        Histogram<int> h;
        h.Add(1);
        h.Add(2);
        VERIFY_ARE_EQUAL(h.GetMin(), 1);
        VERIFY_ARE_EQUAL(h.GetMax(), 2);
    }

    void HistogramUnitTests::Test_GetPercentile()
    {
        Histogram<int> h;
        h.Add(1);
        h.Add(2);
        h.Add(3);
        VERIFY_ARE_EQUAL(h.GetPercentile(0.5), 2);
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
        string sXml = profile.GetXml();
        //printf("'%s'\n", sXml.c_str());
        VERIFY_IS_TRUE(sXml == "<Profile>\n"
                               "<Progress>0</Progress>\n"
                               "<ResultFormat>text</ResultFormat>\n"
                               "<Verbose>false</Verbose>\n"
                               "<TimeSpans>\n"
                               "</TimeSpans>\n"
                               "</Profile>\n");
    }

    void ProfileUnitTests::Test_GetXmlPrecreateFilesUseMaxSize()
    {
        Profile profile;
        profile.SetPrecreateFiles(PrecreateFiles::UseMaxSize);
        string sXml = profile.GetXml();
        //printf("'%s'\n", sXml.c_str());
        VERIFY_IS_TRUE(sXml == "<Profile>\n"
                               "<Progress>0</Progress>\n"
                               "<ResultFormat>text</ResultFormat>\n"
                               "<Verbose>false</Verbose>\n"
                               "<PrecreateFiles>UseMaxSize</PrecreateFiles>\n"
                               "<TimeSpans>\n"
                               "</TimeSpans>\n"
                               "</Profile>\n");
    }

    void ProfileUnitTests::Test_GetXmlPrecreateFilesOnlyFilesWithConstantSizes()
    {
        Profile profile;
        profile.SetPrecreateFiles(PrecreateFiles::OnlyFilesWithConstantSizes);
        string sXml = profile.GetXml();
        //printf("'%s'\n", sXml.c_str());
        VERIFY_IS_TRUE(sXml == "<Profile>\n"
                               "<Progress>0</Progress>\n"
                               "<ResultFormat>text</ResultFormat>\n"
                               "<Verbose>false</Verbose>\n"
                               "<PrecreateFiles>CreateOnlyFilesWithConstantSizes</PrecreateFiles>\n"
                               "<TimeSpans>\n"
                               "</TimeSpans>\n"
                               "</Profile>\n");
    }

    void ProfileUnitTests::Test_GetXmlPrecreateFilesOnlyFilesWithConstantOrZeroSizes()
    {
        Profile profile;
        profile.SetPrecreateFiles(PrecreateFiles::OnlyFilesWithConstantOrZeroSizes);
        string sXml = profile.GetXml();
        //printf("'%s'\n", sXml.c_str());
        VERIFY_IS_TRUE(sXml == "<Profile>\n"
                               "<Progress>0</Progress>\n"
                               "<ResultFormat>text</ResultFormat>\n"
                               "<Verbose>false</Verbose>\n"
                               "<PrecreateFiles>CreateOnlyFilesWithConstantOrZeroSizes</PrecreateFiles>\n"
                               "<TimeSpans>\n"
                               "</TimeSpans>\n"
                               "</Profile>\n");
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
        system.processorTopology._vProcessorGroupInformation.emplace_back((BYTE)2, (BYTE)2, (WORD)0, (KAFFINITY)0x3);
        system.processorTopology._vProcessorGroupInformation.emplace_back((BYTE)2, (BYTE)2, (WORD)1, (KAFFINITY)0x3);

        TimeSpan timeSpan;
        Profile profile;

        // assign to each proc
        profile.ClearTimeSpans();
        timeSpan.ClearAffinityAssignment();
        timeSpan.AddAffinityAssignment(0, 0);
        timeSpan.AddAffinityAssignment(0, 1);
        timeSpan.AddAffinityAssignment(1, 0);
        timeSpan.AddAffinityAssignment(1, 1);
        profile.AddTimeSpan(timeSpan);
        VERIFY_IS_TRUE(profile.Validate(true, &system));

        // shrink active mask
        system.processorTopology._vProcessorGroupInformation.clear();
        system.processorTopology._vProcessorGroupInformation.emplace_back((BYTE)2, (BYTE)2, (WORD)0, (KAFFINITY)0x1);
        system.processorTopology._vProcessorGroupInformation.emplace_back((BYTE)2, (BYTE)2, (WORD)1, (KAFFINITY)0x1);

        // fail assignment to inactive procs
        VERIFY_IS_FALSE(profile.Validate(true, &system));

        // shrink procs, still fail
        system.processorTopology._vProcessorGroupInformation.clear();
        system.processorTopology._vProcessorGroupInformation.emplace_back((BYTE)1, (BYTE)1, (WORD)0, (KAFFINITY)0x1);
        system.processorTopology._vProcessorGroupInformation.emplace_back((BYTE)1, (BYTE)1, (WORD)1, (KAFFINITY)0x1);

        // now fail
        VERIFY_IS_FALSE(profile.Validate(true, &system));

        // assign to low procs, and succeed
        profile.ClearTimeSpans();
        timeSpan.ClearAffinityAssignment();
        timeSpan.AddAffinityAssignment(0, 0);
        timeSpan.AddAffinityAssignment(1, 0);
        profile.AddTimeSpan(timeSpan);
        VERIFY_IS_TRUE(profile.Validate(true, &system));

        // shrink groups
        system.processorTopology._vProcessorGroupInformation.clear();
        system.processorTopology._vProcessorGroupInformation.emplace_back((BYTE)1, (BYTE)1, (WORD)0, (KAFFINITY)0x1);

        // now fail
        VERIFY_IS_FALSE(profile.Validate(true, &system));

        // assign to low proc, and succeed
        profile.ClearTimeSpans();
        timeSpan.ClearAffinityAssignment();
        timeSpan.AddAffinityAssignment(0, 0);
        profile.AddTimeSpan(timeSpan);
        VERIFY_IS_TRUE(profile.Validate(true, &system));

        // assign to invalid group
        profile.ClearTimeSpans();
        timeSpan.ClearAffinityAssignment();
        timeSpan.AddAffinityAssignment(1, 0);
        profile.AddTimeSpan(timeSpan);
        VERIFY_IS_FALSE(profile.Validate(true, &system));
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
        string sXml = target.GetXml();
        VERIFY_IS_TRUE(sXml == "<Target>\n"
                               "<Path></Path>\n"
                               "<BlockSize>65536</BlockSize>\n"
                               "<BaseFileOffset>0</BaseFileOffset>\n"
                               "<SequentialScan>false</SequentialScan>\n"
                               "<RandomAccess>false</RandomAccess>\n"
                               "<TemporaryFile>false</TemporaryFile>\n"
                               "<UseLargePages>false</UseLargePages>\n"
                               "<WriteBufferContent>\n"
                               "<Pattern>sequential</Pattern>\n"
                               "</WriteBufferContent>\n"
                               "<ParallelAsyncIO>false</ParallelAsyncIO>\n"
                               "<StrideSize>65536</StrideSize>\n"
                               "<InterlockedSequential>false</InterlockedSequential>\n"
                               "<ThreadStride>0</ThreadStride>\n"
                               "<MaxFileSize>0</MaxFileSize>\n"
                               "<RequestCount>2</RequestCount>\n"
                               "<WriteRatio>0</WriteRatio>\n"
                               "<Throughput>0</Throughput>\n"
                               "<ThreadsPerFile>1</ThreadsPerFile>\n"
                               "<IOPriority>3</IOPriority>\n"
                               "<Weight>1</Weight>\n"
                               "</Target>\n");
    }

    void TargetUnitTests::Test_TargetGetXmlWriteBufferContentZero()
    {
        Target target;
        target.SetZeroWriteBuffers(true);
        string sXml = target.GetXml();
        VERIFY_IS_TRUE(sXml == "<Target>\n"
                               "<Path></Path>\n"
                               "<BlockSize>65536</BlockSize>\n"
                               "<BaseFileOffset>0</BaseFileOffset>\n"
                               "<SequentialScan>false</SequentialScan>\n"
                               "<RandomAccess>false</RandomAccess>\n"
                               "<TemporaryFile>false</TemporaryFile>\n"
                               "<UseLargePages>false</UseLargePages>\n"
                               "<WriteBufferContent>\n"
                               "<Pattern>zero</Pattern>\n"
                               "</WriteBufferContent>\n"
                               "<ParallelAsyncIO>false</ParallelAsyncIO>\n"
                               "<StrideSize>65536</StrideSize>\n"
                               "<InterlockedSequential>false</InterlockedSequential>\n"
                               "<ThreadStride>0</ThreadStride>\n"
                               "<MaxFileSize>0</MaxFileSize>\n"
                               "<RequestCount>2</RequestCount>\n"
                               "<WriteRatio>0</WriteRatio>\n"
                               "<Throughput>0</Throughput>\n"
                               "<ThreadsPerFile>1</ThreadsPerFile>\n"
                               "<IOPriority>3</IOPriority>\n"
                               "<Weight>1</Weight>\n"
                               "</Target>\n");
    }

    void TargetUnitTests::Test_TargetGetXmlWriteBufferContentRandomNoFilePath()
    {
        Target target;
        target.SetRandomDataWriteBufferSize(224433);
        string sXml = target.GetXml();
        VERIFY_IS_TRUE(sXml == "<Target>\n"
                               "<Path></Path>\n"
                               "<BlockSize>65536</BlockSize>\n"
                               "<BaseFileOffset>0</BaseFileOffset>\n"
                               "<SequentialScan>false</SequentialScan>\n"
                               "<RandomAccess>false</RandomAccess>\n"
                               "<TemporaryFile>false</TemporaryFile>\n"
                               "<UseLargePages>false</UseLargePages>\n"
                               "<WriteBufferContent>\n"
                               "<Pattern>random</Pattern>\n"
                               "<RandomDataSource>\n"
                               "<SizeInBytes>224433</SizeInBytes>\n"
                               "</RandomDataSource>\n"
                               "</WriteBufferContent>\n"
                               "<ParallelAsyncIO>false</ParallelAsyncIO>\n"
                               "<StrideSize>65536</StrideSize>\n"
                               "<InterlockedSequential>false</InterlockedSequential>\n"
                               "<ThreadStride>0</ThreadStride>\n"
                               "<MaxFileSize>0</MaxFileSize>\n"
                               "<RequestCount>2</RequestCount>\n"
                               "<WriteRatio>0</WriteRatio>\n"
                               "<Throughput>0</Throughput>\n"
                               "<ThreadsPerFile>1</ThreadsPerFile>\n"
                               "<IOPriority>3</IOPriority>\n"
                               "<Weight>1</Weight>\n"
                               "</Target>\n");
    }

    void TargetUnitTests::Test_TargetGetXmlWriteBufferContentRandomWithFilePath()
    {
        Target target;
        target.SetRandomDataWriteBufferSize(224433);
        target.SetRandomDataWriteBufferSourcePath("x:\\foo\\bar.baz");
        string sXml = target.GetXml();
        VERIFY_IS_TRUE(sXml == "<Target>\n"
                               "<Path></Path>\n"
                               "<BlockSize>65536</BlockSize>\n"
                               "<BaseFileOffset>0</BaseFileOffset>\n"
                               "<SequentialScan>false</SequentialScan>\n"
                               "<RandomAccess>false</RandomAccess>\n"
                               "<TemporaryFile>false</TemporaryFile>\n"
                               "<UseLargePages>false</UseLargePages>\n"
                               "<WriteBufferContent>\n"
                               "<Pattern>random</Pattern>\n"
                               "<RandomDataSource>\n"
                               "<SizeInBytes>224433</SizeInBytes>\n"
                               "<FilePath>x:\\foo\\bar.baz</FilePath>\n"
                               "</RandomDataSource>\n"
                               "</WriteBufferContent>\n"
                               "<ParallelAsyncIO>false</ParallelAsyncIO>\n"
                               "<StrideSize>65536</StrideSize>\n"
                               "<InterlockedSequential>false</InterlockedSequential>\n"
                               "<ThreadStride>0</ThreadStride>\n"
                               "<MaxFileSize>0</MaxFileSize>\n"
                               "<RequestCount>2</RequestCount>\n"
                               "<WriteRatio>0</WriteRatio>\n"
                               "<Throughput>0</Throughput>\n"
                               "<ThreadsPerFile>1</ThreadsPerFile>\n"
                               "<IOPriority>3</IOPriority>\n"
                               "<Weight>1</Weight>\n"
                               "</Target>\n");
    }

    void TargetUnitTests::Test_TargetGetXmlDisableAllCache()
    {
        Target target;
        target.SetCacheMode(TargetCacheMode::DisableOSCache);
        target.SetWriteThroughMode(WriteThroughMode::On);
        string sXml = target.GetXml();
        VERIFY_IS_TRUE(sXml == "<Target>\n"
            "<Path></Path>\n"
            "<BlockSize>65536</BlockSize>\n"
            "<BaseFileOffset>0</BaseFileOffset>\n"
            "<SequentialScan>false</SequentialScan>\n"
            "<RandomAccess>false</RandomAccess>\n"
            "<TemporaryFile>false</TemporaryFile>\n"
            "<UseLargePages>false</UseLargePages>\n"
            "<DisableOSCache>true</DisableOSCache>\n"
            "<WriteThrough>true</WriteThrough>\n"
            "<WriteBufferContent>\n"
            "<Pattern>sequential</Pattern>\n"
            "</WriteBufferContent>\n"
            "<ParallelAsyncIO>false</ParallelAsyncIO>\n"
            "<StrideSize>65536</StrideSize>\n"
            "<InterlockedSequential>false</InterlockedSequential>\n"
            "<ThreadStride>0</ThreadStride>\n"
            "<MaxFileSize>0</MaxFileSize>\n"
            "<RequestCount>2</RequestCount>\n"
            "<WriteRatio>0</WriteRatio>\n"
            "<Throughput>0</Throughput>\n"
            "<ThreadsPerFile>1</ThreadsPerFile>\n"
            "<IOPriority>3</IOPriority>\n"
            "<Weight>1</Weight>\n"
            "</Target>\n");
    }

    void TargetUnitTests::Test_TargetGetXmlDisableLocalCache()
    {
        Target target;
        target.SetCacheMode(TargetCacheMode::DisableLocalCache);
        string sXml = target.GetXml();
        VERIFY_IS_TRUE(sXml == "<Target>\n"
            "<Path></Path>\n"
            "<BlockSize>65536</BlockSize>\n"
            "<BaseFileOffset>0</BaseFileOffset>\n"
            "<SequentialScan>false</SequentialScan>\n"
            "<RandomAccess>false</RandomAccess>\n"
            "<TemporaryFile>false</TemporaryFile>\n"
            "<UseLargePages>false</UseLargePages>\n"
            "<DisableLocalCache>true</DisableLocalCache>\n"
            "<WriteBufferContent>\n"
            "<Pattern>sequential</Pattern>\n"
            "</WriteBufferContent>\n"
            "<ParallelAsyncIO>false</ParallelAsyncIO>\n"
            "<StrideSize>65536</StrideSize>\n"
            "<InterlockedSequential>false</InterlockedSequential>\n"
            "<ThreadStride>0</ThreadStride>\n"
            "<MaxFileSize>0</MaxFileSize>\n"
            "<RequestCount>2</RequestCount>\n"
            "<WriteRatio>0</WriteRatio>\n"
            "<Throughput>0</Throughput>\n"
            "<ThreadsPerFile>1</ThreadsPerFile>\n"
            "<IOPriority>3</IOPriority>\n"
            "<Weight>1</Weight>\n"
            "</Target>\n");
    }

    void TargetUnitTests::Test_TargetGetXmlDisableOSCache()
    {
        Target target;
        target.SetCacheMode(TargetCacheMode::DisableOSCache);
        string sXml = target.GetXml();
        VERIFY_IS_TRUE(sXml == "<Target>\n"
            "<Path></Path>\n"
            "<BlockSize>65536</BlockSize>\n"
            "<BaseFileOffset>0</BaseFileOffset>\n"
            "<SequentialScan>false</SequentialScan>\n"
            "<RandomAccess>false</RandomAccess>\n"
            "<TemporaryFile>false</TemporaryFile>\n"
            "<UseLargePages>false</UseLargePages>\n"
            "<DisableOSCache>true</DisableOSCache>\n"
            "<WriteBufferContent>\n"
            "<Pattern>sequential</Pattern>\n"
            "</WriteBufferContent>\n"
            "<ParallelAsyncIO>false</ParallelAsyncIO>\n"
            "<StrideSize>65536</StrideSize>\n"
            "<InterlockedSequential>false</InterlockedSequential>\n"
            "<ThreadStride>0</ThreadStride>\n"
            "<MaxFileSize>0</MaxFileSize>\n"
            "<RequestCount>2</RequestCount>\n"
            "<WriteRatio>0</WriteRatio>\n"
            "<Throughput>0</Throughput>\n"
            "<ThreadsPerFile>1</ThreadsPerFile>\n"
            "<IOPriority>3</IOPriority>\n"
            "<Weight>1</Weight>\n"
            "</Target>\n");
    }

    void TargetUnitTests::Test_TargetGetXmlBufferedWriteThrough()
    {
        Target target;
        target.SetWriteThroughMode(WriteThroughMode::On);
        string sXml = target.GetXml();
        VERIFY_IS_TRUE(sXml == "<Target>\n"
            "<Path></Path>\n"
            "<BlockSize>65536</BlockSize>\n"
            "<BaseFileOffset>0</BaseFileOffset>\n"
            "<SequentialScan>false</SequentialScan>\n"
            "<RandomAccess>false</RandomAccess>\n"
            "<TemporaryFile>false</TemporaryFile>\n"
            "<UseLargePages>false</UseLargePages>\n"
            "<WriteThrough>true</WriteThrough>\n"
            "<WriteBufferContent>\n"
            "<Pattern>sequential</Pattern>\n"
            "</WriteBufferContent>\n"
            "<ParallelAsyncIO>false</ParallelAsyncIO>\n"
            "<StrideSize>65536</StrideSize>\n"
            "<InterlockedSequential>false</InterlockedSequential>\n"
            "<ThreadStride>0</ThreadStride>\n"
            "<MaxFileSize>0</MaxFileSize>\n"
            "<RequestCount>2</RequestCount>\n"
            "<WriteRatio>0</WriteRatio>\n"
            "<Throughput>0</Throughput>\n"
            "<ThreadsPerFile>1</ThreadsPerFile>\n"
            "<IOPriority>3</IOPriority>\n"
            "<Weight>1</Weight>\n"
            "</Target>\n");
    }

    void TargetUnitTests::Test_TargetGetXmlMemoryMappedIo()
    {
        Target target;
        target.SetMemoryMappedIoMode(MemoryMappedIoMode::On);
        string sXml = target.GetXml();
        VERIFY_IS_TRUE(sXml == "<Target>\n"
            "<Path></Path>\n"
            "<BlockSize>65536</BlockSize>\n"
            "<BaseFileOffset>0</BaseFileOffset>\n"
            "<SequentialScan>false</SequentialScan>\n"
            "<RandomAccess>false</RandomAccess>\n"
            "<TemporaryFile>false</TemporaryFile>\n"
            "<UseLargePages>false</UseLargePages>\n"
            "<MemoryMappedIo>true</MemoryMappedIo>\n"
            "<WriteBufferContent>\n"
            "<Pattern>sequential</Pattern>\n"
            "</WriteBufferContent>\n"
            "<ParallelAsyncIO>false</ParallelAsyncIO>\n"
            "<StrideSize>65536</StrideSize>\n"
            "<InterlockedSequential>false</InterlockedSequential>\n"
            "<ThreadStride>0</ThreadStride>\n"
            "<MaxFileSize>0</MaxFileSize>\n"
            "<RequestCount>2</RequestCount>\n"
            "<WriteRatio>0</WriteRatio>\n"
            "<Throughput>0</Throughput>\n"
            "<ThreadsPerFile>1</ThreadsPerFile>\n"
            "<IOPriority>3</IOPriority>\n"
            "<Weight>1</Weight>\n"
            "</Target>\n");
    }

    void TargetUnitTests::Test_TargetGetXmlMemoryMappedIoFlushModeViewOfFile()
    {
        Target target;
        target.SetMemoryMappedIoMode(MemoryMappedIoMode::On);
        target.SetMemoryMappedIoFlushMode(MemoryMappedIoFlushMode::ViewOfFile);
        string sXml = target.GetXml();
        VERIFY_IS_TRUE(sXml == "<Target>\n"
            "<Path></Path>\n"
            "<BlockSize>65536</BlockSize>\n"
            "<BaseFileOffset>0</BaseFileOffset>\n"
            "<SequentialScan>false</SequentialScan>\n"
            "<RandomAccess>false</RandomAccess>\n"
            "<TemporaryFile>false</TemporaryFile>\n"
            "<UseLargePages>false</UseLargePages>\n"
            "<MemoryMappedIo>true</MemoryMappedIo>\n"
            "<FlushType>ViewOfFile</FlushType>\n"
            "<WriteBufferContent>\n"
            "<Pattern>sequential</Pattern>\n"
            "</WriteBufferContent>\n"
            "<ParallelAsyncIO>false</ParallelAsyncIO>\n"
            "<StrideSize>65536</StrideSize>\n"
            "<InterlockedSequential>false</InterlockedSequential>\n"
            "<ThreadStride>0</ThreadStride>\n"
            "<MaxFileSize>0</MaxFileSize>\n"
            "<RequestCount>2</RequestCount>\n"
            "<WriteRatio>0</WriteRatio>\n"
            "<Throughput>0</Throughput>\n"
            "<ThreadsPerFile>1</ThreadsPerFile>\n"
            "<IOPriority>3</IOPriority>\n"
            "<Weight>1</Weight>\n"
            "</Target>\n");
    }

    void TargetUnitTests::Test_TargetGetXmlMemoryMappedIoFlushModeNonVolatileMemory()
    {
        Target target;
        target.SetMemoryMappedIoMode(MemoryMappedIoMode::On);
        target.SetMemoryMappedIoFlushMode(MemoryMappedIoFlushMode::NonVolatileMemory);
        string sXml = target.GetXml();
        VERIFY_IS_TRUE(sXml == "<Target>\n"
            "<Path></Path>\n"
            "<BlockSize>65536</BlockSize>\n"
            "<BaseFileOffset>0</BaseFileOffset>\n"
            "<SequentialScan>false</SequentialScan>\n"
            "<RandomAccess>false</RandomAccess>\n"
            "<TemporaryFile>false</TemporaryFile>\n"
            "<UseLargePages>false</UseLargePages>\n"
            "<MemoryMappedIo>true</MemoryMappedIo>\n"
            "<FlushType>NonVolatileMemory</FlushType>\n"
            "<WriteBufferContent>\n"
            "<Pattern>sequential</Pattern>\n"
            "</WriteBufferContent>\n"
            "<ParallelAsyncIO>false</ParallelAsyncIO>\n"
            "<StrideSize>65536</StrideSize>\n"
            "<InterlockedSequential>false</InterlockedSequential>\n"
            "<ThreadStride>0</ThreadStride>\n"
            "<MaxFileSize>0</MaxFileSize>\n"
            "<RequestCount>2</RequestCount>\n"
            "<WriteRatio>0</WriteRatio>\n"
            "<Throughput>0</Throughput>\n"
            "<ThreadsPerFile>1</ThreadsPerFile>\n"
            "<IOPriority>3</IOPriority>\n"
            "<Weight>1</Weight>\n"
            "</Target>\n");
    }

    void TargetUnitTests::Test_TargetGetXmlMemoryMappedIoFlushModeNonVolatileMemoryNoDrain()
    {
        Target target;
        target.SetMemoryMappedIoMode(MemoryMappedIoMode::On);
        target.SetMemoryMappedIoFlushMode(MemoryMappedIoFlushMode::NonVolatileMemoryNoDrain);
        string sXml = target.GetXml();
        VERIFY_IS_TRUE(sXml == "<Target>\n"
            "<Path></Path>\n"
            "<BlockSize>65536</BlockSize>\n"
            "<BaseFileOffset>0</BaseFileOffset>\n"
            "<SequentialScan>false</SequentialScan>\n"
            "<RandomAccess>false</RandomAccess>\n"
            "<TemporaryFile>false</TemporaryFile>\n"
            "<UseLargePages>false</UseLargePages>\n"
            "<MemoryMappedIo>true</MemoryMappedIo>\n"
            "<FlushType>NonVolatileMemoryNoDrain</FlushType>\n"
            "<WriteBufferContent>\n"
            "<Pattern>sequential</Pattern>\n"
            "</WriteBufferContent>\n"
            "<ParallelAsyncIO>false</ParallelAsyncIO>\n"
            "<StrideSize>65536</StrideSize>\n"
            "<InterlockedSequential>false</InterlockedSequential>\n"
            "<ThreadStride>0</ThreadStride>\n"
            "<MaxFileSize>0</MaxFileSize>\n"
            "<RequestCount>2</RequestCount>\n"
            "<WriteRatio>0</WriteRatio>\n"
            "<Throughput>0</Throughput>\n"
            "<ThreadsPerFile>1</ThreadsPerFile>\n"
            "<IOPriority>3</IOPriority>\n"
            "<Weight>1</Weight>\n"
            "</Target>\n");
    }

    void TargetUnitTests::Test_TargetGetXmlRandomAccessHint()
    {
        Target target;
        target.SetRandomAccessHint(true);
        string sXml = target.GetXml();
        VERIFY_IS_TRUE(sXml == "<Target>\n"
            "<Path></Path>\n"
            "<BlockSize>65536</BlockSize>\n"
            "<BaseFileOffset>0</BaseFileOffset>\n"
            "<SequentialScan>false</SequentialScan>\n"
            "<RandomAccess>true</RandomAccess>\n"
            "<TemporaryFile>false</TemporaryFile>\n"
            "<UseLargePages>false</UseLargePages>\n"
            "<WriteBufferContent>\n"
            "<Pattern>sequential</Pattern>\n"
            "</WriteBufferContent>\n"
            "<ParallelAsyncIO>false</ParallelAsyncIO>\n"
            "<StrideSize>65536</StrideSize>\n"
            "<InterlockedSequential>false</InterlockedSequential>\n"
            "<ThreadStride>0</ThreadStride>\n"
            "<MaxFileSize>0</MaxFileSize>\n"
            "<RequestCount>2</RequestCount>\n"
            "<WriteRatio>0</WriteRatio>\n"
            "<Throughput>0</Throughput>\n"
            "<ThreadsPerFile>1</ThreadsPerFile>\n"
            "<IOPriority>3</IOPriority>\n"
            "<Weight>1</Weight>\n"
            "</Target>\n");
    }

    void TargetUnitTests::Test_TargetGetXmlSequentialScanHint()
    {
        Target target;
        target.SetSequentialScanHint(true);
        string sXml = target.GetXml();
        VERIFY_IS_TRUE(sXml == "<Target>\n"
            "<Path></Path>\n"
            "<BlockSize>65536</BlockSize>\n"
            "<BaseFileOffset>0</BaseFileOffset>\n"
            "<SequentialScan>true</SequentialScan>\n"
            "<RandomAccess>false</RandomAccess>\n"
            "<TemporaryFile>false</TemporaryFile>\n"
            "<UseLargePages>false</UseLargePages>\n"
            "<WriteBufferContent>\n"
            "<Pattern>sequential</Pattern>\n"
            "</WriteBufferContent>\n"
            "<ParallelAsyncIO>false</ParallelAsyncIO>\n"
            "<StrideSize>65536</StrideSize>\n"
            "<InterlockedSequential>false</InterlockedSequential>\n"
            "<ThreadStride>0</ThreadStride>\n"
            "<MaxFileSize>0</MaxFileSize>\n"
            "<RequestCount>2</RequestCount>\n"
            "<WriteRatio>0</WriteRatio>\n"
            "<Throughput>0</Throughput>\n"
            "<ThreadsPerFile>1</ThreadsPerFile>\n"
            "<IOPriority>3</IOPriority>\n"
            "<Weight>1</Weight>\n"
            "</Target>\n");
    }

    void TargetUnitTests::Test_TargetGetXmlCombinedAccessHint()
    {
        Target target;
        target.SetSequentialScanHint(true);
        target.SetTemporaryFileHint(true);
        string sXml = target.GetXml();
        VERIFY_IS_TRUE(sXml == "<Target>\n"
            "<Path></Path>\n"
            "<BlockSize>65536</BlockSize>\n"
            "<BaseFileOffset>0</BaseFileOffset>\n"
            "<SequentialScan>true</SequentialScan>\n"
            "<RandomAccess>false</RandomAccess>\n"
            "<TemporaryFile>true</TemporaryFile>\n"
            "<UseLargePages>false</UseLargePages>\n"
            "<WriteBufferContent>\n"
            "<Pattern>sequential</Pattern>\n"
            "</WriteBufferContent>\n"
            "<ParallelAsyncIO>false</ParallelAsyncIO>\n"
            "<StrideSize>65536</StrideSize>\n"
            "<InterlockedSequential>false</InterlockedSequential>\n"
            "<ThreadStride>0</ThreadStride>\n"
            "<MaxFileSize>0</MaxFileSize>\n"
            "<RequestCount>2</RequestCount>\n"
            "<WriteRatio>0</WriteRatio>\n"
            "<Throughput>0</Throughput>\n"
            "<ThreadsPerFile>1</ThreadsPerFile>\n"
            "<IOPriority>3</IOPriority>\n"
            "<Weight>1</Weight>\n"
            "</Target>\n");
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
}
