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
#define INITGUID        //Include this #define to use SystemTraceControlGuid in Evntrace.h.
#include <Evntrace.h>   //ETW
#include <Winternl.h>   //ntdll.dll


void PrintError(const char *format, ...);
namespace UnitTests
{
    class IORequestGeneratorUnitTests;
}

class IORequestGenerator
{
public:
    IORequestGenerator() :
        _hNTDLL(nullptr)
    {

    }

    bool GenerateRequests(Profile& profile, IResultParser& resultParser, struct Synchronization *pSynch);

private:

    struct CreateFileParameters
    {
        string sPath;
        UINT64 ullFileSize;
        bool fZeroWriteBuffers;
    };

    bool _GenerateRequestsForTimeSpan(const Profile& profile, const TimeSpan& timeSpan, Results& results, struct Synchronization *pSynch);
    void _AbortWorkerThreads(HANDLE hStartEvent, vector<HANDLE>& vhThreads) const;
    void _CloseOpenFiles(vector<HANDLE>& vhFiles) const;
    DWORD _CreateDirectoryPath(const char *path) const;
    bool _CreateFile(UINT64 ullFileSize, const char *pszFilename, bool fZeroBuffers, bool fVerbose) const;
    bool _GetActiveGroupsAndProcs() const;
    struct ETWSessionInfo _GetResultETWSession(const EVENT_TRACE_PROPERTIES *pTraceProperties) const;
    bool _GetSystemPerfInfo(SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION *pInfo, UINT32 uCpuCount) const;
    void _InitializeGlobalParameters();
    bool _LoadDLLs();
    bool _StopETW(bool fUseETW, TRACEHANDLE hTraceSession) const;
    void _TerminateWorkerThreads(vector<HANDLE>& vhThreads) const;
    bool _ValidateProfile(const Profile& profile) const;
    vector<struct CreateFileParameters> _GetFilesToPrecreate(const Profile& profile) const;
    void _MarkFilesAsCreated(Profile& profile, const vector<struct CreateFileParameters>& vFiles) const;
    bool _PrecreateFiles(Profile& profile) const;

    HINSTANCE volatile _hNTDLL;     //handle to ntdll.dll

    friend class UnitTests::IORequestGeneratorUnitTests;
};
