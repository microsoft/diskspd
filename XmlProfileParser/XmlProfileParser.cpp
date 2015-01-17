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
#include <assert.h>

// the vc com headers define a partial set of smartptr typedefs, which unfortunately
// aren't 1) complete for our use case and 2) vary between revs of the headers.
// this define disables the automatic definitions, letting us do them ourselves.
#define _COM_NO_STANDARD_GUIDS_
#include <comdef.h>

_COM_SMARTPTR_TYPEDEF(IXMLDOMDocument2, __uuidof(IXMLDOMDocument2));
_COM_SMARTPTR_TYPEDEF(IXMLDOMNode, __uuidof(IXMLDOMNodeList));
_COM_SMARTPTR_TYPEDEF(IXMLDOMNodeList, __uuidof(IXMLDOMNodeList));

bool XmlProfileParser::ParseFile(const char *pszPath, Profile *pProfile)
{
    assert(pszPath != nullptr);
    assert(pProfile != nullptr);

    bool fComInitialized = false;
    HRESULT hr = CoInitialize(nullptr);
    if (SUCCEEDED(hr))
    {
        fComInitialized = true;
        IXMLDOMDocument2Ptr spXmlDoc;
        hr = CoCreateInstance(__uuidof(DOMDocument60), nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&spXmlDoc));
        if (SUCCEEDED(hr))
        {
            hr = spXmlDoc->put_async(VARIANT_FALSE);
            if (SUCCEEDED(hr))
            {
                VARIANT_BOOL fvIsOk;
                _variant_t vPath(pszPath);
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

HRESULT XmlProfileParser::_ParseEtw(IXMLDOMDocument2 &XmlDoc, Profile *pProfile)
{
    bool fEtwProcess;
    HRESULT hr = _GetBool(XmlDoc, "//Profile/ETW/Process", &fEtwProcess);
    if (SUCCEEDED(hr) && (hr != S_FALSE))
    {
        pProfile->SetEtwEnabled(true);
        pProfile->SetEtwProcess(fEtwProcess);
    }

    if (SUCCEEDED(hr))
    {
        bool fEtwThread;
        hr = _GetBool(XmlDoc, "//Profile/ETW/Thread", &fEtwThread);
        if (SUCCEEDED(hr) && (hr != S_FALSE))
        {
            pProfile->SetEtwEnabled(true);
            pProfile->SetEtwThread(fEtwThread);
        }
    }

    if (SUCCEEDED(hr))
    {
        bool fEtwImageLoad;
        hr = _GetBool(XmlDoc, "//Profile/ETW/ImageLoad", &fEtwImageLoad);
        if (SUCCEEDED(hr) && (hr != S_FALSE))
        {
            pProfile->SetEtwEnabled(true);
            pProfile->SetEtwImageLoad(fEtwImageLoad);
        }
    }

    if (SUCCEEDED(hr))
    {
        bool fEtwDiskIO;
        hr = _GetBool(XmlDoc, "//Profile/ETW/DiskIO", &fEtwDiskIO);
        if (SUCCEEDED(hr) && (hr != S_FALSE))
        {
            pProfile->SetEtwEnabled(true);
            pProfile->SetEtwDiskIO(fEtwDiskIO);
        }
    }

    if (SUCCEEDED(hr))
    {
        bool fEtwMemoryPageFaults;
        hr = _GetBool(XmlDoc, "//Profile/ETW/MemoryPageFaults", &fEtwMemoryPageFaults);
        if (SUCCEEDED(hr) && (hr != S_FALSE))
        {
            pProfile->SetEtwEnabled(true);
            pProfile->SetEtwMemoryPageFaults(fEtwMemoryPageFaults);
        }
    }

    if (SUCCEEDED(hr))
    {
        bool fEtwMemoryHardFaults;
        hr = _GetBool(XmlDoc, "//Profile/ETW/MemoryHardFaults", &fEtwMemoryHardFaults);
        if (SUCCEEDED(hr) && (hr != S_FALSE))
        {
            pProfile->SetEtwEnabled(true);
            pProfile->SetEtwMemoryHardFaults(fEtwMemoryHardFaults);
        }
    }

    if (SUCCEEDED(hr))
    {
        bool fEtwNetwork;
        hr = _GetBool(XmlDoc, "//Profile/ETW/Network", &fEtwNetwork);
        if (SUCCEEDED(hr) && (hr != S_FALSE))
        {
            pProfile->SetEtwEnabled(true);
            pProfile->SetEtwNetwork(fEtwNetwork);
        }
    }

    if (SUCCEEDED(hr))
    {
        bool fEtwRegistry;
        hr = _GetBool(XmlDoc, "//Profile/ETW/Registry", &fEtwRegistry);
        if (SUCCEEDED(hr) && (hr != S_FALSE))
        {
            pProfile->SetEtwEnabled(true);
            pProfile->SetEtwRegistry(fEtwRegistry);
        }
    }

    if (SUCCEEDED(hr))
    {
        bool fEtwUsePagedMemory;
        hr = _GetBool(XmlDoc, "//Profile/ETW/UsePagedMemory", &fEtwUsePagedMemory);
        if (SUCCEEDED(hr) && (hr != S_FALSE))
        {
            pProfile->SetEtwEnabled(true);
            pProfile->SetEtwUsePagedMemory(fEtwUsePagedMemory);
        }
    }

    if (SUCCEEDED(hr))
    {
        bool fEtwUsePerfTimer;
        hr = _GetBool(XmlDoc, "//Profile/ETW/UsePerfTimer", &fEtwUsePerfTimer);
        if (SUCCEEDED(hr) && (hr != S_FALSE))
        {
            pProfile->SetEtwEnabled(true);
            pProfile->SetEtwUsePerfTimer(fEtwUsePerfTimer);
        }
    }

    if (SUCCEEDED(hr))
    {
        bool fEtwUseSystemTimer;
        hr = _GetBool(XmlDoc, "//Profile/ETW/UseSystemTimer", &fEtwUseSystemTimer);
        if (SUCCEEDED(hr) && (hr != S_FALSE))
        {
            pProfile->SetEtwEnabled(true);
            pProfile->SetEtwUseSystemTimer(fEtwUseSystemTimer);
        }
    }

    if (SUCCEEDED(hr))
    {
        bool fEtwUseCyclesCounter;
        hr = _GetBool(XmlDoc, "//Profile/ETW/UseCyclesCounter", &fEtwUseCyclesCounter);
        if (SUCCEEDED(hr) && (hr != S_FALSE))
        {
            pProfile->SetEtwEnabled(true);
            pProfile->SetEtwUseCyclesCounter(fEtwUseCyclesCounter);
        }
    }

    return hr;
}

HRESULT XmlProfileParser::_ParseTimeSpans(IXMLDOMDocument2 &XmlDoc, Profile *pProfile)
{
    IXMLDOMNodeListPtr spNodeList;
    _variant_t query("//Profile/TimeSpans/TimeSpan");
    HRESULT hr = XmlDoc.selectNodes(query.bstrVal, &spNodeList);
    if (SUCCEEDED(hr))
    {
        long cNodes;
        hr = spNodeList->get_length(&cNodes);
        if (SUCCEEDED(hr))
        {
            for (int i = 0; i < cNodes; i++)
            {
                IXMLDOMNodePtr spNode;
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

HRESULT XmlProfileParser::_ParseTimeSpan(IXMLDOMNode &XmlNode, TimeSpan *pTimeSpan)
{
    UINT32 ulDuration;
    HRESULT hr = _GetUINT32(XmlNode, "Duration", &ulDuration);
    if (SUCCEEDED(hr) && (hr != S_FALSE))
    {
        pTimeSpan->SetDuration(ulDuration);
    }

    if (SUCCEEDED(hr))
    {
        UINT32 ulWarmup;
        hr = _GetUINT32(XmlNode, "Warmup", &ulWarmup);
        if (SUCCEEDED(hr) && (hr != S_FALSE))
        {
            pTimeSpan->SetWarmup(ulWarmup);
        }
    }

    if (SUCCEEDED(hr))
    {
        UINT32 ulCooldown;
        hr = _GetUINT32(XmlNode, "Cooldown", &ulCooldown);
        if (SUCCEEDED(hr) && (hr != S_FALSE))
        {
            pTimeSpan->SetCooldown(ulCooldown);
        }
    }

    if (SUCCEEDED(hr))
    {
        UINT32 ulRandSeed;
        hr = _GetUINT32(XmlNode, "RandSeed", &ulRandSeed);
        if (SUCCEEDED(hr) && (hr != S_FALSE))
        {
            pTimeSpan->SetRandSeed(ulRandSeed);
        }
    }

    if (SUCCEEDED(hr))
    {
        UINT32 ulThreadCount;
        hr = _GetUINT32(XmlNode, "ThreadCount", &ulThreadCount);
        if (SUCCEEDED(hr) && (hr != S_FALSE))
        {
            pTimeSpan->SetThreadCount(ulThreadCount);
        }
    }

    if (SUCCEEDED(hr))
    {
        bool fGroupAffinity;
        hr = _GetBool(XmlNode, "GroupAffinity", &fGroupAffinity);
        if (SUCCEEDED(hr) && (hr != S_FALSE))
        {
            pTimeSpan->SetGroupAffinity(fGroupAffinity);
        }
    }

    if (SUCCEEDED(hr))
    {
        bool fDisableAffinity;
        hr = _GetBool(XmlNode, "DisableAffinity", &fDisableAffinity);
        if (SUCCEEDED(hr) && (hr != S_FALSE))
        {
            pTimeSpan->SetDisableAffinity(fDisableAffinity);
        }
    }

    if (SUCCEEDED(hr))
    {
        bool fCompletionRoutines;
        hr = _GetBool(XmlNode, "CompletionRoutines", &fCompletionRoutines);
        if (SUCCEEDED(hr) && (hr != S_FALSE))
        {
            pTimeSpan->SetCompletionRoutines(fCompletionRoutines);
        }
    }

    if (SUCCEEDED(hr))
    {
        bool fMeasureLatency;
        hr = _GetBool(XmlNode, "MeasureLatency", &fMeasureLatency);
        if (SUCCEEDED(hr) && (hr != S_FALSE))
        {
            pTimeSpan->SetMeasureLatency(fMeasureLatency);
        }
    }

    if (SUCCEEDED(hr))
    {
        bool fCalculateIopsStdDev;
        hr = _GetBool(XmlNode, "CalculateIopsStdDev", &fCalculateIopsStdDev);
        if (SUCCEEDED(hr) && (hr != S_FALSE))
        {
            pTimeSpan->SetCalculateIopsStdDev(fCalculateIopsStdDev);
        }
    }

    if (SUCCEEDED(hr))
    {
        UINT32 ulIoBucketDuration;
        hr = _GetUINT32(XmlNode, "IoBucketDuration", &ulIoBucketDuration);
        if (SUCCEEDED(hr) && (hr != S_FALSE))
        {
            pTimeSpan->SetIoBucketDurationInMilliseconds(ulIoBucketDuration);
        }
    }

    if (SUCCEEDED(hr))
    {
        hr = _ParseAffinityAssignment(XmlNode, pTimeSpan);
    }

    if (SUCCEEDED(hr))
    {
        hr = _ParseTargets(XmlNode, pTimeSpan);
    }
    return hr;
}

HRESULT XmlProfileParser::_ParseTargets(IXMLDOMNode &XmlNode, TimeSpan *pTimeSpan)
{
    _variant_t query("Targets/Target");
    IXMLDOMNodeListPtr spNodeList;
    HRESULT hr = XmlNode.selectNodes(query.bstrVal, &spNodeList);
    if (SUCCEEDED(hr))
    {
        long cNodes;
        hr = spNodeList->get_length(&cNodes);
        if (SUCCEEDED(hr))
        {
            for (int i = 0; i < cNodes; i++)
            {
                IXMLDOMNodePtr spNode;
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

HRESULT XmlProfileParser::_ParseRandomDataSource(IXMLDOMNode &XmlNode, Target *pTarget)
{
    IXMLDOMNodeListPtr spNodeList;
    _variant_t query("RandomDataSource");
    HRESULT hr = XmlNode.selectNodes(query.bstrVal, &spNodeList);
    if (SUCCEEDED(hr))
    {
        long cNodes;
        hr = spNodeList->get_length(&cNodes);
        if (SUCCEEDED(hr) && (cNodes == 1))
        {
            IXMLDOMNodePtr spNode;
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

HRESULT XmlProfileParser::_ParseWriteBufferContent(IXMLDOMNode &XmlNode, Target *pTarget)
{
    IXMLDOMNodeListPtr spNodeList;
    _variant_t query("WriteBufferContent");
    HRESULT hr = XmlNode.selectNodes(query.bstrVal, &spNodeList);
    if (SUCCEEDED(hr))
    {
        long cNodes;
        hr = spNodeList->get_length(&cNodes);
        if (SUCCEEDED(hr) && (cNodes == 1))
        {
            IXMLDOMNodePtr spNode;
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

HRESULT XmlProfileParser::_ParseTarget(IXMLDOMNode &XmlNode, Target *pTarget)
{
    string sPath;
    HRESULT hr = _GetString(XmlNode, "Path", &sPath);
    if (SUCCEEDED(hr) && (hr != S_FALSE))
    {
        pTarget->SetPath(sPath);
    }

    if (SUCCEEDED(hr))
    {
        DWORD dwBlockSize;
        hr = _GetDWORD(XmlNode, "BlockSize", &dwBlockSize);
        if (SUCCEEDED(hr) && (hr != S_FALSE))
        {
            pTarget->SetBlockSizeInBytes(dwBlockSize);
        }
    }

    if (SUCCEEDED(hr))
    {
        UINT64 ullStrideSize;
        hr = _GetUINT64(XmlNode, "StrideSize", &ullStrideSize);
        if (SUCCEEDED(hr) && (hr != S_FALSE))
        {
            pTarget->SetBlockAlignmentInBytes(ullStrideSize);
        }
    }
    
    if (SUCCEEDED(hr))
    {
        bool fInterlockedSequential;
        hr = _GetBool(XmlNode, "InterlockedSequential", &fInterlockedSequential);
        if (SUCCEEDED(hr) && (hr != S_FALSE))
        {
            pTarget->SetUseInterlockedSequential(fInterlockedSequential);
        }
    }

    if (SUCCEEDED(hr))
    {
        UINT64 ullBaseFileOffset;
        hr = _GetUINT64(XmlNode, "BaseFileOffset", &ullBaseFileOffset);
        if (SUCCEEDED(hr) && (hr != S_FALSE))
        {
            pTarget->SetBaseFileOffsetInBytes(ullBaseFileOffset);
        }
    }

    if (SUCCEEDED(hr))
    {
        bool fSequentialScan;
        hr = _GetBool(XmlNode, "SequentialScan", &fSequentialScan);
        if (SUCCEEDED(hr) && (hr != S_FALSE))
        {
            pTarget->SetSequentialScanHint(fSequentialScan);
        }
    }

    if (SUCCEEDED(hr))
    {
        bool fRandomAccess;
        hr = _GetBool(XmlNode, "RandomAccess", &fRandomAccess);
        if (SUCCEEDED(hr) && (hr != S_FALSE))
        {
            pTarget->SetRandomAccessHint(fRandomAccess);
        }
    }

    if (SUCCEEDED(hr))
    {
        bool fUseLargePages;
        hr = _GetBool(XmlNode, "UseLargePages", &fUseLargePages);
        if (SUCCEEDED(hr) && (hr != S_FALSE))
        {
            pTarget->SetUseLargePages(fUseLargePages);
        }
    }

    if (SUCCEEDED(hr))
    {
        DWORD dwRequestCount;
        hr = _GetDWORD(XmlNode, "RequestCount", &dwRequestCount);
        if (SUCCEEDED(hr) && (hr != S_FALSE))
        {
            pTarget->SetRequestCount(dwRequestCount);
        }
    }

    if (SUCCEEDED(hr))
    {
        UINT64 ullRandom;
        hr = _GetUINT64(XmlNode, "Random", &ullRandom);
        if (SUCCEEDED(hr) && (hr != S_FALSE))
        {
            pTarget->SetUseRandomAccessPattern(true);
            pTarget->SetBlockAlignmentInBytes(ullRandom);
        }
    }

    if (SUCCEEDED(hr))
    {
        bool fDisableOSCache;
        hr = _GetBool(XmlNode, "DisableOSCache", &fDisableOSCache);
        if (SUCCEEDED(hr) && (hr != S_FALSE))
        {
            pTarget->SetDisableOSCache(fDisableOSCache);
        }
    }

    if (SUCCEEDED(hr))
    {
        bool fDisableAllCache;
        hr = _GetBool(XmlNode, "DisableAllCache", &fDisableAllCache);
        if (SUCCEEDED(hr) && (hr != S_FALSE))
        {
            pTarget->SetDisableAllCache(fDisableAllCache);
        }
    }

    if (SUCCEEDED(hr))
    {
        hr = _ParseWriteBufferContent(XmlNode, pTarget);
    }

    if (SUCCEEDED(hr))
    {
        DWORD dwBurstSize;
        hr = _GetDWORD(XmlNode, "BurstSize", &dwBurstSize);
        if (SUCCEEDED(hr) && (hr != S_FALSE))
        {
            pTarget->SetBurstSize(dwBurstSize);
            pTarget->SetUseBurstSize(true);
        }
    }

    if (SUCCEEDED(hr))
    {
        DWORD dwThinkTime;
        hr = _GetDWORD(XmlNode, "ThinkTime", &dwThinkTime);
        if (SUCCEEDED(hr) && (hr != S_FALSE))
        {
            pTarget->SetThinkTime(dwThinkTime);
            pTarget->SetEnableThinkTime(true);
        }
    }

    if (SUCCEEDED(hr))
    {
        DWORD dwThroughput;
        hr = _GetDWORD(XmlNode, "Throughput", &dwThroughput);
        if (SUCCEEDED(hr) && (hr != S_FALSE))
        {
            pTarget->SetThroughput(dwThroughput);
        }
    }

    if (SUCCEEDED(hr))
    {
        DWORD dwThreadsPerFile;
        hr = _GetDWORD(XmlNode, "ThreadsPerFile", &dwThreadsPerFile);
        if (SUCCEEDED(hr) && (hr != S_FALSE))
        {
            pTarget->SetThreadsPerFile(dwThreadsPerFile);
        }
    }

    if (SUCCEEDED(hr))
    {
        UINT64 ullFileSize;
        hr = _GetUINT64(XmlNode, "FileSize", &ullFileSize);
        if (SUCCEEDED(hr) && (hr != S_FALSE))
        {
            pTarget->SetFileSize(ullFileSize);
            pTarget->SetCreateFile(true);
        }
    }

    if (SUCCEEDED(hr))
    {
        UINT64 ullMaxFileSize;
        hr = _GetUINT64(XmlNode, "MaxFileSize", &ullMaxFileSize);
        if (SUCCEEDED(hr) && (hr != S_FALSE))
        {
            pTarget->SetMaxFileSize(ullMaxFileSize);
        }
    }

    if (SUCCEEDED(hr))
    {
        UINT32 ulWriteRatio;
        hr = _GetUINT32(XmlNode, "WriteRatio", &ulWriteRatio);
        if (SUCCEEDED(hr) && (hr != S_FALSE))
        {
            pTarget->SetWriteRatio(ulWriteRatio);
        }
    }

    if (SUCCEEDED(hr))
    {
        bool fParallelAsyncIO;
        hr = _GetBool(XmlNode, "ParallelAsyncIO", &fParallelAsyncIO);
        if (SUCCEEDED(hr) && (hr != S_FALSE))
        {
            pTarget->SetUseParallelAsyncIO(fParallelAsyncIO);
        }
    }

    if (SUCCEEDED(hr))
    {
        UINT64 ullThreadStride;
        hr = _GetUINT64(XmlNode, "ThreadStride", &ullThreadStride);
        if (SUCCEEDED(hr) && (hr != S_FALSE))
        {
            pTarget->SetThreadStrideInBytes(ullThreadStride);
        }
    }

    if (SUCCEEDED(hr))
    {
        UINT32 ulIOPriority;
        hr = _GetUINT32(XmlNode, "IOPriority", &ulIOPriority);
        if (SUCCEEDED(hr) && (hr != S_FALSE))
        {
            PRIORITY_HINT hint[] = { IoPriorityHintVeryLow, IoPriorityHintLow, IoPriorityHintNormal };
            pTarget->SetIOPriorityHint(hint[ulIOPriority - 1]);
        }
    }
    return hr;
}

HRESULT XmlProfileParser::_ParseAffinityAssignment(IXMLDOMNode &XmlNode, TimeSpan *pTimeSpan)
{
    IXMLDOMNodeListPtr spNodeList;
    _variant_t query("Affinity/AffinityAssignment");
    HRESULT hr = XmlNode.selectNodes(query.bstrVal, &spNodeList);
    if (SUCCEEDED(hr))
    {
        long cNodes;
        hr = spNodeList->get_length(&cNodes);
        if (SUCCEEDED(hr))
        {
            for (int i = 0; i < cNodes; i++)
            {
                IXMLDOMNodePtr spNode;
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

HRESULT XmlProfileParser::_GetUINT32(IXMLDOMNode &XmlNode, const char *pszQuery, UINT32 *pulValue) const
{
    IXMLDOMNodePtr spNode;
    _variant_t query(pszQuery);
    HRESULT hr = XmlNode.selectSingleNode(query.bstrVal, &spNode);
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

HRESULT XmlProfileParser::_GetString(IXMLDOMNode &XmlNode, const char *pszQuery, string *psValue) const
{
    IXMLDOMNodePtr spNode;
    _variant_t query(pszQuery);
    HRESULT hr = XmlNode.selectSingleNode(query.bstrVal, &spNode);
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

HRESULT XmlProfileParser::_GetUINT64(IXMLDOMNode &XmlNode, const char *pszQuery, UINT64 *pullValue) const
{
    IXMLDOMNodePtr spNode;
    _variant_t query(pszQuery);
    HRESULT hr = XmlNode.selectSingleNode(query.bstrVal, &spNode);
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

HRESULT XmlProfileParser::_GetDWORD(IXMLDOMNode &XmlNode, const char *pszQuery, DWORD *pdwValue) const
{
    UINT32 value = 0;
    HRESULT hr = _GetUINT32(XmlNode, pszQuery, &value);
    if (SUCCEEDED(hr))
    {
        *pdwValue = value;
    }
    return hr;
}

HRESULT XmlProfileParser::_GetBool(IXMLDOMNode &XmlNode, const char *pszQuery, bool *pfValue) const
{
    HRESULT hr = S_OK;
    IXMLDOMNodePtr spNode;
    _variant_t query(pszQuery);
    hr = XmlNode.selectSingleNode(query.bstrVal, &spNode);
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

HRESULT XmlProfileParser::_GetVerbose(IXMLDOMDocument2 &pXmlDoc, bool *pfVerbose)
{
    return _GetBool(pXmlDoc, "//Profile/Verbose", pfVerbose);
}

HRESULT XmlProfileParser::_GetProgress(IXMLDOMDocument2 &pXmlDoc, DWORD *pdwProgress)
{
    return _GetDWORD(pXmlDoc, "//Profile/Progress", pdwProgress);
}