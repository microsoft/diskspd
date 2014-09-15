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
#include <Windows.h>

// ThroughputMeter class assists in metering out throughput over
// time.  The meter is started by calling Start() with the throughput
// to be simulated.  GetSleepTime() returns 0 when the next IO can be issued.
// Adjust() is called to notify the ThroughputMeter about how many bytes were read/written.
class ThroughputMeter
{
public:
    ThroughputMeter(void);

    bool IsRunning(void) const;
    void Start(DWORD cBytesPerMillisecond, DWORD dwBlockSize, DWORD dwThinkTime, DWORD dwBurstSize);
    DWORD GetSleepTime(void) const;
    void Adjust(size_t cb);

private:
    DWORD _GetThrottleTime(void) const;

    bool _fRunning;                 // true = throughput monitoring is on
    bool _fThrottle;                // true = throttling is on
    bool _fThink;                   // true = think time is enabled
    ULONGLONG _cbCompleted;         // completed IO
    DWORD _cbBlockSize;
    DWORD _cBytesPerMillisecond;    // rate of throttling
    ULONGLONG _ullStartTimestamp;
    ULONGLONG _ullDelayUntil;       // timestamp at which the next IO can be executed
    DWORD _thinkTime;               // time to sleep between burst of IOs
    DWORD _burstSize;               // number of IOs in a burst. meaningless if think time is zero
    DWORD _cIO;                     // count of IOs in the current burst
};