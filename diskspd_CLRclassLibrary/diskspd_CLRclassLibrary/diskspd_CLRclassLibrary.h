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

// diskspdCLRclassLibrary.h : Defines the diskspd CLR class to include in CLR projects
// Contributed by: David Cohen, Enmotus Inc., dave.cohen@enmotus.com
//
#pragma once

#include "CmdRequestCreator.h"
#include <windows.h>
#include <stdlib.h>
#include <assert.h>
#include <traceloggingprovider.h>
#include "msclr\marshal_cppstd.h"
#include <iostream>
#include <sstream>
#include "common.h"
#include "errors.h"
#include "CmdLineParser.h"
#include "XmlProfileParser.h"
#include "IORequestGenerator.h"
#include "ResultParser.h"
#include "XmlResultParser.h"


#using <System.Xml.dll>

/*****************************************************************************/
// global variables
static HANDLE g_hAbortEvent = NULL;     // handle to the 'abort' event
										// it allows stopping I/O Request Generator in the middle of its work
										// the results of its work will be passed to the Results Parser
static HANDLE g_hEventStarted = NULL;   // event signalled to notify that the actual (measured) test is to be started
static HANDLE g_hEventFinished = NULL;  // event signalled to notify that the actual test has finished

/*****************************************************************************/
// wrapper for printf. printf cannot be used directly, because IORequestGenerator.dll
// may be consumed by gui app which doesn't have stdout
void WINAPI PrintOut(const char *format, va_list args)
{
	vprintf(format, args);
}

/*****************************************************************************/
// wrapper for fprintf. fprintf cannot be used directly, because IORequestGenerator.dll
// may be consumed by gui app which doesn't have stdout
void WINAPI PrintError(const char *format, va_list args)
{
	vfprintf(stderr, format, args);
}

/*****************************************************************************/
void TestStarted()
{
	if ((NULL != g_hEventStarted) && !SetEvent(g_hEventStarted))
	{
		fprintf(stderr, "Warning: Setting test start notification event failed (error code: %u)\n", GetLastError());
	}
}

/*****************************************************************************/
void TestFinished()
{
	if ((NULL != g_hEventFinished) && !SetEvent(g_hEventFinished))
	{
		fprintf(stderr, "Warning: Setting test finish notification event failed (error code: %u)\n", GetLastError());
	}
}

namespace diskspdCLRclassLibrary {
	public ref class diskspd
	{
	public:
		diskspd()
		{
			// do standard diskspd initialization stuph.
			// ignore command line (they will become parameters) and CTRL-C 
			m_pSynch = new Synchronization;
			memset(m_pSynch, 0, sizeof(Synchronization));
			m_pSynch->ulStructSize = sizeof(Synchronization);

			m_pSynch->pfnCallbackTestStarted = TestStarted;
			m_pSynch->pfnCallbackTestFinished = TestFinished;

			try
			{

				if (NULL == m_pSynch->hStopEvent)
				{
					m_pSynch->hStopEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
					if (NULL == m_pSynch->hStopEvent)
					{
						throw("Unable to create an abort event");
					}
				}

				g_hAbortEvent = m_pSynch->hStopEvent;   // set abort event to either stop event provided by user or the just created event

			}
			catch (System::Exception^ ex)
			{
				;
			}

		}
		~diskspd()
		{
			delete m_pSynch;
		}

		double GetSequentialDevicePerformance(int queudepth, int threads, int device) //minimal specific performance numbers I needed.	
		{
			Profile profile;
			profile.SetResultsFormat(ResultsFormat::Xml);

			IORequestGenerator ioGenerator;
			XmlResultParser xmlResultParser;
			IResultParser *pResultParser = &xmlResultParser;

			vector<Target> vTargets;
			Target target;
			target.SetPath("#" + to_string(device));
			target.SetCacheMode(TargetCacheMode::DisableOSCache);
			target.SetWriteThroughMode(WriteThroughMode::On);
			target.SetMemoryMappedIoMode(MemoryMappedIoMode::Undefined);
			target.SetMemoryMappedIoFlushMode(MemoryMappedIoFlushMode::Undefined);

			target.SetRequestCount(queudepth);
			target.SetThreadsPerFile(threads);

			vTargets.push_back(target);
			TimeSpan timeSpan;
			timeSpan.SetDuration(15);
			for (auto i = vTargets.begin(); i != vTargets.end(); i++)
			{
				timeSpan.AddTarget(*i);
			}
			profile.AddTimeSpan(timeSpan);

			System::Xml::XmlDocument xmlDoc;


			std::string results = ioGenerator.GenerateRequests(profile, *pResultParser, m_pSynch);
			System::String^ sResults = gcnew System::String(results.c_str());
			xmlDoc.LoadXml(sResults);

			try
			{
			
				System::Xml::XmlNode^ secondsNode = xmlDoc.SelectSingleNode("/Results/TimeSpan/TestTimeSeconds");
				double seconds = System::Double::Parse(secondsNode->InnerText);


				System::Xml::XmlNodeList^ threadresults = xmlDoc.SelectNodes("/Results/TimeSpan/Thread");

				double totalBytes = 0;
				for each(System::Xml::XmlNode^ thread in threadresults)
				{
					totalBytes += System::Double::Parse(thread->ChildNodes->Item(1)->SelectSingleNode("BytesCount")->InnerText);
				}

				if (totalBytes && seconds)
				{
					return ((totalBytes / seconds) / 1048576);
				}
			}
			catch(const char* msg)
			{
				// when the xml fails to parse, it usually means that the error is embedded in the xml.  This will pass the error back to the caller.
				throw sResults;
			}
			return 0;
		}

		int GetProcessorCount()  //this is a nifty way to decide how many threads you want to use.
		{
			
			SystemInformation system;
			return system.processorTopology._ulActiveProcCount;
		}

	protected:
		struct Synchronization* m_pSynch;        //sychronization structure


	};
}
