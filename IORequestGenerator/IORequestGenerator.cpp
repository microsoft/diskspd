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

//FUTURE EXTENSION: make it compile with /W4

// Windows 7
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0601
#endif

#include "common.h"
#include "IORequestGenerator.h"

#include <stdio.h>
#include <stdlib.h>
#include <Winioctl.h>   //DISK_GEOMETRY
#include <windows.h>
#include <stddef.h>

#include <Wmistr.h>     //WNODE_HEADER

#include "etw.h"
#include <assert.h>
#include "ThroughputMeter.h"
#include "OverlappedQueue.h"

// Flags for RtlFlushNonVolatileMemory
#ifndef FLUSH_NV_MEMORY_IN_FLAG_NO_DRAIN
#define FLUSH_NV_MEMORY_IN_FLAG_NO_DRAIN    (0x00000001)
#endif

/*****************************************************************************/
// gets size of a dynamic volume, return zero on failure
//
UINT64 GetDynamicPartitionSize(HANDLE hFile)
{
    assert(NULL != hFile && INVALID_HANDLE_VALUE != hFile);

    UINT64 size = 0;
    VOLUME_DISK_EXTENTS diskExt = {0};
    PVOLUME_DISK_EXTENTS pDiskExt = &diskExt;
    DWORD bytesReturned;

    DWORD status = ERROR_SUCCESS;
    BOOL rslt;

    OVERLAPPED ovlp = {0};
    ovlp.hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    if (ovlp.hEvent == nullptr)
    {
        PrintError("ERROR: Failed to create event (error code: %u)\n", GetLastError());
        return 0;
    }

    rslt = DeviceIoControl(hFile,
                            IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS,
                            NULL,
                            0,
                            pDiskExt,
                            sizeof(VOLUME_DISK_EXTENTS),
                            &bytesReturned,
                            &ovlp);
    if (!rslt) {
        status = GetLastError();
        if (status == ERROR_MORE_DATA) {
            status = ERROR_SUCCESS;

            bytesReturned = sizeof(VOLUME_DISK_EXTENTS) + ((pDiskExt->NumberOfDiskExtents - 1) * sizeof(DISK_EXTENT));
            pDiskExt = (PVOLUME_DISK_EXTENTS)LocalAlloc(LPTR, bytesReturned);

            if (pDiskExt)
            {
                rslt = DeviceIoControl(hFile,
                                    IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS,
                                    NULL,
                                    0,
                                    pDiskExt,
                                    bytesReturned,
                                    &bytesReturned,
                                    &ovlp);
                if (!rslt)
                {
                    status = GetLastError();
                    if (status == ERROR_IO_PENDING)
                    {
                        if (WAIT_OBJECT_0 != WaitForSingleObject(ovlp.hEvent, INFINITE))
                        {
                            status = GetLastError();
                            PrintError("ERROR: Failed while waiting for event to be signaled (error code: %u)\n", status);
                        }
                        else
                        {
                            status = ERROR_SUCCESS;
                            assert(pDiskExt->NumberOfDiskExtents <= 1);
                        }
                    }
                    else
                    {
                        PrintError("ERROR: Could not obtain dynamic volume extents (error code: %u)\n", status);
                    }
                }
            }
            else
            {
                status = GetLastError();
                PrintError("ERROR: Could not allocate memory (error code: %u)\n", status);
            }
        }
        else if (status == ERROR_IO_PENDING)
        {
            if (WAIT_OBJECT_0 != WaitForSingleObject(ovlp.hEvent, INFINITE))
            {
                status = GetLastError();
                PrintError("ERROR: Failed while waiting for event to be signaled (error code: %u)\n", status);
            }
            else
            {
                status = ERROR_SUCCESS;
                assert(pDiskExt->NumberOfDiskExtents <= 1);
            }
        }
        else
        {
            PrintError("ERROR: Could not obtain dynamic volume extents (error code: %u)\n", status);
        }
    }
    else
    {
        assert(pDiskExt->NumberOfDiskExtents <= 1);
    }

    if (status == ERROR_SUCCESS)
    {
        for (DWORD n = 0; n < pDiskExt->NumberOfDiskExtents; n++) {
            size += pDiskExt->Extents[n].ExtentLength.QuadPart;
        }
    }

    if (pDiskExt && (pDiskExt != &diskExt)) {
        LocalFree(pDiskExt);
    }
    CloseHandle(ovlp.hEvent);

    return size;
}

/*****************************************************************************/
// gets partition size, return zero on failure
//
UINT64 GetPartitionSize(HANDLE hFile)
{
    assert(NULL != hFile && INVALID_HANDLE_VALUE != hFile);

    PARTITION_INFORMATION_EX pinf;
    OVERLAPPED ovlp = {};

    ovlp.hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    if (ovlp.hEvent == nullptr)
    {
        PrintError("ERROR: Failed to create event (error code: %u)\n", GetLastError());
        return 0;
    }

    DWORD rbcnt = 0;
    DWORD status = ERROR_SUCCESS;
    UINT64 size = 0;

    if (!DeviceIoControl(hFile,
                        IOCTL_DISK_GET_PARTITION_INFO_EX,
                        NULL,
                        0,
                        &pinf,
                        sizeof(pinf),
                        &rbcnt,
                        &ovlp)
        )
    {
        status = GetLastError();
        if (status == ERROR_IO_PENDING)
        {
            if (WAIT_OBJECT_0 != WaitForSingleObject(ovlp.hEvent, INFINITE))
            {
                PrintError("ERROR: Failed while waiting for event to be signaled (error code: %u)\n", GetLastError());
            }
            else
            {
                size = pinf.PartitionLength.QuadPart;
            }
        }
        else
        {
            size = GetDynamicPartitionSize(hFile);
        }
    }
    else
    {
        size = pinf.PartitionLength.QuadPart;
    }

    CloseHandle(ovlp.hEvent);

    return size;
}

/*****************************************************************************/
// gets physical drive size, return zero on failure
//
UINT64 GetPhysicalDriveSize(HANDLE hFile)
{
    assert(NULL != hFile && INVALID_HANDLE_VALUE != hFile);

    DISK_GEOMETRY_EX geom;
    OVERLAPPED ovlp = {};

    ovlp.hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    if (ovlp.hEvent == nullptr)
    {
        PrintError("ERROR: Failed to create event (error code: %u)\n", GetLastError());
        return 0;
    }

    DWORD rbcnt = 0;
    DWORD status = ERROR_SUCCESS;
    BOOL rslt;

    rslt = DeviceIoControl(hFile,
        IOCTL_DISK_GET_DRIVE_GEOMETRY_EX,
        NULL,
        0,
        &geom,
        sizeof(geom),
        &rbcnt,
        &ovlp);

    if (!rslt)
    {
        status = GetLastError();
        if (status == ERROR_IO_PENDING)
        {
            if (WAIT_OBJECT_0 != WaitForSingleObject(ovlp.hEvent, INFINITE))
            {
                PrintError("ERROR: Failed while waiting for event to be signaled (error code: %u)\n", GetLastError());
            }
            else
            {
                rslt = TRUE;
            }
        }
        else
        {
            PrintError("ERROR: Could not obtain drive geometry (error code: %u)\n", status);
        }
    }

    CloseHandle(ovlp.hEvent);

    if (!rslt)
    {
        return 0;
    }

    return (UINT64)geom.DiskSize.QuadPart;
}

/*****************************************************************************/
// activates specified privilege in process token
//
bool SetPrivilege(LPCSTR pszPrivilege, LPCSTR pszErrorPrefix = "ERROR:")
{
    TOKEN_PRIVILEGES TokenPriv;
    HANDLE hToken = INVALID_HANDLE_VALUE;
    DWORD dwError;
    bool fOk = true;

    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES, &hToken))
    {
        PrintError("%s Error opening process token (error code: %u)\n", pszErrorPrefix, GetLastError());
        fOk = false;
        goto cleanup;
    }

    TokenPriv.PrivilegeCount = 1;
    TokenPriv.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
    if (!LookupPrivilegeValue(nullptr, pszPrivilege, &TokenPriv.Privileges[0].Luid))
    {
        PrintError("%s Error looking up privilege value %s (error code: %u)\n", pszErrorPrefix, pszPrivilege, GetLastError());
        fOk = false;
        goto cleanup;
    }

    if (!AdjustTokenPrivileges(hToken, FALSE, &TokenPriv, 0, nullptr, nullptr))
    {
        PrintError("%s Error adjusting token privileges for %s (error code: %u)\n", pszErrorPrefix, pszPrivilege, GetLastError());
        fOk = false;
        goto cleanup;
    }

    if (ERROR_SUCCESS != (dwError = GetLastError()))
    {
        PrintError("%s Error adjusting token privileges for %s (error code: %u)\n", pszErrorPrefix, pszPrivilege, dwError);
        fOk = false;
        goto cleanup;
    }

cleanup:
    if (hToken != INVALID_HANDLE_VALUE)
    {
        CloseHandle(hToken);
    }

    return fOk;
}

BOOL
DisableLocalCache(
    HANDLE h
)
/*++
Routine Description:

    Disables local caching of I/O to a file by SMB. All reads/writes will flow to the server.

Arguments:

    h - Handle to the file

Return Value:

    Returns ERROR_SUCCESS (0) on success, nonzero error code on failure.

--*/
{
    DWORD BytesReturned = 0;
    OVERLAPPED Overlapped = { 0 };
    DWORD Status = ERROR_SUCCESS;
    BOOL Success = false;

    Overlapped.hEvent = CreateEvent(nullptr, true, false, nullptr);
    if (!Overlapped.hEvent)
    {
        return GetLastError();
    }

#ifndef FSCTL_DISABLE_LOCAL_BUFFERING
#define FSCTL_DISABLE_LOCAL_BUFFERING   CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 174, METHOD_BUFFERED, FILE_ANY_ACCESS)
#endif

    Success = DeviceIoControl(h,
        FSCTL_DISABLE_LOCAL_BUFFERING,
        nullptr,
        0,
        nullptr,
        0,
        nullptr,
        &Overlapped);

    if (!Success) {
        Status = GetLastError();
    }

    if (!Success && Status == ERROR_IO_PENDING)
    {
        if (!GetOverlappedResult(h, &Overlapped, &BytesReturned, true))
        {
            Status = GetLastError();
        }
        else
        {
            Status = (DWORD) Overlapped.Internal;
        }
    }

    if (Overlapped.hEvent)
    {
        CloseHandle(Overlapped.hEvent);
    }

    return Status;
}

/*****************************************************************************/
// structures and global variables
//
struct ETWEventCounters g_EtwEventCounters;

__declspec(align(4)) static LONG volatile g_lRunningThreadsCount = 0;   //must be aligned on a 32-bit boundary, otherwise InterlockedIncrement
                                                                        //and InterlockedDecrement will fail on 64-bit systems

static BOOL volatile g_bRun;                    //used for letting threads know that they should stop working

typedef NTSTATUS (__stdcall *NtQuerySysInfo)(SYSTEM_INFORMATION_CLASS, PVOID, ULONG, PULONG);
static NtQuerySysInfo g_pfnNtQuerySysInfo;

typedef VOID (__stdcall *RtlCopyMemNonTemporal)(VOID UNALIGNED *, VOID UNALIGNED *, SIZE_T);
static RtlCopyMemNonTemporal g_pfnRtlCopyMemoryNonTemporal;

typedef NTSTATUS (__stdcall *RtlFlushNvMemory)(PVOID, PVOID, SIZE_T, ULONG);
static RtlFlushNvMemory g_pfnRtlFlushNonVolatileMemory;

typedef NTSTATUS(__stdcall *RtlGetNvToken)(PVOID, SIZE_T, PVOID *);
static RtlGetNvToken g_pfnRtlGetNonVolatileToken;

typedef NTSTATUS(__stdcall *RtlFreeNvToken)(PVOID);
static RtlFreeNvToken g_pfnRtlFreeNonVolatileToken;

static BOOL volatile g_bThreadError = FALSE;    //true means that an error has occured in one of the threads
BOOL volatile g_bTracing = TRUE;                //true means that ETW is turned on

// TODO: is this still needed?
__declspec(align(4)) static LONG volatile g_lGeneratorRunning = 0;  //used to detect if GenerateRequests is already running

static BOOL volatile g_bError = FALSE;                              //true means there was fatal error during intialization and threads shouldn't perform their work

VOID SetProcGroupMask(WORD wGroupNum, DWORD dwProcNum, PGROUP_AFFINITY pGroupAffinity)
{
    //must zero this structure first, otherwise it fails to set affinity
    memset(pGroupAffinity, 0, sizeof(GROUP_AFFINITY));

    pGroupAffinity->Group = wGroupNum;
    pGroupAffinity->Mask = (KAFFINITY)1<<dwProcNum;
}

/*****************************************************************************/
void IORequestGenerator::_CloseOpenFiles(vector<HANDLE>& vhFiles) const
{
    for (size_t x = 0; x < vhFiles.size(); ++x)
    {
        if ((INVALID_HANDLE_VALUE != vhFiles[x]) && (nullptr != vhFiles[x]))
        {
            if (!CloseHandle(vhFiles[x]))
            {
                PrintError("Warning: unable to close file handle (error code: %u)\n", GetLastError());
            }
            vhFiles[x] = nullptr;
        }
    }
}

/*****************************************************************************/
// wrapper for stderr
void PrintError(const char *format, ...)
{
    assert(NULL != format);

    va_list listArg;
    va_start(listArg, format);
    vfprintf(stderr, format, listArg);
    va_end(listArg);
}

/*****************************************************************************/
// prints the string only if verbose mode is set to true
//
static void PrintVerbose(bool fVerbose, const char *format, ...)
{
    assert(NULL != format);

    if(fVerbose )
    {
        va_list argList;
        va_start(argList, format);
        vprintf(format, argList);
        va_end(argList);
    }
}

/*****************************************************************************/
// thread for gathering ETW data (etw functions are defined in etw.cpp)
//
DWORD WINAPI etwThreadFunc(LPVOID cookie)
{
    UNREFERENCED_PARAMETER(cookie);

    g_bTracing = TRUE;
    BOOL result = TraceEvents();
    g_bTracing = FALSE;

    return result ? 0 : 1;
}

/*****************************************************************************/
bool IORequestGenerator::_LoadDLLs()
{
    _hNTDLL = LoadLibraryExW(L"ntdll.dll", nullptr, 0);
    if( nullptr == _hNTDLL )
    {
        return false;
    }

    g_pfnNtQuerySysInfo = (NtQuerySysInfo)GetProcAddress(_hNTDLL, "NtQuerySystemInformation");
    if( nullptr == g_pfnNtQuerySysInfo )
    {
        return false;
    }

    g_pfnRtlCopyMemoryNonTemporal = (RtlCopyMemNonTemporal)GetProcAddress(_hNTDLL, "RtlCopyMemoryNonTemporal");
    g_pfnRtlFlushNonVolatileMemory = (RtlFlushNvMemory)GetProcAddress(_hNTDLL, "RtlFlushNonVolatileMemory");
    g_pfnRtlGetNonVolatileToken = (RtlGetNvToken)GetProcAddress(_hNTDLL, "RtlGetNonVolatileToken");
    g_pfnRtlFreeNonVolatileToken = (RtlFreeNvToken)GetProcAddress(_hNTDLL, "RtlFreeNonVolatileToken");

    return true;
}

/*****************************************************************************/
bool IORequestGenerator::_GetSystemPerfInfo(SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION *pInfo, UINT32 uCpuCount) const
{
    NTSTATUS Status = NO_ERROR;
    UINT32 uCpuCtr;
    WORD wActiveGroupCtr;
    BYTE bActiveProc;
    HANDLE hThread = GetCurrentThread();
    GROUP_AFFINITY GroupAffinity;
    PROCESSOR_NUMBER procNumber;
    bool fOk = true;

    assert(NULL != pInfo);
    assert(uCpuCount > 0);

    for (uCpuCtr=0,wActiveGroupCtr=0; wActiveGroupCtr < g_SystemInformation.processorTopology._vProcessorGroupInformation.size(); wActiveGroupCtr++)
    {
        ProcessorGroupInformation *pGroup = &g_SystemInformation.processorTopology._vProcessorGroupInformation[wActiveGroupCtr];

        if (pGroup->_activeProcessorCount != 0) {

            //
            // Affinitize to the group we're querying counters from
            //

            GetCurrentProcessorNumberEx(&procNumber);

            if (procNumber.Group != wActiveGroupCtr)
            {
                for (bActiveProc = 0; bActiveProc < pGroup->_maximumProcessorCount; bActiveProc++)
                {
                    if (pGroup->IsProcessorActive(bActiveProc))
                    {
                        SetProcGroupMask(wActiveGroupCtr, bActiveProc, &GroupAffinity);
                        break;
                    }
                }

                if (bActiveProc == pGroup->_maximumProcessorCount ||
                    SetThreadGroupAffinity(hThread, &GroupAffinity, nullptr) == FALSE)
                {
                    fOk = false;
                    break;
                }
            }

            Status = g_pfnNtQuerySysInfo(SystemProcessorPerformanceInformation,
                                         (PVOID)(pInfo + uCpuCtr),
                                         (sizeof(*pInfo) * uCpuCount) - (uCpuCtr * sizeof(SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION)),
                                         NULL);

            if (!NT_SUCCESS(Status))
            {
                fOk = false;
                break;
            }
        }

        uCpuCtr += pGroup->_maximumProcessorCount;
    }

    return fOk;
}

VOID CALLBACK fileIOCompletionRoutine(DWORD dwErrorCode, DWORD dwBytesTransferred, LPOVERLAPPED pOverlapped);

static bool issueNextIO(ThreadParameters *p, IORequest *pIORequest, DWORD *pdwBytesTransferred, bool useCompletionRoutines)
{
    OVERLAPPED *pOverlapped = pIORequest->GetOverlapped();
    Target *pTarget = pIORequest->GetCurrentTarget();
    size_t iTarget = pTarget - &p->vTargets[0];
    UINT32 iRequest = pIORequest->GetRequestIndex();
    LARGE_INTEGER li;
    BOOL rslt = true;

    //
    // Compute next IO
    //

    p->vTargetStates[iTarget].NextIORequest(*pIORequest);

    li.LowPart = pIORequest->GetOverlapped()->Offset;
    li.HighPart = pIORequest->GetOverlapped()->OffsetHigh;

    if (TraceLoggingProviderEnabled(g_hEtwProvider,
                                    TRACE_LEVEL_VERBOSE,
                                    DISKSPD_TRACE_IO))
    {
        GUID ActivityId = p->NextActivityId();
        pIORequest->SetActivityId(ActivityId);

        TraceLoggingWriteActivity(g_hEtwProvider,
                                  "DiskSpd IO",
                                  &ActivityId,
                                  NULL,
                                  TraceLoggingKeyword(DISKSPD_TRACE_IO),
                                  TraceLoggingOpcode(EVENT_TRACE_TYPE_START),
                                  TraceLoggingLevel(TRACE_LEVEL_VERBOSE),
                                  TraceLoggingUInt32(p->ulThreadNo, "Thread"),
                                  TraceLoggingString(pIORequest->GetIoType() == IOOperation::ReadIO ? "Read" : "Write", "IO Type"),
                                  TraceLoggingUInt64(iTarget, "Target"),
                                  TraceLoggingInt32(pTarget->GetBlockSizeInBytes(), "Block Size"),
                                  TraceLoggingInt64(li.QuadPart, "Offset"));
    }

#if 0
    PrintError("t[%u:%u] issuing %u %s @ %I64u)\n", p->ulThreadNo, iTarget,
            pTarget->GetBlockSizeInBytes(),
            (pIORequest->GetIoType() == IOOperation::ReadIO ? "read" : "write"),
            li.QuadPart);
#endif

    if (p->pTimeSpan->GetMeasureLatency())
    {
        pIORequest->SetStartTime(PerfTimer::GetTime());
    }

    if (pIORequest->GetIoType() == IOOperation::ReadIO)
    {
        if (pTarget->GetMemoryMappedIoMode() == MemoryMappedIoMode::On)
        {
            if (pTarget->GetWriteThroughMode() == WriteThroughMode::On )
            {
                g_pfnRtlCopyMemoryNonTemporal(p->GetReadBuffer(iTarget, iRequest), pTarget->GetMappedView() + li.QuadPart, pTarget->GetBlockSizeInBytes());
            }
            else
            {
                memcpy(p->GetReadBuffer(iTarget, iRequest), pTarget->GetMappedView() + li.QuadPart, pTarget->GetBlockSizeInBytes());
            }
            *pdwBytesTransferred = pTarget->GetBlockSizeInBytes();
        }
        else
        {
            if (useCompletionRoutines)
            {
                rslt = ReadFileEx(p->vhTargets[iTarget], p->GetReadBuffer(iTarget, iRequest), pTarget->GetBlockSizeInBytes(), pOverlapped, fileIOCompletionRoutine);
            }
            else
            {
                rslt = ReadFile(p->vhTargets[iTarget], p->GetReadBuffer(iTarget, iRequest), pTarget->GetBlockSizeInBytes(), pdwBytesTransferred, pOverlapped);
            }
        }
    }
    else
    {
        if (pTarget->GetMemoryMappedIoMode() == MemoryMappedIoMode::On)
        {
            if (pTarget->GetWriteThroughMode() == WriteThroughMode::On)
            {
                g_pfnRtlCopyMemoryNonTemporal(pTarget->GetMappedView() + li.QuadPart, p->GetWriteBuffer(iTarget, iRequest), pTarget->GetBlockSizeInBytes());
            }
            else
            {
                memcpy(pTarget->GetMappedView() + li.QuadPart, p->GetWriteBuffer(iTarget, iRequest), pTarget->GetBlockSizeInBytes());

                switch (pTarget->GetMemoryMappedIoFlushMode())
                {
                    case MemoryMappedIoFlushMode::ViewOfFile:
                        FlushViewOfFile(pTarget->GetMappedView() + li.QuadPart, pTarget->GetBlockSizeInBytes());
                        break;
                    case MemoryMappedIoFlushMode::NonVolatileMemory:
                        g_pfnRtlFlushNonVolatileMemory(pTarget->GetMemoryMappedIoNvToken(), pTarget->GetMappedView() + li.QuadPart, pTarget->GetBlockSizeInBytes(), 0);
                        break;
                    case MemoryMappedIoFlushMode::NonVolatileMemoryNoDrain:
                        g_pfnRtlFlushNonVolatileMemory(pTarget->GetMemoryMappedIoNvToken(), pTarget->GetMappedView() + li.QuadPart, pTarget->GetBlockSizeInBytes(), FLUSH_NV_MEMORY_IN_FLAG_NO_DRAIN);
                        break;
                }
            }
            *pdwBytesTransferred = pTarget->GetBlockSizeInBytes();
        }
        else
        {
            if (useCompletionRoutines)
            {
                rslt = WriteFileEx(p->vhTargets[iTarget], p->GetWriteBuffer(iTarget, iRequest), pTarget->GetBlockSizeInBytes(), pOverlapped, fileIOCompletionRoutine);
            }
            else
            {
                rslt = WriteFile(p->vhTargets[iTarget], p->GetWriteBuffer(iTarget, iRequest), pTarget->GetBlockSizeInBytes(), pdwBytesTransferred, pOverlapped);
            }
        }
    }

    if (p->vThroughputMeters.size() != 0 && p->vThroughputMeters[iTarget].IsRunning())
    {
        p->vThroughputMeters[iTarget].Adjust(pTarget->GetBlockSizeInBytes());
    }

    return (rslt) ? true : false;
}

static void completeIO(ThreadParameters *p, IORequest *pIORequest, DWORD dwBytesTransferred)
{
    Target *pTarget = pIORequest->GetCurrentTarget();
    size_t iTarget = pTarget - &p->vTargets[0];

    if (TraceLoggingProviderEnabled(g_hEtwProvider,
                                    TRACE_LEVEL_VERBOSE,
                                    DISKSPD_TRACE_IO))
    {
        GUID ActivityId = pIORequest->GetActivityId();

        TraceLoggingWriteActivity(g_hEtwProvider,
                                  "DiskSpd IO",
                                  &ActivityId,
                                  NULL,
                                  TraceLoggingKeyword(DISKSPD_TRACE_IO),
                                  TraceLoggingOpcode(EVENT_TRACE_TYPE_STOP),
                                  TraceLoggingLevel(TRACE_LEVEL_VERBOSE));
    }

    //check if I/O transferred all of the requested bytes
    if (dwBytesTransferred != pTarget->GetBlockSizeInBytes())
    {
        PrintError("Warning: thread %u transferred %u bytes instead of %u bytes\n",
            p->ulThreadNo,
            dwBytesTransferred,
            pTarget->GetBlockSizeInBytes());
    }

    if (*p->pfAccountingOn)
    {
        p->pResults->vTargetResults[iTarget].Add(dwBytesTransferred,
            pIORequest->GetIoType(),
            pIORequest->GetStartTime(),
            *(p->pullStartTime),
            p->pTimeSpan->GetMeasureLatency(),
            p->pTimeSpan->GetCalculateIopsStdDev());
    }

    // check if we should print a progress dot
    if (p->pProfile->GetProgress() != 0)
    {
        DWORD dwIOCnt = ++p->dwIOCnt;
        if (dwIOCnt % p->pProfile->GetProgress() == 0)
        {
            printf(".");
        }
    }
}

/*****************************************************************************/
// function called from worker thread
// performs synch I/O
//
static bool doWorkUsingSynchronousIO(ThreadParameters *p)
{
    bool fOk = true;
    BOOL rslt = FALSE;
    DWORD dwBytesTransferred;
    size_t cIORequests = p->vIORequest.size();

    while(g_bRun && !g_bThreadError)
    {
        DWORD dwMinSleepTime = ~((DWORD)0);
        for (size_t i = 0; i < cIORequests; i++)
        {
            IORequest *pIORequest = &p->vIORequest[i];
            Target *pTarget = pIORequest->GetNextTarget();

            if (p->vThroughputMeters.size() != 0)
            {
                size_t iTarget = pTarget - &p->vTargets[0];
                ThroughputMeter *pThroughputMeter = &p->vThroughputMeters[iTarget];

                DWORD dwSleepTime = pThroughputMeter->GetSleepTime();
                dwMinSleepTime = min(dwMinSleepTime, dwSleepTime);
                if (pThroughputMeter->IsRunning() && dwSleepTime > 0)
                {
                    continue;
                }
            }

            rslt = issueNextIO(p, pIORequest, &dwBytesTransferred, false);

            if (!rslt)
            {
                PrintError("t[%u] error during %s error code: %u)\n", (UINT32)i, (pIORequest->GetIoType() == IOOperation::ReadIO ? "read" : "write"), GetLastError());
                fOk = false;
                goto cleanup;
            }

            completeIO(p, pIORequest, dwBytesTransferred);
        }

        // if no IOs were issued, wait for the next scheduling time
        if (dwMinSleepTime != ~((DWORD)0) && dwMinSleepTime != 0)
        {
            Sleep(dwMinSleepTime);
        }

        assert(!g_bError);  // at this point we shouldn't be seeing initialization error
    }

cleanup:
    return fOk;
}

/*****************************************************************************/
// function called from worker thread
// performs asynch I/O using IO Completion Ports
//
static bool doWorkUsingIOCompletionPorts(ThreadParameters *p, HANDLE hCompletionPort)
{
    assert(nullptr!= p);
    assert(nullptr != hCompletionPort);

    bool fOk = true;
    BOOL rslt = FALSE;
    OVERLAPPED * pCompletedOvrp;
    ULONG_PTR ulCompletionKey;
    DWORD dwBytesTransferred;
    OverlappedQueue overlappedQueue;
    size_t cIORequests = p->vIORequest.size();

    //start IO operations
    for (size_t i = 0; i < cIORequests; i++)
    {
        overlappedQueue.Add(p->vIORequest[i].GetOverlapped());
    }

    //
    // perform work
    //
    while(g_bRun && !g_bThreadError)
    {
        DWORD dwMinSleepTime = ~((DWORD)0);
        for (size_t i = 0; i < overlappedQueue.GetCount(); i++)
        {
            OVERLAPPED *pReadyOverlapped = overlappedQueue.Remove();
            IORequest *pIORequest = IORequest::OverlappedToIORequest(pReadyOverlapped);
            Target *pTarget = pIORequest->GetNextTarget();

            if (p->vThroughputMeters.size() != 0)
            {
                size_t iTarget = pTarget - &p->vTargets[0];
                ThroughputMeter *pThroughputMeter = &p->vThroughputMeters[iTarget];

                DWORD dwSleepTime = pThroughputMeter->GetSleepTime();
                if (pThroughputMeter->IsRunning() && dwSleepTime > 0)
                {
                    dwMinSleepTime = min(dwMinSleepTime, dwSleepTime);
                    overlappedQueue.Add(pReadyOverlapped);
                    continue;
                }
            }

            rslt = issueNextIO(p, pIORequest, &dwBytesTransferred, false);

            if (!rslt && GetLastError() != ERROR_IO_PENDING)
            {
                UINT32 iIORequest = (UINT32)(pIORequest - &p->vIORequest[0]);
                PrintError("t[%u] error during %s error code: %u)\n", iIORequest, (pIORequest->GetIoType()== IOOperation::ReadIO ? "read" : "write"), GetLastError());
                fOk = false;
                goto cleanup;
            }

            if (rslt && pTarget->GetMemoryMappedIoMode() == MemoryMappedIoMode::On)
            {
                completeIO(p, pIORequest, dwBytesTransferred);
                overlappedQueue.Add(pReadyOverlapped);
            }
        }

        // if no IOs are in flight, wait for the next scheduling time
        if ((overlappedQueue.GetCount() == p->vIORequest.size()) && dwMinSleepTime != ~((DWORD)0))
        {
            Sleep(dwMinSleepTime);
        }

        // wait till one of the IO operations finishes
        if (GetQueuedCompletionStatus(hCompletionPort, &dwBytesTransferred, &ulCompletionKey, &pCompletedOvrp, 1) != 0)
        {
            //find which I/O operation it was (so we know to which buffer should we use)
            IORequest *pIORequest = IORequest::OverlappedToIORequest(pCompletedOvrp);
            completeIO(p, pIORequest, dwBytesTransferred);
            overlappedQueue.Add(pCompletedOvrp);
        }
        else
        {
            DWORD err = GetLastError();
            if (err != WAIT_TIMEOUT)
            {
                PrintError("error during overlapped IO operation (error code: %u)\n", err);
                fOk = false;
                goto cleanup;
            }
        }
    } // end work loop

cleanup:
    return fOk;
}

/*****************************************************************************/
// I/O completion routine. used by ReadFileEx and WriteFileEx
//

VOID CALLBACK fileIOCompletionRoutine(DWORD dwErrorCode, DWORD dwBytesTransferred, LPOVERLAPPED pOverlapped)
{
    assert(NULL != pOverlapped);

    BOOL rslt = FALSE;

    ThreadParameters *p = (ThreadParameters *)pOverlapped->hEvent;

    assert(NULL != p);

    //check error code
    if (0 != dwErrorCode)
    {
        PrintError("Thread %u failed executing an I/O operation (error code: %u)\n", p->ulThreadNo, dwErrorCode);
        goto cleanup;
    }

    IORequest *pIORequest = IORequest::OverlappedToIORequest(pOverlapped);

    completeIO(p, pIORequest, dwBytesTransferred);

    // start a new IO operation
    if (g_bRun && !g_bThreadError)
    {
        Target *pTarget = pIORequest->GetNextTarget();
        size_t iTarget = pTarget - &p->vTargets[0];

        rslt = issueNextIO(p, pIORequest, NULL, true);

        if (!rslt)
        {
            PrintError("t[%u:%u] error during %s error code: %u)\n", p->ulThreadNo, iTarget, (pIORequest->GetIoType() == IOOperation::ReadIO ? "read" : "write"), GetLastError());
            goto cleanup;
        }
    }

cleanup:
    return;
}

/*****************************************************************************/
// function called from worker thread
// performs asynch I/O using IO Completion Routines (ReadFileEx, WriteFileEx)
//
static bool doWorkUsingCompletionRoutines(ThreadParameters *p)
{
    assert(NULL != p);
    bool fOk = true;
    BOOL rslt = FALSE;

    //start IO operations
    UINT32 cIORequests = (UINT32)p->vIORequest.size();

    for (size_t iIORequest = 0; iIORequest < cIORequests; iIORequest++) {
        IORequest *pIORequest = &p->vIORequest[iIORequest];
        Target *pTarget = pIORequest->GetNextTarget();
        size_t iTarget = pTarget - &p->vTargets[0];

        rslt = issueNextIO(p, pIORequest, NULL, true);

        if (!rslt)
        {
            PrintError("t[%u:%u] error during %s error code: %u)\n", p->ulThreadNo, iTarget, (pIORequest->GetIoType() == IOOperation::ReadIO ? "read" : "write"), GetLastError());
            fOk = false;
            goto cleanup;
        }
    }

    DWORD dwWaitResult = 0;
    while( g_bRun && !g_bThreadError )
    {
        dwWaitResult = WaitForSingleObjectEx(p->hEndEvent, INFINITE, TRUE);

        assert(WAIT_IO_COMPLETION == dwWaitResult || (WAIT_OBJECT_0 == dwWaitResult && (!g_bRun || g_bThreadError)));

        //check WaitForSingleObjectEx status
        if( WAIT_IO_COMPLETION != dwWaitResult && WAIT_OBJECT_0 != dwWaitResult )
        {
            PrintError("Error in thread %u during WaitForSingleObjectEx (in completion routines)\n", p->ulThreadNo);
            fOk = false;
            goto cleanup;
        }
    }
cleanup:
    return fOk;
}

struct UniqueTarget {
    string path;
    TargetCacheMode caching;
    PRIORITY_HINT priority;
    DWORD dwDesiredAccess;
    DWORD dwFlags;

    bool operator < (const struct UniqueTarget &ut) const {
        if (path < ut.path) {
            return true;
        }
        else if (ut.path < path) {
            return false;
        }

        if (caching < ut.caching) {
            return true;
        }
        else if (ut.caching < caching) {
            return false;
        }

        if (priority < ut.priority) {
            return true;
        }
        else if (ut.priority < priority) {
            return false;
        }

        if (dwDesiredAccess < ut.dwDesiredAccess) {
            return true;
        }
        else if (ut.dwDesiredAccess < dwDesiredAccess) {
            return false;
        }

        if (dwFlags < ut.dwFlags) {
            return true;
        }

        return false;
    }
};

/*****************************************************************************/
// worker thread function
//
DWORD WINAPI threadFunc(LPVOID cookie)
{
    bool fOk = true;
    bool fAnyMappedIo = false;
    bool fAllMappedIo = true;
    ThreadParameters *p = reinterpret_cast<ThreadParameters *>(cookie);
    HANDLE hCompletionPort = nullptr;

    //
    // A single file can be specified in multiple targets, so only open one
    // handle for each unique file.
    //

    vector<HANDLE> vhUniqueHandles;
    map<UniqueTarget, UINT32> mHandleMap;

    bool fCalculateIopsStdDev = p->pTimeSpan->GetCalculateIopsStdDev();
    UINT64 ioBucketDuration = 0;
    UINT32 expectedNumberOfBuckets = 0;
    if(fCalculateIopsStdDev)
    {
        UINT32 ioBucketDurationInMilliseconds = p->pTimeSpan->GetIoBucketDurationInMilliseconds();
        ioBucketDuration = PerfTimer::MillisecondsToPerfTime(ioBucketDurationInMilliseconds);
        expectedNumberOfBuckets = Util::QuotientCeiling(p->pTimeSpan->GetDuration() * 1000, ioBucketDurationInMilliseconds);
    }

    // apply affinity. The specific assignment is provided in the thread profile up front.
    if (!p->pTimeSpan->GetDisableAffinity())
    {
        GROUP_AFFINITY GroupAffinity;

        PrintVerbose(p->pProfile->GetVerbose(), "affinitizing thread %u to Group %u / CPU %u\n", p->ulThreadNo, p->wGroupNum, p->bProcNum);
        SetProcGroupMask(p->wGroupNum, p->bProcNum, &GroupAffinity);

        HANDLE hThread = GetCurrentThread();
        if (SetThreadGroupAffinity(hThread, &GroupAffinity, nullptr) == FALSE)
        {
            PrintError("Error setting affinity mask in thread %u\n", p->ulThreadNo);
            fOk = false;
            goto cleanup;
        }
    }

    // adjust thread token if large pages are needed
    for (auto pTarget = p->vTargets.begin(); pTarget != p->vTargets.end(); pTarget++)
    {
        if (pTarget->GetUseLargePages())
        {
            if (!SetPrivilege(SE_LOCK_MEMORY_NAME))
            {
                fOk = false;
                goto cleanup;
            }
            break;
        }
    }

    UINT32 cIORequests = p->GetTotalRequestCount();

    size_t iTarget = 0;
    for (auto pTarget = p->vTargets.begin(); pTarget != p->vTargets.end(); pTarget++)
    {
        bool fPhysical = false;
        bool fPartition = false;

        string sPath(pTarget->GetPath());
        const char *filename = sPath.c_str();

        const char *fname = nullptr;    //filename (can point to physFN)
        char physFN[32];                //disk/partition name

        if (NULL == filename || NULL == *(filename))
        {
            PrintError("FATAL ERROR: invalid filename\n");
            fOk = false;
            goto cleanup;
        }

        //check if it is a physical drive
        if ('#' == *filename && NULL != *(filename + 1))
        {
            if (pTarget->GetMemoryMappedIoMode() == MemoryMappedIoMode::On)
            {
                PrintError("Memory mapped I/O is not supported on physical drives\n");
                fOk = false;
                goto cleanup;
            }
            UINT32 nDriveNo = (UINT32)atoi(filename + 1);
            fPhysical = true;
            sprintf_s(physFN, 32, "\\\\.\\PhysicalDrive%u", nDriveNo);
            fname = physFN;
        }

        //check if it is a partition
        if (!fPhysical && NULL != *(filename + 1) && NULL == *(filename + 2) && isalpha((unsigned char)filename[0]) && ':' == filename[1])
        {
            if (pTarget->GetMemoryMappedIoMode() == MemoryMappedIoMode::On)
            {
                PrintError("Memory mapped I/O is not supported on partitions\n");
                fOk = false;
                goto cleanup;
            }
            fPartition = true;

            sprintf_s(physFN, 32, "\\\\.\\%c:", filename[0]);
            fname = physFN;
        }

        //check if it is a regular file
        if (!fPhysical && !fPartition)
        {
            fname = sPath.c_str();
        }

        // get/set file flags
        DWORD dwFlags = pTarget->GetCreateFlags(cIORequests > 1);
        DWORD dwDesiredAccess = 0;
        if (pTarget->GetWriteRatio() == 0)
        {
            dwDesiredAccess = GENERIC_READ;
        }
        else if (pTarget->GetWriteRatio() == 100)
        {
            dwDesiredAccess = GENERIC_WRITE;
        }
        else
        {
            dwDesiredAccess = GENERIC_READ | GENERIC_WRITE;
        }

        if (pTarget->GetMemoryMappedIoMode() == MemoryMappedIoMode::On)
        {
            dwDesiredAccess = GENERIC_READ | GENERIC_WRITE;
            fAnyMappedIo = true;
        }
        else
        {
            fAllMappedIo = false;
        }

        HANDLE hFile;
        UniqueTarget ut;
        ut.path = sPath;
        ut.priority = pTarget->GetIOPriorityHint();
        ut.caching = pTarget->GetCacheMode();
        ut.dwDesiredAccess = dwDesiredAccess;
        ut.dwFlags = dwFlags;

        if (mHandleMap.find(ut) == mHandleMap.end()) {
            hFile = CreateFile(fname,
                dwDesiredAccess,
                FILE_SHARE_READ | FILE_SHARE_WRITE,
                nullptr,        //security
                OPEN_EXISTING,
                dwFlags,        //flags
                nullptr);       //template file
            if (INVALID_HANDLE_VALUE == hFile)
            {
                // TODO: error out
                PrintError("Error opening file: %s [%u]\n", sPath.c_str(), GetLastError());
                fOk = false;
                goto cleanup;
            }

            if (pTarget->GetCacheMode() == TargetCacheMode::DisableLocalCache)
            {
                DWORD Status = DisableLocalCache(hFile);
                if (Status != ERROR_SUCCESS)
                {
                    PrintError("Failed to disable local caching (error %u). NOTE: only supported on remote filesystems with Windows 8 or newer.\n", Status);
                    fOk = false;
                    goto cleanup;
                }
            }

            //set IO priority
            if (pTarget->GetIOPriorityHint() != IoPriorityHintNormal)
            {
                _declspec(align(8)) FILE_IO_PRIORITY_HINT_INFO hintInfo;
                hintInfo.PriorityHint = pTarget->GetIOPriorityHint();
                if (!SetFileInformationByHandle(hFile, FileIoPriorityHintInfo, &hintInfo, sizeof(hintInfo)))
                {
                    PrintError("Error setting IO priority for file: %s [%u]\n", sPath.c_str(), GetLastError());
                    fOk = false;
                    goto cleanup;
                }
            }

            mHandleMap[ut] = (UINT32)vhUniqueHandles.size();
            vhUniqueHandles.push_back(hFile);
        }
        else {
            hFile = vhUniqueHandles[mHandleMap[ut]];
        }

        p->vhTargets.push_back(hFile);

        // obtain file/disk/partition size
        {
            UINT64 fsize = 0;   //file size

            //check if it is a disk
            if (fPhysical)
            {
                fsize = GetPhysicalDriveSize(hFile);
            }
            // check if it is a partition
            else if (fPartition)
            {
                fsize = GetPartitionSize(hFile);
            }
            // it has to be a regular file
            else
            {
                ULARGE_INTEGER ulsize;

                ulsize.LowPart = GetFileSize(hFile, &ulsize.HighPart);
                if (INVALID_FILE_SIZE == ulsize.LowPart && GetLastError() != NO_ERROR)
                {
                    PrintError("Error getting file size\n");
                    fOk = false;
                    goto cleanup;
                }
                else
                {
                    fsize = ulsize.QuadPart;
                }
            }

            // check if file size is valid (if it's == 0, it won't be useful)
            if (0 == fsize)
            {
                // TODO: error out
                PrintError("ERROR: target size could not be determined\n");
                fOk = false;
                goto cleanup;
            }

            if (fsize < pTarget->GetMaxFileSize())
            {
                PrintError("WARNING: file size %I64u is less than MaxFileSize %I64u\n", fsize, pTarget->GetMaxFileSize());
            }

            //
            // Build target state.
            //

            p->vTargetStates.emplace_back(
                p,
                iTarget,
                fsize);

            //
            // Ensure this thread can start given stride/size of target.
            //

            if (!p->vTargetStates[iTarget].CanStart())
            {
                PrintError("The file is too small. File: '%s' relative thread %u: file size: %I64u, base offset: %I64u, thread stride: %I64u, block size: %u\n",
                    pTarget->GetPath().c_str(),
                    p->ulRelativeThreadNo,
                    fsize,
                    pTarget->GetBaseFileOffsetInBytes(),
                    pTarget->GetThreadStrideInBytes(),
                    pTarget->GetBlockSizeInBytes());
                fOk = false;
                goto cleanup;
            }

            PrintVerbose(p->pProfile->GetVerbose(), "thread %u starting: file '%s' relative thread %u",
                p->ulThreadNo,
                pTarget->GetPath().c_str(),
                p->ulRelativeThreadNo);

            if (pTarget->GetRandomRatio() > 0)
            {
                PrintVerbose(p->pProfile->GetVerbose(), ", %u% random pattern\n",
                    pTarget->GetRandomRatio());
            }
            else
            {
                PrintVerbose(p->pProfile->GetVerbose(), ", %ssequential file offset\n", pTarget->GetUseInterlockedSequential() ? "interlocked ":"");
            }
        }

        // allocate memory for a data buffer
        if (!p->AllocateAndFillBufferForTarget(*pTarget))
        {
            PrintError("ERROR: Could not allocate a buffer for target '%s'. Error code: 0x%x\n", pTarget->GetPath().c_str(), GetLastError());
            fOk = false;
            goto cleanup;
        }

        // initialize memory mapped views of files
        if (pTarget->GetMemoryMappedIoMode() == MemoryMappedIoMode::On)
        {
            NTSTATUS status;
            PVOID nvToken;

            pTarget->SetMappedViewFileHandle(hFile);
            if (!p->InitializeMappedViewForTarget(*pTarget, dwDesiredAccess))
            {
                PrintError("ERROR: Could not map view for target '%s'. Error code: 0x%x\n", pTarget->GetPath().c_str(), GetLastError());
                fOk = false;
                goto cleanup;
            }

            if (pTarget->GetWriteThroughMode() == WriteThroughMode::On && nullptr == g_pfnRtlCopyMemoryNonTemporal)
            {
                PrintError("ERROR: Windows runtime environment does not support the non-temporal memory copy API for target '%s'.\n", pTarget->GetPath().c_str());
                fOk = false;
                goto cleanup;
            }

            if ((pTarget->GetMemoryMappedIoFlushMode() == MemoryMappedIoFlushMode::NonVolatileMemory) || (pTarget->GetMemoryMappedIoFlushMode() == MemoryMappedIoFlushMode::NonVolatileMemoryNoDrain))
            {
                // RtlGetNonVolatileToken() works only on DAX enabled PMEM devices.
                if (g_pfnRtlGetNonVolatileToken != nullptr && g_pfnRtlFreeNonVolatileToken != nullptr)
                {
                    status = g_pfnRtlGetNonVolatileToken(pTarget->GetMappedView(), (SIZE_T) pTarget->GetFileSize(), &nvToken);
                    if (!NT_SUCCESS(status))
                    {
                        PrintError("ERROR: Could not get non-volatile token for target '%s'. Error code: 0x%x\n", pTarget->GetPath().c_str(), GetLastError());
                        fOk = false;
                        goto cleanup;
                    }
                    pTarget->SetMemoryMappedIoNvToken(nvToken);
                }
                else
                {
                    PrintError("ERROR: Windows runtime environment does not support the non-volatile memory flushing APIs for target '%s'.\n", pTarget->GetPath().c_str());
                    fOk = false;
                    goto cleanup;
                }
            }
        }

        iTarget++;
    }

    // TODO: copy parameters for better memory locality?
    // TODO: tell the main thread we're ready

    PrintVerbose(p->pProfile->GetVerbose(), "thread %u started (random seed: %u)\n", p->ulThreadNo, p->ulRandSeed);

    p->pResults->vTargetResults.clear();
    p->pResults->vTargetResults.resize(p->vTargets.size());

    for (size_t i = 0; i < p->vTargets.size(); i++)
    {
        p->pResults->vTargetResults[i].sPath = p->vTargets[i].GetPath();
        p->pResults->vTargetResults[i].ullFileSize = p->vTargetStates[i].TargetSize();

        if(fCalculateIopsStdDev)
        {
            p->pResults->vTargetResults[i].readBucketizer.Initialize(ioBucketDuration, expectedNumberOfBuckets);
            p->pResults->vTargetResults[i].writeBucketizer.Initialize(ioBucketDuration, expectedNumberOfBuckets);
        }

        //
        // Copy effective distribution range to results for reporting (may be empty)
        //

        p->pResults->vTargetResults[i].vDistributionRange = p->vTargetStates[i]._vDistributionRange;
    }

    //
    // fill the IORequest structures
    //

    p->vIORequest.clear();

    if (p->pTimeSpan->GetThreadCount() != 0 &&
        p->pTimeSpan->GetRequestCount() != 0)
    {
        p->vIORequest.resize(cIORequests, IORequest(p->pRand));

        for (UINT32 iIORequest = 0; iIORequest < cIORequests; iIORequest++)
        {
            p->vIORequest[iIORequest].SetRequestIndex(iIORequest);

            for (unsigned int iFile = 0; iFile < p->vTargets.size(); iFile++)
            {
                Target *pTarget = &p->vTargets[iFile];
                const vector<ThreadTarget> vThreadTargets = pTarget->GetThreadTargets();
                UINT32 ulWeight = pTarget->GetWeight();

                for (UINT32 iThreadTarget = 0; iThreadTarget < vThreadTargets.size(); iThreadTarget++)
                {
                    if (vThreadTargets[iThreadTarget].GetThread() == p->ulRelativeThreadNo)
                    {
                        if (vThreadTargets[iThreadTarget].GetWeight() != 0)
                        {
                            ulWeight = vThreadTargets[iThreadTarget].GetWeight();
                        }
                        break;
                    }
                }

                //
                // Parallel async is not supported with -O for exactly this reason,
                // and is validated in the profile before reaching here. Document this
                // with the assert in comparison to the code in the non-O case below.
                // Parallel depends on the IORequest being for a single file only (the
                // seq offset is in the IORequest itself).
                //

                assert(pTarget->GetUseParallelAsyncIO() == false);

                p->vIORequest[iIORequest].AddTarget(pTarget, ulWeight);
            }
        }
    }
    else
    {
        for (unsigned int iFile = 0; iFile < p->vTargets.size(); iFile++)
        {
            Target *pTarget = &p->vTargets[iFile];

            for (DWORD iRequest = 0; iRequest < pTarget->GetRequestCount(); ++iRequest)
            {
                IORequest ioRequest(p->pRand);
                ioRequest.AddTarget(pTarget, 1);
                ioRequest.SetRequestIndex(iRequest);
                if (pTarget->GetUseParallelAsyncIO())
                {
                    p->vTargetStates[iFile].InitializeParallelAsyncIORequest(ioRequest);
                }

                p->vIORequest.push_back(ioRequest);
            }
        }
    }

    //
    // fill the throughput meter structures
    //
    size_t cTargets = p->vTargets.size();
    bool fUseThrougputMeter = false;
    for (size_t i = 0; i < cTargets; i++)
    {
        ThroughputMeter throughputMeter;
        Target *pTarget = &p->vTargets[i];
        DWORD dwBurstSize = pTarget->GetBurstSize();
        if (p->pTimeSpan->GetThreadCount() > 0)
        {
            if (pTarget->GetThreadTargets().size() == 0)
            {
                dwBurstSize /= p->pTimeSpan->GetThreadCount();
            }
            else
            {
                dwBurstSize /= (DWORD)pTarget->GetThreadTargets().size();
            }
        }
        else
        {
            dwBurstSize /= pTarget->GetThreadsPerFile();
        }

        if (pTarget->GetThroughputInBytesPerMillisecond() > 0 || pTarget->GetThinkTime() > 0)
        {
            fUseThrougputMeter = true;
            throughputMeter.Start(pTarget->GetThroughputInBytesPerMillisecond(), pTarget->GetBlockSizeInBytes(), pTarget->GetThinkTime(), dwBurstSize);
        }

        p->vThroughputMeters.push_back(throughputMeter);
    }

    if (!fUseThrougputMeter)
    {
        p->vThroughputMeters.clear();
    }

    //FUTURE EXTENSION: enable asynchronous I/O even if only 1 outstanding I/O per file (requires another parameter)
    if (cIORequests == 1 || fAllMappedIo)
    {
        //synchronous IO - no setup needed
    }
    else if (p->pTimeSpan->GetCompletionRoutines() && !fAnyMappedIo)
    {
        //in case of completion routines hEvent field is not used,
        //so we can use it to pass a pointer to the thread parameters
        for (UINT32 iIORequest = 0; iIORequest < cIORequests; iIORequest++) {
            OVERLAPPED *pOverlapped;

            pOverlapped = p->vIORequest[iIORequest].GetOverlapped();
            pOverlapped->hEvent = (HANDLE)p;
        }
    }
    else
    {
        //
        // create IO completion port if not doing completion routines or synchronous IO
        //
        for (unsigned int i = 0; i < vhUniqueHandles.size(); i++)
        {
            hCompletionPort = CreateIoCompletionPort(vhUniqueHandles[i], hCompletionPort, 0, 1);
            if (nullptr == hCompletionPort)
            {
                PrintError("unable to create IO completion port (error code: %u)\n", GetLastError());
                fOk = false;
                goto cleanup;
            }
        }
    }

    //
    // wait for a signal to start
    //
    PrintVerbose(p->pProfile->GetVerbose(), "thread %u: waiting for a signal to start\n", p->ulThreadNo);
    if( WAIT_FAILED == WaitForSingleObject(p->hStartEvent, INFINITE) )
    {
        PrintError("Waiting for a signal to start failed (error code: %u)\n", GetLastError());
        fOk = false;
        goto cleanup;
    }
    PrintVerbose(p->pProfile->GetVerbose(), "thread %u: received signal to start\n", p->ulThreadNo);

    //check if everything is ok
    if (g_bError)
    {
        fOk = false;
        goto cleanup;
    }

    //error handling and memory freeing is done in doWorkUsingIOCompletionPorts and doWorkUsingCompletionRoutines
    if (cIORequests == 1 || fAllMappedIo)
    {
        // use synchronous IO (it will also clse the event)
        if (!doWorkUsingSynchronousIO(p))
        {
            fOk = false;
            goto cleanup;
        }
    }
    else if (!p->pTimeSpan->GetCompletionRoutines() || fAnyMappedIo)
    {
        // use IO Completion Ports (it will also close the I/O completion port)
        if (!doWorkUsingIOCompletionPorts(p, hCompletionPort))
        {
            fOk = false;
            goto cleanup;
        }
    }
    else
    {
        //use completion routines
        if (!doWorkUsingCompletionRoutines(p))
        {
            fOk = false;
            goto cleanup;
        }
    }

    assert(!g_bError);  // at this point we shouldn't be seeing initialization error

    // save results

cleanup:
    if (!fOk)
    {
        g_bThreadError = TRUE;
    }

    // free memory allocated with VirtualAlloc
    for (auto i = p->vpDataBuffers.begin(); i != p->vpDataBuffers.end(); i++)
    {
        if (nullptr != *i)
        {
#pragma prefast(suppress:6001, "Prefast does not understand this vector will only contain validly allocated buffer pointers")
            VirtualFree(*i, 0, MEM_RELEASE);
        }
    }

    // free NV tokens
    for (auto i = p->vTargets.begin(); i != p->vTargets.end(); i++)
    {
        if (i->GetMemoryMappedIoNvToken() != nullptr && g_pfnRtlFreeNonVolatileToken != nullptr)
        {
            g_pfnRtlFreeNonVolatileToken(i->GetMemoryMappedIoNvToken());
            i->SetMemoryMappedIoNvToken(nullptr);
        }
    }

    // close files
    for (auto i = vhUniqueHandles.begin(); i != vhUniqueHandles.end(); i++)
    {
        CloseHandle(*i);
    }

    // close completion ports
    if (hCompletionPort != nullptr)
    {
        CloseHandle(hCompletionPort);
    }

    delete p->pRand;
    delete p;

    // notify master thread that we've finished
    InterlockedDecrement(&g_lRunningThreadsCount);

    return fOk ? 1 : 0;
}

/*****************************************************************************/
struct ETWSessionInfo IORequestGenerator::_GetResultETWSession(const EVENT_TRACE_PROPERTIES *pTraceProperties) const
{
    struct ETWSessionInfo session = {};
    if (nullptr != pTraceProperties)
    {
        session.lAgeLimit = pTraceProperties->AgeLimit;
        session.ulBufferSize = pTraceProperties->BufferSize;
        session.ulBuffersWritten = pTraceProperties->BuffersWritten;
        session.ulEventsLost = pTraceProperties->EventsLost;
        session.ulFlushTimer = pTraceProperties->FlushTimer;
        session.ulFreeBuffers = pTraceProperties->FreeBuffers;
        session.ulLogBuffersLost = pTraceProperties->LogBuffersLost;
        session.ulMaximumBuffers = pTraceProperties->MaximumBuffers;
        session.ulMinimumBuffers = pTraceProperties->MinimumBuffers;
        session.ulNumberOfBuffers = pTraceProperties->NumberOfBuffers;
        session.ulRealTimeBuffersLost = pTraceProperties->RealTimeBuffersLost;
    }
    return session;
}

DWORD IORequestGenerator::_CreateDirectoryPath(const char *pszPath) const
{
    char *c = nullptr;          //variable used to browse the path
    char dirPath[MAX_PATH];  //copy of the path (it will be altered)

    //only support absolute paths that specify the drive letter
    if (pszPath[0] == '\0' || pszPath[1] != ':')
    {
        return ERROR_NOT_SUPPORTED;
    }

    if (strcpy_s(dirPath, _countof(dirPath), pszPath) != 0)
    {
        return ERROR_BUFFER_OVERFLOW;
    }

    c = dirPath;
    while('\0' != *c)
    {
        if ('\\' == *c)
        {
            //skip the first one as it will be the drive name
            if (c-dirPath >= 3)
            {
                *c = '\0';
                //create directory if it doesn't exist
                if (GetFileAttributes(dirPath) == INVALID_FILE_ATTRIBUTES)
                {
                    if (CreateDirectory(dirPath, NULL) == FALSE)
                    {
                        return GetLastError();
                    }
                }
                *c = L'\\';
            }
        }

        c++;
    }

    return ERROR_SUCCESS;
}

/*****************************************************************************/
// create a file of the given size
//
bool IORequestGenerator::_CreateFile(UINT64 ullFileSize, const char *pszFilename, bool fZeroBuffers, bool fVerbose) const
{
    bool fSlowWrites = false;
    PrintVerbose(fVerbose, "Creating file '%s' of size %I64u.\n", pszFilename, ullFileSize);

    //enable SE_MANAGE_VOLUME_NAME privilege, required to set valid size of a file
    if (!SetPrivilege(SE_MANAGE_VOLUME_NAME, "WARNING:"))
    {
        PrintError("WARNING: Could not set privileges for setting valid file size; will use a slower method of preparing the file\n", GetLastError());
        fSlowWrites = true;
    }

    // there are various forms of paths we do not support creating subdir hierarchies
    // for - relative and unc paths specifically. this is fine, and not neccesary to
    // warn about. we can add support in the future.
    DWORD dwError = _CreateDirectoryPath(pszFilename);
    if (dwError != ERROR_SUCCESS && dwError != ERROR_NOT_SUPPORTED)
    {
        PrintError("WARNING: Could not create intermediate directory (error code: %u)\n", dwError);
    }

    // create handle to the file
    HANDLE hFile = CreateFile(pszFilename,
                              GENERIC_READ | GENERIC_WRITE,
                              FILE_SHARE_READ | FILE_SHARE_WRITE,
                              nullptr,
                              CREATE_ALWAYS,
                              FILE_ATTRIBUTE_NORMAL,
                              nullptr);
    if (INVALID_HANDLE_VALUE == hFile)
    {
        PrintError("Could not create the file (error code: %u)\n", GetLastError());
        return false;
    }

    if (ullFileSize > 0)
    {
        LARGE_INTEGER li;
        li.QuadPart = ullFileSize;

        LARGE_INTEGER liNewFilePointer;

        if (!SetFilePointerEx(hFile, li, &liNewFilePointer, FILE_BEGIN))
        {
            PrintError("Could not set file pointer during file creation when extending file (error code: %u)\n", GetLastError());
            CloseHandle(hFile);
            return false;
        }
        if (liNewFilePointer.QuadPart != li.QuadPart)
        {
            PrintError("File pointer improperly moved during file creation when extending file\n");
            CloseHandle(hFile);
            return false;
        }

        //extends file (warning! this is a kind of "reservation" of space; valid size of the file is still 0!)
        if (!SetEndOfFile(hFile))
        {
            PrintError("Error setting end of file (error code: %u)\n", GetLastError());
            CloseHandle(hFile);
            return false;
        }
        //try setting valid size of the file (privileges for that are enabled before CreateFile)
        if (!fSlowWrites && !SetFileValidData(hFile, ullFileSize))
        {
            PrintError("WARNING: Could not set valid file size (error code: %u); trying a slower method of filling the file"
                       " (this does not affect performance, just makes the test preparation longer)\n",
                       GetLastError());
            fSlowWrites = true;
        }

        //if setting valid size couldn't be performed, fill in the file by simply writing to it (slower)
        if (fSlowWrites)
        {
            li.QuadPart = 0;
            if (!SetFilePointerEx(hFile, li, &liNewFilePointer, FILE_BEGIN))
            {
                PrintError("Could not set file pointer during file creation (error code: %u)\n", GetLastError());
                CloseHandle(hFile);
                return false;
            }
            if (liNewFilePointer.QuadPart != li.QuadPart)
            {
                PrintError("File pointer improperly moved during file creation\n");
                CloseHandle(hFile);
                return false;
            }

            UINT32 ulBufSize;
            UINT64 ullRemainSize;

            ulBufSize = 1024*1024;
            if (ullFileSize < (UINT64)ulBufSize)
            {
                ulBufSize = (UINT32)ullFileSize;
            }

            vector<BYTE> vBuf(ulBufSize);
            for (UINT32 i=0; i<ulBufSize; ++i)
            {
                vBuf[i] = fZeroBuffers ? 0 : (BYTE)(i&0xFF);
            }

            ullRemainSize = ullFileSize;
            while (ullRemainSize > 0)
            {
                DWORD dwBytesWritten;
                if ((UINT64)ulBufSize > ullRemainSize)
                {
                    ulBufSize = (UINT32)ullRemainSize;
                }

                if (!WriteFile(hFile, &vBuf[0], ulBufSize, &dwBytesWritten, NULL))
                {
                    PrintError("Error while writng during file creation (error code: %u)\n", GetLastError());
                    CloseHandle(hFile);
                    return false;
                }

                if (dwBytesWritten != ulBufSize)
                {
                    PrintError("Improperly written data during file creation\n");
                    CloseHandle(hFile);
                    return false;
                }

                ullRemainSize -= ulBufSize;
            }
        }
    }

    //if compiled with debug support, check file size
#ifndef NDEBUG
    LARGE_INTEGER li;
    if( GetFileSizeEx(hFile, &li) )
    {
        assert(li.QuadPart == (LONGLONG)ullFileSize);
    }
#endif

    CloseHandle(hFile);

    return true;
}

/*****************************************************************************/
void IORequestGenerator::_TerminateWorkerThreads(vector<HANDLE>& vhThreads) const
{
    for (UINT32 x = 0; x < vhThreads.size(); ++x)
    {
        assert(NULL != vhThreads[x]);
#pragma warning( push )
#pragma warning( disable : 6258 )
        if (!TerminateThread(vhThreads[x], 0))
        {
            PrintError("Warning: unable to terminate worker thread %u\n", x);
        }
#pragma warning( pop )
    }
}
/*****************************************************************************/
void IORequestGenerator::_AbortWorkerThreads(HANDLE hStartEvent, vector<HANDLE>& vhThreads) const
{
    assert(NULL != hStartEvent);

    if (NULL == hStartEvent)
    {
        return;
    }

    g_bError = TRUE;
    if (!SetEvent(hStartEvent))
    {
        PrintError("Error signaling start event\n");
        _TerminateWorkerThreads(vhThreads);
    }
    else
    {
        //FUTURE EXTENSION: maximal timeout may be added here (and below)
        while (g_lRunningThreadsCount > 0)
        {
            Sleep(100);
        }
    }
}

/*****************************************************************************/
bool IORequestGenerator::_StopETW(bool fUseETW, TRACEHANDLE hTraceSession) const
{
    bool fOk = true;
    if (fUseETW)
    {
        PEVENT_TRACE_PROPERTIES pETWSession = StopETWSession(hTraceSession);
        if (nullptr == pETWSession)
        {
            PrintError("Error stopping ETW session\n");
            fOk = false;
        }
        else
        {
            free(pETWSession);
        }
    }
    return fOk;
}

/*****************************************************************************/
// initializes all global parameters
//
void IORequestGenerator::_InitializeGlobalParameters()
{
    g_lRunningThreadsCount = 0;     //number of currently running worker threads
    g_bRun = TRUE;                  //used for letting threads know that they should stop working

    g_bThreadError = FALSE;         //true means that an error has occured in one of the threads
    g_bTracing = FALSE;             //true means that ETW is turned on

    _hNTDLL = nullptr;              //handle to ntdll.dll
    g_bError = FALSE;               //true means there was fatal error during intialization and threads shouldn't perform their work
}

bool IORequestGenerator::_PrecreateFiles(Profile& profile) const
{
    bool fOk = true;

    if (profile.GetPrecreateFiles() != PrecreateFiles::None)
    {
        vector<CreateFileParameters> vFilesToCreate = _GetFilesToPrecreate(profile);
        vector<string> vCreatedFiles;
        for (auto file : vFilesToCreate)
        {
            fOk = _CreateFile(file.ullFileSize, file.sPath.c_str(), file.fZeroWriteBuffers, profile.GetVerbose());
            if (!fOk)
            {
                break;
            }
            vCreatedFiles.push_back(file.sPath);
        }

        if (fOk)
        {
            profile.MarkFilesAsPrecreated(vCreatedFiles);
        }
    }

    return fOk;
}

bool IORequestGenerator::GenerateRequests(Profile& profile, IResultParser& resultParser, struct Synchronization *pSynch)
{
    bool fOk = _PrecreateFiles(profile);
    if (fOk)
    {
        const vector<TimeSpan>& vTimeSpans = profile.GetTimeSpans();
        vector<Results> vResults(vTimeSpans.size());
        for (size_t i = 0; fOk && (i < vTimeSpans.size()); i++)
        {
            PrintVerbose(profile.GetVerbose(), "Generating requests for timespan %u.\n", i + 1);
            fOk = _GenerateRequestsForTimeSpan(profile, vTimeSpans[i], vResults[i], pSynch);
        }

        // TODO: show results only for timespans that succeeded
        SystemInformation system;
        EtwResultParser::ParseResults(vResults);
        string sResults = resultParser.ParseResults(profile, system, vResults);
        printf("%s", sResults.c_str());
        fflush(stdout);
    }

    return fOk;
}

bool IORequestGenerator::_GenerateRequestsForTimeSpan(const Profile& profile, const TimeSpan& timeSpan, Results& results, struct Synchronization *pSynch)
{
    //FUTURE EXTENSION: add new I/O capabilities presented in Longhorn
    //FUTURE EXTENSION: add a check if the folder is compressed (cache is always enabled in case of compressed folders)

    //check if I/O request generator is already running
    LONG lGenState = InterlockedExchange(&g_lGeneratorRunning, 1);
    if (1 == lGenState)
    {
        PrintError("FATAL ERROR: I/O Request Generator already running\n");
        return false;
    }

    //initialize all global parameters (in case of second run, after the first one is finished)
    _InitializeGlobalParameters();

    HANDLE hStartEvent = nullptr;                       // start event (used to inform the worker threads that they should start the work)
    HANDLE hEndEvent = nullptr;                         // end event (used only in case of completin routines (not for IO Completion Ports))

    memset(&g_EtwEventCounters, 0, sizeof(struct ETWEventCounters));  // reset all etw event counters

    bool fUseETW = profile.GetEtwEnabled();            //true if user wants ETW

    //
    // load dlls
    //
    assert(nullptr == _hNTDLL);
    if (!_LoadDLLs())
    {
        PrintError("Error loading NtQuerySystemInformation\n");
        return false;
    }

    //FUTURE EXTENSION: check for conflicts in alignment (when cache is turned off only sector aligned I/O are permitted)
    //FUTURE EXTENSION: check if file sizes are enough to have at least first requests not wrapping around

    Random r;
    vector<Target> vTargets = timeSpan.GetTargets();
    // allocate memory for random data write buffers
    for (auto i = vTargets.begin(); i != vTargets.end(); i++)
    {
        if ((i->GetRandomDataWriteBufferSize() > 0) && !i->AllocateAndFillRandomDataWriteBuffer(&r))
        {
            return false;
        }
    }

    // check if user wanted to create a file
    for (auto i = vTargets.begin(); i != vTargets.end(); i++)
    {
        if ((i->GetFileSize() > 0) && (i->GetPrecreated() == false))
        {
            string str = i->GetPath();
            if (str.empty())
            {
                PrintError("You have to provide a filename\n");
                return false;
            }

            //skip physical drives and partitions
            if ('#' == str[0] || (':' == str[1] && '\0' == str[2]))
            {
                continue;
            }

            //create only regular files
            if (!_CreateFile(i->GetFileSize(), str.c_str(), i->GetZeroWriteBuffers(), profile.GetVerbose()))
            {
                return false;
            }
        }
    }

    // get thread count
    UINT32 cThreads = timeSpan.GetThreadCount();
    if (cThreads < 1)
    {
        for (auto i = vTargets.begin(); i != vTargets.end(); i++)
        {
            cThreads += i->GetThreadsPerFile();
        }
    }

    // allocate memory for thread handles
    vector<HANDLE> vhThreads(cThreads);

    //
    // allocate memory for performance counters
    //
    vector<SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION> vPerfInit(g_SystemInformation.processorTopology._ulProcCount);
    vector<SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION> vPerfDone(g_SystemInformation.processorTopology._ulProcCount);
    vector<SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION> vPerfDiff(g_SystemInformation.processorTopology._ulProcCount);

    //
    //create start event
    //

    hStartEvent = CreateEvent(NULL, TRUE, FALSE, "");
    if (NULL == hStartEvent)
    {
        PrintError("Error creating the start event\n");
        return false;
    }

    //
    // create end event
    //
    if (timeSpan.GetCompletionRoutines())
    {
        hEndEvent = CreateEvent(NULL, TRUE, FALSE, "");
        if (NULL == hEndEvent)
        {
            PrintError("Error creating the end event\n");
            return false;
        }
    }

    //
    // set to high priority to ensure the controller thread gets to run immediately
    // when signalled.
    //

    SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_HIGHEST);

    //
    // create the threads
    //

    g_bRun = TRUE;

    // gather affinity information, and move to the first active processor
    const auto& vAffinity = timeSpan.GetAffinityAssignments();
    WORD wGroupCtr = 0;
    BYTE bProcCtr = 0;
    g_SystemInformation.processorTopology.GetActiveGroupProcessor(wGroupCtr, bProcCtr, false);

    volatile bool fAccountingOn = false;
    UINT64 ullStartTime;    //start time
    UINT64 ullTimeDiff;  //elapsed test time (in units returned by QueryPerformanceCounter)
    vector<UINT64> vullSharedSequentialOffsets(vTargets.size(), 0);

    results.vThreadResults.clear();
    results.vThreadResults.resize(cThreads);
    for (UINT32 iThread = 0; iThread < cThreads; ++iThread)
    {
        PrintVerbose(profile.GetVerbose(), "creating thread %u\n", iThread);
        ThreadParameters *cookie = new ThreadParameters();  // threadFunc is going to free the memory
        if (nullptr == cookie)
        {
            PrintError("FATAL ERROR: could not allocate memory\n");
            _AbortWorkerThreads(hStartEvent, vhThreads);
            return false;
        }

        // each thread has a different random seed
        Random *pRand = new Random(timeSpan.GetRandSeed() + iThread);
        if (nullptr == pRand)
        {
            PrintError("FATAL ERROR: could not allocate memory\n");
            _AbortWorkerThreads(hStartEvent, vhThreads);
            delete cookie;
            return false;
        }

        UINT32 ulRelativeThreadNo = 0;

        if (timeSpan.GetThreadCount() > 0)
        {
            // fixed thread mode: threads operate on specified files
            // and receive the entire seq index array.
            // relative thread number is the same as thread number.
            cookie->pullSharedSequentialOffsets = &vullSharedSequentialOffsets[0];
            ulRelativeThreadNo = iThread;
            for (auto i = vTargets.begin();
                 i != vTargets.end();
                 i++)
            {
                const vector<ThreadTarget> vThreadTargets = i->GetThreadTargets();

                // no thread targets specified - add to all threads
                if (vThreadTargets.size() == 0)
                {
                    cookie->vTargets.push_back(*i);
                }
                else
                {
                    // check if the target should be added to the current thread
                    for (UINT32 iThreadTarget = 0; iThreadTarget < vThreadTargets.size(); iThreadTarget++)
                    {
                        if (vThreadTargets[iThreadTarget].GetThread() == iThread)
                        {
                            // confirm copy constructor?
                            cookie->vTargets.push_back(*i);
                            break;
                        }
                    }
                }
            }
        }
        else
        {
            size_t cAssignedThreads = 0;
            size_t cBaseThread = 0;
            auto psi = vullSharedSequentialOffsets.begin();
            for (auto i = vTargets.begin();
                 i != vTargets.end();
                 i++, psi++)
            {
                // per-file thread mode: groups of threads operate on individual files
                // and receive the specific seq index for their file (note: singular).
                // loop up through the targets to assign thread n to the appropriate file.
                // relative thread number is file-relative, so keep track of the base
                // thread number for the file and calculate relative to that.
                //
                // ex: two files, two threads per file
                //  t0: rt0 for f0 (cAssigned = 2, cBase = 0)
                //  t1: rt1 for f0 (cAssigned = 2, cBase = 0)
                //  t2: rt0 for f1 (cAssigned = 4, cBase = 2)
                //  t3: rt1 for f1 (cAssigned = 4, cBase = 2)

                cAssignedThreads += i->GetThreadsPerFile();
                if (iThread < cAssignedThreads)
                {
                    // confirm copy constructor?
                    cookie->vTargets.push_back(*i);
                    cookie->pullSharedSequentialOffsets = &(*psi);
                    ulRelativeThreadNo = (iThread - cBaseThread) % i->GetThreadsPerFile();

                    PrintVerbose(profile.GetVerbose(), "thread %u is relative thread %u for %s\n", iThread, ulRelativeThreadNo, i->GetPath().c_str());
                    break;
                }
                cBaseThread += i->GetThreadsPerFile();
            }
        }

        cookie->pProfile = &profile;
        cookie->pTimeSpan = &timeSpan;
        cookie->hStartEvent = hStartEvent;
        cookie->hEndEvent = hEndEvent;
        cookie->ulThreadNo = iThread;
        cookie->ulRelativeThreadNo = ulRelativeThreadNo;
        cookie->pfAccountingOn = &fAccountingOn;
        cookie->pullStartTime = &ullStartTime;
        cookie->ulRandSeed = timeSpan.GetRandSeed() + iThread;  // each thread has a different random seed
        cookie->pRand = pRand;

        //Set thread group and proc affinity

        // Default: Round robin cores in order of groups, starting at group 0.
        //          Fill each group before moving to next.
        if (vAffinity.size() == 0)
        {
            cookie->wGroupNum = wGroupCtr;
            cookie->bProcNum = bProcCtr;

            // advance to next active
            g_SystemInformation.processorTopology.GetActiveGroupProcessor(wGroupCtr, bProcCtr, true);
        }
        // Assigned affinity. Round robin through the assignment list.
        else
        {
            ULONG i = iThread % vAffinity.size();

            cookie->wGroupNum = vAffinity[i].wGroup;
            cookie->bProcNum = vAffinity[i].bProc;
        }

        //create thread
        cookie->pResults = &results.vThreadResults[iThread];

        InterlockedIncrement(&g_lRunningThreadsCount);
        DWORD dwThreadId;
        HANDLE hThread = CreateThread(NULL, 64 * 1024, threadFunc, cookie, 0, &dwThreadId);
        if (NULL == hThread)
        {
            //in case of error terminate running worker threads
            PrintError("ERROR: unable to create thread (error code: %u)\n", GetLastError());
            InterlockedDecrement(&g_lRunningThreadsCount);
            _AbortWorkerThreads(hStartEvent, vhThreads);
            delete pRand;
            delete cookie;
            return false;
        }

        //store handle to the thread
        vhThreads[iThread] = hThread;
    }

    if (STRUCT_SYNCHRONIZATION_SUPPORTS(pSynch, hStartEvent) && (NULL != pSynch->hStartEvent))
    {
        if (WAIT_OBJECT_0 != WaitForSingleObject(pSynch->hStartEvent, INFINITE))
        {
            PrintError("Error during WaitForSingleObject\n");
            _AbortWorkerThreads(hStartEvent, vhThreads);
            return false;
        }
    }

    //
    // get cycle count (it will be used to calculate actual work time)
    //
    DWORD dwWaitStatus = 0;

    //bAccountingOn = FALSE; // clear the accouning flag so that threads didn't count what they do while in the warmup phase

    BOOL bSynchStop = STRUCT_SYNCHRONIZATION_SUPPORTS(pSynch, hStopEvent) && (NULL != pSynch->hStopEvent);
    BOOL bBreak = FALSE;
    PEVENT_TRACE_PROPERTIES pETWSession = NULL;

    PrintVerbose(profile.GetVerbose(), "starting warm up...\n");
    //
    // send start signal
    //
    if (!SetEvent(hStartEvent))
    {
        PrintError("Error signaling start event\n");
        //        stopETW(bUseETW, hTraceSession);
        _TerminateWorkerThreads(vhThreads);    //FUTURE EXTENSION: timeout for worker threads
        return false;
    }

    //
    // wait specified amount of time in each phase (warm up, test, cool down)
    //
    if (timeSpan.GetWarmup() > 0)
    {
        TraceLoggingActivity<g_hEtwProvider, DISKSPD_TRACE_INFO, TRACE_LEVEL_NONE> WarmActivity;
        TraceLoggingWriteStart(WarmActivity, "Warm Up");

        if (bSynchStop)
        {
            assert(NULL != pSynch->hStopEvent);
            dwWaitStatus = WaitForSingleObject(pSynch->hStopEvent, 1000 * timeSpan.GetWarmup());
            if (WAIT_OBJECT_0 != dwWaitStatus && WAIT_TIMEOUT != dwWaitStatus)
            {
                PrintError("Error during WaitForSingleObject\n");
                _TerminateWorkerThreads(vhThreads);
                return false;
            }
            bBreak = (WAIT_TIMEOUT != dwWaitStatus);
        }
        else
        {
            Sleep(1000 * timeSpan.GetWarmup());
        }

        TraceLoggingWriteStop(WarmActivity, "Warm Up");
    }

    if (!bBreak) // proceed only if user didn't break the test
    {
        //FUTURE EXTENSION: starting ETW session shouldn't be done brutally here, should be done before warmup and here just a fast signal to start logging (see also stopping ETW session)
        //FUTURE EXTENSION: put an ETW mark here, for easier parsing by external tools

        //
        // start etw session
        //
        TRACEHANDLE hTraceSession = NULL;
        if (fUseETW)
        {
            PrintVerbose(profile.GetVerbose(), "starting trace session\n");
            hTraceSession = StartETWSession(profile);
            if (NULL == hTraceSession)
            {
                PrintError("Could not start ETW session\n");
                _TerminateWorkerThreads(vhThreads);
                return false;
            }

            if (NULL == CreateThread(NULL, 64 * 1024, etwThreadFunc, NULL, 0, NULL))
            {
                PrintError("Warning: unable to create thread for ETW session\n");
                _TerminateWorkerThreads(vhThreads);
                return false;
            }
            PrintVerbose(profile.GetVerbose(), "tracing events\n");
        }

        PrintVerbose(profile.GetVerbose(), "starting measurements...\n");

        //
        // notify the front-end that the test is about to start;
        // do it before starting timing in order not to perturb measurements
        //
        if (STRUCT_SYNCHRONIZATION_SUPPORTS(pSynch, pfnCallbackTestStarted) && (NULL != pSynch->pfnCallbackTestStarted))
        {
            pSynch->pfnCallbackTestStarted();
        }

        //
        // read performance counters
        //
        if (_GetSystemPerfInfo(&vPerfInit[0], g_SystemInformation.processorTopology._ulProcCount) == FALSE)
        {
            PrintError("Error reading performance counters\n");
            _StopETW(fUseETW, hTraceSession);
            _TerminateWorkerThreads(vhThreads);
            return false;
        }

        TraceLoggingActivity<g_hEtwProvider, DISKSPD_TRACE_INFO, TRACE_LEVEL_NONE> RunActivity;
        TraceLoggingWriteStart(RunActivity, "Run Time");

        //get cycle count (it will be used to calculate actual work time)
        ullStartTime = PerfTimer::GetTime();

#pragma warning( push )
#pragma warning( disable : 28931 )
        fAccountingOn = true;
#pragma warning( pop )

        assert(timeSpan.GetDuration() > 0);
        if (bSynchStop)
        {
            assert(NULL != pSynch->hStopEvent);
            dwWaitStatus = WaitForSingleObject(pSynch->hStopEvent, 1000 * timeSpan.GetDuration());
            if (WAIT_OBJECT_0 != dwWaitStatus && WAIT_TIMEOUT != dwWaitStatus)
            {
                PrintError("Error during WaitForSingleObject\n");
                _StopETW(fUseETW, hTraceSession);
                _TerminateWorkerThreads(vhThreads);    //FUTURE EXTENSION: worker threads should have a chance to free allocated memory (see also other places calling terminateWorkerThreads())
                return FALSE;
            }
            bBreak = (WAIT_TIMEOUT != dwWaitStatus);
        }
        else
        {
            Sleep(1000 * timeSpan.GetDuration());
        }

        fAccountingOn = false;

        //get cycle count and perf counters
        ullTimeDiff = PerfTimer::GetTime() - ullStartTime;

        TraceLoggingWriteStop(RunActivity, "Run Time");

        if (_GetSystemPerfInfo(&vPerfDone[0], g_SystemInformation.processorTopology._ulProcCount) == FALSE)
        {
            PrintError("Error getting performance counters\n");
            _StopETW(fUseETW, hTraceSession);
            _TerminateWorkerThreads(vhThreads);
            return false;
        }

        //
        // notify the front-end that the test has just finished;
        // do it after stopping timing in order not to perturb measurements
        //
        if (STRUCT_SYNCHRONIZATION_SUPPORTS(pSynch, pfnCallbackTestFinished) && (NULL != pSynch->pfnCallbackTestFinished))
        {
            pSynch->pfnCallbackTestFinished();
        }

        //
        // stop etw session
        //
        if (fUseETW)
        {
            PrintVerbose(profile.GetVerbose(), "stopping ETW session\n");
            pETWSession = StopETWSession(hTraceSession);
            if (NULL == pETWSession)
            {
                PrintError("Error stopping ETW session\n");
                return false;
            }
        }
    }
    else
    {
        ullTimeDiff = 0; // mark that no test was run
    }

    PrintVerbose(profile.GetVerbose(), "starting cool down...\n");
    if ((timeSpan.GetCooldown() > 0) && !bBreak)
    {
        TraceLoggingActivity<g_hEtwProvider, DISKSPD_TRACE_INFO, TRACE_LEVEL_NONE> CoolActivity;
        TraceLoggingWriteStart(CoolActivity, "Cool Down");

        if (bSynchStop)
        {
            assert(NULL != pSynch->hStopEvent);
            dwWaitStatus = WaitForSingleObject(pSynch->hStopEvent, 1000 * timeSpan.GetCooldown());
            if (WAIT_OBJECT_0 != dwWaitStatus && WAIT_TIMEOUT != dwWaitStatus)
            {
                PrintError("Error during WaitForSingleObject\n");
                //                stopETW(bUseETW, hTraceSession);
                _TerminateWorkerThreads(vhThreads);
                return false;
            }
        }
        else
        {
            Sleep(1000 * timeSpan.GetCooldown());
        }

        TraceLoggingWriteStop(CoolActivity, "Cool Down");
    }
    PrintVerbose(profile.GetVerbose(), "finished test...\n");

    //
    // signal the threads to finish
    //
    g_bRun = FALSE;
    if (timeSpan.GetCompletionRoutines())
    {
        if (!SetEvent(hEndEvent))
        {
            PrintError("Error signaling end event\n");
            //            stopETW(bUseETW, hTraceSession);
            return false;
        }
    }

    //
    // wait till all of the threads finish
    //
#pragma warning( push )
#pragma warning( disable : 28112 )
    while (g_lRunningThreadsCount > 0)
    {
        Sleep(10);    //FUTURE EXTENSION: a timeout should be implemented
    }
#pragma warning( pop )


    //check if there has been an error during threads execution
    if (g_bThreadError)
    {
        PrintError("There has been an error during threads execution\n");
        return false;
    }

    //
    // close events' handles
    //
    CloseHandle(hStartEvent);
    hStartEvent = NULL;

    if (NULL != hEndEvent)
    {
        CloseHandle(hEndEvent);
        hEndEvent = NULL;
    }
    //FUTURE EXTENSION: hStartEvent and hEndEvent should be closed in case of error too

    //
    // compute time spent by each cpu
    //
    for (unsigned int p = 0; p < g_SystemInformation.processorTopology._ulProcCount; ++p)
    {
        assert(vPerfDone[p].IdleTime.QuadPart >= vPerfInit[p].IdleTime.QuadPart);
        assert(vPerfDone[p].KernelTime.QuadPart >= vPerfInit[p].KernelTime.QuadPart);
        assert(vPerfDone[p].UserTime.QuadPart >= vPerfInit[p].UserTime.QuadPart);
        assert(vPerfDone[p].Reserved1[0].QuadPart >= vPerfInit[p].Reserved1[0].QuadPart);
        assert(vPerfDone[p].Reserved1[1].QuadPart >= vPerfInit[p].Reserved1[1].QuadPart);
        assert(vPerfDone[p].Reserved2 >= vPerfInit[p].Reserved2);

        vPerfDiff[p].IdleTime.QuadPart = vPerfDone[p].IdleTime.QuadPart - vPerfInit[p].IdleTime.QuadPart;
        vPerfDiff[p].KernelTime.QuadPart = vPerfDone[p].KernelTime.QuadPart - vPerfInit[p].KernelTime.QuadPart;
        vPerfDiff[p].UserTime.QuadPart = vPerfDone[p].UserTime.QuadPart - vPerfInit[p].UserTime.QuadPart;
        vPerfDiff[p].Reserved1[0].QuadPart = vPerfDone[p].Reserved1[0].QuadPart - vPerfInit[p].Reserved1[0].QuadPart;
        vPerfDiff[p].Reserved1[1].QuadPart = vPerfDone[p].Reserved1[1].QuadPart - vPerfInit[p].Reserved1[1].QuadPart;
        vPerfDiff[p].Reserved2 = vPerfDone[p].Reserved2 - vPerfInit[p].Reserved2;
    }

    //
    // process results and pass them to the result parser
    //

    // get processors perf. info
    results.vSystemProcessorPerfInfo = vPerfDiff;
    results.ullTimeCount = ullTimeDiff;

    //
    // create structure containing etw results and properties
    //
    results.fUseETW = fUseETW;
    if (fUseETW)
    {
        results.EtwEventCounters = g_EtwEventCounters;
        results.EtwSessionInfo = _GetResultETWSession(pETWSession);

        // TODO: refactor to a separate function
        results.EtwMask.bProcess = profile.GetEtwProcess();
        results.EtwMask.bThread = profile.GetEtwThread();
        results.EtwMask.bImageLoad = profile.GetEtwImageLoad();
        results.EtwMask.bDiskIO = profile.GetEtwDiskIO();
        results.EtwMask.bMemoryPageFaults = profile.GetEtwMemoryPageFaults();
        results.EtwMask.bMemoryHardFaults = profile.GetEtwMemoryHardFaults();
        results.EtwMask.bNetwork = profile.GetEtwNetwork();
        results.EtwMask.bRegistry = profile.GetEtwRegistry();
        results.EtwMask.bUsePagedMemory = profile.GetEtwUsePagedMemory();
        results.EtwMask.bUsePerfTimer = profile.GetEtwUsePerfTimer();
        results.EtwMask.bUseSystemTimer = profile.GetEtwUseSystemTimer();
        results.EtwMask.bUseCyclesCounter = profile.GetEtwUseCyclesCounter();

        free(pETWSession);
    }

    // free memory used by random data write buffers
    for (auto i = vTargets.begin(); i != vTargets.end(); i++)
    {
        i->FreeRandomDataWriteBuffer();
    }

    // TODO: this won't catch error cases, which exit early
    InterlockedExchange(&g_lGeneratorRunning, 0);
    return true;
}

vector<struct IORequestGenerator::CreateFileParameters> IORequestGenerator::_GetFilesToPrecreate(const Profile& profile) const
{
    vector<struct CreateFileParameters> vFilesToCreate;
    const vector<TimeSpan>& vTimeSpans = profile.GetTimeSpans();
    map<string, vector<struct CreateFileParameters>> filesMap;
    for (const auto& timeSpan : vTimeSpans)
    {
        vector<Target> vTargets(timeSpan.GetTargets());
        for (const auto& target : vTargets)
        {
            struct CreateFileParameters createFileParameters;
            createFileParameters.sPath = target.GetPath();
            createFileParameters.ullFileSize = target.GetFileSize();
            createFileParameters.fZeroWriteBuffers = target.GetZeroWriteBuffers();

            filesMap[createFileParameters.sPath].push_back(createFileParameters);
        }
    }

    PrecreateFiles filter = profile.GetPrecreateFiles();
    for (auto fileMapEntry : filesMap)
    {
        if (fileMapEntry.second.size() > 0)
        {
            UINT64 ullLastNonZeroSize = fileMapEntry.second[0].ullFileSize;
            UINT64 ullMaxSize = fileMapEntry.second[0].ullFileSize;
            bool fLastZeroWriteBuffers = fileMapEntry.second[0].fZeroWriteBuffers;
            bool fHasZeroSizes = false;
            bool fConstantSize = true;
            bool fConstantZeroWriteBuffers = true;
            for (auto file : fileMapEntry.second)
            {
                ullMaxSize = max(ullMaxSize, file.ullFileSize);
                if (ullLastNonZeroSize == 0)
                {
                    ullLastNonZeroSize = file.ullFileSize;
                }
                if (file.ullFileSize == 0)
                {
                    fHasZeroSizes = true;
                }
                if ((file.ullFileSize != 0) && (file.ullFileSize != ullLastNonZeroSize))
                {
                    fConstantSize = false;
                }
                if (file.fZeroWriteBuffers != fLastZeroWriteBuffers)
                {
                    fConstantZeroWriteBuffers = false;
                }
                if (file.ullFileSize != 0)
                {
                    ullLastNonZeroSize = file.ullFileSize;
                }
                fLastZeroWriteBuffers = file.fZeroWriteBuffers;
            }

            if (fConstantZeroWriteBuffers && ullMaxSize > 0)
            {
                struct CreateFileParameters file = fileMapEntry.second[0];
                file.ullFileSize = ullMaxSize;
                if (filter == PrecreateFiles::UseMaxSize)
                {
                    vFilesToCreate.push_back(file);
                }
                else if ((filter == PrecreateFiles::OnlyFilesWithConstantSizes) && fConstantSize && !fHasZeroSizes)
                {
                    vFilesToCreate.push_back(file);
                }
                else if ((filter == PrecreateFiles::OnlyFilesWithConstantOrZeroSizes) && fConstantSize)
                {
                    vFilesToCreate.push_back(file);
                }
            }
        }
    }

    return vFilesToCreate;
}