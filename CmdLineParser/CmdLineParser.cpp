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
#include "..\XmlProfileParser\XmlProfileParser.h"
#include <assert.h>
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

// get size in bytes from a string (it can end with K, M, G for KB, MB, GB)
// FUTURE EXTENSION: it can be optimized (remove strdup)
//
UINT64 CmdLineParser::_GetSizeInBytes(const char *pszSize) const
{
    if (nullptr == pszSize || '\0' == *pszSize)
    {
        return 0;
    }

    UINT64 ullSize = 0;

    size_t l = strlen(pszSize);
    if ('K' == toupper(pszSize[l - 1]))
    {
        char *s = _strdup(pszSize);
        s[l - 1] = '\0';
        ullSize = _atoi64(s) << 10;
        free(s);
    }
    else if ('M' == toupper(pszSize[l - 1]))
    {
        char *s = _strdup(pszSize);
        s[l - 1] = '\0';
        ullSize = _atoi64(s) << 20;
        free(s);
    }
    else if ('G' == toupper(pszSize[l - 1]))
    {
        char *s = _strdup(pszSize);
        s[l - 1] = '\0';
        ullSize = _atoi64(s) << 30;
        free(s);
    }
    else if ('B' == toupper(pszSize[l - 1]))
    {
        char *s = _strdup(pszSize);
        s[l - 1] = '\0';
        ullSize = _atoi64(s) * _dwBlockSize;
        free(s);
    }
    else
    {
        ullSize = _atoi64(pszSize);
    }
    //FUTURE EXTENSION: better checking of the value and suffixes

    return ullSize;
}

void CmdLineParser::_GetRandomDataWriteBufferData(const string& sArg, UINT64& cb, string& sPath)
{
    size_t iComma = sArg.find(',');
    if (iComma == sArg.npos)
    {
        cb = _GetSizeInBytes(sArg.c_str());
        sPath = "";
    }
    else
    {
        cb = _GetSizeInBytes(sArg.substr(0, iComma).c_str());
        sPath = sArg.substr(iComma + 1);
    }
}

void CmdLineParser::_DisplayUsageInfo(const char *pszFilename) const
{
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
    printf("  -?                 display usage information\n");
    printf("  -a#[,#[...]]       advanced CPU affinity - affinitize threads to CPUs provided after -a\n");
    printf("                       in a round-robin manner within current KGroup (CPU count starts with 0); the same CPU\n");
    printf("                       can be listed more than once and the number of CPUs can be different\n");
    printf("                       than the number of files or threads (cannot be used with -n)\n");
    printf("  -ag                group affinity - affinitize threads in a round-robin manner across KGroups\n");
    printf("  -b<size>[K|M|G]    block size in bytes/KB/MB/GB [default=64K]\n");
    printf("  -B<offs>[K|M|G|b]  base file offset in bytes/KB/MB/GB/blocks [default=0]\n");
    printf("                       (offset from the beginning of the file)\n");
    printf("  -c<size>[K|M|G|b]  create files of the given size.\n");
    printf("                       Size can be stated in bytes/KB/MB/GB/blocks\n");
    printf("  -C<seconds>        cool down time - duration of the test after measurements finished [default=0s].\n");
    printf("  -D<bucketDuration> Print IOPS standard deviations. The deviations are calculated for samples of duration <bucketDuration>.\n");
    printf("                       <bucketDuration> is given in milliseconds and the default value is 1000.\n");
    printf("  -d<seconds>        duration (in seconds) to run test [default=10s]\n");
    printf("  -f<size>[K|M|G|b]  file size - this parameter can be used to use only the part of the file/disk/partition\n");
    printf("                       for example to test only the first sectors of disk\n");
    printf("  -fr                open file with the FILE_FLAG_RANDOM_ACCESS hint\n");
    printf("  -fs                open file with the FILE_FLAG_SEQUENTIAL_SCAN hint\n");
    printf("  -F<count>          total number of threads (cannot be used with -t)\n");
    printf("  -g<bytes per ms>   throughput per thread is throttled to given bytes per millisecond\n");
    printf("                       note that this can not be specified when using completion routines\n");
    printf("  -h                 disable both software and hardware caching\n");
    printf("  -i<count>          number of IOs (burst size) before thinking. must be specified with -j\n");
    printf("  -j<duration>       time to think in ms before issuing a burst of IOs (burst size). must be specified with -i\n");
    printf("  -I<priority>       Set IO priority to <priority>. Available values are: 1-very low, 2-low, 3-normal (default)\n");
    printf("  -l                 Use large pages for IO buffers\n");
    printf("  -L                 measure latency statistics\n");
    printf("  -n                 disable affinity (cannot be used with -a)\n");
    printf("  -o<count>          number of overlapped I/O requests per file per thread\n");
    printf("                       (1=synchronous I/O, unless more than 1 thread is specified with -F)\n");
    printf("                       [default=2]\n");
    printf("  -p                 start async (overlapped) I/O operations with the same offset\n");
    printf("                       (makes sense only with -o2 or grater)\n");
    printf("  -P<count>          enable printing a progress dot after each <count> completed I/O operations\n");
    printf("                       (counted separately by each thread) [default count=65536]\n");
    printf("  -r<align>[K|M|G|b] random I/O aligned to <align> bytes (doesn't make sense with -s).\n");
    printf("                       <align> can be stated in bytes/KB/MB/GB/blocks\n");
    printf("                       [default access=sequential, default alignment=block size]\n");
    printf("  -R<text|xml>       output format. Default is text.\n");
    printf("  -s<size>[K|M|G|b]  stride size (offset between starting positions of subsequent I/O operations)\n");
    printf("  -S                 disable OS caching\n");
    printf("  -t<count>          number of threads per file (cannot be used with -F)\n");
    printf("  -T<offs>[K|M|G|b]  stride between I/O operations performed on the same file by different threads\n");
    printf("                       [default=0] (starting offset = base file offset + (thread number * <offs>)\n");
    printf("                       it makes sense only with -t or -F\n");
    printf("  -v                 verbose mode\n");
    printf("  -w<percentage>     percentage of write requests (-w and -w0 are equivalent).\n");
    printf("                     absence of this switch indicates 100%% reads\n");
    printf("                       IMPORTANT: Your data will be destroyed without a warning\n");
    printf("  -W<seconds>        warm up time - duration of the test before measurements start [default=5s].\n");
    printf("  -x                 use completion routines instead of I/O Completion Ports\n");
    printf("  -X<path>           use an XML file for configuring the workload. Cannot be used with other parameters.\n");
    printf("  -z                 set random seed [default=0 if parameter not provided, GetTickCount() if value not provided]\n");
    printf("\n");
    printf("Write buffers:\n");
    printf("  -Z                        zero buffers used for write tests\n");
    printf("  -Z<size>[K|M|G|b]         use a global <size> buffer filled with random data as a source for write operations.\n");
    printf("  -Z<size>[K|M|G|b],<file>  use a global <size> buffer filled with data from <file> as a source for write operations.\n");
    printf("                              If <file> is smaller than <size>, its content will be repeated multiple times in the buffer.\n");
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
    printf("  -yp<eventname>     allows to stop the run when event <eventname> is set; it also binds CTRL+C to this event\n");
    printf("                       (creates a notification event if <eventname> does not exist)\n");
    printf("  -ye<eventname>     sets event <eventname> and quits\n");
    printf("\n");
    printf("Event Tracing:\n");
    printf("  -ep                   use paged memory for NT Kernel Logger (by default it uses non-paged memory)\n");
    printf("  -eq                   use perf timer\n");
    printf("  -es                   use system timer (default)\n");
    printf("  -ec                   use cycle count\n");
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
    printf("  %s -b4K -t2 -r -o32 -d10 -h testfile.dat\n\n", pszFilename);
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
    if (*c == '\0')
    {
        // simple affinity (-a) is turned on by default, do nothing
        return false;
    }

    if (*c == 'g')
    {
        pTimeSpan->SetGroupAffinity(true);
        return true;
    }

    // TODO: will treat ,, as ,0,
    // more complex affinity (-a#[,#[,#...]])
    int nCpu = 0;
    while (fOk && (*c != '\0'))
    {
        if ((*c >= '0') && (*c <= '9'))
        {
            nCpu = 10 * nCpu + (*c - '0');
        }
        else if (*c == ',')
        {
            pTimeSpan->AddAffinityAssignment(nCpu);
            nCpu = 0;
        }
        else
        {
            fOk = false;
            fprintf(stderr, "error parsing affinity (invalid character: %c)\n", *c);
        }
        c++;
    }
    if (fOk)
    {
        pTimeSpan->AddAffinityAssignment(nCpu);
    }

    return fOk;
}

bool CmdLineParser::_ReadParametersFromCmdLine(const int argc, const char *argv[], Profile *pProfile, struct Synchronization *synch)
{
    /* Process any command-line options */
    int nParamCnt = argc - 1;
    const char** args = argv + 1;

    // find block size (other parameters may be stated in terms of blocks)
    for (int x = 1; x<argc; ++x)
    {
        if ((nullptr != argv[x]) && (('-' == argv[x][0]) || ('/' == argv[x][0])) && ('b' == argv[x][1]) && ('\0' != argv[x][2]))
        {
            _dwBlockSize = (DWORD)_GetSizeInBytes(&argv[x][2]);
            break;
        }
    }

    // create targets
    vector<Target> vTargets;
    int iFirstFile = -1;
    for (int i = 1; i < argc; i++)
    {
        if (argv[i][0] != '-' && argv[i][0] != '/')
        {
            iFirstFile = i;
            Target target;
            target.SetPath(argv[i]);
            vTargets.push_back(target);
        }
    }

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

        if ('\0' == *arg)
        {
            fprintf(stderr, "Invalid option\n");
            exit(1);
        }

        switch (*arg)
        {
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
            {
                UINT64 cb = _GetSizeInBytes(arg + 1);
                if (cb > 0)
                {
                    for (auto i = vTargets.begin(); i != vTargets.end(); i++)
                    {
                        // TODO: UINT64->DWORD
                        i->SetBlockSizeInBytes((DWORD)cb);
                    }
                }
                else
                {
                    fError = true;
                }
            }
            break;

        case 'B':    //base file offset (offset from the beggining of the file), cannot be used with 'random'
            if (*(arg + 1) != '\0')
            {
                UINT64 cb = _GetSizeInBytes(arg + 1);
                for (auto i = vTargets.begin(); i != vTargets.end(); i++)
                {
                    i->SetBaseFileOffsetInBytes(cb);
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
                UINT64 cb = _GetSizeInBytes(arg + 1);
                for (auto i = vTargets.begin(); i != vTargets.end(); i++)
                {
                    i->SetFileSize(cb);
                    i->SetCreateFile(true);
                }
            }
            else
            {
                fError = true;
            }
            break;

        case 'C':    //cool down time
            {
                int c = atoi(arg + 1);
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
                int x = atoi(arg + 1);
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
            if ('r' == *(arg + 1))
            {
                for (auto i = vTargets.begin(); i != vTargets.end(); i++)
                {
                    i->SetRandomAccess(true);
                }
            }
            else if ('s' == *(arg + 1))
            {
                for (auto i = vTargets.begin(); i != vTargets.end(); i++)
                {
                    i->SetSequentialScan(true);
                }
            }
            else
            {
                if (*(arg + 1) != '\0')
                {
                    UINT64 cb = _GetSizeInBytes(arg + 1);
                    for (auto i = vTargets.begin(); i != vTargets.end(); i++)
                    {
                        i->SetMaxFileSize(cb);
                    }
                }
                else
                {
                    fError = true;
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

        case 'g':    //throughput in bytes per millisecond
            {
                int c = atoi(arg + 1);
                if (c > 0)
                {
                    for (auto i = vTargets.begin(); i != vTargets.end(); i++)
                    {
                        i->SetThroughput(c);
                    }
                }
                else
                {
                    fError = true;
                }
            }
            break;

        case 'h':    //disable both software and hardware caching
            for (auto i = vTargets.begin(); i != vTargets.end(); i++)
            {
                i->SetDisableAllCache(true);
            }
            break;

        case 'i':    //number of IOs to issue before think time
            {
                int c = atoi(arg + 1);
                if (c > 0)
                {
                    for (auto i = vTargets.begin(); i != vTargets.end(); i++)
                    {
                        i->SetBurstSize(c);
                        i->SetUseBurstSize(true);
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
                    for (auto i = vTargets.begin(); i != vTargets.end(); i++)
                    {
                        i->SetThinkTime(c);
                        i->SetEnableThinkTime(true);
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
                    for (auto i = vTargets.begin(); i != vTargets.end(); i++)
                    {
                        i->SetIOPriorityHint(hint[x - 1]);
                    }
                }
                else
                {
                    fError = true;
                }
            }
            break;

        case 'l':    //large pages
            for (auto i = vTargets.begin(); i != vTargets.end(); i++)
            {
                i->SetUseLargePages(true);
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
                int c = atoi(arg + 1);
                if (c > 0)
                {
                    for (auto i = vTargets.begin(); i != vTargets.end(); i++)
                    {
                        i->SetRequestCount(c);
                    }
                }
                else
                {
                    fError = true;
                }
            }
            break;

        case 'p':    //start async IO operations with the same offset
            //makes sense only for -o2 and greater
            for (auto i = vTargets.begin(); i != vTargets.end(); i++)
            {
                i->SetUseParallelAsyncIO(true);
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
                UINT64 cb = _GetSizeInBytes(arg + 1);
                if (cb < 1)
                {
                    cb = _dwBlockSize;
                }
                for (auto i = vTargets.begin(); i != vTargets.end(); i++)
                {
                    i->SetRandomAlignmentInBytes(cb);
                    i->SetUseRandomAlignment(true);
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
            if (*(arg + 1) != '\0')
            {
                UINT64 cb = _GetSizeInBytes(arg + 1);
                for (auto i = vTargets.begin(); i != vTargets.end(); i++)
                {
                    i->SetStrideSizeInBytes(cb);
                    i->SetEnableCustomStrideSize(true);
                }
            }
            else
            {
                fError = true;
            }
            break;

        case 'S':   //disable OS caching (software buffering)
            //IMPORTANT: File access must begin at byte offsets within the file that are integer multiples of the volume's sector size. 
            // File access must be for numbers of bytes that are integer multiples of the volume's sector size. For example, if the sector
            // size is 512 bytes, an application can request reads and writes of 512, 1024, or 2048 bytes, but not of 335, 981, or 7171 bytes. 
            // Buffer addresses for read and write operations should be sector aligned (aligned on addresses in memory that are integer
            // multiples of the volume's sector size). Depending on the disk, this requirement may not be enforced. 
            for (auto i = vTargets.begin(); i != vTargets.end(); i++)
            {
                i->SetDisableOSCache(true);
            }
            break;

        case 't':    //number of threads per file
            {
                int c = atoi(arg + 1);
                if (c > 0)
                {
                    for (auto i = vTargets.begin(); i != vTargets.end(); i++)
                    {
                        i->SetThreadsPerFile(c);
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
                UINT64 cb = _GetSizeInBytes(arg + 1);
                if (cb > 0)
                {
                    for (auto i = vTargets.begin(); i != vTargets.end(); i++)
                    {
                        i->SetThreadStrideInBytes(cb);
                    }
                }
                else
                {
                    fError = true;
                }
            }
            break;

        case 'v':    //verbose mode
            pProfile->SetVerbose(true);
            break;

        case 'w':    //write test [default=read]
            {
                int c = -1;
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
                    for (auto i = vTargets.begin(); i != vTargets.end(); i++)
                    {
                        i->SetWriteRatio(c);
                    }
                }
            }
            break;

        case 'W':    //warm up time
            {
                int c = atoi(arg + 1);
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
            if (*(arg + 1) == '\0')
            {
                timeSpan.SetRandSeed(GetTickCount());
            }
            else
            {
                int c = atoi(arg + 1);
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
                for (auto i = vTargets.begin(); i != vTargets.end(); i++)
                {
                    i->SetZeroWriteBuffers(true);
                }
            }
            else
            {
                UINT64 cb = 0;
                string sPath;
                _GetRandomDataWriteBufferData(string(arg + 1), cb, sPath);
                if (cb > 0)
                {
                    for (auto i = vTargets.begin(); i != vTargets.end(); i++)
                    {
                        i->SetRandomDataWriteBufferSize(cb);
                        i->SetRandomDataWriteBufferSourcePath(sPath);
                    }
                }
                else
                {
                    fError = true;
                }
            }
            break;

        default:
            fprintf(stderr, "Invalid option: '%s'\n", arg);
            exit(1);                    // TODO: this class shouldn't terminate the process
        }

        if (fError)
        {
            fprintf(stderr, "Incorrectly provided option: '%s'\n", arg);
            exit(1);                    // TODO: this class shouldn't terminate the process
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
        fprintf(stderr, "You need to provide at least one filename\n");
        return false;
    }

    for (auto i = vTargets.begin(); i != vTargets.end(); i++)
    {
        timeSpan.AddTarget(*i);
    }
    pProfile->AddTimeSpan(timeSpan);

    return true;
}

bool CmdLineParser::_ReadParametersFromXmlFile(const char *pszPath, Profile *pProfile)
{
    XmlProfileParser parser;
    return parser.ParseFile(pszPath, pProfile);
}

bool CmdLineParser::ParseCmdLine(const int argc, const char *argv[], Profile *pProfile, struct Synchronization *synch)
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

    //check if parameters should be read from an xml file
    bool fOk = true;

    if (argc == 2 && (argv[1][0] == '-' || argv[1][0] == '/') && argv[1][1] == 'X' && argv[1][2] != '\0')
    {
        fOk = _ReadParametersFromXmlFile(argv[1] + 2, pProfile);
    }
    else
    {
        fOk = _ReadParametersFromCmdLine(argc, argv, pProfile, synch);
    }

    return fOk;
}