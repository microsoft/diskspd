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

#include "CmdLineParser.h"
#include "Common.h"
#include "XmlProfileParser.h"
#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>

CmdLineParser::CmdLineParser() :
    _dwBlockSize(64 * 1024),
    _ulWriteRatio(0),
    _hEventStarted(nullptr),
    _hEventFinished(nullptr)
{
}

CmdLineParser::~CmdLineParser()
{
}

// Get size in bytes from a string (KMGTb)
bool CmdLineParser::_GetSizeInBytes(const char *pszSize, UINT64& ullSize, const char **pszRest) const
{
    bool fOk = true;
    UINT64 ullResult = 0;
    UINT64 ullMultiplier = 1;
    const char *rest = nullptr;

    fOk = Util::ParseUInt(pszSize, ullResult, rest);

    if (fOk)
    {
        char ch = static_cast<char>(toupper(*rest));

        switch (ch)
        {
            case '\0':  { break; }
            case 'T':   { ullMultiplier *= 1024; }
            case 'G':   { ullMultiplier *= 1024; }
            case 'M':   { ullMultiplier *= 1024; }
            case 'K':   { ullMultiplier *= 1024;        ++rest; break; }
            case 'B':   { ullMultiplier = _dwBlockSize; ++rest; break; }
            default: {

                //
                // If caller is not expecting continuation, we know this is malformed now and
                // can say so with respect to size specifiers.
                // If there is continuation, caller is responsible for validating.
                //

                if (!pszRest)
                {
                    fOk = false;
                    fprintf(stderr, "Invalid size '%c'. Valid: K - KiB, M - MiB, G - GiB, T - TiB, B - block\n", *rest);
                }
            }
        }

        if (fOk)
        {
            //
            // Second chance after parsing valid size qualifier.
            //

            if (!pszRest && *rest != '\0')
            {
                fOk = false;
                fprintf(stderr, "Unrecognized characters after size specification\n");
            }

            //
            // Now apply size specifier.
            //

            else if (ullResult <= MAXUINT64 / ullMultiplier)
            {
                ullResult *= ullMultiplier;
            }
            else
            {
                // overflow
                fOk = false;
                fprintf(stderr, "Overflow applying multipler '%c'\n", ch);
            }
        }
    }
    else
    {
        fprintf(stderr, "Invalid integer\n");
    }

    if (fOk)
    {
        ullSize = ullResult;

        if (pszRest)
        {
            *pszRest = rest;
        }
    }

    return fOk;
}

bool CmdLineParser::_GetRandomDataWriteBufferData(const string& sArg, UINT64& cb, string& sPath)
{
    bool fOk = true;
    size_t iComma = sArg.find(',');
    if (iComma == sArg.npos)
    {
        fOk = _GetSizeInBytes(sArg.c_str(), cb, nullptr);
        sPath = "";
    }
    else
    {
        fOk = _GetSizeInBytes(sArg.substr(0, iComma).c_str(), cb, nullptr);
        sPath = sArg.substr(iComma + 1);
    }
    return fOk;
}

void CmdLineParser::_DisplayUsageInfo(const char *pszFilename) const
{
    // ISSUE-REVIEW: this formats badly in the default 80 column command prompt
    printf("\n");
    printf("Usage: %s [options] target1 [ target2 [ target3 ...] ]\n", pszFilename);
    printf("version %s (%s)\n", DISKSPD_NUMERIC_VERSION_STRING, DISKSPD_DATE_VERSION_STRING);
    printf("\n");

    printf(
        "Valid targets:\n"
        "     file_path\n"
        "     #<physical drive number>\n"
        "     <drive_letter>:\n"
        "\n"
        "Sizes, offsets and lengths are specified as integer bytes, or with an\n"
        "optional suffix of KMGT (KiB/MiB/GiB/TiB) or b (for blocks, see -b).\n"
        "Examples: 4k = 4096\n"
        "          with -b4k, 8b = 32768 (8 * 4KiB)\n"
        "\n"
        "Available options:\n"
        "  -?                    display usage information\n"
        "  -:<flags>             experimental behaviors, as a bitmask of flags. current:\n"
        "                          1 - allow throughput rate limit sleeps >1ms if indicated by rate\n"
        "  -ag                   group affinity - threads assigned round-robin to CPUs by processor groups, 0 - n.\n"
        "                          By default, P-cores are ordered before E-cores within each group (BIG.little-aware).\n"
        "                          Groups are fully consumed before moving to the next.\n"
        "                          [default; use -n to fully disable affinity]\n"
        "  -ac                   SMT/core-aware assignment - 1st thread per core placed before 2nd (opt-in)\n"
        "  -as                   span groups - interleave across all groups before filling each\n"
        "  -ap/-aP/-ae/-aE       efficiency class ordering on heterogeneous BIG.little (P/E core) systems:\n"
        "                          p : P-cores first within each pass [default]\n"
        "                          P : fill all P-core passes before any E-core\n"
        "                          e : E-cores first within each pass\n"
        "                          E : fill all E-core passes before any P-core\n"
        "                          Composes with -ac and -as (e.g. -acsE, -aPc).\n"
        "  -aup                  unset P/E ordering default (no efficiency class reordering)\n"
        "  -a#[,#,...]           explicit CPU affinity masks - threads assigned by the effective ordering\n"
        "  -ag#,#[,#,...]...       policies; g# is processor group for the following CPUs. If no group stated, 0 is default.\n"
        "  -ag#                    CPU ranges can be specified with N-M syntax.\n"
        "                          Use -ag# for whole-group. -a#[,#,...] short format specifies CPUs within group 0.\n"
        "                          Multiple masks combine in order to form the final CPU affinity mask/ordering.\n"
        "                          Examples: -a0,1,2 and -ag0,0,1,2 are equivalent.\n"
        "                                    -ag0,0-10,15-20 specifies CPUs 0-10 and 15-20 in group 0.\n"
        "                                    -ag1 -ag0 specifies whole group 1 then whole group 0.\n"
        "  -b<size>              IO size, defines the block \'b\' for sizes stated in units of blocks [default=64K]\n"
        "  -bs<flag>             buffer separation strategy for thread IO buffers [default=p]\n"
        "                          n : no separation (system default allocation)\n"
        "                          p : thread optimized (pde cache line separation)\n"
        "  -B<base>[:length]     bounds; specify range of target to issue IO to - base offset and length\n"
        "                          (default: IO is issued across the entire target)\n"
        "  -c<size>              create file targets of the given size. Conflicts with non-file target specifications.\n"
        "  -C<seconds>           cool down time - duration of the test after measurements finished [default=0s].\n"
        "  -D<milliseconds>      Capture IOPs statistics in intervals of <milliseconds>; these are per-thread\n"
        "                          per-target: text output provides IOPs standard deviation, XML provides the full\n"
        "                          IOPs time series in addition. [default=1000, 1 second].\n"
        "  -d<seconds>           duration (in seconds) to run test [default=10s]\n"
        "  -f<size>              maximum target offset to issue IO to (non-inclusive); -Bbase -f(base+length) is the same\n"
        "                         as -Bbase:length. For example, to test only the first sectors of a disk.\n"
        "  -f<rst>               open file with one or more additional access hints\n"
        "                          r : the FILE_FLAG_RANDOM_ACCESS hint\n"
        "                          s : the FILE_FLAG_SEQUENTIAL_SCAN hint\n"
        "                          t : the FILE_ATTRIBUTE_TEMPORARY hint\n"
        "                          [default: none]\n"
        "  -F<count>             total number of threads (conflicts with -t)\n"
        "  -g<value>[i]          throughput per-thread per-target throttled to given value; defaults to bytes per millisecond\n"
        "                          With the optional i qualifier the value is IOPS of the specified block size (-b).\n"
        "                          Throughput limits cannot be specified when using completion routines (-x)\n"
        "                          [default: no limit]\n"
        "  -h                    deprecated, see -Sh\n"
        "  -i<count>             number of IOs per burst; see -j [default: inactive]\n"
        "  -j<milliseconds>      interval in <milliseconds> between issuing IO bursts; see -i [default: inactive]\n"
        "  -I<priority>          Set IO priority to <priority>. Available values are: 1-very low, 2-low, 3-normal (default)\n"
        "  -l                    Use large pages for IO buffers\n"
        "  -L                    measure latency statistics\n"
        "  -n                    disable default affinity (-a)\n"
        "  -N<vni>               specify the flush mode for memory mapped I/O\n"
        "                          v : uses the FlushViewOfFile API\n"
        "                          n : uses the RtlFlushNonVolatileMemory API\n"
        "                          i : uses RtlFlushNonVolatileMemory without waiting for the flush to drain\n"
        "                          [default: none]\n"
        "  -o<count>             number of outstanding I/O requests per target per thread\n"
        "                          (1=synchronous I/O, unless more than 1 thread is specified with -F)\n"
        "                          [default=2]\n");

    printf(
        "  -oc<count>            completion port IO batch depth (1-%u) [default=%u]\n"
        "                          controls the maximum number of IO completions dequeued per batch;\n"
        "                          only applies to IO Completion Port mode (conflicts with -x)\n",
        c_maximumCompletionDepth, c_defaultCompletionDepth);

    printf(
        "  -O<count>             number of outstanding I/O requests per thread - for use with -F\n"
        "                          (1=synchronous I/O)\n"
        "  -p                    start parallel sequential I/O operations with the same offset\n"
        "                          (ignored if -r is specified, makes sense only with -o2 or greater)\n"
        "  -P<count>             enable printing a progress dot after each <count> [default=65536]\n"
        "                          completed I/O operations, counted separately by each thread \n"
        "  -r[align]             random I/O aligned to [align] byte offsets within the target range (overrides -s)\n"
        "                          [default alignment=block size (-b)]\n"
        "  -rd<dist>[params]     specify an non-uniform distribution for random IO in the target\n"
        "                          [default uniformly random]\n"
        "                           distributions: pct, abs\n"
        "                           all:  IO%% and %%Target/Size are cumulative. If the sum of IO%% is less than 100%% the\n"
        "                                 remainder is applied to the remainder of the target. An IO%% of 0 indicates a gap -\n"
        "                                 no IO will be issued to that range of the target.\n"
        "                           pct : parameter is a combination of IO%%/%%Target separated by : (colon)\n"
        "                                 Example: -rdpct90/10:0/10:5/20 specifies 90%% of IO in 10%% of the target, no IO\n"
        "                                   next 10%%, 5%% IO in the next 20%% and the remaining 5%% of IO in the last 60%%\n"
        "                           abs : parameter is a combination of IO%%/Target Size separated by : (colon)\n"
        "                                 If the actual target size is smaller than the distribution, the relative values of IO%%\n"
        "                                 for the valid elements define the effective distribution.\n"
        "                                 Example: -rdabs90/10G:0/10G:5/20G specifies 90%% of IO in 10GiB of the target, no IO\n"
        "                                   next 10GiB, 5%% IO in the next 20GiB and the remaining 5%% of IO in the remaining\n"
        "                                   capacity of the target. If the target is only 20G, the distribution truncates at\n"
        "                                   90/10G:0:10G and all IO is directed to the first 10G (equivalent to -f10G).\n"
        "  -rs<percentage>       percentage of requests which should be issued randomly; -r is used to specify IO alignment.\n"
        "                          Sequential IO runs are homogeneous when a mixed r/w ratio is specified (-w) and their lengths\n"
        "                          follow a geometric distribution based on the percentage (chance of next IO being sequential).\n"
        "  -R[p|s]<text|xml>     output format\n"
        "                          p prefix: input profile (command line or XML) is validated and re-output without running\n"
        "                            load, useful for checking or building complex profiles\n"
        "                          s prefix: output system information only, and exit\n"
        "                          [default: text]\n"
        "  -s[i][align]          stride size of [align] bytes, alignment & offset between operations\n"
        "                          [default=non-interlocked, default alignment=block size (-b)]\n"
        "                          By default threads track independent sequential IO offsets starting at base offset of the target.\n"
        "                          With multiple threads this results in threads overlapping their IOs - see -T to divide\n"
        "                          them into multiple separate sequential streams on the target.\n"
        "                          With the optional i qualifier (-si) threads interlock on a shared sequential offset.\n"
        "                          Interlocked operations may introduce overhead but make it possible to issue a single\n"
        "                          sequential stream to a target which responds faster than one thread can drive.\n"
        "                          (ignored if -r specified, -si conflicts with -p, -rs and -T)\n"
        "  -S[bhmruwyY]          control caching and I/O path behavior (caching, writethrough, memory mapped I/O, BypassIO)\n"
        "                          [default: caching is enabled, no writethrough, no BypassIO]\n"
        "                          non-conflicting flags may be combined in any order; ex: -Sbw, -Suw, -Swu\n"
        "  -S                    equivalent to -Su\n"
        "  -Sb                   enable caching (default, explicitly stated)\n"
        "  -Sh                   equivalent -Suw\n"
        "  -Sm                   enable memory mapped I/O\n"
        "  -Sr                   disable local caching, with remote sw caching enabled; only valid for remote filesystems\n"
        "  -Su                   disable software caching, equivalent to FILE_FLAG_NO_BUFFERING\n"
        "  -Sw                   enable writethrough (no hardware write caching), equivalent to FILE_FLAG_WRITE_THROUGH or\n"
        "                          non-temporal writes for memory mapped I/O (-Sm)\n"
        "  -Sy                   use BypassIO to bypass file system, volume and storage stack filters; allows the test to continue with\n"
        "                          partial bypass if volume and storage stack filters cannot be bypassed but file system filters can be bypassed\n"
        "  -SY                   use BypassIO to bypass file system, volume and storage stack filters; test fails if both stacks cannot be bypassed\n"
        "  -t<count>             number of threads per target (conflicts with -F)\n"
        "  -T<offs>              starting separation between I/O operations performed on the same target by different threads\n"
        "                          [default=0] (starting offset = base target offset + (thread number * <offs>)\n"
        "                          only applies to -s sequential IO with #threads > 1, conflicts with -r and -si\n"
        "  -u[b]<percentage>     use IoRing for submitting I/O requests with optional registered buffers and batch size\n"
        "                          [default: 25%% batch size, no registered buffers]\n"
        "                          With b, use registered buffers with IoRing\n"
        "                          With batch size, specify the percentage of I/O requests to be queued before submitting\n"
        "                          Batch size is given as percentage of outstanding I/O requests per thread\n"
        "  -v[s]                 verbose mode - with s, only provide additional summary statistics\n"
        "  -w<percentage>        percentage of write requests (-w and -w0 are equivalent and result in a read-only workload).\n"
        "                        absence of this switch indicates 100%% reads\n"
        "                          IMPORTANT: a write test will destroy existing data without a warning\n"
        "  -W<seconds>           warm up time - duration of the test before measurements start [default=5s]\n"
        "  -x                    use completion routines instead of IO Completion Ports\n"
        "  -X<filepath>          use an XML file to configure the workload. Profile defaults for -W/d/C (durations) and -R/v/z\n"
        "                          (output format, verbosity and random seed) may be overriden by direct specification.\n"
        "                          Targets can be defined in XML profiles as template paths of the form *<integer> (*1, *2, ...).\n"
        "                          When run, specify the paths to substitute for the template paths in order on the command line.\n"
        "                          The first specified target is *1, second is *2, and so on.\n"
        "                          Example: diskspd -d60 -Xprof.xml first.bin second.bin (prof.xml using *1 and *2, 60s run)\n"
        "  -z[seed]              set random seed [with no -z, seed=0; with plain -z, seed is based on system run time]\n"
        "\n"
        "Write buffers:\n"
        "  -Z                    zero buffers used for write tests\n"
        "  -Zr                   per IO random buffers used for write tests - this incurrs additional run-time\n"
        "                         overhead to create random content and shouln't be compared to results run\n"
        "                         without -Zr\n"
        "  -Z<size>              use a <size> buffer filled with random data as a source for write operations.\n"
        "  -Z<size>,<file>       use a <size> buffer filled with data from <file> as a source for write operations.\n"
        "\n"
        "  By default, write source buffers are filled with a repeating pattern (0, 1, 2, ..., 255, 0, 1, ...)\n"
        "\n"
        "Synchronization:\n"
        "  -ys<eventname>     signals event <eventname> before starting the actual run (no warmup)\n"
        "                       (creates a notification event if <eventname> does not exist)\n"
        "  -yf<eventname>     signals event <eventname> after the actual run finishes (no cooldown)\n"
        "                       (creates a notification event if <eventname> does not exist)\n"
        "  -yr<eventname>     waits on event <eventname> before starting the run (including warmup)\n"
        "                       (creates a notification event if <eventname> does not exist)\n"
        "  -yp<eventname>     stops the run when event <eventname> is set; CTRL+C is bound to this event\n"
        "                       (creates a notification event if <eventname> does not exist)\n"
        "  -ye<eventname>     sets event <eventname> and quits\n"
        "\n"
        "Event Tracing:\n"
        "  -e<q|c|s>             Use query perf timer (qpc), cycle count, or system timer respectively.\n"
        "                          [default = q, query perf timer (qpc)]\n"
        "  -ep                   use paged memory for the NT Kernel Logger [default=non-paged memory]\n"
        "  -ePROCESS             process start & end\n"
        "  -eTHREAD              thread start & end\n"
        "  -eIMAGE_LOAD          image load\n"
        "  -eDISK_IO             physical disk IO\n"
        "  -eMEMORY_PAGE_FAULTS  all page faults\n"
        "  -eMEMORY_HARD_FAULTS  hard faults only\n"
        "  -eNETWORK             TCP/IP, UDP/IP send & receive\n"
        "  -eREGISTRY            registry calls\n"
        "\n\n");

    printf("Examples:\n\n");
    printf("Create 8192KB file and run read test on it for 1 second:\n\n");
    printf("  %s -c8192K -d1 testfile.dat\n", pszFilename);
    printf("\n");
    printf("Set block size to 4KB, create 2 threads per file, 32 overlapped (outstanding)\n");
    printf("I/O operations per thread, disable all caching mechanisms and run block-aligned random\n");
    printf("access read test lasting 10 seconds:\n\n");
    printf("  %s -b4K -t2 -r -o32 -d10 -Sh testfile.dat\n\n", pszFilename);
    printf("Create two 1GB files, set block size to 4KB, create 2 threads per file, affinitize threads\n");
    printf("to CPUs 0 and 1 (each file will have threads affinitized to both CPUs) and run read test\n");
    printf("lasting 10 seconds:\n\n");
    printf("  %s -c1G -b4K -t2 -d10 -a0,1 testfile1.dat testfile2.dat\n", pszFilename);

    printf("\n");
}

bool CmdLineParser::_ParseETWParameter(const char *arg, Profile *pProfile)
{
    assert(nullptr != arg);
    assert(0 != *arg);

    bool fOk = true;
    pProfile->SetEtwEnabled(true);
    if (*(arg + 1) != '\0')
    {
        const char *c = arg + 1;
        if (*c == 'p')
        {
            pProfile->SetEtwUsePagedMemory(true);
        }
        else if (*c == 'q')
        {
            pProfile->SetEtwUsePerfTimer(true);
        }
        else if (*c == 's')
        {
            pProfile->SetEtwUseSystemTimer(true);       //default
        }
        else if (*c == 'c')
        {
            pProfile->SetEtwUseCyclesCounter(true);
        }
        else if (strcmp(c, "PROCESS") == 0)             //process start & end
        {
            pProfile->SetEtwProcess(true);
        }
        else if (strcmp(c, "THREAD") == 0)              //thread start & end
        {
            pProfile->SetEtwThread(true);
        }
        else if (strcmp(c, "IMAGE_LOAD") == 0)          //image load
        {
            pProfile->SetEtwImageLoad(true);
        }
        else if (strcmp(c, "DISK_IO") == 0)             //physical disk IO
        {
            pProfile->SetEtwDiskIO(true);
        }
        else if (strcmp(c, "MEMORY_PAGE_FAULTS") == 0)  //all page faults
        {
            pProfile->SetEtwMemoryPageFaults(true);
        }
        else if (strcmp(c, "MEMORY_HARD_FAULTS") == 0)  //hard faults only
        {
            pProfile->SetEtwMemoryHardFaults(true);
        }
        else if (strcmp(c, "NETWORK") == 0)             //tcpip send & receive
        {
            pProfile->SetEtwNetwork(true);
        }
        else if (strcmp(c, "REGISTRY") == 0)            //registry calls
        {
            pProfile->SetEtwRegistry(true);
        }
        else
        {
            fOk = false;
        }
    }
    else
    {
        fOk = false;
    }

    return fOk;
}

// Parse affinity specification.
//
// Forms:
//   -a            default group affinity (no-op)
//   -ag           default group affinity (no-op)
//   -ag<num>      whole-group effective (mask=0)
//   -a[g<num>,]<cpuspec>[,<cpuspec>...][,g<num>,<cpuspec>...]
//
// Grammar for CPU-specified form: item (',' item)*
//   item: 'g' <number> ',' cpuspec   (group switch; must be followed by CPU)
//       | cpuspec                     (CPU in current group)
//   cpuspec: <number> '-' <number>    (range, inclusive)
//          | <number>                 (single CPU)

bool CmdLineParser::_ParseAffinity(const char *arg, TimeSpan *pTimeSpan)
{
    assert(nullptr != arg);
    assert('\0' != *arg);

    const char *c = arg + 1;
    DWORD nGroup = 0;
    DWORD n = 2; // count of characters consumed into the switch; so far '-a'

    //
    // Parse lambda for consuming character at current position
    //

    auto consumeChar = [&](void) -> void
    {
        // advance parser and total characters consumed
        c++; n++;
        return;
    };

    //
    // Parse lambda for consuming number from current position
    //

    auto consumeNumber = [&](DWORD* pVal) -> bool
    {
        DWORD last = 0;
        DWORD v = 0;

        if (!isdigit((unsigned char)*c))
        {
            return false;
        }

        while (isdigit((unsigned char)*c))
        {
            v = 10 * v + (*c - '0');
            if (v < last)
            {
                return false; // defensive check for overflow
            }
            last = v;
            consumeChar();
        }

        *pVal = v;
        return true;
    };

    //
    // Parse lambda for consuming parsed cpu#
    //

    auto emitCpu = [&](DWORD cpu) -> bool
    {
        if (cpu > c_maxCpuIndexPerGroup)
        {
            fprintf(stderr, "ERROR: CPU %u is out of range\n", cpu);
            return false;
        }
        pTimeSpan->AddAffinityGroupMaskCpu((WORD)nGroup, (BYTE)cpu);
        return true;
    };

    //
    // Parse lambda for consuming parsed cpu# range
    //

    auto emitRange = [&](DWORD first, DWORD last) -> bool {
        if (last < first)
        {
            fprintf(stderr, "ERROR: CPU range %u-%u is inverted\n", first, last);
            return false;
        }
        if (last > c_maxCpuIndexPerGroup)
        {
            fprintf(stderr, "ERROR: CPU range %u-%u is out of range\n", first, last);
            return false;
        }
        for (DWORD cpu = first; cpu <= last; cpu++)
        {
            pTimeSpan->AddAffinityGroupMaskCpu((WORD)nGroup, (BYTE)cpu);
        }
        return true;
    };

    // -a or -ag with no further specification: default affinity; no-ops
    if (*c == '\0')
    {
        return true;
    }
    if (*c == 'g' && *(c + 1) == '\0')
    {
        return true;
    }

    // -au prefix: standalone switch for unsetting defaults (e.g., -aup for unordered efficiency)
    if (*c == 'u')
    {
        consumeChar();
        switch (*c)
        {
        case 'p':
            consumeChar();
            if (*c != '\0')
            {
                fprintf(stderr, "ERROR: unexpected character '%c' after -aup\n", *c);
                return false;
            }
            if (pTimeSpan->GetAffinityEfficiencyOrder(false) != AffinityEfficiencyOrder::Unspecified)
            {
                fprintf(stderr, "ERROR: -aup conflicts with explicit efficiency ordering\n");
                return false;
            }
            pTimeSpan->SetAffinityEfficiencyOrder(AffinityEfficiencyOrder::Unordered);
            return true;
        case '\0':
            fprintf(stderr, "ERROR: -au requires an option (e.g., -aup)\n");
            return false;
        default:
            fprintf(stderr, "ERROR: unknown -au option '%c'\n", *c);
            return false;
        }
    }

    // Composing affinity modifiers: c (CoreAware), s (Span), p/P/e/E (efficiency order)
    // c and s compose freely and are idempotent. Efficiency modifiers are mutually exclusive.
    if (*c == 'c' || *c == 's' ||
        *c == 'p' || *c == 'P' ||
        *c == 'e' || *c == 'E')
    {
        AffinityEfficiencyOrder order = AffinityEfficiencyOrder::Unspecified;

        while (*c != '\0')
        {
            switch (*c)
            {
            case 'c':
                pTimeSpan->SetAffinityTraversal(AffinityTraversal::CoreAware);
                consumeChar();
                break;
            case 's':
                pTimeSpan->SetAffinityGroupSpan(AffinityGroupSpan::Span);
                consumeChar();
                break;
            case 'p':
                if (order != AffinityEfficiencyOrder::Unspecified)
                {
                    goto error_multiple_efficiency;
                }
                order = AffinityEfficiencyOrder::PFirst;
                consumeChar();
                break;
            case 'P':
                if (order != AffinityEfficiencyOrder::Unspecified)
                {
                    goto error_multiple_efficiency;
                }
                order = AffinityEfficiencyOrder::FillPFirst;
                consumeChar();
                break;
            case 'e':
                if (order != AffinityEfficiencyOrder::Unspecified)
                {
                    goto error_multiple_efficiency;
                }
                order = AffinityEfficiencyOrder::EFirst;
                consumeChar();
                break;
            case 'E':
                if (order != AffinityEfficiencyOrder::Unspecified)
                {
                    goto error_multiple_efficiency;
                }
                order = AffinityEfficiencyOrder::FillEFirst;
                consumeChar();
                break;

            default:
                fprintf(stderr, "ERROR: unexpected character '%c' in affinity modifier\n", *c);
                goto error_at_c;
            }
        }

        if (order != AffinityEfficiencyOrder::Unspecified)
        {
            // Conflict with prior -aup or prior explicit efficiency (from a separate -a switch)
            if (pTimeSpan->GetAffinityEfficiencyOrder(false) != AffinityEfficiencyOrder::Unspecified)
            {
                if (pTimeSpan->GetAffinityEfficiencyOrder() == AffinityEfficiencyOrder::Unordered)
                {
                    fprintf(stderr, "ERROR: explicit efficiency ordering conflicts with -aup\n");
                }
                else
                {
                    fprintf(stderr, "ERROR: multiple efficiency order specifiers are not allowed\n");
                }
                return false;
            }
            pTimeSpan->SetAffinityEfficiencyOrder(order);
        }

        return true;

error_multiple_efficiency:

        fprintf(stderr, "ERROR: multiple efficiency order specifiers are not allowed\n");
        return false;

    }

    // Main parse loop: item (',' item)*
    bool fFirstItem = true;
    for (;;)
    {
        // Group switch: 'g' <number>
        if (*c == 'g')
        {
            consumeChar();
            DWORD g;
            if (!consumeNumber(&g))
            {
                goto error_at_c;
            }
            if (g > MAXWORD)
            {
                fprintf(stderr, "ERROR: group %u is out of range\n", g);
                return false;
            }
            nGroup = g;

            // g<num> at EOL: whole-group if this is the only item, error otherwise
            if (*c == '\0')
            {
                if (fFirstItem)
                {
                    pTimeSpan->AddAffinityGroupMask((WORD)nGroup, (KAFFINITY)0);
                    return true;
                }
                fprintf(stderr, "ERROR: group %u specified without CPU numbers; use separate -ag%u for whole-group\n", g, g);
                return false;
            }

            // CPU-specified form: group must be followed by comma then CPU spec
            if (*c != ',')
            {
                goto error_at_c;
            }
            consumeChar();
            fFirstItem = false;

            if (*c == '\0')
            {
                goto error_incomplete;
            }
            if (*c == 'g')
            {
                continue;
            }
        }

        // CPU spec: <number> ['-' <number>]
        DWORD cpu;
        if (!consumeNumber(&cpu))
        {
            goto error_at_c;
        }

        if (*c == '-')
        {
            consumeChar();
            DWORD cpuEnd;
            if (!consumeNumber(&cpuEnd))
            {
                goto error_at_c;
            }
            if (!emitRange(cpu, cpuEnd))
            {
                return false;
            }
        }
        else
        {
            if (!emitCpu(cpu))
            {
                return false;
            }
        }

        fFirstItem = false;

        // After item: comma continues, EOL terminates, anything else is error
        if (*c == '\0')
        {
            return true;
        }
        if (*c != ',')
        {
            goto error_at_c;
        }
        consumeChar();

        if (*c == '\0')
        {
            goto error_incomplete;
        }
    }

error_at_c:
    fprintf(stderr, "ERROR: syntax error parsing affinity at highlighted character\n-%s\n", arg);
    {
        DWORD pos = n;
        while (pos-- > 0)
        {
            fprintf(stderr, " ");
        }
    }
    fprintf(stderr, "^\n");
    return false;

error_incomplete:
    fprintf(stderr, "ERROR: incomplete affinity specification\n");
    return false;
}

bool CmdLineParser::_ParseFlushParameter(const char *arg, MemoryMappedIoFlushMode *FlushMode)
{
    assert(nullptr != arg);
    assert(0 != *arg);

    bool fOk = true;
    if (*(arg + 1) != '\0')
    {
        const char *c = arg + 1;
        if (strcmp(c, "v") == 0)
        {
            *FlushMode = MemoryMappedIoFlushMode::ViewOfFile;
        }
        else if (strcmp(c, "n") == 0)
        {
            *FlushMode = MemoryMappedIoFlushMode::NonVolatileMemory;
        }
        else if (strcmp(c, "i") == 0)
        {
            *FlushMode = MemoryMappedIoFlushMode::NonVolatileMemoryNoDrain;
        }
        else
        {
            fOk = false;
        }
    }
    else
    {
        fOk = false;
    }
    return fOk;
}

bool CmdLineParser::_ParseRandomDistribution(const char *arg, vector<Target>& vTargets)
{
    vector<DistributionRange> vOr;
    DistributionType dType;
    bool fOk = false;
    UINT32 pctAcc = 0, pctCur;          // accumulated/cur pct io
    UINT64 targetAcc = 0, targetCur;    // accumulated/cur target

    if (!strncmp(arg, "pct", 3))
    {
        dType = DistributionType::Percent;
    }
    else if (strncmp(arg, "abs", 3))
    {
        fprintf(stderr, "Unrecognized random distribution type\n");
        return false;
    }
    else
    {
        dType = DistributionType::Absolute;
    }

    arg += 3;

    //
    // Parse pairs of
    //
    //  * pct:  percentage/target percentage
    //  * abs:  percentage/absolute range of target
    //
    // Ex: 90/10:5/5 => [0,90) -> [0, 10) :: [90, 95) -> [10, 15)
    //  a remainder of [95, 100) -> [15, 100) would be applied.
    //
    // Percentages are cumulative and successively define the span of
    // the preceding definition. Absolute ranges are also cumulative:
    // 10/1G:90/1G puts 90% of accesses in the second 1G range of the
    // target.
    //
    // A single percentage can be 100 but is of limited value since it
    // would only be valid as a single element distribution.
    //
    // Basic numeric validations are done here (similar to XSD for XML).
    // Cross validation with other workload parameters (blocksize) and whole
    // distribution validation is delayed to common code.
    //

    while (true)
    {
        // Consume IO% integer
        fOk = Util::ParseUInt(arg, pctCur, arg);
        if (!fOk)
        {
            fprintf(stderr, "Invalid integer IO%%: must be > 0 and <= %u\n", 100 - pctAcc);
            return false;
        }
        // hole is ok
        else if (pctCur > 100)
        {
            fprintf(stderr, "Invalid IO%% %u: must be >= 0 and <= %u\n", pctCur, 100 - pctAcc);
            return false;
        }

        // Expect separator
        if (*arg++ != '/')
        {
            fprintf(stderr, "Expected / separator after %u\n", pctCur);
            return false;
        }

        // Consume Target%/Absolute range integer
        if (dType == DistributionType::Percent)
        {
            // Percent specification
            fOk = Util::ParseUInt(arg, targetCur, arg);
            if (!fOk)
            {
                fprintf(stderr, "Invalid integer Target%%: must be > 0 and <= %I64u\n", 100 - targetAcc);
                return false;
            }
            // no hole
            else if (targetCur == 0 || targetCur > 100)
            {
                fprintf(stderr, "Invalid Target%% %I64u: must be > 0 and <= %I64u\n", targetCur, 100 - targetAcc);
                return false;
            }
        }
        else
        {
            // Size specification
            fOk = CmdLineParser::_GetSizeInBytes(arg, targetCur, &arg);
            if (!fOk)
            {
                // error already emitted
                return fOk;
            }

            if (targetCur == 0)
            {
                fprintf(stderr, "Invalid zero length target range\n");
                return false;
            }
        }

        // Add range from [accumulator - accumulator + current) => ...
        // Note that zero pctCur indicates a hole where no IO is desired - this is recorded
        // for fidelity of display/profile but will never match on lookup, as intended.
        vOr.emplace_back(pctAcc, pctCur, make_pair(targetAcc, targetCur));

        // Now move accumulators for the next tuple/completion
        pctAcc += pctCur;
        targetAcc += targetCur;

        // Expect/consume separator for next tuple?
        if (*arg == ':')
        {
            ++arg;
            continue;
        }

        // Done?
        if (*arg == '\0')
        {
            break;
        }

        fprintf(stderr, "Unexpected characters in specification '%s'\n", arg);
        return false;
    }

    // Apply to all targets
    for (auto& t : vTargets)
    {
        t.SetDistributionRange(vOr, dType);
    }

    return true;
}

bool CmdLineParser::_ReadParametersFromCmdLine(const int argc, const char *argv[], Profile *pProfile, struct Synchronization *synch, bool& fXMLProfile)
{
    int nParamCnt = argc - 1;
    const char** args = argv + 1;
    bool fError = false;

    TimeSpan timeSpan;

    //
    // Pass 1 - determine parameter set type: cmdline specification or XML, and preparse targets/blocksize
    //

    ParseState isXMLSet = ParseState::Unknown;

    ParseState isXMLResultFormat = ParseState::Unknown;
    ParseState isProfileOnly = ParseState::Unknown;
    ParseState isSystemInfoOnly = ParseState::Unknown;
    ParseState isVerbose = ParseState::Unknown;
    ParseState isVerboseStats = ParseState::Unknown;
    ParseState isRandomSeed = ParseState::Unknown;
    ParseState isWarmupTime = ParseState::Unknown;
    ParseState isDurationTime = ParseState::Unknown;
    ParseState isCooldownTime = ParseState::Unknown;
    ParseState isExperimentFlags = ParseState::Unknown;

    ULONG randomSeedValue = 0;
    ULONG warmupTime = 0;
    ULONG durationTime = 0;
    ULONG cooldownTime = 0;
    ULONG experimentFlags = 0;
    const char *xmlProfile = nullptr;

    //
    // Find all target specifications. Note that this assumes all non-target
    // parameters are single tokens; e.g. "-Hsomevalue" and never "-H somevalue".
    // Targets follow parameter specifications.
    //

    vector<Target> vTargets;
    for (int i = 0, inTargets = false; i < nParamCnt; ++i)
    {
        if (!_IsSwitchChar(args[i][0]))
        {
            inTargets = true;

            Target target;
            target.SetPath(args[i]);
            vTargets.push_back(target);
        }
        else if (inTargets)
        {
            fprintf(stderr, "ERROR: parameters (%s) must come before targets on the command line\n", args[i]);
            return false;
        }
    }

    //
    // Find composable and dependent parameters as we resolve the parameter set.
    //

    for (int i = 0; i < nParamCnt; ++i)
    {
        if (_IsSwitchChar(args[i][0]))
        {
            const char *arg = &args[i][2];

            switch(args[i][1])
            {


            case ':':
                // Experiment flags
                experimentFlags = atoi(arg);
                isExperimentFlags = ParseState::True;
                break;

            case 'b':

                if (arg[0] == 's')
                {
                    // Buffer separation sub-option: -bs<flag>
                    if (arg[1] == '\0')
                    {
                        fprintf(stderr, "ERROR: -bs requires a flag character (n or p)\n");
                        return false;
                    }

                    BufferSeparation bs;
                    switch (arg[1])
                    {
                    case 'n':
                        bs = BufferSeparation::SystemDefault;
                        break;
                    case 'p':
                        bs = BufferSeparation::PDECacheLine;
                        break;
                    default:
                        fprintf(stderr, "ERROR: invalid buffer separation flag '%c' passed to -bs (expected n or p)\n", arg[1]);
                        return false;
                    }

                    if (arg[2] != '\0')
                    {
                        fprintf(stderr, "ERROR: unexpected characters after -bs%c\n", arg[1]);
                        return false;
                    }

                    timeSpan.SetBufferSeparation(bs);
                    timeSpan.SetBufferSeparationExplicit(true);
                }
                // Block size does not compose with XML profile spec
                else if (isXMLSet == ParseState::True)
                {
                    fprintf(stderr, "ERROR: -b is not compatible with -X XML profile specification\n");
                    return false;
                }
                else
                {
                    UINT64 ullBlockSize;
                    if (_GetSizeInBytes(arg, ullBlockSize, nullptr) && ullBlockSize < MAXUINT32)
                    {
                        for (auto &j : vTargets)
                        {
                            j.SetBlockSizeInBytes((DWORD)ullBlockSize);
                        }
                    }
                    else
                    {
                        fprintf(stderr, "ERROR: invalid block size passed to -b\n");
                        return false;
                    }
                    _dwBlockSize = (DWORD)ullBlockSize;

                    isXMLSet = ParseState::False;
                }
                break;

            case 'C':
                {
                    int c = atoi(arg);
                    if (c >= 0)
                    {
                        cooldownTime = c;
                        isCooldownTime = ParseState::True;
                    }
                    else
                    {
                        fprintf(stderr, "ERROR: invalid cooldown time (-C): '%s'\n", arg);
                        return false;
                    }
                }
                break;

            case 'd':
                {
                    int c = atoi(arg);
                    if (c >= 0)
                    {
                        durationTime = c;
                        isDurationTime = ParseState::True;
                    }
                    else
                    {
                        fprintf(stderr, "ERROR: invalid measured duration time (-d): '%s'\n", arg);
                        return false;
                    }
                }
                break;

            case 'W':
                {
                    int c = atoi(arg);
                    if (c >= 0)
                    {
                        warmupTime = c;
                        isWarmupTime = ParseState::True;
                    }
                    else
                    {
                        fprintf(stderr, "ERROR: invalid warmup time (-W): '%s'\n", arg);
                        return false;
                    }
                }
                break;

            case 'R':

                // re-output profile only (no run)
                if ('p' == *arg)
                {
                    isProfileOnly = ParseState::True;
                    ++arg;
                }

                // system information only (no run)
                if ('s' == *arg)
                {
                    isSystemInfoOnly = ParseState::True;
                    ++arg;
                }

                if ('\0' != *arg)
                {
                    // Explicit results format
                    if (strcmp(arg, "xml") == 0)
                    {
                        isXMLResultFormat = ParseState::True;
                    }
                    else if (strcmp(arg, "text") != 0)
                    {
                        fprintf(stderr, "ERROR: invalid results format (-R): '%s'\n", arg);
                        return false;
                    }
                    else
                    {
                        isXMLResultFormat = ParseState::False;
                    }
                }
                else
                {
                    // allow for -Rp/-Rs shorthand for default format
                    if (isProfileOnly != ParseState::True &&
                        isSystemInfoOnly != ParseState::True)
                    {
                        fprintf(stderr, "ERROR: unspecified results format -R: use [p|s]<text|xml>\n");
                        return false;
                    }
                }
                break;

            case 'v':

                if (*arg == '\0')
                {
                    isVerbose = ParseState::True;
                }
                else if (*arg == 's' && *(arg+1) == '\0') // -vs
                {
                    isVerboseStats = ParseState::True;
                }
                else
                {
                    fprintf(stderr, "ERROR: invalid verbose mode (-v): '%s'\n", arg);
                    return false;
                }
                break;

            case 'X':

                if (isXMLSet == ParseState::Unknown)
                {
                    isXMLSet = ParseState::True;
                }
                else
                {
                    fprintf(stderr, "ERROR: multiple XML profiles specified (-X)\n");
                    return false;
                }
                xmlProfile = arg;
                break;

            case 'z':
                {
                    char *endPtr = nullptr;

                    if (*arg == '\0')
                    {
                        randomSeedValue = (ULONG) GetTickCount64();
                    }
                    else
                    {
                        randomSeedValue = strtoul(arg, &endPtr, 10);
                        if (*endPtr != '\0')
                        {
                            fprintf(stderr, "ERROR: invalid random seed value '%s' specified - must be a valid 32 bit integer\n", arg);
                            return false;
                        }
                    }

                    isRandomSeed = ParseState::True;
                }
                break;

            default:
                // no other switches are valid in combination with -X
                // if we've seen X, this means it is bad
                // if not, we know it will not be X
                if (isXMLSet == ParseState::True)
                {
                    fprintf(stderr, "ERROR: invalid XML profile specification; parameter %s not compatible with -X\n", args[i]);
                    return false;
                }
                else
                {
                    isXMLSet = ParseState::False;
                }
            }
        }
    }

    // XML profile?
    if (isXMLSet == ParseState::True)
    {
        if (!_ReadParametersFromXmlFile(xmlProfile, pProfile, &vTargets))
        {
            return false;
        }
    }

    //
    // Apply profile common parameters - note that results format is unmodified if R not explicitly provided
    //

    if (isXMLResultFormat == ParseState::True)
    {
        pProfile->SetResultsFormat(ResultsFormat::Xml);
    }
    else if (isXMLResultFormat == ParseState::False)
    {
        pProfile->SetResultsFormat(ResultsFormat::Text);
    }

    if (isProfileOnly == ParseState::True)
    {
        pProfile->SetProfileOnly(true);
    }

    if (isSystemInfoOnly == ParseState::True)
    {
        // -Rs is standalone: it must be the only argument
        if (argc != 2)
        {
            fprintf(stderr, "ERROR: -Rs must be the only parameter\n");
            return false;
        }

        pProfile->SetSystemInformationOnly(true);
        return true;
    }

    if (isVerbose == ParseState::True)
    {
        pProfile->SetVerbose(true);
    }

    if (isVerboseStats == ParseState::True)
    {
        pProfile->SetVerboseStats(true);
    }

    if (isExperimentFlags == ParseState::True)
    {
        g_ExperimentFlags = experimentFlags;
    }

    //
    // Apply timespan common composable parameters
    //

    if (isXMLSet == ParseState::True)
    {
        for (auto& ts : const_cast<vector<TimeSpan> &>(pProfile->GetTimeSpans()))
        {
            if (isRandomSeed == ParseState::True)   { ts.SetRandSeed(randomSeedValue); }
            if (isWarmupTime == ParseState::True)   { ts.SetWarmup(warmupTime); }
            if (isDurationTime == ParseState::True) { ts.SetDuration(durationTime); }
            if (isCooldownTime == ParseState::True) { ts.SetCooldown(cooldownTime); }
        }
    }
    else
    {
        if (isRandomSeed == ParseState::True)   { timeSpan.SetRandSeed(randomSeedValue); }
        if (isWarmupTime == ParseState::True)   { timeSpan.SetWarmup(warmupTime); }
        if (isDurationTime == ParseState::True) { timeSpan.SetDuration(durationTime); }
        if (isCooldownTime == ParseState::True) { timeSpan.SetCooldown(cooldownTime); }
    }

    // Now done if XML profile
    if (isXMLSet == ParseState::True)
    {
        fXMLProfile = true;
        return true;
    }

    //
    // Parse full command line for profile
    //

    // initial parse for cache/writethrough
    // these are built up across the entire cmd line and applied at the end.
    // this allows for conflicts to be thrown for mixed -h/-S as needed.
    TargetCacheMode t = TargetCacheMode::Undefined;
    WriteThroughMode w = WriteThroughMode::Undefined;
    MemoryMappedIoMode m = MemoryMappedIoMode::Undefined;
    MemoryMappedIoFlushMode f = MemoryMappedIoFlushMode::Undefined;
    BypassIoMode b = BypassIoMode::Undefined;

    // seen base/max target offset specification yet?
    ParseState isMaxTargetOffset = ParseState::Unknown;
    ParseState isBaseTargetOffset = ParseState::Unknown;

    bool bExit = false;
    while (nParamCnt)
    {
        const char* arg = *args;
        const char* const carg = arg;     // save for error reporting, arg is modified during parse

        // Targets follow parameters on command line. If this is a target, we are done now.
        if (!_IsSwitchChar(*arg))
        {
            break;
        }

        // skip switch character, provide length
        ++arg;
        const size_t argLen = strlen(arg);

        switch (*arg)
        {
        case '?':
            _DisplayUsageInfo(argv[0]);
            exit(0);

        case ':': // experiment flags
            // handled during composable parameter evaluation
            break;

        case 'a': // affinity
            //-a1,2,3,4 (assign threads to cpus 1,2,3,4 (round robin))
            if (!_ParseAffinity(arg, &timeSpan))
            {
                fError = true;
            }
            break;

        case 'b':    //block size
            // handled during composable parameter evaluation
            break;

        case 'B':    //base target offset (offset from 0) and optional length
            if (*(arg + 1) != '\0')
            {
                UINT64 base;
                UINT64 len;
                const char *rest;

                if (isBaseTargetOffset == ParseState::True)
                {
                    fprintf(stderr, "ERROR: base target offset (-Bbase[:length]) can only be specified once\n");
                    fError = true;
                    break;
                }

                if (_GetSizeInBytes(arg + 1, base, &rest))
                {
                    for (auto &i : vTargets)
                    {
                        i.SetBaseFileOffsetInBytes(base);
                    }

                    isBaseTargetOffset = ParseState::True;
                }
                else
                {
                    fprintf(stderr, "ERROR: invalid base target offset passed to -B\n");
                    fError = true;
                    break;
                }

                if (rest && *rest != '\0')
                {
                    if (*rest != ':')
                    {
                        fprintf(stderr, "ERROR: unexpected characters after -Bbase; use \':\' to separate -Bbase:length\n");
                        fError = true;
                        break;
                    }

                    if (*(++rest) == '\0')
                    {
                        fprintf(stderr, "ERROR: -Bbase:length - no length provided\n");
                        fError = true;
                        break;
                    }

                    if (isMaxTargetOffset == ParseState::True)
                    {
                        fprintf(stderr, "ERROR: maximum target offset (-Bbase:length or -fsize) can only be specified once\n");
                        fError = true;
                        break;
                    }

                    if (!_GetSizeInBytes(rest, len, nullptr) || base + len < base)
                    {
                        fprintf(stderr, "ERROR: invalid target length passed to -B\n");
                        fError = true;
                        break;
                    }

                    for (auto &i : vTargets)
                    {
                        i.SetMaxFileSize(base + len);
                    }

                    isMaxTargetOffset = ParseState::True;
                }
            }
            else
            {
                fError = true;
            }
            break;

        case 'c':    //create file of the given size
            if (*(arg + 1) != '\0')
            {
                UINT64 cb;
                if (_GetSizeInBytes(arg + 1, cb, nullptr))
                {
                    for (auto &i : vTargets)
                    {
                        i.SetFileSize(cb);
                        i.SetCreateFile(true);
                    }
                }
                else
                {
                    fprintf(stderr, "ERROR: invalid target size passed to -c\n");
                    fError = true;
                }
            }
            else
            {
                fError = true;
            }
            break;

        case 'C':    //cool down time
            // handled during composable parameter evaluation
            break;

        case 'd':    //duration
            // handled during composable parameter evaluation
            break;

        case 'D':    //standard deviation
            {
                timeSpan.SetCalculateIopsStdDev(true);

                int x = atoi(arg + 1);
                if (x > 0)
                {
                    timeSpan.SetIoBucketDurationInMilliseconds(x);
                }
            }
            break;

        case 'e':    //etw
            if (!_ParseETWParameter(arg, pProfile))
            {
                fError = true;
            }
            break;

        case 'f':
            if ('\0' == *(arg + 1))
            {
                fError = true;
                break;
            }

            // max target offset form?
            if (isdigit(*(arg + 1)))
            {
                UINT64 cb;

                if (isMaxTargetOffset == ParseState::True)
                {
                    fprintf(stderr, "ERROR: maximum target offset (-Bbase:length or -fsize) can only be specified once\n");
                    fError = true;
                    break;
                }

                if (!_GetSizeInBytes(arg + 1, cb, nullptr))
                {
                    fprintf(stderr, "ERROR: invalid max target size passed to -f\n");
                    fError = true;
                    break;
                }

                for (auto &i : vTargets)
                {
                    i.SetMaxFileSize(cb);
                }

                isMaxTargetOffset = ParseState::True;
                break;
            }

            // while -frs (or -fsr) are generally conflicting intentions as far as
            // the OS is concerned, do not enforce
            while (*(++arg) != '\0')
            {
                switch (*arg)
                {
                case 'r':
                    for (auto &i : vTargets)
                    {
                        i.SetRandomAccessHint(true);
                    }
                    break;
                case 's':
                    for (auto &i : vTargets)
                    {
                        i.SetSequentialScanHint(true);
                    }
                    break;
                case 't':
                    for (auto &i : vTargets)
                    {
                        i.SetTemporaryFileHint(true);
                    }
                    break;
                default:
                    fError = true;
                    break;
                }
            }
            break;

        case 'F':    //total number of threads
            {
                int c = atoi(arg + 1);
                if (c > 0)
                {
                    timeSpan.SetThreadCount(c);
                }
                else
                {
                    fError = true;
                }
            }
            break;

        case 'g':    //throughput in bytes per millisecond (gNNN) OR iops (gNNNi)
            {
                // units?
                bool isBpms = false;
                if (isdigit(arg[argLen - 1]))
                {
                    isBpms = true;
                }
                else if (arg[argLen - 1] != 'i')
                {
                    // not IOPS, so its bad
                    fError = true;
                }

                if (!fError)
                {
                    int c = atoi(arg + 1);
                    if (c > 0)
                    {
                        for (auto &i : vTargets)
                        {
                            if (isBpms)
                            {
                                i.SetThroughput(c);
                            }
                            else
                            {
                                i.SetThroughputIOPS(c);
                            }
                        }
                    }
                    else
                    {
                        fError = true;
                    }
                }
            }
            break;

        case 'h':    // compat: disable os cache and set writethrough; now equivalent to -Sh
            if (t == TargetCacheMode::Undefined &&
                w == WriteThroughMode::Undefined)
            {
                t = TargetCacheMode::DisableOSCache;
                w = WriteThroughMode::On;
            }
            else
            {
                fprintf(stderr, "ERROR: -h conflicts with earlier specification of cache/writethrough\n");
                fError = true;
            }
            break;

        case 'i':    //number of IOs to issue before think time
            {
                int c = atoi(arg + 1);
                if (c > 0)
                {
                    for (auto &i : vTargets)
                    {
                        i.SetBurstSize(c);
                        i.SetUseBurstSize(true);
                    }
                }
                else
                {
                    fError = true;
                }
            }
            break;

        case 'j':    //time to wait between bursts of IOs
            {
                int c = atoi(arg + 1);
                if (c > 0)
                {
                    for (auto &i : vTargets)
                    {
                        i.SetThinkTime(c);
                        i.SetEnableThinkTime(true);
                    }
                }
                else
                {
                    fError = true;
                }
            }
            break;

        case 'I':   //io priority
            {
                int x = atoi(arg + 1);
                if (x > 0 && x < 4)
                {
                    PRIORITY_HINT hint[] = { IoPriorityHintVeryLow, IoPriorityHintLow, IoPriorityHintNormal };
                    for (auto &i : vTargets)
                    {
                        i.SetIOPriorityHint(hint[x - 1]);
                    }
                }
                else
                {
                    fError = true;
                }
            }
            break;

        case 'l':    //large pages
            for (auto &i : vTargets)
            {
                i.SetUseLargePages(true);
            }
            break;

        case 'L':    //measure latency
            timeSpan.SetMeasureLatency(true);
            break;

        case 'n':    //disable affinity (by default simple affinity is turned on)
            timeSpan.SetDisableAffinity(true);
            break;

        case 'N':
            if (!_ParseFlushParameter(arg, &f))
            {
                fError = true;
            }
            break;

        case 'o':    //request count (1==synchronous) or -oc<count> completion depth
            {
                if (arg[1] == 'c')
                {
                    int c = atoi(arg + 2);
                    if (c >= 1 && c <= (int)c_maximumCompletionDepth)
                    {
                        timeSpan.SetCompletionDepth(c);
                        timeSpan.SetCompletionDepthExplicit(true);
                    }
                    else
                    {
                        fprintf(stderr, "ERROR: completion depth (-oc) must be between 1 and %u\n", c_maximumCompletionDepth);
                        fError = true;
                    }
                }
                else
                {
                    int c = atoi(arg + 1);
                    if (c > 0)
                    {
                        for (auto &i : vTargets)
                        {
                            i.SetRequestCount(c);
                        }
                    }
                    else
                    {
                        fError = true;
                    }
                }
            }
            break;

        case 'O':   //total number of IOs/thread - for use with -F
            {
                int c = atoi(arg + 1);
                if (c > 0)
                {
                    timeSpan.SetRequestCount(c);
                }
                else
                {
                    fError = true;
                }
            }
            break;

        case 'p':    //start async IO operations with the same offset
            //makes sense only for -o2 and greater
            for (auto &i : vTargets)
            {
                i.SetUseParallelAsyncIO(true);
            }
            break;

        case 'P':    //show progress every x IO operations
            {
                int c = atoi(arg + 1);
                if (c < 1)
                {
                    c = 65536;
                }
                pProfile->SetProgress(c);
            }
            break;

        case 'r':    //random access
            {
                // mixed random/sequential pct split?
                if (*(arg + 1) == 's')
                {
                    int c = 0;

                    ++arg;
                    if (*(arg + 1) == '\0')
                    {
                        fprintf(stderr, "ERROR: no random percentage passed to -rs\n");
                        fError = true;
                    }
                    else
                    {
                        c = atoi(arg + 1);
                        if (c <= 0 || c > 100)
                        {
                            fprintf(stderr, "ERROR: random percentage passed to -rs should be between 1 and 100\n");
                            fError = true;
                        }
                    }
                    if (!fError)
                    {
                        for (auto &i : vTargets)
                        {
                            // if random ratio is unset and actual alignment is already specified,
                            // -s was used: don't allow this for clarity of intent
                            if (!i.GetRandomRatio() &&
                                i.GetBlockAlignmentInBytes(true))
                            {
                                fprintf(stderr, "ERROR: use -r to specify IO alignment when using mixed random/sequential IO (-rs)\n");
                                fError = true;
                                break;
                            }
                            // if random ratio was already set to something other than 100% (-r)
                            // then -rs was specified multiple times: catch and block this
                            if (i.GetRandomRatio() &&
                                i.GetRandomRatio() != 100)
                            {
                                fprintf(stderr, "ERROR: mixed random/sequential IO (-rs) specified multiple times\n");
                                fError = true;
                                break;
                            }
                            // Note that -rs100 is the same as -r. It will not result in the <RandomRatio> element
                            // in the XML profile; we will still only emit/accept 1-99 there.
                            //
                            // Saying -rs0 (sequential) would create an ambiguity between that and -r[nnn]. Rather
                            // than bend the intepretation of -r[nnn] for the special case of -rs0 we will error
                            // it out in the bounds check above.
                            i.SetRandomRatio(c);
                        }
                    }
                }

                // random distribution

                else if (*(arg + 1) == 'd')
                {
                    // advance past the d
                    arg += 2;

                    fError = !_ParseRandomDistribution(arg, vTargets);
                }

                // random block alignment
                // if mixed random/sequential not already specified, set to 100%
                else
                {

                    UINT64 cb = _dwBlockSize;
                    if (*(arg + 1) != '\0')
                    {
                        if (!_GetSizeInBytes(arg + 1, cb, nullptr) || (cb == 0))
                        {
                            fprintf(stderr, "ERROR: invalid alignment passed to -r\n");
                            fError = true;
                        }
                    }
                    if (!fError)
                    {
                        for (auto &i : vTargets)
                        {
                            // Do not override -rs specification
                            if (!i.GetRandomRatio())
                            {
                                i.SetRandomRatio(100);
                            }
                            // Multiple -rNN?
                            // Note that -rs100 -r[NN] will pass since -rs does not set alignment.
                            // We are only validating a single -rNN specification.
                            else if (i.GetRandomRatio() == 100 &&
                                     i.GetBlockAlignmentInBytes(true))
                            {
                                fprintf(stderr, "ERROR: random IO (-r) specified multiple times\n");
                                fError = true;
                                break;
                            }
                            // -s already set the alignment?
                            if (i.GetBlockAlignmentInBytes(true))
                            {
                                fprintf(stderr, "ERROR: sequential IO (-s) conflicts with random IO (-r/-rs)\n");
                                fError = true;
                                break;
                            }
                            i.SetBlockAlignmentInBytes(cb);
                        }
                    }
                }
            }
            break;

        case 'R':   // output profile/results format engine
            // handled during composable parameter evaluation
            break;

        case 's':   //stride size
            {
                int idx = 1;

                if ('i' == *(arg + idx))
                {
                    // do interlocked sequential mode
                    // ISSUE-REVIEW: this does nothing if -r is specified
                    // ISSUE-REVIEW: this does nothing if -p is specified
                    // ISSUE-REVIEW: this does nothing if we are single-threaded
                    for (auto &i : vTargets)
                    {
                        i.SetUseInterlockedSequential(true);
                    }

                    idx++;
                }

                for (auto &i : vTargets)
                {
                    // conflict -s with -rs/-s
                    if (i.GetRandomRatio())
                    {
                        if (i.GetRandomRatio() == 100) {
                            fprintf(stderr, "ERROR: sequential IO (-s) conflicts with random IO (-r/-rs)\n");
                        }
                        else
                        {
                            fprintf(stderr, "ERROR: use -r to specify IO alignment for -rs\n");
                        }
                        fError = true;
                        break;
                    }

                    // conflict with multiple -s
                    if (i.GetBlockAlignmentInBytes(true))
                    {
                        fprintf(stderr, "ERROR: sequential IO (-s) specified multiple times\n");
                        fError = true;
                        break;
                    }
                }

                if (*(arg + idx) != '\0')
                {
                    UINT64 cb;
                    // Note that we allow -s0, as unusual as that would be.
                    // The counter-case of -r0 is invalid and checked for.
                    if (_GetSizeInBytes(arg + idx, cb, nullptr))
                    {
                        for (auto &i : vTargets)
                        {
                            i.SetBlockAlignmentInBytes(cb);
                        }
                    }
                    else
                    {
                        fprintf(stderr, "ERROR: invalid stride size passed to -s\n");
                        fError = true;
                    }
                }
                else
                {
                    // explicitly pass through the block size so that we can detect
                    // -rs/-s intent conflicts when attempting to set -rs
                    for (auto &i : vTargets)
                    {
                        i.SetBlockAlignmentInBytes(i.GetBlockSizeInBytes());
                    }
                }
            }
            break;

        case 'S':   //control os/hw/remote caching and writethrough
            {
                // parse flags - it is an error to multiply specify either property, which
                //   can be detected simply by checking if we move one from !undefined.
                //   this also handles conflict cases.
                int idx;
                for (idx = 1; !fError && *(arg + idx) != '\0'; idx++)
                {
                    switch (*(arg + idx))
                    {
                    case 'b':
                        if (t == TargetCacheMode::Undefined)
                        {
                            t = TargetCacheMode::Cached;
                        }
                        else
                        {
                            fprintf(stderr, "ERROR: -Sb conflicts with earlier specification of cache mode\n");
                            fError = true;
                        }
                        break;
                    case 'h':
                        if (t == TargetCacheMode::Undefined &&
                            w == WriteThroughMode::Undefined &&
                            m == MemoryMappedIoMode::Undefined)
                        {
                            t = TargetCacheMode::DisableOSCache;
                            w = WriteThroughMode::On;
                        }
                        else
                        {
                            fprintf(stderr, "ERROR: -Sh conflicts with earlier specification of cache/writethrough/memory mapped\n");
                            fError = true;
                        }
                        break;
                    case 'm':
                        if (m == MemoryMappedIoMode::Undefined &&
                            t != TargetCacheMode::DisableOSCache)
                        {
                            m = MemoryMappedIoMode::On;
                        }
                        else
                        {
                            fprintf(stderr, "ERROR: -Sm conflicts with earlier specification of memory mapped IO/unbuffered IO\n");
                            fError = true;
                        }
                        break;
                    case 'r':
                        if (t == TargetCacheMode::Undefined)
                        {
                            t = TargetCacheMode::DisableLocalCache;
                        }
                        else
                        {
                            fprintf(stderr, "ERROR: -Sr conflicts with earlier specification of cache mode\n");
                            fError = true;
                        }
                        break;
                    case 'u':
                        if (t == TargetCacheMode::Undefined &&
                            m == MemoryMappedIoMode::Undefined)
                        {
                            t = TargetCacheMode::DisableOSCache;
                        }
                        else
                        {
                            fprintf(stderr, "ERROR: -Su conflicts with earlier specification of cache mode/memory mapped IO\n");
                            fError = true;
                        }
                        break;
                    case 'w':
                        if (w == WriteThroughMode::Undefined)
                        {
                            w = WriteThroughMode::On;
                        }
                        else
                        {
                            fprintf(stderr, "ERROR -Sw conflicts with earlier specification of write through\n");
                            fError = true;
                        }
                        break;
                    case 'y':
                        if (b == BypassIoMode::Undefined)
                        {
                            b = BypassIoMode::Partial;
                        }
                        else
                        {
                            fprintf(stderr, "ERROR: -Sy conflicts with earlier specification of BypassIO mode\n");
                            fError = true;
                        }
                        break;
                    case 'Y':
                        if (b == BypassIoMode::Undefined)
                        {
                            b = BypassIoMode::Full;
                        }
                        else
                        {
                            fprintf(stderr, "ERROR: -SY conflicts with earlier specification of BypassIO mode\n");
                            fError = true;
                        }
                        break;
                    default:
                        fprintf(stderr, "ERROR: unrecognized option provided to -S\n");
                        fError = true;
                        break;
                    }
                }

                // bare -S, parse loop did not advance
                if (!fError && idx == 1)
                {
                    if (t == TargetCacheMode::Undefined &&
                        m == MemoryMappedIoMode::Undefined)
                    {
                        t = TargetCacheMode::DisableOSCache;
                    }
                    else
                    {
                        fprintf(stderr, "ERROR: -S conflicts with earlier specification of cache mode\n");
                        fError = true;
                    }
                }
            }
            break;

        case 't':    //number of threads per file
            {
                int c = atoi(arg + 1);
                if (c > 0)
                {
                    for (auto &i : vTargets)
                    {
                        i.SetThreadsPerFile(c);
                    }
                }
                else
                {
                    fError = true;
                }
            }
            break;

        case 'T':    //offsets between threads reading the same file
            {
                UINT64 cb;
                if (_GetSizeInBytes(arg + 1, cb, nullptr) && (cb > 0))
                {
                    for (auto &i : vTargets)
                    {
                        i.SetThreadStrideInBytes(cb);
                    }
                }
                else
                {
                    fprintf(stderr, "ERROR: invalid offset passed to -T\n");
                    fError = true;
                }
            }
            break;

        case 'u':    //IoRing
            {
                // Parse IoRing -u[b][percentage] format
                const char *c = arg + 1;
                bool useRegisteredBuffers = false;

                // Check for optional 'b' qualifier for registered buffers
                if (*c == 'b')
                {
                    useRegisteredBuffers = true;
                    c++;
                }

                // Parse percentage value if provided
                if (*c != '\0')
                {
                    int percentage = atoi(c);
                    if (percentage > 0 && percentage <= 100)
                    {
                        timeSpan.SetIoRingBatchSize(percentage);
                    }
                    else
                    {
                        fprintf(stderr, "ERROR: invalid IoRing batch size percentage\n");
                        fError = true;
                        break;
                    }
                }

                // Set IoRing parameters
                timeSpan.SetUseIoRing(true);
                if (useRegisteredBuffers)
                {
                    timeSpan.SetUseRegBuffer(true);
                }
            }
            break;

        case 'v':    //verbose mode
            // handled during composable parameter evaluation
            break;

        case 'w':    //write test [default=read]
            {
                int c = 0;

                if (*(arg + 1) == '\0')
                {
                    fprintf(stderr, "ERROR: no write ratio passed to -w\n");
                    fError = true;
                }
                else
                {
                    c = atoi(arg + 1);
                    if (c < 0 || c > 100)
                    {
                        fprintf(stderr, "ERROR: write ratio passed to -w must be between 0 and 100 (percent)\n");
                        fError = true;
                    }
                }
                if (!fError)
                {
                    for (auto &i : vTargets)
                    {
                        i.SetWriteRatio(c);
                    }
                }
            }
            break;

        case 'W':    //warm up time
            // handled during composable parameter evaluation
            break;

        case 'x':    //completion routines
            timeSpan.SetCompletionRoutines(true);
            break;

        case 'y':    //external synchronization
            switch (*(arg + 1))
            {

            case 's':
                _hEventStarted = CreateEvent(NULL, TRUE, FALSE, arg + 2);
                if (NULL == _hEventStarted)
                {
                    fprintf(stderr, "Error creating/opening start notification event: '%s'\n", arg + 2);
                    exit(1);    // TODO: this class shouldn't terminate the process
                }
                break;

            case 'f':
                _hEventFinished = CreateEvent(NULL, TRUE, FALSE, arg + 2);
                if (NULL == _hEventFinished)
                {
                    fprintf(stderr, "Error creating/opening finish notification event: '%s'\n", arg + 2);
                    exit(1);    // TODO: this class shouldn't terminate the process
                }
                break;

            case 'r':
                synch->hStartEvent = CreateEvent(NULL, TRUE, FALSE, arg + 2);
                if (NULL == synch->hStartEvent)
                {
                    fprintf(stderr, "Error creating/opening wait-for-start event: '%s'\n", arg + 2);
                    exit(1);    // TODO: this class shouldn't terminate the process
                }
                break;

            case 'p':
                synch->hStopEvent = CreateEvent(NULL, TRUE, FALSE, arg + 2);
                if (NULL == synch->hStopEvent)
                {
                    fprintf(stderr, "Error creating/opening force-stop event: '%s'\n", arg + 2);
                    exit(1);    // TODO: this class shouldn't terminate the process
                }
                break;

            case 'e':
                {
                    HANDLE hEvent = OpenEvent(EVENT_MODIFY_STATE, FALSE, arg + 2);
                    if (NULL == hEvent)
                    {
                        fprintf(stderr, "Error opening event '%s'\n", arg + 2);
                        exit(1);    // TODO: this class shouldn't terminate the process
                    }
                    if (!SetEvent(hEvent))
                    {
                        fprintf(stderr, "Error setting event '%s'\n", arg + 2);
                        exit(1);    // TODO: this class shouldn't terminate the process
                    }
                    CloseHandle(hEvent);
                    printf("Succesfully set event: '%s'\n", arg + 2);
                    bExit = true;
                    break;
                }

            default:
                fError = true;
            }

        case 'z':    //random seed
            // handled during composable parameter evaluation
            break;

        case 'Z':    //zero write buffers
            if (*(arg + 1) == '\0')
            {
                for (auto &i : vTargets)
                {
                    i.SetZeroWriteBuffers(true);
                }
            }
            else if (*(arg + 1) == 'r' && *(arg + 2) == '\0')
            {
                timeSpan.SetRandomWriteData(true);
            }
            else
            {
                UINT64 cb = 0;
                string sPath;
                if (_GetRandomDataWriteBufferData(string(arg + 1), cb, sPath) && (cb > 0))
                {
                    for (auto &i : vTargets)
                    {
                        i.SetRandomDataWriteBufferSize(cb);
                        i.SetRandomDataWriteBufferSourcePath(sPath);
                    }
                }
                else
                {
                    fprintf(stderr, "ERROR: invalid size passed to -Z\n");
                    fError = true;
                }
            }
            break;

        default:
            fprintf(stderr, "ERROR: invalid option: '%s'\n", carg);
            return false;
        }

        if (fError)
        {
            // note: original pointer to the cmdline argument, without parse movement
            fprintf(stderr, "ERROR: incorrectly provided option: '%s'\n", carg);
            return false;
        }

        --nParamCnt;
        ++args;
    }

    //
    // exit if a user specified an action which was already satisfied and doesn't require running test
    //
    if (bExit)
    {
        printf("Now exiting...\n");
        exit(1);                        // TODO: this class shouldn't terminate the process
    }

    if (vTargets.size() < 1)
    {
        fprintf(stderr, "ERROR: need to provide at least one filename\n");
        return false;
    }

    // apply resultant cache/writethrough/memory mapped io modes to the targets
    for (auto &i : vTargets)
    {
        if (t != TargetCacheMode::Undefined)
        {
            i.SetCacheMode(t);
        }
        if (w != WriteThroughMode::Undefined)
        {
            i.SetWriteThroughMode(w);
        }
        if (m != MemoryMappedIoMode::Undefined)
        {
            i.SetMemoryMappedIoMode(m);
        }
        if (f != MemoryMappedIoFlushMode::Undefined)
        {
            i.SetMemoryMappedIoFlushMode(f);
        }
        if (b != BypassIoMode::Undefined)
        {
            i.SetBypassIoMode(b);
        }
    }

    // ... and apply targets to the timespan
    for (auto &i : vTargets)
    {
        timeSpan.AddTarget(i);
    }
    pProfile->AddTimeSpan(timeSpan);

    return true;
}

bool CmdLineParser::_ReadParametersFromXmlFile(const char *pszPath, Profile *pProfile, vector<Target> *pvSubstTargets)
{
    XmlProfileParser parser;

    return parser.ParseFile(pszPath, pProfile, pvSubstTargets, NULL);
}

bool CmdLineParser::ParseCmdLine(const int argc, const char *argv[], Profile *pProfile, struct Synchronization *synch, SystemInformation *pSystem)
{
    assert(nullptr != argv);
    assert(nullptr != pProfile);
    assert(NULL != synch);

    if (argc < 2)
    {
        _DisplayUsageInfo(argv[0]);
        return false;
    }

    string sCmdLine;
    for (int i = 0; i < argc - 1; i++)
    {
        sCmdLine += argv[i];
        sCmdLine += ' ';
    }
    if (argc > 0)
    {
        sCmdLine += argv[argc - 1];
    }
    pProfile->SetCmdLine(sCmdLine);

    bool fOk = true;
    bool fXMLProfile = false;

    fOk = _ReadParametersFromCmdLine(argc, argv, pProfile, synch, fXMLProfile);

    // Check additional restrictions and conditions on the parsed profile.
    // Note that on the current cmdline, all targets receive the same parameters
    // so their mutual consistency only needs to be checked once. Do not check
    // system consistency in profile-only operation (this is only required at
    // execution time).

    if (fOk && !pProfile->GetSystemInformationOnly())
    {
        fOk = pProfile->Validate(!fXMLProfile, pProfile->GetProfileOnly() ? nullptr : pSystem);
    }

    return fOk;
}
