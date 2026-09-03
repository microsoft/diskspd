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
#include <powersetting.h>
#include <powrprof.h>
#include <VersionHelpers.h>
#include <TraceLoggingProvider.h>
#include <TraceLoggingActivity.h>
#include <evntrace.h>
#include <ctime>
#include <vector>
#include <algorithm>
#include <map>
#include <set>
#include <locale>
#include <codecvt>
#include <Winternl.h>   //ntdll.dll
#include <assert.h>
#include "IoRingWrapper.h"
#include "Histogram.h"
#include "IoBucketizer.h"
#include "ThroughputMeter.h"
#include "Version.h"

using namespace std;

TRACELOGGING_DECLARE_PROVIDER(g_hEtwProvider);

#define DISKSPD_TRACE_INFO      0x00000000
#define DISKSPD_TRACE_RESERVED  0x00000001
#define DISKSPD_TRACE_IO        0x00000100

typedef void (WINAPI *PRINTF)(const char*, va_list);                            //function used for displaying formatted data (printf style)

#define ROUND_DOWN(_x,_alignment) \
    ( ((_x)/(_alignment)) * (_alignment) )

#define ROUND_UP(_x,_alignment) \
    ROUND_DOWN((_x) + (_alignment) - 1, (_alignment))

#define DECLARE_DISABLE_COPY(xClass) \
    xClass(xClass const &) = delete; \
    xClass & operator=(xClass const &) = delete;

template <class T>
class CScopeGuardF
{
public:
    CScopeGuardF(T const & function) : _function(function), _fDismissed(false) {}
    explicit CScopeGuardF(CScopeGuardF && other) noexcept
        :
        _function(std::move(other._function)),
        _fDismissed(other._fDismissed)
    {
        other.Dismiss();
    }
    CScopeGuardF const & operator=(CScopeGuardF && other) noexcept
    {
        _function = std::move(other._function);
        _fDismissed = other._fDismissed;
        other.Dismiss();
        return *this;
    }

    ~CScopeGuardF() { Execute(); }
    void Execute() noexcept
    {
        if (!_fDismissed)
        {
            try {
                _fDismissed = true;
                _function();
            }
            catch (...)
            {
                // If the function throws an exception, terminate (noexcept)
                std::terminate();
            }
        }
    }

    void Dismiss() noexcept { _fDismissed = true; }

private:
    T                   _function;
    bool                _fDismissed;

    DECLARE_DISABLE_COPY(CScopeGuardF);
};

template <class T>
CScopeGuardF<T> make_sg(T const & function)
{
    return CScopeGuardF<T>(function);
}

#define TB (((UINT64)1)<<40)
#define GB (((UINT64)1)<<30)
#define MB (((UINT64)1)<<20)
#define KB (((UINT64)1)<<10)

#define EXPERIMENT_TPUT_CALC        0x1 // precise ms sleep calculation for low rate throughput control
extern ULONG g_ExperimentFlags;

// Default and maximum number of IO completions to dequeue per
// batched GetQueuedCompletionStatusEx call. The default starts at the maximum;
// -oc allows users to reduce the batch depth for experimental purposes.
constexpr DWORD c_defaultCompletionDepth = 16;
constexpr DWORD c_maximumCompletionDepth = 16;

// Maximum processor number within a group, determined by KAFFINITY bit width.
constexpr DWORD c_maxCpuIndexPerGroup = sizeof(KAFFINITY) * 8 - 1;

// Maximum outstanding IO request count per target (or globally via -O).
// Capped so that requestCount * batchSizePercent stays within UINT32 range
// for IoRing batch size calculation. 65536 outstanding IOs is far beyond
// any rational workload.
constexpr DWORD c_maximumRequestCount = 65536;

// Allocate a buffer with optional alignment. If alignment > 0, uses
// VirtualAlloc2 with a MEM_ADDRESS_REQUIREMENTS extended parameter.
// If alignment is 0, uses plain VirtualAlloc.
BYTE* AllocateAlignedBuffer(size_t cb, DWORD alignment);

//
// Thread-safe diagnostic output. Initialize() must be called once at startup.
// SetVerbose() controls whether PrintVerbose() produces output.
// All output is serialized through a critical section to prevent interleaving
// when multiple threads print concurrently.
//

class Diagnostics
{
public:
    static void Initialize();
    static void SetVerbose(bool fVerbose) { s_fVerbose = fVerbose; }
    static bool GetVerbose() { return s_fVerbose; }

    static void PrintError(const char *format, ...);
    static void PrintVerbose(const char *format, ...);

private:
    static CRITICAL_SECTION s_cs;
    static bool s_fVerbose;
    static bool s_fInitialized;
};

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
    class ThreadParametersUnitTests;
    class IORequestGeneratorUnitTests;
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

template <typename T1, typename T2>
class Range
{
public:
    Range(
        T1 Source,
        T1 Span,
        T2 Dest
    ) :
        _src(Source),
        _span(Span),
        _dst(Dest)
    {}

    constexpr bool operator<(const Range<T1, T2>& other) const
    {
        //
        // This is used for comparison of effective distributions during result reporting (dedup).
        //
        // A hole with _span == 0 sorts < range with _span > 0
        // Note that a hole will never match in a find().
        //

        return _src < other._src ||
                (_src == other._src &&
                    (_span < other._span ||
                    (_span == other._span && _dst < other._dst)));
    }

    static Range<T1, T2> const * find(const vector<Range<T1, T2>>& v, T1 c)
    {
        // v must be sorted
        size_t s = 0, mid, e = v.size() - 1;

        while (true)
        {
            mid = s + ((e - s) / 2);
            if (c < v[mid]._src) {
                if (s == mid)
                {
                    return nullptr;
                }
                e = mid - 1;
            }
            else if (c > v[mid]._src + v[mid]._span - 1)
            {
                if (e == mid)
                {
                    return nullptr;
                }
                s = mid + 1;
            }
            else
            {
                return &v[mid];
            }
        }
    }

    T1 _src, _span;
    T2 _dst;
};

//
// A DistributionRange maps an IO percentage range to a target range.
//
//   _src:          starting IO% for this range (0-based cumulative)
//   _span:         IO% width of this range (0 = hole, no IO issued here)
//   _dst.first:    target range start (% for Percent, bytes for Absolute/finalized)
//   _dst.second:   target range length (% for Percent, bytes for Absolute/finalized;
//                  0 in last position = open end, resolved at Finalize)
//

typedef Range<UINT32, pair<UINT64, UINT64>> DistributionRange;

enum class DistributionType
{
    None,
    Absolute,
    Percent
};

enum class BufferSeparation
{
    SystemDefault,
    PDECacheLine
};

enum class AffinityTraversal
{
    Unspecified,            // Internal: not yet set by user (resolves to Cpu)
    CoreAware,              // Core-first pigeon-hole assignment (-ac)
    Cpu                     // Direct CPU order, no core-aware reordering (default)
};

enum class AffinityGroupSpan
{
    Unspecified,            // Internal: not yet set by user (resolves to Fill)
    Fill,                   // Fill each scope unit before moving to next (default)
    Span                    // Span across all scope units (-as)
};

enum class AffinityEfficiencyOrder
{
    Unspecified,            // Internal: not yet set by user (resolves to PFirst)
    Unordered,              // No efficiency ordering (-aup)
    PFirst,                 // P-cores before E-cores within each pass (default, -ap)
    EFirst,                 // E-cores before P-cores within each pass (-ae)
    FillPFirst,             // All P-core passes before any E-core (-aP)
    FillEFirst              // All E-core passes before any P-core (-aE)
};

class Distribution
{
public:
    Distribution() : _type(DistributionType::None), _ioSpan(100) {}

    //
    // Set the stated distribution from command line parsing.
    // Places the final tail element if IO% < 100.
    //
    void Set(const vector<DistributionRange>& v, DistributionType t);

    //
    // Finalize the distribution against target geometry.
    // Resolves percent/absolute ranges to aligned byte offsets.
    // relTargetSizeAligned: max aligned offset for IO
    // relTargetSize: actual usable target size (for absolute end-of-target)
    // blockSize: IO block size
    // blockAlignment: IO alignment
    //
    void Finalize(UINT64 relTargetSizeAligned, UINT64 relTargetSize,
                  UINT32 blockSize, UINT64 blockAlignment);

    // Rendering
    string GetText(UINT32 indent) const;
    string GetXml(UINT32 indent, bool fRenderHoles = false) const;

    //
    // Validate the stated distribution. Returns true if valid.
    // Reports errors to stderr.
    //
    bool Validate(UINT32 blockSize) const;

    // Accessors
    DistributionType GetType() const { return _type; }
    const vector<DistributionRange>& GetRanges() const { return _vRanges; }
    UINT32 GetIOSpan() const { return _ioSpan; }
    bool IsEmpty() const { return _type == DistributionType::None; }
    bool HasRanges() const { return !_vRanges.empty(); }

private:
    DistributionType _type;
    vector<DistributionRange> _vRanges;  // stated or finalized ranges
    UINT32 _ioSpan;                      // total IO% span (100 normally, < 100 for trimmed absolute)
};

//
// This code implements Bob Jenkins public domain simple random number generator
// See http://burtleburtle.net/bob/rand/smallprng.html for details
//

class Random
{
public:
    Random(UINT64 ulSeed = 0);

    inline UINT64 Rand64()
    {
        UINT64 e;

        e =           _ulState[0] - _rotl64(_ulState[1], 7);
        _ulState[0] = _ulState[1] ^ _rotl64(_ulState[2], 13);
        _ulState[1] = _ulState[2] + _rotl64(_ulState[3], 37);
        _ulState[2] = _ulState[3] + e;
        _ulState[3] = e + _ulState[0];

        return _ulState[3];
    }

    inline UINT32 Rand32()
    {
        return (UINT32)Rand64();
    }

    void RandBuffer(BYTE *pBuffer, UINT32 ulLength, bool fPseudoRandomOkay);

private:
    UINT64 _ulState[4];
};

struct PercentileDescriptor
{
    double Percentile;
    string Name;
};

namespace Util
{
    string DoubleToStringHelper(const double);

    template<typename T> T QuotientCeiling(T dividend, T divisor)
    {
        return (dividend + divisor - 1) / divisor;
    }

    // True if result is <= ratio.
    // The ratio is on the interval [0, 100]:
    //  0 will never occur (always false)
    //  100 will always occur (always true)

    inline bool BooleanRatio(Random *pRand, UINT32 ulRatio)
    {
        return ((pRand->Rand32() % 100 + 1) <= ulRatio);
    }

    //
    // This is close to strtoul[l], returning the next character to parse in the input string.
    // This character can be used for validation (should there be any non-integer remaining),
    // interpreting units that follow the integer (KMGTB), or parsing further (int[<sep><more>])
    // content in the string.
    //
    // Return value indicates whether any integers were parsed to Output. Continue is only modified
    // on success, and will point to the terminator on completion. False is returned on overflow.
    //

    template<typename T>
    bool ParseUInt(const char* Input, T& Output, const char*& Continue)
    {
        T current = 0, last = 0;
        const char* input = Input;
        bool parsed = false;

        while (*input)
        {
            if (*input < '0' || *input > '9')
            {
                break;
            }

            parsed = true;
            current *= 10;
            current += static_cast<T>(*input) - static_cast<T>('0');

            //
            // Overflow?
            //

            if (current < last)
            {
                parsed = false;
                break;
            }
            last = current;

            input += 1;
        }

        //
        // Return if string was consumed
        //
        //

        if (parsed)
        {
            Continue = input;
            Output = current;
        }

        return parsed;
    }

    //
    // Format a power-of-2 size as a human-readable string (e.g., "32KiB", "1.25MiB").
    // Uses IEC binary suffixes (KiB, MiB, GiB, TiB, PiB) and "B" for sizes below 1KiB.
    // Trailing fractional zeros are stripped (e.g., "1.5KiB" not "1.50KiB").
    //

    string GetSizeKMGT(UINT64 size);

    //
    // Compute the buffer alignment size for the given separation mode.
    // Returns 0 for SystemDefault (no alignment).
    // For PDECacheLine, the alignment ensures that each allocation
    // begins on a VA boundary such that no two allocations share the cache
    // lines holding their PTE page pointers (PDEs).
    //
    // The alignment is: (CacheLineSize / MMPTE_SIZE) * (PageSize / MMPTE_SIZE) * PageSize
    //   - PageSize / MMPTE_SIZE = PTEs per PTE page (e.g. 4096/8 = 512)
    //   - CacheLineSize / MMPTE_SIZE = PDE pointers per cache line (e.g. 64/8 = 8)
    //   - Each PTE page covers PTEsPerPage * PageSize of VA (e.g. 512 * 4K = 2MiB)
    //   - A cache line of PDEs covers 8 * 2MiB = 16MiB
    //
    // MMPTE_SIZE is 8 bytes on all current Windows platforms: x64/arm64 natively and x86
    // under PAE (default since XP SP2 / Server 2003 SP1 for DEP). If future
    // platforms change the PTE size, this constant must be updated.
    //
    // pageSize: system page size from GetNativeSystemInfo
    // cacheLineSize: largest cache line from ProcessorTopology (0 falls back to 64)
    //

    DWORD GetBufferAlignmentSize(BufferSeparation mode, DWORD pageSize, WORD cacheLineSize);

    // Format a ULONG_PTR bitmask as ranges of set bits: "0-3,7,10-12"
    string MaskRanges(ULONG_PTR mask);

    // Format a sorted vector of WORD values as compressed ranges: {0,1,2} -> "0-2"
    string IntRanges(const vector<WORD>& values);

    // Trim and collapse whitespace in-place: strip leading/trailing whitespace,
    // collapse internal runs of spaces/tabs to a single space
    inline void ShrinkContiguousWhitespace(string& s)
    {
        auto dst = s.begin();
        bool prevSpace = true; // true to strip leading whitespace
        for (auto src = s.begin(); src != s.end(); ++src)
        {
            if (*src == ' ' || *src == '\t')
            {
                // copy exactly the first space for a run
                if (!prevSpace)
                {
                    *dst++ = ' ';
                }
                prevSpace = true;
            }
            else
            {
                // copy non-space character iff dst/src have diverged due to shrinking
                if (dst != src)
                {
                    *dst = *src;
                }
                ++dst;
                prevSpace = false;
            }
        }

        // reject final trailing space?
        if (dst != s.begin() && *(dst - 1) == ' ')
        {
            --dst;
        }

        // shrink the string to the trimmed length
        s.resize(dst - s.begin());
    }
};

// To keep track of which type of IO was issued
enum class IOOperation
{
    Unknown = 0,
    ReadIO,
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

    void Add(
        DWORD dwBytesTransferred,
        IOOperation type,
        UINT64 ullIoStartTime,
        UINT64 ullIoEndTime,
        UINT64 ullSpanStartTime,
        bool fMeasureLatency,
        bool fCalculateIopsStdDev
        )
    {
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

        // end time is 0 if we're not measuring latency
        assert(((fMeasureLatency || fCalculateIopsStdDev) && ullIoEndTime != 0) ||
               (!fMeasureLatency && !fCalculateIopsStdDev));

        if (ullIoEndTime == 0)
        {
            return;
        }

        UINT64 ullDuration = ullIoEndTime - ullIoStartTime;;
        double lfDurationUsec = PerfTimer::PerfTimeToMicroseconds(ullDuration);

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
            UINT64 ullRelativeCompletionTime = ullIoEndTime - ullSpanStartTime;

            if (type == IOOperation::ReadIO)
            {
                readBucketizer.Add(ullRelativeCompletionTime, lfDurationUsec);
            }
            else
            {
                writeBucketizer.Add(ullRelativeCompletionTime, lfDurationUsec);
            }
        }
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

    // Effective distribution after applying to target size (if specified/non-empty)
    Distribution distribution;
};

// Number of completion-count buckets in WAIT_STATS.
// Bucket 0 = zero completions, 1 = one, ... (N-1) = (N-1) or more.
constexpr DWORD c_nCompletionBuckets = 8;

typedef struct _WAIT_STATS {
    ULONGLONG Wait;
    ULONGLONG ThrottleWait;
    ULONGLONG ThrottleSleep;
    ULONGLONG Lookaside;
    ULONGLONG WaitCompletion[c_nCompletionBuckets];       // completions per regular wait
    ULONGLONG LookasideCompletion[c_nCompletionBuckets];  // completions per lookaside
    bool fThrottled;                   // true if this thread had throttled targets
} WAIT_STATS;

class ThreadResults
{
public:
    ThreadResults()
    {
        WaitStats = { 0 };
    }

    void AddIoRingSubmitCount(UINT64 ullCount)
    {
        ullSubmitCount += ullCount;
    }

    WAIT_STATS WaitStats;
    vector<TargetResults> vTargetResults;
    UINT64 ullSubmitCount = 0;      //number of submits performed by IoRing per thread
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

class ProcessorGroupInformation
{
public:
    WORD _groupNumber;
    BYTE _maximumProcessorCount;
    BYTE _activeProcessorCount;
    KAFFINITY _activeProcessorMask;

    ProcessorGroupInformation() = delete;
    ProcessorGroupInformation(
        WORD Group,
        BYTE MaximumProcessorCount,
        BYTE ActiveProcessorCount,
        KAFFINITY ActiveProcessorMask) :
        _groupNumber(Group),
        _maximumProcessorCount(MaximumProcessorCount),
        _activeProcessorCount(ActiveProcessorCount),
        _activeProcessorMask(ActiveProcessorMask)
    {
    }

    ProcessorGroupInformation(
        WORD Group,
        PROCESSOR_GROUP_INFO& GroupInfo) :
        _groupNumber(Group),
        _maximumProcessorCount(GroupInfo.MaximumProcessorCount),
        _activeProcessorCount(GroupInfo.ActiveProcessorCount),
        _activeProcessorMask(GroupInfo.ActiveProcessorMask)
    {
    }

    // This logic is strictly unaware that sparse processor masks are not possible;
    // address this later, not important. See comments around RelationGroup query.
    bool IsProcessorActive(BYTE Processor) const
    {
        return (IsProcessorValid(Processor) &&
                (((KAFFINITY)1 << Processor) & _activeProcessorMask) != 0);
    }

    bool IsProcessorValid(BYTE Processor) const
    {
        return (Processor < _maximumProcessorCount);
    }
};

class ProcessorNumaInformation
{
public:
    DWORD _ulProcCount;
    DWORD _nodeNumber;
    vector<pair<WORD, KAFFINITY>> _vProcessorMasks;
};

class ProcessorCoreInformation
{
public:
    WORD _groupNumber;
    KAFFINITY _processorMask;
    BYTE _efficiencyClass;
    WORD _groupCoreNumber;

    ProcessorCoreInformation() = delete;
    ProcessorCoreInformation(
        WORD Group,
        KAFFINITY ProcessorMask,
        BYTE EfficiencyClass) :
        _groupNumber(Group),
        _processorMask(ProcessorMask),
        _efficiencyClass(EfficiencyClass),
        _groupCoreNumber(0)
    {
    }
};

class ProcessorSocketInformation
{
public:
    DWORD _ulProcCount;
    DWORD _ulSocketNumber;
    vector<pair<WORD, KAFFINITY>> _vProcessorMasks;
};

class ProcessorCacheInformation
{
public:
    BYTE _level;            // 1, 2, or 3 (L1/L2/L3)
    BYTE _associativity;    // 0xFF = fully associative
    WORD _lineSize;         // bytes
    DWORD _cacheSize;       // bytes
    PROCESSOR_CACHE_TYPE _type; // CacheUnified, CacheInstruction, CacheData, CacheTrace
    vector<pair<WORD, KAFFINITY>> _processorMasks; // group+mask pairs sharing this cache

    ProcessorCacheInformation() = delete;
    ProcessorCacheInformation(
        BYTE Level,
        BYTE Associativity,
        WORD LineSize,
        DWORD CacheSize,
        PROCESSOR_CACHE_TYPE Type) :
        _level(Level),
        _associativity(Associativity),
        _lineSize(LineSize),
        _cacheSize(CacheSize),
        _type(Type)
    {
    }

    static const char* TypeName(PROCESSOR_CACHE_TYPE type)
    {
        switch (type)
        {
        case CacheUnified:     return "Unified";
        case CacheInstruction: return "Instruction";
        case CacheData:        return "Data";
        case CacheTrace:       return "Trace";
        default:               return "Unknown";
        }
    }

    static const char* TypeAbbreviation(PROCESSOR_CACHE_TYPE type)
    {
        switch (type)
        {
        case CacheUnified:     return "";
        case CacheInstruction: return "i";
        case CacheData:        return "d";
        case CacheTrace:       return "t";
        default:               return "?";
        }
    }

    bool SameGeometry(const ProcessorCacheInformation& other) const
    {
        return _level == other._level &&
               _type == other._type &&
               _cacheSize == other._cacheSize &&
               _lineSize == other._lineSize &&
               _associativity == other._associativity;
    }
};

class ProcessorTopology
{
public:
    vector<ProcessorGroupInformation> _vProcessorGroupInformation;
    vector<ProcessorNumaInformation> _vProcessorNumaInformation;
    vector<ProcessorSocketInformation> _vProcessorSocketInformation;
    vector<ProcessorCoreInformation> _vProcessorCoreInformation;
    vector<ProcessorCacheInformation> _vProcessorCacheInformation;

    DWORD _ulProcessorCount;            // total number of (active) processors
    BYTE _ubPerformanceEfficiencyClass; // highest performance class present
    bool _fSMT;                         // any SMT cores present

    ProcessorTopology()
    {
        BOOL fResult;
        PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX pInformation;
        DWORD AllocSize = 1024;
        DWORD ReturnedLength = AllocSize;
        LOGICAL_PROCESSOR_RELATIONSHIP NumaRelation;
        pInformation = (PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX) new char[AllocSize];

        _ulProcessorCount = 0;
        _ubPerformanceEfficiencyClass = 0;
        _fSMT = false;

        ////
        // Group Relations
        ////

        fResult = GetLogicalProcessorInformationEx(RelationGroup, pInformation, &ReturnedLength);
        if (!fResult && GetLastError() == ERROR_INSUFFICIENT_BUFFER)
        {
            delete [] pInformation;
            AllocSize = ReturnedLength;
            pInformation = (PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX) new char[AllocSize];
            fResult = GetLogicalProcessorInformationEx(RelationGroup, pInformation, &ReturnedLength);
        }

        if (fResult)
        {
            // Group information comes back as a single (large) element, not an array.
            assert(ReturnedLength == pInformation->Size);

            //
            // Fill in group topology vector
            //
            // Note: maximum processor count has no utility other than an indication of the
            // bit width of the KAFFINITY mask that might have set values. But:
            //
            //   1) any mask will be a contiguous run of set bits (no sparse holes); there is
            //      no case where a 0 bit will be present to indicate a gap/disabled processor
            //   2) all system APIs (such as the cpu utilization query) are defined over active
            //      processors
            //
            // There are (new?) cases where maximum is represented as > active on large systems,
            // which makes these distinctions critical... active processor count is the only
            // count that matters.
            //
            // For the sake of documentation we do save & report out the masks as reported by the
            // system, but the only ones we look at are limited to cases where we get information
            // in the form of GROUP_AFFINITY, which is just group # and mask (like NUMA and package
            // association).
            //

            for (WORD i = 0; i < pInformation->Group.ActiveGroupCount; i++)
            {
                _vProcessorGroupInformation.emplace_back(
                    i,
                    pInformation->Group.GroupInfo[i]
                    );

                _ulProcessorCount += _vProcessorGroupInformation[i]._activeProcessorCount;
            }
        }

        ////
        // NUMA Relations
        ////

        //
        // Dynamically detect the available NUMA relations. Non-Ex returns exactly one relation and
        // does not define the GroupCount field. Ex scales to return multiple groups for large systems
        // with > 64 per NUMA domain and does populate GroupCount.
        //

        NumaRelation = RelationNumaNodeEx;
        ReturnedLength = AllocSize;
        fResult = GetLogicalProcessorInformationEx(NumaRelation, pInformation, &ReturnedLength);
        if (!fResult && GetLastError() == ERROR_GEN_FAILURE)
        {
            NumaRelation = RelationNumaNode;
            fResult = GetLogicalProcessorInformationEx(NumaRelation, pInformation, &ReturnedLength);
        }
        if (!fResult && GetLastError() == ERROR_INSUFFICIENT_BUFFER)
        {
            delete [] pInformation;
            AllocSize = ReturnedLength;
            pInformation = (PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX) new char[AllocSize];
            fResult = GetLogicalProcessorInformationEx(NumaRelation, pInformation, &ReturnedLength);
        }

        if (fResult)
        {
            PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX cur = pInformation;

            while (ReturnedLength > 0)
            {
                ProcessorNumaInformation node;

                assert(ReturnedLength >= cur->Size);

                if (cur->Size > ReturnedLength)
                {
                    break;
                }

                node._nodeNumber = cur->NumaNode.NodeNumber;
                node._ulProcCount = 0;
                for (WORD i = 0; i < (NumaRelation == RelationNumaNode ? 1 : cur->NumaNode.GroupCount); i++)
                {
                    node._ulProcCount += ProcessorTopology::MaskCount(cur->NumaNode.GroupMasks[i].Mask);
                    node._vProcessorMasks.emplace_back(cur->NumaNode.GroupMasks[i].Group,
                                                       cur->NumaNode.GroupMasks[i].Mask);
                }

                _vProcessorNumaInformation.push_back(node);

                ReturnedLength -= cur->Size;
                cur = (PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX)((PCHAR)cur + cur->Size);
            }
        }

        ////
        // Socket/Package Relations
        ////

        ReturnedLength = AllocSize;
        fResult = GetLogicalProcessorInformationEx(RelationProcessorPackage, pInformation, &ReturnedLength);
        if (!fResult && GetLastError() == ERROR_INSUFFICIENT_BUFFER)
        {
            delete [] pInformation;
            AllocSize = ReturnedLength;
            pInformation = (PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX) new char[AllocSize];
            fResult = GetLogicalProcessorInformationEx(RelationProcessorPackage, pInformation, &ReturnedLength);
        }

        if (fResult)
        {
            PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX cur = pInformation;

            DWORD socketNumber = 0;
            while (ReturnedLength != 0)
            {
                ProcessorSocketInformation socket;

                assert(ReturnedLength >= cur->Size);

                if (cur->Size > ReturnedLength)
                {
                    break;
                }

                socket._ulProcCount = 0;
                socket._ulSocketNumber = socketNumber;
                for (WORD i = 0; i < cur->Processor.GroupCount; i++)
                {
                    socket._ulProcCount += ProcessorTopology::MaskCount(cur->Processor.GroupMask[i].Mask);
                    socket._vProcessorMasks.emplace_back(cur->Processor.GroupMask[i].Group,
                                                         cur->Processor.GroupMask[i].Mask);
                }

                _vProcessorSocketInformation.push_back(socket);

                ReturnedLength -= cur->Size;
                cur = (PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX)((PCHAR)cur + cur->Size);
                socketNumber += 1;
            }
        }

        ////
        // Core Relations
        ////

        ReturnedLength = AllocSize;
        fResult = GetLogicalProcessorInformationEx(RelationProcessorCore, pInformation, &ReturnedLength);
        if (!fResult && GetLastError() == ERROR_INSUFFICIENT_BUFFER)
        {
            delete [] pInformation;
            AllocSize = ReturnedLength;
            pInformation = (PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX) new char[AllocSize];
            fResult = GetLogicalProcessorInformationEx(RelationProcessorCore, pInformation, &ReturnedLength);
        }

        //
        // The EfficiencyClass member was added with Windows 10
        //

        BOOL fEfficiencyClass = false;
        if (IsWindows10OrGreater())
        {
            fEfficiencyClass = true;
        }

        if (fResult)
        {
            PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX cur = pInformation;
            BYTE curEfficiency;

            while (ReturnedLength != 0)
            {
                assert(ReturnedLength >= cur->Size);

                if (cur->Size > ReturnedLength)
                {
                    break;
                }

                //
                // Determine the highest performance core class and presence of SMT as we sweep.
                // Note that SMT is per core and can be asymmetric.
                //

                if (fEfficiencyClass)
                {
                    curEfficiency = cur->Processor.EfficiencyClass;
                    if (_ubPerformanceEfficiencyClass < curEfficiency)
                    {
                        _ubPerformanceEfficiencyClass = curEfficiency;
                    }
                }

                if (cur->Processor.Flags & LTP_PC_SMT)
                {
                    _fSMT = true;
                }

                assert(pInformation->Processor.GroupCount == 1);

                _vProcessorCoreInformation.emplace_back(cur->Processor.GroupMask[0].Group,
                                                        cur->Processor.GroupMask[0].Mask,
                                                        fEfficiencyClass ? cur->Processor.EfficiencyClass : (BYTE)0);

                ReturnedLength -= cur->Size;
                cur = (PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX)((PCHAR)cur + cur->Size);
            }

            // Now guarantee ascending order of group number & cpu mask so that group-relative core number can be assigned

            sort(_vProcessorCoreInformation.begin(), _vProcessorCoreInformation.end(),
                [](const ProcessorCoreInformation& a, const ProcessorCoreInformation& b)
                {
                    return a._groupNumber < b._groupNumber ||
                          (a._groupNumber == b._groupNumber && a._processorMask < b._processorMask);
                });

            // Assign group-relative core number

            WORD coreNumber = 0;
            WORD group = 0;
            for (auto& core : _vProcessorCoreInformation)
            {
                if (core._groupNumber != group)
                {
                    group = core._groupNumber;
                    coreNumber = 0;
                }
                core._groupCoreNumber = coreNumber++;
            }
        }

        ////
        // Cache Relations
        ////

        ReturnedLength = AllocSize;
        fResult = GetLogicalProcessorInformationEx(RelationCache, pInformation, &ReturnedLength);
        if (!fResult && GetLastError() == ERROR_INSUFFICIENT_BUFFER)
        {
            delete [] pInformation;
            AllocSize = ReturnedLength;
            pInformation = (PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX) new char[AllocSize];
            fResult = GetLogicalProcessorInformationEx(RelationCache, pInformation, &ReturnedLength);
        }

        if (fResult)
        {
            PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX cur = pInformation;

            while (ReturnedLength != 0)
            {
                assert(ReturnedLength >= cur->Size);

                if (cur->Size > ReturnedLength)
                {
                    break;
                }

                ProcessorCacheInformation cache(
                    cur->Cache.Level,
                    cur->Cache.Associativity,
                    cur->Cache.LineSize,
                    cur->Cache.CacheSize,
                    cur->Cache.Type);

                for (WORD i = 0; i < (cur->Cache.GroupCount ? cur->Cache.GroupCount : 1); i++)
                {
                    cache._processorMasks.emplace_back(cur->Cache.GroupMasks[i].Group,
                                                       cur->Cache.GroupMasks[i].Mask);
                }

                _vProcessorCacheInformation.push_back(cache);

                ReturnedLength -= cur->Size;
                cur = (PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX)((PCHAR)cur + cur->Size);
            }
        }

        delete [] pInformation;
    }

    bool IsGroupValid(WORD Group)
    {
        if (Group < _vProcessorGroupInformation.size())
        {
            return true;
        }
        else
        {
            return false;
        }
    }

    // Return the next active processor in the system, exclusive (Next = true)
    // or inclusive (Next = false) of the input group/processor.
    // Iteration is in order of absolute processor number.
    // This does assume at least one core is active, but that is a given.
    //
    // This logic is strictly unaware that sparse processor masks are not possible;
    // address this later, not important. See comments around RelationGroup query.
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

    //
    // Efficiency of these mappings is not a first order concern. We simply use these to avoid assuming
    // ordering of groups/masks of processors within topology structures. There's strictly no reason,
    // for example, that socket 0 contains the first groups (0, 1, etc.) of processors, at least not
    // documented or guaranteed.
    //

    DWORD GetNumaOfProcessor(WORD Group, BYTE Processor) const
    {
        for (const auto& numa : _vProcessorNumaInformation)
        {
            for (const auto& mask : numa._vProcessorMasks)
            {
                if (mask.first == Group && (mask.second & ((KAFFINITY)1 << Processor)))
                {
                    return numa._nodeNumber;
                }
            }
        }

        assert(false);
        return 0;
    }

    DWORD GetSocketOfProcessor(WORD Group, BYTE Processor) const
    {
        for (const auto& socket : _vProcessorSocketInformation)
        {
            for (const auto& mask : socket._vProcessorMasks)
            {
                if (mask.first == Group && (mask.second & ((KAFFINITY)1 << Processor)))
                {
                    return socket._ulSocketNumber;
                }
            }
        }

        assert(false);
        return 0;
    }

    WORD GetCoreOfProcessor(WORD Group, BYTE Processor, BYTE& EfficiencyClass) const
    {
        for (const auto& core : _vProcessorCoreInformation)
        {
            if (core._groupNumber == Group && (core._processorMask & ((KAFFINITY)1 << Processor)))
            {
                EfficiencyClass = core._efficiencyClass;
                return core._groupCoreNumber;
            }
        }

        assert(false);
        return 0;
    }

    //
    // Return the largest cache line size for the given cache level (1, 2, 3).
    // If level is 0, return the largest cache line size across all levels.
    // Returns 0 if no matching caches are present.
    //

    WORD GetLargestCacheLineSize(BYTE level = 0) const
    {
        WORD largestLineSize = 0;
        for (const auto& cache : _vProcessorCacheInformation)
        {
            if ((level == 0 || cache._level == level) && cache._lineSize > largestLineSize)
            {
                largestLineSize = cache._lineSize;
            }
        }
        return largestLineSize;
    }

    static unsigned int MaskCount(KAFFINITY Mask)
    {
        //
        // Trivial popcount for affinity mask w/o insn dependency
        //

        unsigned int count = 0;

        while (Mask)
        {
            Mask &= (Mask - 1);
            count++;
        }

        return count;
    }

    enum class Section {
        All,
        Topology,
        Cache
    };

    string GetText(UINT32 indent, Section section = Section::All) const;
    string GetXml(UINT32 indent, Section section = Section::All) const;

    // Format a set of (group, mask) pairs as group mask range string.
    // Single group (fMultiGroup=false): "0-3,7" (no group prefix)
    // Multi group: "0/0-3 1/0-7" (space-separated group entries)
    static string GroupMaskRanges(const vector<pair<WORD, KAFFINITY>>& masks, bool fMultiGroup);
};

//
// Helper macros for outputting indented XML. They assume a local variable "indent".
// Use the Inc form when outputting the opening tag for a multi-line section: <SomeSection>
// Use Dec for the closing tag: </SomeSection>
//

// start line with indent
#define AddXml(s,str)       { (s).append(indent, ' '); (s) += (str); }
// start new indented section
#define AddXmlInc(s,str)    { (s).append(indent, ' '); indent += 2; (s) += (str); }
// end indented section
#define AddXmlDec(s,str)    { if (indent >= 2) { indent -= 2; }; (s).append(indent, ' '); (s) += (str); }

class SystemInformation
{
private:
    SYSTEMTIME StartTime;

public:
    ProcessorTopology processorTopology;
    string sComputerName;
    string sProcessorName;
    string sActivePolicyName;
    string sActivePolicyGuid;
    DWORD dwPageSize;

    SystemInformation()
    {
        char buffer[128];
        DWORD cb = _countof(buffer);
        GUID *guid = NULL;
        BOOL fResult;

        SYSTEM_INFO sysInfo = {};
        GetNativeSystemInfo(&sysInfo);
        dwPageSize = sysInfo.dwPageSize;

#pragma prefast(suppress:38020, "Yes, we're aware this is an ANSI API in a UNICODE project")
        fResult = GetComputerNameExA(ComputerNamePhysicalDnsHostname, buffer, &cb);
        if (fResult)
        {
            sComputerName = buffer;
        }

        // capture processor name from registry (same source as Task Manager)
        cb = _countof(buffer);
        if (RegGetValueA(HKEY_LOCAL_MACHINE,
                         "HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0",
                         "ProcessorNameString",
                         RRF_RT_REG_SZ,
                         NULL,
                         buffer,
                         &cb) == ERROR_SUCCESS)
        {
            sProcessorName = buffer;
            Util::ShrinkContiguousWhitespace(sProcessorName);
        }

        if (sProcessorName.empty())
        {
            sProcessorName = "<unknown>";
        }

        // capture start time
        GetSystemTime(&StartTime);

        if (PowerGetActiveScheme(NULL, &guid) == ERROR_SUCCESS &&
            PowerReadFriendlyName(NULL, guid, NULL, NULL, NULL, &cb) == ERROR_SUCCESS)
        {
            PUCHAR pwrBuffer;

            if (cb <= _countof(buffer))
            {
                pwrBuffer = (PUCHAR) buffer;
            }
            else
            {
                pwrBuffer = new UCHAR[cb];
            }

            if (PowerReadFriendlyName(NULL, guid, NULL, NULL, pwrBuffer, &cb) == ERROR_SUCCESS)
            {
                // Cast wide string down to basic - all of our current output streams are basic
                wstring wActivePolicyName = (PWCHAR) pwrBuffer;
                std::wstring_convert<std::codecvt_utf8<wchar_t>> cvt;
                sActivePolicyName = cvt.to_bytes(wActivePolicyName);
            }

            if (pwrBuffer != (PVOID) buffer)
            {
                delete[] pwrBuffer;
            }
        }

        if (sActivePolicyName.empty())
        {
            sActivePolicyName = "<unknown>";
        }

        if (guid)
        {
            sprintf_s(buffer, _countof(buffer),
                "%08lx-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x",
                guid->Data1, guid->Data2, guid->Data3,
                guid->Data4[0], guid->Data4[1], guid->Data4[2], guid->Data4[3],
                guid->Data4[4], guid->Data4[5], guid->Data4[6], guid->Data4[7]);

            sActivePolicyGuid = buffer;

            LocalFree(guid);
        }
    }

    // for unit test, squelch variable timestamp
    void ResetTime()
    {
        StartTime = { 0 };
    }

    // Update the start timestamp to current time
    void CaptureTime()
    {
        GetSystemTime(&StartTime);
    }

    string GetText(UINT32 indent = 0) const
    {
        int nWritten;
        string sText;
        string sIndent(indent, ' ');

        sText += "System information:\n\n";

        sText += sIndent + "  computer name:        ";
        sText += sComputerName;
        sText += "\n";

        sText += sIndent + "  processor name:       ";
        sText += sProcessorName;
        sText += "\n";

        sText += sIndent + "  start time:           ";
        if (StartTime.wYear) {

            char szBuffer[128];

            nWritten = sprintf_s(szBuffer, _countof(szBuffer),
                "%u/%02u/%02u %02u:%02u:%02u UTC",
                StartTime.wYear,
                StartTime.wMonth,
                StartTime.wDay,
                StartTime.wHour,
                StartTime.wMinute,
                StartTime.wSecond);
            assert(nWritten && nWritten < _countof(szBuffer));
            sText += szBuffer;
        }
        sText += "\n";

        sText += sIndent + "  active power scheme:  ";
        sText += sActivePolicyName;

        if (!sActivePolicyGuid.empty())
        {
            sText += " (";
            sText += sActivePolicyGuid;
            sText += ")";
        }

        sText += "\n";
        sText += sIndent + "  page size:            " + Util::GetSizeKMGT(dwPageSize) + "\n";

        sText += "\n";
        sText += processorTopology.GetText(indent + 2);

        return sText;
    }

    string GetXml(UINT32 indent) const
    {
        char szBuffer[64]; // enough for 64bit mask (17ch) and timestamp
        int nWritten;
        string sXml;

        AddXmlInc(sXml, "<System>\n");

        // identify computer which ran the test
        AddXml(sXml, "<ComputerName>");
        sXml += sComputerName;
        sXml += "</ComputerName>\n";

        AddXml(sXml, "<ProcessorName>");
        sXml += sProcessorName;
        sXml += "</ProcessorName>\n";

        // identify tool version which performed the test
        AddXmlInc(sXml, "<Tool>\n");
        AddXml(sXml,"<Version>" DISKSPD_NUMERIC_VERSION_STRING "</Version>\n");
        AddXml(sXml, "<VersionDate>" DISKSPD_DATE_VERSION_STRING "</VersionDate>\n");
        AddXmlDec(sXml, "</Tool>\n");

        AddXml(sXml, "<RunTime>");
        if (StartTime.wYear) {

            nWritten = sprintf_s(szBuffer, _countof(szBuffer),
                "%u/%02u/%02u %02u:%02u:%02u UTC",
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

        AddXml(sXml, "<PowerScheme Name=\"")
        sXml += sActivePolicyName;
        sXml += "\" Guid=\"";
        sXml += sActivePolicyGuid;
        sXml += "\"/>\n";

        nWritten = sprintf_s(szBuffer, _countof(szBuffer), "<PageSize>%u</PageSize>\n", dwPageSize);
        assert(nWritten && nWritten < _countof(szBuffer));
        AddXml(sXml, szBuffer);

        sXml += processorTopology.GetXml(indent);

        AddXmlDec(sXml, "</System>\n");

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

// memory mapped IO modes
// off -> default
// on -> (-Sm or -Smw)
enum class MemoryMappedIoMode {
    Undefined = 0,
    Off,
    On,
};

// memory mapped IO flush modes
// off / Undefined -> default
// on -> (-Sm or -Smw)
enum class MemoryMappedIoFlushMode {
    Undefined = 0,
    ViewOfFile,
    NonVolatileMemory,
    NonVolatileMemoryNoDrain,
};

// BypassIO modes
// undefined -> default (not specified, no BypassIO)
// partial -> bypass file system filters if storage stack cannot be bypassed (-Sy)
// full -> requires both file system filters and storage stack bypass (-SY)
enum class BypassIoMode {
    Undefined = 0,
    Partial,
    Full,
};

enum class IOMode
{
    Unknown,
    Random,
    Sequential,
    Mixed,
    InterlockedSequential,
    ParallelAsync
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

    string GetXml(UINT32 indent) const;

private:
    UINT32 _ulThread;
    UINT32 _ulWeight;
};

// Character which leads off a template target definition; e.g. *1, *2
#define TEMPLATE_TARGET_PREFIX ('*')

class Target
{
public:

    Target() :
        _dwBlockSize(64 * 1024),
        _dwRequestCount(2),
        _ullBlockAlignment(0),
        _ulWriteRatio(0),
        _ulRandomRatio(0),
        _ullBaseFileOffset(0),
        _fParallelAsyncIO(false),
        _fInterlockedSequential(false),
        _cacheMode(TargetCacheMode::Cached),
        _writeThroughMode(WriteThroughMode::Off),
        _memoryMappedIoMode(MemoryMappedIoMode::Off),
        _memoryMappedIoNvToken(nullptr),
        _memoryMappedIoFlushMode(MemoryMappedIoFlushMode::Undefined),
        _fZeroWriteBuffers(false),
        _dwThreadsPerFile(1),
        _ullThreadStride(0),
        _fCreateFile(false),
        _fPrecreated(false),
        _ullFileSize(0),
        _ullMaxFileSize(0),
        _fUseBurstSize(false),
        _dwBurstSize(0),
        _dwThinkTime(0),
        _fThinkTime(false),
        _fSequentialScanHint(false),
        _fRandomAccessHint(false),
        _fTemporaryFileHint(false),
        _fUseLargePages(false),
        _mappedViewFileHandle(INVALID_HANDLE_VALUE),
        _mappedView(NULL),
        _ioPriorityHint(IoPriorityHintNormal),
        _ulWeight(1),
        _dwThroughputBytesPerMillisecond(0),
        _dwThroughputIOPS(0),
        _cbRandomDataWriteBuffer(0),
        _sRandomDataWriteBufferSourcePath(),
        _pRandomDataWriteBuffer(nullptr),
        _fOwnsRandomDataWriteBuffer(false),
        _bypassIoMode(BypassIoMode::Undefined)
    {
    }

    // Copy constructor: default member-wise copy is correct because ownership
    // is only set on the original Target AFTER all copies have been distributed
    // to thread cookies. Copies always have _fOwnsRandomDataWriteBuffer = false.
    //
    // IMPORTANT: never copy a Target after _fOwnsRandomDataWriteBuffer has been
    // set to true -- that would cause double-free on destruction.
    Target(const Target& other) = default;
    Target& operator=(const Target&) = delete;

    ~Target()
    {
        if (_fOwnsRandomDataWriteBuffer && _pRandomDataWriteBuffer != nullptr)
        {
            VirtualFree(_pRandomDataWriteBuffer, 0, MEM_RELEASE);
        }
    }

    IOMode GetIOMode() const
    {
        if (GetRandomRatio() == 100)
        {
            return IOMode::Random;
        }
        else if (GetRandomRatio() != 0)
        {
            return IOMode::Mixed;
        }
        else if (GetUseParallelAsyncIO())
        {
            return IOMode::ParallelAsync;
        }
        else if (GetUseInterlockedSequential())
        {
            return IOMode::InterlockedSequential;
        }
        else
        {
            return IOMode::Sequential;
        }
    }

    void SetPath(const string& sPath) { _sPath = sPath; }
    void SetPath(const char *pPath) { _sPath = pPath; }
    const string& GetPath() const { return _sPath; }

    void SetBlockSizeInBytes(DWORD dwBlockSize) { _dwBlockSize = dwBlockSize; }
    DWORD GetBlockSizeInBytes() const { return _dwBlockSize; }

    void SetBlockAlignmentInBytes(UINT64 ullBlockAlignment)
    {
        _ullBlockAlignment = ullBlockAlignment;
    }
    // actual is used in validation to detect unclear/mis-specified intent
    // like -rs<xx> -s
    UINT64 GetBlockAlignmentInBytes(bool actual = false) const
    {
        return _ullBlockAlignment ? _ullBlockAlignment : (actual ? 0 : _dwBlockSize);
    }

    void SetWriteRatio(UINT32 writeRatio) { _ulWriteRatio = writeRatio; }
    UINT32 GetWriteRatio() const { return _ulWriteRatio; }

    void SetRandomRatio(UINT32 randomRatio) { _ulRandomRatio = randomRatio; }
    UINT32 GetRandomRatio() const { return _ulRandomRatio; }

    void SetBaseFileOffsetInBytes(UINT64 ullBaseFileOffset) { _ullBaseFileOffset = ullBaseFileOffset; }
    UINT64 GetBaseFileOffsetInBytes() const { return _ullBaseFileOffset; }
    UINT64 GetThreadBaseRelativeOffsetInBytes(UINT32 ulThreadNo) const { return ulThreadNo * _ullThreadStride; }
    UINT64 GetThreadBaseFileOffsetInBytes(UINT32 ulThreadNo) const { return _ullBaseFileOffset + GetThreadBaseRelativeOffsetInBytes(ulThreadNo); }


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
    WriteThroughMode GetWriteThroughMode() const { return _writeThroughMode; }

    void SetMemoryMappedIoMode(MemoryMappedIoMode memoryMappedIoMode ) { _memoryMappedIoMode = memoryMappedIoMode; }
    MemoryMappedIoMode GetMemoryMappedIoMode() const { return _memoryMappedIoMode; }

    void SetMemoryMappedIoNvToken(PVOID memoryMappedIoNvToken) { _memoryMappedIoNvToken = memoryMappedIoNvToken; }
    PVOID GetMemoryMappedIoNvToken() const { return _memoryMappedIoNvToken; }

    void SetMemoryMappedIoFlushMode(MemoryMappedIoFlushMode memoryMappedIoFlushMode) { _memoryMappedIoFlushMode = memoryMappedIoFlushMode; }
    MemoryMappedIoFlushMode GetMemoryMappedIoFlushMode() const { return _memoryMappedIoFlushMode; }

    void SetBypassIoMode(BypassIoMode bypassIoMode) { _bypassIoMode = bypassIoMode; }
    BypassIoMode GetBypassIoMode() const { return _bypassIoMode; }

    void SetZeroWriteBuffers(bool fBool) { _fZeroWriteBuffers = fBool; }
    bool GetZeroWriteBuffers() const { return _fZeroWriteBuffers; }

    void SetRandomDataWriteBufferSize(UINT64 cbWriteBuffer) { _cbRandomDataWriteBuffer = cbWriteBuffer; }
    UINT64 GetRandomDataWriteBufferSize(void) const { return _cbRandomDataWriteBuffer; }

    void SetRandomDataWriteBufferSourcePath(string sPath) { _sRandomDataWriteBufferSourcePath = sPath; }
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

    void SetUseParallelAsyncIO(bool fBool) { _fParallelAsyncIO = fBool; }
    bool GetUseParallelAsyncIO() const { return _fParallelAsyncIO; }

    void SetUseInterlockedSequential(bool fBool) { _fInterlockedSequential = fBool; }
    bool GetUseInterlockedSequential() const { return _fInterlockedSequential; }

    void SetThreadStrideInBytes(UINT64 ullThreadStride) { _ullThreadStride = ullThreadStride; }
    UINT64 GetThreadStrideInBytes() const { return _ullThreadStride; }

    void SetMappedViewFileHandle(HANDLE FileHandle) { _mappedViewFileHandle = FileHandle; }
    HANDLE GetMappedViewFileHandle() const { return _mappedViewFileHandle; }

    void SetMappedView(BYTE *MappedView) { _mappedView = MappedView; }
    BYTE* GetMappedView() const { return _mappedView; }

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

    // Convert units to BPMS. Nonzero value of IOPS indicates originally specified units for display/profile.
    void SetThroughputIOPS(DWORD dwIOPS)
    {
        _dwThroughputIOPS = dwIOPS;
        _dwThroughputBytesPerMillisecond = (dwIOPS * _dwBlockSize) / 1000;
    }
    DWORD GetThroughputIOPS() const { return _dwThroughputIOPS; }
    void SetThroughput(DWORD dwThroughputBytesPerMillisecond)
    {
        _dwThroughputIOPS = 0;
        _dwThroughputBytesPerMillisecond = dwThroughputBytesPerMillisecond;
    }
    DWORD GetThroughputInBytesPerMillisecond() const { return _dwThroughputBytesPerMillisecond; }

    string GetXml(UINT32 indent) const;
    string GetText(UINT32 indent, bool fUseThreadsPerFile, bool fUseRequestsPerFile, bool fCompletionRoutines, bool fUseIoRing = false) const;

    bool AllocateAndFillRandomDataWriteBuffer(Random *pRand);
    BYTE* GetRandomDataWriteBuffer(Random *pRand) const;
    BYTE* GetRandomDataWriteBuffer() const { return _pRandomDataWriteBuffer; }

    // Replace the write source buffer with a new allocation, taking ownership.
    // Used by worker threads to create per-thread separated copies.
    void SetRandomDataWriteBuffer(BYTE *pBuffer)
    {
        assert(!_fOwnsRandomDataWriteBuffer);
        _pRandomDataWriteBuffer = pBuffer;
        _fOwnsRandomDataWriteBuffer = true;
    }

    void SetDistributionRange(const vector<DistributionRange>& v, DistributionType t)
    {
        _distribution.Set(v, t);
    }
    auto& GetDistributionRange() const { return _distribution.GetRanges(); }
    auto GetDistributionType() const { return _distribution.GetType(); }
    Distribution& GetDistribution() { return _distribution; }
    const Distribution& GetDistribution() const { return _distribution; }

    DWORD GetCreateFlags(bool fAsync)
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
    UINT32 _ulWriteRatio;
    UINT32 _ulRandomRatio;

    UINT64 _ullBaseFileOffset;

    TargetCacheMode _cacheMode;
    WriteThroughMode _writeThroughMode;
    MemoryMappedIoMode _memoryMappedIoMode;
    MemoryMappedIoFlushMode _memoryMappedIoFlushMode;
    PVOID _memoryMappedIoNvToken;
    BypassIoMode _bypassIoMode;
    DWORD _dwThreadsPerFile;
    UINT64 _ullThreadStride;

    UINT64 _ullFileSize;
    UINT64 _ullMaxFileSize;

    DWORD _dwBurstSize;     // number of IOs in a burst
    DWORD _dwThinkTime;     // time to pause before issuing the next burst of IOs

    DWORD _dwThroughputBytesPerMillisecond; // set to 0 to disable throttling
    DWORD _dwThroughputIOPS;                // if IOPS are specified they are converted to BPMS but saved for fidelity to XML/output

    bool _fThinkTime:1;             // variable to decide whether to think between IOs (default is false) (removed by using _dwThinkTime==0?)
    bool _fUseBurstSize:1;          // TODO: "use" or "enable"?; since burst size must be specified with the think time, one variable should be sufficient
    bool _fZeroWriteBuffers:1;
    bool _fCreateFile:1;
    bool _fPrecreated:1;            // used to track which files have been created before the first timespan and which have to be created later
    bool _fParallelAsyncIO:1;
    bool _fInterlockedSequential:1;
    bool _fSequentialScanHint:1;    // open file with the FILE_FLAG_SEQUENTIAL_SCAN hint
    bool _fRandomAccessHint:1;      // open file with the FILE_FLAG_RANDOM_ACCESS hint
    bool _fTemporaryFileHint:1;     // open file with the FILE_ATTRIBUTE_TEMPORARY hint
    bool _fUseLargePages:1;         // Use large pages for IO buffers

    UINT64 _cbRandomDataWriteBuffer;            // if > 0, then the write buffer should be filled with random data
    string _sRandomDataWriteBufferSourcePath;   // file that should be used for filling the write buffer (if the path is not available, use a crypto provider)
    BYTE *_pRandomDataWriteBuffer;              // a buffer used for write data when _cbWriteBuffer > 0; it's shared by all the threads working on this target
    bool _fOwnsRandomDataWriteBuffer;           // true if this Target instance owns (allocated) the write buffer and should free it

    HANDLE _mappedViewFileHandle;
    BYTE *_mappedView;

    PRIORITY_HINT _ioPriorityHint;

    UINT32 _ulWeight;
    vector<ThreadTarget> _vThreadTargets;

    Distribution _distribution;

    bool _FillRandomDataWriteBuffer(Random *pRand);

    friend class UnitTests::ProfileUnitTests;
    friend class UnitTests::TargetUnitTests;
    friend class UnitTests::ThreadParametersUnitTests;
};

class AffinityAssignment
{
public:
    WORD wGroup;
    BYTE bProc;
    BYTE bEfficiencyClass;
    WORD wCore;

    AffinityAssignment() = delete;
    AffinityAssignment(WORD wGroup, BYTE bProc, BYTE bEfficiencyClass = 0, WORD wCore = 0) :
        wGroup(wGroup),
        bProc(bProc),
        bEfficiencyClass(bEfficiencyClass),
        wCore(wCore)
    {
    }
};

// Configuration-level affinity specification: a group and a bitmask of CPUs.
// A mask with a single bit = one CPU; multiple bits = a set; mask=0 = whole-group
// effective (resolved at finalization using the system's active processor mask).
// LSB-MSB expansion of a mask reproduces the specification order.

class AffinityGroupMask
{
public:
    WORD wGroup;
    KAFFINITY mask;

    AffinityGroupMask() = delete;
    AffinityGroupMask(WORD p_wGroup, KAFFINITY p_mask) :
        wGroup(p_wGroup),
        mask(p_mask)
    {
    }

    // Can a new CPU bit merge into this entry?
    // Merge is allowed only if the new bit is strictly above all current bits,
    // preserving the invariant that LSB-MSB expansion of the mask reproduces
    // the original specification order.
    bool CanMerge(WORD group, BYTE proc) const
    {
        if (group != wGroup || mask == 0)
        {
            return false;
        }

        return ((KAFFINITY)1 << proc) > mask;
    }

    void Merge(BYTE proc)
    {
        mask |= ((KAFFINITY)1 << proc);
    }

    // Compact a vector of per-CPU assignments into group/mask entries,
    // merging where LSB-MSB ordering allows.
    static vector<AffinityGroupMask> Compact(const vector<AffinityAssignment>& v)
    {
        vector<AffinityGroupMask> result;
        for (const auto& a : v)
        {
            if (!result.empty() && result.back().CanMerge(a.wGroup, a.bProc))
            {
                result.back().Merge(a.bProc);
            }
            else
            {
                result.emplace_back(a.wGroup, (KAFFINITY)1 << a.bProc);
            }
        }
        return result;
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
        _affinityTraversal(AffinityTraversal::Unspecified),
        _affinityGroupSpan(AffinityGroupSpan::Unspecified),
        _affinityEfficiencyOrder(AffinityEfficiencyOrder::Unspecified),
        _fCompletionRoutines(false),
        _fMeasureLatency(false),
        _fCalculateIopsStdDev(false),
        _ulIoBucketDurationInMilliseconds(1000),
        _bufferSeparation(BufferSeparation::PDECacheLine),
        _fBufferSeparationExplicit(false),
        _pSystem(&g_SystemInformation),
        _dwEffectiveBufferSeparation(0),
        _fFinalized(false),
        _dwCompletionDepth(c_defaultCompletionDepth),
        _fCompletionDepthExplicit(false),
        _fUseIoRing(false),
        _ulIoRingBatchSize(25),
        _fIoRingBatchSizeIsPercent(true),
        _fUseRegBuffer(true)
    {
    }

    // Configuration-level affinity: group/mask pairs preserving specification order.
    void ClearAffinityGroupMasks()
    {
        _vAffinityMasks.clear();
    }
    void AddAffinityGroupMask(WORD wGroup, KAFFINITY mask)
    {
        _vAffinityMasks.emplace_back(wGroup, mask);
    }
    void AddAffinityGroupMaskCpu(WORD wGroup, BYTE bProc)
    {
        KAFFINITY bit = (KAFFINITY)1 << bProc;
        if (!_vAffinityMasks.empty() && _vAffinityMasks.back().CanMerge(wGroup, bProc))
        {
            _vAffinityMasks.back().Merge(bProc);
        }
        else
        {
            _vAffinityMasks.emplace_back(wGroup, bit);
        }
    }
    const auto& GetAffinityGroupMasks() const { return _vAffinityMasks; }

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

    void SetAffinityTraversal(AffinityTraversal traversal) { _affinityTraversal = traversal; }
    AffinityTraversal GetAffinityTraversal(bool fResolve = true) const
    {
        if (fResolve && _affinityTraversal == AffinityTraversal::Unspecified)
        {
            return AffinityTraversal::Cpu;
        }
        return _affinityTraversal;
    }

    void SetAffinityGroupSpan(AffinityGroupSpan span) { _affinityGroupSpan = span; }
    AffinityGroupSpan GetAffinityGroupSpan(bool fResolve = true) const
    {
        if (fResolve && _affinityGroupSpan == AffinityGroupSpan::Unspecified)
        {
            return AffinityGroupSpan::Fill;
        }
        return _affinityGroupSpan;
    }

    void SetAffinityEfficiencyOrder(AffinityEfficiencyOrder order) { _affinityEfficiencyOrder = order; }
    AffinityEfficiencyOrder GetAffinityEfficiencyOrder(bool fResolve = true) const
    {
        if (fResolve && _affinityEfficiencyOrder == AffinityEfficiencyOrder::Unspecified)
        {
            return AffinityEfficiencyOrder::PFirst;
        }
        return _affinityEfficiencyOrder;
    }

    void SetCompletionRoutines(bool fCompletionRoutines) { _fCompletionRoutines = fCompletionRoutines; }
    bool GetCompletionRoutines() const { return _fCompletionRoutines; }

    void SetMeasureLatency(bool fMeasureLatency) { _fMeasureLatency = fMeasureLatency; }
    bool GetMeasureLatency() const { return _fMeasureLatency; }

    void SetCalculateIopsStdDev(bool fCalculateStdDev) { _fCalculateIopsStdDev = fCalculateStdDev; }
    bool GetCalculateIopsStdDev() const { return _fCalculateIopsStdDev; }

    void SetIoBucketDurationInMilliseconds(UINT32 ulIoBucketDurationInMilliseconds) { _ulIoBucketDurationInMilliseconds = ulIoBucketDurationInMilliseconds; }
    UINT32 GetIoBucketDurationInMilliseconds() const { return _ulIoBucketDurationInMilliseconds; }

    void SetBufferSeparation(BufferSeparation bufferSeparation) { _bufferSeparation = bufferSeparation; }
    BufferSeparation GetBufferSeparation() const { return _bufferSeparation; }

    void SetBufferSeparationExplicit(bool fExplicit) { _fBufferSeparationExplicit = fExplicit; }
    bool IsBufferSeparationExplicit() const { return _fBufferSeparationExplicit; }

    void SetSystem(const SystemInformation *pSystem) { _pSystem = pSystem; }

    //
    // Finalize all effective values from configured policies and system
    // information. Must be called exactly once before accessing effective
    // values (e.g., before IO buffer allocation or result reporting).
    //

    void Finalize() const
    {
        assert(!_fFinalized);
        _FinalizeBufferSeparation();
        _FinalizeAffinity();
        _fFinalized = true;
    }

    DWORD GetEffectiveBufferSeparation() const
    {
        assert(_fFinalized);
        return _dwEffectiveBufferSeparation;
    }

    // Effective affinity: expanded individual CPU assignments, computed at finalization.
    const auto& GetEffectiveAffinityAssignments() const
    {
        assert(_fFinalized);
        return _vEffectiveAffinity;
    }

    // Truncate effective affinity to the number actually used for thread assignment.
    // When threads < effective entries, the tail was never assigned.
    void TruncateEffectiveAffinity(size_t cThreads) const
    {
        assert(_fFinalized);
        if (cThreads < _vEffectiveAffinity.size())
        {
            _vEffectiveAffinity.erase(
                _vEffectiveAffinity.begin() + cThreads,
                _vEffectiveAffinity.end());
        }
    }

    bool IsFinalized() const { return _fFinalized; }

    void SetCompletionDepth(DWORD dwCompletionDepth) { _dwCompletionDepth = dwCompletionDepth; }
    DWORD GetCompletionDepth() const { return _dwCompletionDepth; }

    void SetCompletionDepthExplicit(bool fExplicit) { _fCompletionDepthExplicit = fExplicit; }
    bool IsCompletionDepthExplicit() const { return _fCompletionDepthExplicit; }

    void SetUseIoRing(bool fUseIoRing) { _fUseIoRing = fUseIoRing; }
    bool GetUseIoRing() const { return _fUseIoRing; }

    void SetIoRingBatchSize(UINT32 ulIoRingBatchSize) { _ulIoRingBatchSize = ulIoRingBatchSize; }
    UINT32 GetIoRingBatchSize() const { return _ulIoRingBatchSize; }

    void SetIoRingBatchSizeIsPercent(bool fIsPercent) { _fIoRingBatchSizeIsPercent = fIsPercent; }
    bool GetIoRingBatchSizeIsPercent() const { return _fIoRingBatchSizeIsPercent; }

    void SetUseRegBuffer(bool fUseRegBuffer) { _fUseRegBuffer = fUseRegBuffer; }
    bool GetUseRegBuffer() const { return _fUseRegBuffer; }

    string GetXml(UINT32 indent) const;
    string GetText(UINT32 indent) const;
    void MarkFilesAsPrecreated(const vector<string> vFiles);

private:

    void _FinalizeBufferSeparation() const
    {
        _dwEffectiveBufferSeparation = Util::GetBufferAlignmentSize(
            _bufferSeparation,
            _pSystem->dwPageSize,
            _pSystem->processorTopology.GetLargestCacheLineSize());
    }

    void _FinalizeAffinity() const
    {
        _vEffectiveAffinity.clear();
        if (_fDisableAffinity)
        {
            return;
        }

        // If no explicit affinity was specified, synthesize from group topology
        // and allow policy to determine the final assignment order. Note that
        // the _vAffinityMasks remains as-specified, empty if empty.
        vector<AffinityGroupMask> vMasks;
        if (_vAffinityMasks.empty())
        {
            assert(_pSystem != nullptr);
            for (const auto& g : _pSystem->processorTopology._vProcessorGroupInformation)
            {
                vMasks.emplace_back(g._groupNumber, g._activeProcessorMask);
            }
        }

        const auto& vSource = _vAffinityMasks.empty() ? vMasks : _vAffinityMasks;
        assert(_pSystem != nullptr);
        const auto& vCores = _pSystem->processorTopology._vProcessorCoreInformation;

        // Expand config masks into a flat list of CPUs in LSB-MSB order,
        // tagging each with its core index & efficiency class from the topology.
        //
        // Cores are sorted ascending by group+mask, so as we iterate bits LSB->MSB within
        // a mask, cores advance in the same direction. A speculative check on the current/next
        // core avoids the full search in the common case, and no need to build a separate map.

        vector<AffinityAssignment> vExpanded;
        size_t core = 0;

        for (const auto& gm : vSource)
        {
            KAFFINITY m = gm.mask;

            if (m == 0)
            {
                assert(gm.wGroup < _pSystem->processorTopology._vProcessorGroupInformation.size());
                m = _pSystem->processorTopology._vProcessorGroupInformation[gm.wGroup]._activeProcessorMask;
            }

            BYTE bit = 0;
            while (m)
            {
                if (m & 1)
                {
                    KAFFINITY cpuBit = (KAFFINITY)1 << bit;

                    // Speculate: current hint or next core covers this CPU
                    if (vCores[core]._groupNumber == gm.wGroup &&
                        (vCores[core]._processorMask & cpuBit))
                    {
                        // hit
                    }
                    else if (core + 1 < vCores.size() &&
                             vCores[core + 1]._groupNumber == gm.wGroup &&
                             (vCores[core + 1]._processorMask & cpuBit))
                    {
                        core++;
                    }
                    else
                    {
                        // Fallback: full search (group change or non-sequential mask).
                        // Every active CPU appears in some core's mask - assert
                        size_t ci = 0;
                        for (; ci < vCores.size(); ci++)
                        {
                            if (vCores[ci]._groupNumber == gm.wGroup &&
                                (vCores[ci]._processorMask & cpuBit))
                            {
                                core = ci;
                                break;
                            }
                        }
                        assert(ci != vCores.size());
                    }

                    vExpanded.emplace_back(gm.wGroup, bit,
                        vCores[core]._efficiencyClass,
                        (WORD)core);
                }
                m >>= 1;
                ++bit;
            }
        }

        // Classify each expanded CPU for the unified sort.
        //
        // Both Cpu and CoreAware modes use the same ClassifiedCpu struct and sort.
        // The difference is only in pass assignment:
        //   Cpu: all entries get pass = 0 (no core pigeon-holing)
        //   CoreAware: pass assigned via per-core occupancy counter
        //
        // The sort key handles all combinations of group span and efficiency
        // ordering. Original specification order is always the final tiebreaker,
        // achieving stable-sort semantics across the full integer range of
        // efficiency classes.

        struct ClassifiedCpu {
            WORD wGroup;
            BYTE bProc;
            BYTE effClass;
            BYTE pass;      // per-core occupancy: 0 = first in core, 1 = second, ...
            size_t order;   // original position for stable ordering
        };

        vector<ClassifiedCpu> vClassified;
        vClassified.reserve(vExpanded.size());

        if (GetAffinityTraversal() == AffinityTraversal::CoreAware)
        {
            // Core-aware: assign passes based on per-core occupancy.
            // For each core, the Nth CPU from the expanded list goes into pass N.
            vector<BYTE> vCoreOccupancy(vCores.size(), 0);

            for (size_t i = 0; i < vExpanded.size(); i++)
            {
                BYTE pass = vCoreOccupancy[vExpanded[i].wCore]++;
                vClassified.push_back({ vExpanded[i].wGroup, vExpanded[i].bProc,
                    vExpanded[i].bEfficiencyClass, pass, i });
            }
        }
        else
        {
            // Cpu mode: pass = 0 for all entries (no core pigeon-holing).
            for (size_t i = 0; i < vExpanded.size(); i++)
            {
                vClassified.push_back({ vExpanded[i].wGroup, vExpanded[i].bProc,
                    vExpanded[i].bEfficiencyClass, 0, i });
            }
        }

        // Sort by group span and efficiency ordering.
        //
        // Group span determines whether group is outermost (Fill) or
        // inside pass/efficiency (Span).
        //
        // Efficiency ordering determines the relationship between pass and
        // efficiency class:
        //   PFirst/EFirst: pass -> effClass -> ...
        //   FillPFirst/FillEFirst: effClass -> pass -> ...
        //   Unordered: effClass omitted from sort key
        //
        // PFirst/FillPFirst sort effClass descending (higher = P first).
        // EFirst/FillEFirst sort effClass ascending (lower = E first).

        // Resolved policies (Unspecified -> defaults handled by getters).
        bool fGroupFirst = (GetAffinityGroupSpan() == AffinityGroupSpan::Fill);
        bool fEffUnordered = (GetAffinityEfficiencyOrder() == AffinityEfficiencyOrder::Unordered);
        bool fEffDescending = (GetAffinityEfficiencyOrder() == AffinityEfficiencyOrder::PFirst ||
                               GetAffinityEfficiencyOrder() == AffinityEfficiencyOrder::FillPFirst);
        bool fEffBeforePass = (GetAffinityEfficiencyOrder() == AffinityEfficiencyOrder::FillPFirst ||
                               GetAffinityEfficiencyOrder() == AffinityEfficiencyOrder::FillEFirst);

        sort(vClassified.begin(), vClassified.end(),
            [fGroupFirst, fEffUnordered, fEffDescending, fEffBeforePass](const ClassifiedCpu& a, const ClassifiedCpu& b)
            {
                if (fGroupFirst && a.wGroup != b.wGroup) return a.wGroup < b.wGroup;

                if (fEffUnordered)
                {
                    if (a.pass != b.pass) return a.pass < b.pass;
                }
                else if (fEffBeforePass)
                {
                    if (a.effClass != b.effClass) return fEffDescending ? a.effClass > b.effClass : a.effClass < b.effClass;
                    if (a.pass != b.pass) return a.pass < b.pass;
                }
                else
                {
                    if (a.pass != b.pass) return a.pass < b.pass;
                    if (a.effClass != b.effClass) return fEffDescending ? a.effClass > b.effClass : a.effClass < b.effClass;
                }

                if (!fGroupFirst && a.wGroup != b.wGroup) return a.wGroup < b.wGroup;
                return a.order < b.order;
            });

        for (const auto& cc : vClassified)
        {
            _vEffectiveAffinity.emplace_back(cc.wGroup, cc.bProc, cc.effClass);
        }
    }

    vector<Target> _vTargets;
    UINT32 _ulDuration;
    UINT32 _ulWarmUp;
    UINT32 _ulCoolDown;
    UINT32 _ulRandSeed;
    DWORD _dwThreadCount;
    DWORD _dwRequestCount;
    bool _fRandomWriteData;
    bool _fDisableAffinity;
    AffinityTraversal _affinityTraversal;
    AffinityGroupSpan _affinityGroupSpan;
    AffinityEfficiencyOrder _affinityEfficiencyOrder;
    vector<AffinityGroupMask> _vAffinityMasks;
    bool _fCompletionRoutines;
    bool _fMeasureLatency;
    bool _fCalculateIopsStdDev;
    UINT32 _ulIoBucketDurationInMilliseconds;
    BufferSeparation _bufferSeparation;
    bool _fBufferSeparationExplicit;
    const SystemInformation *_pSystem;

    // Effective buffer separation is computed once during finalization
    // and then used for both IO buffer allocation and result reporting.
    mutable DWORD _dwEffectiveBufferSeparation;
    mutable bool _fFinalized;

    // Effective affinity: expanded individual CPU assignments from config masks.
    mutable vector<AffinityAssignment> _vEffectiveAffinity;

    DWORD _dwCompletionDepth;
    bool _fCompletionDepthExplicit;

    bool _fUseIoRing;
    UINT32 _ulIoRingBatchSize;
    bool _fIoRingBatchSizeIsPercent;
    bool _fUseRegBuffer;

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
        _fProfileOnly(false),
        _fSystemInformationOnly(false),
        _fVerbose(false),
        _fVerboseStats(false),
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

    void ClearTimeSpans()
    {
        _vTimeSpans.clear();
    }

    void AddTimeSpan(const TimeSpan& timeSpan)
    {
        _vTimeSpans.push_back(TimeSpan(timeSpan));
    }

    const vector<TimeSpan>& GetTimeSpans() const { return _vTimeSpans; }

    void SetProfileOnly(bool b) { _fProfileOnly = b; }
    bool GetProfileOnly() const { return _fProfileOnly; }

    void SetSystemInformationOnly(bool b) { _fSystemInformationOnly = b; }
    bool GetSystemInformationOnly() const { return _fSystemInformationOnly; }

    void SetVerbose(bool b) { _fVerbose = b; }
    bool GetVerbose() const { return _fVerbose; }

    void SetVerboseStats(bool b) { _fVerboseStats = b; }
    bool GetVerboseStats() const { return _fVerboseStats; }

    void SetProgress(DWORD dwProgress) { _dwProgress = dwProgress; }
    DWORD GetProgress() const { return _dwProgress; }

    void SetCmdLine(string sCmdLine) { _sCmdLine = sCmdLine; }
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

    string GetXml(UINT32 indent) const;
    bool Validate(bool fSingleSpec, SystemInformation *pSystem = nullptr) const;
    void MarkFilesAsPrecreated(const vector<string> vFiles);

private:
    Profile(const Profile& T);

    vector<TimeSpan>_vTimeSpans;
    bool _fVerbose;
    bool _fVerboseStats;
    bool _fProfileOnly;
    bool _fSystemInformationOnly;
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
    IORequest(Random *pRand) :
        _ioType(IOOperation::ReadIO),
        _pRand(pRand),
        _iCurrentTarget(0),
        _ullStartTime(0),
        _ulRequestIndex(0xFFFFFFFF),
        _ullTotalWeight(0),
        _fEqualWeights(true),
        _ActivityId()
    {
        memset(&_overlapped, 0, sizeof(OVERLAPPED));
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

    Target *GetCurrentTarget() { return _vTargets[_iCurrentTarget]; }
    size_t GetCurrentTargetIndex() { return _iCurrentTarget; }

    Target *GetNextTarget()
    {
        UINT64 ullWeight;

        if (_vTargets.size() == 1) {
            _iCurrentTarget = 0;
        }
        else if (_fEqualWeights) {
            _iCurrentTarget = _pRand->Rand32() % _vTargets.size();
        }
        else {
            ullWeight = _pRand->Rand64() % _ullTotalWeight;

            for (size_t iTarget = 0; iTarget < _vTargets.size(); iTarget++) {
                if (ullWeight < _vulTargetWeights[iTarget]) {
                    _iCurrentTarget = iTarget;
                    break;
                }

                ullWeight -= _vulTargetWeights[iTarget];
            }
        }

        return GetCurrentTarget();
    }

    void SetIoType(IOOperation ioType) { _ioType = ioType; }
    IOOperation GetIoType() const { return _ioType; }

    void SetStartTime(UINT64 ullStartTime) { _ullStartTime = ullStartTime; }
    UINT64 GetStartTime() const { return _ullStartTime; }

    void SetRequestIndex(UINT32 ulRequestIndex) { _ulRequestIndex = ulRequestIndex; }
    UINT32 GetRequestIndex() const { return _ulRequestIndex; }

    void SetActivityId(GUID ActivityId) { _ActivityId = ActivityId; }
    GUID GetActivityId() const { return _ActivityId; }

private:
    OVERLAPPED _overlapped;
    vector<Target*> _vTargets;
    vector<UINT32> _vulTargetWeights;
    UINT64 _ullTotalWeight;
    bool _fEqualWeights;
    Random *_pRand;
    size_t _iCurrentTarget;
    IOOperation _ioType;
    UINT64 _ullStartTime;
    UINT32 _ulRequestIndex;
    GUID _ActivityId;
};

typedef struct _ACTIVITY_ID {
    UINT32 Thread;
    UINT32 Reserved;
    UINT64 Count;
} ACTIVITY_ID;

C_ASSERT(sizeof(ACTIVITY_ID) == sizeof(GUID));

// Forward declaration
class ThreadTargetState;

class ThreadParameters;

class IoRing
{
public:
    IoRing();
    HRESULT Initialize(ThreadParameters* pThreadParameters);

    ~IoRing()
    {
        if (_hIoRing != NULL)
        {
            if (s_pfnCloseIoRing != nullptr)
            {
                s_pfnCloseIoRing(_hIoRing);
            }
            _hIoRing = NULL;
        }

        if (_pBufferInfo != NULL)
        {
            delete[] _pBufferInfo;
            _pBufferInfo = NULL;
        }
    }

    HIORING GetHandle() const { return _hIoRing; }

    IORING_BUFFER_REF GetReadBufferRef(UINT32 iTarget, UINT32 iRequest);
    IORING_BUFFER_REF GetWriteBufferRef(UINT32 iTarget, UINT32 iRequest);

private:
    ThreadParameters *_tp;
    HIORING _hIoRing;
    bool _useRegBuffer;

    UINT32 _bufferCount;
    IORING_BUFFER_INFO* _pBufferInfo;
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

    ~ThreadParameters()
    {
        for (auto pBuffer : vpDataBuffers)
        {
            if (pBuffer != nullptr)
            {
                VirtualFree(pBuffer, 0, MEM_RELEASE);
            }
        }
    }

    const Profile *pProfile;
    const TimeSpan *pTimeSpan;

    vector<Target> vTargets;
    vector<ThreadTargetState> vTargetStates;
    vector<HANDLE> vhTargets;

    vector<size_t> vulReadBufferSize;
    vector<BYTE *> vpDataBuffers;
    vector<IORequest> vIORequest;
    vector<ThroughputMeter> vThroughputMeters;

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

    IoRing ioRing;

    bool AllocateAndFillBufferForTarget(Target& target);
    BYTE* GetReadBuffer(size_t iTarget, size_t iRequest);
    BYTE* GetWriteBuffer(size_t iTarget, size_t iRequest);
    DWORD GetTotalRequestCount() const;
    DWORD GetTargetRequestCount(const Target& target) const;
    size_t GetTargetBufferLength(const Target& target) const;
    bool  InitializeMappedViewForTarget(Target& target, DWORD DesiredAccess);

    GUID NextActivityId()
    {
        GUID ActivityId;
        ACTIVITY_ID* ActivityGuid = (ACTIVITY_ID*)&ActivityId;

        ActivityGuid->Thread = ulThreadNo;
        ActivityGuid->Reserved = 0;
        // The count is byte swapped so it's understandable in a trace.
        ActivityGuid->Count = _byteswap_uint64(++_ullActivityCount);

        return ActivityId;
    }

private:
    ThreadParameters(const ThreadParameters& T);
    UINT64 _ullActivityCount;
};

class ThreadTargetState
{
    public:

    ThreadTargetState(
        const ThreadParameters *pTp,
        size_t iTarget,
        UINT64 targetSize
    ) :
        _tp(pTp),
        _target(&_tp->vTargets[iTarget]),
        _targetSize(targetSize),
        _mode(_target->GetIOMode()),

        _nextSeqOffset(0),
        _lastIO(IOOperation::Unknown),
        _sharedSeqOffset(nullptr)
    {
        //
        // Now calculate the maximum base-relative file offset that IO can be issued at.
        //
        // Trim by max file size limit, and reduce by base file offset.
        //

        if (_target->GetMaxFileSize())
        {
            _relTargetSize = _targetSize > _target->GetMaxFileSize() ? _target->GetMaxFileSize() : _targetSize;
        }
        else
        {
            _relTargetSize = _targetSize;
        }

        _relTargetSize -= _target->GetBaseFileOffsetInBytes();

        //
        // Align relative to the maximum offset at which aligned IO could be issued at.
        //

        _relTargetSizeAligned = _relTargetSize - _target->GetBlockSizeInBytes();
        _relTargetSizeAligned -= _relTargetSizeAligned % _target->GetBlockAlignmentInBytes();
        _relTargetSizeAligned += _target->GetBlockAlignmentInBytes();

        // Grab the shared sequential pointer if this is interlocked.

        if (_mode == IOMode::InterlockedSequential)
        {
            assert(_tp->pullSharedSequentialOffsets != nullptr);
            _sharedSeqOffset = &_tp->pullSharedSequentialOffsets[iTarget];
        }

        // Convert and finalize the random distribution stated in the target using final bounds.

        if (!_target->GetDistribution().IsEmpty())
        {
            _distribution = _target->GetDistribution();
            _distribution.Finalize(_relTargetSizeAligned, _relTargetSize,
                                   _target->GetBlockSizeInBytes(), _target->GetBlockAlignmentInBytes());
        }

        Reset();
    }

    //
    // Reset IO pointer/type state to initial conditions.
    //

    VOID Reset()
    {
        //
        // Now set the (base-relative) initial sequential offset
        //  * sequential: based on thread stride
        //  * mixed: randomized starting position
        //
        // Note this is repeated for ParallelAsync initialization since sequential offset is in the IO request there.
        //

        switch (_mode)
        {
            case IOMode::Sequential:
            _nextSeqOffset = _target->GetThreadBaseRelativeOffsetInBytes(_tp->ulRelativeThreadNo);
            break;

            case IOMode::Mixed:
            _nextSeqOffset = NextRelativeRandomOffset();
            break;

            default:
            break;
        }

        _lastIO = NextIOType(true);
    }

    //
    // Validate whether this thread can start IO given thread stride and file size.
    //

    bool CanStart()
    {
        UINT64 startingFileOffset = _target->GetThreadBaseRelativeOffsetInBytes(_tp->ulRelativeThreadNo);

        if (startingFileOffset + _target->GetBlockSizeInBytes() > _relTargetSize)
        {
            return false;
        }

        return true;
    }

    UINT64 TargetSize()
    {
        return _targetSize;
    }

    VOID InitializeParallelAsyncIORequest(IORequest& ioRequest) const
    {
        ULARGE_INTEGER initialOffset;

        //
        // Bias backwards by one IO so that this functions as the last-IO-issued pointer.
        // It will be incremented to the expected first offset. Note: absolute offset.
        //

        initialOffset.QuadPart = _target->GetThreadBaseFileOffsetInBytes(_tp->ulRelativeThreadNo) - _target->GetBlockAlignmentInBytes();

        ioRequest.GetOverlapped()->Offset = initialOffset.LowPart;
        ioRequest.GetOverlapped()->OffsetHigh = initialOffset.HighPart;
    }

    UINT64 NextRelativeSeqOffset()
    {
        UINT64 nextOffset;

        nextOffset = _nextSeqOffset;

        // Wrap?

        if (nextOffset + _target->GetBlockSizeInBytes() > _relTargetSize) {
            nextOffset = _target->GetThreadBaseRelativeOffsetInBytes(_tp->ulRelativeThreadNo) % _target->GetBlockAlignmentInBytes();
        }

        _nextSeqOffset = nextOffset + _target->GetBlockAlignmentInBytes();

        return nextOffset;
    }

    UINT64 NextRelativeInterlockedSeqOffset()
    {
        UINT64 nextOffset;

        // advance shared and rewind to get offset to use
        nextOffset = InterlockedAdd64((PLONG64) _sharedSeqOffset, _target->GetBlockAlignmentInBytes());
        nextOffset -=  _target->GetBlockAlignmentInBytes();

        nextOffset %= _relTargetSizeAligned;
        return nextOffset;
    }

    UINT64 NextRelativeParaSeqOffset(IORequest& ioRequest)
    {
        ULARGE_INTEGER nextOffset;

        //
        // Note: parallel seq differs from the other sequential cases in that the
        // pointer indicates the prior IO, not the offset to issue the current at.
        // Advance it.
        //

        nextOffset.LowPart = ioRequest.GetOverlapped()->Offset;
        nextOffset.HighPart = ioRequest.GetOverlapped()->OffsetHigh;
        nextOffset.QuadPart -= _target->GetBaseFileOffsetInBytes();     // absolute -> relative
        nextOffset.QuadPart += _target->GetBlockAlignmentInBytes();     // advance past last IO (!)

        // Wrap?

        if (nextOffset.QuadPart + _target->GetBlockSizeInBytes() > _relTargetSize) {
            nextOffset.QuadPart = _target->GetThreadBaseRelativeOffsetInBytes(_tp->ulRelativeThreadNo) % _target->GetBlockAlignmentInBytes();
        }

        return nextOffset.QuadPart;
    }

    UINT64 NextRelativeRandomOffset() const
    {
        UINT64 nextOffset = _tp->pRand->Rand64();
        nextOffset -= nextOffset % _target->GetBlockAlignmentInBytes();

        //
        // With a distribution we choose by bucket. Note the bucket is already aligned.
        //

        if (_distribution.HasRanges())
        {
            auto r = DistributionRange::find(_distribution.GetRanges(), _tp->pRand->Rand64() % _distribution.GetIOSpan());
            nextOffset %= r->_dst.second;   // trim to range length (already aligned)
            nextOffset += r->_dst.first;    // bump by range base
        }
        // Full width.
        else
        {
            nextOffset %= _relTargetSizeAligned;
        }

        return nextOffset;
    }

    UINT64 NextRelativeMixedOffset(bool& fRandom)
    {
        ULARGE_INTEGER nextOffset;

        fRandom = Util::BooleanRatio(_tp->pRand, _target->GetRandomRatio());

        if (fRandom)
        {
            nextOffset.QuadPart = NextRelativeRandomOffset();
            _nextSeqOffset = nextOffset.QuadPart + _target->GetBlockAlignmentInBytes();
            return nextOffset.QuadPart;
        }

        return NextRelativeSeqOffset();
    }

    IOOperation NextIOType(bool newType)
    {
        IOOperation ioType;

        if (_target->GetWriteRatio() == 0)
        {
           ioType = IOOperation::ReadIO;
        }
        else if (_target->GetWriteRatio() == 100)
        {
            ioType = IOOperation::WriteIO;
        }
        else if (_mode == IOMode::Mixed && !newType)
        {
            // repeat last IO if not needing a new choice (e.g., random)
            ioType = _lastIO;
        }
        else
        {
            ioType = Util::BooleanRatio(_tp->pRand, _target->GetWriteRatio()) ? IOOperation::WriteIO : IOOperation::ReadIO;
            _lastIO = ioType;
        }

        return ioType;
    }

    void NextIORequest(IORequest &ioRequest)
    {
        bool fRandom = false;
        ULARGE_INTEGER nextOffset = { 0 };

        switch (_mode)
        {
            case IOMode::Sequential:
            nextOffset.QuadPart = NextRelativeSeqOffset();
            break;

            case IOMode::InterlockedSequential:
            nextOffset.QuadPart = NextRelativeInterlockedSeqOffset();
            break;

            case IOMode::ParallelAsync:
            nextOffset.QuadPart = NextRelativeParaSeqOffset(ioRequest);
            break;

            case IOMode::Mixed:
            nextOffset.QuadPart = NextRelativeMixedOffset(fRandom);
            break;

            case IOMode::Random:
            nextOffset.QuadPart = NextRelativeRandomOffset();
            fRandom = true;
            break;

            default:
            assert(false);
        }

        //
        //  Convert relative offset to absolute.
        //

        nextOffset.QuadPart += _target->GetBaseFileOffsetInBytes();

        //
        // Move offset into the IO request and decide what IO type will be issued.
        // Mixed which has chosen sequential will repeat last IO type so that seq
        // runs are homogeneous.
        //

        ioRequest.GetOverlapped()->Offset = nextOffset.LowPart;
        ioRequest.GetOverlapped()->OffsetHigh = nextOffset.HighPart;
        ioRequest.SetIoType(NextIOType(fRandom));
    }

    private:

    const ThreadParameters *_tp;
    const Target *_target;
    const UINT64 _targetSize;   // unmodified absolute target size
    const IOMode _mode;         // thread's mode of IO operations to this target (Random, Sequential, etc.)

    //
    // Offsets/sizes are zero-based relative to target base offset, not absolute file offset.
    // Relative size is trimmed with respect to block alignment, if specified.
    //

    UINT64 _relTargetSize;              // relative target size for IO v. base/max
    UINT64 _relTargetSizeAligned;       // relative target size for zero-base aligned IO (applies to: Random, InterlockedSequential)
    UINT64 _nextSeqOffset;              // next IO offset to issue sequential IO at (applies to: Sequential & Mixed)
    volatile UINT64 *_sharedSeqOffset;  // ... for interlocked IO (applies to: InterlockedSequential)
    IOOperation _lastIO;                // last IO type (applies to: Mixed)

public:

    //
    // Random distribution (stated in absolute offsets of target)
    //

    Distribution _distribution;

    friend class UnitTests::IORequestGeneratorUnitTests;
};

class IResultParser
{
public:
    virtual string ParseResults(const Profile& profile, const SystemInformation& system, vector<Results> vResults) = 0;
    virtual string ParseProfile(const Profile& profile) = 0;
    virtual string ParseSystemInformation(const SystemInformation& system) = 0;
};

class EtwResultParser
{
public:
    static void ParseResults(vector<Results> vResults);

private:
    static void _WriteResults(IOOperation type, const TargetResults& targetResults, size_t uThread);
};
