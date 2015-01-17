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

string Util::DoubleToStringHelper(const double d)
{
    char szFloatBuffer[100];
    sprintf_s(szFloatBuffer, _countof(szFloatBuffer), "%10.3lf", d);

    return string(szFloatBuffer);
}

string Target::GetXml() const
{
    char buffer[4096];
    string sXml("<Target>\n");
    sXml += "<Path>" + _sPath + "</Path>\n";

    sprintf_s(buffer, _countof(buffer), "<BlockSize>%u</BlockSize>\n", _dwBlockSize);
    sXml += buffer;

    sprintf_s(buffer, _countof(buffer), "<BaseFileOffset>%I64u</BaseFileOffset>\n", _ullBaseFileOffset);
    sXml += buffer;

    sXml += _fSequentialScanHint ? "<SequentialScan>true</SequentialScan>\n" : "<SequentialScan>false</SequentialScan>\n";
    sXml += _fRandomAccessHint ? "<RandomAccess>true</RandomAccess>\n" : "<RandomAccess>false</RandomAccess>\n";
    sXml += _fUseLargePages ? "<UseLargePages>true</UseLargePages>\n" : "<UseLargePages>false</UseLargePages>\n";
    sXml += _fDisableAllCache ? "<DisableAllCache>true</DisableAllCache>\n" : "<DisableAllCache>false</DisableAllCache>\n";
    // normalize cache controls - disabling the OS cache is included if all caches are disabled,
    // so specifying it twice is not neccesary (this generates a warning on the cmdline)
    if (!_fDisableAllCache)
    {
        sXml += _fDisableOSCache ? "<DisableOSCache>true</DisableOSCache>\n" : "<DisableOSCache>false</DisableOSCache>\n";
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
        if (_sRandomDataWriteBufferSourcePath != "")
        {
            sXml += "<FilePath>" + _sRandomDataWriteBufferSourcePath + "</FilePath>\n";
        }
        sXml += "</RandomDataSource>\n";
    }
    sXml += "</WriteBufferContent>\n";

    sXml += _fParallelAsyncIO ? "<ParallelAsyncIO>true</ParallelAsyncIO>\n" : "<ParallelAsyncIO>false</ParallelAsyncIO>\n";

    if (_fUseBurstSize)
    {
        sprintf_s(buffer, _countof(buffer), "<BurstSize>%u</BurstSize>\n", _dwBurstSize);
        sXml += buffer;
    }

    if (_fThinkTime)
    {
        sprintf_s(buffer, _countof(buffer), "<ThinkTime>%u</ThinkTime>\n", _dwThinkTime);
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

    sprintf_s(buffer, _countof(buffer), "<RequestCount>%u</RequestCount>\n", _dwRequestCount);
    sXml += buffer;

    sprintf_s(buffer, _countof(buffer), "<WriteRatio>%u</WriteRatio>\n", _ulWriteRatio);
    sXml += buffer;

    sprintf_s(buffer, _countof(buffer), "<Throughput>%u</Throughput>\n", _dwThroughputBytesPerMillisecond);
    sXml += buffer;

    sprintf_s(buffer, _countof(buffer), "<ThreadsPerFile>%u</ThreadsPerFile>\n", _dwThreadsPerFile);
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

    sXml += "</Target>\n";


    return sXml;
}

bool Target::_FillRandomDataWriteBuffer()
{
    assert(_pRandomDataWriteBuffer != nullptr);
    bool fOk = true;
    size_t cb = static_cast<size_t>(GetRandomDataWriteBufferSize());
    if (GetRandomDataWriteBufferSourcePath() == "")
    {
        // fill buffer with random data
        for (size_t i = 0; i < cb; i++)
        {
            _pRandomDataWriteBuffer[i] = (rand() % 256);
        }
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
            fOk = false;
            // TODO: print error message?
        }
    }
    return fOk;
}

bool Target::AllocateAndFillRandomDataWriteBuffer()
{
    assert(_pRandomDataWriteBuffer == nullptr);
    bool fOk = true;
    size_t cb = static_cast<size_t>(GetRandomDataWriteBufferSize());
    assert(cb > 0);

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
        fOk = _FillRandomDataWriteBuffer();
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

BYTE* Target::GetRandomDataWriteBuffer()
{
    size_t cbBuffer = static_cast<size_t>(GetRandomDataWriteBufferSize());
    size_t cbBlock = GetBlockSizeInBytes();

    // leave enough bytes in the buffer for one block
    size_t randomOffset = rand() % (cbBuffer - (cbBlock - 1));

    bool fUnbufferedIO = (GetDisableOSCache() || GetDisableAllCache());
    if (fUnbufferedIO)
    {
        // for unbuffered IO, offset in the buffer needs to be DWORD-aligned
        const size_t cbAlignment = 4;
        randomOffset -= (randomOffset % cbAlignment);
    }

    BYTE *pBuffer = reinterpret_cast<BYTE*>(reinterpret_cast<ULONG_PTR>(_pRandomDataWriteBuffer)+randomOffset);

    // unbuffered IO needs aligned addresses
    assert(!fUnbufferedIO || (reinterpret_cast<ULONG_PTR>(pBuffer) % 4 == 0));

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
    sXml += _fGroupAffinity ? "<GroupAffinity>true</GroupAffinity>\n" : "<GroupAffinity>false</GroupAffinity>\n";

    sprintf_s(buffer, _countof(buffer), "<Duration>%u</Duration>\n", _ulDuration);
    sXml += buffer;

    sprintf_s(buffer, _countof(buffer), "<Warmup>%u</Warmup>\n", _ulWarmUp);
    sXml += buffer;

    sprintf_s(buffer, _countof(buffer), "<Cooldown>%u</Cooldown>\n", _ulCoolDown);
    sXml += buffer;

    sprintf_s(buffer, _countof(buffer), "<ThreadCount>%u</ThreadCount>\n", _dwThreadCount);
    sXml += buffer;

    sprintf_s(buffer, _countof(buffer), "<IoBucketDuration>%u</IoBucketDuration>\n", _ulIoBucketDurationInMilliseconds);
    sXml += buffer;

    sprintf_s(buffer, _countof(buffer), "<RandSeed>%u</RandSeed>\n", _ulRandSeed);
    sXml += buffer;

    if (_vAffinity.size() > 0)
    {
        sXml += "<Affinity>\n";
        for (auto a : _vAffinity)
        {
            sprintf_s(buffer, _countof(buffer), "<AffinityAssignment>%u</AffinityAssignment>\n", a);
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

string Profile::GetXml() const
{
    string sXml("<Profile>\n");
    char buffer[4096];

    sprintf_s(buffer, _countof(buffer), "<Progress>%u</Progress>\n", _dwProgress);
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

void Profile::MarkFilesAsPrecreated(const vector<string> vFiles)
{
    for (auto pTimeSpan = _vTimeSpans.begin(); pTimeSpan != _vTimeSpans.end(); pTimeSpan++)
    {
        pTimeSpan->MarkFilesAsPrecreated(vFiles);
    }
}

bool Profile::Validate(bool fSingleSpec) const
{
    bool fOk = true;
    for (const auto& timeSpan : GetTimeSpans())
    {
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

            if (target.GetDisableAllCache() && target.GetDisableOSCache())
            {
                fprintf(stderr, "WARNING: -S is included in the effect of -h, specifying both is not required\n");
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
                    fprintf(stderr, "ERROR: custom write buffer (-Z) is smaller than the block size. Write buffer size: %I64u block size: %u\n",
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
    return fOk;
}

bool ThreadParameters::AllocateAndFillBufferForTarget(const Target& target)
{
    bool fOk = true;
    BYTE *pDataBuffer = nullptr;
    size_t cbDataBuffer = target.GetBlockSizeInBytes() * target.GetRequestCount();
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
        pBuffer = vpDataBuffers[iTarget] + (iRequest * vTargets[iTarget].GetBlockSizeInBytes());
    }
    else
    {
        pBuffer = target.GetRandomDataWriteBuffer();
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

    return cRequests;
}
