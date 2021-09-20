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
#include <MsXml6.h>
#include "Common.h"

class XmlProfileParser
{
public:
    bool ParseFile(const char *pszPath, Profile *pProfile, vector<Target> *pvSubstTargets, HMODULE hModule);

private:
    HRESULT _ParseEtw(IXMLDOMDocument2 *pXmlDoc, Profile *pProfile);
    HRESULT _ParseTimeSpans(IXMLDOMDocument2 *pXmlDoc, Profile *pProfile, vector<pair<string, bool>>& vSubsts);
    HRESULT _ParseTimeSpan(IXMLDOMNode *pXmlNode, TimeSpan *pTimeSpan, vector<pair<string, bool>>& vSubsts);
    HRESULT _ParseTargets(IXMLDOMNode *pXmlNode, TimeSpan *pTimeSpan, vector<pair<string, bool>>& vSubsts);
    HRESULT _ParseRandomDataSource(IXMLDOMNode *pXmlNode, Target *pTarget);
    HRESULT _ParseWriteBufferContent(IXMLDOMNode *pXmlNode, Target *pTarget);
    HRESULT _ParseTarget(IXMLDOMNode *pXmlNode, Target *pTarget);
    HRESULT _ParseThreadTargets(IXMLDOMNode *pXmlNode, Target *pTarget);
    HRESULT _ParseThroughput(IXMLDOMNode *pXmlNode, Target *pTarget);
    HRESULT _ParseThreadTarget(IXMLDOMNode *pXmlNode, ThreadTarget *pThreadTarget);
    HRESULT _ParseAffinityAssignment(IXMLDOMNode *pXmlNode, TimeSpan *pTimeSpan);
    HRESULT _ParseAffinityGroupAssignment(IXMLDOMNode *pXmlNode, TimeSpan *pTimeSpan);
    HRESULT _ParseDistribution(IXMLDOMNode *pXmlNode, Target *pTarget);
    HRESULT _SubstTarget(Target *pTarget, vector<pair<string, bool>>& vSubsts);

    HRESULT _GetString(IXMLDOMNode *pXmlNode, const char *pszQuery, string *psValue) const;
    HRESULT _GetUINT32(IXMLDOMNode *pXmlNode, const char *pszQuery, UINT32 *pulValue) const;
    HRESULT _GetUINT64(IXMLDOMNode *pXmlNode, const char *pszQuery, UINT64 *pullValue) const;
    HRESULT _GetDWORD(IXMLDOMNode *pXmlNode, const char *pszQuery, DWORD *pdwValue) const;
    HRESULT _GetBool(IXMLDOMNode *pXmlNode, const char *pszQuery, bool *pfValue) const;

    HRESULT _GetUINT32Attr(IXMLDOMNode *pXmlNode, const char *pszAttr, UINT32 *pulValue) const;
    
    HRESULT _GetVerbose(IXMLDOMDocument2 *pXmlDoc, bool *pfVerbose);
    HRESULT _GetProgress(IXMLDOMDocument2 *pXmlDoc, DWORD *pdwProgress);
};