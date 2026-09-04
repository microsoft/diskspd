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

#if !defined(NTDDI_WIN10_NI) || (NTDDI_VERSION < NTDDI_WIN10_NI)

typedef enum FILE_WRITE_FLAGS
{
    FILE_WRITE_FLAGS_NONE = 0,
    FILE_WRITE_FLAGS_WRITE_THROUGH = 0x00000001,
} FILE_WRITE_FLAGS;
DEFINE_ENUM_FLAG_OPERATORS(FILE_WRITE_FLAGS)

typedef enum FILE_FLUSH_MODE
{
    FILE_FLUSH_DEFAULT = 0,
    FILE_FLUSH_DATA,
    FILE_FLUSH_MIN_METADATA,
    FILE_FLUSH_NO_SYNC,
} FILE_FLUSH_MODE;

#endif

#pragma push_macro("NTDDI_VERSION")
#undef NTDDI_VERSION
#define NTDDI_VERSION 0x0A00000C  // NTDDI_WIN10_NI

#include <ioringapi.h>

#pragma pop_macro("NTDDI_VERSION")

#ifndef FSCTL_MANAGE_BYPASS_IO
#define FSCTL_MANAGE_BYPASS_IO CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 274, METHOD_BUFFERED, FILE_ANY_ACCESS)

typedef enum _FS_BPIO_OPERATIONS {
    FS_BPIO_OP_ENABLE = 1,
    FS_BPIO_OP_DISABLE = 2,
    FS_BPIO_OP_QUERY = 3,
    FS_BPIO_OP_VOLUME_STACK_PAUSE = 4,
    FS_BPIO_OP_VOLUME_STACK_RESUME = 5,
    FS_BPIO_OP_STREAM_PAUSE = 6,
    FS_BPIO_OP_STREAM_RESUME = 7,
    FS_BPIO_OP_GET_INFO = 8,
} FS_BPIO_OPERATIONS;

typedef enum _FS_BPIO_INFLAGS {
    FSBPIO_INFL_None = 0,
    FSBPIO_INFL_SKIP_STORAGE_STACK_QUERY = 1,
} FS_BPIO_INFLAGS;
DEFINE_ENUM_FLAG_OPERATORS(FS_BPIO_INFLAGS)

typedef struct _FS_BPIO_INPUT {
    FS_BPIO_OPERATIONS Operation;
    FS_BPIO_INFLAGS InFlags;
    DWORDLONG Reserved1;
    DWORDLONG Reserved2;
} FS_BPIO_INPUT, *PFS_BPIO_INPUT;

typedef enum _FS_BPIO_OUTFLAGS {
    FSBPIO_OUTFL_None = 0,
    FSBPIO_OUTFL_VOLUME_STACK_BYPASS_PAUSED = 1,
    FSBPIO_OUTFL_STREAM_BYPASS_PAUSED = 2,
    FSBPIO_OUTFL_FILTER_ATTACH_BLOCKED = 4,
    FSBPIO_OUTFL_COMPATIBLE_STORAGE_DRIVER = 8,
} FS_BPIO_OUTFLAGS;
DEFINE_ENUM_FLAG_OPERATORS(FS_BPIO_OUTFLAGS)

typedef struct _FS_BPIO_RESULTS {
    DWORD OpStatus;
    WORD FailingDriverNameLen;
    WCHAR FailingDriverName[32];
    WORD FailureReasonLen;
    WCHAR FailureReason[128];
} FS_BPIO_RESULTS, *PFS_BPIO_RESULTS;

typedef struct _FS_BPIO_INFO {
    DWORD ActiveBypassIoCount;
    WORD StorageDriverNameLen;
    WCHAR StorageDriverName[32];
} FS_BPIO_INFO, *PFS_BPIO_INFO;

typedef struct _FS_BPIO_OUTPUT {
    FS_BPIO_OPERATIONS Operation;
    FS_BPIO_OUTFLAGS OutFlags;
    DWORDLONG Reserved1;
    DWORDLONG Reserved2;
    union {
        FS_BPIO_RESULTS Enable;
        FS_BPIO_RESULTS Query;
        FS_BPIO_RESULTS VolumeStackResume;
        FS_BPIO_RESULTS StreamResume;
        FS_BPIO_INFO GetInfo;
    };
} FS_BPIO_OUTPUT, *PFS_BPIO_OUTPUT;

#endif // FSCTL_MANAGE_BYPASS_IO

typedef HRESULT (STDAPICALLTYPE* PFN_CreateIoRing)(
    IORING_VERSION ioringVersion,
    IORING_CREATE_FLAGS flags,
    UINT32 submissionQueueSize,
    UINT32 completionQueueSize,
    _Out_ HIORING* h
    );

typedef HRESULT (STDAPICALLTYPE* PFN_CloseIoRing)(
    _In_ _Post_ptr_invalid_ HIORING ioRing
    );

typedef HRESULT (STDAPICALLTYPE* PFN_SubmitIoRing)(
    _In_ HIORING ioRing,
    UINT32 waitOperations,
    UINT32 milliseconds,
    _Out_opt_ UINT32* submittedEntries
    );

typedef HRESULT (STDAPICALLTYPE* PFN_PopIoRingCompletion)(
    _In_ HIORING ioRing,
    _Out_ IORING_CQE* cqe
    );

typedef HRESULT (STDAPICALLTYPE* PFN_BuildIoRingReadFile)(
    _In_ HIORING ioRing,
    IORING_HANDLE_REF fileRef,
    IORING_BUFFER_REF dataRef,
    UINT32 numberOfBytesToRead,
    UINT64 fileOffset,
    UINT_PTR userData,
    IORING_SQE_FLAGS sqeFlags
    );

typedef HRESULT (STDAPICALLTYPE* PFN_BuildIoRingWriteFile)(
    _In_ HIORING ioRing,
    IORING_HANDLE_REF fileRef,
    IORING_BUFFER_REF bufferRef,
    UINT32 numberOfBytesToWrite,
    UINT64 fileOffset,
    FILE_WRITE_FLAGS writeFlags,
    UINT_PTR userData,
    IORING_SQE_FLAGS sqeFlags
    );

typedef HRESULT (STDAPICALLTYPE* PFN_BuildIoRingRegisterBuffers)(
    _In_ HIORING ioRing,
    UINT32 count,
    _In_reads_(count) IORING_BUFFER_INFO const buffers[],
    UINT_PTR userData
    );

extern PFN_CreateIoRing               s_pfnCreateIoRing;
extern PFN_CloseIoRing                s_pfnCloseIoRing;
extern PFN_SubmitIoRing               s_pfnSubmitIoRing;
extern PFN_PopIoRingCompletion        s_pfnPopIoRingCompletion;
extern PFN_BuildIoRingReadFile        s_pfnBuildIoRingReadFile;
extern PFN_BuildIoRingWriteFile       s_pfnBuildIoRingWriteFile;
extern PFN_BuildIoRingRegisterBuffers s_pfnBuildIoRingRegisterBuffers;

// Resolve IoRing API function pointers from kernelbase.dll.
// Called once per timespan from _GenerateRequestsForTimeSpan on the main
// thread before worker threads are spawned. The check-then-act guard on
// s_pfnCreateIoRing is safe because of this single-threaded call contract.
HRESULT LoadIoRingApis();
