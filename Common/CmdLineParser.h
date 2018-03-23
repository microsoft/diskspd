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
#include "Common.h"

namespace UnitTests { class CmdLineParserUnitTests; }

class CmdLineParser
{
public:
    CmdLineParser();
    ~CmdLineParser();

    bool ParseCmdLine(int argc, const char* argv[], Profile* pProfile, struct Synchronization* synch,
                      SystemInformation* pSystem = nullptr);

private:
    bool _ReadParametersFromCmdLine(int argc, const char *argv[], Profile *pProfile, struct Synchronization *synch);
	static bool _ReadParametersFromXmlFile(const char *pszPath, Profile *pProfile);

	static bool _ParseETWParameter(const char *arg, Profile *pProfile);
	static bool _ParseAffinity(const char *arg, TimeSpan *pTimeSpan);

	static void _DisplayUsageInfo(const char *pszFilename);
    bool _GetSizeInBytes(const char *pszSize, UINT64& ullSize) const;
    bool _GetRandomDataWriteBufferData(const string& sArg, UINT64& cb, string& sPath) const;

    // variables that used to be global
    DWORD _dwBlockSize;         // block size; other parameters may be stated in blocks
                                // so the block size is needed to process them

    UINT32 _ulWriteRatio;       // default percentage of write requests

    HANDLE _hEventStarted;      // event signalled to notify that the actual (measured) test is to be started
    HANDLE _hEventFinished;     // event signalled to notify that the actual test has finished

    friend class UnitTests::CmdLineParserUnitTests;
};
