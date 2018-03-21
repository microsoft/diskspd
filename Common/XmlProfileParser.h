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
	static bool ParseFile(const char *pszPath, Profile *pProfile);

private:
	static HRESULT _ParseEtw(IXMLDOMDocument2 *pXmlDoc, Profile *pProfile);
	static HRESULT _ParseTimeSpans(IXMLDOMDocument2 *pXmlDoc, Profile *pProfile);
	static HRESULT _ParseTimeSpan(IXMLDOMNode *pXmlNode, TimeSpan *pTimeSpan);
	static HRESULT _ParseTargets(IXMLDOMNode *pXmlNode, TimeSpan *pTimeSpan);
	static HRESULT _ParseRandomDataSource(IXMLDOMNode *pXmlNode, Target *pTarget);
	static HRESULT _ParseWriteBufferContent(IXMLDOMNode *pXmlNode, Target *pTarget);
	static HRESULT _ParseTarget(IXMLDOMNode *pXmlNode, Target *pTarget);
	static HRESULT _ParseThreadTargets(IXMLDOMNode *pXmlNode, Target *pTarget);
	static HRESULT _ParseThreadTarget(IXMLDOMNode *pXmlNode, ThreadTarget *pThreadTarget);
	static HRESULT _ParseAffinityAssignment(IXMLDOMNode *pXmlNode, TimeSpan *pTimeSpan);
	static HRESULT _ParseAffinityGroupAssignment(IXMLDOMNode *pXmlNode, TimeSpan *pTimeSpan);

	static HRESULT _GetString(IXMLDOMNode *pXmlNode, const char *pszQuery, string *psValue);
	static HRESULT _GetUINT32(IXMLDOMNode *pXmlNode, const char *pszQuery, UINT32 *pulValue);
	static HRESULT _GetUINT64(IXMLDOMNode *pXmlNode, const char *pszQuery, UINT64 *pullValue);
	static HRESULT _GetDWORD(IXMLDOMNode *pXmlNode, const char *pszQuery, DWORD *pdwValue);
	static HRESULT _GetBool(IXMLDOMNode *pXmlNode, const char *pszQuery, bool *pfValue);

	static HRESULT _GetUINT32Attr(IXMLDOMNode *pXmlNode, const char *pszAttr, UINT32 *pulValue);

	static HRESULT _GetVerbose(IXMLDOMDocument2 *pXmlDoc, bool *pfVerbose);
	static HRESULT _GetProgress(IXMLDOMDocument2 *pXmlDoc, DWORD *pdwProgress);
};
