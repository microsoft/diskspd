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
void *ManagedMalloc(size_t size);
namespace UnitTests
{
    class IORequestGeneratorUnitTests;
}

#define FIRST_OFFSET 0xFFFFFFFFFFFFFFFFULL

class IORequestGenerator
{
public:
    IORequestGenerator() :
        _hNTDLL(nullptr)
    {

    }

    bool GenerateRequests(Profile& profile, IResultParser& resultParser, PRINTF pPrintOut, PRINTF pPrintError, PRINTF pPrintVerbose, struct Synchronization *pSynch);
    static UINT64 GetNextFileOffset(ThreadParameters& tp, size_t targetNum, UINT64 prevOffset);

private:

    struct CreateFileParameters
    {
		string sPath;
        UINT64 ullFileSize = 0;
        bool fZeroWriteBuffers=false;
    };

    bool _GenerateRequestsForTimeSpan(const Profile& profile, const TimeSpan& timeSpan, Results& results, struct Synchronization *pSynch);
	static void _AbortWorkerThreads(HANDLE hStartEvent, vector<HANDLE>& vhThreads);
	static void _CloseOpenFiles(vector<HANDLE>& vhFiles);
    DWORD _CreateDirectoryPath(const char *pszPath) const;
    bool _CreateFile(UINT64 ullFileSize, const char *pszFilename, bool fZeroBuffers, bool fVerbose) const;
	static void _DisplayFileSizeVerbose(bool fVerbose, UINT64 fsize);
    bool _GetActiveGroupsAndProcs() const; /* not implemented */
	static struct ETWSessionInfo _GetResultETWSession(const EVENT_TRACE_PROPERTIES *pTraceProperties);
	static bool _GetSystemPerfInfo(SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION *pInfo, UINT32 uCpuCount);
    void _InitializeGlobalParameters();
    bool _LoadDLLs();
	static bool _StopETW(bool fUseETW, TRACEHANDLE hTraceSession);
	static void _TerminateWorkerThreads(vector<HANDLE>& vhThreads);
    bool _ValidateProfile(const Profile& profile) const; /* Not implemented*/
	static vector<struct CreateFileParameters> _GetFilesToPrecreate(const Profile& profile);
    void _MarkFilesAsCreated(Profile& profile, const vector<struct CreateFileParameters>& vFiles) const; /* Not implemented */
    bool _PrecreateFiles(Profile& profile) const;

    HINSTANCE volatile _hNTDLL;     //handle to ntdll.dll

    friend class UnitTests::IORequestGeneratorUnitTests;
};
