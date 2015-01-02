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
    bool ParseFile(const char *pszPath, Profile *pProfile);

private:
    HRESULT _ParseEtw(IXMLDOMDocument2 &XmlDoc, Profile *pProfile);
    HRESULT _ParseTimeSpans(IXMLDOMDocument2 &XmlDoc, Profile *pProfile);
    HRESULT _ParseTimeSpan(IXMLDOMNode &XmlNode, TimeSpan *pTimeSpan);
    HRESULT _ParseTargets(IXMLDOMNode &XmlNode, TimeSpan *pTimeSpan);
    HRESULT _ParseRandomDataSource(IXMLDOMNode &XmlNode, Target *pTarget);
    HRESULT _ParseWriteBufferContent(IXMLDOMNode &XmlNode, Target *pTarget);
    HRESULT _ParseTarget(IXMLDOMNode &XmlNode, Target *pTarget);
    HRESULT _ParseAffinityAssignment(IXMLDOMNode &XmlNode, TimeSpan *pTimeSpan);

    HRESULT _GetString(IXMLDOMNode &XmlNode, const char *pszQuery, string *psValue) const;
    HRESULT _GetUINT32(IXMLDOMNode &XmlNode, const char *pszQuery, UINT32 *pulValue) const;
    HRESULT _GetUINT64(IXMLDOMNode &XmlNode, const char *pszQuery, UINT64 *pullValue) const;
    HRESULT _GetDWORD(IXMLDOMNode &XmlNode, const char *pszQuery, DWORD *pdwValue) const;
    HRESULT _GetBool(IXMLDOMNode &XmlNode, const char *pszQuery, bool *pfValue) const;

    HRESULT _GetVerbose(IXMLDOMDocument2 &XmlDoc, bool *pfVerbose);
    HRESULT _GetProgress(IXMLDOMDocument2 &XmlDoc, DWORD *pdwProgress);
};