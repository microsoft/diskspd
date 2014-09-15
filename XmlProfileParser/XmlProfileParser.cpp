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

#include "XmlProfileParser.h"
#include <Objbase.h>
#include <msxml6.h>
#include <atlcomcli.h>
#include <assert.h>

bool XmlProfileParser::ParseFile(const char *pszPath, Profile *pProfile)
{
    assert(pszPath != nullptr);
    assert(pProfile != nullptr);

    bool fComInitialized = false;
    HRESULT hr = CoInitialize(nullptr);
    if (SUCCEEDED(hr))
    {
        fComInitialized = true;
        CComPtr<IXMLDOMDocument2> spXmlDoc = nullptr;
        hr = CoCreateInstance(__uuidof(DOMDocument60), nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&spXmlDoc));
        if (SUCCEEDED(hr))
        {
            hr = spXmlDoc->put_async(VARIANT_FALSE);
            if (SUCCEEDED(hr))
            {
                VARIANT_BOOL fvIsOk;
                CComVariant vPath(pszPath);
                hr = spXmlDoc->load(vPath, &fvIsOk);
                if (SUCCEEDED(hr))
                {
                    bool fVerbose;
                    hr = _GetVerbose(spXmlDoc, &fVerbose);
                    if (SUCCEEDED(hr) && (hr != S_FALSE))
                    {
                        pProfile->SetVerbose(fVerbose);
                    }

                    if (SUCCEEDED(hr))
                    {
                        DWORD dwProgress;
                        hr = _GetProgress(spXmlDoc, &dwProgress);
                        if (SUCCEEDED(hr) && (hr != S_FALSE))
                        {
                            pProfile->SetProgress(dwProgress);
                        }
                    }

                    if (SUCCEEDED(hr))
                    {
                        string sResultFormat;
                        hr = _GetString(spXmlDoc, "//Profile/ResultFormat", &sResultFormat);
                        if (SUCCEEDED(hr) && (hr != S_FALSE) && sResultFormat == "xml")
                        {
                            pProfile->SetResultsFormat(ResultsFormat::Xml);
                        }
                    }

                    if (SUCCEEDED(hr))
                    {
                        string sCreateFiles;
                        hr = _GetString(spXmlDoc, "//Profile/PrecreateFiles", &sCreateFiles);
                        if (SUCCEEDED(hr) && (hr != S_FALSE))
                        {
                            if (sCreateFiles == "UseMaxSize")
                            {
                                pProfile->SetPrecreateFiles(PrecreateFiles::UseMaxSize);
                            }
                            else if (sCreateFiles == "CreateOnlyFilesWithConstantSizes")
                            {
                                pProfile->SetPrecreateFiles(PrecreateFiles::OnlyFilesWithConstantSizes);
                            }
                            else if (sCreateFiles == "CreateOnlyFilesWithConstantOrZeroSizes")
                            {
                                pProfile->SetPrecreateFiles(PrecreateFiles::OnlyFilesWithConstantOrZeroSizes);
                            }
                            else
                            {
                                hr = E_INVALIDARG;
                            }
                        }
                    }

                    if (SUCCEEDED(hr))
                    {
                        hr = _ParseEtw(spXmlDoc, pProfile);
                    }

                    if (SUCCEEDED(hr))
                    {
                        hr = _ParseTimeSpans(spXmlDoc, pProfile);
                    }
                }
            }
        }
    }
    if (fComInitialized)
    {
        CoUninitialize();
    }
    return SUCCEEDED(hr);
}

HRESULT XmlProfileParser::_ParseEtw(IXMLDOMDocument2 *pXmlDoc, Profile *pProfile)
{
    bool fEtwProcess;
    HRESULT hr = _GetBool(pXmlDoc, "//Profile/ETW/Process", &fEtwProcess);
    if (SUCCEEDED(hr) && (hr != S_FALSE))
    {
        pProfile->SetEtwEnabled(true);
        pProfile->SetEtwProcess(fEtwProcess);
    }

    if (SUCCEEDED(hr))
    {
        bool fEtwThread;
        hr = _GetBool(pXmlDoc, "//Profile/ETW/Thread", &fEtwThread);
        if (SUCCEEDED(hr) && (hr != S_FALSE))
        {
            pProfile->SetEtwEnabled(true);
            pProfile->SetEtwThread(fEtwThread);
        }
    }

    if (SUCCEEDED(hr))
    {
        bool fEtwImageLoad;
        hr = _GetBool(pXmlDoc, "//Profile/ETW/ImageLoad", &fEtwImageLoad);
        if (SUCCEEDED(hr) && (hr != S_FALSE))
        {
            pProfile->SetEtwEnabled(true);
            pProfile->SetEtwImageLoad(fEtwImageLoad);
        }
    }

    if (SUCCEEDED(hr))
    {
        bool fEtwDiskIO;
        hr = _GetBool(pXmlDoc, "//Profile/ETW/DiskIO", &fEtwDiskIO);
        if (SUCCEEDED(hr) && (hr != S_FALSE))
        {
            pProfile->SetEtwEnabled(true);
            pProfile->SetEtwDiskIO(fEtwDiskIO);
        }
    }

    if (SUCCEEDED(hr))
    {
        bool fEtwMemoryPageFaults;
        hr = _GetBool(pXmlDoc, "//Profile/ETW/MemoryPageFaults", &fEtwMemoryPageFaults);
        if (SUCCEEDED(hr) && (hr != S_FALSE))
        {
            pProfile->SetEtwEnabled(true);
            pProfile->SetEtwMemoryPageFaults(fEtwMemoryPageFaults);
        }
    }

    if (SUCCEEDED(hr))
    {
        bool fEtwMemoryHardFaults;
        hr = _GetBool(pXmlDoc, "//Profile/ETW/MemoryHardFaults", &fEtwMemoryHardFaults);
        if (SUCCEEDED(hr) && (hr != S_FALSE))
        {
            pProfile->SetEtwEnabled(true);
            pProfile->SetEtwMemoryHardFaults(fEtwMemoryHardFaults);
        }
    }

    if (SUCCEEDED(hr))
    {
        bool fEtwNetwork;
        hr = _GetBool(pXmlDoc, "//Profile/ETW/Network", &fEtwNetwork);
        if (SUCCEEDED(hr) && (hr != S_FALSE))
        {
            pProfile->SetEtwEnabled(true);
            pProfile->SetEtwNetwork(fEtwNetwork);
        }
    }

    if (SUCCEEDED(hr))
    {
        bool fEtwRegistry;
        hr = _GetBool(pXmlDoc, "//Profile/ETW/Registry", &fEtwRegistry);
        if (SUCCEEDED(hr) && (hr != S_FALSE))
        {
            pProfile->SetEtwEnabled(true);
            pProfile->SetEtwRegistry(fEtwRegistry);
        }
    }

    if (SUCCEEDED(hr))
    {
        bool fEtwUsePagedMemory;
        hr = _GetBool(pXmlDoc, "//Profile/ETW/UsePagedMemory", &fEtwUsePagedMemory);
        if (SUCCEEDED(hr) && (hr != S_FALSE))
        {
            pProfile->SetEtwEnabled(true);
            pProfile->SetEtwUsePagedMemory(fEtwUsePagedMemory);
        }
    }

    if (SUCCEEDED(hr))
    {
        bool fEtwUsePerfTimer;
        hr = _GetBool(pXmlDoc, "//Profile/ETW/UsePerfTimer", &fEtwUsePerfTimer);
        if (SUCCEEDED(hr) && (hr != S_FALSE))
        {
            pProfile->SetEtwEnabled(true);
            pProfile->SetEtwUsePerfTimer(fEtwUsePerfTimer);
        }
    }

    if (SUCCEEDED(hr))
    {
        bool fEtwUseSystemTimer;
        hr = _GetBool(pXmlDoc, "//Profile/ETW/UseSystemTimer", &fEtwUseSystemTimer);
        if (SUCCEEDED(hr) && (hr != S_FALSE))
        {
            pProfile->SetEtwEnabled(true);
            pProfile->SetEtwUseSystemTimer(fEtwUseSystemTimer);
        }
    }

    if (SUCCEEDED(hr))
    {
        bool fEtwUseCyclesCounter;
        hr = _GetBool(pXmlDoc, "//Profile/ETW/UseCyclesCounter", &fEtwUseCyclesCounter);
        if (SUCCEEDED(hr) && (hr != S_FALSE))
        {
            pProfile->SetEtwEnabled(true);
            pProfile->SetEtwUseCyclesCounter(fEtwUseCyclesCounter);
        }
    }

    return hr;
}

HRESULT XmlProfileParser::_ParseTimeSpans(IXMLDOMDocument2 *pXmlDoc, Profile *pProfile)
{
    CComPtr<IXMLDOMNodeList> spNodeList = nullptr;
    CComVariant query("//Profile/TimeSpans/TimeSpan");
    HRESULT hr = pXmlDoc->selectNodes(query.bstrVal, &spNodeList);
    if (SUCCEEDED(hr))
    {
        long cNodes;
        hr = spNodeList->get_length(&cNodes);
        if (SUCCEEDED(hr))
        {
            for (int i = 0; i < cNodes; i++)
            {
                CComPtr<IXMLDOMNode> spNode = nullptr;
                hr = spNodeList->get_item(i, &spNode);
                if (SUCCEEDED(hr))
                {
                    TimeSpan timeSpan;
                    hr = _ParseTimeSpan(spNode, &timeSpan);
                    if (SUCCEEDED(hr))
                    {
                        pProfile->AddTimeSpan(timeSpan);
                    }
                }
            }
        }
    }

    return hr;
}

HRESULT XmlProfileParser::_ParseTimeSpan(IXMLDOMNode *pXmlNode, TimeSpan *pTimeSpan)
{
    UINT32 ulDuration;
    HRESULT hr = _GetUINT32(pXmlNode, "Duration", &ulDuration);
    if (SUCCEEDED(hr) && (hr != S_FALSE))
    {
        pTimeSpan->SetDuration(ulDuration);
    }

    if (SUCCEEDED(hr))
    {
        UINT32 ulWarmup;
        hr = _GetUINT32(pXmlNode, "Warmup", &ulWarmup);
        if (SUCCEEDED(hr) && (hr != S_FALSE))
        {
            pTimeSpan->SetWarmup(ulWarmup);
        }
    }

    if (SUCCEEDED(hr))
    {
        UINT32 ulCooldown;
        hr = _GetUINT32(pXmlNode, "Cooldown", &ulCooldown);
        if (SUCCEEDED(hr) && (hr != S_FALSE))
        {
            pTimeSpan->SetCooldown(ulCooldown);
        }
    }

    if (SUCCEEDED(hr))
    {
        UINT32 ulRandSeed;
        hr = _GetUINT32(pXmlNode, "RandSeed", &ulRandSeed);
        if (SUCCEEDED(hr) && (hr != S_FALSE))
        {
            pTimeSpan->SetRandSeed(ulRandSeed);
        }
    }

    if (SUCCEEDED(hr))
    {
        UINT32 ulThreadCount;
        hr = _GetUINT32(pXmlNode, "ThreadCount", &ulThreadCount);
        if (SUCCEEDED(hr) && (hr != S_FALSE))
        {
            pTimeSpan->SetThreadCount(ulThreadCount);
        }
    }

    if (SUCCEEDED(hr))
    {
        bool fGroupAffinity;
        hr = _GetBool(pXmlNode, "GroupAffinity", &fGroupAffinity);
        if (SUCCEEDED(hr) && (hr != S_FALSE))
        {
            pTimeSpan->SetGroupAffinity(fGroupAffinity);
        }
    }

    if (SUCCEEDED(hr))
    {
        bool fDisableAffinity;
        hr = _GetBool(pXmlNode, "DisableAffinity", &fDisableAffinity);
        if (SUCCEEDED(hr) && (hr != S_FALSE))
        {
            pTimeSpan->SetDisableAffinity(fDisableAffinity);
        }
    }

    if (SUCCEEDED(hr))
    {
        bool fCompletionRoutines;
        hr = _GetBool(pXmlNode, "CompletionRoutines", &fCompletionRoutines);
        if (SUCCEEDED(hr) && (hr != S_FALSE))
        {
            pTimeSpan->SetCompletionRoutines(fCompletionRoutines);
        }
    }

    if (SUCCEEDED(hr))
    {
        bool fMeasureLatency;
        hr = _GetBool(pXmlNode, "MeasureLatency", &fMeasureLatency);
        if (SUCCEEDED(hr) && (hr != S_FALSE))
        {
            pTimeSpan->SetMeasureLatency(fMeasureLatency);
        }
    }

    if (SUCCEEDED(hr))
    {
        bool fCalculateIopsStdDev;
        hr = _GetBool(pXmlNode, "CalculateIopsStdDev", &fCalculateIopsStdDev);
        if (SUCCEEDED(hr) && (hr != S_FALSE))
        {
            pTimeSpan->SetCalculateIopsStdDev(fCalculateIopsStdDev);
        }
    }

    if (SUCCEEDED(hr))
    {
        UINT32 ulIoBucketDuration;
        hr = _GetUINT32(pXmlNode, "IoBucketDuration", &ulIoBucketDuration);
        if (SUCCEEDED(hr) && (hr != S_FALSE))
        {
            pTimeSpan->SetIoBucketDurationInMilliseconds(ulIoBucketDuration);
        }
    }

    if (SUCCEEDED(hr))
    {
        hr = _ParseAffinityAssignment(pXmlNode, pTimeSpan);
    }

    if (SUCCEEDED(hr))
    {
        hr = _ParseTargets(pXmlNode, pTimeSpan);
    }
    return hr;
}

HRESULT XmlProfileParser::_ParseTargets(IXMLDOMNode *pXmlNode, TimeSpan *pTimeSpan)
{
    CComVariant query("Targets/Target");
    CComPtr<IXMLDOMNodeList> spNodeList;
    HRESULT hr = pXmlNode->selectNodes(query.bstrVal, &spNodeList);
    if (SUCCEEDED(hr))
    {
        long cNodes;
        hr = spNodeList->get_length(&cNodes);
        if (SUCCEEDED(hr))
        {
            for (int i = 0; i < cNodes; i++)
            {
                CComPtr<IXMLDOMNode> spNode;
                hr = spNodeList->get_item(i, &spNode);
                if (SUCCEEDED(hr))
                {
                    Target target;
                    _ParseTarget(spNode, &target);
                    pTimeSpan->AddTarget(target);
                }
            }
        }
    }
    return hr;
}

HRESULT XmlProfileParser::_ParseRandomDataSource(IXMLDOMNode *pXmlNode, Target *pTarget)
{
    CComPtr<IXMLDOMNodeList> spNodeList = nullptr;
    CComVariant query("RandomDataSource");
    HRESULT hr = pXmlNode->selectNodes(query.bstrVal, &spNodeList);
    if (SUCCEEDED(hr))
    {
        long cNodes;
        hr = spNodeList->get_length(&cNodes);
        if (SUCCEEDED(hr) && (cNodes == 1))
        {
            CComPtr<IXMLDOMNode> spNode = nullptr;
            hr = spNodeList->get_item(0, &spNode);
            if (SUCCEEDED(hr))
            {
                UINT64 cb;
                hr = _GetUINT64(spNode, "SizeInBytes", &cb);
                if (SUCCEEDED(hr) && (S_FALSE != hr))
                {
                    pTarget->SetRandomDataWriteBufferSize(cb);

                    string sPath;
                    hr = _GetString(spNode, "FilePath", &sPath);
                    if (SUCCEEDED(hr) && (S_FALSE != hr))
                    {
                        pTarget->SetRandomDataWriteBufferSourcePath(sPath);
                    }
                }
            }
        }
    }
    return hr;
}

HRESULT XmlProfileParser::_ParseWriteBufferContent(IXMLDOMNode *pXmlNode, Target *pTarget)
{
    CComPtr<IXMLDOMNodeList> spNodeList = nullptr;
    CComVariant query("WriteBufferContent");
    HRESULT hr = pXmlNode->selectNodes(query.bstrVal, &spNodeList);
    if (SUCCEEDED(hr))
    {
        long cNodes;
        hr = spNodeList->get_length(&cNodes);
        if (SUCCEEDED(hr) && (cNodes == 1))
        {
            CComPtr<IXMLDOMNode> spNode = nullptr;
            hr = spNodeList->get_item(0, &spNode);
            if (SUCCEEDED(hr))
            {
                string sPattern;
                hr = _GetString(spNode, "Pattern", &sPattern);
                if (SUCCEEDED(hr) && (hr != S_FALSE))
                {
                    if (sPattern == "sequential")
                    {
                        // that's the default option - do nothing
                    }
                    else if (sPattern == "zero")
                    {
                        pTarget->SetZeroWriteBuffers(true);
                    }
                    else if (sPattern == "random")
                    {
                        hr = _ParseRandomDataSource(spNode, pTarget);
                    }
                    else
                    {
                        hr = E_INVALIDARG;
                    }
                }
            }
        }
    }
    return hr;
}

HRESULT XmlProfileParser::_ParseTarget(IXMLDOMNode *pXmlNode, Target *pTarget)
{
    string sPath;
    HRESULT hr = _GetString(pXmlNode, "Path", &sPath);
    if (SUCCEEDED(hr) && (hr != S_FALSE))
    {
        pTarget->SetPath(sPath);
    }

    if (SUCCEEDED(hr))
    {
        DWORD dwBlockSize;
        hr = _GetDWORD(pXmlNode, "BlockSize", &dwBlockSize);
        if (SUCCEEDED(hr) && (hr != S_FALSE))
        {
            pTarget->SetBlockSizeInBytes(dwBlockSize);
        }
    }

    if (SUCCEEDED(hr))
    {
        UINT64 ullStrideSize;
        hr = _GetUINT64(pXmlNode, "StrideSize", &ullStrideSize);
        if (SUCCEEDED(hr) && (hr != S_FALSE))
        {
            pTarget->SetStrideSizeInBytes(ullStrideSize);
            pTarget->SetEnableCustomStrideSize(true);
        }
    }

    if (SUCCEEDED(hr))
    {
        UINT64 ullBaseFileOffset;
        hr = _GetUINT64(pXmlNode, "BaseFileOffset", &ullBaseFileOffset);
        if (SUCCEEDED(hr) && (hr != S_FALSE))
        {
            pTarget->SetBaseFileOffsetInBytes(ullBaseFileOffset);
        }
    }

    if (SUCCEEDED(hr))
    {
        bool fSequentialScan;
        hr = _GetBool(pXmlNode, "SequentialScan", &fSequentialScan);
        if (SUCCEEDED(hr) && (hr != S_FALSE))
        {
            pTarget->SetSequentialScan(fSequentialScan);
        }
    }

    if (SUCCEEDED(hr))
    {
        bool fRandomAccess;
        hr = _GetBool(pXmlNode, "RandomAccess", &fRandomAccess);
        if (SUCCEEDED(hr) && (hr != S_FALSE))
        {
            pTarget->SetRandomAccess(fRandomAccess);
        }
    }

    if (SUCCEEDED(hr))
    {
        bool fUseLargePages;
        hr = _GetBool(pXmlNode, "UseLargePages", &fUseLargePages);
        if (SUCCEEDED(hr) && (hr != S_FALSE))
        {
            pTarget->SetUseLargePages(fUseLargePages);
        }
    }

    if (SUCCEEDED(hr))
    {
        DWORD dwRequestCount;
        hr = _GetDWORD(pXmlNode, "RequestCount", &dwRequestCount);
        if (SUCCEEDED(hr) && (hr != S_FALSE))
        {
            pTarget->SetRequestCount(dwRequestCount);
        }
    }

    if (SUCCEEDED(hr))
    {
        UINT64 ullRandom;
        hr = _GetUINT64(pXmlNode, "Random", &ullRandom);
        if (SUCCEEDED(hr) && (hr != S_FALSE))
        {
            pTarget->SetRandomAlignmentInBytes(ullRandom);
            pTarget->SetUseRandomAlignment(true);
        }
    }

    if (SUCCEEDED(hr))
    {
        bool fDisableOSCache;
        hr = _GetBool(pXmlNode, "DisableOSCache", &fDisableOSCache);
        if (SUCCEEDED(hr) && (hr != S_FALSE))
        {
            pTarget->SetDisableOSCache(fDisableOSCache);
        }
    }

    if (SUCCEEDED(hr))
    {
        bool fDisableAllCache;
        hr = _GetBool(pXmlNode, "DisableAllCache", &fDisableAllCache);
        if (SUCCEEDED(hr) && (hr != S_FALSE))
        {
            pTarget->SetDisableAllCache(fDisableAllCache);
        }
    }

    if (SUCCEEDED(hr))
    {
        hr = _ParseWriteBufferContent(pXmlNode, pTarget);
    }

    if (SUCCEEDED(hr))
    {
        DWORD dwBurstSize;
        hr = _GetDWORD(pXmlNode, "BurstSize", &dwBurstSize);
        if (SUCCEEDED(hr) && (hr != S_FALSE))
        {
            pTarget->SetBurstSize(dwBurstSize);
            pTarget->SetUseBurstSize(true);
        }
    }

    if (SUCCEEDED(hr))
    {
        DWORD dwThinkTime;
        hr = _GetDWORD(pXmlNode, "ThinkTime", &dwThinkTime);
        if (SUCCEEDED(hr) && (hr != S_FALSE))
        {
            pTarget->SetThinkTime(dwThinkTime);
            pTarget->SetEnableThinkTime(true);
        }
    }

    if (SUCCEEDED(hr))
    {
        DWORD dwThroughput;
        hr = _GetDWORD(pXmlNode, "Throughput", &dwThroughput);
        if (SUCCEEDED(hr) && (hr != S_FALSE))
        {
            pTarget->SetThroughput(dwThroughput);
        }
    }

    if (SUCCEEDED(hr))
    {
        DWORD dwThreadsPerFile;
        hr = _GetDWORD(pXmlNode, "ThreadsPerFile", &dwThreadsPerFile);
        if (SUCCEEDED(hr) && (hr != S_FALSE))
        {
            pTarget->SetThreadsPerFile(dwThreadsPerFile);
        }
    }

    if (SUCCEEDED(hr))
    {
        UINT64 ullFileSize;
        hr = _GetUINT64(pXmlNode, "FileSize", &ullFileSize);
        if (SUCCEEDED(hr) && (hr != S_FALSE))
        {
            pTarget->SetFileSize(ullFileSize);
            pTarget->SetCreateFile(true);
        }
    }

    if (SUCCEEDED(hr))
    {
        UINT64 ullMaxFileSize;
        hr = _GetUINT64(pXmlNode, "MaxFileSize", &ullMaxFileSize);
        if (SUCCEEDED(hr) && (hr != S_FALSE))
        {
            pTarget->SetMaxFileSize(ullMaxFileSize);
        }
    }

    if (SUCCEEDED(hr))
    {
        UINT32 ulWriteRatio;
        hr = _GetUINT32(pXmlNode, "WriteRatio", &ulWriteRatio);
        if (SUCCEEDED(hr) && (hr != S_FALSE))
        {
            pTarget->SetWriteRatio(ulWriteRatio);
        }
    }

    if (SUCCEEDED(hr))
    {
        bool fParallelAsyncIO;
        hr = _GetBool(pXmlNode, "ParallelAsyncIO", &fParallelAsyncIO);
        if (SUCCEEDED(hr) && (hr != S_FALSE))
        {
            pTarget->SetUseParallelAsyncIO(fParallelAsyncIO);
        }
    }

    if (SUCCEEDED(hr))
    {
        UINT64 ullThreadStride;
        hr = _GetUINT64(pXmlNode, "ThreadStride", &ullThreadStride);
        if (SUCCEEDED(hr) && (hr != S_FALSE))
        {
            pTarget->SetThreadStrideInBytes(ullThreadStride);
        }
    }

    if (SUCCEEDED(hr))
    {
        UINT32 ulIOPriority;
        hr = _GetUINT32(pXmlNode, "IOPriority", &ulIOPriority);
        if (SUCCEEDED(hr) && (hr != S_FALSE))
        {
            PRIORITY_HINT hint[] = { IoPriorityHintVeryLow, IoPriorityHintLow, IoPriorityHintNormal };
            pTarget->SetIOPriorityHint(hint[ulIOPriority - 1]);
        }
    }
    return hr;
}

HRESULT XmlProfileParser::_ParseAffinityAssignment(IXMLDOMNode *pXmlNode, TimeSpan *pTimeSpan)
{
    CComPtr<IXMLDOMNodeList> spNodeList = nullptr;
    CComVariant query("Affinity/AffinityAssignment");
    HRESULT hr = pXmlNode->selectNodes(query.bstrVal, &spNodeList);
    if (SUCCEEDED(hr))
    {
        long cNodes;
        hr = spNodeList->get_length(&cNodes);
        if (SUCCEEDED(hr))
        {
            for (int i = 0; i < cNodes; i++)
            {
                CComPtr<IXMLDOMNode> spNode = nullptr;
                hr = spNodeList->get_item(i, &spNode);
                if (SUCCEEDED(hr))
                {
                    BSTR bstrText;
                    hr = spNode->get_text(&bstrText);
                    if (SUCCEEDED(hr))
                    {
                        pTimeSpan->AddAffinityAssignment(_wtoi((wchar_t *)bstrText));   // TODO: change to unsigned
                        SysFreeString(bstrText);
                    }
                }
            }
        }
    }
    return hr;
}

HRESULT XmlProfileParser::_GetUINT32(IXMLDOMNode *pXmlNode, const char *pszQuery, UINT32 *pulValue) const
{
    CComPtr<IXMLDOMNode> spNode = nullptr;
    CComVariant query(pszQuery);
    HRESULT hr = pXmlNode->selectSingleNode(query.bstrVal, &spNode);
    if (SUCCEEDED(hr) && (hr != S_FALSE))
    {
        BSTR bstrText;
        hr = spNode->get_text(&bstrText);
        if (SUCCEEDED(hr))
        {
            *pulValue = _wtoi((wchar_t *)bstrText);  // TODO: make sure it works on large unsigned ints
            SysFreeString(bstrText);
        }
    }
    return hr;
}

HRESULT XmlProfileParser::_GetString(IXMLDOMNode *pXmlNode, const char *pszQuery, string *psValue) const
{
    CComPtr<IXMLDOMNode> spNode = nullptr;
    CComVariant query(pszQuery);
    HRESULT hr = pXmlNode->selectSingleNode(query.bstrVal, &spNode);
    if (SUCCEEDED(hr) && (hr != S_FALSE))
    {
        BSTR bstrText;
        hr = spNode->get_text(&bstrText);
        if (SUCCEEDED(hr))
        {
            // TODO: use wstring?
            char path[MAX_PATH] = {};
            WideCharToMultiByte(CP_UTF8, 0 /*dwFlags*/, (wchar_t *)bstrText, static_cast<int>(wcslen((wchar_t *)bstrText)), path, sizeof(path)-1, 0 /*lpDefaultChar*/, 0 /*lpUsedDefaultChar*/);
            *psValue = string(path);
        }
        SysFreeString(bstrText);
    }
    return hr;
}

HRESULT XmlProfileParser::_GetUINT64(IXMLDOMNode *pXmlNode, const char *pszQuery, UINT64 *pullValue) const
{
    CComPtr<IXMLDOMNode> spNode = nullptr;
    CComVariant query(pszQuery);
    HRESULT hr = pXmlNode->selectSingleNode(query.bstrVal, &spNode);
    if (SUCCEEDED(hr) && (hr != S_FALSE))
    {
        BSTR bstrText;
        hr = spNode->get_text(&bstrText);
        if (SUCCEEDED(hr))
        {
            *pullValue = _wtoi64((wchar_t *)bstrText);  // TODO: make sure it works on large unsigned ints
        }
        SysFreeString(bstrText);
    }
    return hr;
}

HRESULT XmlProfileParser::_GetDWORD(IXMLDOMNode *pXmlNode, const char *pszQuery, DWORD *pdwValue) const
{
    UINT32 value = 0;
    HRESULT hr = _GetUINT32(pXmlNode, pszQuery, &value);
    if (SUCCEEDED(hr))
    {
        *pdwValue = value;
    }
    return hr;
}

HRESULT XmlProfileParser::_GetBool(IXMLDOMNode *pXmlNode, const char *pszQuery, bool *pfValue) const
{
    HRESULT hr = S_OK;
    CComPtr<IXMLDOMNode> spNode = nullptr;
    CComVariant query(pszQuery);
    hr = pXmlNode->selectSingleNode(query.bstrVal, &spNode);
    if (SUCCEEDED(hr) && (hr != S_FALSE))
    {
        BSTR bstrText;
        hr = spNode->get_text(&bstrText);
        if (SUCCEEDED(hr))
        {
            *pfValue = (_wcsicmp(L"true", (wchar_t *)bstrText) == 0);
            SysFreeString(bstrText);
        }
    }
    return hr;
}

HRESULT XmlProfileParser::_GetVerbose(IXMLDOMDocument2 *pXmlDoc, bool *pfVerbose)
{
    return _GetBool(pXmlDoc, "//Profile/Verbose", pfVerbose);
}

HRESULT XmlProfileParser::_GetProgress(IXMLDOMDocument2 *pXmlDoc, DWORD *pdwProgress)
{
    return _GetDWORD(pXmlDoc, "//Profile/Progress", pdwProgress);
}