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
#include <assert.h>

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
	HRESULT hr;

	hr = pXMLError->get_line(&line);
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
		"ERROR: failed to load %s, line %lu, line position %lu, errorCode %08x\nERROR: reason: %S\n",
		pszName, line, linePos, errorCode, (PWCHAR)bReason);

	return errorCode;
}

bool XmlProfileParser::ParseFile(const char *pszPath, Profile *pProfile, vector<Target> *pvSubstTargets, HMODULE hModule)
{
    assert(pszPath != nullptr);
    assert(pProfile != nullptr);

    // import schema from the named resource
    HRSRC hSchemaXmlResource = FindResource(hModule, L"DISKSPD.XSD", RT_HTML);
    assert(hSchemaXmlResource != NULL);
    HGLOBAL hSchemaXml = LoadResource(hModule, hSchemaXmlResource);
    assert(hSchemaXml != NULL);
    LPVOID pSchemaXml = LockResource(hSchemaXml);
    assert(pSchemaXml != NULL);
    
    // convert from utf-8 produced by the xsd authoring tool to utf-16
    int cchSchemaXml = MultiByteToWideChar(CP_UTF8, 0, (LPCSTR)pSchemaXml, -1, NULL, 0);
	vector<WCHAR> vWideSchemaXml(cchSchemaXml);
    int dwcchWritten = MultiByteToWideChar(CP_UTF8, 0, (LPCSTR)pSchemaXml, -1, vWideSchemaXml.data(), cchSchemaXml);
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
        if (SUCCEEDED(hr))
        {
            VARIANT_BOOL fvIsOk;
            hr = spXmlSchema->loadXML(bSchemaXml, &fvIsOk);
            if (FAILED(hr) || fvIsOk != VARIANT_TRUE)
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
        if (SUCCEEDED(hr))
        {
            VARIANT_BOOL fvIsOk;
            CComVariant vPath(pszPath);
            hr = spXmlDoc->load(vPath, &fvIsOk);
            if (FAILED(hr) || fvIsOk != VARIANT_TRUE)
            {
                hr = spXmlDoc->get_parseError(&spXmlParseError);
                if (SUCCEEDED(hr))
                {
                    ReportXmlError("profile", spXmlParseError);
                }
                hr = E_FAIL;
            }
        }

        //
        // XML has now passed basic schema validation. Bulld the target substitutions and parse the profile.
        //

        vector<pair<string, bool>> vSubsts;

        if (pvSubstTargets)
        {
            for (auto target : *pvSubstTargets)
            {
                vSubsts.emplace_back(make_pair(target.GetPath(), false));
            }
        }

        if (SUCCEEDED(hr))
        {
            bool b;
            hr = _GetBool(spXmlDoc, "//Profile/Verbose", &b);
            if (SUCCEEDED(hr) && (hr != S_FALSE))
            {
                pProfile->SetVerbose(b);
            }
        }

        if (SUCCEEDED(hr))
        {
            DWORD i;
            hr = _GetDWORD(spXmlDoc, "//Profile/Progress", &i);
            if (SUCCEEDED(hr) && (hr != S_FALSE))
            {
                pProfile->SetProgress(i);
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
            hr = _ParseTimeSpans(spXmlDoc, pProfile, vSubsts);
        }

        //
        // Error on unused substitutions - user should ensure these match up.
        //
        // Note that no (zero) substitutions are OK at the point of parsing, which allows
        // for -Rp forms on template profiles. Validation for executed profiles will occur
        // later during common validation.
        //
        // Generate an error for each unused substitution.
        //

        if (SUCCEEDED(hr))        
        {
            for (size_t i = 1; i <= vSubsts.size(); ++i)
            {
                if (!vSubsts[i - 1].second)
                {
                    fprintf(stderr, "ERROR: unused template target substitution _%u -> %s - check profile\n", (int) i, vSubsts[i - 1].first.c_str());
                    hr = E_INVALIDARG;
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

HRESULT XmlProfileParser::_ParseTimeSpans(IXMLDOMDocument2 *pXmlDoc, Profile *pProfile, vector<pair<string, bool>>& vSubsts)
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
                    hr = _ParseTimeSpan(spNode, &timeSpan, vSubsts);
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

HRESULT XmlProfileParser::_ParseTimeSpan(IXMLDOMNode *pXmlNode, TimeSpan *pTimeSpan, vector<pair<string, bool>>& vSubsts)
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
        hr = _ParseTargets(pXmlNode, pTimeSpan, vSubsts);
    }

    return hr;
}

HRESULT XmlProfileParser::_SubstTarget(Target *pTarget, vector<pair<string, bool>>& vSubsts)
{
    auto& sPath = pTarget->GetPath();

    if (sPath.length() >= 1 && sPath[0] == TEMPLATE_TARGET_PREFIX)
    {
        auto str = sPath.c_str();
        char *strend;
        ULONG i;

        //
        // Note that we will parse all template targets for correctness but allow empty substuttion lists
        // to pass through. If this profile will be executed, validation of paths will catch unsubst template targets
        //
        // strtoul will accept signed integers (e.g., -1), so we need to add our explicit assertion that this is indeed a digit.
        //

        i = strtoul(str + 1, &strend, 10);

        if (i == 0 || *strend != '\0' || !isdigit(*(str + 1)))
        {
            fprintf(stderr, "ERROR: template path '%s' is not a valid path reference - must be %c<integer> - check profile\n", str, TEMPLATE_TARGET_PREFIX);
            return E_INVALIDARG;
        }

        if (vSubsts.size() != 0)
        {
            if (i > vSubsts.size())
            {
                fprintf(stderr, "ERROR: template path '%s' does not have a specified substitution - check profile\n", str);
                return E_INVALIDARG;
            }

            //
            // Substitute this target, indicating this substitution was used (for validation).
            // Note 1 based count and 0 based index.
            //

            pTarget->SetPath(vSubsts[i - 1].first);
            vSubsts[i - 1].second = true;
        }
    }

    return S_OK;
}

HRESULT XmlProfileParser::_ParseTargets(IXMLDOMNode *pXmlNode, TimeSpan *pTimeSpan, vector<pair<string, bool>>& vSubsts)
{
    CComVariant query("Targets/Target");
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
                    hr = _ParseTarget(spNode, &target);
                    if (SUCCEEDED(hr))
                    {
                        hr = _SubstTarget(&target, vSubsts);
                    }
                    if (SUCCEEDED(hr))
                    {
                        pTimeSpan->AddTarget(target);    
                    }
                }

                if (!SUCCEEDED(hr))
                {
                    break;
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
    // For enforcement of sequential/random conflicts.
    // This is simplified for the XML since we control parse order.
    bool fSequential = false;

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
        UINT64 ullStrideSize;
        hr = _GetUINT64(pXmlNode, "StrideSize", &ullStrideSize);
        if (SUCCEEDED(hr) && (hr != S_FALSE))
        {
            pTarget->SetBlockAlignmentInBytes(ullStrideSize);
            fSequential = true;
        }
    }

    if (SUCCEEDED(hr))
    {
        UINT64 ullRandom;
        hr = _GetUINT64(pXmlNode, "Random", &ullRandom);
        if (SUCCEEDED(hr) && (hr != S_FALSE))
        {
            // conflict with sequential
            if (fSequential)
            {
                fprintf(stderr, "sequential <StrideSize> conflicts with <Random>\n");
                hr = HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER);
            }
            else
            {
                pTarget->SetRandomRatio(100);
                pTarget->SetBlockAlignmentInBytes(ullRandom);
            }
        }
    }

    // now override default of 100% random?
    if (SUCCEEDED(hr))
    {
        UINT32 ulRandomRatio;
        hr = _GetUINT32(pXmlNode, "RandomRatio", &ulRandomRatio);
        if (SUCCEEDED(hr) && (hr != S_FALSE))
        {
            // conflict with sequential
            if (fSequential)
            {
                fprintf(stderr, "sequential <StrideSize> conflicts with <RandomRatio>\n");
                hr = HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER);
            }
            else
            {
                pTarget->SetRandomRatio(ulRandomRatio);
            }
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
        hr = _GetBool(pXmlNode, "MemoryMappedIo", &fBool);
        if (SUCCEEDED(hr) && (hr != S_FALSE) && fBool)
        {
            pTarget->SetMemoryMappedIoMode(MemoryMappedIoMode::On);
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
        string sFlushType;
        hr = _GetString(pXmlNode, "FlushType", &sFlushType);
        if (SUCCEEDED(hr) && (hr != S_FALSE))
        {
            if (sFlushType == "ViewOfFile")
            {
                pTarget->SetMemoryMappedIoFlushMode(MemoryMappedIoFlushMode::ViewOfFile);
            }
            else if (sFlushType == "NonVolatileMemory")
            {
                pTarget->SetMemoryMappedIoFlushMode(MemoryMappedIoFlushMode::NonVolatileMemory);
            }
            else if (sFlushType == "NonVolatileMemoryNoDrain")
            {
                pTarget->SetMemoryMappedIoFlushMode(MemoryMappedIoFlushMode::NonVolatileMemoryNoDrain);
            }
            else
            {
                hr = E_INVALIDARG;
            }
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
        hr = _ParseThroughput(pXmlNode, pTarget);
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

    //
    // Note: XSD validation ensures only one type of distribution is specified, but it as simple
    // here to probe for each.
    //

    if  (SUCCEEDED(hr))
    {
        hr = _ParseDistribution(pXmlNode, pTarget);
    }

    if (SUCCEEDED(hr))
    {
        hr = _ParseThreadTargets(pXmlNode, pTarget);
    }
    return hr;
}

HRESULT XmlProfileParser::_ParseThroughput(IXMLDOMNode *pXmlNode, Target *pTarget)
{
    CComPtr<IXMLDOMNode> spNode = nullptr;
    CComVariant query("Throughput");
    HRESULT hr = pXmlNode->selectSingleNode(query.bstrVal, &spNode);
    if (SUCCEEDED(hr) && (hr != S_FALSE))
    {
        // get value
        UINT32 value = 0;

        BSTR bstrText;
        hr = spNode->get_text(&bstrText);
        if (SUCCEEDED(hr))
        {
            value = (UINT32) _wtoi64((wchar_t *)bstrText);  // XSD constrains s.t. cast is safe
            SysFreeString(bstrText);
        }
        else
        {
            return hr;
        }

        // get unit - bpms default
        bool isBpms = true;

        CComPtr<IXMLDOMNamedNodeMap> spNamedNodeMap = nullptr;
        CComBSTR attr("unit");
        hr = spNode->get_attributes(&spNamedNodeMap);
        if (SUCCEEDED(hr) && (hr != S_FALSE))
        {
            CComPtr<IXMLDOMNode> spAttrNode = nullptr;
            HRESULT hr = spNamedNodeMap->getNamedItem(attr, &spAttrNode);
            if (SUCCEEDED(hr) && (hr != S_FALSE))
            {
                BSTR bstrText;
                hr = spAttrNode->get_text(&bstrText);
                if (SUCCEEDED(hr))
                {
                    isBpms = wcscmp((wchar_t *)bstrText, L"IOPS");
                    SysFreeString(bstrText);
                }
            }
        }

        if (SUCCEEDED(hr) && (hr != S_FALSE))
        {
            if (isBpms)
            {
                pTarget->SetThroughput(value);
            }
            else
            {
                // NOTE: this depends on parse order s.t. blocksize is available
                pTarget->SetThroughputIOPS(value);
            }
        }
    }
    return hr;
}

HRESULT XmlProfileParser::_ParseThreadTargets(IXMLDOMNode *pXmlNode, Target *pTarget)
{
    CComVariant query("ThreadTargets/ThreadTarget");
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

struct {
    char* xPath;
    DistributionType t;
} distributionTypes[] = {
    { "Distribution/Absolute/Range",    DistributionType::Absolute },
    { "Distribution/Percent/Range",     DistributionType::Percent }
};

HRESULT XmlProfileParser::_ParseDistribution(IXMLDOMNode *pXmlNode, Target *pTarget)
{
    HRESULT hr = S_OK;

    for (auto& type : distributionTypes)
    {
        CComPtr<IXMLDOMNodeList> spNodeList = nullptr;
        CComVariant query(type.xPath);
        hr = pXmlNode->selectNodes(query.bstrVal, &spNodeList);
        if (SUCCEEDED(hr))
        {
            long cNodes;
            hr = spNodeList->get_length(&cNodes);
            if (SUCCEEDED(hr) && cNodes != 0)
            {
                UINT64 targetBase = 0, targetSpan;
                UINT32 ioBase = 0, ioSpan;
                vector<DistributionRange> v;

                for (int i = 0; i < cNodes; i++)
                {
                    // target span from the element
                    // note that this is the same 64bit int for both distribution types,
                    // it is the interpretation at the time the effective is calculated
                    // that makes the distinction. XSD covers range validations.
                    CComPtr<IXMLDOMNode> spNode = nullptr;
                    hr = spNodeList->get_item(i, &spNode);
                    if (SUCCEEDED(hr))
                    {
                        BSTR bstrText;
                        hr = spNode->get_text(&bstrText);
                        if (SUCCEEDED(hr))
                        {
                            targetSpan = _wtoi64((wchar_t *)bstrText);
                            SysFreeString(bstrText);
                        }
                    }

                    if (SUCCEEDED(hr))
                    {
                        // io span from the attribute
                        CComPtr<IXMLDOMNamedNodeMap> spNamedNodeMap = nullptr;
                        CComBSTR attr("IO");
                        hr = spNode->get_attributes(&spNamedNodeMap);
                        if (SUCCEEDED(hr) && (hr != S_FALSE))
                        {
                            CComPtr<IXMLDOMNode> spAttrNode = nullptr;
                            HRESULT hr = spNamedNodeMap->getNamedItem(attr, &spAttrNode);
                            if (SUCCEEDED(hr) && (hr != S_FALSE))
                            {
                                BSTR bstrText;
                                hr = spAttrNode->get_text(&bstrText);
                                if (SUCCEEDED(hr))
                                {
                                    ioSpan = _wtoi((wchar_t *)bstrText);
                                    SysFreeString(bstrText);
                                }
                            }
                        }
                    }

                    if (SUCCEEDED(hr) && (hr != S_FALSE))
                    {
                        v.emplace_back(ioBase, ioSpan,
                             make_pair(targetBase, targetSpan));
                        ioBase += ioSpan;
                        targetBase += targetSpan;
                    }
                    // failed during parse
                    else
                    {
                        break;
                    }

                    //
                    // Note that we are aware here whether we got to 100% IO specification.
                    // This validation is delayed to the common path for XML/cmdline.
                    //
                }

                if (SUCCEEDED(hr) && (hr != S_FALSE))
                {                    
                    pTarget->SetDistributionRange(v, type.t);
                }
                     
                // if we parsed into the element, we are done (success or failure) - only one type is possible.
                return hr;
            }
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
                        pTimeSpan->AddAffinityAssignment((WORD)0, (BYTE)_wtoi((wchar_t *)bstrText));
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
    CComVariant query("Affinity/AffinityGroupAssignment");

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
                        _GetUINT32Attr(spNode, "Processor", &dwProc);
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
                            pTimeSpan->AddAffinityAssignment((WORD)dwGroup, (BYTE)dwProc);
                        }
                        
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

HRESULT XmlProfileParser::_GetUINT32Attr(IXMLDOMNode *pXmlNode, const char *pszAttr, UINT32 *pulValue) const
{
    CComPtr<IXMLDOMNamedNodeMap> spNamedNodeMap = nullptr;
    CComBSTR attr(pszAttr);
    HRESULT hr = pXmlNode->get_attributes(&spNamedNodeMap);
    if (SUCCEEDED(hr) && (hr != S_FALSE))
    {
        CComPtr<IXMLDOMNode> spNode = nullptr;
        HRESULT hr = spNamedNodeMap->getNamedItem(attr, &spNode);
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
            *pullValue = _wtoi64((wchar_t *)bstrText);
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