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
#include <cassert>
#include <cstdio>
#include <cstdlib>

CmdLineParser::CmdLineParser() :
    _dwBlockSize(64 * 1024),
    _ulWriteRatio(0),
    _hEventStarted(nullptr),
    _hEventFinished(nullptr)
{
}

CmdLineParser::~CmdLineParser() = default;

// Get size in bytes from a string (it can end with K, M, G for KB, MB, GB and b for block)
bool CmdLineParser::_GetSizeInBytes(const char *pszSize, UINT64& ullSize) const
{
    bool fOk = true;
    UINT64 ullResult = 0;
    bool fLastCharacterFound = false;
    for (char ch = *pszSize; fOk && (ch != '\0'); ch = *(++pszSize))
    {
        if (fLastCharacterFound)
        {
            fOk = false;
        }
        else if ((ch >= '0') && (ch <= '9'))
        {
            if (ullResult <= (MAXUINT64 - (ch - '0')) / 10)
            {
                ullResult = ((ullResult * 10) + (ch - '0'));
            }
            else
            {
                fOk = false;
            }

        }
        else
        {
            ch = static_cast<char>(toupper(ch));
            if ((ch == 'B') || (ch == 'K') || (ch == 'M') || (ch == 'G'))
            {
                UINT64 ullMultiplier = 0;
                if (ch == 'B')          { ullMultiplier = _dwBlockSize; }
                else if (ch == 'K')     { ullMultiplier = 1024; }
                else if (ch == 'M')     { ullMultiplier = 1024 * 1024; }
                else if (ch == 'G')     { ullMultiplier = 1024 * 1024 * 1024; }

                if (ullResult <= MAXUINT64 / ullMultiplier)
                {
                    ullResult = ullResult * ullMultiplier;
                    fLastCharacterFound = true;
                }
                else
                {
                    // overflow
                    fOk = false;
                }
            }
            else
            {
                fOk = false;
                fprintf(stderr, "Invalid size specifier '%c'. Valid ones are: K - KB, M - MB, G - GB, B - block\n", ch);
            }
        }
    }

    if (fOk)
    {
        ullSize = ullResult;
    }
    return fOk;
}

bool CmdLineParser::_GetRandomDataWriteBufferData(const string& sArg, UINT64& cb, string& sPath) const
{
    bool fOk;
	const size_t iComma = sArg.find(',');
    if (iComma == std::string::npos)
    {
        fOk = _GetSizeInBytes(sArg.c_str(), cb);
        sPath = "";
    }
    else
    {
        fOk = _GetSizeInBytes(sArg.substr(0, iComma).c_str(), cb);
        sPath = sArg.substr(iComma + 1);
    }
    return fOk;
}

void CmdLineParser::_DisplayUsageInfo(const char *pszFilename)
{
    // ISSUE-REVIEW: this formats badly in the default 80 column command prompt
    printf("\n");
    printf("Usage: %s [options] target1 [ target2 [ target3 ...] ]\n", pszFilename);
    printf("version %s (%s)\n", DISKSPD_NUMERIC_VERSION_STRING, DISKSPD_DATE_VERSION_STRING);
    printf("\n");
    printf("Available targets:\n");
    printf("       file_path\n");
    printf("       #<physical drive number>\n");
    printf("       <partition_drive_letter>:\n");
    printf("\n");
    printf("Available options:\n");
    printf("  -?                    display usage information\n");
    printf("  -ag                   group affinity - affinitize threads round-robin to cores in Processor Groups 0 - n.\n");
    printf("                          Group 0 is filled before Group 1, and so forth.\n");
    printf("                          [default; use -n to disable default affinity]\n");
    printf("  -ag#,#[,#,...]>       advanced CPU affinity - affinitize threads round-robin to the CPUs provided. The g# notation\n");
    printf("                          specifies Processor Groups for the following CPU core #s. Multiple Processor Groups\n");
    printf("                          may be specified, and groups/cores may be repeated. If no group is specified, 0 is assumed.\n");
    printf("                          Additional groups/processors may be added, comma separated, or on separate parameters.\n");
    printf("                          Examples: -a0,1,2 and -ag0,0,1,2 are equivalent.\n");
    printf("                                    -ag0,0,1,2,g1,0,1,2 specifies the first three cores in groups 0 and 1.\n");
    printf("                                    -ag0,0,1,2 -ag1,0,1,2 is equivalent.\n");
    printf("  -b<size>[K|M|G]       block size in bytes or KiB/MiB/GiB [default=64K]\n");
    printf("  -B<offs>[K|M|G|b]     base target offset in bytes or KiB/MiB/GiB/blocks [default=0]\n");
    printf("                          (offset from the beginning of the file)\n");
    printf("  -c<size>[K|M|G|b]     create files of the given size.\n");
    printf("                          Size can be stated in bytes or KiB/MiB/GiB/blocks\n");
    printf("  -C<seconds>           cool down time - duration of the test after measurements finished [default=0s].\n");
    printf("  -D<milliseconds>      Capture IOPs statistics in intervals of <milliseconds>; these are per-thread\n");
    printf("                          per-target: text output provides IOPs standard deviation, XML provides the full\n");
    printf("                          IOPs time series in addition. [default=1000, 1 second].\n");
    printf("  -d<seconds>           duration (in seconds) to run test [default=10s]\n");
    printf("  -f<size>[K|M|G|b]     target size - use only the first <size> bytes or KiB/MiB/GiB/blocks of the file/disk/partition,\n");
    printf("                          for example to test only the first sectors of a disk\n");
    printf("  -f<rst>               open file with one or more additional access hints\n");
    printf("                          r : the FILE_FLAG_RANDOM_ACCESS hint\n");
    printf("                          s : the FILE_FLAG_SEQUENTIAL_SCAN hint\n");
    printf("                          t : the FILE_ATTRIBUTE_TEMPORARY hint\n");
    printf("                          [default: none]\n");
    printf("  -F<count>             total number of threads (conflicts with -t)\n");
    printf("  -g<bytes per ms>      throughput per-thread per-target throttled to given bytes per millisecond\n");
    printf("                          note that this can not be specified when using completion routines\n");
    printf("                          [default inactive]\n"); 
    printf("  -h                    deprecated, see -Sh\n");
    printf("  -i<count>             number of IOs per burst; see -j [default: inactive]\n");
    printf("  -j<milliseconds>      interval in <milliseconds> between issuing IO bursts; see -i [default: inactive]\n");
    printf("  -I<priority>          Set IO priority to <priority>. Available values are: 1-very low, 2-low, 3-normal (default)\n");
    printf("  -l                    Use large pages for IO buffers\n");
    printf("  -L                    measure latency statistics\n");
    printf("  -n                    disable default affinity (-a)\n");
    printf("  -o<count>             number of outstanding I/O requests per target per thread\n");
    printf("                          (1=synchronous I/O, unless more than 1 thread is specified with -F)\n");
    printf("                          [default=2]\n");
    printf("  -O<count>             number of outstanding I/O requests per thread - for use with -F\n");
    printf("                          (1=synchronous I/O)\n");
    printf("  -p                    start parallel sequential I/O operations with the same offset\n");
    printf("                          (ignored if -r is specified, makes sense only with -o2 or greater)\n");
    printf("  -P<count>             enable printing a progress dot after each <count> [default=65536]\n");
    printf("                          completed I/O operations, counted separately by each thread \n");
    printf("  -r<align>[K|M|G|b]    random I/O aligned to <align> in bytes/KiB/MiB/GiB/blocks (overrides -s)\n");
    printf("  -R<text|xml>          output format. Default is text.\n");
    printf("  -s[i]<size>[K|M|G|b]  sequential stride size, offset between subsequent I/O operations\n");
    printf("                          [default access=non-interlocked sequential, default stride=block size]\n");
    printf("                          In non-interlocked mode, threads do not coordinate, so the pattern of offsets\n");
    printf("                          as seen by the target will not be truly sequential.  Under -si the threads\n");
    printf("                          manipulate a shared offset with InterlockedIncrement, which may reduce throughput,\n");
    printf("                          but promotes a more sequential pattern.\n");
    printf("                          (ignored if -r specified, -si conflicts with -T and -p)\n");
    printf("  -S[bhruw]             control caching behavior [default: caching is enabled, no writethrough]\n");
    printf("                          non-conflicting flags may be combined in any order; ex: -Sbw, -Suw, -Swu\n");
    printf("  -S                    equivalent to -Su\n");
    printf("  -Sb                   enable caching (default, explicitly stated)\n");
    printf("  -Sh                   equivalent -Suw\n");
    printf("  -Su                   disable software caching, equivalent to FILE_FLAG_NO_BUFFERING\n");
    printf("  -Sr                   disable local caching, with remote sw caching enabled; only valid for remote filesystems\n");
    printf("  -Sw                   enable writethrough (no hardware write caching), equivalent to FILE_FLAG_WRITE_THROUGH\n");
    printf("  -t<count>             number of threads per target (conflicts with -F)\n");
    printf("  -T<offs>[K|M|G|b]     starting stride between I/O operations performed on the same target by different threads\n");
    printf("                          [default=0] (starting offset = base file offset + (thread number * <offs>)\n");
    printf("                          makes sense only with #threads > 1\n");
    printf("  -v                    verbose mode\n");
    printf("  -w<percentage>        percentage of write requests (-w and -w0 are equivalent and result in a read-only workload).\n");
    printf("                        absence of this switch indicates 100%% reads\n");
    printf("                          IMPORTANT: a write test will destroy existing data without a warning\n");
    printf("  -W<seconds>           warm up time - duration of the test before measurements start [default=5s]\n");
    printf("  -x                    use completion routines instead of I/O Completion Ports\n");
    printf("  -X<filepath>          use an XML file for configuring the workload. Cannot be used with other parameters.\n");
    printf("  -z[seed]              set random seed [with no -z, seed=0; with plain -z, seed is based on system run time]\n");
    printf("\n");
    printf("Write buffers:\n");
    printf("  -Z                        zero buffers used for write tests\n");
    printf("  -Zr                       per IO random buffers used for write tests - this incurrs additional run-time\n");
    printf("                              overhead to create random content and shouln't be compared to results run\n");
    printf("                              without -Zr\n");
    printf("  -Z<size>[K|M|G|b]         use a <size> buffer filled with random data as a source for write operations.\n");
    printf("  -Z<size>[K|M|G|b],<file>  use a <size> buffer filled with data from <file> as a source for write operations.\n");
    printf("\n");
    printf("  By default, the write buffers are filled with a repeating pattern (0, 1, 2, ..., 255, 0, 1, ...)\n");
    printf("\n");
    printf("Synchronization:\n");
    printf("  -ys<eventname>     signals event <eventname> before starting the actual run (no warmup)\n");
    printf("                       (creates a notification event if <eventname> does not exist)\n");
    printf("  -yf<eventname>     signals event <eventname> after the actual run finishes (no cooldown)\n");
    printf("                       (creates a notification event if <eventname> does not exist)\n");
    printf("  -yr<eventname>     waits on event <eventname> before starting the run (including warmup)\n");
    printf("                       (creates a notification event if <eventname> does not exist)\n");
    printf("  -yp<eventname>     stops the run when event <eventname> is set; CTRL+C is bound to this event\n");
    printf("                       (creates a notification event if <eventname> does not exist)\n");
    printf("  -ye<eventname>     sets event <eventname> and quits\n");
    printf("\n");
    printf("Event Tracing:\n");
    printf("  -e<q|c|s>             Use query perf timer (qpc), cycle count, or system timer respectively.\n");
    printf("                          [default = q, query perf timer (qpc)]\n");
    printf("  -ep                   use paged memory for the NT Kernel Logger [default=non-paged memory]\n");
    printf("  -ePROCESS             process start & end\n");
    printf("  -eTHREAD              thread start & end\n");
    printf("  -eIMAGE_LOAD          image load\n");
    printf("  -eDISK_IO             physical disk IO\n");
    printf("  -eMEMORY_PAGE_FAULTS  all page faults\n");
    printf("  -eMEMORY_HARD_FAULTS  hard faults only\n");
    printf("  -eNETWORK             TCP/IP, UDP/IP send & receive\n");
    printf("  -eREGISTRY            registry calls\n");
    printf("\n\n");
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

bool CmdLineParser::_ParseAffinity(const char *arg, TimeSpan *pTimeSpan)
{
    bool fOk = true;

    assert(nullptr != arg);
    assert('\0' != *arg);

    const char *c = arg + 1;

    // -a and -ag are functionally equivalent; group-aware affinity.
    // Note that group-aware affinity is default.

    // look for the -a simple case
    if (*c == '\0')
    {
        return true;
    }

    // look for the -ag simple case
    if (*c == 'g')
    {
        // peek ahead, done?
        if (*(c + 1) == '\0')
        {
            return true;
        }

        // leave the parser at the g; this is the start of a group number
    }

    // more complex affinity -ag0,0,1,2,g1,0,1,2,... OR -a0,1,2,..
    // n counts the -a prefix, the first parsed character is string index 2
    DWORD nGroup = 0, nNum = 0, n = 2;
    bool fGroup = false, fNum = false;
    while (*c != '\0')
    {
        if ((*c >= '0') && (*c <= '9'))
        {
            // accumulating a number
            fNum = true;
            nNum = 10 * nNum + (*c - '0');
        }
        else if (*c == 'g')
        {
            // bad: ggggg
            if (fGroup)
            {
                fOk = false;
            }

            // now parsing a group number
            fGroup = true;
        }
        else if (*c == ',')
        {
            // separator; if parsing group and have a number, now have the group
            if (fGroup && fNum)
            {
                if (nNum > MAXWORD)
                {
                  fprintf(stderr, "ERROR: group %lu is out of range\n", nNum);
                  fOk = false;
                }
                else
                {
                    nGroup = nNum;
                    nNum = 0;
                    fGroup = false;
                }
            }
            // at a split but don't have a parsed number, error
            else if (!fNum)
            {
                fOk = false;
            }
            // have a parsed core number
            else
            {
                if (nNum > MAXBYTE)
                {
                    fprintf(stderr, "ERROR: core %lu is out of range\n", nNum);
                    fOk = false;
                }
                else
                {
                    pTimeSpan->AddAffinityAssignment(static_cast<WORD>(nGroup), static_cast<BYTE>(nNum));
                    nNum = 0;
                    fNum = false;
                }
            }
        }
        else
        {
            fOk = false;
        }

        // bail out to error pretty print on error
        if (!fOk)
        {
            break;
        }

        c++;
        n++;
    }

    // if parsing a group or don't have a final number, error
    if (fGroup || !fNum)
    {
        fOk = false;
    }

    if (fOk && nNum > MAXBYTE)
    {
        fprintf(stderr, "ERROR: core %lu is out of range\n", nNum);
        fOk = false;
    }

    if (!fOk)
    {
        // mid-parse error, show the point at which it occured
        if (*c != '\0') {
            fprintf(stderr, "ERROR: syntax error parsing affinity at highlighted character\n-%s\n", arg);
            while (n-- > 0)
            {
                fprintf(stderr, " ");
            }
            fprintf(stderr, "^\n");
        }
        else
        {
            fprintf(stderr, "ERROR: incomplete affinity specification\n");
        }
    }

    if (fOk)
    {
        // fprintf(stderr, "FINAL parsed group %d core %d\n", nGroup, nNum);
        pTimeSpan->AddAffinityAssignment(static_cast<WORD>(nGroup), static_cast<BYTE>(nNum));
    }

    return fOk;
}

bool CmdLineParser::_ReadParametersFromCmdLine(const int argc, const char *argv[], Profile *pProfile, struct Synchronization *synch)
{
    /* Process any command-line options */
    int nParamCnt = argc - 1;
    const char** args = argv + 1;

    // create targets
    vector<Target> vTargets;
    //int iFirstFile = -1;
    for (int i = 1; i < argc; i++)
    {
        if (argv[i][0] != '-' && argv[i][0] != '/')
        {
            //iFirstFile = i;
            Target target;
            target.SetPath(argv[i]);
            vTargets.push_back(target);
        }
    }

    // find block size (other parameters may be stated in terms of blocks)
    for (int x = 1; x < argc; ++x)
    {
        if ((nullptr != argv[x]) && (('-' == argv[x][0]) || ('/' == argv[x][0])) && ('b' == argv[x][1]) && ('\0' != argv[x][2]))
        {
            _dwBlockSize = 0;
            UINT64 ullBlockSize;
            if (_GetSizeInBytes(&argv[x][2], ullBlockSize))
            {
                for (auto& vTarget : vTargets)
                {
                    // TODO: UINT64->DWORD
	                vTarget.SetBlockSizeInBytes(static_cast<DWORD>(ullBlockSize));
                }
            }
            else
            {
                fprintf(stderr, "Invalid block size passed to -b\n");
                return false;
            }
            _dwBlockSize = static_cast<DWORD>(ullBlockSize);
            break;
        }
    }

    // initial parse for cache/writethrough
    // these are built up across the entire cmd line and applied at the end.
    // this allows for conflicts to be thrown for mixed -h/-S as needed.
    TargetCacheMode t = TargetCacheMode::Undefined;
    WriteThroughMode w = WriteThroughMode::Undefined;

    TimeSpan timeSpan;
    bool bExit = false;
    while (nParamCnt)
    {
        const char* arg = *args;
        bool fError = false;

        // check if it is a parameter or already path
        if ('-' != *arg && '/' != *arg)
        {
            break;
        }

        // skip '-' or '/'
        ++arg;

        switch (*arg)
        {
        case '\0':
            // back up so that the error complaint mentions the switch char
            arg--;
            fError = true;
            break;

        case '?':
            _DisplayUsageInfo(argv[0]);
            exit(0);

        case 'a':    //affinity
            //-a1,2,3,4 (assign threads to cpus 1,2,3,4 (round robin))
            if (!_ParseAffinity(arg, &timeSpan))
            {
                fError = true;
            }
            break;

        case 'b':    //block size
            // nop - block size has been taken care of before the loop
            break;

        case 'B':    //base file offset (offset from the beginning of the file)
            if (*(arg + 1) != '\0')
            {
                UINT64 cb;
                if (_GetSizeInBytes(arg + 1, cb))
                {
                    for (auto& vTarget : vTargets)
                    {
	                    vTarget.SetBaseFileOffsetInBytes(cb);
                    }
                }
                else
                {
                    fprintf(stderr, "Invalid base file offset passed to -B\n");
                    fError = true;
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
                if (_GetSizeInBytes(arg + 1, cb))
                {
                    for (auto& vTarget : vTargets)
                    {
	                    vTarget.SetFileSize(cb);
	                    vTarget.SetCreateFile(true);
                    }
                }
                else
                {
                    fprintf(stderr, "Invalid file size passed to -c\n");
                    fError = true;
                }
            }
            else
            {
                fError = true;
            }
            break;

        case 'C':    //cool down time
            {
	            const int c = atoi(arg + 1);
                if (c >= 0)
                {
                    timeSpan.SetCooldown(c);
                }
                else
                {
                    fError = true;
                }
            }
            break;

        case 'd':    //duration
            {
	            const int x = atoi(arg + 1);
                if (x > 0)
                {
                    timeSpan.SetDuration(x);
                }
                else
                {
                    fError = true;
                }
            }
            break;

        case 'D':    //standard deviation
            {
                timeSpan.SetCalculateIopsStdDev(true);

	            const int x = atoi(arg + 1);
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
            if (isdigit(*(arg + 1)))
            {
                UINT64 cb;
                if (_GetSizeInBytes(arg + 1, cb))
                {
                    for (auto& vTarget : vTargets)
                    {
	                    vTarget.SetMaxFileSize(cb);
                    }
                }
                else
                {
                    fprintf(stderr, "Invalid max file size passed to -f\n");
                    fError = true;
                }
            }
            else
            {
                if ('\0' == *(arg + 1))
                {
                    fError = true;
                }
                else
                {
                    // while -frs (or -fsr) are generally conflicting intentions as far as
                    // the OS is concerned, do not enforce
                    while (*(++arg) != '\0')
                    {
                        switch (*arg)
                        {
                        case 'r':
                            for (auto& vTarget : vTargets)
                            {
	                            vTarget.SetRandomAccessHint(true);
                            }
                            break;
                        case 's':
                            for (auto& vTarget : vTargets)
                            {
	                            vTarget.SetSequentialScanHint(true);
                            }
                            break;
                        case 't':
                            for (auto& vTarget : vTargets)
                            {
	                            vTarget.SetTemporaryFileHint(true);
                            }
                            break;
                        default:
                            fError = true;
                            break;
                        }
                    }
                }
            }
            break;

        case 'F':    //total number of threads
            {
	            const int c = atoi(arg + 1);
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

        case 'g':    //throughput in bytes per millisecond
            {
	            const int c = atoi(arg + 1);
                if (c > 0)
                {
                    for (auto& vTarget : vTargets)
                    {
	                    vTarget.SetThroughput(c);
                    }
                }
                else
                {
                    fError = true;
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
                fprintf(stderr, "-h conflicts with earlier specification of cache/writethrough\n");
                fError = true;
            }
            break;

        case 'i':    //number of IOs to issue before think time
            {
	            const int c = atoi(arg + 1);
                if (c > 0)
                {
                    for (auto& vTarget : vTargets)
                    {
	                    vTarget.SetBurstSize(c);
	                    vTarget.SetUseBurstSize(true);
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
	            const int c = atoi(arg + 1);
                if (c > 0)
                {
                    for (auto& vTarget : vTargets)
                    {
	                    vTarget.SetThinkTime(c);
	                    vTarget.SetEnableThinkTime(true);
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
	            const int x = atoi(arg + 1);
                if (x > 0 && x < 4)
                {
                    PRIORITY_HINT hint[] = { IoPriorityHintVeryLow, IoPriorityHintLow, IoPriorityHintNormal };
                    for (auto& vTarget : vTargets)
                    {
	                    vTarget.SetIOPriorityHint(hint[x - 1]);
                    }
                }
                else
                {
                    fError = true;
                }
            }
            break;

        case 'l':    //large pages
            for (auto& vTarget : vTargets)
            {
	            vTarget.SetUseLargePages(true);
            }
            break;
        
        case 'L':    //measure latency
            timeSpan.SetMeasureLatency(true);
            break;

        case 'n':    //disable affinity (by default simple affinity is turned on)
            timeSpan.SetDisableAffinity(true);
            break;

        case 'o':    //request count (1==synchronous)
            {
	            const int c = atoi(arg + 1);
                if (c > 0)
                {
                    for (auto& vTarget : vTargets)
                    {
	                    vTarget.SetRequestCount(c);
                    }
                }
                else
                {
                    fError = true;
                }
            }
            break;

        case 'O':   //total number of IOs/thread - for use with -F
            {
	            const int c = atoi(arg + 1);
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
            for (auto& vTarget : vTargets)
            {
	            vTarget.SetUseParallelAsyncIO(true);
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
                UINT64 cb = _dwBlockSize;
                if (*(arg + 1) != '\0')
                {
                    if (!_GetSizeInBytes(arg + 1, cb) || (cb == 0))
                    {
                        fprintf(stderr, "Invalid alignment passed to -r\n");
                        fError = true;
                    }
                }
                if (!fError)
                {
                    for (auto& vTarget : vTargets)
                    {
	                    vTarget.SetUseRandomAccessPattern(true);
	                    vTarget.SetBlockAlignmentInBytes(cb);
                    }
                }
            }
            break;

        case 'R':    //custom result parser
            if (0 != *(arg + 1))
            {
                const char* pszArg = arg + 1;
                if (strcmp(pszArg, "xml") == 0)
                {
                    pProfile->SetResultsFormat(ResultsFormat::Xml);
                }
                else if (strcmp(pszArg, "text") != 0)
                {
                    fError = true;
                    fprintf(stderr, "Invalid results format: '%s'.\n", pszArg);
                }
            }
            else
            {
                fError = true;
            }
            break;

        case 's':    //stride size
            {
                int idx = 1;

                if ('i' == *(arg + idx))
                {
                    // do interlocked sequential mode
                    // ISSUE-REVIEW: this does nothing if -r is specified
                    // ISSUE-REVIEW: this does nothing if -p is specified
                    // ISSUE-REVIEW: this does nothing if we are single-threaded 
                    for (auto& vTarget : vTargets)
                    {
	                    vTarget.SetUseInterlockedSequential(true);
                    }

                    idx++;
                }
                
                if (*(arg + idx) != '\0')
                {
                    UINT64 cb;
                    if (_GetSizeInBytes(arg + idx, cb))
                    {
                        for (auto& vTarget : vTargets)
                        {
	                        vTarget.SetBlockAlignmentInBytes(cb);
                        }
                    }
                    else
                    {
                        fprintf(stderr, "Invalid stride size passed to -s\n");
                        fError = true;
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
                            fprintf(stderr, "-Sb conflicts with earlier specification of cache mode\n");
                            fError = true;
                        }
                        break; 
                    case 'h':
                        if (t == TargetCacheMode::Undefined &&
                            w == WriteThroughMode::Undefined)
                        {
                            t = TargetCacheMode::DisableOSCache;
                            w = WriteThroughMode::On;
                        }
                        else
                        {
                            fprintf(stderr, "-Sh conflicts with earlier specification of cache/writethrough\n");
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
                            fprintf(stderr, "-Sr conflicts with earlier specification of cache mode\n");
                            fError = true;
                        }
                        break;
                    case 'u':
                        if (t == TargetCacheMode::Undefined)
                        {
                            t = TargetCacheMode::DisableOSCache;
                        }
                        else
                        {
                            fprintf(stderr, "-Su conflicts with earlier specification of cache mode\n");
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
                            fprintf(stderr, "-Sw conflicts with earlier specification of write through\n");
                            fError = true;
                        }
                        break;
                    default:
                        fprintf(stderr, "unrecognized option provided to -S\n");
                        fError = true;
                        break;
                    }
                }

                // bare -S, parse loop did not advance
                if (!fError && idx == 1)
                {
                    if (t == TargetCacheMode::Undefined)
                    {
                        t = TargetCacheMode::DisableOSCache;
                    }
                    else
                    {
                        fprintf(stderr, "-S conflicts with earlier specification of cache mode\n");
                        fError = true;
                    }
                }
            }
            break;

        case 't':    //number of threads per file
            {
	            const int c = atoi(arg + 1);
                if (c > 0)
                {
                    for (auto& vTarget : vTargets)
                    {
	                    vTarget.SetThreadsPerFile(c);
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
                if (_GetSizeInBytes(arg + 1, cb) && (cb > 0))
                {
                    for (auto& vTarget : vTargets)
                    {
	                    vTarget.SetThreadStrideInBytes(cb);
                    }
                }
                else
                {
                    fprintf(stderr, "Invalid offset passed to -T\n");
                    fError = true;
                }
            }
            break;

        case 'v':    //verbose mode
            pProfile->SetVerbose(true);
            break;

        case 'w':    //write test [default=read]
            {
                int c;
                if (*(arg + 1) == '\0')
                {
                    c = _ulWriteRatio;
                }
                else
                {
                    c = atoi(arg + 1);
                    if (c < 0 || c > 100)
                    {
                        c = -1;
                        fError = true;
                    }
                }
                if (c != -1)
                {
                    for (auto& vTarget : vTargets)
                    {
	                    vTarget.SetWriteRatio(c);
                    }
                }
            }
            break;

        case 'W':    //warm up time
            {
	            const int c = atoi(arg + 1);
                if (c >= 0)
                {
                    timeSpan.SetWarmup(c);
                }
                else
                {
                    fError = true;
                }
            }
            break;

        case 'x':    //completion routines
            timeSpan.SetCompletionRoutines(true);
            break;

        case 'y':    //external synchronization
            switch (*(arg + 1))
            {

            case 's':
                _hEventStarted = CreateEvent(nullptr, TRUE, FALSE, arg + 2);
                if (nullptr == _hEventStarted)
                {
                    fprintf(stderr, "Error creating/opening start notification event: '%s'\n", arg + 2);
                    exit(1);    // TODO: this class shouldn't terminate the process
                }
                break;

            case 'f':
                _hEventFinished = CreateEvent(nullptr, TRUE, FALSE, arg + 2);
                if (nullptr == _hEventFinished)
                {
                    fprintf(stderr, "Error creating/opening finish notification event: '%s'\n", arg + 2);
                    exit(1);    // TODO: this class shouldn't terminate the process
                }
                break;

            case 'r':
                synch->hStartEvent = CreateEvent(nullptr, TRUE, FALSE, arg + 2);
                if (nullptr == synch->hStartEvent)
                {
                    fprintf(stderr, "Error creating/opening wait-for-start event: '%s'\n", arg + 2);
                    exit(1);    // TODO: this class shouldn't terminate the process
                }
                break;

            case 'p':
                synch->hStopEvent = CreateEvent(nullptr, TRUE, FALSE, arg + 2);
                if (nullptr == synch->hStopEvent)
                {
                    fprintf(stderr, "Error creating/opening force-stop event: '%s'\n", arg + 2);
                    exit(1);    // TODO: this class shouldn't terminate the process
                }
                break;

            case 'e':
                {
	                HANDLE hEvent = OpenEvent(EVENT_MODIFY_STATE, FALSE, arg + 2);
                    if (nullptr == hEvent)
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
            if (*(arg + 1) == '\0')
            {
                timeSpan.SetRandSeed(static_cast<ULONG>(GetTickCount64()));
            }
            else
            {
	            const int c = atoi(arg + 1);
                if (c >= 0)
                {
                    timeSpan.SetRandSeed(c);
                }
                else
                {
                    fError = true;
                }
            }
            break;

        case 'Z':    //zero write buffers
            if (*(arg + 1) == '\0')
            {
                for (auto& vTarget : vTargets)
                {
	                vTarget.SetZeroWriteBuffers(true);
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
                    for (auto& vTarget : vTargets)
                    {
	                    vTarget.SetRandomDataWriteBufferSize(cb);
	                    vTarget.SetRandomDataWriteBufferSourcePath(sPath);
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
            fprintf(stderr, "ERROR: invalid option: '%s'\n", arg);
            return false;
        }

        if (fError)
        {
            fprintf(stderr, "ERROR: incorrectly provided option: '%s'\n", arg);
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

    if (vTargets.empty())
    {
        fprintf(stderr, "ERROR: need to provide at least one filename\n");
        return false;
    }

    // apply resultant cache/writethrough modes to the targets
    for (auto& vTarget : vTargets)
    {
        if (t != TargetCacheMode::Undefined)
        {
	        vTarget.SetCacheMode(t);
        }
        if (w != WriteThroughMode::Undefined)
        {
	        vTarget.SetWriteThroughMode(w);
        }
    }

    // ... and apply targets to the timespan
    for (auto& vTarget : vTargets)
    {
        timeSpan.AddTarget(vTarget);
    }
    pProfile->AddTimeSpan(timeSpan);

    return true;
}

bool CmdLineParser::_ReadParametersFromXmlFile(const char *pszPath, Profile *pProfile)
{
    return XmlProfileParser::ParseFile(pszPath, pProfile);
}

bool CmdLineParser::ParseCmdLine(const int argc, const char *argv[], Profile *pProfile, struct Synchronization *synch, SystemInformation *pSystem)
{
    assert(nullptr != argv);
    assert(nullptr != pProfile);
    assert(nullptr != synch);

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

    //check if parameters should be read from an xml file
    bool fOk;
    bool fCmdLine;

    if (argc == 2 && (argv[1][0] == '-' || argv[1][0] == '/') && argv[1][1] == 'X' && argv[1][2] != '\0')
    {
        fOk = _ReadParametersFromXmlFile(argv[1] + 2, pProfile);
        fCmdLine = false;
    }
    else
    {
        fOk = _ReadParametersFromCmdLine(argc, argv, pProfile, synch);
        fCmdLine = true;
    }
    
    // check additional restrictions and conditions on the passed parameters.
    // note that on the cmdline, all targets receive the same parameters so
    // that their mutual consistency only needs to be checked once.
    if (fOk)
    {
        fOk = pProfile->Validate(fCmdLine, pSystem);
    }

    return fOk;
}
