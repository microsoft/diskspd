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
        target.SetUseRandomAccessPattern(true);

        Random r;
        ThreadParameters tp;
        tp.pRand = &r;
        tp.vTargets.push_back(target);

        TimeSpan timespan;
        tp.pTimeSpan = &timespan;

        tp.vullPrivateSequentialOffsets.push_back(0);
        tp.vullFileSizes.push_back(3000);

        for( int i = 0; i < 10; ++i )
        {
            UINT64 nextOffset = IORequestGenerator::GetNextFileOffset(tp, 0, 0);
            VERIFY_IS_GREATER_THAN_OR_EQUAL(nextOffset, 1000);
            VERIFY_IS_LESS_THAN_OR_EQUAL(nextOffset, 2000);
            VERIFY_ARE_EQUAL(nextOffset % 500, 0);
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

        tp.vullPrivateSequentialOffsets.push_back(0);
        tp.vullFileSizes.push_back(3000);

        UINT64 nextOffset;

        nextOffset = IORequestGenerator::GetNextFileOffset(tp, 0, FIRST_OFFSET);
        VERIFY_ARE_EQUAL(nextOffset, 1000);
        nextOffset = IORequestGenerator::GetNextFileOffset(tp, 0, 0);
        VERIFY_ARE_EQUAL(nextOffset, 1500);
        nextOffset = IORequestGenerator::GetNextFileOffset(tp, 0, 0);
        VERIFY_ARE_EQUAL(nextOffset, 2000);
        nextOffset = IORequestGenerator::GetNextFileOffset(tp, 0, 0);
        VERIFY_ARE_EQUAL(nextOffset, 1000);
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

        tp1.vullFileSizes.push_back(3000);
        tp2.vullFileSizes.push_back(3000);

        UINT64 nextOffset;

        // begin at base
        nextOffset = IORequestGenerator::GetNextFileOffset(tp1, 0, FIRST_OFFSET);
        VERIFY_ARE_EQUAL(nextOffset, 1000);
        nextOffset = IORequestGenerator::GetNextFileOffset(tp1, 0, 0);
        VERIFY_ARE_EQUAL(nextOffset, 1500);
        nextOffset = IORequestGenerator::GetNextFileOffset(tp2, 0, FIRST_OFFSET);
        VERIFY_ARE_EQUAL(nextOffset, 2000);
        nextOffset = IORequestGenerator::GetNextFileOffset(tp1, 0, 0);
        VERIFY_ARE_EQUAL(nextOffset, 1000);
        nextOffset = IORequestGenerator::GetNextFileOffset(tp2, 0, 0);
        VERIFY_ARE_EQUAL(nextOffset, 1500);
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

        tp.vullPrivateSequentialOffsets.push_back(0);
        tp.vullFileSizes.push_back(3000);

        UINT64 nextOffset;

        {
            UINT64 aOff[] = { 1000, 1500, 2000, 1000, 1500 };
            vector<UINT64> vOff(aOff, aOff + _countof(aOff));

            nextOffset = IORequestGenerator::GetNextFileOffset(tp, 0, FIRST_OFFSET);
            for (auto off : vOff)
            {
                VERIFY_ARE_EQUAL(nextOffset, off, L"case 1");
                nextOffset = IORequestGenerator::GetNextFileOffset(tp, 0, nextOffset);
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
        tp.vTargets[0].SetUseRandomAccessPattern(false);
        tp.vTargets[1].SetUseRandomAccessPattern(false);

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
        tp.vTargets[0].SetUseRandomAccessPattern(false);
        tp.vTargets[1].SetUseRandomAccessPattern(false);

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
        tp.vullPrivateSequentialOffsets.push_back(0);
        tp.vullFileSizes.push_back(3000);

        // this is equivalent to -c2000 -T250 -s500 -b1000

        UINT64 nextOffset;

        // relative thread zero should loop back to base
        tp.ulThreadNo = 0;
        tp.ulRelativeThreadNo = 0;
        {
            UINT64 aOff[] = { 0, 500, 1000, 1500, 2000, 0, 500 };
            vector<UINT64> vOff(aOff, aOff + _countof(aOff));

            nextOffset = IORequestGenerator::GetNextFileOffset(tp, 0, FIRST_OFFSET);
            for (auto off : vOff)
            {
                VERIFY_ARE_EQUAL(nextOffset, off, L"case 1");
                nextOffset = IORequestGenerator::GetNextFileOffset(tp, 0, nextOffset);
            }
        }

        // relative thread one should loop back directly to initial offset
        // it will also detect and handle not issuing an IO spanning eof.
        tp.ulThreadNo = 1;
        tp.ulRelativeThreadNo = 1;
        {
            UINT64 aOff[] = { 250, 750, 1250, 1750, 250, 750 };
            vector<UINT64> vOff(aOff, aOff + _countof(aOff));

            nextOffset = IORequestGenerator::GetNextFileOffset(tp, 0, FIRST_OFFSET);
            for (auto off : vOff)
            {
                VERIFY_ARE_EQUAL(nextOffset, off, L"case 2");
                nextOffset = IORequestGenerator::GetNextFileOffset(tp, 0, nextOffset);
            }
        }

        // increasing the stride, relative thread one will loop back to an earlier offset
        // before returning to its initial offset
        tp.vTargets[0].SetThreadStrideInBytes(750);
        {
            UINT64 aOff[] = { 750, 1250, 1750, 250, 750, 1250 };
            vector<UINT64> vOff(aOff, aOff + _countof(aOff));

            nextOffset = IORequestGenerator::GetNextFileOffset(tp, 0, FIRST_OFFSET);
            for (auto off : vOff)
            {
                VERIFY_ARE_EQUAL(nextOffset, off, L"case 3");
                nextOffset = IORequestGenerator::GetNextFileOffset(tp, 0, nextOffset);
            }
        }
    }

    void IORequestGeneratorUnitTests::Test_SequentialWithStrideNonInterleaved()
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
        tp.vullPrivateSequentialOffsets.push_back(0);
        tp.vullFileSizes.push_back(3000);

        // this is equivalent to -c2000 -T250 -s500 -b1000

        UINT64 nextOffset;

        // relative thread zero should loop back to base
        tp.ulThreadNo = 0;
        tp.ulRelativeThreadNo = 0;
        {
            UINT64 aOff[] = { 0, 250, 500, 750, 1000, 1250, 1500, 1750, 2000, 0, 250 };
            vector<UINT64> vOff(aOff, aOff + _countof(aOff));

            nextOffset = IORequestGenerator::GetNextFileOffset(tp, 0, FIRST_OFFSET);
            for (auto off : vOff)
            {
                VERIFY_ARE_EQUAL(nextOffset, off, L"case 1");
                nextOffset = IORequestGenerator::GetNextFileOffset(tp, 0, nextOffset);
            }
        }

        // relative thread one should also loop back to base
        tp.ulThreadNo = 1;
        tp.ulRelativeThreadNo = 1;
        {
            UINT64 aOff[] = { 500, 750, 1000, 1250, 1500, 1750, 2000, 0, 250, 500, 750 };
            vector<UINT64> vOff(aOff, aOff + _countof(aOff));

            nextOffset = IORequestGenerator::GetNextFileOffset(tp, 0, FIRST_OFFSET);
            for (auto off : vOff)
            {
                VERIFY_ARE_EQUAL(nextOffset, off, L"case 2");
                nextOffset = IORequestGenerator::GetNextFileOffset(tp, 0, nextOffset);
            }
        }

        // making the stride uneven w.r.t the alignment ... thread zero is the same
        tp.vTargets[0].SetThreadStrideInBytes(800);
        tp.ulThreadNo = 0;
        tp.ulRelativeThreadNo = 0;
        {
            UINT64 aOff[] = { 0, 250, 500, 750, 1000, 1250, 1500, 1750, 2000, 0, 250 };
            vector<UINT64> vOff(aOff, aOff + _countof(aOff));

            nextOffset = IORequestGenerator::GetNextFileOffset(tp, 0, FIRST_OFFSET);
            for (auto off : vOff)
            {
                VERIFY_ARE_EQUAL(nextOffset, off, L"case 3");
                nextOffset = IORequestGenerator::GetNextFileOffset(tp, 0, nextOffset);
            }
        }

        // while thread one loops back to an early point in the file, but not base
        tp.ulThreadNo = 1;
        tp.ulRelativeThreadNo = 1;
        {
            UINT64 aOff[] = { 800, 1050, 1300, 1550, 1800, 50, 300, 550, 800, 1050 };
            vector<UINT64> vOff(aOff, aOff + _countof(aOff));

            nextOffset = IORequestGenerator::GetNextFileOffset(tp, 0, FIRST_OFFSET);
            for (auto off : vOff)
            {
                VERIFY_ARE_EQUAL(nextOffset, off, L"case 4");
                nextOffset = IORequestGenerator::GetNextFileOffset(tp, 0, nextOffset);
            }
        }
    }
}
