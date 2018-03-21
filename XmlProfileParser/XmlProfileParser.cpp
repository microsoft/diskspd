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
#include <atlbase.h>
#include <cassert>

HRESULT ReportXmlError(
	const char *pszName,
	IXMLDOMParseError *pXMLError
	)
{
	long line;
	long linePos;
	long errorCode = E_FAIL;
	CComBSTR bReason;
	BSTR bstr;

	HRESULT hr = pXMLError->get_line(&line);
	if (FAILED(hr))
	{
		line = 0;
	}
	hr = pXMLError->get_linepos(&linePos);
	if (FAILED(hr))
	{
		linePos = 0;
	}
	hr = pXMLError->get_errorCode(&errorCode);
	if (FAILED(hr))
	{
		errorCode = E_FAIL;
	}
	hr = pXMLError->get_reason(&bstr);
	if (SUCCEEDED(hr))
	{
		bReason.Attach(bstr);
	}

	fprintf(stderr,
		"ERROR: failed to load %s, line %li, line position %li, errorCode %08lx\nERROR: reason: %S\n",
		pszName, line, linePos, errorCode, static_cast<PWCHAR>(bReason));

	return errorCode;
}

bool XmlProfileParser::ParseFile(const char *pszPath, Profile *pProfile)
{
    assert(pszPath != nullptr);
    assert(pProfile != nullptr);

    // import schema from the named resource
	HRSRC hSchemaXmlResource = FindResource(nullptr, L"DISKSPD.XSD", RT_HTML);
    assert(hSchemaXmlResource != nullptr);
	const HGLOBAL hSchemaXml = LoadResource(nullptr, hSchemaXmlResource);
    assert(hSchemaXml != nullptr);
	const auto pSchemaXml = LockResource(hSchemaXml);
    assert(pSchemaXml != nullptr);
    
    // convert from utf-8 produced by the xsd authoring tool to utf-16
	const int cchSchemaXml = MultiByteToWideChar(CP_UTF8, 0, static_cast<LPCSTR>(pSchemaXml), -1, nullptr, 0);
	vector<WCHAR> vWideSchemaXml(cchSchemaXml);
	const int dwcchWritten = MultiByteToWideChar(CP_UTF8, 0, static_cast<LPCSTR>(pSchemaXml), -1, vWideSchemaXml.data(), cchSchemaXml);
    UNREFERENCED_PARAMETER(dwcchWritten);
    assert(dwcchWritten == cchSchemaXml);
    // ... and finally, packed in a bstr for the loadXml interface
    CComBSTR bSchemaXml(vWideSchemaXml.data());

    bool fComInitialized = false;
    HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    if (SUCCEEDED(hr))
    {
        fComInitialized = true;
        CComPtr<IXMLDOMDocument2> spXmlDoc = nullptr;
        CComPtr<IXMLDOMDocument2> spXmlSchema = nullptr;
        CComPtr<IXMLDOMSchemaCollection2> spXmlSchemaColl = nullptr;
        CComPtr<IXMLDOMParseError> spXmlParseError = nullptr;

        // create com objects and decorate
        hr = CoCreateInstance(__uuidof(DOMDocument60), nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&spXmlSchema));
        if (SUCCEEDED(hr))
        {
            hr = spXmlSchema->put_async(VARIANT_FALSE);
        }
        if (SUCCEEDED(hr))
        {
            hr = spXmlSchema->setProperty(CComBSTR("ProhibitDTD"), CComVariant(VARIANT_FALSE));
        }
        if (SUCCEEDED(hr))
        {
            hr = CoCreateInstance(__uuidof(XMLSchemaCache60), nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&spXmlSchemaColl));
        }
        if (SUCCEEDED(hr))
        {
            hr = spXmlSchemaColl->put_validateOnLoad(VARIANT_TRUE);
        }
        if (SUCCEEDED(hr))
        {
            hr = CoCreateInstance(__uuidof(DOMDocument60), nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&spXmlDoc));
        }
        if (SUCCEEDED(hr))
        {
            hr = spXmlDoc->put_async(VARIANT_FALSE);
        }
        if (SUCCEEDED(hr))
        {
            hr = spXmlDoc->put_validateOnParse(VARIANT_TRUE);
        }
        // work in progress to complete XML schema validation
        // load schema and attach to schema collection, attach schema collection to spec doc, then load specification
#if 0
		//
		// Issue at the moment: load fails with error as follows.
		// ERROR: failed to load schema, line 1, line position 1, errorCode c00ce556
		// ERROR: reason : Invalid at the top level of the document.
        if (SUCCEEDED(hr))
        {
            VARIANT_BOOL fvIsOk;
            hr = spXmlSchema->loadXML(bSchemaXml.GetBSTR(), &fvIsOk);
            if (SUCCEEDED(hr) && fvIsOk != VARIANT_TRUE)
            {
                hr = spXmlSchema->get_parseError(&spXmlParseError);
                if (SUCCEEDED(hr))
                {
                    ReportXmlError("schema", spXmlParseError);
                }
                hr = E_FAIL;
            }
        }
		if (SUCCEEDED(hr))
        {
            CComVariant vXmlSchema(spXmlSchema);
            CComBSTR bNull("");
            hr = spXmlSchemaColl->add(bNull, vXmlSchema);
        }
        if (SUCCEEDED(hr))
        {
            CComVariant vSchemaCache(spXmlSchemaColl);
            hr = spXmlDoc->putref_schemas(vSchemaCache);
        }
#endif
        if (SUCCEEDED(hr))
        {
            VARIANT_BOOL fvIsOk;
	        const CComVariant vPath(pszPath);
            hr = spXmlDoc->load(vPath, &fvIsOk);
            if (SUCCEEDED(hr) && fvIsOk != VARIANT_TRUE)
            {
                hr = E_FAIL;
            }
        }

        // now parse the specification, if correct
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
	const CComVariant query("//Profile/TimeSpans/TimeSpan");
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
        bool fRandomWriteData;
        hr = _GetBool(pXmlNode, "RandomWriteData", &fRandomWriteData);
        if (SUCCEEDED(hr) && (hr != S_FALSE))
        {
            pTimeSpan->SetRandomWriteData(fRandomWriteData);
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
        UINT32 ulRequestCount;
        hr = _GetUINT32(pXmlNode, "RequestCount", &ulRequestCount);
        if (SUCCEEDED(hr) && (hr != S_FALSE))
        {
            pTimeSpan->SetRequestCount(ulRequestCount);
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

    // Look for downlevel non-group aware assignment
    if (SUCCEEDED(hr))
    {
        hr = _ParseAffinityAssignment(pXmlNode, pTimeSpan);
    }

    // Look for uplevel group aware assignment.
    if (SUCCEEDED(hr))
    {
        hr = _ParseAffinityGroupAssignment(pXmlNode, pTimeSpan);
    }

    if (SUCCEEDED(hr))
    {
        hr = _ParseTargets(pXmlNode, pTimeSpan);
    }
    return hr;
}

HRESULT XmlProfileParser::_ParseTargets(IXMLDOMNode *pXmlNode, TimeSpan *pTimeSpan)
{
	const CComVariant query("Targets/Target");
    CComPtr<IXMLDOMNodeList> spNodeList = nullptr;
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
	const CComVariant query("RandomDataSource");
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
	const CComVariant query("WriteBufferContent");
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
            pTarget->SetBlockAlignmentInBytes(ullStrideSize);
        }
    }
    
    if (SUCCEEDED(hr))
    {
        bool fInterlockedSequential;
        hr = _GetBool(pXmlNode, "InterlockedSequential", &fInterlockedSequential);
        if (SUCCEEDED(hr) && (hr != S_FALSE))
        {
            pTarget->SetUseInterlockedSequential(fInterlockedSequential);
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
        bool fBool;
        hr = _GetBool(pXmlNode, "SequentialScan", &fBool);
        if (SUCCEEDED(hr) && (hr != S_FALSE))
        {
            pTarget->SetSequentialScanHint(fBool);
        }
    }

    if (SUCCEEDED(hr))
    {
        bool fBool;
        hr = _GetBool(pXmlNode, "RandomAccess", &fBool);
        if (SUCCEEDED(hr) && (hr != S_FALSE))
        {
            pTarget->SetRandomAccessHint(fBool);
        }
    }

    if (SUCCEEDED(hr))
    {
        bool fBool;
        hr = _GetBool(pXmlNode, "TemporaryFile", &fBool);
        if (SUCCEEDED(hr) && (hr != S_FALSE))
        {
            pTarget->SetTemporaryFileHint(fBool);
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
            pTarget->SetUseRandomAccessPattern(true);
            pTarget->SetBlockAlignmentInBytes(ullRandom);
        }
    }

    if (SUCCEEDED(hr))
    {
        bool fBool;
        hr = _GetBool(pXmlNode, "DisableOSCache", &fBool);
        if (SUCCEEDED(hr) && (hr != S_FALSE) && fBool)
        {
            pTarget->SetCacheMode(TargetCacheMode::DisableOSCache);
        }
    }

    if (SUCCEEDED(hr))
    {
        bool fBool;
        hr = _GetBool(pXmlNode, "DisableAllCache", &fBool);
        if (SUCCEEDED(hr) && (hr != S_FALSE) && fBool)
        {
            pTarget->SetCacheMode(TargetCacheMode::DisableOSCache);
            pTarget->SetWriteThroughMode(WriteThroughMode::On);
        }
    }

    if (SUCCEEDED(hr))
    {
        bool fBool;
        hr = _GetBool(pXmlNode, "DisableLocalCache", &fBool);
        if (SUCCEEDED(hr) && (hr != S_FALSE) && fBool)
        {
            pTarget->SetCacheMode(TargetCacheMode::DisableLocalCache);
        }
    }

    if (SUCCEEDED(hr))
    {
        bool fBool;
        hr = _GetBool(pXmlNode, "WriteThrough", &fBool);
        if (SUCCEEDED(hr) && (hr != S_FALSE) && fBool)
        {
            pTarget->SetWriteThroughMode(WriteThroughMode::On);
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

    if (SUCCEEDED(hr))
    {
        UINT32 ulWeight;
        hr = _GetUINT32(pXmlNode, "Weight", &ulWeight);
        if (SUCCEEDED(hr) && (hr != S_FALSE))
        {
            pTarget->SetWeight(ulWeight);
        }
    }

    if (SUCCEEDED(hr))
    {
        hr = _ParseThreadTargets(pXmlNode, pTarget);
    }
    return hr;
}

HRESULT XmlProfileParser::_ParseThreadTargets(IXMLDOMNode *pXmlNode, Target *pTarget)
{
	const CComVariant query("ThreadTarget");
    CComPtr<IXMLDOMNodeList> spNodeList = nullptr;
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
                    ThreadTarget threadTarget;
                    _ParseThreadTarget(spNode, &threadTarget);
                    pTarget->AddThreadTarget(threadTarget);
                }
            }
        }
    }
    return hr;
}

HRESULT XmlProfileParser::_ParseThreadTarget(IXMLDOMNode *pXmlNode, ThreadTarget *pThreadTarget)
{
    UINT32 ulThread;
    HRESULT hr = _GetUINT32(pXmlNode, "Thread", &ulThread);
    if (SUCCEEDED(hr) && (hr != S_FALSE))
    {
        pThreadTarget->SetThread(ulThread);
    }

    if (SUCCEEDED(hr))
    {
        UINT32 ulWeight;
        hr = _GetUINT32(pXmlNode, "Weight", &ulWeight);
        if (SUCCEEDED(hr) && (hr != S_FALSE))
        {
            pThreadTarget->SetWeight(ulWeight);
        }
    }
    return hr;
}

// Compatibility with the old, non-group aware affinity assignment. Preserved to allow downlevel XML profiles
// to run without modification.
// Any assignment done through this method will only assign within group 0, and is equivalent to the non-group
// specification -a#,#,# (contrast to -ag#,#,#,...). While not strictly equivalent to the old non-group aware
// behavior, this should be acceptably good-enough.
//
// The XML result parser no longer emits this form.

HRESULT XmlProfileParser::_ParseAffinityAssignment(IXMLDOMNode *pXmlNode, TimeSpan *pTimeSpan)
{
    CComPtr<IXMLDOMNodeList> spNodeList = nullptr;
	const CComVariant query("Affinity/AffinityAssignment");
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
                        pTimeSpan->AddAffinityAssignment(static_cast<WORD>(0), static_cast<BYTE>(_wtoi(static_cast<wchar_t *>(bstrText))));
                        SysFreeString(bstrText);
                    }
                }
            }
        }
    }
    return hr;
}

// Group aware affinity assignment. This is the only form emitted by the XML result parser.

HRESULT XmlProfileParser::_ParseAffinityGroupAssignment(IXMLDOMNode *pXmlNode, TimeSpan *pTimeSpan)
{
    CComPtr<IXMLDOMNodeList> spNodeList = nullptr;
	const CComVariant query("Affinity/AffinityGroupAssignment");

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
                    UINT32 dwGroup = 0, dwProc = 0;
                    hr = _GetUINT32Attr(spNode, "Group", &dwGroup);
                    if (SUCCEEDED(hr))
                    {
                        _GetUINT32Attr(spNode, "Processor", &dwProc); /* result unused */
                    }
                    if (SUCCEEDED(hr))
                    {
                        if (dwProc > MAXBYTE)
                        {
                            fprintf(stderr, "ERROR: profile specifies group assignment to core %u, out of range\n", dwProc);
                            hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
                        }
                        if (dwGroup > MAXWORD)
                        {
                            fprintf(stderr, "ERROR: profile specifies group assignment group %u, out of range\n", dwGroup);
                            hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
                        }

                        if (SUCCEEDED(hr)) {
                            pTimeSpan->AddAffinityAssignment(static_cast<WORD>(dwGroup), static_cast<BYTE>(dwProc));
                        }
                        
                    }
                }
            }
        }
    }
    return hr;
}

HRESULT XmlProfileParser::_GetUINT32(IXMLDOMNode *pXmlNode, const char *pszQuery, UINT32 *pulValue)
{
    CComPtr<IXMLDOMNode> spNode = nullptr;
	const CComVariant query(pszQuery);
    HRESULT hr = pXmlNode->selectSingleNode(query.bstrVal, &spNode);
    if (SUCCEEDED(hr) && (hr != S_FALSE))
    {
        BSTR bstrText;
        hr = spNode->get_text(&bstrText);
        if (SUCCEEDED(hr))
        {
            *pulValue = _wtoi(static_cast<wchar_t *>(bstrText));  // TODO: make sure it works on large unsigned ints
            SysFreeString(bstrText);
        }
    }
    return hr;
}

HRESULT XmlProfileParser::_GetUINT32Attr(IXMLDOMNode *pXmlNode, const char *pszAttr, UINT32 *pulValue)
{
    CComPtr<IXMLDOMNamedNodeMap> spNamedNodeMap = nullptr;
	const CComBSTR attr(pszAttr);
	const HRESULT hr = pXmlNode->get_attributes(&spNamedNodeMap);
    if (SUCCEEDED(hr) && (hr != S_FALSE))
    {
        CComPtr<IXMLDOMNode> spNode = nullptr;
        HRESULT hr2 = spNamedNodeMap->getNamedItem(attr, &spNode);
        if (SUCCEEDED(hr2) && (hr2 != S_FALSE))
        {
            BSTR bstrText;
            hr2 = spNode->get_text(&bstrText);
            if (SUCCEEDED(hr2))
            {
                *pulValue = _wtoi(static_cast<wchar_t *>(bstrText));  // TODO: make sure it works on large unsigned ints
                SysFreeString(bstrText);
            }
        }
    }
    return hr;
}

HRESULT XmlProfileParser::_GetString(IXMLDOMNode *pXmlNode, const char *pszQuery, string *psValue)
{
    CComPtr<IXMLDOMNode> spNode = nullptr;
	const CComVariant query(pszQuery);
    HRESULT hr = pXmlNode->selectSingleNode(query.bstrVal, &spNode);
    if (SUCCEEDED(hr) && (hr != S_FALSE))
    {
        BSTR bstrText;
        hr = spNode->get_text(&bstrText);
        if (SUCCEEDED(hr))
        {
            // TODO: use wstring?
            char path[MAX_PATH] = {};
            WideCharToMultiByte(CP_UTF8, 0 /*dwFlags*/, static_cast<wchar_t *>(bstrText),
                                static_cast<int>(wcslen(static_cast<wchar_t *>(bstrText))), path, sizeof(path) - 1,
                                nullptr /*lpDefaultChar*/, nullptr /*lpUsedDefaultChar*/);
            *psValue = string(path);
        }
        SysFreeString(bstrText);
    }
    return hr;
}

HRESULT XmlProfileParser::_GetUINT64(IXMLDOMNode *pXmlNode, const char *pszQuery, UINT64 *pullValue)
{
    CComPtr<IXMLDOMNode> spNode = nullptr;
	const CComVariant query(pszQuery);
    HRESULT hr = pXmlNode->selectSingleNode(query.bstrVal, &spNode);
    if (SUCCEEDED(hr) && (hr != S_FALSE))
    {
        BSTR bstrText;
        hr = spNode->get_text(&bstrText);
        if (SUCCEEDED(hr))
        {
            *pullValue = _wtoi64(static_cast<wchar_t *>(bstrText));  // TODO: make sure it works on large unsigned ints
        }
        SysFreeString(bstrText);
    }
    return hr;
}

HRESULT XmlProfileParser::_GetDWORD(IXMLDOMNode *pXmlNode, const char *pszQuery, DWORD *pdwValue)
{
    UINT32 value = 0;
	const HRESULT hr = _GetUINT32(pXmlNode, pszQuery, &value);
    if (SUCCEEDED(hr))
    {
        *pdwValue = value;
    }
    return hr;
}

HRESULT XmlProfileParser::_GetBool(IXMLDOMNode *pXmlNode, const char *pszQuery, bool *pfValue)
{
	CComPtr<IXMLDOMNode> spNode = nullptr;
	const CComVariant query(pszQuery);
    HRESULT hr = pXmlNode->selectSingleNode(query.bstrVal, &spNode);
    if (SUCCEEDED(hr) && (hr != S_FALSE))
    {
        BSTR bstrText;
        hr = spNode->get_text(&bstrText);
        if (SUCCEEDED(hr))
        {
            *pfValue = (_wcsicmp(L"true", static_cast<wchar_t *>(bstrText)) == 0);
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
