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
#include <assert.h>
#include "Histogram.h"
#include "IoBucketizer.h"

using namespace std;

/// structures used for passing the input parameters

// versioning material. for simplicity in consumption, please ensure that the date string
// parses via the System.Datetime constructor as follows (in Powershell):
//
//      [datetime] "string"
//
// this should result in a valid System.Datetime object, rendered like:
//
//      Monday, June 16, 2014 12:00:00 AM

#define DISKSPD_RELEASE_TAG ""
#define DISKSPD_NUMERIC_VERSION_STRING "2.0.15" DISKSPD_RELEASE_TAG
#define DISKSPD_DATE_VERSION_STRING "2015/01/09"

typedef void (WINAPI *PRINTF)(const char*, va_list);                            //function used for displaying formatted data (printf style)

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

    static double PerfTimeToMicroseconds(const double);
    static double PerfTimeToMilliseconds(const double);
    static double PerfTimeToSeconds(const double);
    static double PerfTimeToMicroseconds(const UINT64);
    static double PerfTimeToMilliseconds(const UINT64);
    static double PerfTimeToSeconds(const UINT64);

    static UINT64 MicrosecondsToPerfTime(const double);
    static UINT64 MillisecondsToPerfTime(const double);
    static UINT64 SecondsToPerfTime(const double);

private:

    static const UINT64 TIMER_FREQ;
    static UINT64 _GetPerfTimerFreq();

    friend class UnitTests::PerfTimerUnitTests;
};

struct PercentileDescriptor
{
    double Percentile;
    string Name;
};

class Util
{
public:
    static string DoubleToStringHelper(const double);
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
             PUINT64 pullIoStartTime,
             PUINT64 pullSpanStartTime,
             bool fMeasureLatency,
             bool fCalculateIopsStdDev
             )
    {
        float fDurationMsec = 0;
        UINT64 ullEndTime = 0;
        // assume it is worthwhile to stay off of the time query path unless needed (micro-overhead)
        if (fMeasureLatency || fCalculateIopsStdDev)
        {
            ullEndTime = PerfTimer::GetTime();
        }

        if (fMeasureLatency)
        {
            UINT64 ullDuration = ullEndTime - *pullIoStartTime;
            fDurationMsec = static_cast<float>(PerfTimer::PerfTimeToMicroseconds(ullDuration));

            if (type == IOOperation::ReadIO)
            {
                readLatencyHistogram.Add(fDurationMsec);
            }
            else
            {
                writeLatencyHistogram.Add(fDurationMsec);
            }
        }

        UINT64 ullRelativeCompletionTime = 0;
        if (fCalculateIopsStdDev)
        {
            ullRelativeCompletionTime = ullEndTime - *pullSpanStartTime;

            if (type == IOOperation::ReadIO)
            {
                readBucketizer.Add(ullRelativeCompletionTime);
            }
            else
            {
                writeBucketizer.Add(ullRelativeCompletionTime);
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

typedef void (*CALLBACK_TEST_STARTED)();    //callback function to notify that the measured test is about to start
typedef void (*CALLBACK_TEST_FINISHED)();   //callback function to notify that the measured test has just finished

class SystemInformation
{
public:
    string sComputerName;

    SystemInformation()
    {
        char buffer[64];
        DWORD cb = _countof(buffer);
        BOOL fResult;

#pragma prefast(suppress:38020, "Yes, we're aware this is an ANSI API in a UNICODE project")
        fResult = GetComputerNameExA(ComputerNamePhysicalDnsHostname, buffer, &cb);
        if (fResult)
        {
            sComputerName = buffer;
        }
    }

    string SystemInformation::GetXml() const
    {
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

        sXml += "</System>\n";

        return sXml;
    }
};

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
        _fDisableOSCache(false),
        _fDisableAllCache(false),
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
        _fSequentialScanHint(false),
        _fRandomAccessHint(false),
        _fUseLargePages(false),
        _ioPriorityHint(IoPriorityHintNormal),
        _dwThroughputBytesPerMillisecond(0),
        _cbRandomDataWriteBuffer(0),
        _sRandomDataWriteBufferSourcePath(),
        _pRandomDataWriteBuffer(nullptr)
    {
    }

    void SetPath(string sPath) { _sPath = sPath; }
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
    
    void SetUseRandomAccessPattern(bool fUseRandomAccessPattern) { _fUseRandomAccessPattern = fUseRandomAccessPattern; }
    bool GetUseRandomAccessPattern() const { return _fUseRandomAccessPattern; } 

    void SetBaseFileOffsetInBytes(UINT64 ullBaseFileOffset) { _ullBaseFileOffset = ullBaseFileOffset; }
    UINT64 GetBaseFileOffsetInBytes() const { return _ullBaseFileOffset; }

    void SetSequentialScanHint(bool fSequentialScanHint) { _fSequentialScanHint = fSequentialScanHint; }
    bool GetSequentialScanHint() const { return _fSequentialScanHint; }

    void SetRandomAccessHint(bool fRandomAccessHint) { _fRandomAccessHint = fRandomAccessHint; }
    bool GetRandomAccessHint() const { return _fRandomAccessHint; }

    void SetUseLargePages(bool fUseLargePages) { _fUseLargePages = fUseLargePages; }
    bool GetUseLargePages() const { return _fUseLargePages; }

    void SetRequestCount(DWORD dwRequestCount) { _dwRequestCount = dwRequestCount; }
    DWORD GetRequestCount() const { return _dwRequestCount; }

    void SetDisableOSCache(bool fDisableOSCache) { _fDisableOSCache = fDisableOSCache; }
    bool GetDisableOSCache() const { return _fDisableOSCache; }

    void SetDisableAllCache(bool fDisableAllCache) { _fDisableAllCache = fDisableAllCache; }
    bool GetDisableAllCache() const { return _fDisableAllCache; }

    void SetZeroWriteBuffers(bool fZeroWriteBuffers) { _fZeroWriteBuffers = fZeroWriteBuffers; }
    bool GetZeroWriteBuffers() const { return _fZeroWriteBuffers; }

    void SetRandomDataWriteBufferSize(UINT64 cbWriteBuffer) { _cbRandomDataWriteBuffer = cbWriteBuffer; }
    UINT64 GetRandomDataWriteBufferSize(void) const { return _cbRandomDataWriteBuffer; }

    void SetRandomDataWriteBufferSourcePath(string sPath) { _sRandomDataWriteBufferSourcePath = sPath; }
    string GetRandomDataWriteBufferSourcePath() const { return _sRandomDataWriteBufferSourcePath; }

    void SetUseBurstSize(bool fUseBurstSize) { _fUseBurstSize = fUseBurstSize; }
    bool GetUseBurstSize() const { return _fUseBurstSize; }

    void SetBurstSize(DWORD dwBurstSize) { _dwBurstSize = dwBurstSize; }
    DWORD GetBurstSize() const { return _dwBurstSize; }

    void SetThinkTime(DWORD dwThinkTime) { _dwThinkTime = dwThinkTime; }
    DWORD GetThinkTime() const { return _dwThinkTime; }

    void SetEnableThinkTime(bool fEnable)   { _fThinkTime = fEnable; }
    bool GetEnableThinkTime() const { return _fThinkTime; }

    void SetThreadsPerFile(DWORD dwThreadsPerFile) { _dwThreadsPerFile = dwThreadsPerFile; }
    DWORD GetThreadsPerFile() const { return _dwThreadsPerFile; }

    void SetCreateFile(bool fCreateFile) { _fCreateFile = fCreateFile; }
    bool GetCreateFile() const { return _fCreateFile; }

    void SetFileSize(UINT64 ullFileSize) { _ullFileSize = ullFileSize; }
    UINT64 GetFileSize() const { return _ullFileSize; } // TODO: InBytes

    void SetMaxFileSize(UINT64 ullMaxFileSize) { _ullMaxFileSize = ullMaxFileSize; }
    UINT64 GetMaxFileSize() const { return _ullMaxFileSize; }

    void SetWriteRatio(UINT32 ulWriteRatio) { _ulWriteRatio = ulWriteRatio; }
    UINT32 GetWriteRatio() const { return _ulWriteRatio; }

    void SetUseParallelAsyncIO(bool fParallelAsyncIO) { _fParallelAsyncIO = fParallelAsyncIO; }
    bool GetUseParallelAsyncIO() const { return _fParallelAsyncIO; }
    
    void SetUseInterlockedSequential(bool fInterlockedSequential) { _fInterlockedSequential = fInterlockedSequential; }
    bool GetUseInterlockedSequential() const { return _fInterlockedSequential; }

    void SetThreadStrideInBytes(UINT64 ullThreadStride) { _ullThreadStride = ullThreadStride; }
    UINT64 GetThreadStrideInBytes() const { return _ullThreadStride; }

    void SetIOPriorityHint(PRIORITY_HINT _hint)
    {
        assert(_hint < MaximumIoPriorityHintType);
        _ioPriorityHint = _hint;
    }
    PRIORITY_HINT GetIOPriorityHint() const { return _ioPriorityHint; }

    void SetPrecreated(bool fPrecreated) { _fPrecreated = fPrecreated; }
    bool GetPrecreated() const { return _fPrecreated; }

    void SetThroughput(DWORD dwThroughputBytesPerMillisecond) { _dwThroughputBytesPerMillisecond = dwThroughputBytesPerMillisecond; }
    DWORD GetThroughputInBytesPerMillisecond() const { return _dwThroughputBytesPerMillisecond; }

    string GetXml() const;

    bool AllocateAndFillRandomDataWriteBuffer();
    void FreeRandomDataWriteBuffer();
    BYTE* GetRandomDataWriteBuffer();

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
    bool _fDisableOSCache;
    bool _fDisableAllCache;
    bool _fZeroWriteBuffers;
    DWORD _dwThreadsPerFile;
    UINT64 _ullThreadStride;

    bool _fCreateFile;
    bool _fPrecreated;      // used to track which files have been created before the first timespan and which have to be created later
    UINT64 _ullFileSize;
    UINT64 _ullMaxFileSize;
    UINT32 _ulWriteRatio;
    bool _fUseBurstSize;    // TODO: "use" or "enable"?; since burst size must be specified with the think time, one variable should be sufficient
    DWORD _dwBurstSize;     // number of IOs in a burst
    DWORD _dwThinkTime;     // time to pause before issuing the next burst of IOs
    // TODO: could this be removed by using _dwThinkTime==0?
    bool _fThinkTime;       //variable to decide whether to think between IOs (default is false)
    DWORD _dwThroughputBytesPerMillisecond; // set to 0 to disable throttling

    bool _fSequentialScanHint;          // open file with the FILE_FLAG_SEQUENTIAL_SCAN hint
    bool _fRandomAccessHint;            // open file with the FILE_FLAG_RANDOM_ACCESS hint
    bool _fUseLargePages;           // Use large pages for IO buffers

    UINT64 _cbRandomDataWriteBuffer;            // if > 0, then the write buffer should be filled with random data
    string _sRandomDataWriteBufferSourcePath;   // file that should be used for filling the write buffer (if the path is not available, use a crypto provider)
    BYTE *_pRandomDataWriteBuffer;              // a buffer used for write data when _cbWriteBuffer > 0; it's shared by all the threads working on this target

    PRIORITY_HINT _ioPriorityHint;

    bool _FillRandomDataWriteBuffer();

    friend class UnitTests::ProfileUnitTests;
    friend class UnitTests::TargetUnitTests;
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
        _fGroupAffinity(false),
        _fDisableAffinity(false),
        _fCompletionRoutines(false),
        _fMeasureLatency(false),
        _fCalculateIopsStdDev(false),
        _ulIoBucketDurationInMilliseconds(1000)
    {
    }

    void AddAffinityAssignment(UINT32 ulAffinity)
    {
        _vAffinity.push_back(ulAffinity);
    }
    vector<UINT32> GetAffinityAssignments() const { return _vAffinity; }

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

    void SetThreadCount(DWORD dwThreadCount) { _dwThreadCount = dwThreadCount; }
    DWORD GetThreadCount() const { return _dwThreadCount; }

    void SetGroupAffinity(bool fGroupAffinity) { _fGroupAffinity = fGroupAffinity; }
    bool GetGroupAffinity() const { return _fGroupAffinity; }

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
    void MarkFilesAsPrecreated(const vector<string> vFiles);

private:
    vector<Target> _vTargets;
    UINT32 _ulDuration;
    UINT32 _ulWarmUp;
    UINT32 _ulCoolDown;
    UINT32 _ulRandSeed;
    DWORD _dwThreadCount;
    bool _fGroupAffinity;
    bool _fDisableAffinity;
    vector<UINT32> _vAffinity;
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
        _fEtwUseCyclesCounter(false),
        _resultsFormat(ResultsFormat::Text),
        _precreateFiles(PrecreateFiles::None)
    {
    }

    void AddTimeSpan(const TimeSpan& timeSpan)
    {
        _vTimeSpans.push_back(TimeSpan(timeSpan));
    }

    const vector<TimeSpan>& GetTimeSpans() const { return _vTimeSpans; }

    void SetVerbose(bool fVerbose) { _fVerbose = fVerbose; }
    bool GetVerbose() const { return _fVerbose; }

    void SetProgress(DWORD dwProgress) { _dwProgress = dwProgress; }
    DWORD GetProgress() const { return _dwProgress; }

    void SetCmdLine(string sCmdLine) { _sCmdLine = sCmdLine; }
    string GetCmdLine() const { return _sCmdLine; };

    void SetResultsFormat(ResultsFormat format) { _resultsFormat = format; }
    ResultsFormat GetResultsFormat() const { return _resultsFormat; }

    void SetPrecreateFiles(PrecreateFiles c) { _precreateFiles = c; }
    PrecreateFiles GetPrecreateFiles() const { return _precreateFiles; }

    //ETW
    void SetEtwEnabled(bool fEtwEnabled)                    { _fEtwEnabled = fEtwEnabled; }
    void SetEtwProcess(bool fEtwProcess)                    { _fEtwProcess = fEtwProcess; }
    void SetEtwThread(bool fEtwThread)                      { _fEtwThread = fEtwThread; }
    void SetEtwImageLoad(bool fEtwImageLoad)                { _fEtwImageLoad = fEtwImageLoad; }
    void SetEtwDiskIO(bool fEtwDiskIO)                      { _fEtwDiskIO = fEtwDiskIO; }
    void SetEtwMemoryPageFaults(bool fEtwMemoryPageFaults)  { _fEtwMemoryPageFaults = fEtwMemoryPageFaults; }
    void SetEtwMemoryHardFaults(bool fEtwMemoryHardFaults)  { _fEtwMemoryHardFaults = fEtwMemoryHardFaults; }
    void SetEtwNetwork(bool fEtwNetwork)                    { _fEtwNetwork = fEtwNetwork; }
    void SetEtwRegistry(bool fEtwRegistry)                  { _fEtwRegistry = fEtwRegistry; }
    void SetEtwUsePagedMemory(bool fEtwUsePagedMemory)      { _fEtwUsePagedMemory = fEtwUsePagedMemory; }
    void SetEtwUsePerfTimer(bool fEtwUsePerfTimer)          { _fEtwUsePerfTimer = fEtwUsePerfTimer; }
    void SetEtwUseSystemTimer(bool fEtwUseSystemTimer)      { _fEtwUseSystemTimer = fEtwUseSystemTimer; }
    void SetEtwUseCyclesCounter(bool fEtwUseCyclesCounter)  { _fEtwUseCyclesCounter = fEtwUseCyclesCounter; }

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
    bool Validate(bool fSingleSpec) const;
    void MarkFilesAsPrecreated(const vector<string> vFiles);

private:
    Profile(const Profile& T);

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

class ThreadParameters
{
public:
    ThreadParameters() :
        pProfile(nullptr),
        pTimeSpan(nullptr),
        pullSharedSequentialOffsets(nullptr),
        ulRandSeed(0),
        ulThreadNo(0),
        ulRelativeThreadNo(0)
    {
    }

    const Profile *pProfile;
    const TimeSpan *pTimeSpan;

    vector<Target> vTargets;
    vector<HANDLE> vhTargets;
    vector<UINT64> vullFileSizes;
    vector<BYTE *> vpDataBuffers;
    vector<OVERLAPPED> vOverlapped;             // each target has RequestCount OVERLAPPED structures
    vector<size_t> vOverlappedIdToTargetId;
    vector<size_t> vFirstOverlappedIdForTargetId;   //id of the first overlapped structure in the vOverlapped vector by target
    vector<IOOperation> vdwIoType;                        //as many as vOverlapped; used by the completion routines
    vector<UINT64> vIoStartTimes;
  
    // For vanilla sequential access (-s):
    // Private per-thread offsets, incremented directly, indexed to number of targets
    vector<UINT64> vullPrivateSequentialOffsets; 

    // For interlocked sequential access (-si):
    // Pointers to offsets shared between threads, incremented with an interlocked op
    UINT64* pullSharedSequentialOffsets;

    UINT32 ulRandSeed;
    UINT32 ulThreadNo;
    UINT32 ulRelativeThreadNo;

    // accounting
    volatile bool *pfAccountingOn;
    PUINT64 pullStartTime;
    ThreadResults *pResults;

    //group affinity
    WORD wGroupNum;
    DWORD dwProcNum;
    GROUP_AFFINITY GroupAffinity;

    HANDLE hStartEvent;

    // TODO: check how it's used
    HANDLE hEndEvent;        //used only in case of completion routines (not for IO Completion Ports)
    
    bool AllocateAndFillBufferForTarget(const Target& target);
    BYTE* GetReadBuffer(size_t iTarget, size_t iRequest);
    BYTE* GetWriteBuffer(size_t iTarget, size_t iRequest);
    DWORD GetTotalRequestCount() const;

private:
    ThreadParameters(const ThreadParameters& T);
};

class IResultParser
{
public:
    virtual string ParseResults(Profile& profile, const SystemInformation& system, vector<Results> vResults) = 0;
};
