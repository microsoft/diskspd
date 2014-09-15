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

#include "ThroughputMeter.h"

ThroughputMeter::ThroughputMeter(void) :
    _fRunning(false)
{
}

bool ThroughputMeter::IsRunning(void) const
{
    return _fRunning;
}

void ThroughputMeter::Start(DWORD cBytesPerMillisecond, DWORD dwBlockSize, DWORD dwThinkTime, DWORD dwBurstSize)
{
    // Initialization
    _cbCompleted = 0;
    _cIO = 0; // number of completed IOs in the current burst
    _cbBlockSize = dwBlockSize;

    _fThrottle = false;
    _cBytesPerMillisecond = 0;
    _fThink = false;
    _ullDelayUntil = 0;
    _thinkTime = 0;
    _burstSize = 0;
    _fRunning = false;

    _ullStartTimestamp = GetTickCount64();

    if (0 != cBytesPerMillisecond)
    {
        _fThrottle = true;
        _cBytesPerMillisecond = cBytesPerMillisecond;
        _fRunning = true;
    }
    else if (0 != dwThinkTime)
    {
        _fThink = true;
        _thinkTime = dwThinkTime;
        _burstSize = dwBurstSize;
        _fRunning = true;
    }
}

DWORD ThroughputMeter::GetSleepTime(void) const
{
    if (_fThink)
    {
        ULONGLONG ullTimestamp = GetTickCount64();
        if (ullTimestamp < _ullDelayUntil)
        {
            return (DWORD)(_ullDelayUntil - ullTimestamp);
        }
        else
        {
            return (_fThrottle) ? _GetThrottleTime() : 0;
        }
    }
    else
    {
        if (_fThrottle) // think time has not been specified only check for throttling
        {
            return _GetThrottleTime();
        }
        else
        {
            return 0;
        }
    }
}

DWORD ThroughputMeter::_GetThrottleTime(void) const
{
    ULONGLONG cbExpected = (GetTickCount64() - _ullStartTimestamp) * _cBytesPerMillisecond;
    return cbExpected >= (_cbCompleted + _cbBlockSize) ? 0 : 1;
}

void ThroughputMeter::Adjust(size_t cb)
{
    _cbCompleted += cb;
    _cIO++;
    if (_fThink)
    {
        if (_cIO >= _burstSize)
        {
            _cIO = 0;
            _ullDelayUntil = GetTickCount64() + _thinkTime;
        }
    }
}