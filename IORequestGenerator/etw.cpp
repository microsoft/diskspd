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

#include "etw.h"
#include "common.h"
#include <stdio.h>
#include <stdlib.h>

#include <Wmistr.h>     //WNODE_HEADER

#define INITGUID        //Include this #define to use SystemTraceControlGuid in Evntrace.h.
#include <Evntrace.h>   //ETW

#include "IORequestGenerator.h"

// variables declared in IORequestGenerator.cpp
extern struct ETWEventCounters g_EtwEventCounters;
extern BOOL volatile g_bTracing;


DEFINE_GUID ( /* 3d6fa8d4-fe05-11d0-9dda-00c04fd7ba7c */
    DiskIoGuid,
    0x3d6fa8d4,
    0xfe05,
    0x11d0,
    0x9d, 0xda, 0x00, 0xc0, 0x4f, 0xd7, 0xba, 0x7c
  );

DEFINE_GUID ( /* 90cbdc39-4a3e-11d1-84f4-0000f80464e3 */
    FileIoGuid,
    0x90cbdc39,
    0x4a3e,
    0x11d1,
    0x84, 0xf4, 0x00, 0x00, 0xf8, 0x04, 0x64, 0xe3
  );

DEFINE_GUID ( /* 2cb15d1d-5fc1-11d2-abe1-00a0c911f518 */
    ImageLoadGuid,
    0x2cb15d1d,
    0x5fc1,
    0x11d2,
    0xab, 0xe1, 0x00, 0xa0, 0xc9, 0x11, 0xf5, 0x18
  );

DEFINE_GUID ( /* 3d6fa8d3-fe05-11d0-9dda-00c04fd7ba7c */
    PageFaultGuid,
    0x3d6fa8d3,
    0xfe05,
    0x11d0,
    0x9d, 0xda, 0x00, 0xc0, 0x4f, 0xd7, 0xba, 0x7c
  );

DEFINE_GUID ( /* 9a280ac0-c8e0-11d1-84e2-00c04fb998a2 */
    TcpIpGuid,
    0x9a280ac0,
    0xc8e0,
    0x11d1,
    0x84, 0xe2, 0x00, 0xc0, 0x4f, 0xb9, 0x98, 0xa2
  );

DEFINE_GUID ( /* bf3a50c5-a9c9-4988-a005-2df0b7c80f80 */
    UdpIpGuid,
    0xbf3a50c5,
    0xa9c9,
    0x4988,
    0xa0, 0x05, 0x2d, 0xf0, 0xb7, 0xc8, 0x0f, 0x80
  );

DEFINE_GUID ( /* 3d6fa8d0-fe05-11d0-9dda-00c04fd7ba7c */
    ProcessGuid,
    0x3d6fa8d0,
    0xfe05,
    0x11d0,
    0x9d, 0xda, 0x00, 0xc0, 0x4f, 0xd7, 0xba, 0x7c
  );

DEFINE_GUID ( /* AE53722E-C863-11d2-8659-00C04FA321A1 */
    RegistryGuid, 
    0xae53722e,
    0xc863,
    0x11d2,
    0x86, 0x59, 0x0, 0xc0, 0x4f, 0xa3, 0x21, 0xa1
);

DEFINE_GUID ( /* 3d6fa8d1-fe05-11d0-9dda-00c04fd7ba7c */
    ThreadGuid,
    0x3d6fa8d1,
    0xfe05,
    0x11d0,
    0x9d, 0xda, 0x00, 0xc0, 0x4f, 0xd7, 0xba, 0x7c
  );

/*****************************************************************************/
// consumes events' data
//
BOOL TraceEvents()
{
    TRACEHANDLE handles[1];
    EVENT_TRACE_LOGFILE logfile;

    memset(&logfile, 0, sizeof(EVENT_TRACE_LOGFILE));

    logfile.LoggerName = KERNEL_LOGGER_NAME;
    logfile.LogFileName = NULL;
    logfile.LogFileMode = EVENT_TRACE_REAL_TIME_MODE;

    logfile.IsKernelTrace = true;

    handles[0] = OpenTrace(&logfile);
    if( (TRACEHANDLE)INVALID_HANDLE_VALUE == handles[0] )
    {
        PrintError("ETW ERROR: OpenTrace failed (error code: %d)\n", GetLastError());
        return false;
    }
    else
    {
        ProcessTrace(handles, 1, 0, 0);
        CloseTrace(handles[0]);
    }

    return true;
}

/*****************************************************************************/
// allocates memory for ETW properties structure
//
PEVENT_TRACE_PROPERTIES allocateEventTraceProperties()
{
    PEVENT_TRACE_PROPERTIES pProperties = NULL;
    size_t size = 0;


    size = sizeof(EVENT_TRACE_PROPERTIES)+sizeof(KERNEL_LOGGER_NAME);
    pProperties = (PEVENT_TRACE_PROPERTIES)malloc(size);
    if( NULL == pProperties )
    {
        PrintError("FATAL ERROR: unable to allocate memory (error code: %d)\n", GetLastError());
        return NULL;
    }

    memset(pProperties, 0, size);

    pProperties->LoggerNameOffset = sizeof(EVENT_TRACE_PROPERTIES);
    pProperties->Wnode.BufferSize = (ULONG)size;
    pProperties->Wnode.Flags = WNODE_FLAG_TRACED_GUID;

    strcpy_s((char *)pProperties+pProperties->LoggerNameOffset,
        size-pProperties->LoggerNameOffset, KERNEL_LOGGER_NAME);

    return pProperties;
}

/*****************************************************************************/
// callback function for disk I/O events
void WINAPI eventDiskIo(PEVENT_TRACE pEvent)
{
    if( EVENT_TRACE_TYPE_IO_READ == pEvent->Header.Class.Type )
    {
        ++g_EtwEventCounters.ullIORead;
    }
    else if( EVENT_TRACE_TYPE_IO_WRITE == pEvent->Header.Class.Type )
    {
        ++g_EtwEventCounters.ullIOWrite;
    }
}

/*****************************************************************************/
// callback function for LoadImage events
//
void WINAPI eventLoadImage(PEVENT_TRACE pEvent)
{
    if( EVENT_TRACE_TYPE_LOAD == pEvent->Header.Class.Type )
    {
        ++g_EtwEventCounters.ullImageLoad;
    }
}

/*****************************************************************************/
// callback function for memory events
//
void WINAPI eventPageFault(PEVENT_TRACE pEvent)
{
    if( EVENT_TRACE_TYPE_MM_COW == pEvent->Header.Class.Type )
    {
        ++g_EtwEventCounters.ullMMCopyOnWrite;
    }
    else if( EVENT_TRACE_TYPE_MM_DZF == pEvent->Header.Class.Type )
    {
        ++g_EtwEventCounters.ullMMDemandZeroFault;
    }
    else if( EVENT_TRACE_TYPE_MM_GPF == pEvent->Header.Class.Type )
    {
        ++g_EtwEventCounters.ullMMGuardPageFault;
    }
    else if( EVENT_TRACE_TYPE_MM_HPF == pEvent->Header.Class.Type )
    {
        ++g_EtwEventCounters.ullMMHardPageFault;
    }
    else if( EVENT_TRACE_TYPE_MM_TF == pEvent->Header.Class.Type )
    {
        ++g_EtwEventCounters.ullMMTransitionFault;
    }
}

/*****************************************************************************/
// callback function for network and TCP/IP events
//
void WINAPI eventTcpIp(PEVENT_TRACE pEvent)
{
    if( EVENT_TRACE_TYPE_ACCEPT == pEvent->Header.Class.Type )
    {
        ++g_EtwEventCounters.ullNetAccept;
    }
    else if( EVENT_TRACE_TYPE_CONNECT == pEvent->Header.Class.Type )
    {
        ++g_EtwEventCounters.ullNetConnect;
    }
    else if( EVENT_TRACE_TYPE_DISCONNECT == pEvent->Header.Class.Type )
    {
        ++g_EtwEventCounters.ullNetDisconnect;
    }
    else if( EVENT_TRACE_TYPE_RECEIVE == pEvent->Header.Class.Type )
    {
        ++g_EtwEventCounters.ullNetTcpReceive;
    }
    else if( EVENT_TRACE_TYPE_RECONNECT == pEvent->Header.Class.Type )
    {
        ++g_EtwEventCounters.ullNetReconnect;
    }
    else if( EVENT_TRACE_TYPE_RETRANSMIT == pEvent->Header.Class.Type )
    {
        ++g_EtwEventCounters.ullNetRetransmit;
    }
    else if( EVENT_TRACE_TYPE_SEND == pEvent->Header.Class.Type )
    {
        ++g_EtwEventCounters.ullNetTcpSend;
    }
}

/*****************************************************************************/
// callback function for UDP/IP events
//
void WINAPI eventUdpIp(PEVENT_TRACE pEvent)
{
    if( EVENT_TRACE_TYPE_SEND == pEvent->Header.Class.Type )
    {
        ++g_EtwEventCounters.ullNetUdpSend;
    }
    else if( EVENT_TRACE_TYPE_RECEIVE == pEvent->Header.Class.Type )
    {
        ++g_EtwEventCounters.ullNetUdpReceive;
    }
}

/*****************************************************************************/
// callback function for thread events
//
void WINAPI eventThread(PEVENT_TRACE pEvent)
{
    if( EVENT_TRACE_TYPE_START == pEvent->Header.Class.Type )
    {
        ++g_EtwEventCounters.ullThreadStart;
    }
    else if( EVENT_TRACE_TYPE_END == pEvent->Header.Class.Type )
    {
        ++g_EtwEventCounters.ullThreadEnd;
    }
}

/*****************************************************************************/
// callback function for process events
//
void WINAPI eventProcess(PEVENT_TRACE pEvent)
{
    if( EVENT_TRACE_TYPE_START == pEvent->Header.Class.Type )
    {
        ++g_EtwEventCounters.ullProcessStart;
    }
    else if( EVENT_TRACE_TYPE_END == pEvent->Header.Class.Type )
    {
        ++g_EtwEventCounters.ullProcessEnd;
    }
}

/*****************************************************************************/
// callback function for registry events
//
void WINAPI eventRegistry(PEVENT_TRACE pEvent)
{
    if( EVENT_TRACE_TYPE_REGCREATE == pEvent->Header.Class.Type )
    {
        ++g_EtwEventCounters.ullRegCreate;
    }
    else if( EVENT_TRACE_TYPE_REGDELETE == pEvent->Header.Class.Type )
    {
        ++g_EtwEventCounters.ullRegDelete;
    }
    else if( EVENT_TRACE_TYPE_REGDELETEVALUE == pEvent->Header.Class.Type )
    {
        ++g_EtwEventCounters.ullRegDeleteValue;
    }
    else if( EVENT_TRACE_TYPE_REGENUMERATEKEY == pEvent->Header.Class.Type )
    {
        ++g_EtwEventCounters.ullRegEnumerateKey;
    }
    else if( EVENT_TRACE_TYPE_REGENUMERATEVALUEKEY == pEvent->Header.Class.Type )
    {
        ++g_EtwEventCounters.ullRegEnumerateValueKey;
    }
    else if( EVENT_TRACE_TYPE_REGFLUSH == pEvent->Header.Class.Type )
    {
        ++g_EtwEventCounters.ullRegFlush;
    }
    else if( EVENT_TRACE_TYPE_REGOPEN == pEvent->Header.Class.Type )
    {
        ++g_EtwEventCounters.ullRegOpen;
    }
    else if( EVENT_TRACE_TYPE_REGQUERY == pEvent->Header.Class.Type )
    {
        ++g_EtwEventCounters.ullRegQuery;
    }
    else if( EVENT_TRACE_TYPE_REGQUERYMULTIPLEVALUE == pEvent->Header.Class.Type )
    {
        ++g_EtwEventCounters.ullRegQueryMultipleValue;
    }
    else if( EVENT_TRACE_TYPE_REGQUERYVALUE == pEvent->Header.Class.Type )
    {
        ++g_EtwEventCounters.ullRegQueryValue;
    }
    else if( EVENT_TRACE_TYPE_REGSETINFORMATION == pEvent->Header.Class.Type )
    {
        ++g_EtwEventCounters.ullRegSetInformation;
    }
    else if( EVENT_TRACE_TYPE_REGSETVALUE == pEvent->Header.Class.Type )
    {
        ++g_EtwEventCounters.ullRegSetValue;
    }
}

/*****************************************************************************/
// starts ETW session and sets callback functions for various groups of events
//
TRACEHANDLE StartETWSession(const Profile& profile)
{
    PEVENT_TRACE_PROPERTIES pProperties;

    pProperties = allocateEventTraceProperties();
    if (nullptr == pProperties)
    {
        return 0;
    }

    pProperties->LogFileMode = EVENT_TRACE_REAL_TIME_MODE;

    //use paged memory
    if (profile.GetEtwUsePagedMemory())
    {
        pProperties->LogFileMode |= EVENT_TRACE_USE_PAGED_MEMORY;
    }

    //
    // event classes
    //
    if (profile.GetEtwProcess())
    {
        pProperties->EnableFlags |= EVENT_TRACE_FLAG_PROCESS;
        SetTraceCallback(&ProcessGuid, eventProcess);
    }

    if (profile.GetEtwThread())
    {
        pProperties->EnableFlags |= EVENT_TRACE_FLAG_THREAD;
        SetTraceCallback(&ThreadGuid, eventThread);
    }

    if (profile.GetEtwImageLoad())
    {
        pProperties->EnableFlags |= EVENT_TRACE_FLAG_IMAGE_LOAD;
        SetTraceCallback(&ImageLoadGuid, eventLoadImage);
    }

    if (profile.GetEtwDiskIO())
    {
        pProperties->EnableFlags |= EVENT_TRACE_FLAG_DISK_IO;
        SetTraceCallback(&DiskIoGuid, eventDiskIo);
    }

    if (profile.GetEtwMemoryPageFaults())
    {
        pProperties->EnableFlags |= EVENT_TRACE_FLAG_MEMORY_PAGE_FAULTS;
        SetTraceCallback(&PageFaultGuid, eventPageFault);
    }

    if (profile.GetEtwMemoryHardFaults())
    {
        pProperties->EnableFlags |= EVENT_TRACE_FLAG_MEMORY_HARD_FAULTS;
        SetTraceCallback(&PageFaultGuid, eventPageFault);
    }

    if (profile.GetEtwNetwork())
    {
        pProperties->EnableFlags |= EVENT_TRACE_FLAG_NETWORK_TCPIP;
        SetTraceCallback(&TcpIpGuid, eventTcpIp);
        SetTraceCallback(&UdpIpGuid, eventUdpIp);
    }

    if (profile.GetEtwRegistry())
    {
        pProperties->EnableFlags |= EVENT_TRACE_FLAG_REGISTRY;
        SetTraceCallback(&RegistryGuid, eventRegistry);
    }

    //
    // timer
    //
    if (profile.GetEtwUsePerfTimer())
    {
        pProperties->Wnode.ClientContext = 1;
    }
    if (profile.GetEtwUseSystemTimer())
    {
        pProperties->Wnode.ClientContext = 2;
    }
    if (profile.GetEtwUseCyclesCounter())
    {
        pProperties->Wnode.ClientContext = 3;
    }

    pProperties->Wnode.Guid = SystemTraceControlGuid;

    TRACEHANDLE hTraceSession;
    ULONG ret = StartTrace(&hTraceSession, KERNEL_LOGGER_NAME, pProperties);
    free(pProperties);
    if (ERROR_SUCCESS != ret)
    {
        PrintError("Error starting trace session\n");
        return 0;
    }

    return hTraceSession;
}

/*****************************************************************************/
// stops ETW session
//
PEVENT_TRACE_PROPERTIES StopETWSession(TRACEHANDLE hTraceSession)
{
    PEVENT_TRACE_PROPERTIES pProperties;

    pProperties = allocateEventTraceProperties();
    if( NULL == pProperties )
    {
        return NULL;
    }

    ULONG ret;

    ret = ControlTrace(hTraceSession, NULL, pProperties, EVENT_TRACE_CONTROL_STOP);
    if( ERROR_SUCCESS != ret )
    {
        PrintError("Error stopping trace session\n");
        return NULL;
    }

    //wait
    while(g_bTracing)
    {
        Sleep(10);  // TODO: remove active waiting
    }

    return pProperties;
}