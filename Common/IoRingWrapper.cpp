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

#include "Common.h"

PFN_CreateIoRing               s_pfnCreateIoRing               = nullptr;
PFN_CloseIoRing                s_pfnCloseIoRing                = nullptr;
PFN_SubmitIoRing               s_pfnSubmitIoRing               = nullptr;
PFN_PopIoRingCompletion        s_pfnPopIoRingCompletion        = nullptr;
PFN_BuildIoRingReadFile        s_pfnBuildIoRingReadFile        = nullptr;
PFN_BuildIoRingWriteFile       s_pfnBuildIoRingWriteFile       = nullptr;
PFN_BuildIoRingRegisterBuffers s_pfnBuildIoRingRegisterBuffers = nullptr;

HRESULT LoadIoRingApis()
{
    if (s_pfnCreateIoRing != nullptr)
    {
        return S_OK;
    }

    // kernelbase.dll is always loaded in every process; use GetModuleHandle
    // to avoid an unnecessary reference count increment.
    HMODULE hMod = GetModuleHandleW(L"kernelbase.dll");
    if (hMod == nullptr)
    {
        return E_FAIL;
    }

    s_pfnCreateIoRing = reinterpret_cast<PFN_CreateIoRing>(
        GetProcAddress(hMod, "CreateIoRing"));

    s_pfnCloseIoRing = reinterpret_cast<PFN_CloseIoRing>(
        GetProcAddress(hMod, "CloseIoRing"));

    s_pfnSubmitIoRing = reinterpret_cast<PFN_SubmitIoRing>(
        GetProcAddress(hMod, "SubmitIoRing"));

    s_pfnPopIoRingCompletion = reinterpret_cast<PFN_PopIoRingCompletion>(
        GetProcAddress(hMod, "PopIoRingCompletion"));

    s_pfnBuildIoRingReadFile = reinterpret_cast<PFN_BuildIoRingReadFile>(
        GetProcAddress(hMod, "BuildIoRingReadFile"));

    s_pfnBuildIoRingWriteFile = reinterpret_cast<PFN_BuildIoRingWriteFile>(
        GetProcAddress(hMod, "BuildIoRingWriteFile"));

    s_pfnBuildIoRingRegisterBuffers = reinterpret_cast<PFN_BuildIoRingRegisterBuffers>(
        GetProcAddress(hMod, "BuildIoRingRegisterBuffers"));

    // If any pointer is missing, clear all and fail.
    if (s_pfnCreateIoRing == nullptr ||
        s_pfnCloseIoRing == nullptr ||
        s_pfnSubmitIoRing == nullptr ||
        s_pfnPopIoRingCompletion == nullptr ||
        s_pfnBuildIoRingReadFile == nullptr ||
        s_pfnBuildIoRingWriteFile == nullptr ||
        s_pfnBuildIoRingRegisterBuffers == nullptr)
    {
        s_pfnCreateIoRing               = nullptr;
        s_pfnCloseIoRing                = nullptr;
        s_pfnSubmitIoRing               = nullptr;
        s_pfnPopIoRingCompletion        = nullptr;
        s_pfnBuildIoRingReadFile        = nullptr;
        s_pfnBuildIoRingWriteFile       = nullptr;
        s_pfnBuildIoRingRegisterBuffers = nullptr;

        return E_FAIL;
    }

    return S_OK;
}
