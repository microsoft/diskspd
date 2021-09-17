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
#include "XmlProfileParser.UnitTests.h"
#include <stdlib.h>

using namespace WEX::TestExecution;
using namespace WEX::Logging;
using namespace std;

namespace UnitTests
{
    bool ModuleSetup()
    {
        return true;
    }

    bool ModuleCleanup()
    {
        return true;
    }

    bool XmlProfileParserUnitTests::ClassSetup()
    {
        bool fOk = true;
        _hModule = GetModuleHandle(L"XmlProfileParser.UnitTests.dll");
        char szTempDirPath[MAX_PATH] = {};
        DWORD cch = GetTempPathA(_countof(szTempDirPath), szTempDirPath);
        if (cch != 0)
        {
            _sTempFilePath = szTempDirPath;
            _sTempFilePath += "DiskSpdXmlProfileParser.xml";
            //printf("deleting %s\n", _sTempFilePath.c_str());
            DeleteFileA(_sTempFilePath.c_str());
        }
        else
        {
            fOk = false;
        }
        return fOk;
    }

    bool XmlProfileParserUnitTests::ClassCleanup()
    {
        return true;
    }

    bool XmlProfileParserUnitTests::MethodSetup()
    {
        return true;
    }

    bool XmlProfileParserUnitTests::MethodCleanup()
    {
        DeleteFileA(_sTempFilePath.c_str());
        return true;
    }

    void XmlProfileParserUnitTests::Test_ParseFile()
    {
        FILE *pFile;
        fopen_s(&pFile, _sTempFilePath.c_str(), "wb");
        VERIFY_IS_TRUE(pFile != nullptr);
        fprintf(pFile, "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
                        "<Profile>\n"
                        "    <Verbose>true</Verbose>\n"
                        "    <Progress>15</Progress>\n"
                        "    <TimeSpans>\n"
                        "        <TimeSpan>\n"
                        "            <Duration>10</Duration>\n"
                        "            <Warmup>20</Warmup>\n"
                        "            <Cooldown>30</Cooldown>\n"
                        "            <RandSeed>40</RandSeed>\n"
                        "            <ThreadCount>50</ThreadCount>\n"
                        "            <DisableAffinity>true</DisableAffinity>\n"
                        "            <CompletionRoutines>true</CompletionRoutines>\n"
                        "            <MeasureLatency>true</MeasureLatency>\n"
                        "            <Targets>\n"
                        "                <Target>\n"
                        "                    <Path>testfile.dat</Path>\n"
                        "                    <BlockSize>100</BlockSize>\n"
                        "                    <RequestCount>123</RequestCount>\n"
                        "                    <Random>234</Random>\n"
                        "                    <BaseFileOffset>333</BaseFileOffset>\n"
                        "                    <ParallelAsyncIO>true</ParallelAsyncIO>\n"
                        "                    <DisableOSCache>true</DisableOSCache>\n"
                        "                    <WriteThrough>true</WriteThrough>\n"
                        "                    <ThreadsPerFile>3344</ThreadsPerFile>\n"
                        "                    <ThreadStride>4433</ThreadStride>\n"
                        "                    <FileSize>56</FileSize>\n"
                        "                    <MaxFileSize>561</MaxFileSize>\n"
                        "                    <WriteRatio>84</WriteRatio>\n"
                        "                    <BurstSize>86</BurstSize>\n"
                        "                    <ThinkTime>88</ThinkTime>\n"
                        "                    <SequentialScan>true</SequentialScan>\n"
                        "                    <RandomAccess>true</RandomAccess>\n"
                        "                    <UseLargePages>true</UseLargePages>\n"
                        "                    <Throughput>60</Throughput>\n"
                        "                </Target>\n"
                        "                <Target>\n"
                        "                    <Path>testfile2.dat</Path>\n"
                        "                    <BlockSize>200</BlockSize>\n"
                        "                    <WriteRatio>85</WriteRatio>\n"
                        "                    <IOPriority>2</IOPriority>\n"
                        "                    <StrideSize>2222</StrideSize>\n"
                        "                    <InterlockedSequential>true</InterlockedSequential>\n"
                        "                </Target>\n"
                        "                <Target>\n"
                        "                    <Path>testfile3.dat</Path>\n"
                        "                    <BlockSize>200</BlockSize>\n"
                        "                    <WriteRatio>85</WriteRatio>\n"
                        "                    <IOPriority>2</IOPriority>\n"
                        "                    <StrideSize>2222</StrideSize>\n"
                        "                    <DisableLocalCache>true</DisableLocalCache>\n"
                        "                    <TemporaryFile>true</TemporaryFile>\n"
                        "                    <InterlockedSequential>true</InterlockedSequential>\n"
                        "                </Target>\n"
                        "                <Target>\n"
                        "                    <Path>testfile4.dat</Path>\n"
                        "                    <BlockSize>200</BlockSize>\n"
                        "                    <WriteRatio>85</WriteRatio>\n"
                        "                    <IOPriority>2</IOPriority>\n"
                        "                    <StrideSize>2222</StrideSize>\n"
                        "                    <DisableAllCache>true</DisableAllCache>\n"
                        "                    <TemporaryFile>true</TemporaryFile>\n"
                        "                    <InterlockedSequential>true</InterlockedSequential>\n"
                        "                </Target>\n"
                        "            </Targets>\n"
                        "        </TimeSpan>\n"
                        "        <TimeSpan>\n"
                        "            <Duration>10</Duration>\n"
                        "            <Warmup>20</Warmup>\n"
                        "            <Cooldown>30</Cooldown>\n"
                        "            <RandSeed>40</RandSeed>\n"
                        "            <ThreadCount>50</ThreadCount>\n"
                        "            <DisableAffinity>true</DisableAffinity>\n"
                        "            <MeasureLatency>true</MeasureLatency>\n"
                        "            <Targets>\n"
                        "                <Target>\n"
                        "                    <Path>testfile.dat</Path>\n"
                        "                    <BlockSize>100</BlockSize>\n"
                        "                    <WriteRatio>84</WriteRatio>\n"
                        "                    <MemoryMappedIo>true</MemoryMappedIo>\n"
                        "                </Target>\n"
                        "                <Target>\n"
                        "                    <Path>testfile1.dat</Path>\n"
                        "                    <BlockSize>100</BlockSize>\n"
                        "                    <WriteRatio>84</WriteRatio>\n"
                        "                    <MemoryMappedIo>true</MemoryMappedIo>\n"
                        "                    <FlushType>ViewOfFile</FlushType>"
                        "                </Target>\n"
                        "                <Target>\n"
                        "                    <Path>testfile2.dat</Path>\n"
                        "                    <BlockSize>100</BlockSize>\n"
                        "                    <WriteRatio>84</WriteRatio>\n"
                        "                    <MemoryMappedIo>true</MemoryMappedIo>\n"
                        "                    <FlushType>NonVolatileMemory</FlushType>"
                        "                </Target>\n"
                        "                <Target>\n"
                        "                    <Path>testfile3.dat</Path>\n"
                        "                    <BlockSize>100</BlockSize>\n"
                        "                    <WriteRatio>84</WriteRatio>\n"
                        "                    <MemoryMappedIo>true</MemoryMappedIo>\n"
                        "                    <FlushType>NonVolatileMemoryNoDrain</FlushType>"
                        "                </Target>\n"
                        "            </Targets>\n"
                        "        </TimeSpan>\n"
                        "    </TimeSpans>\n"
                        "</Profile>\n");
/*
        VERIFY_IS_TRUE(t.GetSequentialScanHint() == false);
        VERIFY_IS_TRUE(t.GetRandomAccessHint() == false);
        VERIFY_IS_TRUE(t.GetUseLargePages() == false);*/
        fclose(pFile);

        XmlProfileParser p;
        Profile profile;
        VERIFY_IS_TRUE(p.ParseFile(_sTempFilePath.c_str(), &profile, nullptr, _hModule));
        // note: many conflicts, this ut is a broad blend of specific element parsing, NOT a valid profile
        VERIFY_IS_FALSE(profile.Validate(false));
        VERIFY_IS_TRUE(profile.GetVerbose() == true);
        VERIFY_IS_TRUE(profile.GetProgress() == 15);
// TODO:        VERIFY_IS_TRUE(profile.GetCmdLine().compare("foo -b4K -w84 -a1,4,6 testfile.dat testfile2.dat") == 0);
        VERIFY_IS_TRUE(profile.GetEtwEnabled() == false);
        VERIFY_IS_TRUE(profile.GetEtwProcess() == false);
        VERIFY_IS_TRUE(profile.GetEtwThread() == false);
        VERIFY_IS_TRUE(profile.GetEtwImageLoad() == false);
        VERIFY_IS_TRUE(profile.GetEtwDiskIO() == false);
        VERIFY_IS_TRUE(profile.GetEtwMemoryPageFaults() == false);
        VERIFY_IS_TRUE(profile.GetEtwMemoryHardFaults() == false);
        VERIFY_IS_TRUE(profile.GetEtwNetwork() == false);
        VERIFY_IS_TRUE(profile.GetEtwRegistry() == false);
        VERIFY_IS_TRUE(profile.GetEtwUsePagedMemory() == false);
        VERIFY_IS_TRUE(profile.GetEtwUsePerfTimer() == false);
        VERIFY_IS_TRUE(profile.GetEtwUseSystemTimer() == false);
        VERIFY_IS_TRUE(profile.GetEtwUseCyclesCounter() == false);

        vector<TimeSpan> vSpans(profile.GetTimeSpans());
        VERIFY_ARE_EQUAL(vSpans.size(), (size_t)2);
        VERIFY_ARE_EQUAL(vSpans[0].GetDuration(), (UINT32)10);
        VERIFY_ARE_EQUAL(vSpans[0].GetWarmup(), (UINT32)20);
        VERIFY_ARE_EQUAL(vSpans[0].GetCooldown(), (UINT32)30);
        VERIFY_ARE_EQUAL(vSpans[0].GetRandSeed(), (UINT32)40);
        VERIFY_ARE_EQUAL(vSpans[0].GetThreadCount(), (DWORD)50);
        VERIFY_ARE_EQUAL(vSpans[0].GetRequestCount(), (DWORD)0);
        VERIFY_IS_TRUE(vSpans[0].GetDisableAffinity() == true);
        VERIFY_IS_TRUE(vSpans[0].GetCompletionRoutines() == true);
        VERIFY_IS_TRUE(vSpans[0].GetMeasureLatency() == true);

        vector<Target> vTargets(vSpans[0].GetTargets());
        VERIFY_ARE_EQUAL(vTargets.size(), (size_t)4);
        Target t(vTargets[0]);

        VERIFY_IS_TRUE(t.GetPath().compare("testfile.dat") == 0);
        VERIFY_ARE_EQUAL(t.GetBlockSizeInBytes(), (DWORD)(100));
        VERIFY_ARE_EQUAL(t.GetRequestCount(), (DWORD)123);
        VERIFY_IS_TRUE(t.GetRandomRatio() == 100);
        VERIFY_IS_TRUE(t.GetUseInterlockedSequential() == false);
        VERIFY_ARE_EQUAL(t.GetBlockAlignmentInBytes(), 234);
        VERIFY_ARE_EQUAL(t.GetBaseFileOffsetInBytes(), 333);
        VERIFY_IS_TRUE(t.GetUseParallelAsyncIO() == true);
        VERIFY_IS_TRUE(t.GetCacheMode() == TargetCacheMode::DisableOSCache);
        VERIFY_IS_TRUE(t.GetMemoryMappedIoMode() == MemoryMappedIoMode::Off);
        VERIFY_IS_TRUE(t.GetWriteThroughMode() == WriteThroughMode::On);
        VERIFY_IS_TRUE(t.GetZeroWriteBuffers() == false);
        VERIFY_ARE_EQUAL(t.GetThreadsPerFile(), (DWORD)3344);
        VERIFY_ARE_EQUAL(t.GetThreadStrideInBytes(), 4433);
        VERIFY_IS_TRUE(t.GetCreateFile() == true);
        VERIFY_ARE_EQUAL(t.GetFileSize(), 56);
        VERIFY_ARE_EQUAL(t.GetMaxFileSize(), 561);
        VERIFY_ARE_EQUAL(t.GetWriteRatio(), (DWORD)84);
        VERIFY_IS_TRUE(t.GetUseBurstSize() == true);
        VERIFY_ARE_EQUAL(t.GetBurstSize(), (DWORD)86);
        VERIFY_ARE_EQUAL(t.GetThinkTime(), (DWORD)88);
        VERIFY_IS_TRUE(t.GetEnableThinkTime() == true);
        VERIFY_IS_TRUE(t.GetSequentialScanHint() == true);
        VERIFY_IS_TRUE(t.GetRandomAccessHint() == true);
        VERIFY_IS_TRUE(t.GetTemporaryFileHint() == false);
        VERIFY_IS_TRUE(t.GetUseLargePages() == true);
        VERIFY_IS_TRUE(t.GetIOPriorityHint() == IoPriorityHintNormal);
        VERIFY_ARE_EQUAL(t.GetThroughputInBytesPerMillisecond(), (DWORD)60);

        t = vTargets[1];
        VERIFY_IS_TRUE(t.GetPath().compare("testfile2.dat") == 0);
        VERIFY_ARE_EQUAL(t.GetBlockSizeInBytes(), (DWORD)(200));
        VERIFY_ARE_EQUAL(t.GetRequestCount(), (DWORD)2);
        VERIFY_IS_TRUE(t.GetRandomRatio() == 0);
        VERIFY_IS_TRUE(t.GetUseInterlockedSequential() == true);
        VERIFY_ARE_EQUAL(t.GetBlockAlignmentInBytes(), 2222);
        VERIFY_ARE_EQUAL(t.GetBaseFileOffsetInBytes(), 0);
        VERIFY_IS_TRUE(t.GetUseParallelAsyncIO() == false);
        VERIFY_IS_TRUE(t.GetCacheMode() == TargetCacheMode::Cached);
        VERIFY_IS_TRUE(t.GetMemoryMappedIoMode() == MemoryMappedIoMode::Off);
        VERIFY_IS_TRUE(t.GetWriteThroughMode() == WriteThroughMode::Off);
        VERIFY_IS_TRUE(t.GetZeroWriteBuffers() == false);
        VERIFY_ARE_EQUAL(t.GetThreadsPerFile(), (DWORD)1);
        VERIFY_ARE_EQUAL(t.GetThreadStrideInBytes(), 0);
        VERIFY_IS_TRUE(t.GetCreateFile() == false);
        VERIFY_ARE_EQUAL(t.GetFileSize(), 0);
        VERIFY_ARE_EQUAL(t.GetMaxFileSize(), 0);
        VERIFY_ARE_EQUAL(t.GetWriteRatio(), (DWORD)85);
        VERIFY_IS_TRUE(t.GetUseBurstSize() == false);
        VERIFY_ARE_EQUAL(t.GetBurstSize(), (DWORD)0);
        VERIFY_ARE_EQUAL(t.GetThinkTime(), (DWORD)0);
        VERIFY_IS_TRUE(t.GetEnableThinkTime() == false);
        VERIFY_IS_TRUE(t.GetSequentialScanHint() == false);
        VERIFY_IS_TRUE(t.GetRandomAccessHint() == false);
        VERIFY_IS_TRUE(t.GetTemporaryFileHint() == false);
        VERIFY_IS_TRUE(t.GetUseLargePages() == false);
        VERIFY_IS_TRUE(t.GetIOPriorityHint() == IoPriorityHintLow);
        VERIFY_IS_TRUE(profile.GetPrecreateFiles() == PrecreateFiles::None);

        // verify DisableLocalCache
        t = vTargets[2];
        VERIFY_IS_TRUE(t.GetPath().compare("testfile3.dat") == 0);
        VERIFY_ARE_EQUAL(t.GetBlockSizeInBytes(), (DWORD)(200));
        VERIFY_ARE_EQUAL(t.GetRequestCount(), (DWORD)2);
        VERIFY_IS_TRUE(t.GetRandomRatio() == 0);
        VERIFY_IS_TRUE(t.GetUseInterlockedSequential() == true);
        VERIFY_ARE_EQUAL(t.GetBlockAlignmentInBytes(), 2222);
        VERIFY_ARE_EQUAL(t.GetBaseFileOffsetInBytes(), 0);
        VERIFY_IS_TRUE(t.GetUseParallelAsyncIO() == false);
        VERIFY_IS_TRUE(t.GetCacheMode() == TargetCacheMode::DisableLocalCache);
        VERIFY_IS_TRUE(t.GetMemoryMappedIoMode() == MemoryMappedIoMode::Off);
        VERIFY_IS_TRUE(t.GetWriteThroughMode() == WriteThroughMode::Off);
        VERIFY_IS_TRUE(t.GetZeroWriteBuffers() == false);
        VERIFY_ARE_EQUAL(t.GetThreadsPerFile(), (DWORD)1);
        VERIFY_ARE_EQUAL(t.GetThreadStrideInBytes(), 0);
        VERIFY_IS_TRUE(t.GetCreateFile() == false);
        VERIFY_ARE_EQUAL(t.GetFileSize(), 0);
        VERIFY_ARE_EQUAL(t.GetMaxFileSize(), 0);
        VERIFY_ARE_EQUAL(t.GetWriteRatio(), (DWORD)85);
        VERIFY_IS_TRUE(t.GetUseBurstSize() == false);
        VERIFY_ARE_EQUAL(t.GetBurstSize(), (DWORD)0);
        VERIFY_ARE_EQUAL(t.GetThinkTime(), (DWORD)0);
        VERIFY_IS_TRUE(t.GetEnableThinkTime() == false);
        VERIFY_IS_TRUE(t.GetSequentialScanHint() == false);
        VERIFY_IS_TRUE(t.GetRandomAccessHint() == false);
        VERIFY_IS_TRUE(t.GetTemporaryFileHint() == true);
        VERIFY_IS_TRUE(t.GetUseLargePages() == false);
        VERIFY_IS_TRUE(t.GetIOPriorityHint() == IoPriorityHintLow);
        VERIFY_IS_TRUE(profile.GetPrecreateFiles() == PrecreateFiles::None);

        // verify DisableAllCache
        t = vTargets[3];
        VERIFY_IS_TRUE(t.GetPath().compare("testfile4.dat") == 0);
        VERIFY_ARE_EQUAL(t.GetBlockSizeInBytes(), (DWORD)(200));
        VERIFY_ARE_EQUAL(t.GetRequestCount(), (DWORD)2);
        VERIFY_IS_TRUE(t.GetRandomRatio() == 0);
        VERIFY_IS_TRUE(t.GetUseInterlockedSequential() == true);
        VERIFY_ARE_EQUAL(t.GetBlockAlignmentInBytes(), 2222);
        VERIFY_ARE_EQUAL(t.GetBaseFileOffsetInBytes(), 0);
        VERIFY_IS_TRUE(t.GetUseParallelAsyncIO() == false);
        VERIFY_IS_TRUE(t.GetCacheMode() == TargetCacheMode::DisableOSCache);
        VERIFY_IS_TRUE(t.GetMemoryMappedIoMode() == MemoryMappedIoMode::Off);
        VERIFY_IS_TRUE(t.GetWriteThroughMode() == WriteThroughMode::On);
        VERIFY_IS_TRUE(t.GetZeroWriteBuffers() == false);
        VERIFY_ARE_EQUAL(t.GetThreadsPerFile(), (DWORD)1);
        VERIFY_ARE_EQUAL(t.GetThreadStrideInBytes(), 0);
        VERIFY_IS_TRUE(t.GetCreateFile() == false);
        VERIFY_ARE_EQUAL(t.GetFileSize(), 0);
        VERIFY_ARE_EQUAL(t.GetMaxFileSize(), 0);
        VERIFY_ARE_EQUAL(t.GetWriteRatio(), (DWORD)85);
        VERIFY_IS_TRUE(t.GetUseBurstSize() == false);
        VERIFY_ARE_EQUAL(t.GetBurstSize(), (DWORD)0);
        VERIFY_ARE_EQUAL(t.GetThinkTime(), (DWORD)0);
        VERIFY_IS_TRUE(t.GetEnableThinkTime() == false);
        VERIFY_IS_TRUE(t.GetSequentialScanHint() == false);
        VERIFY_IS_TRUE(t.GetRandomAccessHint() == false);
        VERIFY_IS_TRUE(t.GetTemporaryFileHint() == true);
        VERIFY_IS_TRUE(t.GetUseLargePages() == false);
        VERIFY_IS_TRUE(t.GetIOPriorityHint() == IoPriorityHintLow);
        VERIFY_IS_TRUE(profile.GetPrecreateFiles() == PrecreateFiles::None);

        VERIFY_ARE_EQUAL(vSpans[1].GetDuration(), (UINT32)10);
        VERIFY_ARE_EQUAL(vSpans[1].GetWarmup(), (UINT32)20);
        VERIFY_ARE_EQUAL(vSpans[1].GetCooldown(), (UINT32)30);
        VERIFY_ARE_EQUAL(vSpans[1].GetRandSeed(), (UINT32)40);
        VERIFY_ARE_EQUAL(vSpans[1].GetThreadCount(), (DWORD)50);
        VERIFY_ARE_EQUAL(vSpans[1].GetRequestCount(), (DWORD)0);
        VERIFY_IS_TRUE(vSpans[1].GetDisableAffinity() == true);
        VERIFY_IS_TRUE(vSpans[1].GetMeasureLatency() == true);

        vTargets = vSpans[1].GetTargets();
        VERIFY_ARE_EQUAL(vTargets.size(), (size_t)4);

        t = vTargets[0];
        VERIFY_IS_TRUE(t.GetPath().compare("testfile.dat") == 0);
        VERIFY_ARE_EQUAL(t.GetBlockSizeInBytes(), (DWORD)(100));
        VERIFY_ARE_EQUAL(t.GetWriteRatio(), (DWORD)84);
        VERIFY_IS_TRUE(t.GetMemoryMappedIoMode() == MemoryMappedIoMode::On);

        // verify FlushViewOfFile
        t = vTargets[1];
        VERIFY_IS_TRUE(t.GetPath().compare("testfile1.dat") == 0);
        VERIFY_ARE_EQUAL(t.GetBlockSizeInBytes(), (DWORD)(100));
        VERIFY_ARE_EQUAL(t.GetWriteRatio(), (DWORD)84);
        VERIFY_IS_TRUE(t.GetMemoryMappedIoMode() == MemoryMappedIoMode::On);
        VERIFY_IS_TRUE(t.GetMemoryMappedIoFlushMode() == MemoryMappedIoFlushMode::ViewOfFile);

        // verify RtlFlushNonVolatileMemory
        t = vTargets[2];
        VERIFY_IS_TRUE(t.GetPath().compare("testfile2.dat") == 0);
        VERIFY_ARE_EQUAL(t.GetBlockSizeInBytes(), (DWORD)(100));
        VERIFY_ARE_EQUAL(t.GetWriteRatio(), (DWORD)84);
        VERIFY_IS_TRUE(t.GetMemoryMappedIoMode() == MemoryMappedIoMode::On);
        VERIFY_IS_TRUE(t.GetMemoryMappedIoFlushMode() == MemoryMappedIoFlushMode::NonVolatileMemory);

        // verify RtlFlushNonVolatileMemoryNoDrain
        t = vTargets[3];
        VERIFY_IS_TRUE(t.GetPath().compare("testfile3.dat") == 0);
        VERIFY_ARE_EQUAL(t.GetBlockSizeInBytes(), (DWORD)(100));
        VERIFY_ARE_EQUAL(t.GetWriteRatio(), (DWORD)84);
        VERIFY_IS_TRUE(t.GetMemoryMappedIoMode() == MemoryMappedIoMode::On);
        VERIFY_IS_TRUE(t.GetMemoryMappedIoFlushMode() == MemoryMappedIoFlushMode::NonVolatileMemoryNoDrain);
    }

    void XmlProfileParserUnitTests::Test_ParseFilePrecreateFilesUseMaxSize()
    {
        FILE *pFile;
        fopen_s(&pFile, _sTempFilePath.c_str(), "wb");
        VERIFY_IS_TRUE(pFile != nullptr);
        fprintf(pFile, "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
                       "<Profile>\n"
                       "    <TimeSpans>\n"
                       "        <TimeSpan>\n"
                       "            <Targets>\n"
                       "                <Target>\n"
                       "                    <Path></Path>\n"
                       "                </Target>\n"
                       "            </Targets>\n"
                       "        </TimeSpan>\n"
                       "    </TimeSpans>\n"
                       "    <PrecreateFiles>UseMaxSize</PrecreateFiles>\n"
                       "</Profile>\n");
        fclose(pFile);

        XmlProfileParser p;
        Profile profile;
        VERIFY_IS_TRUE(p.ParseFile(_sTempFilePath.c_str(), &profile, nullptr, _hModule));
        VERIFY_IS_TRUE(profile.Validate(false));
        VERIFY_IS_TRUE(profile.GetPrecreateFiles() == PrecreateFiles::UseMaxSize);
    }

    void XmlProfileParserUnitTests::Test_ParseFilePrecreateFilesOnlyFilesWithConstantSizes()
    {
        FILE *pFile;
        fopen_s(&pFile, _sTempFilePath.c_str(), "wb");
        VERIFY_IS_TRUE(pFile != nullptr);
        fprintf(pFile, "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
                       "<Profile>\n"
                       "    <TimeSpans>\n"
                       "        <TimeSpan>\n"
                       "            <Targets>\n"
                       "                <Target>\n"
                       "                    <Path></Path>\n"
                       "                </Target>\n"
                       "            </Targets>\n"
                       "        </TimeSpan>\n"
                       "    </TimeSpans>\n"
                       "    <PrecreateFiles>CreateOnlyFilesWithConstantSizes</PrecreateFiles>\n"
                       "</Profile>\n");
        fclose(pFile);

        XmlProfileParser p;
        Profile profile;
        VERIFY_IS_TRUE(p.ParseFile(_sTempFilePath.c_str(), &profile, nullptr, _hModule));
        VERIFY_IS_TRUE(profile.Validate(false));
        VERIFY_IS_TRUE(profile.GetPrecreateFiles() == PrecreateFiles::OnlyFilesWithConstantSizes);
    }

    void XmlProfileParserUnitTests::Test_ParseFilePrecreateFilesOnlyFilesWithConstantOrZeroSizes()
    {
        FILE *pFile;
        fopen_s(&pFile, _sTempFilePath.c_str(), "wb");
        VERIFY_IS_TRUE(pFile != nullptr);
        fprintf(pFile, "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
                       "<Profile>\n"
                       "    <TimeSpans>\n"
                       "        <TimeSpan>\n"
                       "            <Targets>\n"
                       "                <Target>\n"
                       "                    <Path></Path>\n"
                       "                </Target>\n"
                       "            </Targets>\n"
                       "        </TimeSpan>\n"
                       "    </TimeSpans>\n"
                       "    <PrecreateFiles>CreateOnlyFilesWithConstantOrZeroSizes</PrecreateFiles>\n"
                       "</Profile>\n");
        fclose(pFile);

        XmlProfileParser p;
        Profile profile;
        VERIFY_IS_TRUE(p.ParseFile(_sTempFilePath.c_str(), &profile, nullptr, _hModule));
        VERIFY_IS_TRUE(profile.Validate(false));
        VERIFY_IS_TRUE(profile.GetPrecreateFiles() == PrecreateFiles::OnlyFilesWithConstantOrZeroSizes);
    }

    void XmlProfileParserUnitTests::Test_ParseFileWriteBufferContentSequential()
    {
        FILE *pFile;
        fopen_s(&pFile, _sTempFilePath.c_str(), "wb");
        VERIFY_IS_TRUE(pFile != nullptr);
        fprintf(pFile, "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
                       "<Profile>\n"
                       "    <TimeSpans>\n"
                       "        <TimeSpan>\n"
                       "            <Targets>\n"
                       "                <Target>\n"
                       "                    <Path></Path>\n"
                       "                    <WriteBufferContent>\n"
                       "                        <Pattern>sequential</Pattern>\n"
                       "                    </WriteBufferContent>\n"
                       "                </Target>\n"
                       "            </Targets>\n"
                       "        </TimeSpan>\n"
                       "    </TimeSpans>\n"
                       "</Profile>\n");
        fclose(pFile);

        XmlProfileParser p;
        Profile profile;
        VERIFY_IS_TRUE(p.ParseFile(_sTempFilePath.c_str(), &profile, nullptr, _hModule));
        VERIFY_IS_TRUE(profile.Validate(false));
        vector<TimeSpan> vTimespans(profile.GetTimeSpans());
        VERIFY_ARE_EQUAL(vTimespans.size(), (size_t)1);
        vector<Target> vTargets(vTimespans[0].GetTargets());
        VERIFY_ARE_EQUAL(vTargets.size(), (size_t)1);
        Target t = vTargets[0];
        VERIFY_IS_TRUE(t.GetZeroWriteBuffers() == false);
        VERIFY_ARE_EQUAL(t.GetRandomDataWriteBufferSize(), 0);
        VERIFY_IS_TRUE(t.GetRandomDataWriteBufferSourcePath() == "");
    }

    void XmlProfileParserUnitTests::Test_ParseGroupAffinity()
    {
        // normal case
        {
            FILE *pFile;
            fopen_s(&pFile, _sTempFilePath.c_str(), "wb");
            VERIFY_IS_TRUE(pFile != nullptr);
            fprintf(pFile, "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
                "<Profile>\n"
                "    <TimeSpans>\n"
                "        <TimeSpan>\n"
                "            <Targets>\n"
                "                <Target>\n"
                "                    <Path></Path>\n"
                "                </Target>\n"
                "            </Targets>\n"
                "            <Affinity>\n"
                "                <AffinityGroupAssignment Group=\"0\" Processor =\"2\"/>\n"
                "                <AffinityGroupAssignment Group=\"1\" Processor =\"32\"/>\n"
                "            </Affinity>\n"
                "        </TimeSpan>\n"
                "    </TimeSpans>\n"
                "</Profile>\n");
            fclose(pFile);

            XmlProfileParser p;
            Profile profile;
            VERIFY_IS_TRUE(p.ParseFile(_sTempFilePath.c_str(), &profile, nullptr, _hModule));
            VERIFY_IS_TRUE(profile.Validate(false));
            vector<TimeSpan> vTimespans(profile.GetTimeSpans());
            VERIFY_ARE_EQUAL(vTimespans.size(), (size_t)1);

            const auto& vAffinity(vTimespans[0].GetAffinityAssignments());
            VERIFY_ARE_EQUAL(vAffinity.size(), (size_t)2);
            VERIFY_ARE_EQUAL(vAffinity[0].wGroup, 0);
            VERIFY_ARE_EQUAL(vAffinity[0].bProc, (BYTE)2);
            VERIFY_ARE_EQUAL(vAffinity[1].wGroup, 1);
            VERIFY_ARE_EQUAL(vAffinity[1].bProc, (BYTE)32);
        }

        // out-of-range processor index (BYTE)
        {
            FILE *pFile;
            fopen_s(&pFile, _sTempFilePath.c_str(), "wb");
            VERIFY_IS_TRUE(pFile != nullptr);
            fprintf(pFile, "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
                "<Profile>\n"
                "    <TimeSpans>\n"
                "        <TimeSpan>\n"
                "            <Targets>\n"
                "                <Target>\n"
                "                    <Path></Path>\n"
                "                </Target>\n"
                "            </Targets>\n"
                "            <Affinity>\n"
                "                <AffinityGroupAssignment Group=\"0\" Processor =\"2\"/>\n"
                "                <AffinityGroupAssignment Group=\"1\" Processor =\"300\"/>\n"
                "            </Affinity>\n"
                "        </TimeSpan>\n"
                "    </TimeSpans>\n"
                "</Profile>\n");
            fclose(pFile);

            XmlProfileParser p;
            Profile profile;
            VERIFY_IS_FALSE(p.ParseFile(_sTempFilePath.c_str(), &profile, nullptr, _hModule));
        }

        // out-of-range group index (WORD)
        {
            FILE *pFile;
            fopen_s(&pFile, _sTempFilePath.c_str(), "wb");
            VERIFY_IS_TRUE(pFile != nullptr);
            fprintf(pFile, "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
                "<Profile>\n"
                "    <TimeSpans>\n"
                "        <TimeSpan>\n"
                "            <Targets>\n"
                "                <Target>\n"
                "                    <Path></Path>\n"
                "                </Target>\n"
                "            </Targets>\n"
                "            <Affinity>\n"
                "                <AffinityGroupAssignment Group=\"0\" Processor =\"2\"/>\n"
                "                <AffinityGroupAssignment Group=\"70000\" Processor =\"32\"/>\n"
                "            </Affinity>\n"
                "        </TimeSpan>\n"
                "    </TimeSpans>\n"
                "</Profile>\n");
            fclose(pFile);

            XmlProfileParser p;
            Profile profile;
            VERIFY_IS_FALSE(p.ParseFile(_sTempFilePath.c_str(), &profile, nullptr, _hModule));
        }
    }

    void XmlProfileParserUnitTests::Test_ParseNonGroupAffinity()
    {
        FILE *pFile;
        fopen_s(&pFile, _sTempFilePath.c_str(), "wb");
        VERIFY_IS_TRUE(pFile != nullptr);
        fprintf(pFile, "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
            "<Profile>\n"
            "    <TimeSpans>\n"
            "        <TimeSpan>\n"
            "            <Targets>\n"
            "                <Target>\n"
            "                    <Path></Path>\n"
            "                </Target>\n"
            "            </Targets>\n"
            "            <Affinity>\n"
            "                <AffinityAssignment>1</AffinityAssignment>\n"
            "                <AffinityAssignment>31</AffinityAssignment>\n"
            "            </Affinity>\n"
            "        </TimeSpan>\n"
            "    </TimeSpans>\n"
            "</Profile>\n");
        fclose(pFile);

        XmlProfileParser p;
        Profile profile;
        VERIFY_IS_TRUE(p.ParseFile(_sTempFilePath.c_str(), &profile, nullptr, _hModule));
        VERIFY_IS_TRUE(profile.Validate(false));
        vector<TimeSpan> vTimespans(profile.GetTimeSpans());
        VERIFY_ARE_EQUAL(vTimespans.size(), (size_t)1);

        // Old style, group is wired to zero.
        const auto& vAffinity(vTimespans[0].GetAffinityAssignments());
        VERIFY_ARE_EQUAL(vAffinity.size(), (size_t)2);
        VERIFY_ARE_EQUAL(vAffinity[0].wGroup, 0);
        VERIFY_ARE_EQUAL(vAffinity[0].bProc, (BYTE)1);
        VERIFY_ARE_EQUAL(vAffinity[1].wGroup, 0);
        VERIFY_ARE_EQUAL(vAffinity[1].bProc, (BYTE)31);
    }
    
    void XmlProfileParserUnitTests::Test_ParseFileWriteBufferContentZero()
    {
        FILE *pFile;
        fopen_s(&pFile, _sTempFilePath.c_str(), "wb");
        VERIFY_IS_TRUE(pFile != nullptr);
        fprintf(pFile, "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
                       "<Profile>\n"
                       "    <TimeSpans>\n"
                       "        <TimeSpan>\n"
                       "            <Targets>\n"
                       "                <Target>\n"
                       "                    <Path></Path>\n"
                       "                    <WriteBufferContent>\n"
                       "                        <Pattern>zero</Pattern>\n"
                       "                    </WriteBufferContent>\n"
                       "                </Target>\n"
                       "            </Targets>\n"
                       "        </TimeSpan>\n"
                       "    </TimeSpans>\n"
                       "</Profile>\n");
        fclose(pFile);

        XmlProfileParser p;
        Profile profile;
        VERIFY_IS_TRUE(p.ParseFile(_sTempFilePath.c_str(), &profile, nullptr, _hModule));
        VERIFY_IS_TRUE(profile.Validate(false));
        vector<TimeSpan> vTimespans(profile.GetTimeSpans());
        VERIFY_ARE_EQUAL(vTimespans.size(), (size_t)1);
        vector<Target> vTargets(vTimespans[0].GetTargets());
        VERIFY_ARE_EQUAL(vTargets.size(), (size_t)1);
        Target t = vTargets[0];
        VERIFY_IS_TRUE(t.GetZeroWriteBuffers() == true);
        VERIFY_ARE_EQUAL(t.GetRandomDataWriteBufferSize(), 0);
        VERIFY_IS_TRUE(t.GetRandomDataWriteBufferSourcePath() == "");
    }

    void XmlProfileParserUnitTests::Test_ParseFileWriteBufferContentRandom()
    {
        FILE *pFile;
        fopen_s(&pFile, _sTempFilePath.c_str(), "wb");
        VERIFY_IS_TRUE(pFile != nullptr);
        fprintf(pFile, "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
                       "<Profile>\n"
                       "    <TimeSpans>\n"
                       "        <TimeSpan>\n"
                       "            <RandomWriteData>true</RandomWriteData>"
                       "            <Targets>\n"
                       "                <Target>\n"
                       "                    <Path></Path>\n"
                       "                </Target>\n"
                       "            </Targets>\n"
                       "        </TimeSpan>\n"
                       "    </TimeSpans>\n"
                       "</Profile>\n");
        fclose(pFile);

        XmlProfileParser p;
        Profile profile;
        VERIFY_IS_TRUE(p.ParseFile(_sTempFilePath.c_str(), &profile, nullptr, _hModule));
        VERIFY_IS_TRUE(profile.Validate(false));
        vector<TimeSpan> vTimespans(profile.GetTimeSpans());
        VERIFY_ARE_EQUAL(vTimespans.size(), (size_t)1);
        VERIFY_IS_TRUE(vTimespans[0].GetRandomWriteData() == true);
        vector<Target> vTargets(vTimespans[0].GetTargets());
        VERIFY_ARE_EQUAL(vTargets.size(), (size_t)1);
        Target t = vTargets[0];
        VERIFY_IS_TRUE(t.GetZeroWriteBuffers() == false);
        VERIFY_ARE_EQUAL(t.GetRandomDataWriteBufferSize(), 0);
        VERIFY_IS_TRUE(t.GetRandomDataWriteBufferSourcePath() == "");
    }

    void XmlProfileParserUnitTests::Test_ParseFileWriteBufferContentRandomNoFilePath()
    {
        FILE *pFile;
        fopen_s(&pFile, _sTempFilePath.c_str(), "wb");
        VERIFY_IS_TRUE(pFile != nullptr);
        fprintf(pFile, "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
                       "<Profile>\n"
                       "    <TimeSpans>\n"
                       "        <TimeSpan>\n"
                       "            <Targets>\n"
                       "                <Target>\n"
                       "                    <Path></Path>\n"
                       "                    <WriteBufferContent>\n"
                       "                        <Pattern>random</Pattern>\n"
                       "                        <RandomDataSource>\n"
                       "                            <SizeInBytes>223311</SizeInBytes>\n"
                       "                        </RandomDataSource>\n"
                       "                    </WriteBufferContent>\n"
                       "                </Target>\n"
                       "            </Targets>\n"
                       "        </TimeSpan>\n"
                       "    </TimeSpans>\n"
                       "</Profile>\n");
        fclose(pFile);

        XmlProfileParser p;
        Profile profile;
        VERIFY_IS_TRUE(p.ParseFile(_sTempFilePath.c_str(), &profile, nullptr, _hModule));
        VERIFY_IS_TRUE(profile.Validate(false));
        vector<TimeSpan> vTimespans(profile.GetTimeSpans());
        VERIFY_ARE_EQUAL(vTimespans.size(), (size_t)1);
        vector<Target> vTargets(vTimespans[0].GetTargets());
        VERIFY_ARE_EQUAL(vTargets.size(), (size_t)1);
        Target t = vTargets[0];
        VERIFY_IS_TRUE(t.GetZeroWriteBuffers() == false);
        VERIFY_ARE_EQUAL(t.GetRandomDataWriteBufferSize(), 223311);
        VERIFY_IS_TRUE(t.GetRandomDataWriteBufferSourcePath() == "");
    }

    void XmlProfileParserUnitTests::Test_ParseFileWriteBufferContentRandomWithFilePath()
    {
        FILE *pFile;
        fopen_s(&pFile, _sTempFilePath.c_str(), "wb");
        VERIFY_IS_TRUE(pFile != nullptr);
        fprintf(pFile, "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
                       "<Profile>\n"
                       "    <TimeSpans>\n"
                       "        <TimeSpan>\n"
                       "            <Targets>\n"
                       "                <Target>\n"
                       "                    <Path></Path>\n"
                       "                    <WriteBufferContent>\n"
                       "                        <Pattern>random</Pattern>\n"
                       "                        <RandomDataSource>\n"
                       "                            <SizeInBytes>223311</SizeInBytes>\n"
                       "                            <FilePath>x:\\foo\\bar.dat</FilePath>\n"
                       "                        </RandomDataSource>\n"
                       "                    </WriteBufferContent>\n"
                       "                </Target>\n"
                       "            </Targets>\n"
                       "        </TimeSpan>\n"
                       "    </TimeSpans>\n"
                       "</Profile>\n");
        fclose(pFile);

        XmlProfileParser p;
        Profile profile;
        VERIFY_IS_TRUE(p.ParseFile(_sTempFilePath.c_str(), &profile, nullptr, _hModule));
        VERIFY_IS_TRUE(profile.Validate(false));
        vector<TimeSpan> vTimespans(profile.GetTimeSpans());
        VERIFY_ARE_EQUAL(vTimespans.size(), (size_t)1);
        vector<Target> vTargets(vTimespans[0].GetTargets());
        VERIFY_ARE_EQUAL(vTargets.size(), (size_t)1);
        Target t = vTargets[0];
        VERIFY_IS_TRUE(t.GetZeroWriteBuffers() == false);
        VERIFY_ARE_EQUAL(t.GetRandomDataWriteBufferSize(), 223311);
        VERIFY_IS_TRUE(t.GetRandomDataWriteBufferSourcePath() == "x:\\foo\\bar.dat");
    }

    void XmlProfileParserUnitTests::Test_ParseFileGlobalRequestCount()
    {
        FILE *pFile;
        fopen_s(&pFile, _sTempFilePath.c_str(), "wb");
        VERIFY_IS_TRUE(pFile != nullptr);
        fprintf(pFile, "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
                       "<Profile>\n"
                       "    <TimeSpans>\n"
                       "        <TimeSpan>\n"
                       "            <ThreadCount>4</ThreadCount>\n"
                       "            <RequestCount>6</RequestCount>\n"
                       "            <Targets>\n"
                       "                <Target>\n"
                       "                    <Path></Path>\n"
                       "                    <Weight>100</Weight>\n"
                       "                    <ThreadTargets>\n"
                       "                        <ThreadTarget>\n"
                       "                            <Thread>0</Thread>\n"
                       "                            <Weight>200</Weight>\n"
                       "                        </ThreadTarget>\n"
                       "                        <ThreadTarget>\n"
                       "                            <Thread>1</Thread>\n"
                       "                            <Weight>50</Weight>\n"
                       "                        </ThreadTarget>\n"
                       "                        <ThreadTarget>\n"
                       "                            <Thread>2</Thread>\n"
                       "                        </ThreadTarget>\n"
                       "                    </ThreadTargets>\n"
                       "                </Target>\n"
                       "                <Target>\n"
                       "                    <Path></Path>\n"
                       "                    <Weight>100</Weight>\n"
                       "                </Target>\n"
                       "            </Targets>\n"
                       "        </TimeSpan>\n"
                       "    </TimeSpans>\n"
                       "</Profile>\n");
        fclose(pFile);

        XmlProfileParser p;
        Profile profile;
        VERIFY_IS_TRUE(p.ParseFile(_sTempFilePath.c_str(), &profile, nullptr, _hModule));
        VERIFY_IS_TRUE(profile.Validate(false));
        vector<TimeSpan> vTimespans(profile.GetTimeSpans());
        VERIFY_ARE_EQUAL(vTimespans.size(), (size_t)1);
        VERIFY_ARE_EQUAL(vTimespans[0].GetThreadCount(), (DWORD)4);
        VERIFY_ARE_EQUAL(vTimespans[0].GetRequestCount(), (DWORD)6);
        vector<Target> vTargets(vTimespans[0].GetTargets());
        VERIFY_ARE_EQUAL(vTargets.size(), (size_t)2);
        VERIFY_ARE_EQUAL(vTargets[0].GetWeight(), (UINT32)100);
        vector<ThreadTarget> vThreadTargets(vTargets[0].GetThreadTargets());
        VERIFY_ARE_EQUAL(vThreadTargets.size(), (size_t)3);
        VERIFY_ARE_EQUAL(vThreadTargets[0].GetThread(), (UINT32)0);
        VERIFY_ARE_EQUAL(vThreadTargets[0].GetWeight(), (UINT32)200);
        VERIFY_ARE_EQUAL(vThreadTargets[1].GetThread(), (UINT32)1);
        VERIFY_ARE_EQUAL(vThreadTargets[1].GetWeight(), (UINT32)50);
        VERIFY_ARE_EQUAL(vThreadTargets[2].GetThread(), (UINT32)2);
        VERIFY_ARE_EQUAL(vThreadTargets[2].GetWeight(), (UINT32)0);
        VERIFY_ARE_EQUAL(vTargets[1].GetWeight(), (UINT32)100);
        VERIFY_ARE_EQUAL(vTargets[1].GetThreadTargets().size(), (size_t)0);
    }

    void XmlProfileParserUnitTests::Test_ParseThroughput()
    {
        const PCHAR xmlDoc = "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
                       "<Profile>\n"
                       "    <TimeSpans>\n"
                       "        <TimeSpan>\n"
                       "            <Targets>\n"
                       "                <Target>\n"
                       "                    <Path></Path>\n"
                       "                    <BlockSize>%s</BlockSize>\n"
                       "                    <Throughput%s>%s</Throughput>\n"
                       "                </Target>\n"
                       "            </Targets>\n"
                       "        </TimeSpan>\n"
                       "    </TimeSpans>\n"
                       "</Profile>\n";
        XmlProfileParser p;
        FILE *pFile = nullptr;

        //
        // Positive varations - units & default
        //

        {
            Profile profile;
            fopen_s(&pFile, _sTempFilePath.c_str(), "wb");
            VERIFY_IS_TRUE(pFile != nullptr);
            fprintf(pFile, xmlDoc, "131072", " unit=\"IOPS\"", "10");
            fclose(pFile);
            pFile = nullptr;

            VERIFY_IS_TRUE(p.ParseFile(_sTempFilePath.c_str(), &profile, nullptr, _hModule));
            VERIFY_IS_TRUE(profile.Validate(false));
            VERIFY_ARE_EQUAL(profile.GetTimeSpans()[0].GetTargets()[0].GetThroughputInBytesPerMillisecond(), (DWORD)((128*1024*10)/1000));
        }

        {
            Profile profile;
            fopen_s(&pFile, _sTempFilePath.c_str(), "wb");
            VERIFY_IS_TRUE(pFile != nullptr);
            fprintf(pFile, xmlDoc, "131072", " unit=\"BPMS\"", "1500");
            fclose(pFile);
            pFile = nullptr;

            VERIFY_IS_TRUE(p.ParseFile(_sTempFilePath.c_str(), &profile, nullptr, _hModule));
            VERIFY_IS_TRUE(profile.Validate(false));
            VERIFY_ARE_EQUAL(profile.GetTimeSpans()[0].GetTargets()[0].GetThroughputInBytesPerMillisecond(), (DWORD)1500);
        }

        {
            Profile profile;
            fopen_s(&pFile, _sTempFilePath.c_str(), "wb");
            VERIFY_IS_TRUE(pFile != nullptr);
            fprintf(pFile, xmlDoc, "131072", "", "1024");
            fclose(pFile);
            pFile = nullptr;

            VERIFY_IS_TRUE(p.ParseFile(_sTempFilePath.c_str(), &profile, nullptr, _hModule));
            VERIFY_IS_TRUE(profile.Validate(false));
            VERIFY_ARE_EQUAL(profile.GetTimeSpans()[0].GetTargets()[0].GetThroughputInBytesPerMillisecond(), (DWORD)1024);
        }

        //
        // Negative variations - bad units / good units with bad data
        //

        {
            Profile profile;
            fopen_s(&pFile, _sTempFilePath.c_str(), "wb");
            VERIFY_IS_TRUE(pFile != nullptr);
            fprintf(pFile, xmlDoc, "131072", " unit=\"FRUIT\"", "1500");
            fclose(pFile);
            pFile = nullptr;

            VERIFY_IS_FALSE(p.ParseFile(_sTempFilePath.c_str(), &profile, nullptr, _hModule));
        }

        {
            Profile profile;
            fopen_s(&pFile, _sTempFilePath.c_str(), "wb");
            VERIFY_IS_TRUE(pFile != nullptr);
            fprintf(pFile, xmlDoc, "131072", " unit=\"IOPS\"", "FRUIT");
            fclose(pFile);
            pFile = nullptr;

            VERIFY_IS_FALSE(p.ParseFile(_sTempFilePath.c_str(), &profile, nullptr, _hModule));
        }

        {
            Profile profile;
            fopen_s(&pFile, _sTempFilePath.c_str(), "wb");
            VERIFY_IS_TRUE(pFile != nullptr);
            fprintf(pFile, xmlDoc, "131072", " unit=\"BPMS\"", "FRUIT");
            fclose(pFile);
            pFile = nullptr;

            VERIFY_IS_FALSE(p.ParseFile(_sTempFilePath.c_str(), &profile, nullptr, _hModule));
        }

        {
            Profile profile;
            fopen_s(&pFile, _sTempFilePath.c_str(), "wb");
            VERIFY_IS_TRUE(pFile != nullptr);
            fprintf(pFile, xmlDoc, "131072", "", "FRUIT");
            fclose(pFile);
            pFile = nullptr;

            VERIFY_IS_FALSE(p.ParseFile(_sTempFilePath.c_str(), &profile, nullptr, _hModule));
        }
    }

    void XmlProfileParserUnitTests::Test_ParseRandomSequentialMixed()
    {
        const PCHAR xmlDoc = "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
                       "<Profile>\n"
                       "    <TimeSpans>\n"
                       "        <TimeSpan>\n"
                       "            <Targets>\n"
                       "                <Target>\n"
                       "                    <Path></Path>\n"
                       "%s"                                             // insert conflicts
                       "                    <Random%s>%s</Random%s>"    // Random/RandomRatio
                       "                </Target>\n"
                       "            </Targets>\n"
                       "        </TimeSpan>\n"
                       "    </TimeSpans>\n"
                       "</Profile>\n";
        XmlProfileParser p;
        FILE *pFile = nullptr;

        // Positive cases

        // Isolated

        {
            Profile profile;
            fopen_s(&pFile, _sTempFilePath.c_str(), "wb");
            VERIFY_IS_TRUE(pFile != nullptr);
            fprintf(pFile, xmlDoc, "", "Ratio", "50", "Ratio");
            fclose(pFile);
            pFile = nullptr;

            VERIFY_IS_TRUE(p.ParseFile(_sTempFilePath.c_str(), &profile, nullptr, _hModule));
            VERIFY_IS_TRUE(profile.Validate(false));
            VERIFY_ARE_EQUAL(profile.GetTimeSpans()[0].GetTargets()[0].GetRandomRatio(), (UINT32) 50);
        }

        // With <Random>

        {
            Profile profile;
            fopen_s(&pFile, _sTempFilePath.c_str(), "wb");
            VERIFY_IS_TRUE(pFile != nullptr);
            fprintf(pFile, xmlDoc, "<Random>8192</Random>", "Ratio", "50", "Ratio");
            fclose(pFile);
            pFile = nullptr;

            VERIFY_IS_TRUE(p.ParseFile(_sTempFilePath.c_str(), &profile, nullptr, _hModule));
            VERIFY_IS_TRUE(profile.Validate(false));
            VERIFY_ARE_EQUAL(profile.GetTimeSpans()[0].GetTargets()[0].GetRandomRatio(), (UINT32) 50);
        }        

        // Negative, bounds validation
        
        {
            Profile profile;
            fopen_s(&pFile, _sTempFilePath.c_str(), "wb");
            VERIFY_IS_TRUE(pFile != nullptr);
            fprintf(pFile, xmlDoc, "", "Ratio", "0", "Ratio");
            fclose(pFile);
            pFile = nullptr;

            VERIFY_IS_FALSE(p.ParseFile(_sTempFilePath.c_str(), &profile, nullptr, _hModule));
        }
        {
            Profile profile;
            fopen_s(&pFile, _sTempFilePath.c_str(), "wb");
            VERIFY_IS_TRUE(pFile != nullptr);
            fprintf(pFile, xmlDoc, "", "Ratio", "100", "Ratio");
            fclose(pFile);
            pFile = nullptr;

            VERIFY_IS_FALSE(p.ParseFile(_sTempFilePath.c_str(), &profile, nullptr, _hModule));
        }
        {
            Profile profile;
            fopen_s(&pFile, _sTempFilePath.c_str(), "wb");
            VERIFY_IS_TRUE(pFile != nullptr);
            fprintf(pFile, xmlDoc, "", "Ratio", "-100", "Ratio");
            fclose(pFile);
            pFile = nullptr;

            VERIFY_IS_FALSE(p.ParseFile(_sTempFilePath.c_str(), &profile, nullptr, _hModule));
        }
        {
            Profile profile;
            fopen_s(&pFile, _sTempFilePath.c_str(), "wb");
            VERIFY_IS_TRUE(pFile != nullptr);
            fprintf(pFile, xmlDoc, "", "Ratio", "200", "Ratio");
            fclose(pFile);
            pFile = nullptr;

            VERIFY_IS_FALSE(p.ParseFile(_sTempFilePath.c_str(), &profile, nullptr, _hModule));
        }
        {
            Profile profile;
            fopen_s(&pFile, _sTempFilePath.c_str(), "wb");
            VERIFY_IS_TRUE(pFile != nullptr);
            fprintf(pFile, xmlDoc, "", "Ratio", "junk", "Ratio");
            fclose(pFile);
            pFile = nullptr;

            VERIFY_IS_FALSE(p.ParseFile(_sTempFilePath.c_str(), &profile, nullptr, _hModule));
        }

        // Negative, sequential conflict with RandomRatio
        {
            Profile profile;
            fopen_s(&pFile, _sTempFilePath.c_str(), "wb");
            VERIFY_IS_TRUE(pFile != nullptr);
            fprintf(pFile, xmlDoc, "<StrideSize>8192</StrideSize>", "Ratio", "50", "Ratio");
            fclose(pFile);
            pFile = nullptr;

            VERIFY_IS_FALSE(p.ParseFile(_sTempFilePath.c_str(), &profile, nullptr, _hModule));
        }

        // Negative, sequential conflict with Random
        {
            Profile profile;
            fopen_s(&pFile, _sTempFilePath.c_str(), "wb");
            VERIFY_IS_TRUE(pFile != nullptr);
            fprintf(pFile, xmlDoc, "<StrideSize>8192</StrideSize>", "", "8192", "");
            fclose(pFile);
            pFile = nullptr;

            VERIFY_IS_FALSE(p.ParseFile(_sTempFilePath.c_str(), &profile, nullptr, _hModule));
        }
    }

    // Template document for running distribution tests - type tags bracketing a type-specific insert
    static const PCHAR xmlDistDoc = "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
        "<Profile>\n"
        "    <TimeSpans>\n"
        "        <TimeSpan>\n"
        "            <Targets>\n"
        "                <Target>\n"
        "                    <Path></Path>\n"
        "                    <Random>65536</Random>\n"
        "                    <Distribution>\n"
        "                        <%s>\n"            // type
        "                            %s"            // ranges
        "                        </%s>\n"           // type
        "                    </Distribution>"
        "                </Target>\n"
        "            </Targets>\n"
        "        </TimeSpan>\n"
        "    </TimeSpans>\n"
        "</Profile>\n";

    void XmlProfileParserUnitTests::ValidateDistribution(
        const PWCHAR desc,
        boolean expectedParse,
        boolean expectedValidate,
        DistributionType type,
        PCHAR xmlDoc,
        const RangePair *insert,
        const DistQuad *dist
        )
    {
        Profile profile;
        XmlProfileParser p;
        FILE *pFile = nullptr;
        string xmlInsert;

        // Construct XML range list
        for (UINT32 i = 0; i < insert->size; i++)
        {
            xmlInsert += "<Range IO=\"";
            xmlInsert += to_string(insert->range[i].io);
            xmlInsert += "\">";
            xmlInsert += to_string(insert->range[i].target);
            xmlInsert += "</Range>\n";
        }

        // Bracketing typename - xmlDoc is assumed to be of the form ...<Type>[insert here]</Type>...
        PCHAR typeName = nullptr;

        switch (type)
        {
            case DistributionType::Absolute:
            typeName = "Absolute";
            break;
            
            case DistributionType::Percent:
            typeName = "Percent";
            break;
            
            default:
            assert(false);
            break;
        }

        // Generate content into temporary file and parse/validate
        // Emit the description at parse validation for documentation in the output
        fopen_s(&pFile, _sTempFilePath.c_str(), "wb");
        VERIFY_IS_TRUE(pFile != nullptr);
        fprintf(pFile, xmlDoc, typeName, xmlInsert.c_str(), typeName);
        fclose(pFile);

        if (!expectedParse)
        {
            VERIFY_IS_FALSE(p.ParseFile(_sTempFilePath.c_str(), &profile, nullptr, _hModule), desc);
            return; // done
        }

        VERIFY_IS_TRUE(p.ParseFile(_sTempFilePath.c_str(), &profile, nullptr, _hModule), desc);

        if (!expectedValidate)
        {
            VERIFY_IS_FALSE(profile.Validate(false));
            return; // done
        }

        VERIFY_IS_TRUE(profile.Validate(false));
        auto t = profile.GetTimeSpans()[0].GetTargets()[0];
        auto dt = t.GetDistributionType();
        auto& v = t.GetDistributionRange();

        VERIFY_ARE_EQUAL(type, dt);
        VERIFY_ARE_EQUAL(dist->size, v.size());
        for (size_t i = 0; i < v.size(); ++i)
        {
            VERIFY_ARE_EQUAL(v[i]._src, dist->range[i].ioBase);
            VERIFY_ARE_EQUAL(v[i]._span, dist->range[i].ioSpan);
            VERIFY_ARE_EQUAL(v[i]._dst.first, dist->range[i].targetBase);
            VERIFY_ARE_EQUAL(v[i]._dst.second, dist->range[i].targetSpan);
        }
    }

    void XmlProfileParserUnitTests::Test_ParseDistributionAbsolute()
    {
        //
        // Positive cases - first matches the ResultParser UT
        //

        {
            // -rdabs10/1g:10/1g:0/100g
            // hole + tail
            const RangePair insert = {
                3,
                {{10,   1*GB},
                {10,    1*GB},
                {0,     100*GB}}
            };
            const DistQuad dist = {
                4,
                {{0,    10,     0,      1*GB},
                {10,    10,     1*GB,   1*GB},
                {20,    0,      2*GB,   100*GB},
                {20,    80,     102*GB, 0}}
            };

            ValidateDistribution(L"Case 1", true, true, DistributionType::Absolute, xmlDistDoc, &insert, &dist);
        }

        //
        // Negative cases
        //

        {
            // -rdabs10/10 (note lack of units/small)
            // same negative case in cmdlineparser
            const RangePair insert = {
                1,
                {{10,   10}}
            };

            ValidateDistribution(L"Case 2", true, false, DistributionType::Absolute, xmlDistDoc, &insert, nullptr);
        }
        {
            // zero target not at end
            const RangePair insert = {
                3,
                {{10,   1*GB},
                {10,    0},
                {80,    1*GB}}
            };

            ValidateDistribution(L"Case 3", true, false, DistributionType::Absolute, xmlDistDoc, &insert, nullptr);
        }
        {
            // -rdabs10/10G:80/10G:20:10G
            // IO% overflow
            const RangePair insert = {
                3,
                {{10,   10*GB},
                {80,   10*GB},
                {20,   10*GB}}
            };

            ValidateDistribution(L"Case 4", true, false, DistributionType::Absolute, xmlDistDoc, &insert, nullptr);
        }
    }

    void XmlProfileParserUnitTests::Test_ParseDistributionPercent()
    {
        //
        // Positive cases - first matches the ResultParser UT
        //

        {
            // -rdpct10/10:10/10:0/10
            // hole + tail
            const RangePair insert = {
                3,
                {{10,   10},
                {10,    10},
                {0,     10}}
            };
            const DistQuad dist = {
                4,
                {{0,    10,     0,      10},
                {10,    10,     10,     10},
                {20,    0,      20,     10},
                {20,    80,     30,     70}}
            };

            ValidateDistribution(L"Case 1", true, true, DistributionType::Percent, xmlDistDoc, &insert, &dist);
        }

        //
        // Negative cases
        //

        {
            // zero target not at end - fails parse (before validate)
            const RangePair insert = {
                3,
                {{10,   10},
                {10,    0},
                {80,    10}}
            };

            ValidateDistribution(L"Case 2", false, false, DistributionType::Percent, xmlDistDoc, &insert, nullptr);
        }
        {
            // zero target at end - fails parse (before validate)
            const RangePair insert = {
                2,
                {{10,   10},
                {90,    0}}
            };

            ValidateDistribution(L"Case 3", false, false, DistributionType::Percent, xmlDistDoc, &insert, nullptr);
        }
        {
            // IO% overflow / Target% OK
            const RangePair insert = {
                3,
                {{10,   10},
                {80,    10},
                {20,    80}}
            };

            ValidateDistribution(L"Case 4", true, false, DistributionType::Percent, xmlDistDoc, &insert, nullptr);
        }
        {
            // IO% 100% / Target% incomplete
            const RangePair insert = {
                3,
                {{10,   10},
                {80,    10},
                {10,    10}}
            };

            ValidateDistribution(L"Case 5", true, false, DistributionType::Percent, xmlDistDoc, &insert, nullptr);
        }
        {
            // IO% incomplete / Target% 100%
            const RangePair insert = {
                3,
                {{10,   10},
                {10,    80},
                {10,    10}}
            };

            ValidateDistribution(L"Case 6", true, false, DistributionType::Percent, xmlDistDoc, &insert, nullptr);
        }
    }

    void XmlProfileParserUnitTests::Test_ParseTemplateTargets()
    {
        const PCHAR xmlDocPre = \
                    "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
                    "<Profile>\n"
                    "  <TimeSpans>\n";

        const PCHAR xmlDocPost = \
                    "  </TimeSpans>\n"
                    "</Profile>\n";

        const PCHAR xmlTimeSpanPre = \
                    "<TimeSpan>\n"
                    "  <Targets>\n";

        const PCHAR xmlTimeSpanPost = \
                    "  </Targets>\n"
                    "</TimeSpan>\n";

        const PCHAR xmlTarget = \
                    "<Target>\n"
                    "  <Path>%s</Path>\n"
                    "</Target>\n";

        XmlProfileParser p;
        FILE *pFile = nullptr;

        // Non template baseline - null ptr pass for subst

        {
            Profile profile;
            fopen_s(&pFile, _sTempFilePath.c_str(), "wb");
            VERIFY_IS_TRUE(pFile != nullptr);
            fprintf(pFile, xmlDocPre);
            fprintf(pFile, xmlTimeSpanPre);
            fprintf(pFile, xmlTarget, "foo.bin");
            fprintf(pFile, xmlTimeSpanPost);
            fprintf(pFile, xmlDocPost);
            fclose(pFile);
            pFile = nullptr;

            VERIFY_IS_TRUE(p.ParseFile(_sTempFilePath.c_str(), &profile, nullptr, _hModule));
            VERIFY_IS_TRUE(profile.Validate(false));
            VERIFY_IS_TRUE(!strcmp(profile.GetTimeSpans()[0].GetTargets()[0].GetPath().c_str(), "foo.bin"));
        }

        // Non template baseline - empty pass for subst

        {
            Profile profile;
            fopen_s(&pFile, _sTempFilePath.c_str(), "wb");
            VERIFY_IS_TRUE(pFile != nullptr);
            fprintf(pFile, xmlDocPre);
            fprintf(pFile, xmlTimeSpanPre);
            fprintf(pFile, xmlTarget, "foo.bin");
            fprintf(pFile, xmlTimeSpanPost);
            fprintf(pFile, xmlDocPost);
            fclose(pFile);
            pFile = nullptr;

            vector<Target> vTargets;
 
            VERIFY_IS_TRUE(p.ParseFile(_sTempFilePath.c_str(), &profile, &vTargets, _hModule));
            VERIFY_IS_TRUE(profile.Validate(false));
            VERIFY_IS_TRUE(!strcmp(profile.GetTimeSpans()[0].GetTargets()[0].GetPath().c_str(), "foo.bin"));
        }

        // Template - 1 template, bad formats which will not parse (and will parse independent of subst)

        {

            char *cStrs[] = { "*", "*foo", "*1foo", "**1", "*-1" };

            for (auto s : cStrs)
            {
                Profile profile;
                fopen_s(&pFile, _sTempFilePath.c_str(), "wb");
                VERIFY_IS_TRUE(pFile != nullptr);
                fprintf(pFile, xmlDocPre);
                fprintf(pFile, xmlTimeSpanPre);
                fprintf(pFile, xmlTarget, s);
                fprintf(pFile, xmlTimeSpanPost);
                fprintf(pFile, xmlDocPost);
                fclose(pFile);
                pFile = nullptr;

                vector<Target> vTargets;
    
                VERIFY_IS_FALSE(p.ParseFile(_sTempFilePath.c_str(), &profile, &vTargets, _hModule));
            }
        }

        // Template - 1 template, unsubst, will fail execution validation since !profile only

        {
            Profile profile;
            fopen_s(&pFile, _sTempFilePath.c_str(), "wb");
            VERIFY_IS_TRUE(pFile != nullptr);
            fprintf(pFile, xmlDocPre);
            fprintf(pFile, xmlTimeSpanPre);
            fprintf(pFile, xmlTarget, "*1");
            fprintf(pFile, xmlTimeSpanPost);
            fprintf(pFile, xmlDocPost);
            fclose(pFile);
            pFile = nullptr;

            vector<Target> vTargets;
 
            VERIFY_IS_TRUE(p.ParseFile(_sTempFilePath.c_str(), &profile, &vTargets, _hModule));
            VERIFY_IS_FALSE(profile.Validate(false));
            VERIFY_IS_TRUE(!strcmp(profile.GetTimeSpans()[0].GetTargets()[0].GetPath().c_str(), "*1"));
        }

        // Template - 1 timespan 1 template, unsubst, will pass execution validation since profile only

        {
            Profile profile;
            fopen_s(&pFile, _sTempFilePath.c_str(), "wb");
            VERIFY_IS_TRUE(pFile != nullptr);
            fprintf(pFile, xmlDocPre);
            fprintf(pFile, xmlTimeSpanPre);
            fprintf(pFile, xmlTarget, "*1");
            fprintf(pFile, xmlTimeSpanPost);
            fprintf(pFile, xmlDocPost);
            fclose(pFile);
            pFile = nullptr;

            vector<Target> vTargets;
 
            VERIFY_IS_TRUE(p.ParseFile(_sTempFilePath.c_str(), &profile, &vTargets, _hModule));
            profile.SetProfileOnly(true);
            VERIFY_IS_TRUE(profile.Validate(false));
            VERIFY_IS_TRUE(!strcmp(profile.GetTimeSpans()[0].GetTargets()[0].GetPath().c_str(), "*1"));
        }

        // Template - 1 timespan 1 template 1 subst - will pass execution validation

        {
            Profile profile;
            fopen_s(&pFile, _sTempFilePath.c_str(), "wb");
            VERIFY_IS_TRUE(pFile != nullptr);
            fprintf(pFile, xmlDocPre);
            fprintf(pFile, xmlTimeSpanPre);
            fprintf(pFile, xmlTarget, "*1");
            fprintf(pFile, xmlTimeSpanPost);
            fprintf(pFile, xmlDocPost);
            fclose(pFile);
            pFile = nullptr;

            vector<Target> vTargets;
            vTargets.emplace_back();
            vTargets[0].SetPath("foo.bin");

            VERIFY_IS_TRUE(p.ParseFile(_sTempFilePath.c_str(), &profile, &vTargets, _hModule));
            VERIFY_IS_TRUE(profile.Validate(false));
            VERIFY_IS_TRUE(!strcmp(profile.GetTimeSpans()[0].GetTargets()[0].GetPath().c_str(), "foo.bin"));
        }

        // Template - 1 timespan 1 template 2 subst - will fail parse due to unused subst

        {
            Profile profile;
            fopen_s(&pFile, _sTempFilePath.c_str(), "wb");
            VERIFY_IS_TRUE(pFile != nullptr);
            fprintf(pFile, xmlDocPre);
            fprintf(pFile, xmlTimeSpanPre);
            fprintf(pFile, xmlTarget, "*1");
            fprintf(pFile, xmlTimeSpanPost);
            fprintf(pFile, xmlDocPost);
            fclose(pFile);
            pFile = nullptr;

            vector<Target> vTargets;
            vTargets.emplace_back();
            vTargets[0].SetPath("foo.bin");
            vTargets.emplace_back();
            vTargets[1].SetPath("bar.bin");

            VERIFY_IS_FALSE(p.ParseFile(_sTempFilePath.c_str(), &profile, &vTargets, _hModule));
        }        

        // 1 timespan 2 same template 1 subst

        {
            Profile profile;
            fopen_s(&pFile, _sTempFilePath.c_str(), "wb");
            VERIFY_IS_TRUE(pFile != nullptr);
            fprintf(pFile, xmlDocPre);
            fprintf(pFile, xmlTimeSpanPre);
            fprintf(pFile, xmlTarget, "*1");
            fprintf(pFile, xmlTarget, "*1");
            fprintf(pFile, xmlTimeSpanPost);
            fprintf(pFile, xmlDocPost);
            fclose(pFile);
            pFile = nullptr;

            vector<Target> vTargets;
            vTargets.emplace_back();
            vTargets[0].SetPath("foo.bin");

            VERIFY_IS_TRUE(p.ParseFile(_sTempFilePath.c_str(), &profile, &vTargets, _hModule));
            VERIFY_IS_TRUE(profile.Validate(false));
            VERIFY_IS_TRUE(!strcmp(profile.GetTimeSpans()[0].GetTargets()[0].GetPath().c_str(), "foo.bin"));
            VERIFY_IS_TRUE(!strcmp(profile.GetTimeSpans()[0].GetTargets()[1].GetPath().c_str(), "foo.bin"));
        }        

        // 1 timespan 2 template 2 subst

        {
            Profile profile;
            fopen_s(&pFile, _sTempFilePath.c_str(), "wb");
            VERIFY_IS_TRUE(pFile != nullptr);
            fprintf(pFile, xmlDocPre);
            fprintf(pFile, xmlTimeSpanPre);
            fprintf(pFile, xmlTarget, "*1");
            fprintf(pFile, xmlTarget, "*2");
            fprintf(pFile, xmlTimeSpanPost);
            fprintf(pFile, xmlDocPost);
            fclose(pFile);
            pFile = nullptr;

            vector<Target> vTargets;
            vTargets.emplace_back();
            vTargets[0].SetPath("foo.bin");
            vTargets.emplace_back();
            vTargets[1].SetPath("bar.bin");

            VERIFY_IS_TRUE(p.ParseFile(_sTempFilePath.c_str(), &profile, &vTargets, _hModule));
            VERIFY_IS_TRUE(profile.Validate(false));
            VERIFY_IS_TRUE(!strcmp(profile.GetTimeSpans()[0].GetTargets()[0].GetPath().c_str(), "foo.bin"));
            VERIFY_IS_TRUE(!strcmp(profile.GetTimeSpans()[0].GetTargets()[1].GetPath().c_str(), "bar.bin"));
        }        

        // 1 timespan 2 template 1 subst - will fail since second template does not have a subst

        {
            Profile profile;
            fopen_s(&pFile, _sTempFilePath.c_str(), "wb");
            VERIFY_IS_TRUE(pFile != nullptr);
            fprintf(pFile, xmlDocPre);
            fprintf(pFile, xmlTimeSpanPre);
            fprintf(pFile, xmlTarget, "*1");
            fprintf(pFile, xmlTarget, "*2");
            fprintf(pFile, xmlTimeSpanPost);
            fprintf(pFile, xmlDocPost);
            fclose(pFile);
            pFile = nullptr;

            vector<Target> vTargets;
            vTargets.emplace_back();
            vTargets[0].SetPath("foo.bin");

            VERIFY_IS_FALSE(p.ParseFile(_sTempFilePath.c_str(), &profile, &vTargets, _hModule));
        }        

        // 2 timespan same template 1 subst

        {
            Profile profile;
            fopen_s(&pFile, _sTempFilePath.c_str(), "wb");
            VERIFY_IS_TRUE(pFile != nullptr);
            fprintf(pFile, xmlDocPre);
            fprintf(pFile, xmlTimeSpanPre);
            fprintf(pFile, xmlTarget, "*1");
            fprintf(pFile, xmlTimeSpanPost);
            fprintf(pFile, xmlTimeSpanPre);
            fprintf(pFile, xmlTarget, "*1");
            fprintf(pFile, xmlTimeSpanPost);
            fprintf(pFile, xmlDocPost);
            fclose(pFile);
            pFile = nullptr;

            vector<Target> vTargets;
            vTargets.emplace_back();
            vTargets[0].SetPath("foo.bin");

            VERIFY_IS_TRUE(p.ParseFile(_sTempFilePath.c_str(), &profile, &vTargets, _hModule));
            VERIFY_IS_TRUE(profile.Validate(false));
            VERIFY_IS_TRUE(!strcmp(profile.GetTimeSpans()[0].GetTargets()[0].GetPath().c_str(), "foo.bin"));
            VERIFY_IS_TRUE(!strcmp(profile.GetTimeSpans()[1].GetTargets()[0].GetPath().c_str(), "foo.bin"));
        }        

        // 2 timespan 1 template/ea 2 subst

        {
            Profile profile;
            fopen_s(&pFile, _sTempFilePath.c_str(), "wb");
            VERIFY_IS_TRUE(pFile != nullptr);
            fprintf(pFile, xmlDocPre);
            fprintf(pFile, xmlTimeSpanPre);
            fprintf(pFile, xmlTarget, "*1");
            fprintf(pFile, xmlTimeSpanPost);
            fprintf(pFile, xmlTimeSpanPre);
            fprintf(pFile, xmlTarget, "*2");
            fprintf(pFile, xmlTimeSpanPost);
            fprintf(pFile, xmlDocPost);
            fclose(pFile);
            pFile = nullptr;

            vector<Target> vTargets;
            vTargets.emplace_back();
            vTargets[0].SetPath("foo.bin");
            vTargets.emplace_back();
            vTargets[1].SetPath("bar.bin");

            VERIFY_IS_TRUE(p.ParseFile(_sTempFilePath.c_str(), &profile, &vTargets, _hModule));
            VERIFY_IS_TRUE(profile.Validate(false));
            VERIFY_IS_TRUE(!strcmp(profile.GetTimeSpans()[0].GetTargets()[0].GetPath().c_str(), "foo.bin"));
            VERIFY_IS_TRUE(!strcmp(profile.GetTimeSpans()[1].GetTargets()[0].GetPath().c_str(), "bar.bin"));
        }        
    }
}