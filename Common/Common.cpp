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

#include "Common.h"

TRACELOGGING_DEFINE_PROVIDER(g_hEtwProvider,
                             "Microsoft-Windows-DiskSpd", // {CA13DB84-D0A9-5145-FCA4-468DA92FDC2D}
                             (0xca13db84, 0xd0a9, 0x5145, 0xfc, 0xa4, 0x46, 0x8d, 0xa9, 0x2f, 0xdc, 0x2d));

SystemInformation g_SystemInformation;

UINT64 PerfTimer::GetTime()
{
    LARGE_INTEGER li;
    QueryPerformanceCounter(&li);
    return li.QuadPart;
}

UINT64 PerfTimer::_GetPerfTimerFreq()
{
    LARGE_INTEGER li;
    QueryPerformanceFrequency(&li);
    return li.QuadPart;
}

const UINT64 PerfTimer::TIMER_FREQ = _GetPerfTimerFreq();

double PerfTimer::PerfTimeToMicroseconds(const double perfTime)
{
    return perfTime / (TIMER_FREQ / 1000000.0);
}

double PerfTimer::PerfTimeToMilliseconds(const double perfTime)
{
    return PerfTimeToMicroseconds(perfTime) / 1000;
}

double PerfTimer::PerfTimeToSeconds(const double perfTime)
{
    return PerfTimeToMilliseconds(perfTime) / 1000;
}

double PerfTimer::PerfTimeToMicroseconds(const UINT64 perfTime)
{
    return PerfTimeToMicroseconds(static_cast<double>(perfTime));
}

double PerfTimer::PerfTimeToMilliseconds(const UINT64 perfTime)
{
    return PerfTimeToMilliseconds(static_cast<double>(perfTime));
}

double PerfTimer::PerfTimeToSeconds(const UINT64 perfTime)
{
    return PerfTimeToSeconds(static_cast<double>(perfTime));
}

UINT64 PerfTimer::MicrosecondsToPerfTime(const double microseconds)
{
    return static_cast<UINT64>(TIMER_FREQ * (microseconds / 1000000.0));
}

UINT64 PerfTimer::MillisecondsToPerfTime(const double milliseconds)
{
    return static_cast<UINT64>(TIMER_FREQ * (milliseconds / 1000.0));
}

UINT64 PerfTimer::SecondsToPerfTime(const double seconds)
{
    return static_cast<UINT64>(TIMER_FREQ * seconds);
}

Random::Random(UINT64 ulSeed)
{
    UINT32 i;
    
    _ulState[0] = 0xf1ea5eed;
    _ulState[1] = ulSeed;
    _ulState[2] = ulSeed;
    _ulState[3] = ulSeed;
    
    for (i = 0; i < 20; i++) {
        Rand64();
    }
}

void Random::RandBuffer(BYTE *pBuffer, UINT32 ulLength, bool fPseudoRandomOkay)
{
    UINT64 *pBuffer64;
    UINT32 Remaining = (UINT32)(((ULONG_PTR)pBuffer) & 7);
    UINT64 r1, r2, r3, r4, r5;

    //
    // Align to 8 bytes
    //

    if (Remaining != 0) {
        r1 = Rand64();

        while (Remaining != 0 && ulLength != 0) {
            *pBuffer = (BYTE)(r1 & 0xFF);
            r1 >>= 8;
            pBuffer++;
            ulLength--;
            Remaining--;
        }
    }

    pBuffer64 = (UINT64*)pBuffer;
    Remaining = ulLength / 8;
    ulLength -= Remaining * 8;
    pBuffer += Remaining * 8;

    if (fPseudoRandomOkay) {
        
        //
        // Generate 5 random numbers and then mix them to produce
        // 16 random (but correlated) numbers.  We want to do 16 
        // numbers at a time for optimal cache line alignment.
        // Only do this if the caller is okay with numbers that
        // aren't independent.  A detailed analysis of the data
        // could probably detect that the first 5 numbers determine
        // the next 11.  For most purposes this won't matter (for
        // instance it's unlikely compression algorithms will be
        // able to detect this and utilize it).
        //
        
        while (Remaining > 16) {
            r1 = Rand64();
            r2 = Rand64();
            r3 = Rand64();
            r4 = Rand64();
            r5 = Rand64();

            pBuffer64[0]  = r1;
            pBuffer64[1]  = r2;
            pBuffer64[2]  = r3;
            pBuffer64[3]  = r4;
            pBuffer64[4]  = r5;

            //
            // Throw in some rotates so that the below numbers
            // aren't the xor sum of previous numbers.
            //

            r1 = _rotl64(r1, 7);
            pBuffer64[5]  = r1 ^ r2;
            pBuffer64[6]  = r1 ^ r3;
            pBuffer64[7]  = r1 ^ r4;
            pBuffer64[8]  = r1 ^ r5;

            r2 = _rotl64(r2, 13);
            pBuffer64[9]  = r2 ^ r3;
            pBuffer64[10] = r2 ^ r4;
            pBuffer64[11] = r2 ^ r5;

            r3 = _rotl64(r3, 19);
            pBuffer64[12] = r3 ^ r4;
            pBuffer64[13] = r3 ^ r5;

            pBuffer64[14] = r1 ^ r2 ^ r3;
            pBuffer64[15] = r1 ^ _rotl64(r4 ^ r5, 39);

            pBuffer64 += 16;
            Remaining -= 16;
        }
    }

    //
    // Fill in the tail of the buffer
    //
    
    while (Remaining >= 4) {
        r1 = Rand64();
        r2 = Rand64();
        r3 = Rand64();
        r4 = Rand64();

        pBuffer64[0]  = r1;
        pBuffer64[1]  = r2;
        pBuffer64[2]  = r3;
        pBuffer64[3]  = r4;

        pBuffer64 += 4;
        Remaining -= 4;
    }

    while (Remaining != 0) {
        *pBuffer64 = Rand64();
        pBuffer64++;
        Remaining--;
    }

    if (ulLength != 0) {
        r1 = Rand64();

        while (ulLength != 0) {
            *pBuffer = (BYTE)(r1 & 0xFF);
            r1 >>= 8;
            pBuffer++;
            ulLength--;
        }
    }
}

string Util::DoubleToStringHelper(const double d)
{
    char szFloatBuffer[100];
    sprintf_s(szFloatBuffer, _countof(szFloatBuffer), "%10.3lf", d);

    return string(szFloatBuffer);
}

string ThreadTarget::GetXml(UINT32 indent) const
{
    char buffer[4096];
    string sXml;
    
    AddXmlInc(sXml, "<ThreadTarget>\n");

    sprintf_s(buffer, _countof(buffer), "<Thread>%u</Thread>\n", _ulThread);
    AddXml(sXml, buffer);

    if (_ulWeight != 0)
    {
        sprintf_s(buffer, _countof(buffer), "<Weight>%u</Weight>\n", _ulWeight);
        AddXml(sXml, buffer);
    }

    AddXmlDec(sXml, "</ThreadTarget>\n");

    return sXml;
}

string Target::GetXml(UINT32 indent) const
{
    char buffer[4096];
    string sXml;
    
    AddXmlInc(sXml, "<Target>\n");
    AddXml(sXml, "<Path>" + _sPath + "</Path>\n");

    sprintf_s(buffer, _countof(buffer), "<BlockSize>%u</BlockSize>\n", _dwBlockSize);
    AddXml(sXml, buffer);

    sprintf_s(buffer, _countof(buffer), "<BaseFileOffset>%I64u</BaseFileOffset>\n", _ullBaseFileOffset);
    AddXml(sXml, buffer);

    AddXml(sXml, _fSequentialScanHint ? "<SequentialScan>true</SequentialScan>\n" : "<SequentialScan>false</SequentialScan>\n");
    AddXml(sXml, _fRandomAccessHint ? "<RandomAccess>true</RandomAccess>\n" : "<RandomAccess>false</RandomAccess>\n");
    AddXml(sXml, _fTemporaryFileHint ? "<TemporaryFile>true</TemporaryFile>\n" : "<TemporaryFile>false</TemporaryFile>\n");
    AddXml(sXml, _fUseLargePages ? "<UseLargePages>true</UseLargePages>\n" : "<UseLargePages>false</UseLargePages>\n");

    // TargetCacheMode::Cached is implied default
    switch (_cacheMode)
    {
    case TargetCacheMode::DisableLocalCache:
        AddXml(sXml, "<DisableLocalCache>true</DisableLocalCache>\n");
        break;
    case TargetCacheMode::DisableOSCache:
        AddXml(sXml, "<DisableOSCache>true</DisableOSCache>\n");
        break;
    }

    // WriteThroughMode::Off is implied default
    switch (_writeThroughMode)
    {
    case WriteThroughMode::On:
        AddXml(sXml, "<WriteThrough>true</WriteThrough>\n");
        break;
    }
    
    // MemoryMappedIoMode::Off is implied default
    switch (_memoryMappedIoMode)
    {
    case MemoryMappedIoMode::On:
        AddXml(sXml, "<MemoryMappedIo>true</MemoryMappedIo>\n");
        break;
    }

    // MemoryMappedIoFlushMode::Undefined is implied default
    switch (_memoryMappedIoFlushMode)
    {
    case MemoryMappedIoFlushMode::ViewOfFile:
        AddXml(sXml, "<FlushType>ViewOfFile</FlushType>\n");
        break;
    case MemoryMappedIoFlushMode::NonVolatileMemory:
        AddXml(sXml, "<FlushType>NonVolatileMemory</FlushType>\n")
        break;
    case MemoryMappedIoFlushMode::NonVolatileMemoryNoDrain:
        AddXml(sXml, "<FlushType>NonVolatileMemoryNoDrain</FlushType>\n");
        break;
    }

    AddXmlInc(sXml, "<WriteBufferContent>\n");
    if (_fZeroWriteBuffers)
    {
        AddXml(sXml, "<Pattern>zero</Pattern>\n");
    }
    else if (_cbRandomDataWriteBuffer == 0)
    {
        AddXml(sXml, "<Pattern>sequential</Pattern>\n");
    }
    else
    {
        AddXml(sXml, "<Pattern>random</Pattern>\n");
        AddXmlInc(sXml, "<RandomDataSource>\n");
        sprintf_s(buffer, _countof(buffer), "<SizeInBytes>%I64u</SizeInBytes>\n", _cbRandomDataWriteBuffer);
        AddXml(sXml, buffer);
        if (_sRandomDataWriteBufferSourcePath != "")
        {
            AddXml(sXml, "<FilePath>" + _sRandomDataWriteBufferSourcePath + "</FilePath>\n");
        }
        AddXmlDec(sXml, "</RandomDataSource>\n");
    }
    AddXmlDec(sXml, "</WriteBufferContent>\n");

    AddXml(sXml, _fParallelAsyncIO ? "<ParallelAsyncIO>true</ParallelAsyncIO>\n" : "<ParallelAsyncIO>false</ParallelAsyncIO>\n");

    if (_fUseBurstSize)
    {
        sprintf_s(buffer, _countof(buffer), "<BurstSize>%u</BurstSize>\n", _dwBurstSize);
        AddXml(sXml, buffer);
    }

    if (_fThinkTime)
    {
        sprintf_s(buffer, _countof(buffer), "<ThinkTime>%u</ThinkTime>\n", _dwThinkTime);
        AddXml(sXml, buffer);
    }

    if (_fCreateFile)
    {
        sprintf_s(buffer, _countof(buffer), "<FileSize>%I64u</FileSize>\n", _ullFileSize);
        AddXml(sXml, buffer);
    }

    // If XML contains <Random>, <StrideSize> is ignored
    if (_ulRandomRatio > 0)
    {
        sprintf_s(buffer, _countof(buffer), "<Random>%I64u</Random>\n", GetBlockAlignmentInBytes());
        AddXml(sXml, buffer);

        // 100% random is <Random> alone
        if (_ulRandomRatio != 100)
        {
            sprintf_s(buffer, _countof(buffer), "<RandomRatio>%u</RandomRatio>\n", GetRandomRatio());
            AddXml(sXml, buffer);
        }

        // Distributions only occur in profiles with random IO.

        if (_vDistributionRange.size())
        {
            char *type = nullptr;

            switch (_distributionType)
            {
                case DistributionType::Absolute:
                type = "Absolute";
                break;

                case DistributionType::Percent:
                type = "Percent";
                break;

                default:
                assert(false);
            }

            AddXmlInc(sXml, "<Distribution>\n");
            AddXmlInc(sXml, "<");
            sXml += type;
            sXml += ">\n";

            for (auto r : _vDistributionRange)
            {
                sprintf_s(buffer, _countof(buffer), "<Range IO=\"%u\">%I64u", r._span, r._dst.second);
                AddXml(sXml, buffer);
                sXml += "</Range>\n";
            }

            AddXmlDec(sXml, "</");
            sXml += type;
            sXml += ">\n";
            AddXmlDec(sXml, "</Distribution>\n");
        }
    }
    else
    {
        sprintf_s(buffer, _countof(buffer), "<StrideSize>%I64u</StrideSize>\n", GetBlockAlignmentInBytes());
        AddXml(sXml, buffer);
    
        AddXml(sXml, _fInterlockedSequential ?
            "<InterlockedSequential>true</InterlockedSequential>\n" :
            "<InterlockedSequential>false</InterlockedSequential>\n");
    }

    sprintf_s(buffer, _countof(buffer), "<ThreadStride>%I64u</ThreadStride>\n", _ullThreadStride);
    AddXml(sXml, buffer);

    sprintf_s(buffer, _countof(buffer), "<MaxFileSize>%I64u</MaxFileSize>\n", _ullMaxFileSize);
    AddXml(sXml, buffer);

    sprintf_s(buffer, _countof(buffer), "<RequestCount>%u</RequestCount>\n", _dwRequestCount);
    AddXml(sXml, buffer);

    sprintf_s(buffer, _countof(buffer), "<WriteRatio>%u</WriteRatio>\n", _ulWriteRatio);
    AddXml(sXml, buffer);

    // Preserve specified units
    if (_dwThroughputIOPS)
    {
        sprintf_s(buffer, _countof(buffer), "<Throughput unit=\"IOPS\">%u</Throughput>\n", _dwThroughputIOPS);
        AddXml(sXml, buffer);
    }
    else
    {
        sprintf_s(buffer, _countof(buffer), "<Throughput>%u</Throughput>\n", _dwThroughputBytesPerMillisecond);
        AddXml(sXml, buffer);
    }

    sprintf_s(buffer, _countof(buffer), "<ThreadsPerFile>%u</ThreadsPerFile>\n", _dwThreadsPerFile);
    AddXml(sXml, buffer);

    if (_ioPriorityHint == IoPriorityHintVeryLow)
    {
        AddXml(sXml, "<IOPriority>1</IOPriority>\n");
    }
    else if (_ioPriorityHint == IoPriorityHintLow)
    {
        AddXml(sXml, "<IOPriority>2</IOPriority>\n");
    }
    else if (_ioPriorityHint == IoPriorityHintNormal)
    {
        AddXml(sXml, "<IOPriority>3</IOPriority>\n");
    }
    else
    {
        AddXml(sXml, "<IOPriority>* UNSUPPORTED *</IOPriority>\n");
    }

    sprintf_s(buffer, _countof(buffer), "<Weight>%u</Weight>\n", _ulWeight);
    AddXml(sXml, buffer);

    if (_vThreadTargets.size() > 0)
    {
        AddXmlInc(sXml, "<ThreadTargets>\n");

        for (const auto& threadTarget : _vThreadTargets)
        {
            sXml += threadTarget.GetXml(indent);
        }

        AddXmlDec(sXml, "</ThreadTargets>\n");
    }

    AddXmlDec(sXml, "</Target>\n");

    return sXml;
}

bool Target::_FillRandomDataWriteBuffer(Random *pRand)
{
    assert(_pRandomDataWriteBuffer != nullptr);
    bool fOk = true;
    size_t cb = static_cast<size_t>(GetRandomDataWriteBufferSize());
    if (GetRandomDataWriteBufferSourcePath() == "")
    {
        pRand->RandBuffer(_pRandomDataWriteBuffer, (UINT32)cb, false);
    }
    else
    {
        // fill buffer from file
        HANDLE hFile = CreateFile(GetRandomDataWriteBufferSourcePath().c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, 0, nullptr);
        if (hFile != INVALID_HANDLE_VALUE)
        {
            UINT64 cbLeftToRead = GetRandomDataWriteBufferSize();
            BYTE *pBuffer = _pRandomDataWriteBuffer;
            bool fReadSuccess = true;
            while (fReadSuccess && cbLeftToRead > 0)
            {
                DWORD cbToRead = static_cast<DWORD>(min(64 * 1024, cbLeftToRead));
                DWORD cbRead;
                fReadSuccess = ((ReadFile(hFile, pBuffer, cbToRead, &cbRead, nullptr) == TRUE) && (cbRead > 0));
                pBuffer += cbRead;
            }

            // if the file is smaller than the buffer, repeat its content
            BYTE *pSource = _pRandomDataWriteBuffer;
            const BYTE *pPastEnd = pSource + GetRandomDataWriteBufferSize();
            while (pBuffer < pPastEnd)
            {
                *pBuffer++ = *pSource++;
            }
            CloseHandle(hFile);
        }
        else
        {
            printf("\n\nERROR: Unable to open entropy file '%s'\n\n", GetRandomDataWriteBufferSourcePath().c_str());
            fOk = false;
        }
    }
    return fOk;
}

bool Target::AllocateAndFillRandomDataWriteBuffer(Random *pRand)
{
    assert(_pRandomDataWriteBuffer == nullptr);
    bool fOk = false;
    size_t cb = static_cast<size_t>(GetRandomDataWriteBufferSize());
    if (cb < 1)
    {
        return fOk;
    }

    // TODO: make sure the size if <= max value for size_t
    if (GetUseLargePages())
    {
        size_t cbMinLargePage = GetLargePageMinimum();
        size_t cbRoundedSize = (cb + cbMinLargePage - 1) & ~(cbMinLargePage - 1);
        _pRandomDataWriteBuffer = (BYTE *)VirtualAlloc(nullptr, cbRoundedSize, MEM_COMMIT | MEM_RESERVE | MEM_LARGE_PAGES, PAGE_EXECUTE_READWRITE);
    }
    else
    {
        _pRandomDataWriteBuffer = (BYTE *)VirtualAlloc(nullptr, cb, MEM_COMMIT, PAGE_READWRITE);
    }

    fOk = (_pRandomDataWriteBuffer != nullptr);
    if (fOk)
    {
        fOk = _FillRandomDataWriteBuffer(pRand);
    }
    return fOk;
}

void Target::FreeRandomDataWriteBuffer()
{
    if (nullptr != _pRandomDataWriteBuffer)
    {
        VirtualFree(_pRandomDataWriteBuffer, 0, MEM_RELEASE);
        _pRandomDataWriteBuffer = nullptr;
    }
}

BYTE* Target::GetRandomDataWriteBuffer(Random *pRand)
{
    size_t cbBuffer = static_cast<size_t>(GetRandomDataWriteBufferSize());
    size_t cbBlock = GetBlockSizeInBytes();

    // leave enough bytes in the buffer for one block
    size_t randomOffset = pRand->Rand32() % (cbBuffer - (cbBlock - 1));

    bool fUnbufferedIO = (_cacheMode == TargetCacheMode::DisableOSCache);
    if (fUnbufferedIO)
    {
        // for unbuffered IO, offset in the buffer needs to be 512-byte aligned
        const size_t cbAlignment = 512;
        randomOffset -= (randomOffset % cbAlignment);
    }

    BYTE *pBuffer = reinterpret_cast<BYTE*>(reinterpret_cast<ULONG_PTR>(_pRandomDataWriteBuffer)+randomOffset);

    // unbuffered IO needs aligned addresses
    assert(!fUnbufferedIO || (reinterpret_cast<ULONG_PTR>(pBuffer) % 512 == 0));
    assert(pBuffer >= _pRandomDataWriteBuffer);
    assert(pBuffer <= _pRandomDataWriteBuffer + GetRandomDataWriteBufferSize() - GetBlockSizeInBytes());

    return pBuffer;
}

string TimeSpan::GetXml(UINT32 indent) const
{
    string sXml;
    char buffer[4096];

    AddXmlInc(sXml, "<TimeSpan>\n");
    AddXml(sXml, _fCompletionRoutines ? "<CompletionRoutines>true</CompletionRoutines>\n" : "<CompletionRoutines>false</CompletionRoutines>\n");
    AddXml(sXml,_fMeasureLatency ? "<MeasureLatency>true</MeasureLatency>\n" : "<MeasureLatency>false</MeasureLatency>\n");
    AddXml(sXml, _fCalculateIopsStdDev ? "<CalculateIopsStdDev>true</CalculateIopsStdDev>\n" : "<CalculateIopsStdDev>false</CalculateIopsStdDev>\n");
    AddXml(sXml, _fDisableAffinity ? "<DisableAffinity>true</DisableAffinity>\n" : "<DisableAffinity>false</DisableAffinity>\n");

    sprintf_s(buffer, _countof(buffer), "<Duration>%u</Duration>\n", _ulDuration);
    AddXml(sXml, buffer);

    sprintf_s(buffer, _countof(buffer), "<Warmup>%u</Warmup>\n", _ulWarmUp);
    AddXml(sXml, buffer);

    sprintf_s(buffer, _countof(buffer), "<Cooldown>%u</Cooldown>\n", _ulCoolDown);
    AddXml(sXml, buffer);

    sprintf_s(buffer, _countof(buffer), "<ThreadCount>%u</ThreadCount>\n", _dwThreadCount);
    AddXml(sXml, buffer);

    sprintf_s(buffer, _countof(buffer), "<RequestCount>%u</RequestCount>\n", _dwRequestCount);
    AddXml(sXml, buffer);

    sprintf_s(buffer, _countof(buffer), "<IoBucketDuration>%u</IoBucketDuration>\n", _ulIoBucketDurationInMilliseconds);
    AddXml(sXml, buffer);

    sprintf_s(buffer, _countof(buffer), "<RandSeed>%u</RandSeed>\n", _ulRandSeed);
    AddXml(sXml, buffer);

    if (_vAffinity.size() > 0)
    {
        AddXmlInc(sXml, "<Affinity>\n");
        for (const auto& a : _vAffinity)
        {
            sprintf_s(buffer, _countof(buffer), "<AffinityGroupAssignment Group=\"%u\" Processor=\"%u\"/>\n", a.wGroup, a.bProc);
            AddXml(sXml, buffer);
        }
        AddXmlDec(sXml, "</Affinity>\n");
    }

    AddXmlInc(sXml, "<Targets>\n");
    for (const auto& target : _vTargets)
    {
        sXml += target.GetXml(indent);
    }
    AddXmlDec(sXml, "</Targets>\n");
    AddXmlDec(sXml, "</TimeSpan>\n");
    return sXml;
}

void TimeSpan::MarkFilesAsPrecreated(const vector<string> vFiles)
{
    for (auto sFile : vFiles)
    {
        for (auto pTarget = _vTargets.begin(); pTarget != _vTargets.end(); pTarget++)
        {
            if (sFile == pTarget->GetPath())
            {
                pTarget->SetPrecreated(true);
            }
        }
    }
}

string Profile::GetXml(UINT32 indent) const
{
    string sXml;
    char buffer[4096];

    AddXmlInc(sXml, "<Profile>\n");

    sprintf_s(buffer, _countof(buffer), "<Progress>%u</Progress>\n", _dwProgress);
    AddXml(sXml, buffer);

    if (_resultsFormat == ResultsFormat::Text)
    {
        AddXml(sXml, "<ResultFormat>text</ResultFormat>\n");
    }
    else if (_resultsFormat == ResultsFormat::Xml)
    {
        AddXml(sXml, "<ResultFormat>xml</ResultFormat>\n");
    }
    else
    {
        AddXml(sXml, "<ResultFormat>* UNSUPPORTED *</ResultFormat>\n");
    }

    AddXml(sXml, _fVerbose ? "<Verbose>true</Verbose>\n" : "<Verbose>false</Verbose>\n");
    if (_precreateFiles == PrecreateFiles::UseMaxSize)
    {
        AddXml(sXml, "<PrecreateFiles>UseMaxSize</PrecreateFiles>\n");
    }
    else if (_precreateFiles == PrecreateFiles::OnlyFilesWithConstantSizes)
    {
        AddXml(sXml, "<PrecreateFiles>CreateOnlyFilesWithConstantSizes</PrecreateFiles>\n");
    }
    else if (_precreateFiles == PrecreateFiles::OnlyFilesWithConstantOrZeroSizes)
    {
        AddXml(sXml, "<PrecreateFiles>CreateOnlyFilesWithConstantOrZeroSizes</PrecreateFiles>\n");
    }

    if (_fEtwEnabled)
    {
        AddXmlInc(sXml, "<ETW>\n");
        AddXml(sXml, _fEtwProcess ? "<Process>true</Process>\n" : "<Process>false</Process>\n");
        AddXml(sXml, _fEtwThread ? "<Thread>true</Thread>\n" : "<Thread>false</Thread>\n");
        AddXml(sXml, _fEtwImageLoad ? "<ImageLoad>true</ImageLoad>\n" : "<ImageLoad>false</ImageLoad>\n");
        AddXml(sXml, _fEtwDiskIO ? "<DiskIO>true</DiskIO>\n" : "<DiskIO>false</DiskIO>\n");
        AddXml(sXml, _fEtwMemoryPageFaults ? "<MemoryPageFaults>true</MemoryPageFaults>\n" : "<MemoryPageFaults>false</MemoryPageFaults>\n");
        AddXml(sXml, _fEtwMemoryHardFaults ? "<MemoryHardFaults>true</MemoryHardFaults>\n" : "<MemoryHardFaults>false</MemoryHardFaults>\n");
        AddXml(sXml, _fEtwNetwork ? "<Network>true</Network>\n" : "<Network>false</Network>\n");
        AddXml(sXml, _fEtwRegistry ? "<Registry>true</Registry>\n" : "<Registry>false</Registry>\n");
        AddXml(sXml, _fEtwUsePagedMemory ? "<UsePagedMemory>true</UsePagedMemory>\n" : "<UsePagedMemory>false</UsePagedMemory>\n");
        AddXml(sXml, _fEtwUsePerfTimer ? "<UsePerfTimer>true</UsePerfTimer>\n" : "<UsePerfTimer>false</UsePerfTimer>\n");
        AddXml(sXml, _fEtwUseSystemTimer ? "<UseSystemTimer>true</UseSystemTimer>\n" : "<UseSystemTimer>false</UseSystemTimer>\n");
        AddXml(sXml, _fEtwUseCyclesCounter ? "<UseCyclesCounter>true</UseCyclesCounter>\n" : "<UseCyclesCounter>false</UseCyclesCounter>\n");
        AddXmlDec(sXml, "</ETW>\n");
    }

    AddXmlInc(sXml, "<TimeSpans>\n");
    for (const auto& timespan : _vTimeSpans)
    {
        sXml += timespan.GetXml(indent);
    }
    AddXmlDec(sXml, "</TimeSpans>\n");
    AddXmlDec(sXml, "</Profile>\n");
    return sXml;
}

void Profile::MarkFilesAsPrecreated(const vector<string> vFiles)
{
    for (auto pTimeSpan = _vTimeSpans.begin(); pTimeSpan != _vTimeSpans.end(); pTimeSpan++)
    {
        pTimeSpan->MarkFilesAsPrecreated(vFiles);
    }
}

bool Profile::Validate(bool fSingleSpec, SystemInformation *pSystem) const
{
    bool fOk = true;

    if (GetTimeSpans().size() == 0)
    {
        fprintf(stderr, "ERROR: no timespans specified\n");
        fOk = false;
    }
    else
    {
        for (const auto& timeSpan : GetTimeSpans())
        {
            if (pSystem != nullptr)
            {
                for (const auto& Affinity : timeSpan.GetAffinityAssignments())
                {
                    if (Affinity.wGroup >= pSystem->processorTopology._vProcessorGroupInformation.size())
                    {
                        fprintf(stderr, "ERROR: affinity assignment to group %u; system only has %u groups\n",
                            Affinity.wGroup,
                            (int) pSystem->processorTopology._vProcessorGroupInformation.size());

                        fOk = false;

                    }
                    if (fOk && !pSystem->processorTopology._vProcessorGroupInformation[Affinity.wGroup].IsProcessorValid(Affinity.bProc))
                    {
                        fprintf(stderr, "ERROR: affinity assignment to group %u core %u not possible; group only has %u cores\n",
                            Affinity.wGroup,
                            Affinity.bProc,
                            pSystem->processorTopology._vProcessorGroupInformation[Affinity.wGroup]._maximumProcessorCount);

                        fOk = false;
                    }

                    if (fOk && !pSystem->processorTopology._vProcessorGroupInformation[Affinity.wGroup].IsProcessorActive(Affinity.bProc))
                    {
                        fprintf(stderr, "ERROR: affinity assignment to group %u core %u not possible; core is not active (current mask 0x%Ix)\n",
                            Affinity.wGroup,
                            Affinity.bProc,
                            pSystem->processorTopology._vProcessorGroupInformation[Affinity.wGroup]._activeProcessorMask);

                        fOk = false;
                    }
                }
            }

            if (timeSpan.GetDisableAffinity() && timeSpan.GetAffinityAssignments().size() > 0)
            {
                fprintf(stderr, "ERROR: -n and -a parameters cannot be used together\n");
                fOk = false;
            }

            for (const auto& target : timeSpan.GetTargets())
            {
                const bool targetHasMultipleThreads = (timeSpan.GetThreadCount() > 1) || (target.GetThreadsPerFile() > 1);

                if (timeSpan.GetThreadCount() > 0 && target.GetThreadsPerFile() > 1)
                {
                    fprintf(stderr, "ERROR: -F and -t parameters cannot be used together\n");
                    fOk = false;
                }

                if (target.GetThroughputInBytesPerMillisecond() > 0 && timeSpan.GetCompletionRoutines())
                {
                    fprintf(stderr, "ERROR: -g throughput control cannot be used with -x completion routines\n");
                    fOk = false;
                }

                //  If burst size is specified think time must be specified and If think time is specified burst size should be non zero
                if ((target.GetThinkTime() == 0 && target.GetBurstSize() > 0) || (target.GetThinkTime() > 0 && target.GetBurstSize() == 0))
                {
                    fprintf(stderr, "ERROR: need to specify -j<think time> with -i<burst size>\n");
                    fOk = false;
                }

                if (timeSpan.GetThreadCount() > 0 && timeSpan.GetRequestCount() > 0)
                {
                    if (target.GetThroughputInBytesPerMillisecond() > 0)
                    {
                        fprintf(stderr, "ERROR: -g throughput control cannot be used with -O outstanding requests per thread\n");
                        fOk = false;
                    }

                    if (target.GetThinkTime() > 0)
                    {
                        fprintf(stderr, "ERROR: -j think time cannot be used with -O outstanding requests per thread\n");
                        fOk = false;
                    }

                    if (target.GetUseParallelAsyncIO())
                    {
                        fprintf(stderr, "ERROR: -p parallel IO cannot be used with -O outstanding requests per thread\n");
                        fOk = false;
                    }

                    if (target.GetWeight() == 0)
                    {
                        fprintf(stderr, "ERROR: a non-zero target Weight must be specified\n");
                        fOk = false;
                    }

                    for (const auto& threadTarget : target.GetThreadTargets())
                    {
                        if (threadTarget.GetThread() >= timeSpan.GetThreadCount())
                        {
                            fprintf(stderr, "ERROR: illegal thread specified for ThreadTarget\n");
                            fOk = false;
                        }
                    }
                }
                else if (target.GetThreadTargets().size() != 0)
                {
                    fprintf(stderr, "ERROR: ThreadTargets can only be specified when the timespan ThreadCount and RequestCount are specified\n");
                    fOk = false;
                }

                if (target.GetRandomRatio())
                {
                    if (target.GetThreadStrideInBytes() > 0)
                    {
                        fprintf(stderr, "ERROR: -T conflicts with -r\n");
                        fOk = false;
                        // although ullThreadStride==0 is a valid value, it's interpreted as "not provided" for this warning
                    }

                    if (target.GetUseInterlockedSequential())
                    {
                        fprintf(stderr, "ERROR: -si conflicts with -r\n");
                        fOk = false;
                    }

                    if (target.GetUseParallelAsyncIO())
                    {
                        fprintf(stderr, "ERROR: -p conflicts with -r\n");
                        fOk = false;
                    }
                }
                else
                {
                    if (target.GetDistributionRange().size() != 0)
                    {
                        fprintf(stderr, "ERROR: random distribution ranges (-rd) do not apply to sequential-only IO patterns\n");
                        fOk = false;
                    }
                    
                    if (target.GetUseParallelAsyncIO() && target.GetRequestCount() == 1)
                    {
                        fprintf(stderr, "WARNING: -p does not have effect unless outstanding I/O count (-o) is > 1\n");
                    }

                    if (target.GetUseInterlockedSequential())
                    {
                        if (target.GetThreadStrideInBytes() > 0)
                        {
                            fprintf(stderr, "ERROR: -si conflicts with -T\n");
                            fOk = false;
                        }

                        if (target.GetUseParallelAsyncIO())
                        {
                            fprintf(stderr, "ERROR: -si conflicts with -p\n");
                            fOk = false;
                        }

                        if (!targetHasMultipleThreads)
                        {
                            fprintf(stderr, "WARNING: single-threaded test, -si ignored\n");
                        }
                    }
                    else
                    {
                        if (targetHasMultipleThreads && !target.GetThreadStrideInBytes())
                        {
                            fprintf(stderr, "WARNING: target access pattern will not be sequential, consider -si\n");
                        }

                        if (!targetHasMultipleThreads && target.GetThreadStrideInBytes())
                        {
                            fprintf(stderr, "ERROR: -T has no effect unless multiple threads per target are used\n");
                            fOk = false;
                        }
                    }
                }

                // Distribution ranges are only applied to random loads. Note validation failure in the sequential case.
                // TBD this should be moved to a proper Distribution class.
                {
                    UINT32 ioAcc = 0;
                    UINT64 targetAcc = 0;
                    bool absZero = false, absZeroLast = false;
                    for  (const auto& r : target.GetDistributionRange())
                    {
                        if (target.GetDistributionType() == DistributionType::Absolute)
                        {
                            // allow zero target span in last position
                            absZeroLast = false;
                            if (r._dst.second == 0 && !absZero)
                            {
                                // legal in last position
                                absZero = absZeroLast = true;
                            }
                            else if (r._dst.second < target.GetBlockSizeInBytes())
                            {
                                fprintf(stderr, "ERROR: invalid random distribution target range %I64u - must be a minimum of the specified block size (%u bytes)\n", r._dst.second, target.GetBlockSizeInBytes());
                                fOk = false;
                                break;
                            }
                        }

                        // Validate accumulating IO%
                        if (ioAcc + r._span > 100)
                        {
                            fprintf(stderr, "ERROR: invalid random distribution IO%% %u: can be at most %u - total must be <= 100%%\n", r._span, 100 - ioAcc);
                            fOk = false;
                            break;
                        }

                        // Validate accumulating Target%
                        // Note that absolute range needs no additional validation - known nonzero/large enough for IO
                        if (target.GetDistributionType() == DistributionType::Percent)
                        {
                            if (targetAcc + r._dst.second > 100) 
                            {
                                fprintf(stderr, "ERROR: invalid random distribution Target%% %I64u: can be at most %I64u - total must be <= 100%%\n", r._dst.second, 100 - targetAcc);
                                fOk = false;
                                break;
                            }

                            // Consuming the target before consuming the IO is invalid.
                            // No holes in IO.
                            if (targetAcc + r._dst.second == 100 && ioAcc + r._span < 100)
                            {
                                fprintf(stderr, "ERROR: invalid random distribution: the target is covered with %u%% IO left to distribute\n", 100 - (ioAcc + r._span));
                                fOk = false;
                                break;
                            }
                        }

                        ioAcc += r._span;
                        targetAcc += r._dst.second;
                    }

                    // Percent dist Target% must sum to 100%. IO% underflow (either due to early Target% 100% or Target% overflow) is handled above.
                    if (target.GetDistributionType() == DistributionType::Percent &&
                        targetAcc != 100)
                    {
                        fprintf(stderr, "ERROR: invalid random distribution span: Target%% (%I64u%%) must total 100%%\n", targetAcc);
                        fOk = false;
                    }
                    if (absZero && !absZeroLast)
                    {
                        fprintf(stderr, "ERROR: invalid zero target range in random distribution - must be a minimum of the specified block size (%u bytes)\n", target.GetBlockSizeInBytes());
                        fOk = false;
                    }
                }

                if (target.GetRandomDataWriteBufferSize() > 0)
                {
                    if (target.GetRandomDataWriteBufferSize() < target.GetBlockSizeInBytes())
                    {
                        fprintf(stderr, "ERROR: custom write buffer (-Z) is smaller than the block size. Write buffer size: %I64u block size: %u\n",
                            target.GetRandomDataWriteBufferSize(),
                            target.GetBlockSizeInBytes());
                        fOk = false;
                    }
                }

                if (target.GetMemoryMappedIoMode() == MemoryMappedIoMode::On)
                {
                    if (timeSpan.GetCompletionRoutines())
                    {
                        fprintf(stderr, "ERROR: completion routines (-x) can't be used with memory mapped IO (-Sm)\n");
                        fOk = false;
                    }
                    if (target.GetCacheMode() == TargetCacheMode::DisableOSCache)
                    {
                        fprintf(stderr, "ERROR: unbuffered IO (-Su or -Sh) can't be used with memory mapped IO (-Sm)\n");
                        fOk = false;
                    }
                }

                if (target.GetMemoryMappedIoMode() == MemoryMappedIoMode::Off &&
                    target.GetMemoryMappedIoFlushMode() != MemoryMappedIoFlushMode::Undefined)
                {
                    fprintf(stderr, "ERROR: memory mapped flush mode (-N) can only be specified with memory mapped IO (-Sm)\n");
                    fOk = false;
                }

                if (GetProfileOnly() == false)
                {
                    auto sPath = target.GetPath();

                    if (sPath[0] == TEMPLATE_TARGET_PREFIX)
                    {
                        fprintf(stderr, "ERROR: template target '%s' was not substituted - all template targets must be substituted to run a profile\n", sPath.c_str());
                        fOk = false;
                    }
                }

                // in the cases where there is only a single configuration specified for each target (e.g., cmdline),
                // currently there are no validations specific to individual targets (e.g., pre-existing files)
                // so we can stop validation now. this allows us to only warn/error once, as opposed to repeating
                // it for each target.
                if (fSingleSpec)
                {
                    break;
                }
            }
        }
    }

    return fOk;
}

bool ThreadParameters::AllocateAndFillBufferForTarget(const Target& target)
{
    bool fOk = true;
    BYTE *pDataBuffer = nullptr;
    DWORD requestCount = target.GetRequestCount();
    size_t cbDataBuffer;

    // Use global request count
    if (pTimeSpan->GetThreadCount() != 0 &&
        pTimeSpan->GetRequestCount() != 0) {

        requestCount = pTimeSpan->GetRequestCount();
    }

    // Create separate read & write buffers so the write content doesn't get overriden by reads
    cbDataBuffer = (size_t) target.GetBlockSizeInBytes() * requestCount * 2;
    if (target.GetUseLargePages())
    {
        size_t cbMinLargePage = GetLargePageMinimum();
        size_t cbRoundedSize = (cbDataBuffer + cbMinLargePage - 1) & ~(cbMinLargePage - 1);
        pDataBuffer = (BYTE *)VirtualAlloc(nullptr, cbRoundedSize, MEM_COMMIT | MEM_RESERVE | MEM_LARGE_PAGES, PAGE_EXECUTE_READWRITE);
    }
    else
    {
        pDataBuffer = (BYTE *)VirtualAlloc(nullptr, cbDataBuffer, MEM_COMMIT, PAGE_READWRITE);
    }

    fOk = (pDataBuffer != nullptr);

    //fill buffer (useful only for write tests)
    if (fOk && target.GetWriteRatio() > 0)
    {
        if (target.GetZeroWriteBuffers())
        {
            memset(pDataBuffer, 0, cbDataBuffer);
        }
        else
        {
            for (size_t i = 0; i < cbDataBuffer; i++)
            {
                pDataBuffer[i] = (BYTE)(i % 256);
            }
        }
    }

    if (fOk)
    {
        vpDataBuffers.push_back(pDataBuffer);
        vulReadBufferSize.push_back(cbDataBuffer / 2);
    }

    return fOk;
}

BYTE* ThreadParameters::GetReadBuffer(size_t iTarget, size_t iRequest)
{
    return vpDataBuffers[iTarget] + (iRequest * vTargets[iTarget].GetBlockSizeInBytes());
}

BYTE* ThreadParameters::GetWriteBuffer(size_t iTarget, size_t iRequest)
{
    BYTE *pBuffer = nullptr;
    
    Target& target(vTargets[iTarget]);
    size_t cb = static_cast<size_t>(target.GetRandomDataWriteBufferSize());
    if (cb == 0)
    {
        pBuffer = vpDataBuffers[iTarget] + vulReadBufferSize[iTarget] + (iRequest * vTargets[iTarget].GetBlockSizeInBytes());

        //
        // This is a very efficient algorithm for generating random content at
        // run-time.  When tested in a single-threaded, CPU limited environment
        // with 4K random writes, doing memset to fill the buffer got 112K IOPS,
        // this algorithm got 111K IOPS.  Using a static buffer got 118K IOPS.
        // This was tested with a 64-bit diskspd.exe.  With a 32-bit version it
        // may be more efficient to do 32-bit operations.
        //

        if (pTimeSpan->GetRandomWriteData() &&
            !target.GetZeroWriteBuffers())
        {
            pRand->RandBuffer(pBuffer, vTargets[iTarget].GetBlockSizeInBytes(), true);
        }
    }
    else
    {
        pBuffer = target.GetRandomDataWriteBuffer(pRand);
    }
    return pBuffer;
}

bool ThreadParameters::InitializeMappedViewForTarget(Target& target, DWORD DesiredAccess)
{
    bool fOk = true;
    DWORD dwProtect = PAGE_READWRITE;

    if (DesiredAccess == GENERIC_READ)
    {
        dwProtect = PAGE_READONLY;
    }

    HANDLE hFile = CreateFileMapping(target.GetMappedViewFileHandle(), NULL, dwProtect, 0, 0, NULL);
    fOk = (hFile != NULL);
    if (fOk)
    {
        DWORD dwDesiredAccess = FILE_MAP_WRITE;

        if (DesiredAccess == GENERIC_READ)
        {
            dwDesiredAccess = FILE_MAP_READ;
        }

        BYTE *mapView = (BYTE*) MapViewOfFile(hFile, dwDesiredAccess, 0, 0, 0);
        fOk = (mapView != NULL);
        if (fOk)
        {
            target.SetMappedView(mapView);
        }
        else
        {
            fprintf(stderr, "FATAL ERROR: Could not map view for target '%s'. Error code: 0x%x\n", target.GetPath().c_str(), GetLastError());
        }
    }
    else
    {
        fprintf(stderr, "FATAL ERROR: Could not create a file mapping for target '%s'. Error code: 0x%x\n", target.GetPath().c_str(), GetLastError());
    }
    return fOk;
}

DWORD ThreadParameters::GetTotalRequestCount() const
{
    DWORD cRequests = 0;

    for (const auto& t : vTargets)
    {
        cRequests += t.GetRequestCount();
    }

    if (pTimeSpan->GetRequestCount() != 0 &&
        pTimeSpan->GetThreadCount() != 0)
    {
        cRequests = pTimeSpan->GetRequestCount();
    }

    return cRequests;
}

void EtwResultParser::ParseResults(vector<Results> vResults)
{
    if (TraceLoggingProviderEnabled(g_hEtwProvider,
                                    TRACE_LEVEL_NONE,
                                    DISKSPD_TRACE_INFO))
    {
        for (size_t ullResults = 0; ullResults < vResults.size(); ullResults++)
        {
            const Results& results = vResults[ullResults];
            for (size_t ullThread = 0; ullThread < results.vThreadResults.size(); ullThread++)
            {
                const ThreadResults& threadResults = results.vThreadResults[ullThread];
                for (const auto& targetResults : threadResults.vTargetResults)
                {
                    if (targetResults.ullReadIOCount)
                    {
                        _WriteResults(IOOperation::ReadIO, targetResults, ullThread);
                    }
                    if (targetResults.ullWriteIOCount)
                    {
                        _WriteResults(IOOperation::WriteIO, targetResults, ullThread);
                    }
                }
            }
        }
    }
}

void EtwResultParser::_WriteResults(IOOperation type, const TargetResults& targetResults, size_t ullThread)
{
    UINT64 ullIOCount = (type == IOOperation::ReadIO) ? targetResults.ullReadIOCount : targetResults.ullWriteIOCount;
    UINT64 ullBytesCount = (type == IOOperation::ReadIO) ? targetResults.ullReadBytesCount : targetResults.ullWriteBytesCount;

    TraceLoggingWrite(g_hEtwProvider,
                      "Statistics",
                      TraceLoggingLevel((TRACE_LEVEL_NONE)),
                      TraceLoggingString((type == IOOperation::ReadIO) ? "Read" : "Write", "IO Type"),
                      TraceLoggingUInt64(ullThread, "Thread"),
                      TraceLoggingUInt64(ullBytesCount, "Bytes"),
                      TraceLoggingUInt64(ullIOCount, "IO Count"),
                      TraceLoggingString(targetResults.sPath.c_str(), "Path"),
                      TraceLoggingUInt64(targetResults.ullFileSize, "File Size"));
}
