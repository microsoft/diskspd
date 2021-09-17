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
#include "IORequestGenerator.UnitTests.h"
#include "Common.h"
#include "IORequestGenerator.h"
#include <stdlib.h>

using namespace WEX::TestExecution;
using namespace WEX::Logging;

namespace UnitTests
{
    void IORequestGeneratorUnitTests::Test_GetFilesToPrecreate1()
    {
        Target target1;
        target1.SetPath("file1.txt");
        target1.SetFileSize(100);

        Target target2;
        target2.SetPath("file2.txt");

        Target target3;
        target3.SetPath("file1.txt");
        target3.SetFileSize(150);

        Target target4;
        target4.SetPath("file2.txt");
        target4.SetFileSize(200);

        Target target5;
        target5.SetPath("file3.txt");

        Target target6;
        target6.SetPath("file4.txt");
        target6.SetFileSize(300);

        Target target7;
        target7.SetPath("file4.txt");
        target7.SetFileSize(300);

        Target target8;
        target8.SetPath("file5.txt");
        target8.SetFileSize(300);

        Target target9;
        target9.SetPath("file5.txt");
        target9.SetFileSize(300);
        target9.SetZeroWriteBuffers(true);

        TimeSpan timeSpan1;
        TimeSpan timeSpan2;
        timeSpan1.AddTarget(target1);
        timeSpan1.AddTarget(target2);
        timeSpan2.AddTarget(target3);
        timeSpan2.AddTarget(target4);
        timeSpan2.AddTarget(target5);
        timeSpan2.AddTarget(target6);
        timeSpan2.AddTarget(target7);
        timeSpan2.AddTarget(target8);
        timeSpan2.AddTarget(target9);

        Profile profile;
        profile.AddTimeSpan(timeSpan1);
        profile.AddTimeSpan(timeSpan2);

        {
            profile.SetPrecreateFiles(PrecreateFiles::UseMaxSize);
            IORequestGenerator io;
            vector<struct IORequestGenerator::CreateFileParameters> v = io._GetFilesToPrecreate(profile);
            VERIFY_ARE_EQUAL(v.size(), (size_t)3);

            VERIFY_ARE_EQUAL(v[0].sPath, "file1.txt");
            VERIFY_ARE_EQUAL(v[0].fZeroWriteBuffers, false);
            VERIFY_ARE_EQUAL(v[0].ullFileSize, 150);

            VERIFY_ARE_EQUAL(v[1].sPath, "file2.txt");
            VERIFY_ARE_EQUAL(v[1].fZeroWriteBuffers, false);
            VERIFY_ARE_EQUAL(v[1].ullFileSize, 200);

            VERIFY_ARE_EQUAL(v[2].sPath, "file4.txt");
            VERIFY_ARE_EQUAL(v[2].fZeroWriteBuffers, false);
            VERIFY_ARE_EQUAL(v[2].ullFileSize, 300);
        }

        {
            profile.SetPrecreateFiles(PrecreateFiles::OnlyFilesWithConstantSizes);
            IORequestGenerator io;
            vector<struct IORequestGenerator::CreateFileParameters> v = io._GetFilesToPrecreate(profile);
            VERIFY_ARE_EQUAL(v.size(), (size_t)1);

            VERIFY_ARE_EQUAL(v[0].sPath, "file4.txt");
            VERIFY_ARE_EQUAL(v[0].fZeroWriteBuffers, false);
            VERIFY_ARE_EQUAL(v[0].ullFileSize, 300);
        }

        {
            profile.SetPrecreateFiles(PrecreateFiles::OnlyFilesWithConstantOrZeroSizes);
            IORequestGenerator io;
            vector<struct IORequestGenerator::CreateFileParameters> v = io._GetFilesToPrecreate(profile);
            VERIFY_ARE_EQUAL(v.size(), (size_t)2);

            VERIFY_ARE_EQUAL(v[0].sPath, "file2.txt");
            VERIFY_ARE_EQUAL(v[0].fZeroWriteBuffers, false);
            VERIFY_ARE_EQUAL(v[0].ullFileSize, 200);

            VERIFY_ARE_EQUAL(v[1].sPath, "file4.txt");
            VERIFY_ARE_EQUAL(v[1].fZeroWriteBuffers, false);
            VERIFY_ARE_EQUAL(v[1].ullFileSize, 300);
        }

    }

    void IORequestGeneratorUnitTests::Test_GetFilesToPrecreate2()
    {
        Target target1;
        target1.SetPath("file1.txt");
        target1.SetFileSize(100);

        Target target2;
        target2.SetPath("file2.txt");
        target2.SetFileSize(160);

        Target target3;
        target3.SetPath("file1.txt");
        target3.SetFileSize(150);

        Target target4;
        target4.SetPath("file2.txt");
        target4.SetFileSize(200);


        TimeSpan timeSpan1;
        TimeSpan timeSpan2;
        timeSpan1.AddTarget(target1);
        timeSpan1.AddTarget(target2);
        timeSpan2.AddTarget(target3);
        timeSpan2.AddTarget(target4);

        Profile profile;
        profile.AddTimeSpan(timeSpan1);
        profile.AddTimeSpan(timeSpan2);

        profile.SetPrecreateFiles(PrecreateFiles::UseMaxSize);
        IORequestGenerator io;
        vector<struct IORequestGenerator::CreateFileParameters> v = io._GetFilesToPrecreate(profile);
        VERIFY_ARE_EQUAL(v.size(), (size_t)2);

        VERIFY_ARE_EQUAL(v[0].sPath, "file1.txt");
        VERIFY_ARE_EQUAL(v[0].fZeroWriteBuffers, false);
        VERIFY_ARE_EQUAL(v[0].ullFileSize, 150);

        VERIFY_ARE_EQUAL(v[1].sPath, "file2.txt");
        VERIFY_ARE_EQUAL(v[1].fZeroWriteBuffers, false);
        VERIFY_ARE_EQUAL(v[1].ullFileSize, 200);
    }

    void IORequestGeneratorUnitTests::Test_GetFilesToPrecreateConstantSizes()
    {
        Target target1;
        target1.SetPath("file1.txt");

        Target target2;
        target2.SetPath("file1.txt");
        target2.SetFileSize(160);

        TimeSpan timeSpan1;
        timeSpan1.AddTarget(target1);
        timeSpan1.AddTarget(target2);

        Profile profile;
        profile.AddTimeSpan(timeSpan1);

        profile.SetPrecreateFiles(PrecreateFiles::OnlyFilesWithConstantSizes);
        IORequestGenerator io;
        vector<struct IORequestGenerator::CreateFileParameters> v = io._GetFilesToPrecreate(profile);
        VERIFY_ARE_EQUAL(v.size(), (size_t)0);
    }

    void IORequestGeneratorUnitTests::Test_GetFilesToPrecreateConstantOrZeroSizes()
    {
        Target target1;
        target1.SetPath("file1.txt");

        Target target2;
        target2.SetPath("file1.txt");
        target2.SetFileSize(160);

        TimeSpan timeSpan1;
        timeSpan1.AddTarget(target1);
        timeSpan1.AddTarget(target2);

        Profile profile;
        profile.AddTimeSpan(timeSpan1);

        profile.SetPrecreateFiles(PrecreateFiles::OnlyFilesWithConstantOrZeroSizes);
        IORequestGenerator io;
        vector<struct IORequestGenerator::CreateFileParameters> v = io._GetFilesToPrecreate(profile);
        VERIFY_ARE_EQUAL(v.size(), (size_t)1);
        VERIFY_ARE_EQUAL(v[0].sPath, "file1.txt");
        VERIFY_ARE_EQUAL(v[0].fZeroWriteBuffers, false);
        VERIFY_ARE_EQUAL(v[0].ullFileSize, 160);
    }

    void IORequestGeneratorUnitTests::Test_GetFilesToPrecreateUseMaxSize()
    {
        Target target1;
        target1.SetPath("file1.txt");

        Target target2;
        target2.SetPath("file1.txt");

        TimeSpan timeSpan1;
        timeSpan1.AddTarget(target1);
        timeSpan1.AddTarget(target2);

        Profile profile;
        profile.AddTimeSpan(timeSpan1);

        profile.SetPrecreateFiles(PrecreateFiles::UseMaxSize);
        IORequestGenerator io;
        vector<struct IORequestGenerator::CreateFileParameters> v = io._GetFilesToPrecreate(profile);
        VERIFY_ARE_EQUAL(v.size(), (size_t)0);
    }

    void IORequestGeneratorUnitTests::Test_GetNextFileOffsetRandom()
    {
        Target target;
        target.SetBaseFileOffsetInBytes(1000);
        target.SetBlockAlignmentInBytes(500);
        target.SetBlockSizeInBytes(1000);
        target.SetRandomRatio(100);

        Random r;
        ThreadParameters tp;
        tp.pRand = &r;
        tp.vTargets.push_back(target);

        TimeSpan timespan;
        tp.pTimeSpan = &timespan;

        ThreadTargetState tts(&tp, 0, 3000);
        IORequest ior(tp.pRand);

        ULARGE_INTEGER nextOffset;

        for( int i = 0; i < 10; ++i )
        {
            tts.NextIORequest(ior);
            nextOffset.LowPart = ior.GetOverlapped()->Offset;
            nextOffset.HighPart = ior.GetOverlapped()->OffsetHigh;

            VERIFY_IS_GREATER_THAN_OR_EQUAL(nextOffset.QuadPart, 1000);
            VERIFY_IS_LESS_THAN_OR_EQUAL(nextOffset.QuadPart, 2000);
            VERIFY_ARE_EQUAL(nextOffset.QuadPart % 500, 0);
        }
    }

    void IORequestGeneratorUnitTests::Test_GetNextFileOffsetSequential()
    {
        Target target;
        target.SetBaseFileOffsetInBytes(1000);
        target.SetBlockAlignmentInBytes(500);
        target.SetBlockSizeInBytes(1000);

        Random r;
        ThreadParameters tp;
        tp.pRand = &r;
        tp.vTargets.push_back(target);

        TimeSpan timespan;
        tp.pTimeSpan = &timespan;

        ThreadTargetState tts(&tp, 0, 3000);
        IORequest ior(tp.pRand);

        ULARGE_INTEGER nextOffset;

        tts.NextIORequest(ior);
        nextOffset.LowPart = ior.GetOverlapped()->Offset;
        nextOffset.HighPart = ior.GetOverlapped()->OffsetHigh;
        VERIFY_ARE_EQUAL(nextOffset.QuadPart, 1000);
        tts.NextIORequest(ior);
        nextOffset.LowPart = ior.GetOverlapped()->Offset;
        nextOffset.HighPart = ior.GetOverlapped()->OffsetHigh;
        VERIFY_ARE_EQUAL(nextOffset.QuadPart, 1500);
        tts.NextIORequest(ior);
        nextOffset.LowPart = ior.GetOverlapped()->Offset;
        nextOffset.HighPart = ior.GetOverlapped()->OffsetHigh;
        VERIFY_ARE_EQUAL(nextOffset.QuadPart, 2000);
        tts.NextIORequest(ior);
        nextOffset.LowPart = ior.GetOverlapped()->Offset;
        nextOffset.HighPart = ior.GetOverlapped()->OffsetHigh;
        VERIFY_ARE_EQUAL(nextOffset.QuadPart, 1000);
    }

    void IORequestGeneratorUnitTests::Test_GetNextFileOffsetInterlockedSequential()
    {
        Target target;
        target.SetBaseFileOffsetInBytes(1000);
        target.SetBlockAlignmentInBytes(500);
        target.SetBlockSizeInBytes(1000);
        target.SetUseInterlockedSequential(true);

        Random r;
        ThreadParameters tp1;
        ThreadParameters tp2;
        tp1.pRand = &r;
        tp2.pRand = &r;

        UINT64 sharedIndex = 0;
        tp1.pullSharedSequentialOffsets = &sharedIndex;
        tp2.pullSharedSequentialOffsets = &sharedIndex;

        tp1.vTargets.push_back(target);
        tp2.vTargets.push_back(target);

        TimeSpan timespan;
        timespan.SetThreadCount(2);

        tp1.pTimeSpan = &timespan;
        tp2.pTimeSpan = &timespan;

        ThreadTargetState tts1(&tp1, 0, 3000);
        ThreadTargetState tts2(&tp2, 0, 3000);
        IORequest ior(tp1.pRand);

        ULARGE_INTEGER nextOffset;

        // begin at base
        // thread 2 jumps in and continues the pattern (despite FIRST_OFFSET)
        // note that blocksize is 1000, so we loop back at 2000
        tts1.NextIORequest(ior);
        nextOffset.LowPart = ior.GetOverlapped()->Offset;
        nextOffset.HighPart = ior.GetOverlapped()->OffsetHigh;
        VERIFY_ARE_EQUAL(nextOffset.QuadPart, 1000);
        tts1.NextIORequest(ior);
        nextOffset.LowPart = ior.GetOverlapped()->Offset;
        nextOffset.HighPart = ior.GetOverlapped()->OffsetHigh;
        VERIFY_ARE_EQUAL(nextOffset.QuadPart, 1500);
        tts2.NextIORequest(ior);
        nextOffset.LowPart = ior.GetOverlapped()->Offset;
        nextOffset.HighPart = ior.GetOverlapped()->OffsetHigh;
        VERIFY_ARE_EQUAL(nextOffset.QuadPart, 2000);
        tts1.NextIORequest(ior);
        nextOffset.LowPart = ior.GetOverlapped()->Offset;
        nextOffset.HighPart = ior.GetOverlapped()->OffsetHigh;
        VERIFY_ARE_EQUAL(nextOffset.QuadPart, 1000);
        tts2.NextIORequest(ior);
        nextOffset.LowPart = ior.GetOverlapped()->Offset;
        nextOffset.HighPart = ior.GetOverlapped()->OffsetHigh;
        VERIFY_ARE_EQUAL(nextOffset.QuadPart, 1500);
    }

    void IORequestGeneratorUnitTests::Test_GetNextFileOffsetParallelAsyncIO()
    {
        Target target;
        target.SetBaseFileOffsetInBytes(1000);
        target.SetBlockAlignmentInBytes(500);
        target.SetBlockSizeInBytes(1000);
        target.SetUseParallelAsyncIO(true);

        Random r;
        ThreadParameters tp;
        tp.pRand = &r;
        tp.vTargets.push_back(target);

        TimeSpan timespan;
        tp.pTimeSpan = &timespan;

        ThreadTargetState tts(&tp, 0, 3000);
        IORequest ior(tp.pRand);

        ULARGE_INTEGER nextOffset;

        {
            UINT64 aOff[] = { 1000, 1500, 2000, 1000, 1500 };
            vector<UINT64> vOff(aOff, aOff + _countof(aOff));

            tts.InitializeParallelAsyncIORequest(ior);
            tts.NextIORequest(ior);
            for (auto off : vOff)
            {
                nextOffset.LowPart = ior.GetOverlapped()->Offset;
                nextOffset.HighPart = ior.GetOverlapped()->OffsetHigh;
                VERIFY_ARE_EQUAL(nextOffset.QuadPart, off, L"case 1");
                tts.NextIORequest(ior);
            }
        }
    }

    void IORequestGeneratorUnitTests::Test_GetThreadBaseFileOffset()
    {
        Random r;
        ThreadParameters tp;
        Target target;

        tp.pRand = &r;
        target.SetBaseFileOffsetInBytes(1000);
        target.SetBlockAlignmentInBytes(500);
        target.SetBlockSizeInBytes(1000);
        tp.vTargets.push_back(target);
        tp.vTargets.push_back(target);

        UINT64 startingOffset;

        // normal sequential - both threads, each file at base
        tp.vTargets[0].SetRandomRatio(0);
        tp.vTargets[1].SetRandomRatio(0);

        tp.ulThreadNo = 0;
        tp.ulRelativeThreadNo = 0;
        startingOffset = tp.vTargets[0].GetThreadBaseFileOffsetInBytes(tp.ulRelativeThreadNo);
        VERIFY_ARE_EQUAL(startingOffset, 1000, L"sequential");
        startingOffset = tp.vTargets[1].GetThreadBaseFileOffsetInBytes(tp.ulRelativeThreadNo);
        VERIFY_ARE_EQUAL(startingOffset, 1000, L"sequential");

        tp.ulThreadNo = 1;
        tp.ulRelativeThreadNo = 1;
        startingOffset = tp.vTargets[0].GetThreadBaseFileOffsetInBytes(tp.ulRelativeThreadNo);
        VERIFY_ARE_EQUAL(startingOffset, 1000, L"sequential");
        startingOffset = tp.vTargets[1].GetThreadBaseFileOffsetInBytes(tp.ulRelativeThreadNo);
        VERIFY_ARE_EQUAL(startingOffset, 1000, L"sequential");

        // interlocked seq - both threads, each file at base
        tp.vTargets[0].SetUseInterlockedSequential(true);
        tp.vTargets[1].SetUseInterlockedSequential(true);

        tp.ulThreadNo = 0;
        tp.ulRelativeThreadNo = 0;
        startingOffset = tp.vTargets[0].GetThreadBaseFileOffsetInBytes(tp.ulRelativeThreadNo);
        VERIFY_ARE_EQUAL(startingOffset, 1000, L"interlocked sequential");
        startingOffset = tp.vTargets[1].GetThreadBaseFileOffsetInBytes(tp.ulRelativeThreadNo);
        VERIFY_ARE_EQUAL(startingOffset, 1000, L"interlocked sequential");

        tp.ulThreadNo = 1;
        tp.ulRelativeThreadNo = 1;
        startingOffset = tp.vTargets[0].GetThreadBaseFileOffsetInBytes(tp.ulRelativeThreadNo);
        VERIFY_ARE_EQUAL(startingOffset, 1000, L"interlocked sequential");
        startingOffset = tp.vTargets[1].GetThreadBaseFileOffsetInBytes(tp.ulRelativeThreadNo);
        VERIFY_ARE_EQUAL(startingOffset, 1000, L"interlocked sequential");

        tp.vTargets[0].SetUseInterlockedSequential(false);
        tp.vTargets[1].SetUseInterlockedSequential(false);

        // parallel async seq - both threads, each file at base
        tp.vTargets[0].SetUseParallelAsyncIO(true);
        tp.vTargets[1].SetUseParallelAsyncIO(true);

        tp.ulThreadNo = 0;
        tp.ulRelativeThreadNo = 0;
        startingOffset = tp.vTargets[0].GetThreadBaseFileOffsetInBytes(tp.ulRelativeThreadNo);
        VERIFY_ARE_EQUAL(startingOffset, 1000, L"parasync sequential");
        startingOffset = tp.vTargets[1].GetThreadBaseFileOffsetInBytes(tp.ulRelativeThreadNo);
        VERIFY_ARE_EQUAL(startingOffset, 1000, L"parasync sequential");

        tp.ulThreadNo = 1;
        tp.ulRelativeThreadNo = 1;
        startingOffset = tp.vTargets[0].GetThreadBaseFileOffsetInBytes(tp.ulRelativeThreadNo);
        VERIFY_ARE_EQUAL(startingOffset, 1000, L"parasync sequential");
        startingOffset = tp.vTargets[1].GetThreadBaseFileOffsetInBytes(tp.ulRelativeThreadNo);
        VERIFY_ARE_EQUAL(startingOffset, 1000, L"parasync sequential");
    }

    void IORequestGeneratorUnitTests::Test_GetThreadBaseFileOffsetWithStride()
    {
        Random r;
        ThreadParameters tp;
        Target target;

        tp.pRand = &r;
        target.SetBaseFileOffsetInBytes(1000);
        target.SetBlockAlignmentInBytes(500);
        target.SetBlockSizeInBytes(1000);
        target.SetThreadStrideInBytes(5000);
        tp.vTargets.push_back(target);
        target.SetThreadStrideInBytes(50000);
        tp.vTargets.push_back(target);

        UINT64 startingOffset;

        // normal sequential - first at base, second with stride
        tp.vTargets[0].SetRandomRatio(0);
        tp.vTargets[1].SetRandomRatio(0);

        tp.ulThreadNo = 0;
        tp.ulRelativeThreadNo = 0;
        startingOffset = tp.vTargets[0].GetThreadBaseFileOffsetInBytes(tp.ulRelativeThreadNo);
        VERIFY_ARE_EQUAL(startingOffset, 1000, L"sequential");
        startingOffset = tp.vTargets[1].GetThreadBaseFileOffsetInBytes(tp.ulRelativeThreadNo);
        VERIFY_ARE_EQUAL(startingOffset, 1000, L"sequential");

        tp.ulThreadNo = 1;
        tp.ulRelativeThreadNo = 1;
        startingOffset = tp.vTargets[0].GetThreadBaseFileOffsetInBytes(tp.ulRelativeThreadNo);
        VERIFY_ARE_EQUAL(startingOffset, 6000, L"sequential");
        startingOffset = tp.vTargets[1].GetThreadBaseFileOffsetInBytes(tp.ulRelativeThreadNo);
        VERIFY_ARE_EQUAL(startingOffset, 51000, L"sequential");

        // note: unaffected by ulThreadNo
        tp.ulRelativeThreadNo = 2;
        startingOffset = tp.vTargets[0].GetThreadBaseFileOffsetInBytes(tp.ulRelativeThreadNo);
        VERIFY_ARE_EQUAL(startingOffset, 11000, L"sequential");
        startingOffset = tp.vTargets[1].GetThreadBaseFileOffsetInBytes(tp.ulRelativeThreadNo);
        VERIFY_ARE_EQUAL(startingOffset, 101000, L"sequential");

        // parallel async seq - first at base, second with stride
        tp.vTargets[0].SetUseParallelAsyncIO(true);
        tp.vTargets[1].SetUseParallelAsyncIO(true);

        tp.ulThreadNo = 0;
        tp.ulRelativeThreadNo = 0;
        startingOffset = tp.vTargets[0].GetThreadBaseFileOffsetInBytes(tp.ulRelativeThreadNo);
        VERIFY_ARE_EQUAL(startingOffset, 1000, L"parasync sequential");
        startingOffset = tp.vTargets[1].GetThreadBaseFileOffsetInBytes(tp.ulRelativeThreadNo);
        VERIFY_ARE_EQUAL(startingOffset, 1000, L"parasync sequential");

        tp.ulThreadNo = 1;
        tp.ulRelativeThreadNo = 1;
        startingOffset = tp.vTargets[0].GetThreadBaseFileOffsetInBytes(tp.ulRelativeThreadNo);
        VERIFY_ARE_EQUAL(startingOffset, 6000, L"parasync sequential");
        startingOffset = tp.vTargets[1].GetThreadBaseFileOffsetInBytes(tp.ulRelativeThreadNo);
        VERIFY_ARE_EQUAL(startingOffset, 51000, L"parasync sequential");

        // note: unaffected by ulThreadNo
        tp.ulRelativeThreadNo = 2;
        startingOffset = tp.vTargets[0].GetThreadBaseFileOffsetInBytes(tp.ulRelativeThreadNo);
        VERIFY_ARE_EQUAL(startingOffset, 11000, L"parasync sequential");
        startingOffset = tp.vTargets[1].GetThreadBaseFileOffsetInBytes(tp.ulRelativeThreadNo);
        VERIFY_ARE_EQUAL(startingOffset, 101000, L"parasync sequential");

        // interlocked seq - both threads, each file at base
        tp.vTargets[0].SetUseInterlockedSequential(true);
        tp.vTargets[1].SetUseInterlockedSequential(true);

        // note that thread stride is not permitted with interlocked seq
        // this is handled in profile validation
        tp.vTargets[0].SetThreadStrideInBytes(0);
        tp.vTargets[1].SetThreadStrideInBytes(0);

        tp.ulThreadNo = 0;
        tp.ulRelativeThreadNo = 0;
        startingOffset = tp.vTargets[0].GetThreadBaseFileOffsetInBytes(tp.ulRelativeThreadNo);
        VERIFY_ARE_EQUAL(startingOffset, 1000, L"interlocked sequential");
        startingOffset = tp.vTargets[1].GetThreadBaseFileOffsetInBytes(tp.ulRelativeThreadNo);
        VERIFY_ARE_EQUAL(startingOffset, 1000, L"interlocked sequential");

        tp.ulThreadNo = 1;
        tp.ulRelativeThreadNo = 1;
        startingOffset = tp.vTargets[0].GetThreadBaseFileOffsetInBytes(tp.ulRelativeThreadNo);
        VERIFY_ARE_EQUAL(startingOffset, 1000, L"interlocked sequential");
        startingOffset = tp.vTargets[1].GetThreadBaseFileOffsetInBytes(tp.ulRelativeThreadNo);
        VERIFY_ARE_EQUAL(startingOffset, 1000, L"interlocked sequential");
    }

    void IORequestGeneratorUnitTests::Test_SequentialWithStrideInterleaved()
    {
        // this ut handles the case where -T < -s

        Target target;
        target.SetThreadStrideInBytes(250);
        target.SetBlockAlignmentInBytes(500);
        target.SetBlockSizeInBytes(1000);

        Random r;
        ThreadParameters tp;
        tp.pRand = &r;
        tp.vTargets.push_back(target);

        // this is equivalent to -c3000 -T250 -s500 -b1000

        ThreadTargetState tts(&tp, 0, 3000);
        IORequest ior(tp.pRand);

        ULARGE_INTEGER nextOffset;

        // relative thread zero should loop back to base
        tp.ulThreadNo = 0;
        tp.ulRelativeThreadNo = 0;
        {
            UINT64 aOff[] = { 0, 500, 1000, 1500, 2000, 0, 500 };
            vector<UINT64> vOff(aOff, aOff + _countof(aOff));

            tts.NextIORequest(ior);
            for (auto off : vOff)
            {
                nextOffset.LowPart = ior.GetOverlapped()->Offset;
                nextOffset.HighPart = ior.GetOverlapped()->OffsetHigh;
                VERIFY_ARE_EQUAL(nextOffset.QuadPart, off, L"case 1");
                tts.NextIORequest(ior);
            }
        }

        // relative thread one should loop back directly to initial offset
        // it will also detect and handle not issuing an IO spanning eof.
        tp.ulThreadNo = 1;
        tp.ulRelativeThreadNo = 1;
        tts.Reset();
        {
            UINT64 aOff[] = { 250, 750, 1250, 1750, 250, 750 };
            vector<UINT64> vOff(aOff, aOff + _countof(aOff));

            tts.NextIORequest(ior);
            for (auto off : vOff)
            {
                nextOffset.LowPart = ior.GetOverlapped()->Offset;
                nextOffset.HighPart = ior.GetOverlapped()->OffsetHigh;
                VERIFY_ARE_EQUAL(nextOffset.QuadPart, off, L"case 2");
                tts.NextIORequest(ior);
            }
        }

        // increasing the stride, relative thread one will loop back to an earlier offset
        // before returning to its initial offset
        tp.vTargets[0].SetThreadStrideInBytes(750);
        tts.Reset();
        {
            UINT64 aOff[] = { 750, 1250, 1750, 250, 750, 1250 };
            vector<UINT64> vOff(aOff, aOff + _countof(aOff));

            tts.NextIORequest(ior);
            for (auto off : vOff)
            {
                nextOffset.LowPart = ior.GetOverlapped()->Offset;
                nextOffset.HighPart = ior.GetOverlapped()->OffsetHigh;
                VERIFY_ARE_EQUAL(nextOffset.QuadPart, off, L"case 3");
                tts.NextIORequest(ior);
            }
        }
    }

    void IORequestGeneratorUnitTests::Test_SequentialWithStride()
    {
        // this ut handles the case where -T > -s

        Target target;
        target.SetThreadStrideInBytes(500);
        target.SetBlockAlignmentInBytes(250);
        target.SetBlockSizeInBytes(1000);

        Random r;
        ThreadParameters tp;
        tp.pRand = &r;
        tp.vTargets.push_back(target);

        // this is equivalent to -c3000 -T500 -s250 -b1000

        ThreadTargetState tts(&tp, 0, 3000);
        IORequest ior(tp.pRand);

        ULARGE_INTEGER nextOffset;

        // relative thread zero should loop back to base
        tp.ulThreadNo = 0;
        tp.ulRelativeThreadNo = 0;
        {
            UINT64 aOff[] = { 0, 250, 500, 750, 1000, 1250, 1500, 1750, 2000, 0, 250, 500, 750, 1000, 1250, 1500, 1750, 2000 };
            vector<UINT64> vOff(aOff, aOff + _countof(aOff));

            tts.NextIORequest(ior);
            for (auto off : vOff)
            {
                nextOffset.LowPart = ior.GetOverlapped()->Offset;
                nextOffset.HighPart = ior.GetOverlapped()->OffsetHigh;
                VERIFY_ARE_EQUAL(nextOffset.QuadPart, off, L"case 1");
                tts.NextIORequest(ior);
            }
        }

        // relative thread one should also loop back to base
        tp.ulThreadNo = 1;
        tp.ulRelativeThreadNo = 1;
        tts.Reset();
        {
            UINT64 aOff[] = { 500, 750, 1000, 1250, 1500, 1750, 2000, 0, 250, 500, 750, 1000, 1250, 1500, 1750, 2000 };
            vector<UINT64> vOff(aOff, aOff + _countof(aOff));

            tts.NextIORequest(ior);
            for (auto off : vOff)
            {
                nextOffset.LowPart = ior.GetOverlapped()->Offset;
                nextOffset.HighPart = ior.GetOverlapped()->OffsetHigh;
                VERIFY_ARE_EQUAL(nextOffset.QuadPart, off, L"case 2");
                tts.NextIORequest(ior);
            }
        }
    }

    void IORequestGeneratorUnitTests::Test_SequentialWithStrideUneven()
    {
        // this ut handles the case where -T > -s and -T is not a multiple of -s
        // threads io offsets are disjoint

        Target target;
        target.SetThreadStrideInBytes(500);
        target.SetBlockAlignmentInBytes(200);
        target.SetBlockSizeInBytes(200);

        Random r;
        ThreadParameters tp;
        tp.pRand = &r;
        tp.vTargets.push_back(target);

        // this is equivalent to -c3000 -T500 -s200 -b200

        ThreadTargetState tts(&tp, 0, 3000);
        IORequest ior(tp.pRand);

        ULARGE_INTEGER nextOffset;

        // relative thread zero should loop back to base
        tp.ulThreadNo = 0;
        tp.ulRelativeThreadNo = 0;
        {
            UINT64 aOff[] = { 0, 200, 400, 600, 800, 1000, 1200, 1400, 1600, 1800, 2000, 2200, 2400, 2600, 2800, 0, 200, 400 };
            vector<UINT64> vOff(aOff, aOff + _countof(aOff));

            tts.NextIORequest(ior);
            for (auto off : vOff)
            {
                nextOffset.LowPart = ior.GetOverlapped()->Offset;
                nextOffset.HighPart = ior.GetOverlapped()->OffsetHigh;
                VERIFY_ARE_EQUAL(nextOffset.QuadPart, off, L"case 1");
                tts.NextIORequest(ior);
            }
        }

        // relative thread one should also loop back to base
        tp.ulThreadNo = 1;
        tp.ulRelativeThreadNo = 1;
        tts.Reset();
        {
            UINT64 aOff[] = { 500, 700, 900, 1100, 1300, 1500, 1700, 1900, 2100, 2300, 2500, 2700, 100, 300, 500, 700, 900 };
            vector<UINT64> vOff(aOff, aOff + _countof(aOff));

            tts.NextIORequest(ior);
            for (auto off : vOff)
            {
                nextOffset.LowPart = ior.GetOverlapped()->Offset;
                nextOffset.HighPart = ior.GetOverlapped()->OffsetHigh;
                VERIFY_ARE_EQUAL(nextOffset.QuadPart, off, L"case 2");
                tts.NextIORequest(ior);
            }
        }
    }

    void IORequestGeneratorUnitTests::Test_ThreadTargetStateInit()
    {
        // this ut validates that a constructed ThreadTargetState
        // has an initialized IO type, and that it will then agree
        // with the first generated IO's type for homegenous/mixed
        // write mixes.

        Target target;
        target.SetThreadStrideInBytes(500);
        target.SetBlockAlignmentInBytes(200);
        target.SetBlockSizeInBytes(200);

        Random r;
        ThreadParameters tp;
        tp.pRand = &r;
        tp.vTargets.push_back(target);

        UINT32 writeMix[] = { 0, 50, 100 };
        vector<UINT32> vwriteMix(writeMix, writeMix + _countof(writeMix));

        for (auto w : vwriteMix)
        {
            target.SetWriteRatio(w);

            // Validate that ThreadTargetState defines last IO type in all running modes

            VERIFY_ARE_EQUAL(target.GetIOMode(), IOMode::Sequential);
            {
                ThreadTargetState tts(&tp, 0, 3000);
                VERIFY_ARE_NOT_EQUAL(tts._lastIO, IOOperation::Unknown);
                IORequest ior(tp.pRand);
                VERIFY_ARE_EQUAL(tts._lastIO, ior.GetIoType());
            }
            // nothing to undo

            target.SetUseInterlockedSequential(true);
            VERIFY_ARE_EQUAL(target.GetIOMode(), IOMode::InterlockedSequential);
            {
                ThreadTargetState tts(&tp, 0, 3000);
                VERIFY_ARE_NOT_EQUAL(tts._lastIO, IOOperation::Unknown);
                IORequest ior(tp.pRand);
                VERIFY_ARE_EQUAL(tts._lastIO, ior.GetIoType());
            }
            target.SetUseInterlockedSequential(false);

            target.SetRandomRatio(50);
            VERIFY_ARE_EQUAL(target.GetIOMode(), IOMode::Mixed);
            {
                ThreadTargetState tts(&tp, 0, 3000);
                VERIFY_ARE_NOT_EQUAL(tts._lastIO, IOOperation::Unknown);
                IORequest ior(tp.pRand);
                VERIFY_ARE_EQUAL(tts._lastIO, ior.GetIoType());
            }
            target.SetRandomRatio(0);

            target.SetRandomRatio(100);
            VERIFY_ARE_EQUAL(target.GetIOMode(), IOMode::Random);
            {
                ThreadTargetState tts(&tp, 0, 3000);
                VERIFY_ARE_NOT_EQUAL(tts._lastIO, IOOperation::Unknown);
                IORequest ior(tp.pRand);
                VERIFY_ARE_EQUAL(tts._lastIO, ior.GetIoType());
            }
            target.SetRandomRatio(0);

            target.SetUseParallelAsyncIO(true);
            VERIFY_ARE_EQUAL(target.GetIOMode(), IOMode::ParallelAsync);
            {
                ThreadTargetState tts(&tp, 0, 3000);
                VERIFY_ARE_NOT_EQUAL(tts._lastIO, IOOperation::Unknown);
                IORequest ior(tp.pRand);
                VERIFY_ARE_EQUAL(tts._lastIO, ior.GetIoType());
            }
            target.SetUseParallelAsyncIO(false);
        }
    }

    void IORequestGeneratorUnitTests::Test_ThreadTargetStateEffectiveDistPct()
    {
        // this ut validates that a constructed ThreadTargetState
        // has a properly laid out effective distribution given
        // the specified percent distribution on the target.
        //
        // basic cases:
        //  hole
        //  degenerate span (covers no offsets)
        //      rollover to next (degenerate is not last in dist)
        //      apply to last (degenerate is last in dist)

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
            vector<DistributionRange>& ev = tts._vDistributionRange;

            VERIFY_ARE_EQUAL(tts._ioDistributionSpan, (UINT32) 100);
            VERIFY_ARE_EQUAL(tts._vDistributionRange.size(), (size_t) 3);
            VERIFY_ARE_EQUAL(ev[0]._src, (UINT64) 0);
            VERIFY_ARE_EQUAL(ev[0]._span, (UINT64) 10);
            VERIFY_ARE_EQUAL(ev[0]._dst.first, (UINT64) 0);
            VERIFY_ARE_EQUAL(ev[0]._dst.second, (UINT64) 8*KB);
            VERIFY_ARE_EQUAL(ev[1]._src, (UINT64) 10);           ///
            VERIFY_ARE_EQUAL(ev[1]._span, (UINT64) 10);
            VERIFY_ARE_EQUAL(ev[1]._dst.first, (UINT64) 8*KB);
            VERIFY_ARE_EQUAL(ev[1]._dst.second, (UINT64) 12*KB); // note length
            // note hole removed
            VERIFY_ARE_EQUAL(ev[2]._src, (UINT64) 20);           ///
            VERIFY_ARE_EQUAL(ev[2]._span, (UINT64) 80);
            VERIFY_ARE_EQUAL(ev[2]._dst.first, (UINT64) 28*KB);
            VERIFY_ARE_EQUAL(ev[2]._dst.second, (UINT64) 72*KB);
            tp.vTargets.clear();
            v.clear();
        }

        //
        // Degenerate span cases
        //

        // -rdpct10/10:10/10:10/1 + tail
        // this creates the degenerate - non-degenerate case where
        // the non-degenerate must round up.
        v.emplace_back(0, 10, make_pair(0, 10));
        v.emplace_back(10, 10, make_pair(10, 10));
        v.emplace_back(20, 10, make_pair(20, 1));   // degenerate < alignment
        v.emplace_back(30, 70, make_pair(21, 79));
        target.SetDistributionRange(v, DistributionType::Percent);
        tp.vTargets.push_back(target);

        {
            ThreadTargetState tts(&tp, 0, 100*KB);
            vector<DistributionRange>& ev = tts._vDistributionRange;

            VERIFY_ARE_EQUAL(tts._ioDistributionSpan, (UINT32) 100);
            VERIFY_ARE_EQUAL(tts._vDistributionRange.size(), (size_t) 4);
            VERIFY_ARE_EQUAL(ev[0]._src, (UINT64) 0);
            VERIFY_ARE_EQUAL(ev[0]._span, (UINT64) 10);
            VERIFY_ARE_EQUAL(ev[0]._dst.first, (UINT64) 0);
            VERIFY_ARE_EQUAL(ev[0]._dst.second, (UINT64) 8*KB);
            VERIFY_ARE_EQUAL(ev[1]._src, (UINT64) 10);           ///
            VERIFY_ARE_EQUAL(ev[1]._span, (UINT64) 10);
            VERIFY_ARE_EQUAL(ev[1]._dst.first, (UINT64) 8*KB);
            VERIFY_ARE_EQUAL(ev[1]._dst.second, (UINT64) 12*KB);
            VERIFY_ARE_EQUAL(ev[2]._src, (UINT64) 20);           /// degenerate
            VERIFY_ARE_EQUAL(ev[2]._span, (UINT64) 10);
            VERIFY_ARE_EQUAL(ev[2]._dst.first, (UINT64) 20*KB);
            VERIFY_ARE_EQUAL(ev[2]._dst.second, (UINT64) 4*KB);
            VERIFY_ARE_EQUAL(ev[3]._src, (UINT64) 30);           ///
            VERIFY_ARE_EQUAL(ev[3]._span, (UINT64) 70);
            VERIFY_ARE_EQUAL(ev[3]._dst.first, (UINT64) 24*KB);
            VERIFY_ARE_EQUAL(ev[3]._dst.second, (UINT64) 76*KB);
            tp.vTargets.clear();
            v.clear();
        }

        // -rdpct10/96:10/3:80/1
        // tail is degenerate and needs to roll to last
        v.emplace_back(0, 10, make_pair(0, 96));
        v.emplace_back(10, 10, make_pair(96, 3));
        v.emplace_back(20, 80, make_pair(99, 1));   // degenerate tail
        target.SetDistributionRange(v, DistributionType::Percent);
        tp.vTargets.push_back(target);

        {
            ThreadTargetState tts(&tp, 0, 100*KB);
            vector<DistributionRange>& ev = tts._vDistributionRange;

            VERIFY_ARE_EQUAL(tts._ioDistributionSpan, (UINT32) 100);
            VERIFY_ARE_EQUAL(tts._vDistributionRange.size(), (size_t) 2);
            VERIFY_ARE_EQUAL(ev[0]._src, (UINT64) 0);
            VERIFY_ARE_EQUAL(ev[0]._span, (UINT64) 10);
            VERIFY_ARE_EQUAL(ev[0]._dst.first, (UINT64) 0);
            VERIFY_ARE_EQUAL(ev[0]._dst.second, (UINT64) 96*KB);
            VERIFY_ARE_EQUAL(ev[1]._src, (UINT64) 10);           ///
            VERIFY_ARE_EQUAL(ev[1]._span, (UINT64) 90);
            VERIFY_ARE_EQUAL(ev[1]._dst.first, (UINT64) 96*KB);
            VERIFY_ARE_EQUAL(ev[1]._dst.second, (UINT64) 4*KB);
            tp.vTargets.clear();
            v.clear();
        }

        // -rdpct10/5:10/1:80/94
        // the degenerate cannot immediately combine with its non-degenerate predecessor, so rolls over
        // however, since the predecessor is smaller, it prefers to combine in that direction
        v.emplace_back(0, 10, make_pair(0, 5));
        v.emplace_back(10, 10, make_pair(5, 1));
        v.emplace_back(20, 80, make_pair(6, 94));   // non-degenerate tail
        target.SetDistributionRange(v, DistributionType::Percent);
        tp.vTargets.push_back(target);

        {
            ThreadTargetState tts(&tp, 0, 100*KB);
            vector<DistributionRange>& ev = tts._vDistributionRange;

            VERIFY_ARE_EQUAL(tts._ioDistributionSpan, (UINT32) 100);
            VERIFY_ARE_EQUAL(tts._vDistributionRange.size(), (size_t) 2);
            VERIFY_ARE_EQUAL(ev[0]._src, (UINT64) 0);
            VERIFY_ARE_EQUAL(ev[0]._span, (UINT64) 20);
            VERIFY_ARE_EQUAL(ev[0]._dst.first, (UINT64) 0);
            VERIFY_ARE_EQUAL(ev[0]._dst.second, (UINT64) 4*KB);
            VERIFY_ARE_EQUAL(ev[1]._src, (UINT64) 20);           ///
            VERIFY_ARE_EQUAL(ev[1]._span, (UINT64) 80);
            VERIFY_ARE_EQUAL(ev[1]._dst.first, (UINT64) 4*KB);
            VERIFY_ARE_EQUAL(ev[1]._dst.second, (UINT64) 96*KB);
            tp.vTargets.clear();
            v.clear();
        }

        // -rdpct10/1:10/1
        // first two are degenerate and combine, tail rounds up and consumes rest
        v.emplace_back(0, 10, make_pair(0, 1));
        v.emplace_back(10, 10, make_pair(1, 1));
        v.emplace_back(20, 80, make_pair(2, 98));   // non-degenerate tail
        target.SetDistributionRange(v, DistributionType::Percent);
        tp.vTargets.push_back(target);

        {
            ThreadTargetState tts(&tp, 0, 100*KB);
            vector<DistributionRange>& ev = tts._vDistributionRange;

            VERIFY_ARE_EQUAL(tts._ioDistributionSpan, (UINT32) 100);
            VERIFY_ARE_EQUAL(tts._vDistributionRange.size(), (size_t) 2);
            VERIFY_ARE_EQUAL(ev[0]._src, (UINT64) 0);
            VERIFY_ARE_EQUAL(ev[0]._span, (UINT64) 20);
            VERIFY_ARE_EQUAL(ev[0]._dst.first, (UINT64) 0);
            VERIFY_ARE_EQUAL(ev[0]._dst.second, (UINT64) 4*KB);
            VERIFY_ARE_EQUAL(ev[1]._src, (UINT64) 20);           ///
            VERIFY_ARE_EQUAL(ev[1]._span, (UINT64) 80);
            VERIFY_ARE_EQUAL(ev[1]._dst.first, (UINT64) 4*KB);
            VERIFY_ARE_EQUAL(ev[1]._dst.second, (UINT64) 96*KB);
            tp.vTargets.clear();
            v.clear();
        }

        // repeated 10/1
        // This stresses the combination logic
        // The first four should successively combine in a [0, 4KiB) range, the fifth
        // will land in a new [4KiB,8KiB) range.
        //
        for (int n = 1; n <= 5; ++n)
        {
            int m;
            for (m = 0; m < n; ++m)
            {
                v.emplace_back(m*10, 10, make_pair(m*1, 1));
            }
            v.emplace_back(m*10, 100 - m*10, make_pair(m, 100 - m));   // non-degenerate tail
            target.SetDistributionRange(v, DistributionType::Percent);
            tp.vTargets.push_back(target);

            // Fifth case is three ranges; handle seperately. May be worth wrestling down
            // how to combine these to allow us to sweep n further, but for now this is fine.
            if (n == 5)
            {
                ThreadTargetState tts(&tp, 0, 100*KB);
                vector<DistributionRange>& ev = tts._vDistributionRange;

                VERIFY_ARE_EQUAL(tts._ioDistributionSpan, (UINT32) 100);
                VERIFY_ARE_EQUAL(tts._vDistributionRange.size(), (size_t) 3);
                VERIFY_ARE_EQUAL(ev[0]._src, (UINT64) 0);
                VERIFY_ARE_EQUAL(ev[0]._span, (UINT64) 40);
                VERIFY_ARE_EQUAL(ev[0]._dst.first, (UINT64) 0);
                VERIFY_ARE_EQUAL(ev[0]._dst.second, (UINT64) 4*KB);
                VERIFY_ARE_EQUAL(ev[1]._src, (UINT64) 40);           ///
                VERIFY_ARE_EQUAL(ev[1]._span, (UINT64) 10);
                VERIFY_ARE_EQUAL(ev[1]._dst.first, (UINT64) 4*KB);
                VERIFY_ARE_EQUAL(ev[1]._dst.second, (UINT64) 4*KB);
                VERIFY_ARE_EQUAL(ev[2]._src, (UINT64) 50);           ///
                VERIFY_ARE_EQUAL(ev[2]._span, (UINT64) 50);
                VERIFY_ARE_EQUAL(ev[2]._dst.first, (UINT64) 8*KB);
                VERIFY_ARE_EQUAL(ev[2]._dst.second, (UINT64) 92*KB);
                tp.vTargets.clear();
                v.clear();
                break;
            }

            {
                ThreadTargetState tts(&tp, 0, 100*KB);
                vector<DistributionRange>& ev = tts._vDistributionRange;

                VERIFY_ARE_EQUAL(tts._ioDistributionSpan, (UINT32) 100);
                VERIFY_ARE_EQUAL(tts._vDistributionRange.size(), (size_t) 2);
                VERIFY_ARE_EQUAL(ev[0]._src, (UINT64) 0);
                VERIFY_ARE_EQUAL(ev[0]._span, (UINT64) m*10);
                VERIFY_ARE_EQUAL(ev[0]._dst.first, (UINT64) 0);
                VERIFY_ARE_EQUAL(ev[0]._dst.second, (UINT64) 4*KB);
                VERIFY_ARE_EQUAL(ev[1]._src, (UINT64) m*10);           ///
                VERIFY_ARE_EQUAL(ev[1]._span, (UINT64) 100 - m*10);
                VERIFY_ARE_EQUAL(ev[1]._dst.first, (UINT64) 4*KB);
                VERIFY_ARE_EQUAL(ev[1]._dst.second, (UINT64) 96*KB);
                tp.vTargets.clear();
                v.clear();
            }
        }
    }

    void IORequestGeneratorUnitTests::Test_ThreadTargetStateEffectiveDistAbs()
    {
        // this ut validates that a constructed ThreadTargetState
        // has a properly laid out effective distribution given
        // the specified absolute distribution on the target.

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
            vector<DistributionRange>& ev = tts._vDistributionRange;

            VERIFY_ARE_EQUAL(tts._ioDistributionSpan, (UINT32) 100);
            VERIFY_ARE_EQUAL(tts._vDistributionRange.size(), (size_t) 3);
            VERIFY_ARE_EQUAL(ev[0]._src, (UINT64) 0);
            VERIFY_ARE_EQUAL(ev[0]._span, (UINT64) 10);
            VERIFY_ARE_EQUAL(ev[0]._dst.first, (UINT64) 0);
            VERIFY_ARE_EQUAL(ev[0]._dst.second, (UINT64) 1*GB);
            VERIFY_ARE_EQUAL(ev[1]._src, (UINT64) 10);           ///
            VERIFY_ARE_EQUAL(ev[1]._span, (UINT64) 10);
            VERIFY_ARE_EQUAL(ev[1]._dst.first, (UINT64) 1*GB);
            VERIFY_ARE_EQUAL(ev[1]._dst.second, (UINT64) 1*GB); // note length
            // note hole removed
            VERIFY_ARE_EQUAL(ev[2]._src, (UINT64) 20);           ///
            VERIFY_ARE_EQUAL(ev[2]._span, (UINT64) 80);
            VERIFY_ARE_EQUAL(ev[2]._dst.first, (UINT64) 102*GB);
            VERIFY_ARE_EQUAL(ev[2]._dst.second, (UINT64) 98*GB);
            tp.vTargets.clear();
            v.clear();
        }

        // -rdabs10/1G:10/1G:0/100G:20/1T
        // trim - distribution exceeds target size, end is in last range so no IO% trim
        // note same distribution aside from hardening the tail (no autoscale) to a large value
        v.emplace_back(0,10, make_pair(0, 1*GB));
        v.emplace_back(10,10, make_pair(1*GB, 1*GB));
        v.emplace_back(20,0, make_pair(2*GB, 100*GB));
        v.emplace_back(20,80, make_pair(102*GB, 1*TB));
        target.SetDistributionRange(v, DistributionType::Absolute);
        tp.vTargets.push_back(target);

        {
            ThreadTargetState tts(&tp, 0, 200*GB);
            vector<DistributionRange>& ev = tts._vDistributionRange;

            VERIFY_ARE_EQUAL(tts._ioDistributionSpan, (UINT32) 100);
            VERIFY_ARE_EQUAL(tts._vDistributionRange.size(), (size_t) 3);
            VERIFY_ARE_EQUAL(ev[0]._src, (UINT64) 0);
            VERIFY_ARE_EQUAL(ev[0]._span, (UINT64) 10);
            VERIFY_ARE_EQUAL(ev[0]._dst.first, (UINT64) 0);
            VERIFY_ARE_EQUAL(ev[0]._dst.second, (UINT64) 1*GB);
            VERIFY_ARE_EQUAL(ev[1]._src, (UINT64) 10);           ///
            VERIFY_ARE_EQUAL(ev[1]._span, (UINT64) 10);
            VERIFY_ARE_EQUAL(ev[1]._dst.first, (UINT64) 1*GB);
            VERIFY_ARE_EQUAL(ev[1]._dst.second, (UINT64) 1*GB); // note length
            // note hole removed
            VERIFY_ARE_EQUAL(ev[2]._src, (UINT64) 20);           ///
            VERIFY_ARE_EQUAL(ev[2]._span, (UINT64) 80);
            VERIFY_ARE_EQUAL(ev[2]._dst.first, (UINT64) 102*GB);
            VERIFY_ARE_EQUAL(ev[2]._dst.second, (UINT64) 98*GB);
            tp.vTargets.clear();
            v.clear();
        }

        // -rdabs10/100G:10/100G
        // trim - distribution exceeds target size (autoscale tail); target end aligned to last range start
        // drop the IO% of the trailing range
        v.emplace_back(0,10, make_pair(0, 100*GB));
        v.emplace_back(10,10, make_pair(100*GB, 100*GB));
        v.emplace_back(20,80, make_pair(200*GB, 0));
        target.SetDistributionRange(v, DistributionType::Absolute);
        tp.vTargets.push_back(target);

        {
            ThreadTargetState tts(&tp, 0, 200*GB);
            vector<DistributionRange>& ev = tts._vDistributionRange;

            VERIFY_ARE_EQUAL(tts._vDistributionRange.size(), (size_t) 2);
            VERIFY_ARE_EQUAL(tts._ioDistributionSpan, (UINT32) 20);
            VERIFY_ARE_EQUAL(ev[0]._src, (UINT64) 0);
            VERIFY_ARE_EQUAL(ev[0]._span, (UINT64) 10);
            VERIFY_ARE_EQUAL(ev[0]._dst.first, (UINT64) 0);
            VERIFY_ARE_EQUAL(ev[0]._dst.second, (UINT64) 100*GB);
            VERIFY_ARE_EQUAL(ev[1]._src, (UINT64) 10);           ///
            VERIFY_ARE_EQUAL(ev[1]._span, (UINT64) 10);
            VERIFY_ARE_EQUAL(ev[1]._dst.first, (UINT64) 100*GB);
            VERIFY_ARE_EQUAL(ev[1]._dst.second, (UINT64) 100*GB); // note length
            tp.vTargets.clear();
            v.clear();
        }

        // -rdabs10/100G:10/200G
        // trim - distribution exceeds target size (autoscale tail); last range beyond target end
        // drop the IO% of the trailing range
        v.emplace_back(0,10, make_pair(0, 100*GB));
        v.emplace_back(10,10, make_pair(100*GB, 200*GB));
        v.emplace_back(20,80, make_pair(300*GB, 0));
        target.SetDistributionRange(v, DistributionType::Absolute);
        tp.vTargets.push_back(target);

        {
            ThreadTargetState tts(&tp, 0, 200*GB);
            vector<DistributionRange>& ev = tts._vDistributionRange;

            VERIFY_ARE_EQUAL(tts._vDistributionRange.size(), (size_t) 2);
            VERIFY_ARE_EQUAL(tts._ioDistributionSpan, (UINT32) 20);
            VERIFY_ARE_EQUAL(ev[0]._src, (UINT64) 0);
            VERIFY_ARE_EQUAL(ev[0]._span, (UINT64) 10);
            VERIFY_ARE_EQUAL(ev[0]._dst.first, (UINT64) 0);
            VERIFY_ARE_EQUAL(ev[0]._dst.second, (UINT64) 100*GB);
            VERIFY_ARE_EQUAL(ev[1]._src, (UINT64) 10);           ///
            VERIFY_ARE_EQUAL(ev[1]._span, (UINT64) 10);
            VERIFY_ARE_EQUAL(ev[1]._dst.first, (UINT64) 100*GB);
            VERIFY_ARE_EQUAL(ev[1]._dst.second, (UINT64) 100*GB); // note length
            tp.vTargets.clear();
            v.clear();
        }

        //
        // Unaligned intervals
        //

        target.SetBlockAlignmentInBytes(3*KB);
        target.SetBlockSizeInBytes(3*KB);

        // -rdabs10/10K:10/10K
        // not naturally aligned to target, but is naturally aligned to interval (!)
        // 0-10k and 10k-90k - IO all the way to 100K (97K + 3K) is OK
        v.emplace_back(0,10, make_pair(0, 10*KB));
        v.emplace_back(10,10, make_pair(10*KB, 90*KB));
        v.emplace_back(20,80, make_pair(300*GB, 0));
        target.SetDistributionRange(v, DistributionType::Absolute);
        tp.vTargets.push_back(target);

        {
            ThreadTargetState tts(&tp, 0, 100*KB);
            vector<DistributionRange>& ev = tts._vDistributionRange;

            VERIFY_ARE_EQUAL(tts._vDistributionRange.size(), (size_t) 2);
            VERIFY_ARE_EQUAL(tts._ioDistributionSpan, (UINT32) 20);
            VERIFY_ARE_EQUAL(ev[0]._src, (UINT64) 0);
            VERIFY_ARE_EQUAL(ev[0]._span, (UINT64) 10);
            VERIFY_ARE_EQUAL(ev[0]._dst.first, (UINT64) 0);
            VERIFY_ARE_EQUAL(ev[0]._dst.second, (UINT64) 10*KB);
            VERIFY_ARE_EQUAL(ev[1]._src, (UINT64) 10);           ///
            VERIFY_ARE_EQUAL(ev[1]._span, (UINT64) 10);
            VERIFY_ARE_EQUAL(ev[1]._dst.first, (UINT64) 10*KB);
            VERIFY_ARE_EQUAL(ev[1]._dst.second, (UINT64) 90*KB); // note length
            tp.vTargets.clear();
            v.clear();
        }

        // -rdabs10/10K:10/1G
        // previous distribution, target ends in last range (same result, trimmed)
        v.emplace_back(0,10, make_pair(0, 10*KB));
        v.emplace_back(10,10, make_pair(10*KB, 90*KB));
        v.emplace_back(20,80, make_pair(300*GB, 0));
        target.SetDistributionRange(v, DistributionType::Absolute);
        tp.vTargets.push_back(target);

        {
            ThreadTargetState tts(&tp, 0, 100*KB);
            vector<DistributionRange>& ev = tts._vDistributionRange;

            VERIFY_ARE_EQUAL(tts._vDistributionRange.size(), (size_t) 2);
            VERIFY_ARE_EQUAL(tts._ioDistributionSpan, (UINT32) 20);
            VERIFY_ARE_EQUAL(ev[0]._src, (UINT64) 0);
            VERIFY_ARE_EQUAL(ev[0]._span, (UINT64) 10);
            VERIFY_ARE_EQUAL(ev[0]._dst.first, (UINT64) 0);
            VERIFY_ARE_EQUAL(ev[0]._dst.second, (UINT64) 10*KB);
            VERIFY_ARE_EQUAL(ev[1]._src, (UINT64) 10);           ///
            VERIFY_ARE_EQUAL(ev[1]._span, (UINT64) 10);
            VERIFY_ARE_EQUAL(ev[1]._dst.first, (UINT64) 10*KB);
            VERIFY_ARE_EQUAL(ev[1]._dst.second, (UINT64) 90*KB); // note length
            tp.vTargets.clear();
            v.clear();
        }

        // -rdabs10/98k
        // autoscale tail cannot issue IO, so distribution is not extended
        v.emplace_back(0,10, make_pair(0, 98*KB));
        v.emplace_back(10,90, make_pair(98*KB, 0));
        target.SetDistributionRange(v, DistributionType::Absolute);
        tp.vTargets.push_back(target);

        {
            ThreadTargetState tts(&tp, 0, 100*KB);
            vector<DistributionRange>& ev = tts._vDistributionRange;

            VERIFY_ARE_EQUAL(tts._vDistributionRange.size(), (size_t) 1);
            VERIFY_ARE_EQUAL(tts._ioDistributionSpan, (UINT32) 10);
            VERIFY_ARE_EQUAL(ev[0]._src, (UINT64) 0);
            VERIFY_ARE_EQUAL(ev[0]._span, (UINT64) 10);
            VERIFY_ARE_EQUAL(ev[0]._dst.first, (UINT64) 0);
            VERIFY_ARE_EQUAL(ev[0]._dst.second, (UINT64) 98*KB);
            tp.vTargets.clear();
            v.clear();
        }

        // -rdabs10/96k
        // autoscale tail can issue single IO @ 96K
        v.emplace_back(0,10, make_pair(0, 96*KB));
        v.emplace_back(10,90, make_pair(96*KB, 0));
        target.SetDistributionRange(v, DistributionType::Absolute);
        tp.vTargets.push_back(target);

        {
            ThreadTargetState tts(&tp, 0, 100*KB);
            vector<DistributionRange>& ev = tts._vDistributionRange;

            VERIFY_ARE_EQUAL(tts._vDistributionRange.size(), (size_t) 2);
            VERIFY_ARE_EQUAL(tts._ioDistributionSpan, (UINT32) 100);
            VERIFY_ARE_EQUAL(ev[0]._src, (UINT64) 0);
            VERIFY_ARE_EQUAL(ev[0]._span, (UINT64) 10);
            VERIFY_ARE_EQUAL(ev[0]._dst.first, (UINT64) 0);
            VERIFY_ARE_EQUAL(ev[0]._dst.second, (UINT64) 96*KB);
            VERIFY_ARE_EQUAL(ev[1]._src, (UINT64) 10);
            VERIFY_ARE_EQUAL(ev[1]._span, (UINT64) 90);
            VERIFY_ARE_EQUAL(ev[1]._dst.first, (UINT64) 96*KB);
            VERIFY_ARE_EQUAL(ev[1]._dst.second, (UINT64) 4*KB);
            tp.vTargets.clear();
            v.clear();
        }
    }
}