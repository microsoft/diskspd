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

#include <windows.h>
#include <vector>
#include <Winternl.h>   //ntdll.dll
#include <cassert>
#include "Histogram.h"
#include "IoBucketizer.h"
#include "ThroughputMeter.h"

using namespace std;

// versioning material. for simplicity in consumption, please ensure that the date string
// parses via the System.Datetime constructor as follows (in Powershell):
//
//      [datetime] "string"
//
// this should result in a valid System.Datetime object, rendered like:
//
//      Monday, June 16, 2014 12:00:00 AM

#define DISKSPD_RELEASE_TAG " (MS)"
#define DISKSPD_NUMERIC_VERSION_STRING "2.0.19a" DISKSPD_RELEASE_TAG
#define DISKSPD_DATE_VERSION_STRING "2017/4/28"

using PRINTF = void(WINAPI *)(const char*, va_list);                            //function used for displaying formatted data (printf style)

struct ETWEventCounters
{
    UINT64 ullIORead;                   // Read
    UINT64 ullIOWrite;                  // Write
    UINT64 ullMMTransitionFault;        // Transition fault
    UINT64 ullMMDemandZeroFault;        // Demand Zero fault
    UINT64 ullMMCopyOnWrite;            // Copy on Write
    UINT64 ullMMGuardPageFault;         // Guard Page fault
    UINT64 ullMMHardPageFault;          // Hard page fault
    UINT64 ullNetTcpSend;               // Send
    UINT64 ullNetTcpReceive;            // Receive
    UINT64 ullNetUdpSend;               // Send
    UINT64 ullNetUdpReceive;            // Receive
    UINT64 ullNetConnect;               // Connect
    UINT64 ullNetDisconnect;            // Disconnect
    UINT64 ullNetRetransmit;            // ReTransmit
    UINT64 ullNetAccept;                // Accept
    UINT64 ullNetReconnect;             // ReConnect
    UINT64 ullRegCreate;                // NtCreateKey
    UINT64 ullRegOpen;                  // NtOpenKey
    UINT64 ullRegDelete;                // NtDeleteKey
    UINT64 ullRegQuery;                 // NtQueryKey
    UINT64 ullRegSetValue;              // NtSetValueKey
    UINT64 ullRegDeleteValue;           // NtDeleteValueKey
    UINT64 ullRegQueryValue;            // NtQueryValueKey
    UINT64 ullRegEnumerateKey;          // NtEnumerateKey
    UINT64 ullRegEnumerateValueKey;     // NtEnumerateValueKey
    UINT64 ullRegQueryMultipleValue;    // NtQueryMultipleValueKey
    UINT64 ullRegSetInformation;        // NtSetInformationKey
    UINT64 ullRegFlush;                 // NtFlushKey
    UINT64 ullRegKcbDmp;                // KcbDump/create
    UINT64 ullThreadStart;
    UINT64 ullThreadEnd;
    UINT64 ullProcessStart;
    UINT64 ullProcessEnd;
    UINT64 ullImageLoad;
};

// structure containing informations about ETW session
struct ETWSessionInfo
{
    ULONG ulBufferSize;
    ULONG ulMinimumBuffers;
    ULONG ulMaximumBuffers;
    ULONG ulFreeBuffers;
    ULONG ulBuffersWritten;
    ULONG ulFlushTimer;
    LONG lAgeLimit;
    ULONG ulNumberOfBuffers;
    ULONG ulEventsLost;
    ULONG ulLogBuffersLost;
    ULONG ulRealTimeBuffersLost;
};

// structure containing parameters concerning ETW session provided by user
struct ETWMask
{
    BOOL bProcess;
    BOOL bThread;
    BOOL bImageLoad;
    BOOL bDiskIO;
    BOOL bMemoryPageFaults;
    BOOL bMemoryHardFaults;
    BOOL bNetwork;
    BOOL bRegistry;
    BOOL bUsePagedMemory;
    BOOL bUsePerfTimer;
    BOOL bUseSystemTimer;
    BOOL bUseCyclesCounter;
};

namespace UnitTests
{
    class PerfTimerUnitTests;
    class ProfileUnitTests;
    class TargetUnitTests;
}

class PerfTimer
{
public:

    static UINT64 GetTime();

    static double PerfTimeToMicroseconds(double);
    static double PerfTimeToMilliseconds(double);
    static double PerfTimeToSeconds(double);
    static double PerfTimeToMicroseconds(UINT64);
    static double PerfTimeToMilliseconds(UINT64);
    static double PerfTimeToSeconds(UINT64);

    static UINT64 MicrosecondsToPerfTime(double);
    static UINT64 MillisecondsToPerfTime(double);
    static UINT64 SecondsToPerfTime(double);

private:

    static const UINT64 TIMER_FREQ;
    static UINT64 _GetPerfTimerFreq();

    friend class UnitTests::PerfTimerUnitTests;
};

//
// This code implements Bob Jenkins public domain simple random number generator
// See http://burtleburtle.net/bob/rand/smallprng.html for details
//

class Random
{
public:
    explicit Random(UINT64 ulSeed = 0);

	UINT64 Rand64()
    {
	    const UINT64 e = _ulState[0] - _rotl64(_ulState[1], 7);
        _ulState[0] = _ulState[1] ^ _rotl64(_ulState[2], 13);
        _ulState[1] = _ulState[2] + _rotl64(_ulState[3], 37);
        _ulState[2] = _ulState[3] + e;
        _ulState[3] = e + _ulState[0];
        
        return _ulState[3];
    }

	UINT32 Rand32()
    {
        return static_cast<UINT32>(Rand64());
    }

    void RandBuffer(BYTE *pBuffer, UINT32 ulLength, bool fPseudoRandomOkay);

private:
    UINT64 _ulState[4]{};
};

struct PercentileDescriptor
{
    double Percentile;
    string Name;
};

class Util
{
public:
    static string DoubleToStringHelper(double);
    template<typename T> static T QuotientCeiling(T dividend, T divisor)
    {
        return (dividend + divisor - 1) / divisor;
    }
};

// To keep track of which type of IO was issued
enum class IOOperation
{
    ReadIO = 1,
    WriteIO
};

class TargetResults
{
public:
    TargetResults() :
        ullFileSize(0),
        ullBytesCount(0),
        ullIOCount(0),
        ullReadBytesCount(0),
        ullReadIOCount(0),
        ullWriteBytesCount(0),
        ullWriteIOCount(0)
    {

    }

    void Add(DWORD dwBytesTransferred,
             IOOperation type,
             UINT64 ullIoStartTime,
             UINT64 ullSpanStartTime,
             bool fMeasureLatency,
             bool fCalculateIopsStdDev
             )
    {
        double lfDurationUsec = 0;
        UINT64 ullEndTime = 0;
        
        // assume it is worthwhile to stay off of the time query path unless needed (micro-overhead)
        if (fMeasureLatency || fCalculateIopsStdDev)
        {
            ullEndTime = PerfTimer::GetTime();
            const UINT64 ullDuration = ullEndTime - ullIoStartTime;
            lfDurationUsec = PerfTimer::PerfTimeToMicroseconds(ullDuration);
        }

        if (fMeasureLatency)
        {
            if (type == IOOperation::ReadIO)
            {
                readLatencyHistogram.Add(static_cast<float>(lfDurationUsec));
            }
            else
            {
                writeLatencyHistogram.Add(static_cast<float>(lfDurationUsec));
            }
        }

        if (fCalculateIopsStdDev)
        {
            const UINT64 ullRelativeCompletionTime = ullEndTime - ullSpanStartTime;

            if (type == IOOperation::ReadIO)
            {
                readBucketizer.Add(ullRelativeCompletionTime, lfDurationUsec);
            }
            else
            {
                writeBucketizer.Add(ullRelativeCompletionTime, lfDurationUsec);
            }
        }

        if (type == IOOperation::ReadIO)
        {
            ullReadBytesCount += dwBytesTransferred;    // update read bytes counter
            ullReadIOCount++;                           // update completed read I/O operations counter
        }
        else
        {
            ullWriteBytesCount += dwBytesTransferred;   // update write bytes counter
            ullWriteIOCount++;                          // update completed write I/O operations counter
        }

        ullBytesCount += dwBytesTransferred;            // update bytes counter
        ullIOCount++;                                   // update completed I/O operations counter
    }

    string sPath;
    UINT64 ullFileSize;         //size of the file
    UINT64 ullBytesCount;       //number of accessed bytes
    UINT64 ullIOCount;          //number of performed I/O operations
    UINT64 ullReadBytesCount;   //number of bytes read
    UINT64 ullReadIOCount;      //number of performed Read I/O operations
    UINT64 ullWriteBytesCount;  //number of bytes written
    UINT64 ullWriteIOCount;     //number of performed Write I/O operations

    Histogram<float> readLatencyHistogram;
    Histogram<float> writeLatencyHistogram;

    IoBucketizer readBucketizer;
    IoBucketizer writeBucketizer;
};

class ThreadResults
{
public:
    vector<TargetResults> vTargetResults;
};

class Results
{
public:
    bool fUseETW;
    struct ETWEventCounters EtwEventCounters;
    struct ETWMask EtwMask;
    struct ETWSessionInfo EtwSessionInfo;
    vector<ThreadResults> vThreadResults;
    UINT64 ullTimeCount;
    vector<SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION> vSystemProcessorPerfInfo;
};

using CALLBACK_TEST_STARTED = void(*)();    //callback function to notify that the measured test is about to start
using CALLBACK_TEST_FINISHED = void(*)();   //callback function to notify that the measured test has just finished

class ProcessorGroupInformation
{
public:
    WORD _groupNumber; 
    BYTE _maximumProcessorCount;
    BYTE _activeProcessorCount;
    KAFFINITY _activeProcessorMask;

    ProcessorGroupInformation() = delete;
    ProcessorGroupInformation(
        BYTE MaximumProcessorCount,
        BYTE ActiveProcessorCount,
        WORD Group,
        KAFFINITY ActiveProcessorMask) :
		_groupNumber(Group),
		_maximumProcessorCount(MaximumProcessorCount),
        _activeProcessorCount(ActiveProcessorCount),
        _activeProcessorMask(ActiveProcessorMask)
    {
    }

    bool IsProcessorActive(BYTE Processor) const
    {
	    return IsProcessorValid(Processor) && ((static_cast<KAFFINITY>(1) << Processor) & _activeProcessorMask) != 0;
    }

    bool IsProcessorValid(BYTE Processor) const
    {
	    return Processor < _maximumProcessorCount;
    }
};

class ProcessorTopology
{
public:
    vector<ProcessorGroupInformation> _vProcessorGroupInformation;
    DWORD _ulProcCount;
    DWORD _ulActiveProcCount;

    ProcessorTopology()
    {
	    DWORD ReturnedLength = 1024;
	    auto pInformation = reinterpret_cast<PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX>(new char[ReturnedLength]);

        BOOL fResult = GetLogicalProcessorInformationEx(RelationGroup, pInformation, &ReturnedLength);
        if (!fResult && GetLastError() == ERROR_INSUFFICIENT_BUFFER)
        {
            delete [] pInformation;
            pInformation = reinterpret_cast<PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX>(new char[ReturnedLength]);
            fResult = GetLogicalProcessorInformationEx(RelationGroup, pInformation, &ReturnedLength);
        }

        if (fResult)
        {
            // Group information comes back as a single (large) element, not an array.
            assert(ReturnedLength == pInformation->Size);
            _ulProcCount = 0;
            _ulActiveProcCount = 0;

            // Fill in group topology vector so we can answer questions about active/max procs
            for (WORD i = 0; i < pInformation->Group.ActiveGroupCount; i++)
            {
                _vProcessorGroupInformation.emplace_back(
                    pInformation->Group.GroupInfo[i].MaximumProcessorCount,
                    pInformation->Group.GroupInfo[i].ActiveProcessorCount,
                    i,
                    pInformation->Group.GroupInfo[i].ActiveProcessorMask
                    );
                _ulProcCount += _vProcessorGroupInformation[i]._maximumProcessorCount;
                _ulActiveProcCount += _vProcessorGroupInformation[i]._activeProcessorCount;
            }
        }

        delete [] pInformation;
    }

    bool IsGroupValid(WORD Group) const
    {
	    return Group < _vProcessorGroupInformation.size();
    }

    // Return the next active processor in the system, exclusive (Next = true)
    // or inclusive (Next = false) of the input group/processor.
    // Iteration is in order of absolute processor number.
    // This does assume at least one core is active, but that is a given.
    void GetActiveGroupProcessor(WORD& Group, BYTE& Processor, bool Next)
    {
        if (Next)
        {
            Processor++;
        }

        while (!_vProcessorGroupInformation[Group].IsProcessorActive(Processor))
        {
            if (!_vProcessorGroupInformation[Group].IsProcessorValid(Processor))
            {
                Processor = 0;
                if (!IsGroupValid(++Group))
                {
                    Group = 0;
                }
            }
            else
            {
                Processor++;
            }
        }
    }
};


class SystemInformation
{
private:
    SYSTEMTIME StartTime;

public:
    string sComputerName;
    ProcessorTopology processorTopology;

    SystemInformation() : StartTime({})
    {
        // System Name
        char buffer[64];
        DWORD cb = _countof(buffer);

#pragma prefast(suppress:38020, "Yes, we're aware this is an ANSI API in a UNICODE project")
	    const BOOL fResult = GetComputerNameExA(ComputerNamePhysicalDnsHostname, buffer, &cb);
        if (fResult)
        {
            sComputerName = buffer;
        }

        // capture start time
        GetSystemTime(&StartTime);
    }

    // for unit test, squelch variable timestamp
    void ResetTime()
    {
        StartTime = { 0 };
    }

    string GetXml() const
    {
        char szBuffer[64]; // enough for 64bit mask (17ch) and timestamp
        int nWritten;
        string sXml("<System>\n");

        // identify computer which ran the test
        sXml += "<ComputerName>";
        sXml += sComputerName;
        sXml += "</ComputerName>\n";

        // identify tool version which performed the test
        sXml += "<Tool>\n";
        sXml += "<Version>" DISKSPD_NUMERIC_VERSION_STRING "</Version>\n";
        sXml += "<VersionDate>" DISKSPD_DATE_VERSION_STRING "</VersionDate>\n";
        sXml += "</Tool>\n";

        sXml += "<RunTime>";
        if (StartTime.wYear) {

            nWritten = sprintf_s(szBuffer, _countof(szBuffer),
                "%u/%02u/%02u %02u:%02u:%02u GMT",
                StartTime.wYear,
                StartTime.wMonth,
                StartTime.wDay,
                StartTime.wHour,
                StartTime.wMinute,
                StartTime.wSecond);
            assert(nWritten && nWritten < _countof(szBuffer));
            sXml += szBuffer;
        }
        sXml += "</RunTime>\n";

        // processor topology
        sXml += "<ProcessorTopology>\n";
        for (const auto& g : processorTopology._vProcessorGroupInformation)
        {
            sXml += "<Group Group=\"";
            sXml += to_string(g._groupNumber);
            sXml += "\" MaximumProcessors=\"";
            sXml += to_string(g._maximumProcessorCount);
            sXml += "\" ActiveProcessors=\"";
            sXml += to_string(g._activeProcessorCount);
            sXml += "\" ActiveProcessorMask=\"0x";
            nWritten = sprintf_s(szBuffer, _countof(szBuffer), "%Ix", g._activeProcessorMask);
            assert(nWritten && nWritten < _countof(szBuffer));
            sXml += szBuffer;
            sXml += "\"/>\n";

        }
        sXml += "</ProcessorTopology>\n";

        sXml += "</System>\n";

        return sXml;
    }
};

extern SystemInformation g_SystemInformation;

struct Synchronization
{
    ULONG ulStructSize;     //size of the structure that the caller is aware of (to easier achieve backward compatibility in a future)
    HANDLE hStopEvent;      //an event to be signalled if the scenario is to be stop before time ellapses
    HANDLE hStartEvent;     //an event for signalling start
    CALLBACK_TEST_STARTED pfnCallbackTestStarted;   //a function to be called if the measured test is about to start
    CALLBACK_TEST_FINISHED pfnCallbackTestFinished; //a function to be called as soon as the measrued test finishes
};

#define STRUCT_SYNCHRONIZATION_SUPPORTS(pSynch, Field) ( \
    (NULL != (pSynch)) && \
    ((pSynch)->ulStructSize >= offsetof(struct Synchronization, Field) + sizeof((pSynch)->Field)) \
    )

// caching modes
// cached -> default (-Sb explicitly)
// disableoscache  -> no_intermediate_buffering (-S or -Su)
// disablelocalcache -> cached, but then tear down local rdr cache (-Sr)
enum class TargetCacheMode {
    Undefined = 0,
    Cached,
    DisableOSCache,
    DisableLocalCache
};

// writethrough modes
// off -> default
// on -> (-Sw or implied with -Sh == -Suw/-Swu)
enum class WriteThroughMode {
    Undefined = 0,
    Off,
    On,
};

class ThreadTarget
{
public:

    ThreadTarget() :
        _ulThread(0xFFFFFFFF),
        _ulWeight(0)
    {
    }

    void SetThread(UINT32 ulThread) { _ulThread = ulThread; }
    UINT32 GetThread() const { return _ulThread; }

    void SetWeight(UINT32 ulWeight) { _ulWeight = ulWeight; }
    UINT32 GetWeight() const { return _ulWeight; }

    string GetXml() const;

private:
    UINT32 _ulThread;
    UINT32 _ulWeight;
};

class Target
{
public:

    Target() :
        _dwBlockSize(64 * 1024),
        _dwRequestCount(2),
        _ullBlockAlignment(64 * 1024),
        _fBlockAlignmentValid(false),
        _fUseRandomAccessPattern(false),
        _ullBaseFileOffset(0),
        _fParallelAsyncIO(false),
        _fInterlockedSequential(false),
        _cacheMode(TargetCacheMode::Cached),
        _writeThroughMode(WriteThroughMode::Off),
        _fZeroWriteBuffers(false),
        _dwThreadsPerFile(1),
        _ullThreadStride(0),
        _fCreateFile(false),
        _fPrecreated(false),
        _ullFileSize(0),
        _ullMaxFileSize(0),
        _ulWriteRatio(0),
        _fUseBurstSize(false),
        _dwBurstSize(0),
        _dwThinkTime(0),
        _fThinkTime(false),
        _dwThroughputBytesPerMillisecond(0),
        _fSequentialScanHint(false),
        _fRandomAccessHint(false),
        _fTemporaryFileHint(false),
        _fUseLargePages(false),
        _cbRandomDataWriteBuffer(0),
		_sRandomDataWriteBufferSourcePath(string()),
        _pRandomDataWriteBuffer(nullptr),
        _ioPriorityHint(IoPriorityHintNormal),
        _ulWeight(1)
    {
    }

    void SetPath(const string& sPath) { _sPath = sPath; }
    string GetPath() const { return _sPath; }

    void SetBlockSizeInBytes(DWORD dwBlockSize) { _dwBlockSize = dwBlockSize; }
    DWORD GetBlockSizeInBytes() const { return _dwBlockSize; }

    void SetBlockAlignmentInBytes(UINT64 ullBlockAlignment)
    {
        _ullBlockAlignment = ullBlockAlignment;
        _fBlockAlignmentValid = true;
    }

    UINT64 GetBlockAlignmentInBytes() const
    {
        return _fBlockAlignmentValid ? _ullBlockAlignment : _dwBlockSize;
    }
    
    void SetUseRandomAccessPattern(bool fBool) { _fUseRandomAccessPattern = fBool; }
    bool GetUseRandomAccessPattern() const { return _fUseRandomAccessPattern; } 

    void SetBaseFileOffsetInBytes(UINT64 ullBaseFileOffset) { _ullBaseFileOffset = ullBaseFileOffset; }
    UINT64 GetBaseFileOffsetInBytes() const { return _ullBaseFileOffset; }
    UINT64 GetThreadBaseFileOffsetInBytes(UINT32 ulThreadNo) const { return _ullBaseFileOffset + ulThreadNo * _ullThreadStride; }

    void SetSequentialScanHint(bool fBool) { _fSequentialScanHint = fBool; }
    bool GetSequentialScanHint() const { return _fSequentialScanHint; }

    void SetRandomAccessHint(bool fBool) { _fRandomAccessHint = fBool; }
    bool GetRandomAccessHint() const { return _fRandomAccessHint; }

    void SetTemporaryFileHint(bool fBool) { _fTemporaryFileHint = fBool; }
    bool GetTemporaryFileHint() const { return _fTemporaryFileHint; }

    void SetUseLargePages(bool fBool) { _fUseLargePages = fBool; }
    bool GetUseLargePages() const { return _fUseLargePages; }

    void SetRequestCount(DWORD dwRequestCount) { _dwRequestCount = dwRequestCount; }
    DWORD GetRequestCount() const { return _dwRequestCount; }

    void SetCacheMode(TargetCacheMode cacheMode) { _cacheMode = cacheMode; }
    TargetCacheMode GetCacheMode() const { return _cacheMode;  }

    void SetWriteThroughMode(WriteThroughMode writeThroughMode ) { _writeThroughMode = writeThroughMode; }
    WriteThroughMode GetWriteThroughMode( ) const { return _writeThroughMode; }

    void SetZeroWriteBuffers(bool fBool) { _fZeroWriteBuffers = fBool; }
    bool GetZeroWriteBuffers() const { return _fZeroWriteBuffers; }

    void SetRandomDataWriteBufferSize(UINT64 cbWriteBuffer) { _cbRandomDataWriteBuffer = cbWriteBuffer; }
    UINT64 GetRandomDataWriteBufferSize() const { return _cbRandomDataWriteBuffer; }

    void SetRandomDataWriteBufferSourcePath(const string& sPath) { _sRandomDataWriteBufferSourcePath = sPath; }
    string GetRandomDataWriteBufferSourcePath() const { return _sRandomDataWriteBufferSourcePath; }

    void SetUseBurstSize(bool fBool) { _fUseBurstSize = fBool; }
    bool GetUseBurstSize() const { return _fUseBurstSize; }

    void SetBurstSize(DWORD dwBurstSize) { _dwBurstSize = dwBurstSize; }
    DWORD GetBurstSize() const { return _dwBurstSize; }

    void SetThinkTime(DWORD dwThinkTime) { _dwThinkTime = dwThinkTime; }
    DWORD GetThinkTime() const { return _dwThinkTime; }

    void SetEnableThinkTime(bool fBool)   { _fThinkTime = fBool; }
    bool GetEnableThinkTime() const { return _fThinkTime; }

    void SetThreadsPerFile(DWORD dwThreadsPerFile) { _dwThreadsPerFile = dwThreadsPerFile; }
    DWORD GetThreadsPerFile() const { return _dwThreadsPerFile; }

    void SetCreateFile(bool fBool) { _fCreateFile = fBool; }
    bool GetCreateFile() const { return _fCreateFile; }

    void SetFileSize(UINT64 ullFileSize) { _ullFileSize = ullFileSize; }
    UINT64 GetFileSize() const { return _ullFileSize; } // TODO: InBytes

    void SetMaxFileSize(UINT64 ullMaxFileSize) { _ullMaxFileSize = ullMaxFileSize; }
    UINT64 GetMaxFileSize() const { return _ullMaxFileSize; }

    void SetWriteRatio(UINT32 ulWriteRatio) { _ulWriteRatio = ulWriteRatio; }
    UINT32 GetWriteRatio() const { return _ulWriteRatio; }

    void SetUseParallelAsyncIO(bool fBool) { _fParallelAsyncIO = fBool; }
    bool GetUseParallelAsyncIO() const { return _fParallelAsyncIO; }
    
    void SetUseInterlockedSequential(bool fBool) { _fInterlockedSequential = fBool; }
    bool GetUseInterlockedSequential() const { return _fInterlockedSequential; }

    void SetThreadStrideInBytes(UINT64 ullThreadStride) { _ullThreadStride = ullThreadStride; }
    UINT64 GetThreadStrideInBytes() const { return _ullThreadStride; }

    void SetIOPriorityHint(PRIORITY_HINT _hint)
    {
        assert(_hint < MaximumIoPriorityHintType);
        _ioPriorityHint = _hint;
    }
    PRIORITY_HINT GetIOPriorityHint() const { return _ioPriorityHint; }

    void SetWeight(UINT32 ulWeight) { _ulWeight = ulWeight; }
    UINT32 GetWeight() const { return _ulWeight; }

    void AddThreadTarget(const ThreadTarget &threadTarget) 
    {
        _vThreadTargets.push_back(threadTarget);
    }
    vector<ThreadTarget> GetThreadTargets() const { return _vThreadTargets; }

    void SetPrecreated(bool fBool) { _fPrecreated = fBool; }
    bool GetPrecreated() const { return _fPrecreated; }

    void SetThroughput(DWORD dwThroughputBytesPerMillisecond) { _dwThroughputBytesPerMillisecond = dwThroughputBytesPerMillisecond; }
    DWORD GetThroughputInBytesPerMillisecond() const { return _dwThroughputBytesPerMillisecond; }

    string GetXml() const;

    bool AllocateAndFillRandomDataWriteBuffer(Random *pRand);
    void FreeRandomDataWriteBuffer();
    BYTE* GetRandomDataWriteBuffer(Random *pRand) const;

    DWORD GetCreateFlags(bool fAsync) const
    {
        DWORD dwFlags = FILE_ATTRIBUTE_NORMAL;

        if (GetSequentialScanHint())
        {
            dwFlags |= FILE_FLAG_SEQUENTIAL_SCAN;
        }

        if (GetRandomAccessHint())
        {
            dwFlags |= FILE_FLAG_RANDOM_ACCESS;
        }

        if (GetTemporaryFileHint())
        {
            dwFlags |= FILE_ATTRIBUTE_TEMPORARY;
        }

        if (fAsync)
        {
            dwFlags |= FILE_FLAG_OVERLAPPED;
        }

        if (GetCacheMode() == TargetCacheMode::DisableOSCache)
        {
            dwFlags |= FILE_FLAG_NO_BUFFERING;
        }

        if (GetWriteThroughMode( ) == WriteThroughMode::On)
        {
            dwFlags |= FILE_FLAG_WRITE_THROUGH;
        }

        return dwFlags;
    }

private:
    string _sPath;
    DWORD _dwBlockSize;
    DWORD _dwRequestCount;      // TODO: change the name to something more descriptive (OutstandingRequestCount?)

    UINT64 _ullBlockAlignment;
    bool _fBlockAlignmentValid;
    bool _fUseRandomAccessPattern;
 
    UINT64 _ullBaseFileOffset;
    bool _fParallelAsyncIO;
    bool _fInterlockedSequential;

    TargetCacheMode _cacheMode;
    WriteThroughMode _writeThroughMode;
    bool _fZeroWriteBuffers;
    DWORD _dwThreadsPerFile;
    UINT64 _ullThreadStride;

    bool _fCreateFile;
    bool _fPrecreated;          // used to track which files have been created before the first timespan and which have to be created later
    UINT64 _ullFileSize;
    UINT64 _ullMaxFileSize;

    UINT32 _ulWriteRatio;
    bool _fUseBurstSize;    // TODO: "use" or "enable"?; since burst size must be specified with the think time, one variable should be sufficient
    DWORD _dwBurstSize;     // number of IOs in a burst
    DWORD _dwThinkTime;     // time to pause before issuing the next burst of IOs
    // TODO: could this be removed by using _dwThinkTime==0?
    bool _fThinkTime;       //variable to decide whether to think between IOs (default is false)
    DWORD _dwThroughputBytesPerMillisecond; // set to 0 to disable throttling

    bool _fSequentialScanHint;      // open file with the FILE_FLAG_SEQUENTIAL_SCAN hint
    bool _fRandomAccessHint;        // open file with the FILE_FLAG_RANDOM_ACCESS hint
    bool _fTemporaryFileHint;       // open file with the FILE_ATTRIBUTE_TEMPORARY hint
    bool _fUseLargePages;           // Use large pages for IO buffers

    UINT64 _cbRandomDataWriteBuffer;            // if > 0, then the write buffer should be filled with random data
    string _sRandomDataWriteBufferSourcePath;   // file that should be used for filling the write buffer (if the path is not available, use a crypto provider)
    BYTE *_pRandomDataWriteBuffer;              // a buffer used for write data when _cbWriteBuffer > 0; it's shared by all the threads working on this target

    PRIORITY_HINT _ioPriorityHint;

    UINT32 _ulWeight;
    vector<ThreadTarget> _vThreadTargets;

    bool _FillRandomDataWriteBuffer(Random *pRand) const;

    friend class UnitTests::ProfileUnitTests;
    friend class UnitTests::TargetUnitTests;
};

class AffinityAssignment
{
public:
    WORD wGroup;
    BYTE bProc;

    AffinityAssignment() = delete;
    AffinityAssignment(WORD p_wGroup, BYTE p_bProc) :
        wGroup(p_wGroup),
        bProc(p_bProc)
    {
    }
};

class TimeSpan
{
public:
    TimeSpan() :
        _ulDuration(10),
        _ulWarmUp(5),
        _ulCoolDown(0),
        _ulRandSeed(0),
        _dwThreadCount(0),
        _dwRequestCount(0),
        _fRandomWriteData(false),
        _fDisableAffinity(false),
        _fCompletionRoutines(false),
        _fMeasureLatency(false),
        _fCalculateIopsStdDev(false),
        _ulIoBucketDurationInMilliseconds(1000)
    {
    }

    void ClearAffinityAssignment()
    {
        _vAffinity.clear();
    }
    void AddAffinityAssignment(WORD wGroup, BYTE bProc)
    {
        _vAffinity.emplace_back(wGroup, bProc);
    }
    const auto& GetAffinityAssignments() const { return _vAffinity; }

    void AddTarget(const Target& target)
    {
        _vTargets.push_back(Target(target));
    }

    vector<Target> GetTargets() const { return _vTargets; }

    void SetDuration(UINT32 ulDuration) { _ulDuration = ulDuration; }
    UINT32 GetDuration() const { return _ulDuration; }

    void SetWarmup(UINT32 ulWarmup) { _ulWarmUp = ulWarmup; }
    UINT32 GetWarmup() const { return _ulWarmUp; }

    void SetCooldown(UINT32 ulCooldown) { _ulCoolDown = ulCooldown; }
    UINT32 GetCooldown() const { return _ulCoolDown; }

    void SetRandSeed(UINT32 ulRandSeed) { _ulRandSeed = ulRandSeed; }
    UINT32 GetRandSeed() const { return _ulRandSeed; }

    void SetRandomWriteData(bool fRandomWriteData) { _fRandomWriteData = fRandomWriteData; }
    bool GetRandomWriteData() const { return _fRandomWriteData; }

    void SetThreadCount(DWORD dwThreadCount) { _dwThreadCount = dwThreadCount; }
    DWORD GetThreadCount() const { return _dwThreadCount; }

    void SetRequestCount(DWORD dwRequestCount) { _dwRequestCount = dwRequestCount; }
    DWORD GetRequestCount() const { return _dwRequestCount; }

    void SetDisableAffinity(bool fDisableAffinity) { _fDisableAffinity = fDisableAffinity; }
    bool GetDisableAffinity() const { return _fDisableAffinity; }

    void SetCompletionRoutines(bool fCompletionRoutines) { _fCompletionRoutines = fCompletionRoutines; }
    bool GetCompletionRoutines() const { return _fCompletionRoutines; }
    
    void SetMeasureLatency(bool fMeasureLatency) { _fMeasureLatency = fMeasureLatency; }
    bool GetMeasureLatency() const { return _fMeasureLatency; }

    void SetCalculateIopsStdDev(bool fCalculateStdDev) { _fCalculateIopsStdDev = fCalculateStdDev; }
    bool GetCalculateIopsStdDev() const { return _fCalculateIopsStdDev; }

    void SetIoBucketDurationInMilliseconds(UINT32 ulIoBucketDurationInMilliseconds) { _ulIoBucketDurationInMilliseconds = ulIoBucketDurationInMilliseconds; }
    UINT32 GetIoBucketDurationInMilliseconds() const { return _ulIoBucketDurationInMilliseconds; }
    
    string GetXml() const;
    void MarkFilesAsPrecreated(const vector<string>& vFiles);

private:
    vector<Target> _vTargets;
    UINT32 _ulDuration;
    UINT32 _ulWarmUp;
    UINT32 _ulCoolDown;
    UINT32 _ulRandSeed;
    DWORD _dwThreadCount;
    DWORD _dwRequestCount;
    bool _fRandomWriteData;
    bool _fDisableAffinity;
    vector<AffinityAssignment> _vAffinity;
    bool _fCompletionRoutines;
    bool _fMeasureLatency;
    bool _fCalculateIopsStdDev;
    UINT32 _ulIoBucketDurationInMilliseconds;

    friend class UnitTests::ProfileUnitTests;
};

enum class ResultsFormat
{
    Text,
    Xml
};

enum class PrecreateFiles
{
    None,
    UseMaxSize,
    OnlyFilesWithConstantSizes,
    OnlyFilesWithConstantOrZeroSizes
};

class Profile
{
public:
    Profile() :
        _fVerbose(false),
        _dwProgress(0),
        _resultsFormat(ResultsFormat::Text),
        _precreateFiles(PrecreateFiles::None),
        _fEtwEnabled(false),
        _fEtwProcess(false),
        _fEtwThread(false),
        _fEtwImageLoad(false),
        _fEtwDiskIO(false),
        _fEtwMemoryPageFaults(false),
        _fEtwMemoryHardFaults(false),
        _fEtwNetwork(false),
        _fEtwRegistry(false),
        _fEtwUsePagedMemory(false),
        _fEtwUsePerfTimer(false),
        _fEtwUseSystemTimer(false),
        _fEtwUseCyclesCounter(false)
    {
    }

    void ClearTimeSpans()
    {
        _vTimeSpans.clear();
    }

    void AddTimeSpan(const TimeSpan& timeSpan)
    {
        _vTimeSpans.push_back(TimeSpan(timeSpan));
    }

    const vector<TimeSpan>& GetTimeSpans() const { return _vTimeSpans; }

    void SetVerbose(bool b) { _fVerbose = b; }
    bool GetVerbose() const { return _fVerbose; }

    void SetProgress(DWORD dwProgress) { _dwProgress = dwProgress; }
    DWORD GetProgress() const { return _dwProgress; }

    void SetCmdLine(const string& sCmdLine) { _sCmdLine = sCmdLine; }
    string GetCmdLine() const { return _sCmdLine; };

    void SetResultsFormat(ResultsFormat format) { _resultsFormat = format; }
    ResultsFormat GetResultsFormat() const { return _resultsFormat; }

    void SetPrecreateFiles(PrecreateFiles c) { _precreateFiles = c; }
    PrecreateFiles GetPrecreateFiles() const { return _precreateFiles; }

    //ETW
    void SetEtwEnabled(bool b)          { _fEtwEnabled = b; }
    void SetEtwProcess(bool b)          { _fEtwProcess = b; }
    void SetEtwThread(bool b)           { _fEtwThread = b; }
    void SetEtwImageLoad(bool b)        { _fEtwImageLoad = b; }
    void SetEtwDiskIO(bool b)           { _fEtwDiskIO = b; }
    void SetEtwMemoryPageFaults(bool b) { _fEtwMemoryPageFaults = b; }
    void SetEtwMemoryHardFaults(bool b) { _fEtwMemoryHardFaults = b; }
    void SetEtwNetwork(bool b)          { _fEtwNetwork = b; }
    void SetEtwRegistry(bool b)         { _fEtwRegistry = b; }
    void SetEtwUsePagedMemory(bool b)   { _fEtwUsePagedMemory = b; }
    void SetEtwUsePerfTimer(bool b)     { _fEtwUsePerfTimer = b; }
    void SetEtwUseSystemTimer(bool b)   { _fEtwUseSystemTimer = b; }
    void SetEtwUseCyclesCounter(bool b) { _fEtwUseCyclesCounter = b; }

    bool GetEtwEnabled() const          { return _fEtwEnabled; }
    bool GetEtwProcess() const          { return _fEtwProcess; }
    bool GetEtwThread() const           { return _fEtwThread; }
    bool GetEtwImageLoad() const        { return _fEtwImageLoad; }
    bool GetEtwDiskIO() const           { return _fEtwDiskIO; }
    bool GetEtwMemoryPageFaults() const { return _fEtwMemoryPageFaults; }
    bool GetEtwMemoryHardFaults() const { return _fEtwMemoryHardFaults; }
    bool GetEtwNetwork() const          { return _fEtwNetwork; }
    bool GetEtwRegistry() const         { return _fEtwRegistry; }
    bool GetEtwUsePagedMemory() const   { return _fEtwUsePagedMemory; }
    bool GetEtwUsePerfTimer() const     { return _fEtwUsePerfTimer; }
    bool GetEtwUseSystemTimer() const   { return _fEtwUseSystemTimer; }
    bool GetEtwUseCyclesCounter() const { return _fEtwUseCyclesCounter; }

    string GetXml() const;
    bool Validate(bool fSingleSpec, SystemInformation *pSystem = nullptr) const;
    void MarkFilesAsPrecreated(const vector<string>& vFiles);

	Profile(const Profile& T) = delete;

private:
    vector<TimeSpan>_vTimeSpans;
    bool _fVerbose;
    DWORD _dwProgress;
    string _sCmdLine;
    ResultsFormat _resultsFormat;
    PrecreateFiles _precreateFiles;

    //ETW
    bool _fEtwEnabled;
    bool _fEtwProcess;
    bool _fEtwThread;
    bool _fEtwImageLoad;
    bool _fEtwDiskIO;
    bool _fEtwMemoryPageFaults;
    bool _fEtwMemoryHardFaults;
    bool _fEtwNetwork;
    bool _fEtwRegistry;
    bool _fEtwUsePagedMemory;
    bool _fEtwUsePerfTimer;
    bool _fEtwUseSystemTimer;
    bool _fEtwUseCyclesCounter;

    friend class UnitTests::ProfileUnitTests;
};

class IORequest
{
public:
    explicit IORequest(Random *pRand) :
		_overlapped({}),
        _ullTotalWeight(0),
		_fEqualWeights(true),
		_pRand(pRand),
		_pCurrentTarget(nullptr),
		_ioType(IOOperation::ReadIO),
		_ullStartTime(0),
		_ulRequestIndex(0xFFFFFFFF)
    {
        memset(&_overlapped, 0, sizeof(OVERLAPPED));
        _overlapped.Offset = 0xFFFFFFFF;
        _overlapped.OffsetHigh = 0xFFFFFFFF;
    }

    static IORequest *OverlappedToIORequest(OVERLAPPED *pOverlapped)
    {
        return CONTAINING_RECORD(pOverlapped, IORequest, _overlapped);
    }

    OVERLAPPED *GetOverlapped() { return &_overlapped; }
    
    void AddTarget(Target *pTarget, UINT32 ulWeight)
    { 
        _vTargets.push_back(pTarget); 
        _vulTargetWeights.push_back(ulWeight);
        _ullTotalWeight += ulWeight;

        if (ulWeight != _vulTargetWeights[0]) {
            _fEqualWeights = false;
        }
    }

    Target *GetCurrentTarget() const { return _pCurrentTarget; }

    Target *GetNextTarget()
    {
	    if (_vTargets.size() == 1) {
            _pCurrentTarget = _vTargets[0];
        }
        else if (_fEqualWeights) {
            _pCurrentTarget = _vTargets[_pRand->Rand32() % _vTargets.size()];
        }
        else {
            UINT64 ullWeight = _pRand->Rand64() % _ullTotalWeight;
            
            for (int iTarget = 0; iTarget < _vTargets.size(); iTarget++) {
                if (ullWeight < _vulTargetWeights[iTarget]) {
                    _pCurrentTarget = _vTargets[iTarget];
                    break;
                }
            
                ullWeight -= _vulTargetWeights[iTarget];
            }
        }

        return _pCurrentTarget;
    }

    void SetIoType(IOOperation ioType) { _ioType = ioType; }
    IOOperation GetIoType() const { return _ioType; }

    void SetStartTime(UINT64 ullStartTime) { _ullStartTime = ullStartTime; }
    UINT64 GetStartTime() const { return _ullStartTime; }

    void SetRequestIndex(UINT32 ulRequestIndex) { _ulRequestIndex = ulRequestIndex; }
    UINT32 GetRequestIndex() const { return _ulRequestIndex; }

private:
    OVERLAPPED _overlapped{};
    vector<Target*> _vTargets;
    vector<UINT32> _vulTargetWeights;
    UINT64 _ullTotalWeight;
    bool _fEqualWeights;
    Random *_pRand;
    Target *_pCurrentTarget;
    IOOperation _ioType;
    UINT64 _ullStartTime;
    UINT32 _ulRequestIndex;
};

class ThreadParameters
{
public:
    ThreadParameters() :
		pProfile(nullptr),
		pTimeSpan(nullptr),
		pullSharedSequentialOffsets(nullptr),
		pRand(nullptr),
		ulRandSeed(0),
		ulThreadNo(0),
		ulRelativeThreadNo(0),
		pfAccountingOn(nullptr),
		pullStartTime(nullptr),
		pResults(nullptr),
		dwIOCnt(0),
		wGroupNum(0),
		bProcNum(0),
		hStartEvent(nullptr),
		hEndEvent(nullptr)
	{
	}

    const Profile *pProfile;
    const TimeSpan *pTimeSpan;

    vector<Target> vTargets;
    vector<HANDLE> vhTargets;
    vector<UINT64> vullFileSizes;
    vector<size_t> vulReadBufferSize;
    vector<BYTE *> vpDataBuffers;
    vector<IORequest> vIORequest;
    vector<ThroughputMeter> vThroughputMeters;
  
    // For vanilla sequential access (-s):
    // Private per-thread offsets, incremented directly, indexed to number of targets
    vector<UINT64> vullPrivateSequentialOffsets; 

    // For interlocked sequential access (-si):
    // Pointers to offsets shared between threads, incremented with an interlocked op
    UINT64* pullSharedSequentialOffsets;

    Random *pRand;

    UINT32 ulRandSeed;
    UINT32 ulThreadNo;
    UINT32 ulRelativeThreadNo;

    // accounting
    volatile bool *pfAccountingOn;
    PUINT64 pullStartTime;
    ThreadResults *pResults;

    //progress dots
    DWORD dwIOCnt;

    //group affinity
    WORD wGroupNum;
    DWORD bProcNum;

    HANDLE hStartEvent;

    // TODO: check how it's used
    HANDLE hEndEvent;        //used only in case of completion routines (not for IO Completion Ports)
    
    bool AllocateAndFillBufferForTarget(const Target& target);
    BYTE* GetReadBuffer(size_t iTarget, size_t iRequest);
    BYTE* GetWriteBuffer(size_t iTarget, size_t iRequest);
    DWORD GetTotalRequestCount() const;

	ThreadParameters(const ThreadParameters& T) = delete;
};

class IResultParser
{
public:
	virtual ~IResultParser() = default;
	virtual string ParseResults(Profile& profile, const SystemInformation& system, vector<Results> vResults) = 0;
};
