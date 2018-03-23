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
	_ulState[0] = 0xf1ea5eed;
    _ulState[1] = ulSeed;
    _ulState[2] = ulSeed;
    _ulState[3] = ulSeed;
    
    for (UINT32 i = 0; i < 20; i++) {
        Rand64();
    }
}

void Random::RandBuffer(BYTE *pBuffer, UINT32 ulLength, bool fPseudoRandomOkay)
{
	auto Remaining = static_cast<UINT32>(reinterpret_cast<ULONG_PTR>(pBuffer) & 7);
    UINT64 r1, r2, r3, r4;

    //
    // Align to 8 bytes
    //

    if (Remaining != 0) {
        r1 = Rand64();

        while (Remaining != 0 && ulLength != 0) {
            *pBuffer = static_cast<BYTE>(r1 & 0xFF);
            r1 >>= 8;
            pBuffer++;
            ulLength--;
            Remaining--;
        }
    }

	auto*pBuffer64 = reinterpret_cast<UINT64*>(pBuffer);
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
	        const UINT64 r5 = Rand64();

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
            *pBuffer = static_cast<BYTE>(r1 & 0xFF);
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

string ThreadTarget::GetXml() const
{
    char buffer[4096];
    string sXml("<ThreadTarget>\n");

    sprintf_s(buffer, _countof(buffer), "<Thread>%u</Thread>\n", _ulThread);
    sXml += buffer;

    if (_ulWeight != 0)
    {
        sprintf_s(buffer, _countof(buffer), "<Weight>%u</Weight>\n", _ulWeight);
        sXml += buffer;
    }

    sXml += "</ThreadTarget>\n";

    return sXml;
}

string Target::GetXml() const
{
    char buffer[4096];
    string sXml("<Target>\n");
    sXml += "<Path>" + _sPath + "</Path>\n";

    sprintf_s(buffer, _countof(buffer), "<BlockSize>%lu</BlockSize>\n", _dwBlockSize);
    sXml += buffer;

    sprintf_s(buffer, _countof(buffer), "<BaseFileOffset>%I64u</BaseFileOffset>\n", _ullBaseFileOffset);
    sXml += buffer;

    sXml += _fSequentialScanHint ? "<SequentialScan>true</SequentialScan>\n" : "<SequentialScan>false</SequentialScan>\n";
    sXml += _fRandomAccessHint ? "<RandomAccess>true</RandomAccess>\n" : "<RandomAccess>false</RandomAccess>\n";
    sXml += _fTemporaryFileHint ? "<TemporaryFile>true</TemporaryFile>\n" : "<TemporaryFile>false</TemporaryFile>\n";
    sXml += _fUseLargePages ? "<UseLargePages>true</UseLargePages>\n" : "<UseLargePages>false</UseLargePages>\n";

    // TargetCacheMode::Cached is implied default
    switch (_cacheMode)
    {
    case TargetCacheMode::DisableLocalCache:
        sXml += "<DisableLocalCache>true</DisableLocalCache>\n";
        break;
    case TargetCacheMode::DisableOSCache:
        sXml += "<DisableOSCache>true</DisableOSCache>\n";
        break;
	case TargetCacheMode::Cached:
		break;
	case TargetCacheMode::Undefined:
		/* ? */
		break;
    }

    // WriteThroughMode::Off is implied default
    switch (_writeThroughMode)
    {
    case WriteThroughMode::On:
        sXml += "<WriteThrough>true</WriteThrough>\n";
        break;
	case WriteThroughMode::Off:
		break;
	case WriteThroughMode::Undefined:
		/* ? */
		break;
    }
    
    sXml += "<WriteBufferContent>\n";
    if (_fZeroWriteBuffers)
    {
        sXml += "<Pattern>zero</Pattern>\n";
    }
    else if (_cbRandomDataWriteBuffer == 0)
    {
        sXml += "<Pattern>sequential</Pattern>\n";
    }
    else
    {
        sXml += "<Pattern>random</Pattern>\n";
        sXml += "<RandomDataSource>\n";
        sprintf_s(buffer, _countof(buffer), "<SizeInBytes>%I64u</SizeInBytes>\n", _cbRandomDataWriteBuffer);
        sXml += buffer;
        if (!_sRandomDataWriteBufferSourcePath.empty())
        {
            sXml += "<FilePath>" + _sRandomDataWriteBufferSourcePath + "</FilePath>\n";
        }
        sXml += "</RandomDataSource>\n";
    }
    sXml += "</WriteBufferContent>\n";

    sXml += _fParallelAsyncIO ? "<ParallelAsyncIO>true</ParallelAsyncIO>\n" : "<ParallelAsyncIO>false</ParallelAsyncIO>\n";

    if (_fUseBurstSize)
    {
        sprintf_s(buffer, _countof(buffer), "<BurstSize>%lu</BurstSize>\n", _dwBurstSize);
        sXml += buffer;
    }

    if (_fThinkTime)
    {
        sprintf_s(buffer, _countof(buffer), "<ThinkTime>%lu</ThinkTime>\n", _dwThinkTime);
        sXml += buffer;
    }

    if (_fCreateFile)
    {
        sprintf_s(buffer, _countof(buffer), "<FileSize>%I64u</FileSize>\n", _ullFileSize);
        sXml += buffer;
    }

    // If XML contains <Random>, <StrideSize> is ignored
    if (_fUseRandomAccessPattern)
    {
        sprintf_s(buffer, _countof(buffer), "<Random>%I64u</Random>\n", GetBlockAlignmentInBytes());
        sXml += buffer;
    }
    else
    {
        sprintf_s(buffer, _countof(buffer), "<StrideSize>%I64u</StrideSize>\n", GetBlockAlignmentInBytes());
        sXml += buffer;
    
        sXml += _fInterlockedSequential ?
            "<InterlockedSequential>true</InterlockedSequential>\n" :
            "<InterlockedSequential>false</InterlockedSequential>\n";
    }

    sprintf_s(buffer, _countof(buffer), "<ThreadStride>%I64u</ThreadStride>\n", _ullThreadStride);
    sXml += buffer;

    sprintf_s(buffer, _countof(buffer), "<MaxFileSize>%I64u</MaxFileSize>\n", _ullMaxFileSize);
    sXml += buffer;

    sprintf_s(buffer, _countof(buffer), "<RequestCount>%lu</RequestCount>\n", _dwRequestCount);
    sXml += buffer;

    sprintf_s(buffer, _countof(buffer), "<WriteRatio>%u</WriteRatio>\n", _ulWriteRatio);
    sXml += buffer;

    sprintf_s(buffer, _countof(buffer), "<Throughput>%lu</Throughput>\n", _dwThroughputBytesPerMillisecond);
    sXml += buffer;

    sprintf_s(buffer, _countof(buffer), "<ThreadsPerFile>%lu</ThreadsPerFile>\n", _dwThreadsPerFile);
    sXml += buffer;

    if (_ioPriorityHint == IoPriorityHintVeryLow)
    {
        sXml += "<IOPriority>1</IOPriority>\n";
    }
    else if (_ioPriorityHint == IoPriorityHintLow)
    {
        sXml += "<IOPriority>2</IOPriority>\n";
    }
    else if (_ioPriorityHint == IoPriorityHintNormal)
    {
        sXml += "<IOPriority>3</IOPriority>\n";
    }
    else
    {
        sXml += "<IOPriority>* UNSUPPORTED *</IOPriority>\n";
    }

    sprintf_s(buffer, _countof(buffer), "<Weight>%u</Weight>\n", _ulWeight);
    sXml += buffer;

    for (const auto& threadTarget : _vThreadTargets)
    {
        sXml += threadTarget.GetXml();
    }

    sXml += "</Target>\n";

    return sXml;
}

bool Target::_FillRandomDataWriteBuffer(Random *pRand) const
{
    assert(_pRandomDataWriteBuffer != nullptr);
    bool fOk = true;
	const auto cb = static_cast<size_t>(GetRandomDataWriteBufferSize());
    if (GetRandomDataWriteBufferSourcePath().empty())
    {
        pRand->RandBuffer(_pRandomDataWriteBuffer, static_cast<UINT32>(cb), false);
    }
    else
    {
        // fill buffer from file
	    HANDLE hFile = CreateFile(GetRandomDataWriteBufferSourcePath().c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, 0, nullptr);
        if (hFile != INVALID_HANDLE_VALUE)
        {
	        const UINT64 cbLeftToRead = GetRandomDataWriteBufferSize();
            BYTE *pBuffer = _pRandomDataWriteBuffer;
            bool fReadSuccess = true;
            while (fReadSuccess && cbLeftToRead > 0)
            {
	            const auto cbToRead = static_cast<DWORD>(min(64 * 1024, cbLeftToRead));
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
	const auto cb = static_cast<size_t>(GetRandomDataWriteBufferSize());
    assert(cb > 0);

    // TODO: make sure the size if <= max value for size_t
    if (GetUseLargePages())
    {
	    const size_t cbMinLargePage = GetLargePageMinimum();
	    const size_t cbRoundedSize = (cb + cbMinLargePage - 1) & ~(cbMinLargePage - 1);
        _pRandomDataWriteBuffer = static_cast<BYTE *>(VirtualAlloc(nullptr, cbRoundedSize, MEM_COMMIT | MEM_RESERVE | MEM_LARGE_PAGES,
                                                                   PAGE_EXECUTE_READWRITE));
    }
    else
    {
        _pRandomDataWriteBuffer = static_cast<BYTE *>(VirtualAlloc(nullptr, cb, MEM_COMMIT, PAGE_READWRITE));
    }

	auto fOk = (_pRandomDataWriteBuffer != nullptr);
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

BYTE* Target::GetRandomDataWriteBuffer(Random *pRand) const
{
	const auto cbBuffer = static_cast<size_t>(GetRandomDataWriteBufferSize());
	const size_t cbBlock = GetBlockSizeInBytes();

    // leave enough bytes in the buffer for one block
    size_t randomOffset = pRand->Rand32() % (cbBuffer - (cbBlock - 1));

	const bool fUnbufferedIO = (_cacheMode == TargetCacheMode::DisableOSCache);
    if (fUnbufferedIO)
    {
        // for unbuffered IO, offset in the buffer needs to be 512-byte aligned
        const size_t cbAlignment = 512;
        randomOffset -= (randomOffset % cbAlignment);
    }

	auto pBuffer = reinterpret_cast<BYTE*>(reinterpret_cast<ULONG_PTR>(_pRandomDataWriteBuffer)+randomOffset);

    // unbuffered IO needs aligned addresses
    assert(!fUnbufferedIO || (reinterpret_cast<ULONG_PTR>(pBuffer) % 512 == 0));
    assert(pBuffer >= _pRandomDataWriteBuffer);
    assert(pBuffer <= _pRandomDataWriteBuffer + GetRandomDataWriteBufferSize() - GetBlockSizeInBytes());

    return pBuffer;
}

string TimeSpan::GetXml() const
{
    string sXml("<TimeSpan>\n");
    char buffer[4096];

    sXml += _fCompletionRoutines ? "<CompletionRoutines>true</CompletionRoutines>\n" : "<CompletionRoutines>false</CompletionRoutines>\n";
    sXml += _fMeasureLatency ? "<MeasureLatency>true</MeasureLatency>\n" : "<MeasureLatency>false</MeasureLatency>\n";
    sXml += _fCalculateIopsStdDev ? "<CalculateIopsStdDev>true</CalculateIopsStdDev>\n" : "<CalculateIopsStdDev>false</CalculateIopsStdDev>\n";
    sXml += _fDisableAffinity ? "<DisableAffinity>true</DisableAffinity>\n" : "<DisableAffinity>false</DisableAffinity>\n";

    sprintf_s(buffer, _countof(buffer), "<Duration>%u</Duration>\n", _ulDuration);
    sXml += buffer;

    sprintf_s(buffer, _countof(buffer), "<Warmup>%u</Warmup>\n", _ulWarmUp);
    sXml += buffer;

    sprintf_s(buffer, _countof(buffer), "<Cooldown>%u</Cooldown>\n", _ulCoolDown);
    sXml += buffer;

    sprintf_s(buffer, _countof(buffer), "<ThreadCount>%lu</ThreadCount>\n", _dwThreadCount);
    sXml += buffer;

    sprintf_s(buffer, _countof(buffer), "<RequestCount>%lu</RequestCount>\n", _dwRequestCount);
    sXml += buffer;

    sprintf_s(buffer, _countof(buffer), "<IoBucketDuration>%u</IoBucketDuration>\n", _ulIoBucketDurationInMilliseconds);
    sXml += buffer;

    sprintf_s(buffer, _countof(buffer), "<RandSeed>%u</RandSeed>\n", _ulRandSeed);
    sXml += buffer;

    if (!_vAffinity.empty())
    {
        sXml += "<Affinity>\n";
        for (const auto& a : _vAffinity)
        {
            sprintf_s(buffer, _countof(buffer), "<AffinityGroupAssignment Group=\"%u\" Processor=\"%u\"/>\n", a.wGroup, a.bProc);
            sXml += buffer;
        }
        sXml += "</Affinity>\n";
    }

    sXml += "<Targets>\n";
    for (const auto& target : _vTargets)
    {
        sXml += target.GetXml();
    }
    sXml += "</Targets>\n";
    sXml += "</TimeSpan>\n";
    return sXml;
}

void TimeSpan::MarkFilesAsPrecreated(const vector<string>& vFiles)
{
    for (const auto& sFile : vFiles)
    {
        for (auto& _vTarget : _vTargets)
        {
            if (sFile == _vTarget.GetPath())
            {
	            _vTarget.SetPrecreated(true);
            }
        }
    }
}

string Profile::GetXml() const
{
    string sXml("<Profile>\n");
    char buffer[4096];

    sprintf_s(buffer, _countof(buffer), "<Progress>%lu</Progress>\n", _dwProgress);
    sXml += buffer;

    if (_resultsFormat == ResultsFormat::Text)
    {
        sXml += "<ResultFormat>text</ResultFormat>\n";
    }
    else if (_resultsFormat == ResultsFormat::Xml)
    {
        sXml += "<ResultFormat>xml</ResultFormat>\n";
    }
    else
    {
        sXml += "<ResultFormat>* UNSUPPORTED *</ResultFormat>\n";
    }

    sXml += _fVerbose ? "<Verbose>true</Verbose>\n" : "<Verbose>false</Verbose>\n";
    if (_precreateFiles == PrecreateFiles::UseMaxSize)
    {
        sXml += "<PrecreateFiles>UseMaxSize</PrecreateFiles>\n";
    }
    else if (_precreateFiles == PrecreateFiles::OnlyFilesWithConstantSizes)
    {
        sXml += "<PrecreateFiles>CreateOnlyFilesWithConstantSizes</PrecreateFiles>\n";
    }
    else if (_precreateFiles == PrecreateFiles::OnlyFilesWithConstantOrZeroSizes)
    {
        sXml += "<PrecreateFiles>CreateOnlyFilesWithConstantOrZeroSizes</PrecreateFiles>\n";
    }

    if (_fEtwEnabled)
    {
        sXml += _fEtwProcess ? "<Process>true</Process>\n" : "<Process>false</Process>\n";
        sXml += _fEtwThread ? "<Thread>true</Thread>\n" : "<Thread>false</Thread>\n";
        sXml += _fEtwImageLoad ? "<ImageLoad>true</ImageLoad>\n" : "<ImageLoad>false</ImageLoad>\n";
        sXml += _fEtwDiskIO ? "<DiskIO>true</DiskIO>\n" : "<DiskIO>false</DiskIO>\n";
        sXml += _fEtwMemoryPageFaults ? "<MemoryPageFaults>true</MemoryPageFaults>\n" : "<MemoryPageFaults>false</MemoryPageFaults>\n";
        sXml += _fEtwMemoryHardFaults ? "<MemoryHardFaults>true</MemoryHardFaults>\n" : "<MemoryHardFaults>false</MemoryHardFaults>\n";
        sXml += _fEtwNetwork ? "<Network>true</Network>\n" : "<Network>false</Network>\n";
        sXml += _fEtwRegistry ? "<Registry>true</Registry>\n" : "<Registry>false</Registry>\n";
        sXml += _fEtwUsePagedMemory ? "<UsePagedMemory>true</UsePagedMemory>\n" : "<UsePagedMemory>false</UsePagedMemory>\n";
        sXml += _fEtwUsePerfTimer ? "<UsePerfTimer>true</UsePerfTimer>\n" : "<UsePerfTimer>false</UsePerfTimer>\n";
        sXml += _fEtwUseSystemTimer ? "<UseSystemTimer>true</UseSystemTimer>\n" : "<UseSystemTimer>false</UseSystemTimer>\n";
        sXml += _fEtwUseCyclesCounter ? "<UseCyclesCounter>true</UseCyclesCounter>\n" : "<UseCyclesCounter>false</UseCyclesCounter>\n";
    }

    sXml += "<TimeSpans>\n";
    for (const auto& timespan : _vTimeSpans)
    {
        sXml += timespan.GetXml();
    }
    sXml += "</TimeSpans>\n";
    sXml += "</Profile>\n";
    return sXml;
}

void Profile::MarkFilesAsPrecreated(const vector<string>& vFiles)
{
    for (auto& _vTimeSpan : _vTimeSpans)
    {
	    _vTimeSpan.MarkFilesAsPrecreated(vFiles);
    }
}

bool Profile::Validate(bool fSingleSpec, SystemInformation *pSystem) const
{
    bool fOk = true;

    if (GetTimeSpans().empty())
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
                            static_cast<unsigned int>(pSystem->processorTopology._vProcessorGroupInformation.size()));

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
                        fprintf(
	                        stderr,
	                        "ERROR: affinity assignment to group %u core %u not possible; core is not active (current mask 0x%llx)\n",
                            Affinity.wGroup,
                            Affinity.bProc,
                            pSystem->processorTopology._vProcessorGroupInformation[Affinity.wGroup].
	                        _activeProcessorMask);

                        fOk = false;
                    }
                }
            }

            if (timeSpan.GetDisableAffinity() && !timeSpan.GetAffinityAssignments().empty())
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
                else if (!target.GetThreadTargets().empty())
                {
                    fprintf(stderr, "ERROR: ThreadTargets can only be specified when the timespan ThreadCount and RequestCount are specified\n");
                    fOk = false;
                }

                // FIXME: we can no longer do this check, because the target no longer
                // contains a property that uniquely identifies the case where "-s" or <StrideSize>
                // was passed.  
#if 0
                if (target.GetUseRandomAccessPattern() && target.GetEnableCustomStrideSize())
                {
                    fprintf(stderr, "WARNING: -s is ignored if -r is provided\n");
                }
#endif
                if (target.GetUseRandomAccessPattern())
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
                    if (target.GetUseParallelAsyncIO() && target.GetRequestCount() == 1)
                    {
                        fprintf(stderr, "WARNING: -p does not have effect unless outstanding I/O count (-o) is > 1\n");
                    }

                    if (timeSpan.GetRandSeed() > 0)
                    {
                        fprintf(stderr, "WARNING: -z is ignored if -r is not provided\n");
                        // although ulRandSeed==0 is a valid value, it's interpreted as "not provided" for this warning
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

                if (target.GetRandomDataWriteBufferSize() > 0)
                {
                    if (target.GetRandomDataWriteBufferSize() < target.GetBlockSizeInBytes())
                    {
                        fprintf(stderr, "ERROR: custom write buffer (-Z) is smaller than the block size. Write buffer size: %I64u block size: %lu\n",
                            target.GetRandomDataWriteBufferSize(),
                            target.GetBlockSizeInBytes());
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
    DWORD requestCount = target.GetRequestCount();

	// Use global request count
    if (pTimeSpan->GetThreadCount() != 0 &&
        pTimeSpan->GetRequestCount() != 0) {

        requestCount = pTimeSpan->GetRequestCount();
    }

    // Create separate read & write buffers so the write content doesn't get overriden by reads
	const auto cbDataBuffer = static_cast<size_t>(target.GetBlockSizeInBytes()) * requestCount * 2;
	BYTE *pDataBuffer;
    if (target.GetUseLargePages())
    {
	    const size_t cbMinLargePage = GetLargePageMinimum();
	    const size_t cbRoundedSize = (cbDataBuffer + cbMinLargePage - 1) & ~(cbMinLargePage - 1);
        pDataBuffer = static_cast<BYTE *>(VirtualAlloc(nullptr, cbRoundedSize, MEM_COMMIT | MEM_RESERVE | MEM_LARGE_PAGES,
                                                       PAGE_EXECUTE_READWRITE));
    }
    else
    {
        pDataBuffer = static_cast<BYTE *>(VirtualAlloc(nullptr, cbDataBuffer, MEM_COMMIT, PAGE_READWRITE));
    }

	const bool fOk = (pDataBuffer != nullptr);

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
                pDataBuffer[i] = static_cast<BYTE>(i % 256);
	            /* NB: the writable size may be 'cbRoundedSize' bytes, but '2' bytes might be written */
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
    BYTE *pBuffer;
    
    Target& target(vTargets[iTarget]);
	const auto cb = static_cast<size_t>(target.GetRandomDataWriteBufferSize());
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
