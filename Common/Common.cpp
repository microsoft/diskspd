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

#include "Common.h"

TRACELOGGING_DEFINE_PROVIDER(g_hEtwProvider,
                             "Microsoft-Windows-DiskSpd", // {CA13DB84-D0A9-5145-FCA4-468DA92FDC2D}
                             (0xca13db84, 0xd0a9, 0x5145, 0xfc, 0xa4, 0x46, 0x8d, 0xa9, 0x2f, 0xdc, 0x2d));

SystemInformation g_SystemInformation;
ULONG g_ExperimentFlags;

CRITICAL_SECTION Diagnostics::s_cs;
bool Diagnostics::s_fVerbose = false;
bool Diagnostics::s_fInitialized = false;

void Diagnostics::Initialize()
{
    if (!s_fInitialized)
    {
        InitializeCriticalSection(&s_cs);
        s_fInitialized = true;
    }
}

void Diagnostics::PrintError(const char *format, ...)
{
    assert(s_fInitialized);
    assert(format != nullptr);

    va_list argList;
    va_start(argList, format);
    EnterCriticalSection(&s_cs);
    vfprintf(stderr, format, argList);
    LeaveCriticalSection(&s_cs);
    va_end(argList);
}

void Diagnostics::PrintVerbose(const char *format, ...)
{
    assert(s_fInitialized);
    assert(format != nullptr);

    if (!s_fVerbose)
    {
        return;
    }

    // Build timestamp outside the lock
    char szTimestamp[64] = {};
    SYSTEMTIME now;
    GetLocalTime(&now);
    if (now.wYear)
    {
        sprintf_s(szTimestamp, _countof(szTimestamp),
            "%u-%02u-%02uT%02u:%02u:%02u: ",
            now.wYear, now.wMonth, now.wDay,
            now.wHour, now.wMinute, now.wSecond);
    }

    va_list argList;
    va_start(argList, format);
    EnterCriticalSection(&s_cs);
    printf("%s", szTimestamp);
    vprintf(format, argList);
    LeaveCriticalSection(&s_cs);
    va_end(argList);
}

static VirtualAlloc2FnPtr g_pfnVirtualAlloc2 = nullptr;

bool ResolveVirtualAlloc2()
{
    if (g_pfnVirtualAlloc2 != nullptr)
    {
        return true;
    }

    // kernelbase.dll is always loaded in every process; use GetModuleHandle
    // to avoid an unnecessary reference count increment.
    HMODULE hDll = GetModuleHandleW(L"kernelbase.dll");
    if (hDll == nullptr)
    {
        return false;
    }

    g_pfnVirtualAlloc2 = (VirtualAlloc2FnPtr)GetProcAddress(hDll, "VirtualAlloc2");

    return g_pfnVirtualAlloc2 != nullptr;
}

BYTE* AllocateAlignedBuffer(size_t cb, DWORD alignment)
{
    if (alignment > 0)
    {
        assert(g_pfnVirtualAlloc2 != nullptr);

        MEM_ADDRESS_REQUIREMENTS addressRequirements = {};
        MEM_EXTENDED_PARAMETER memParam = {};

        addressRequirements.Alignment = alignment;
        memParam.Type = MemExtendedParameterAddressRequirements;
        memParam.Pointer = &addressRequirements;

        return (BYTE*)g_pfnVirtualAlloc2(nullptr, nullptr, cb,
                                          MEM_COMMIT, PAGE_READWRITE,
                                          &memParam, 1);
    }

    return (BYTE*)VirtualAlloc(nullptr, cb, MEM_COMMIT, PAGE_READWRITE);
}

UINT64 PerfTimer::GetTime()
{
    LARGE_INTEGER li;
    QueryPerformanceCounter(&li);
    return li.QuadPart;
}

UINT64 PerfTimer::_GetPerfTimerFreq()
{
    LARGE_INTEGER li;
    QueryPerformanceFrequency(&li);
    return li.QuadPart;
}

const UINT64 PerfTimer::TIMER_FREQ = _GetPerfTimerFreq();

double PerfTimer::PerfTimeToMicroseconds(const double perfTime)
{
    return perfTime / (TIMER_FREQ / 1000000.0);
}

double PerfTimer::PerfTimeToMilliseconds(const double perfTime)
{
    return PerfTimeToMicroseconds(perfTime) / 1000;
}

double PerfTimer::PerfTimeToSeconds(const double perfTime)
{
    return PerfTimeToMilliseconds(perfTime) / 1000;
}

double PerfTimer::PerfTimeToMicroseconds(const UINT64 perfTime)
{
    return PerfTimeToMicroseconds(static_cast<double>(perfTime));
}

double PerfTimer::PerfTimeToMilliseconds(const UINT64 perfTime)
{
    return PerfTimeToMilliseconds(static_cast<double>(perfTime));
}

double PerfTimer::PerfTimeToSeconds(const UINT64 perfTime)
{
    return PerfTimeToSeconds(static_cast<double>(perfTime));
}

UINT64 PerfTimer::MicrosecondsToPerfTime(const double microseconds)
{
    return static_cast<UINT64>(TIMER_FREQ * (microseconds / 1000000.0));
}

UINT64 PerfTimer::MillisecondsToPerfTime(const double milliseconds)
{
    return static_cast<UINT64>(TIMER_FREQ * (milliseconds / 1000.0));
}

UINT64 PerfTimer::SecondsToPerfTime(const double seconds)
{
    return static_cast<UINT64>(TIMER_FREQ * seconds);
}

Random::Random(UINT64 ulSeed)
{
    UINT32 i;

    _ulState[0] = 0xf1ea5eed;
    _ulState[1] = ulSeed;
    _ulState[2] = ulSeed;
    _ulState[3] = ulSeed;

    for (i = 0; i < 20; i++) {
        Rand64();
    }
}

void Random::RandBuffer(BYTE *pBuffer, UINT32 ulLength, bool fPseudoRandomOkay)
{
    UINT64 *pBuffer64;
    UINT32 Remaining = (UINT32)(((ULONG_PTR)pBuffer) & 7);
    UINT64 r1, r2, r3, r4, r5;

    //
    // Align to 8 bytes
    //

    if (Remaining != 0)
    {
        r1 = Rand64();

        while (Remaining != 0 && ulLength != 0)
        {
            *pBuffer = (BYTE)(r1 & 0xFF);
            r1 >>= 8;
            pBuffer++;
            ulLength--;
            Remaining--;
        }
    }

    pBuffer64 = (UINT64*)pBuffer;
    Remaining = ulLength / 8;
    ulLength -= Remaining * 8;
    pBuffer += Remaining * 8;

    if (fPseudoRandomOkay)
    {
        //
        // Generate 5 random numbers and then mix them to produce
        // 16 random (but correlated) numbers.  We want to do 16
        // numbers at a time for optimal cache line alignment.
        // Only do this if the caller is okay with numbers that
        // aren't independent.  A detailed analysis of the data
        // could probably detect that the first 5 numbers determine
        // the next 11.  For most purposes this won't matter (for
        // instance it's unlikely compression algorithms will be
        // able to detect this and utilize it).
        //

        while (Remaining > 16)
        {
            r1 = Rand64();
            r2 = Rand64();
            r3 = Rand64();
            r4 = Rand64();
            r5 = Rand64();

            pBuffer64[0]  = r1;
            pBuffer64[1]  = r2;
            pBuffer64[2]  = r3;
            pBuffer64[3]  = r4;
            pBuffer64[4]  = r5;

            //
            // Throw in some rotates so that the below numbers
            // aren't the xor sum of previous numbers.
            //

            r1 = _rotl64(r1, 7);
            pBuffer64[5]  = r1 ^ r2;
            pBuffer64[6]  = r1 ^ r3;
            pBuffer64[7]  = r1 ^ r4;
            pBuffer64[8]  = r1 ^ r5;

            r2 = _rotl64(r2, 13);
            pBuffer64[9]  = r2 ^ r3;
            pBuffer64[10] = r2 ^ r4;
            pBuffer64[11] = r2 ^ r5;

            r3 = _rotl64(r3, 19);
            pBuffer64[12] = r3 ^ r4;
            pBuffer64[13] = r3 ^ r5;

            pBuffer64[14] = r1 ^ r2 ^ r3;
            pBuffer64[15] = r1 ^ _rotl64(r4 ^ r5, 39);

            pBuffer64 += 16;
            Remaining -= 16;
        }
    }

    //
    // Fill in the tail of the buffer
    //

    while (Remaining >= 4)
    {
        r1 = Rand64();
        r2 = Rand64();
        r3 = Rand64();
        r4 = Rand64();

        pBuffer64[0]  = r1;
        pBuffer64[1]  = r2;
        pBuffer64[2]  = r3;
        pBuffer64[3]  = r4;

        pBuffer64 += 4;
        Remaining -= 4;
    }

    while (Remaining != 0)
    {
        *pBuffer64 = Rand64();
        pBuffer64++;
        Remaining--;
    }

    if (ulLength != 0)
    {
        r1 = Rand64();

        while (ulLength != 0)
        {
            *pBuffer = (BYTE)(r1 & 0xFF);
            r1 >>= 8;
            pBuffer++;
            ulLength--;
        }
    }
}

string Util::DoubleToStringHelper(const double d)
{
    char szFloatBuffer[100];
    sprintf_s(szFloatBuffer, _countof(szFloatBuffer), "%10.3lf", d);

    return string(szFloatBuffer);
}

DWORD Util::GetBufferAlignmentSize(BufferSeparation mode, DWORD pageSize, WORD cacheLineSize)
{
    if (mode == BufferSeparation::SystemDefault)
    {
        return 0;
    }

    // PDECacheLine: see comment on declaration for full derivation.
    // MMPTE is 8 bytes on all current Windows platforms (x64/arm64 natively, x86 under PAE).
    constexpr DWORD mmPteSize = 8;

    DWORD ptesPerPage = pageSize / mmPteSize;
    DWORD ptePageCoverage = ptesPerPage * pageSize;

    WORD effectiveCacheLineSize = cacheLineSize > 0 ? cacheLineSize : 64;
    DWORD pdePerCacheLine = effectiveCacheLineSize / mmPteSize;

    return pdePerCacheLine * ptePageCoverage;
}

string Util::GetSizeKMGT(UINT64 size)
{
    struct { UINT32 shift; const char* suffix; } units[] = {
        { 50, "PiB" },
        { 40, "TiB" },
        { 30, "GiB" },
        { 20, "MiB" },
        { 10, "KiB" }
    };

    for (const auto& u : units)
    {
        UINT64 unitSize = (UINT64)1 << u.shift;
        if (size >= unitSize)
        {
            if ((size & (unitSize - 1)) == 0)
            {
                return to_string(static_cast<UINT32>(size >> u.shift)) + u.suffix;
            }
            else
            {
                char buf[32];
                int nWritten = sprintf_s(buf, _countof(buf), "%.2f",
                    static_cast<double>(size) / unitSize);
                assert(nWritten > 0 && nWritten < _countof(buf));

                // Strip trailing zeros after decimal point
                char* dot = strchr(buf, '.');
                if (dot)
                {
                    char* end = buf + nWritten - 1;
                    while (end > dot && *end == '0')
                    {
                        end--;
                    }
                    if (end == dot)
                    {
                        end--;
                    }
                    *(end + 1) = '\0';
                }

                return string(buf) + u.suffix;
            }
        }
    }

    return to_string(size) + "B";
}

//
// Format a vector of WORD values as ranges: "0-3,7,10-12"
//

string Util::IntRanges(const vector<WORD>& values)
{
    string s;
    if (values.empty())
    {
        return s;
    }

    WORD first = values[0];
    WORD last = values[0];

    for (size_t i = 1; i < values.size(); i++)
    {
        if (values[i] == last + 1)
        {
            last = values[i];
        }
        else
        {
            if (!s.empty())
            {
                s += ",";
            }
            if (first == last)
            {
                s += to_string(first);
            }
            else
            {
                s += to_string(first) + "-" + to_string(last);
            }
            first = values[i];
            last = values[i];
        }
    }

    if (!s.empty())
    {
        s += ",";
    }
    if (first == last)
    {
        s += to_string(first);
    }
    else
    {
        s += to_string(first) + "-" + to_string(last);
    }

    return s;
}

//
// Format a ULONG_PTR bitmask as ranges of set bits: "0-3,7,10-12"
//

string Util::MaskRanges(ULONG_PTR mask)
{
    string s;
    BYTE bit = 0;
    DWORD first = MAXDWORD;
    DWORD last = MAXDWORD;

    while (mask)
    {
        if (mask & 1)
        {
            if (first == MAXDWORD)
            {
                first = bit;
                last = bit;
            }
            else if (bit == last + 1)
            {
                last = bit;
            }
            else
            {
                if (!s.empty())
                {
                    s += ",";
                }
                if (first == last)
                {
                    s += to_string(first);
                }
                else
                {
                    s += to_string(first) + "-" + to_string(last);
                }
                first = bit;
                last = bit;
            }
        }
        mask >>= 1;
        bit++;
    }

    if (first != MAXDWORD)
    {
        if (!s.empty())
        {
            s += ",";
        }
        if (first == last)
        {
            s += to_string(first);
        }
        else
        {
            s += to_string(first) + "-" + to_string(last);
        }
    }

    return s;
}

void Distribution::Set(const vector<DistributionRange>& v, DistributionType t)
{
    _vRanges = v;
    _type = t;
    _ioSpan = 100;

    // Now place final element if IO% is < 100.
    // If this is an absolute specification, it will map to zero length here and
    // conversion will occur at the time of target open to the rest of the target.
    // For the percent specification we place the final element as-if directly stated,
    // consuming the tail length.
    //
    // This done here so that the stated specification is indeed complete, and not left
    // for the effective distribution.

    const DistributionRange& last = *_vRanges.rbegin();

    UINT32 ioCur = last._src + last._span;
    if (ioCur < 100)
    {
        UINT64 targetCur = last._dst.first + last._dst.second;
        if (t == DistributionType::Percent && targetCur < 100)
        {
            // tail is available
            // if tail is not available, this will be caught by validation
            _vRanges.emplace_back(ioCur, 100 - ioCur, make_pair(targetCur, 100 - targetCur));
        }
        else
        {
            _vRanges.emplace_back(ioCur, 100 - ioCur, make_pair(targetCur, 0));
        }
    }
}

bool Distribution::Validate(UINT32 blockSize) const
{
    bool fOk = true;
    UINT32 ioAcc = 0;
    UINT64 targetAcc = 0;
    bool absZero = false, absZeroLast = false;

    for (const auto& r : _vRanges)
    {
        if (_type == DistributionType::Absolute)
        {
            // allow zero target span in last position
            absZeroLast = false;
            if (r._dst.second == 0 && !absZero)
            {
                // legal in last position
                absZero = absZeroLast = true;
            }
            else if (r._dst.second < blockSize)
            {
                fprintf(stderr, "ERROR: invalid random distribution target range %I64u - must be a minimum of the specified block size (%u bytes)\n", r._dst.second, blockSize);
                fOk = false;
                break;
            }
        }

        // Validate accumulating IO%
        if (ioAcc + r._span > 100)
        {
            fprintf(stderr, "ERROR: invalid random distribution IO%% %u: can be at most %u - total must be <= 100%%\n", r._span, 100 - ioAcc);
            fOk = false;
            break;
        }

        // Validate accumulating Target%
        // Note that absolute range needs no additional validation - known nonzero/large enough for IO
        if (_type == DistributionType::Percent)
        {
            if (targetAcc + r._dst.second > 100)
            {
                fprintf(stderr, "ERROR: invalid random distribution Target%% %I64u: can be at most %I64u - total must be <= 100%%\n", r._dst.second, 100 - targetAcc);
                fOk = false;
                break;
            }

            // Consuming the target before consuming the IO is invalid.
            // No holes in IO.
            if (targetAcc + r._dst.second == 100 && ioAcc + r._span < 100)
            {
                fprintf(stderr, "ERROR: invalid random distribution: the target is covered with %u%% IO left to distribute\n", 100 - (ioAcc + r._span));
                fOk = false;
                break;
            }
        }

        ioAcc += r._span;
        targetAcc += r._dst.second;
    }

    // Percent dist Target% must sum to 100%. IO% underflow (either due to early Target% 100% or Target% overflow) is handled above.
    if (fOk && _type == DistributionType::Percent && targetAcc != 100)
    {
        fprintf(stderr, "ERROR: invalid random distribution span: Target%% (%I64u%%) must total 100%%\n", targetAcc);
        fOk = false;
    }
    if (fOk && absZero && !absZeroLast)
    {
        fprintf(stderr, "ERROR: invalid zero target range in random distribution - must be a minimum of the specified block size (%u bytes)\n", blockSize);
        fOk = false;
    }

    return fOk;
}

void Distribution::Finalize(UINT64 relTargetSizeAligned, UINT64 relTargetSize,
                             UINT32 blockSize, UINT64 blockAlignment)
{
    // Create a copy of the stated ranges and work from that
    vector<DistributionRange> statedRanges = _vRanges;
    _vRanges.clear();

    switch (_type)
    {
        case DistributionType::Percent:
        {
            UINT32 ioCarry = 0;

            for (auto& r : statedRanges)
            {
                //
                // The basic premise is to align the range's bounds to discover whether there are
                // any aligned offsets within it. To do this we align DOWN. This moves the adjacent
                // end of this range and base of the next in lockstep.
                //
                // There are two basic branches and three subcases in each:
                //
                //  * aligned base
                //  * unaligned base
                //  * and within each
                //      * aligned end
                //      * unaligned end in same alignment unit
                //      * unaligned end in next/following alignment unit
                //
                //  * aligned/aligned will not move b/e, there will be a positive range
                //  * aligned/unaligned-next will move e in step with the following b
                //      and there will be a positive range
                //  * aligned/unaligned-same will result in b=e after aligning; IO at b is
                //      the only possible IO
                //
                // Unaligned base is more interesting due to degenerate spans, spans where the
                // mimimum %range is smaller than the block alignment. For instance, a 100KiB target
                // with a 4K alignment has a 1%/1KB minimum and may create these cases.
                //
                //  * unaligned/aligned aligns base (down) and there is a positive range
                //  * unaligned/unaligned-next aligns both down and there is a positive range
                //  * unaligned/unaligned-same has no aligned offset in the range; we can detect
                //      this by aligning e first and seeing if it is less than unaligned b. there
                //      are two subcases:
                //      * if the prior range is of zero length, we roll this range's IO% onto it -
                //        this combines two or more adjacent degenerate spans
                //      * if it was not of zero length, we roll over the IO% to the next/last range
                //
                //  Now, in the cases where we have a positive range we may still find our aligned
                //  base is the same as the prior range - the prior was degenerate and the current
                //  is not. In this case we need to round our base up so that we do not share a base.
                //  We may then find that our rounded up base makes us degenerate and ... roll over.
                //
                // Note that this is a closed/open interval. The end offset is NOT a member of this
                // range. Consider an 8KiB file divided 50:50 into two 4KB ranges. The first range is
                // [0,4KB) and the second is [4KB, 8KB). The IO at offset 4KB belongs to the second
                // range, not the first.
                //

                //
                // Skip holes. These have the effect of excluding a range of the target by way of
                // zero IO will be issued to them; the resulting range is still IO 0-100%.
                //

                if (!r._span)
                {
                    continue;
                }

                UINT64 b, e;

                b = ((r._dst.first * relTargetSizeAligned) / 100);
                // guarantee end (don't lose it in integer math)
                if (r._dst.first + r._dst.second == 100)
                {
                    e = relTargetSizeAligned;
                }
                else
                {
                    e = b + ((r._dst.second * relTargetSizeAligned) / 100);
                }

                e = ROUND_DOWN(e, blockAlignment);

                // unaligned/unaligned-same
                // carryover IO% to next/last range
                if (e < b)
                {
                    // is the prior range degenerate?
                    // if so, extend its IO%
                    // note that this cannot happen for the first range, so there
                    // will always be a range to look at.
                    if (_vRanges.rbegin()->_dst.first == e)
                    {
                        _vRanges.rbegin()->_span += r._span;
                    }
                    // carry over to next
                    else
                    {
                        ioCarry = r._span;
                    }

                    continue;
                }

                b = ROUND_DOWN(b, blockAlignment);

                // Now if b < e (a positive range) we may discover we're adjacent
                // to a degenerate range. This is the case of re-aligning b up.
                // Note that the degenerate range logically rounds up - this does
                // not affect operation, but presents the correct appearance of a
                // closed/open interval with respect to the subsequent range.
                // Case: -rdpct10/1:10/1
                //
                // It is possible b == e: this is a case where b was already aligned
                // and we're placing a normal degenerate span. No special handling.

                if (b < e &&
                    _vRanges.size() &&
                    _vRanges.rbegin()->_dst.first == b)
                {

                    b += blockAlignment;
                    _vRanges.rbegin()->_dst.second += blockAlignment;

                    // Now there are two degenerate cases to manage.

                    // if we're dealing with a degenerate at the tail, allow carryover
                    if (b == relTargetSizeAligned)
                    {
                        ioCarry = r._span;
                        continue;
                    }

                    // otherwise, if the range became degenerate in the up-alignment, it must
                    // combine with the prior degenerate since its logical range is included
                    // with it.
                    if (b == e)
                    {
                        _vRanges.rbegin()->_span += r._span;
                        continue;
                    }

                    // fall through to place re-aligned b/e (non degenerate)
                }

                // prefer to roll IO% to the smaller of prior range/this range
                if (ioCarry &&
                    _vRanges.rbegin()->_span < r._span)
                {
                    _vRanges.rbegin()->_span += ioCarry;
                    ioCarry = 0;
                }

                _vRanges.emplace_back(
                    r._src - ioCarry,
                    r._span + ioCarry,
                    make_pair(b, e - b));

                ioCarry = 0;
            }

            // Apply trailing carryover to final range, extending it.
            // Guarantee target range extends to aligned size - rollover is always from
            // a degenerate range we could not place directly. We need to gross up the
            // actual tail so that the effective correctly spans the open/closed interval
            // to target size.
            // -rdpct10/96:10/3:80/1 - the last range is degenerate and needs to roll.
            if (ioCarry)
            {
                DistributionRange& last = *_vRanges.rbegin();

                last._span += ioCarry;
                last._dst.second = relTargetSizeAligned - last._dst.first;
            }
        }
        break;

        case DistributionType::Absolute:
        {
            UINT32 ioUsed = 0;

            for (auto& r : statedRanges)
            {
                //
                // The premise for absolute distributions is similar but without the complication of
                // degenerate ranges. The offsets are provided and we only need to push the last to
                // the end of the range if it was left open (its length is zero). They do not need to
                // be aligned, similar to -T thread stride - this is the caller's dilemma. We already
                // know by validation that IO can be issued in the range since any absolute distribution
                // with a range < block size would have been rejected.
                //
                // If the range was not left open we have two cases:
                //
                //  * the end is within the final range
                //  * the end is past it
                //
                // If the end is within the final range that will again be the caller's dilemma, we'll
                // simply trim the length of that range. If it is past it, we will discard the trailing
                // ranges and trim the maximum IO% so that they become a proportional specification of the
                // IO. For instance, if a 10/10/80 winds up with the 80% not addressable in the file, the
                // maximum IO% trims to 20 and it logically becomes a 50:50 split (10:10).
                //

                UINT64 l;

                //
                // Skip holes. These have the effect of excluding a range of the target by way of
                // zero IO will be issued to them; the resulting range is still IO 0-100%.
                //

                if (!r._span)
                {
                    continue;
                }

                // beyond end? done, with whatever tail IO% not seen
                if (r._dst.first >= relTargetSize)
                {
                    break;
                }
                // open end or spans end? - set to aligned remainder
                else if (r._dst.second == 0 ||
                         r._dst.first + r._dst.second > relTargetSize)
                {
                    // ensure tail can accept IO by blocksize - caller has stated this is aligned by
                    // its specification
                    l = relTargetSize - r._dst.first;

                    if (l < blockSize)
                    {
                        break;
                    }
                }
                else
                {
                    l = r._dst.second;
                }

                _vRanges.emplace_back(
                    r._src,
                    r._span,
                    make_pair(r._dst.first, l));

                ioUsed += r._span;
            }

            // reduce the IO distribution to that specified by the ranges consumed.
            // it is still logically 100%, simply over a range of less than 0-100.
            _ioSpan = ioUsed;
        }
        break;

        // none
        default:
        break;
    }

    // After finalization, ranges are absolute byte offsets regardless of original type
    if (!_vRanges.empty())
    {
        _type = DistributionType::Absolute;
    }
}

string Distribution::GetText(UINT32 indent) const
{
    if (_vRanges.empty())
    {
        return string();
    }

    string sText;
    string sIndent(indent, ' ');
    char buf[128];

    // Column width for right-aligning size strings in absolute distribution output.
    // Derived from the maximum formatted size: up to 3 integer digits, 1 decimal point,
    // 2 fractional digits, and a 3-character unit suffix (e.g., "KiB"). 3+1+2+3 = 9.
    constexpr size_t c_sizeColumnWidth = 9;

    switch (_type)
    {
        case DistributionType::Percent:
        for (const auto &r : _vRanges)
        {
            sprintf_s(buf, _countof(buf), "   %3u%% of IO => [%2I64u%% - %3I64u%%) of target\n",
                    r._span,
                    r._dst.first,
                    r._dst.first + r._dst.second
                );
            sText += sIndent + buf;
        }
        break;

        case DistributionType::Absolute:
        {
            const DistributionRange& last = *_vRanges.rbegin();
            UINT32 max = last._src + last._span;

            for (const auto &r : _vRanges)
            {
                string sLine = sIndent;

                // If this is a trimmed distribution (target was smaller than its range)
                // then we need to rescale the trimmed IO% to 100%. Present this with a
                // single decimal point, which may of course show rounding.
                if (max < 100)
                {
                    sprintf_s(buf, _countof(buf), "    %0.1f%% of IO => [", (double) 100 * r._span / max);
                }
                // Otherwise it is a simple 1-100% and can avoid rounding artifacts.
                else
                {
                    sprintf_s(buf, _countof(buf), "   %3u%% of IO => [", r._span);
                }
                sLine += buf;

                if (r._dst.first == 0)
                {
                    // directly emit leading zero so we can align it
                    sLine += "     0   ";
                }
                else
                {
                    string sSize = Util::GetSizeKMGT(r._dst.first);
                    if (c_sizeColumnWidth > sSize.size())
                    {
                        sLine.append(c_sizeColumnWidth - sSize.size(), ' ');
                    }
                    sLine += sSize;
                }
                sLine += " - ";
                // zero length occurs (only) in specification as a placeholder for end of target
                if (r._dst.second)
                {
                    string sSize = Util::GetSizeKMGT(r._dst.first + r._dst.second);
                    if (c_sizeColumnWidth > sSize.size())
                    {
                        sLine.append(c_sizeColumnWidth - sSize.size(), ' ');
                    }
                    sLine += sSize;
                    sLine += ")\n";
                }
                else
                {
                    sLine += "      end)\n";
                }

                sText += sLine;
            }
        }
        break;
    }

    return sText;
}

string Distribution::GetXml(UINT32 indent, bool fRenderHoles) const
{
    if (_vRanges.empty())
    {
        return string();
    }

    char buf[128];
    string sXml;

    const char *type = nullptr;

    switch (_type)
    {
        case DistributionType::Absolute:
        type = "Absolute";
        break;

        case DistributionType::Percent:
        type = "Percent";
        break;

        default:
        assert(false);
    }

    AddXmlInc(sXml, "<Distribution>\n");
    AddXmlInc(sXml, "<");
    sXml += type;
    sXml += ">\n";

    //
    // When rendering effective (finalized) distributions, emit hole ranges
    // to represent gaps between adjacent ranges. This allows the XML consumer
    // to reconstruct the full target layout.
    //

    UINT64 expectBase = 0;
    for (const auto& r : _vRanges)
    {
        if (fRenderHoles && r._dst.first != expectBase)
        {
            sprintf_s(buf, _countof(buf), "<Range IO=\"%u\">%I64u", 0, r._dst.first - expectBase);
            AddXml(sXml, buf);
            sXml += "</Range>\n";
        }

        sprintf_s(buf, _countof(buf), "<Range IO=\"%u\">%I64u", r._span, r._dst.second);
        AddXml(sXml, buf);
        sXml += "</Range>\n";

        if (fRenderHoles)
        {
            expectBase = r._dst.first + r._dst.second;
        }
    }

    AddXmlDec(sXml, "</");
    sXml += type;
    sXml += ">\n";
    AddXmlDec(sXml, "</Distribution>\n");

    return sXml;
}

string ProcessorTopology::GetXml(UINT32 indent, Section section) const
{
    char szBuffer[64];
    int nWritten;
    string sXml;

    if (section == Section::Cache)
    {
        for (const auto& c : _vProcessorCacheInformation)
        {
            AddXmlInc(sXml, "<Cache Level=\"");
            sXml += to_string(c._level);
            sXml += "\" Associativity=\"";
            sXml += to_string(c._associativity);
            sXml += "\" LineSize=\"";
            sXml += to_string(c._lineSize);
            sXml += "\" CacheSize=\"";
            sXml += to_string(c._cacheSize);
            sXml += "\" Type=\"";
            sXml += ProcessorCacheInformation::TypeName(c._type);
            sXml += "\">\n";
            for (const auto& g : c._processorMasks)
            {
                AddXml(sXml, "<Group Group=\"");
                sXml += to_string(g.first);
                sXml += "\" Mask=\"0x";
                nWritten = sprintf_s(szBuffer, _countof(szBuffer), "%Ix", g.second);
                assert(nWritten && nWritten < _countof(szBuffer));
                sXml += szBuffer;
                sXml += "\"/>\n";
            }
            AddXmlDec(sXml, "</Cache>\n");
        }

        return sXml;
    }

    // All or Topology: emit the full ProcessorTopology element

    AddXmlInc(sXml, "<ProcessorTopology Heterogeneous=\"");
    sXml += _ubPerformanceEfficiencyClass ? "true\">\n" : "false\">\n";

    for (const auto& g : _vProcessorGroupInformation)
    {
        AddXml(sXml, "<Group Group=\"");
        sXml += to_string(g._groupNumber);
        sXml += "\" MaximumProcessors=\"";
        sXml += to_string(g._maximumProcessorCount);
        sXml += "\" ActiveProcessors=\"";
        sXml += to_string(g._activeProcessorCount);
        sXml += "\" ActiveProcessorMask=\"0x";
        nWritten = sprintf_s(szBuffer, _countof(szBuffer), "%Ix", g._activeProcessorMask);
        assert(nWritten && nWritten < _countof(szBuffer));
        sXml += szBuffer;
        sXml += "\"/>\n";
    }
    for (const auto& n : _vProcessorNumaInformation)
    {
        AddXmlInc(sXml, "<Node Node=\"");
        sXml += to_string(n._nodeNumber);
        sXml += "\">\n";
        for (const auto& g : n._vProcessorMasks)
        {
            AddXml(sXml, "<Group Group=\"");
            sXml += to_string(g.first);
            sXml += "\" Mask=\"0x";
            nWritten = sprintf_s(szBuffer, _countof(szBuffer), "%Ix", g.second);
            assert(nWritten && nWritten < _countof(szBuffer));
            sXml += szBuffer;
            sXml += "\"/>\n";
        }
        AddXmlDec(sXml, "</Node>\n");
    }
    for (const auto& s : _vProcessorSocketInformation)
    {
        AddXmlInc(sXml, "<Socket Socket=\"");
        sXml += to_string(s._ulSocketNumber);
        sXml += "\">\n";
        for (const auto& g : s._vProcessorMasks)
        {
            AddXml(sXml, "<Group Group=\"");
            sXml += to_string(g.first);
            sXml += "\" Mask=\"0x";
            nWritten = sprintf_s(szBuffer, _countof(szBuffer), "%Ix", g.second);
            assert(nWritten && nWritten < _countof(szBuffer));
            sXml += szBuffer;
            sXml += "\"/>\n";
        }
        AddXmlDec(sXml, "</Socket>\n");
    }
    for (const auto& h : _vProcessorCoreInformation)
    {
        AddXml(sXml, "<Core Group=\"");
        sXml += to_string(h._groupNumber);
        sXml += "\" Core=\"";
        sXml += to_string(h._groupCoreNumber);
        sXml += "\" Mask=\"0x";
        nWritten = sprintf_s(szBuffer, _countof(szBuffer), "%Ix", h._processorMask);
        assert(nWritten && nWritten < _countof(szBuffer));
        sXml += szBuffer;
        sXml += "\" EfficiencyClass=\"";
        sXml += to_string(h._efficiencyClass);
        sXml += "\"/>\n";
    }

    if (section != Section::Topology)
    {
        sXml += GetXml(indent, Section::Cache);
    }

    AddXmlDec(sXml, "</ProcessorTopology>\n");

    return sXml;
}

//
// Format a set of (group, mask) pairs as group mask range string.
//  Single group:              "0-3,7" (no group prefix)
//  Multi group (fMultiGroup): "0/0-3 1/0-7" (space-separated group entries)
//

string ProcessorTopology::GroupMaskRanges(
    const vector<pair<WORD, KAFFINITY>>& masks,
    bool fMultiGroup)
{
    string s;
    for (const auto& gm : masks)
    {
        if (!s.empty())
        {
            s += " ";
        }
        if (fMultiGroup)
        {
            s += to_string(gm.first) + "/";
        }
        s += Util::MaskRanges(gm.second);
    }
    return s;
}

string ProcessorTopology::GetText(UINT32 indent, Section section) const
{
    string sText;
    string sIndent(indent, ' ');

    if (section != Section::Cache)
    {
        sText += sIndent + "cpu count:            " + to_string(_ulProcessorCount) + "\n";
        sText += sIndent + "core count:           " + to_string(_vProcessorCoreInformation.size()) + "\n";
        sText += sIndent + "group count:          " + to_string(_vProcessorGroupInformation.size()) + "\n";
        sText += sIndent + "node count:           " + to_string(_vProcessorNumaInformation.size()) + "\n";
        sText += sIndent + "socket count:         " + to_string(_vProcessorSocketInformation.size()) + "\n";
        sText += sIndent + "heterogeneous cores:  ";
        sText += _ubPerformanceEfficiencyClass ? "y\n" : "n\n";
    }

    if (section != Section::Topology && !_vProcessorCacheInformation.empty())
    {
        bool fMultiGroup = _vProcessorGroupInformation.size() > 1;
        string sTableIndent(indent + 2, ' ');

        sText += "\n" + sIndent + "cache information:\n\n";
        if (fMultiGroup)
        {
            sText += sTableIndent + "Cache |   Size   | Line  | Assoc  | Group/CPU\n";
            sText += sTableIndent + "-----------------------------------------------------------\n";
        }
        else
        {
            sText += sTableIndent + "Cache |   Size   | Line  | Assoc  | CPU\n";
            sText += sTableIndent + "-------------------------------------------------------\n";
        }

        //
        // Two stage process:
        //
        // 1. Pre-classify each cache for combined per-X output
        //
        //   Group: single mask that exactly matches a processor group (multi-group only)
        //   Core:  single mask that exactly matches a multi-cpu core
        //   Cpu:   single mask with exactly one bit set
        //   Other: everything else (multi-mask, or single mask not matching above)
        //

        enum class CacheClass { Group, Core, Cpu, Other };

        vector<CacheClass> cacheClass(_vProcessorCacheInformation.size());

        for (size_t i = 0; i < _vProcessorCacheInformation.size(); i++)
        {
            const auto& cache = _vProcessorCacheInformation[i];

            if (cache._processorMasks.size() != 1)
            {
                cacheClass[i] = CacheClass::Other;
                continue;
            }

            WORD group = cache._processorMasks[0].first;
            KAFFINITY mask = cache._processorMasks[0].second;

            // Group case: multi-group systems only
            if (fMultiGroup)
            {
                bool isGroup = false;
                for (const auto& g : _vProcessorGroupInformation)
                {
                    if (g._groupNumber == group && g._activeProcessorMask == mask)
                    {
                        isGroup = true;
                        break;
                    }
                }
                if (isGroup)
                {
                    cacheClass[i] = CacheClass::Group;
                    continue;
                }
            }

            int popCount = MaskCount(mask);
            assert(popCount > 0);

            // Multi-cpu core? If more than one cpu, it is either a core
            // or falls through to the "Other" classification
            if (popCount > 1)
            {
                bool isCore = false;
                for (const auto& core : _vProcessorCoreInformation)
                {
                    if (core._groupNumber == group && core._processorMask == mask)
                    {
                        isCore = true;
                        break;
                    }
                }
                if (isCore)
                {
                    cacheClass[i] = CacheClass::Core;
                    continue;
                }
            }
            // Defensively check for exactly one so that a malformed zero-popcount
            // mask doesnot get classified as CPU
            else if (popCount == 1)
            {
                // Cpu case: single bit

                cacheClass[i] = CacheClass::Cpu;
                continue;
            }

            cacheClass[i] = CacheClass::Other;
        }

        //
        // 2. Output loop: iterate in vector order, combining forward matches
        //    of the same geometry and classification. This preserves the original
        //    order of the caches in the output.
        //

        vector<bool> emitted(_vProcessorCacheInformation.size(), false);

        for (size_t i = 0; i < _vProcessorCacheInformation.size(); i++)
        {
            if (emitted[i])
            {
                continue;
            }

            const auto& cache = _vProcessorCacheInformation[i];
            emitted[i] = true;

            string sGroupCpu;
            string sSuffix;

            if (cacheClass[i] == CacheClass::Other)
            {
                sGroupCpu = GroupMaskRanges(cache._processorMasks, fMultiGroup);
            }
            else if (cacheClass[i] == CacheClass::Group)
            {
                // Collect group numbers from all forward matches of same geometry + Group class

                vector<WORD> groups;
                groups.push_back(cache._processorMasks[0].first);

                for (size_t j = i + 1; j < _vProcessorCacheInformation.size(); j++)
                {
                    if (emitted[j] ||
                        cacheClass[j] != CacheClass::Group ||
                        !cache.SameGeometry(_vProcessorCacheInformation[j]))
                    {
                        continue;
                    }

                    groups.push_back(_vProcessorCacheInformation[j]._processorMasks[0].first);
                    emitted[j] = true;
                }

                if (groups.size() > 1)
                {
                    sort(groups.begin(), groups.end());
                    sGroupCpu = Util::IntRanges(groups);
                    sSuffix = " (per group)";
                }
                else
                {
                    sGroupCpu = GroupMaskRanges(cache._processorMasks, fMultiGroup);
                }
            }
            else
            {
                // Core or Cpu: combine masks from forward matches of same geometry + same class

                map<WORD, KAFFINITY> combinedMasks;
                combinedMasks[cache._processorMasks[0].first] = cache._processorMasks[0].second;
                size_t matchCount = 0;

                for (size_t j = i + 1; j < _vProcessorCacheInformation.size(); j++)
                {
                    if (emitted[j] ||
                        cacheClass[j] != cacheClass[i] ||
                        !cache.SameGeometry(_vProcessorCacheInformation[j]))
                    {
                        continue;
                    }

                    combinedMasks[_vProcessorCacheInformation[j]._processorMasks[0].first] |=
                        _vProcessorCacheInformation[j]._processorMasks[0].second;
                    emitted[j] = true;
                    matchCount++;
                }

                if (matchCount > 0)
                {
                    vector<pair<WORD, KAFFINITY>> combined(combinedMasks.begin(), combinedMasks.end());
                    sGroupCpu = GroupMaskRanges(combined, fMultiGroup);
                    sSuffix = (cacheClass[i] == CacheClass::Core) ? " (per core)" : " (per cpu)";
                }
                else
                {
                    sGroupCpu = GroupMaskRanges(cache._processorMasks, fMultiGroup);
                }
            }

            //
            // Now format the cache information for output
            //

            string sType = string("L") + to_string(cache._level) +
                           ProcessorCacheInformation::TypeAbbreviation(cache._type);

            string sSize = Util::GetSizeKMGT(cache._cacheSize);
            string sLine = Util::GetSizeKMGT(cache._lineSize);

            string sAssoc;
            if (cache._associativity == 0xFF)
            {
                sAssoc = "full";
            }
            else
            {
                sAssoc = to_string(cache._associativity) + "-way";
            }

            char buf[128];
            sprintf_s(buf, _countof(buf), "%-5s | %8s | %5s | %6s | ",
                sType.c_str(),
                sSize.c_str(),
                sLine.c_str(),
                sAssoc.c_str());
            sText += sTableIndent + buf + sGroupCpu + sSuffix + "\n";
        }
    }

    return sText;
}

string ThreadTarget::GetXml(UINT32 indent) const
{
    char buffer[4096];
    string sXml;

    AddXmlInc(sXml, "<ThreadTarget>\n");

    sprintf_s(buffer, _countof(buffer), "<Thread>%u</Thread>\n", _ulThread);
    AddXml(sXml, buffer);

    if (_ulWeight != 0)
    {
        sprintf_s(buffer, _countof(buffer), "<Weight>%u</Weight>\n", _ulWeight);
        AddXml(sXml, buffer);
    }

    AddXmlDec(sXml, "</ThreadTarget>\n");

    return sXml;
}

string Target::GetXml(UINT32 indent) const
{
    char buffer[4096];
    string sXml;

    AddXmlInc(sXml, "<Target>\n");
    AddXml(sXml, "<Path>" + _sPath + "</Path>\n");

    sprintf_s(buffer, _countof(buffer), "<BlockSize>%u</BlockSize>\n", _dwBlockSize);
    AddXml(sXml, buffer);

    sprintf_s(buffer, _countof(buffer), "<BaseFileOffset>%I64u</BaseFileOffset>\n", _ullBaseFileOffset);
    AddXml(sXml, buffer);

    AddXml(sXml, _fSequentialScanHint ? "<SequentialScan>true</SequentialScan>\n" : "<SequentialScan>false</SequentialScan>\n");
    AddXml(sXml, _fRandomAccessHint ? "<RandomAccess>true</RandomAccess>\n" : "<RandomAccess>false</RandomAccess>\n");
    AddXml(sXml, _fTemporaryFileHint ? "<TemporaryFile>true</TemporaryFile>\n" : "<TemporaryFile>false</TemporaryFile>\n");
    AddXml(sXml, _fUseLargePages ? "<UseLargePages>true</UseLargePages>\n" : "<UseLargePages>false</UseLargePages>\n");

    // TargetCacheMode::Cached is implied default
    switch (_cacheMode)
    {
    case TargetCacheMode::DisableLocalCache:
        AddXml(sXml, "<DisableLocalCache>true</DisableLocalCache>\n");
        break;
    case TargetCacheMode::DisableOSCache:
        AddXml(sXml, "<DisableOSCache>true</DisableOSCache>\n");
        break;
    }

    // WriteThroughMode::Off is implied default
    switch (_writeThroughMode)
    {
    case WriteThroughMode::On:
        AddXml(sXml, "<WriteThrough>true</WriteThrough>\n");
        break;
    }

    // MemoryMappedIoMode::Off is implied default
    switch (_memoryMappedIoMode)
    {
    case MemoryMappedIoMode::On:
        AddXml(sXml, "<MemoryMappedIo>true</MemoryMappedIo>\n");
        break;
    }

    // MemoryMappedIoFlushMode::Undefined is implied default
    switch (_memoryMappedIoFlushMode)
    {
    case MemoryMappedIoFlushMode::ViewOfFile:
        AddXml(sXml, "<FlushType>ViewOfFile</FlushType>\n");
        break;
    case MemoryMappedIoFlushMode::NonVolatileMemory:
        AddXml(sXml, "<FlushType>NonVolatileMemory</FlushType>\n")
        break;
    case MemoryMappedIoFlushMode::NonVolatileMemoryNoDrain:
        AddXml(sXml, "<FlushType>NonVolatileMemoryNoDrain</FlushType>\n");
        break;
    }

    // BypassIoMode::Undefined is implied default
    switch (_bypassIoMode)
    {
    case BypassIoMode::Partial:
        AddXml(sXml, "<BypassIO>Partial</BypassIO>\n");
        break;
    case BypassIoMode::Full:
        AddXml(sXml, "<BypassIO>Full</BypassIO>\n");
        break;
    }

    AddXmlInc(sXml, "<WriteBufferContent>\n");
    if (_fZeroWriteBuffers)
    {
        AddXml(sXml, "<Pattern>zero</Pattern>\n");
    }
    else if (_cbRandomDataWriteBuffer == 0)
    {
        AddXml(sXml, "<Pattern>sequential</Pattern>\n");
    }
    else
    {
        AddXml(sXml, "<Pattern>random</Pattern>\n");
        AddXmlInc(sXml, "<RandomDataSource>\n");
        sprintf_s(buffer, _countof(buffer), "<SizeInBytes>%I64u</SizeInBytes>\n", _cbRandomDataWriteBuffer);
        AddXml(sXml, buffer);
        if (_sRandomDataWriteBufferSourcePath != "")
        {
            AddXml(sXml, "<FilePath>" + _sRandomDataWriteBufferSourcePath + "</FilePath>\n");
        }
        AddXmlDec(sXml, "</RandomDataSource>\n");
    }
    AddXmlDec(sXml, "</WriteBufferContent>\n");

    AddXml(sXml, _fParallelAsyncIO ? "<ParallelAsyncIO>true</ParallelAsyncIO>\n" : "<ParallelAsyncIO>false</ParallelAsyncIO>\n");

    if (_fUseBurstSize)
    {
        sprintf_s(buffer, _countof(buffer), "<BurstSize>%u</BurstSize>\n", _dwBurstSize);
        AddXml(sXml, buffer);
    }

    if (_fThinkTime)
    {
        sprintf_s(buffer, _countof(buffer), "<ThinkTime>%u</ThinkTime>\n", _dwThinkTime);
        AddXml(sXml, buffer);
    }

    if (_fCreateFile)
    {
        sprintf_s(buffer, _countof(buffer), "<FileSize>%I64u</FileSize>\n", _ullFileSize);
        AddXml(sXml, buffer);
    }

    // If XML contains <Random>, <StrideSize> is ignored
    if (_ulRandomRatio > 0)
    {
        sprintf_s(buffer, _countof(buffer), "<Random>%I64u</Random>\n", GetBlockAlignmentInBytes());
        AddXml(sXml, buffer);

        // 100% random is <Random> alone
        if (_ulRandomRatio != 100)
        {
            sprintf_s(buffer, _countof(buffer), "<RandomRatio>%u</RandomRatio>\n", GetRandomRatio());
            AddXml(sXml, buffer);
        }

        // Distributions only occur in profiles with random IO.

        sXml += _distribution.GetXml(indent);
    }
    else
    {
        sprintf_s(buffer, _countof(buffer), "<StrideSize>%I64u</StrideSize>\n", GetBlockAlignmentInBytes());
        AddXml(sXml, buffer);

        AddXml(sXml, _fInterlockedSequential ?
            "<InterlockedSequential>true</InterlockedSequential>\n" :
            "<InterlockedSequential>false</InterlockedSequential>\n");
    }

    sprintf_s(buffer, _countof(buffer), "<ThreadStride>%I64u</ThreadStride>\n", _ullThreadStride);
    AddXml(sXml, buffer);

    sprintf_s(buffer, _countof(buffer), "<MaxFileSize>%I64u</MaxFileSize>\n", _ullMaxFileSize);
    AddXml(sXml, buffer);

    sprintf_s(buffer, _countof(buffer), "<RequestCount>%u</RequestCount>\n", _dwRequestCount);
    AddXml(sXml, buffer);

    sprintf_s(buffer, _countof(buffer), "<WriteRatio>%u</WriteRatio>\n", _ulWriteRatio);
    AddXml(sXml, buffer);

    // Preserve specified units
    if (_dwThroughputIOPS)
    {
        sprintf_s(buffer, _countof(buffer), "<Throughput unit=\"IOPS\">%u</Throughput>\n", _dwThroughputIOPS);
        AddXml(sXml, buffer);
    }
    else
    {
        sprintf_s(buffer, _countof(buffer), "<Throughput>%u</Throughput>\n", _dwThroughputBytesPerMillisecond);
        AddXml(sXml, buffer);
    }

    sprintf_s(buffer, _countof(buffer), "<ThreadsPerFile>%u</ThreadsPerFile>\n", _dwThreadsPerFile);
    AddXml(sXml, buffer);

    if (_ioPriorityHint == IoPriorityHintVeryLow)
    {
        AddXml(sXml, "<IOPriority>1</IOPriority>\n");
    }
    else if (_ioPriorityHint == IoPriorityHintLow)
    {
        AddXml(sXml, "<IOPriority>2</IOPriority>\n");
    }
    else if (_ioPriorityHint == IoPriorityHintNormal)
    {
        AddXml(sXml, "<IOPriority>3</IOPriority>\n");
    }
    else
    {
        AddXml(sXml, "<IOPriority>* UNSUPPORTED *</IOPriority>\n");
    }

    sprintf_s(buffer, _countof(buffer), "<Weight>%u</Weight>\n", _ulWeight);
    AddXml(sXml, buffer);

    if (_vThreadTargets.size() > 0)
    {
        AddXmlInc(sXml, "<ThreadTargets>\n");

        for (const auto& threadTarget : _vThreadTargets)
        {
            sXml += threadTarget.GetXml(indent);
        }

        AddXmlDec(sXml, "</ThreadTargets>\n");
    }

    AddXmlDec(sXml, "</Target>\n");

    return sXml;
}

bool Target::_FillRandomDataWriteBuffer(Random *pRand)
{
    assert(_pRandomDataWriteBuffer != nullptr);
    bool fOk = true;
    size_t cb = static_cast<size_t>(GetRandomDataWriteBufferSize());
    if (GetRandomDataWriteBufferSourcePath() == "")
    {
        pRand->RandBuffer(_pRandomDataWriteBuffer, (UINT32)cb, false);
    }
    else
    {
        // fill buffer from file
        HANDLE hFile = CreateFile(GetRandomDataWriteBufferSourcePath().c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, 0, nullptr);
        if (hFile != INVALID_HANDLE_VALUE)
        {
            UINT64 cbLeftToRead = GetRandomDataWriteBufferSize();
            BYTE *pBuffer = _pRandomDataWriteBuffer;
            bool fReadSuccess = true;
            while (fReadSuccess && cbLeftToRead > 0)
            {
                DWORD cbToRead = static_cast<DWORD>(min(64 * 1024, cbLeftToRead));
                DWORD cbRead;
                fReadSuccess = ((ReadFile(hFile, pBuffer, cbToRead, &cbRead, nullptr) == TRUE) && (cbRead > 0));
                pBuffer += cbRead;
            }

            // if the file is smaller than the buffer, repeat its content
            BYTE *pSource = _pRandomDataWriteBuffer;
            const BYTE *pPastEnd = pSource + GetRandomDataWriteBufferSize();
            while (pBuffer < pPastEnd)
            {
                *pBuffer++ = *pSource++;
            }
            CloseHandle(hFile);
        }
        else
        {
            printf("\n\nERROR: Unable to open entropy file '%s'\n\n", GetRandomDataWriteBufferSourcePath().c_str());
            fOk = false;
        }
    }
    return fOk;
}

bool Target::AllocateAndFillRandomDataWriteBuffer(Random *pRand)
{
    assert(_pRandomDataWriteBuffer == nullptr);
    bool fOk = false;
    size_t cb = static_cast<size_t>(GetRandomDataWriteBufferSize());
    if (cb < 1)
    {
        return fOk;
    }

    // TODO: make sure the size if <= max value for size_t
    if (GetUseLargePages())
    {
        size_t cbMinLargePage = GetLargePageMinimum();
        size_t cbRoundedSize = (cb + cbMinLargePage - 1) & ~(cbMinLargePage - 1);
        _pRandomDataWriteBuffer = (BYTE*)VirtualAlloc(nullptr, cbRoundedSize, MEM_COMMIT | MEM_RESERVE | MEM_LARGE_PAGES, PAGE_READWRITE);
    }
    else
    {
        _pRandomDataWriteBuffer = (BYTE*)VirtualAlloc(nullptr, cb, MEM_COMMIT, PAGE_READWRITE);
    }

    fOk = (_pRandomDataWriteBuffer != nullptr);

    // Note: buffer ownership is NOT set here. The caller must own the buffer
    // on the original template Target until AFTER all copies have been distributed to
    // worker threads (to avoid double-free from default copy constructor).

    if (fOk)
    {
        Diagnostics::PrintVerbose("setup:      write source  %16p  %-26s  '%s'\n",
            _pRandomDataWriteBuffer,
            GetUseLargePages() ? "large page" : "source",
            GetPath().c_str());

        fOk = _FillRandomDataWriteBuffer(pRand);
    }

    return fOk;
}

BYTE* Target::GetRandomDataWriteBuffer(Random *pRand) const
{
    size_t cbBuffer = static_cast<size_t>(GetRandomDataWriteBufferSize());
    size_t cbBlock = GetBlockSizeInBytes();

    // leave enough bytes in the buffer for one block
    size_t randomOffset = pRand->Rand32() % (cbBuffer - (cbBlock - 1));

    bool fUnbufferedIO = (_cacheMode == TargetCacheMode::DisableOSCache);
    if (fUnbufferedIO)
    {
        // for unbuffered IO, offset in the buffer needs to be 512-byte aligned
        const size_t cbAlignment = 512;
        randomOffset -= (randomOffset % cbAlignment);
    }

    BYTE *pBuffer = reinterpret_cast<BYTE*>(reinterpret_cast<ULONG_PTR>(_pRandomDataWriteBuffer)+randomOffset);

    // unbuffered IO needs aligned addresses
    assert(!fUnbufferedIO || (reinterpret_cast<ULONG_PTR>(pBuffer) % 512 == 0));
    assert(pBuffer >= _pRandomDataWriteBuffer);
    assert(pBuffer <= _pRandomDataWriteBuffer + GetRandomDataWriteBufferSize() - GetBlockSizeInBytes());

    return pBuffer;
}

string Target::GetText(UINT32 indent, bool fUseThreadsPerFile, bool fUseRequestsPerFile, bool fCompletionRoutines, bool fUseIoRing) const
{
    string sText;
    string sIndent(indent, ' ');

    if (GetPath().c_str()[0] == TEMPLATE_TARGET_PREFIX)
    {
        sText += sIndent + "path: template target '" + string(GetPath().c_str() + 1) + "'\n";
    }
    else
    {
        sText += sIndent + "path: '" + GetPath() + "'\n";
    }

    // target properties indented 2 more than path
    string sPropIndent(indent + 2, ' ');
    char buf[256];

    sprintf_s(buf, _countof(buf), "think time: %ums\n", GetThinkTime());
    sText += sPropIndent + buf;

    sprintf_s(buf, _countof(buf), "burst size: %u\n", GetBurstSize());
    sText += sPropIndent + buf;

    // TODO: completion routines/ports

    switch (GetCacheMode())
    {
        case TargetCacheMode::Cached:
            sText += sPropIndent + "using software cache\n";
            break;
        case TargetCacheMode::DisableLocalCache:
            sText += sPropIndent + "local software cache disabled, remote cache enabled\n";
            break;
        case TargetCacheMode::DisableOSCache:
            sText += sPropIndent + "software cache disabled\n";
            break;
    }

    if (GetWriteThroughMode() == WriteThroughMode::On)
    {
        // context-appropriate comment on writethrough
        // if sw cache is disabled, commenting on sw write cache is possibly confusing
        switch (GetCacheMode())
        {
        case TargetCacheMode::Cached:
        case TargetCacheMode::DisableLocalCache:
            sText += sPropIndent + "hardware and software write caches disabled, writethrough on\n";
            break;
        case TargetCacheMode::DisableOSCache:
            sText += sPropIndent + "hardware write cache disabled, writethrough on\n";
            break;
        }
    }
    else
    {
        sText += sPropIndent + "using hardware write cache, writethrough off\n";
    }

    if (GetMemoryMappedIoMode() == MemoryMappedIoMode::On)
    {
        string sLine = sPropIndent + "memory mapped I/O enabled";
        switch(GetMemoryMappedIoFlushMode())
        {
        case MemoryMappedIoFlushMode::ViewOfFile:
            sLine += ", flush mode: FlushViewOfFile";
            break;
        case MemoryMappedIoFlushMode::NonVolatileMemory:
            sLine += ", flush mode: FlushNonVolatileMemory";
            break;
        case MemoryMappedIoFlushMode::NonVolatileMemoryNoDrain:
            sLine += ", flush mode: FlushNonVolatileMemory with no drain";
            break;
        }
        sLine += "\n";
        sText += sLine;
    }

    if (GetBypassIoMode() != BypassIoMode::Undefined)
    {
        switch (GetBypassIoMode())
        {
        case BypassIoMode::Partial:
            sText += sPropIndent + "using BypassIO (allow partial)\n";
            break;
        case BypassIoMode::Full:
            sText += sPropIndent + "using BypassIO (full bypass)\n";
            break;
        }
    }

    if (GetZeroWriteBuffers())
    {
        sText += sPropIndent + "zeroing write buffers\n";
    }

    if (GetRandomDataWriteBufferSize() > 0)
    {
        sText += sPropIndent + "write buffer size: " + Util::GetSizeKMGT(GetRandomDataWriteBufferSize()) + "\n";

        string sWriteBufferSourcePath = GetRandomDataWriteBufferSourcePath();
        if (!sWriteBufferSourcePath.empty())
        {
            sText += sPropIndent + "write buffer source: '" + sWriteBufferSourcePath + "'\n";
        }
        else
        {
            sText += sPropIndent + "write buffer source: random fill\n";
        }
    }

    if (GetUseParallelAsyncIO())
    {
        sText += sPropIndent + "using parallel async I/O\n";
    }

    if (GetWriteRatio() == 0)
    {
        sText += sPropIndent + "performing read test\n";
    }
    else if (GetWriteRatio() == 100)
    {
        sText += sPropIndent + "performing write test\n";
    }
    else
    {
        sprintf_s(buf, _countof(buf), "performing mix test (read/write ratio: %d/%d)\n", 100 - GetWriteRatio(), GetWriteRatio());
        sText += sPropIndent + buf;
    }

    sText += sPropIndent + "block size: " + Util::GetSizeKMGT(GetBlockSizeInBytes()) + "\n";

    if (GetRandomRatio() == 100)
    {
        sText += sPropIndent + "using random I/O (alignment: " + Util::GetSizeKMGT(GetBlockAlignmentInBytes()) + ")\n";
    }
    else
    {
        if (GetRandomRatio() > 0)
        {
            sprintf_s(buf, _countof(buf), "using mixed random/sequential I/O (%u%% random) (alignment/stride: ", GetRandomRatio());
            sText += sPropIndent + buf + Util::GetSizeKMGT(GetBlockAlignmentInBytes()) + ")\n";
        }
        else
        {
            sText += sPropIndent + "using" + string(GetUseInterlockedSequential() ? " interlocked" : "") +
                     " sequential I/O (stride: " + Util::GetSizeKMGT(GetBlockAlignmentInBytes()) + ")\n";
        }
    }

    if (fUseRequestsPerFile)
    {
        sprintf_s(buf, _countof(buf), "number of outstanding I/O operations per thread: %d\n", GetRequestCount());
        sText += sPropIndent + buf;
    }
    else
    {
        sprintf_s(buf, _countof(buf), "relative IO weight in thread pool: %u\n", GetWeight());
        sText += sPropIndent + buf;
    }

    if (0 != GetBaseFileOffsetInBytes())
    {
        sText += sPropIndent + "base file offset: " + Util::GetSizeKMGT(GetBaseFileOffsetInBytes()) + "\n";
    }

    if (0 != GetMaxFileSize())
    {
        sText += sPropIndent + "max file size: " + Util::GetSizeKMGT(GetMaxFileSize()) + "\n";
    }

    if (0 != GetThreadStrideInBytes())
    {
        sText += sPropIndent + "thread stride size: " + Util::GetSizeKMGT(GetThreadStrideInBytes()) + "\n";
    }

    if (GetSequentialScanHint())
    {
        sText += sPropIndent + "using FILE_FLAG_SEQUENTIAL_SCAN hint\n";
    }

    if (GetRandomAccessHint())
    {
        sText += sPropIndent + "using FILE_FLAG_RANDOM_ACCESS hint\n";
    }

    if (GetTemporaryFileHint())
    {
        sText += sPropIndent + "using FILE_ATTRIBUTE_TEMPORARY hint\n";
    }

    if (fUseThreadsPerFile)
    {
        sprintf_s(buf, _countof(buf), "threads per file: %d\n", GetThreadsPerFile());
        sText += sPropIndent + buf;
    }
    if (GetRequestCount() > 1 && fUseThreadsPerFile && !fUseIoRing)
    {
        if (fCompletionRoutines)
        {
            sText += sPropIndent + "using completion routines (ReadFileEx/WriteFileEx)\n";
        }
        else
        {
            sText += sPropIndent + "using I/O Completion Ports\n";
        }
    }

    if (GetIOPriorityHint() == IoPriorityHintVeryLow)
    {
        sText += sPropIndent + "IO priority: very low\n";
    }
    else if (GetIOPriorityHint() == IoPriorityHintLow)
    {
        sText += sPropIndent + "IO priority: low\n";
    }
    else if (GetIOPriorityHint() == IoPriorityHintNormal)
    {
        sText += sPropIndent + "IO priority: normal\n";
    }
    else
    {
        sText += sPropIndent + "IO priority: unknown\n";
    }

    if (GetThroughputIOPS())
    {
        sprintf_s(buf, _countof(buf), "throughput rate-limited to %u IOPS\n", GetThroughputIOPS());
        sText += sPropIndent + buf;
    }
    else if (GetThroughputInBytesPerMillisecond())
    {
        sprintf_s(buf, _countof(buf), "throughput rate-limited to %u B/ms\n", GetThroughputInBytesPerMillisecond());
        sText += sPropIndent + buf;
    }

    if (!GetDistribution().IsEmpty())
    {
        sText += sPropIndent + "IO Distribution:\n";
        sText += GetDistribution().GetText(indent + 2);
    }

    return sText;
}

string TimeSpan::GetText(UINT32 indent) const
{
    string sText;
    string sIndent(indent, ' ');
    char buf[256];

    sprintf_s(buf, _countof(buf), "duration: %us\n", GetDuration());
    sText += sIndent + buf;

    sprintf_s(buf, _countof(buf), "warm up time: %us\n", GetWarmup());
    sText += sIndent + buf;

    sprintf_s(buf, _countof(buf), "cool down time: %us\n", GetCooldown());
    sText += sIndent + buf;

    if (GetMeasureLatency())
    {
        sText += sIndent + "measuring latency\n";
    }
    if (GetCalculateIopsStdDev())
    {
        sprintf_s(buf, _countof(buf), "gathering IOPS at intervals of %ums\n", GetIoBucketDurationInMilliseconds());
        sText += sIndent + buf;
    }

    sprintf_s(buf, _countof(buf), "random seed: %u\n", GetRandSeed());
    sText += sIndent + buf;

    if (GetThreadCount() != 0)
    {
        sprintf_s(buf, _countof(buf), "thread pool with %u threads\n", GetThreadCount());
        sText += sIndent + buf;
        sprintf_s(buf, _countof(buf), "number of outstanding I/O operations per thread: %d\n", GetRequestCount());
        sText += sIndent + buf;
    }

    if (GetUseIoRing())
    {
        sprintf_s(buf, _countof(buf), "using IoRing (batch size: %u)\n", GetIoRingBatchSize());
        sText += sIndent + buf;
    }
    if (GetUseRegBuffer())
    {
        sText += sIndent + "using IoRing registered buffers\n";
    }

    if (GetDisableAffinity())
    {
        sText += sIndent + "affinity disabled\n";
    }
    else
    {
        switch (GetAffinityTraversal())
        {
        case AffinityTraversal::CoreAware:
            sText += sIndent + "affinity assignment: core-aware";
            break;
        case AffinityTraversal::Cpu:
            sText += sIndent + "affinity assignment: cpu order";
            break;
        default:
            assert(false);
            break;
        }

        switch (GetAffinityGroupSpan())
        {
        case AffinityGroupSpan::Fill:
            sText += ", fill groups";
            break;
        case AffinityGroupSpan::Span:
            sText += ", span groups";
            break;
        default:
            assert(false);
            break;
        }

        switch (GetAffinityEfficiencyOrder())
        {
        case AffinityEfficiencyOrder::Unspecified:
        case AffinityEfficiencyOrder::PFirst:
            sText += ", P-cores first\n";
            break;
        case AffinityEfficiencyOrder::Unordered:
            sText += ", no P/E order\n";
            break;
        case AffinityEfficiencyOrder::EFirst:
            sText += ", E-cores first\n";
            break;
        case AffinityEfficiencyOrder::FillPFirst:
            sText += ", fill P-cores first\n";
            break;
        case AffinityEfficiencyOrder::FillEFirst:
            sText += ", fill E-cores first\n";
            break;
        default:
            assert(false);
            break;
        }
    }

    const auto& vMasks = GetAffinityGroupMasks();
    if (vMasks.size() > 0)
    {
        bool fMultiGroup = _pSystem->processorTopology._vProcessorGroupInformation.size() > 1;
        string sLine = sIndent + (fMultiGroup ? "affinity (group/cpu): " : "affinity (cpu): ");
        for (size_t x = 0; x < vMasks.size(); ++x)
        {
            if (fMultiGroup)
            {
                sprintf_s(buf, _countof(buf), "%u/", vMasks[x].wGroup);
                sLine += buf;
            }
            if (vMasks[x].mask == 0)
            {
                sLine += "*";
            }
            else
            {
                sLine += Util::MaskRanges(vMasks[x].mask);
            }
            if (x < vMasks.size() - 1)
            {
                sLine += " ";
            }
        }
        sLine += "\n";
        sText += sLine;
    }

    switch (GetBufferSeparation())
    {
    case BufferSeparation::SystemDefault:
        sText += sIndent + "buffer separation: system default\n";
        break;
    case BufferSeparation::PDECacheLine:
        assert(IsFinalized());
        sText += sIndent + "buffer separation: thread optimized (" + Util::GetSizeKMGT(GetEffectiveBufferSeparation()) + ")\n";
        break;
    }

    if (IsCompletionDepthExplicit())
    {
        sprintf_s(buf, _countof(buf), "completion depth: %u\n", GetCompletionDepth());
        sText += sIndent + buf;
    }

    if (GetRandomWriteData())
    {
        sText += sIndent + "generating random data for each write IO\n";
        sText += sIndent + "  WARNING: this increases the CPU cost of issuing writes and should only\n";
        sText += sIndent + "           be compared to other results using the -Zr flag\n";
    }

    vector<Target> vTargets(GetTargets());
    for (auto i = vTargets.begin(); i != vTargets.end(); i++)
    {
        sText += i->GetText(indent,
                           (GetThreadCount() == 0),
                           (GetThreadCount() == 0 || GetRequestCount() == 0),
                           GetCompletionRoutines(),
                           GetUseIoRing());
    }

    return sText;
}

string TimeSpan::GetXml(UINT32 indent) const
{
    string sXml;
    char buffer[4096];

    AddXmlInc(sXml, "<TimeSpan>\n");
    AddXml(sXml, _fCompletionRoutines ? "<CompletionRoutines>true</CompletionRoutines>\n" : "<CompletionRoutines>false</CompletionRoutines>\n");
    AddXml(sXml, _fMeasureLatency ? "<MeasureLatency>true</MeasureLatency>\n" : "<MeasureLatency>false</MeasureLatency>\n");
    AddXml(sXml, _fCalculateIopsStdDev ? "<CalculateIopsStdDev>true</CalculateIopsStdDev>\n" : "<CalculateIopsStdDev>false</CalculateIopsStdDev>\n");

    sprintf_s(buffer, _countof(buffer), "<Duration>%u</Duration>\n", _ulDuration);
    AddXml(sXml, buffer);

    sprintf_s(buffer, _countof(buffer), "<Warmup>%u</Warmup>\n", _ulWarmUp);
    AddXml(sXml, buffer);

    sprintf_s(buffer, _countof(buffer), "<Cooldown>%u</Cooldown>\n", _ulCoolDown);
    AddXml(sXml, buffer);

    sprintf_s(buffer, _countof(buffer), "<ThreadCount>%u</ThreadCount>\n", _dwThreadCount);
    AddXml(sXml, buffer);

    sprintf_s(buffer, _countof(buffer), "<RequestCount>%u</RequestCount>\n", _dwRequestCount);
    AddXml(sXml, buffer);

    sprintf_s(buffer, _countof(buffer), "<IoBucketDuration>%u</IoBucketDuration>\n", _ulIoBucketDurationInMilliseconds);
    AddXml(sXml, buffer);

    sprintf_s(buffer, _countof(buffer), "<RandSeed>%u</RandSeed>\n", _ulRandSeed);
    AddXml(sXml, buffer);

    if (_fUseIoRing)
    {
        AddXmlInc(sXml, "<IoRing>\n");
        sprintf_s(buffer, _countof(buffer), "<IoRingBatchSize>%u</IoRingBatchSize>\n", _ulIoRingBatchSize);
        AddXml(sXml, buffer);
        AddXml(sXml, _fUseRegBuffer ? "<UseRegBuffer>true</UseRegBuffer>\n" : "<UseRegBuffer>false</UseRegBuffer>\n");
        AddXmlDec(sXml, "</IoRing>\n");
    }

    AddXml(sXml, _fDisableAffinity ? "<DisableAffinity>true</DisableAffinity>\n" : "<DisableAffinity>false</DisableAffinity>\n");

    if (!_fDisableAffinity)
    {
        // AffinityTraversal with Group and Efficiency attributes
        const char *pszTraversal = "";
        switch (GetAffinityTraversal())
        {
        case AffinityTraversal::CoreAware: pszTraversal = "CoreAware"; break;
        case AffinityTraversal::Cpu: pszTraversal = "Cpu"; break;
        default: assert(false); break;
        }

        const char *pszGroup = "";
        switch (GetAffinityGroupSpan())
        {
        case AffinityGroupSpan::Fill: pszGroup = "Fill"; break;
        case AffinityGroupSpan::Span: pszGroup = "Span"; break;
        default: assert(false); break;
        }

        const char *pszEfficiency = "";
        switch (GetAffinityEfficiencyOrder())
        {
        case AffinityEfficiencyOrder::PFirst: pszEfficiency = "PFirst"; break;
        case AffinityEfficiencyOrder::Unordered: pszEfficiency = "Unordered"; break;
        case AffinityEfficiencyOrder::EFirst: pszEfficiency = "EFirst"; break;
        case AffinityEfficiencyOrder::FillPFirst: pszEfficiency = "FillPFirst"; break;
        case AffinityEfficiencyOrder::FillEFirst: pszEfficiency = "FillEFirst"; break;
        default: assert(false); break;
        }

        sprintf_s(buffer, _countof(buffer), "<AffinityTraversal Group=\"%s\" Efficiency=\"%s\">%s</AffinityTraversal>\n",
            pszGroup, pszEfficiency, pszTraversal);
        AddXml(sXml, buffer);

        if (_vAffinityMasks.size() > 0)
        {
            AddXmlInc(sXml, "<Affinity>\n");
            for (const auto& gm : _vAffinityMasks)
            {
                sprintf_s(buffer, _countof(buffer), "<Group Group=\"%u\" Mask=\"0x%Ix\"/>\n", gm.wGroup, gm.mask);
                AddXml(sXml, buffer);
            }
            AddXmlDec(sXml, "</Affinity>\n");
        }
    }

    AddXml(sXml, "<BufferSeparation>");
    switch (_bufferSeparation)
    {
    case BufferSeparation::SystemDefault:
        sXml += "SystemDefault";
        break;
    case BufferSeparation::PDECacheLine:
        sXml += "PDECacheLine";
        break;
    }
    sXml += "</BufferSeparation>\n";

    if (_fCompletionDepthExplicit)
    {
        sprintf_s(buffer, _countof(buffer), "<CompletionDepth>%u</CompletionDepth>\n", _dwCompletionDepth);
        AddXml(sXml, buffer);
    }

    AddXmlInc(sXml, "<Targets>\n");
    for (const auto& target : _vTargets)
    {
        sXml += target.GetXml(indent);
    }
    AddXmlDec(sXml, "</Targets>\n");
    AddXmlDec(sXml, "</TimeSpan>\n");
    return sXml;
}

void TimeSpan::MarkFilesAsPrecreated(const vector<string> vFiles)
{
    for (auto sFile : vFiles)
    {
        for (auto pTarget = _vTargets.begin(); pTarget != _vTargets.end(); pTarget++)
        {
            if (sFile == pTarget->GetPath())
            {
                pTarget->SetPrecreated(true);
            }
        }
    }
}

string Profile::GetXml(UINT32 indent) const
{
    string sXml;
    char buffer[4096];

    AddXmlInc(sXml, "<Profile>\n");

    sprintf_s(buffer, _countof(buffer), "<Progress>%u</Progress>\n", _dwProgress);
    AddXml(sXml, buffer);

    if (g_ExperimentFlags)
    {
        // only output if on so that downlevel doesn't get (and fail: not in downlevel xsd) unless actually specified
        sprintf_s(buffer, _countof(buffer), "<ExperimentFlags>%u</ExperimentFlags>\n", g_ExperimentFlags);
        AddXml(sXml, buffer);
    }

    if (_resultsFormat == ResultsFormat::Text)
    {
        AddXml(sXml, "<ResultFormat>text</ResultFormat>\n");
    }
    else if (_resultsFormat == ResultsFormat::Xml)
    {
        AddXml(sXml, "<ResultFormat>xml</ResultFormat>\n");
    }
    else
    {
        AddXml(sXml, "<ResultFormat>* UNSUPPORTED *</ResultFormat>\n");
    }

    AddXml(sXml, _fVerbose ? "<Verbose>true</Verbose>\n" : "<Verbose>false</Verbose>\n");
    if (_fVerboseStats)
    {
        // only output if on so that downlevel doesn't get (and fail: not in downlevel xsd) unless actually specified
        AddXml(sXml, "<VerboseStats>true</VerboseStats>\n");
    }

    if (_precreateFiles == PrecreateFiles::UseMaxSize)
    {
        AddXml(sXml, "<PrecreateFiles>UseMaxSize</PrecreateFiles>\n");
    }
    else if (_precreateFiles == PrecreateFiles::OnlyFilesWithConstantSizes)
    {
        AddXml(sXml, "<PrecreateFiles>CreateOnlyFilesWithConstantSizes</PrecreateFiles>\n");
    }
    else if (_precreateFiles == PrecreateFiles::OnlyFilesWithConstantOrZeroSizes)
    {
        AddXml(sXml, "<PrecreateFiles>CreateOnlyFilesWithConstantOrZeroSizes</PrecreateFiles>\n");
    }

    if (_fEtwEnabled)
    {
        AddXmlInc(sXml, "<ETW>\n");
        AddXml(sXml, _fEtwProcess ? "<Process>true</Process>\n" : "<Process>false</Process>\n");
        AddXml(sXml, _fEtwThread ? "<Thread>true</Thread>\n" : "<Thread>false</Thread>\n");
        AddXml(sXml, _fEtwImageLoad ? "<ImageLoad>true</ImageLoad>\n" : "<ImageLoad>false</ImageLoad>\n");
        AddXml(sXml, _fEtwDiskIO ? "<DiskIO>true</DiskIO>\n" : "<DiskIO>false</DiskIO>\n");
        AddXml(sXml, _fEtwMemoryPageFaults ? "<MemoryPageFaults>true</MemoryPageFaults>\n" : "<MemoryPageFaults>false</MemoryPageFaults>\n");
        AddXml(sXml, _fEtwMemoryHardFaults ? "<MemoryHardFaults>true</MemoryHardFaults>\n" : "<MemoryHardFaults>false</MemoryHardFaults>\n");
        AddXml(sXml, _fEtwNetwork ? "<Network>true</Network>\n" : "<Network>false</Network>\n");
        AddXml(sXml, _fEtwRegistry ? "<Registry>true</Registry>\n" : "<Registry>false</Registry>\n");
        AddXml(sXml, _fEtwUsePagedMemory ? "<UsePagedMemory>true</UsePagedMemory>\n" : "<UsePagedMemory>false</UsePagedMemory>\n");
        AddXml(sXml, _fEtwUsePerfTimer ? "<UsePerfTimer>true</UsePerfTimer>\n" : "<UsePerfTimer>false</UsePerfTimer>\n");
        AddXml(sXml, _fEtwUseSystemTimer ? "<UseSystemTimer>true</UseSystemTimer>\n" : "<UseSystemTimer>false</UseSystemTimer>\n");
        AddXml(sXml, _fEtwUseCyclesCounter ? "<UseCyclesCounter>true</UseCyclesCounter>\n" : "<UseCyclesCounter>false</UseCyclesCounter>\n");
        AddXmlDec(sXml, "</ETW>\n");
    }

    AddXmlInc(sXml, "<TimeSpans>\n");
    for (const auto& timespan : _vTimeSpans)
    {
        sXml += timespan.GetXml(indent);
    }
    AddXmlDec(sXml, "</TimeSpans>\n");
    AddXmlDec(sXml, "</Profile>\n");
    return sXml;
}

void Profile::MarkFilesAsPrecreated(const vector<string> vFiles)
{
    for (auto pTimeSpan = _vTimeSpans.begin(); pTimeSpan != _vTimeSpans.end(); pTimeSpan++)
    {
        pTimeSpan->MarkFilesAsPrecreated(vFiles);
    }
}

bool Profile::Validate(bool fSingleSpec, SystemInformation *pSystem) const
{
    bool fOk = true;

    if (GetTimeSpans().size() == 0)
    {
        fprintf(stderr, "ERROR: no timespans specified\n");
        fOk = false;
    }
    else
    {
        for (const auto& timeSpan : GetTimeSpans())
        {
            // Validate affinity group mask assignments
            if (pSystem != nullptr)
            {
                for (const auto& gm : timeSpan.GetAffinityGroupMasks())
                {
                    if (gm.wGroup >= pSystem->processorTopology._vProcessorGroupInformation.size())
                    {
                        fprintf(stderr, "ERROR: affinity mask specifies group %u; system only has %u groups\n",
                            gm.wGroup,
                            (int) pSystem->processorTopology._vProcessorGroupInformation.size());

                        fOk = false;
                    }

                    if (fOk && gm.mask != 0)
                    {
                        KAFFINITY activeMask = pSystem->processorTopology._vProcessorGroupInformation[gm.wGroup]._activeProcessorMask;
                        KAFFINITY invalidBits = gm.mask & ~activeMask;
                        if (invalidBits != 0)
                        {
                            fprintf(stderr, "ERROR: affinity mask 0x%Ix for group %u specifies inactive processors (active mask 0x%Ix)\n",
                                gm.mask, gm.wGroup, activeMask);
                            fOk = false;
                        }
                    }
                }
            }

            // Validate BufferSeparation: if non-default, resolve VirtualAlloc2.
            if (timeSpan.GetBufferSeparation() != BufferSeparation::SystemDefault)
            {
                if (!ResolveVirtualAlloc2())
                {
                    if (timeSpan.IsBufferSeparationExplicit())
                    {
                        fprintf(stderr, "ERROR: buffer separation requires VirtualAlloc2 which is not available on this system\n");
                        fOk = false;
                    }
                    // If not explicitly set, Finalize will
                    // fall back to alignment 0 (system default) when
                    // VirtualAlloc2 is unavailable.
                }

                // When the user explicitly specifies -bsp alongside -l, warn that
                // large pages take priority for IO buffers. Don't warn when buffer
                // separation is just the default - that would disrupt pre-existing
                // workflows that use -l.
                if (fSingleSpec && timeSpan.IsBufferSeparationExplicit())
                {
                    for (const auto& target : timeSpan.GetTargets())
                    {
                        if (target.GetUseLargePages())
                        {
                            fprintf(stderr, "WARNING: large pages (-l) take priority over buffer separation (-bsp) for IO buffers;\n"
                                            "         random write source buffers (-Z) will still use buffer separation\n");
                            break;
                        }
                    }
                }
            }

            // Validate CompletionDepth range and conflicts
            if (timeSpan.IsCompletionDepthExplicit())
            {
                if (timeSpan.GetCompletionDepth() < 1 || timeSpan.GetCompletionDepth() > c_maximumCompletionDepth)
                {
                    fprintf(stderr, "ERROR: completion depth must be between 1 and %u\n", c_maximumCompletionDepth);
                    fOk = false;
                }

                if (timeSpan.GetCompletionRoutines())
                {
                    fprintf(stderr, "ERROR: completion depth (-oc) cannot be used with completion routines (-x)\n");
                    fOk = false;
                }

                // CompletionDepth is only used by IO Completion Ports; all-mapped-IO
                // uses synchronous IO instead.
                bool fAllMappedIo = true;
                for (const auto& target : timeSpan.GetTargets())
                {
                    if (target.GetMemoryMappedIoMode() != MemoryMappedIoMode::On)
                    {
                        fAllMappedIo = false;
                        break;
                    }
                }
                if (fAllMappedIo)
                {
                    fprintf(stderr, "ERROR: completion depth (-oc) cannot be used when all targets use memory mapped I/O (-Sm)\n");
                    fOk = false;
                }
            }

            // ISSUE: many of the following validation errors are stated in cmdline terms, which is not helpful for XML

            if (timeSpan.GetDisableAffinity() &&
                (timeSpan.GetAffinityGroupMasks().size() > 0 ||
                 timeSpan.GetAffinityTraversal(false) != AffinityTraversal::Unspecified ||
                 timeSpan.GetAffinityGroupSpan(false) != AffinityGroupSpan::Unspecified ||
                 timeSpan.GetAffinityEfficiencyOrder(false) != AffinityEfficiencyOrder::Unspecified))
            {
                fprintf(stderr, "ERROR: -n and -a parameters cannot be used together\n");
                fOk = false;
            }

            if (timeSpan.GetUseIoRing() && timeSpan.GetCompletionRoutines())
            {
                fprintf(stderr, "ERROR: IoRing (-u) can't be used with completion routines (-x)\n");
                fOk = false;
            }

            if (timeSpan.GetRequestCount() > c_maximumRequestCount)
            {
                fprintf(stderr, "ERROR: outstanding request count (-O) of %u exceeds maximum of %u\n",
                    timeSpan.GetRequestCount(), c_maximumRequestCount);
                fOk = false;
            }

            // ISSUE: with XML and the following the target specification validation it would be useful to say what
            //      target they're for

            for (const auto& target : timeSpan.GetTargets())
            {
                const bool targetHasMultipleThreads = (timeSpan.GetThreadCount() > 1) || (target.GetThreadsPerFile() > 1);

                if (timeSpan.GetThreadCount() > 0 && target.GetThreadsPerFile() > 1)
                {
                    fprintf(stderr, "ERROR: -F and -t parameters cannot be used together\n");
                    fOk = false;
                }

                if (target.GetThroughputInBytesPerMillisecond() > 0 && timeSpan.GetCompletionRoutines())
                {
                    fprintf(stderr, "ERROR: -g throughput control cannot be used with -x completion routines\n");
                    fOk = false;
                }

                //  If burst size is specified think time must be specified and If think time is specified burst size should be non zero
                if ((target.GetThinkTime() == 0 && target.GetBurstSize() > 0) || (target.GetThinkTime() > 0 && target.GetBurstSize() == 0))
                {
                    fprintf(stderr, "ERROR: need to specify -j<think time> with -i<burst size>\n");
                    fOk = false;
                }

                if (timeSpan.GetThreadCount() > 0 && timeSpan.GetRequestCount() > 0)
                {
                    if (target.GetThroughputInBytesPerMillisecond() > 0)
                    {
                        fprintf(stderr, "ERROR: -g throughput control cannot be used with -O outstanding requests per thread\n");
                        fOk = false;
                    }

                    if (target.GetThinkTime() > 0)
                    {
                        fprintf(stderr, "ERROR: -j think time cannot be used with -O outstanding requests per thread\n");
                        fOk = false;
                    }

                    if (target.GetUseParallelAsyncIO())
                    {
                        fprintf(stderr, "ERROR: -p parallel IO cannot be used with -O outstanding requests per thread\n");
                        fOk = false;
                    }

                    if (target.GetWeight() == 0)
                    {
                        fprintf(stderr, "ERROR: a non-zero target Weight must be specified\n");
                        fOk = false;
                    }

                    for (const auto& threadTarget : target.GetThreadTargets())
                    {
                        if (threadTarget.GetThread() >= timeSpan.GetThreadCount())
                        {
                            fprintf(stderr, "ERROR: illegal thread specified for ThreadTarget\n");
                            fOk = false;
                        }
                    }
                }
                else if (target.GetThreadTargets().size() != 0)
                {
                    fprintf(stderr, "ERROR: ThreadTargets can only be specified when the timespan ThreadCount and RequestCount are specified\n");
                    fOk = false;
                }

                if (target.GetRandomRatio())
                {
                    if (target.GetThreadStrideInBytes() > 0)
                    {
                        fprintf(stderr, "ERROR: -T conflicts with -r\n");
                        fOk = false;
                        // although ullThreadStride==0 is a valid value, it's interpreted as "not provided" for this warning
                    }

                    if (target.GetUseInterlockedSequential())
                    {
                        fprintf(stderr, "ERROR: -si conflicts with -r\n");
                        fOk = false;
                    }

                    if (target.GetUseParallelAsyncIO())
                    {
                        fprintf(stderr, "ERROR: -p conflicts with -r\n");
                        fOk = false;
                    }
                }
                else
                {
                    if (target.GetDistributionRange().size() != 0)
                    {
                        fprintf(stderr, "ERROR: random distribution ranges (-rd) do not apply to sequential-only IO patterns\n");
                        fOk = false;
                    }

                    if (target.GetUseParallelAsyncIO() && target.GetRequestCount() == 1)
                    {
                        fprintf(stderr, "WARNING: -p does not have effect unless outstanding I/O count (-o) is > 1\n");
                    }

                    if (target.GetRequestCount() > c_maximumRequestCount)
                    {
                        fprintf(stderr, "ERROR: per-target outstanding request count (-o) of %u exceeds maximum of %u\n",
                            target.GetRequestCount(), c_maximumRequestCount);
                        fOk = false;
                    }

                    if (target.GetUseInterlockedSequential())
                    {
                        if (target.GetThreadStrideInBytes() > 0)
                        {
                            fprintf(stderr, "ERROR: -si conflicts with -T\n");
                            fOk = false;
                        }

                        if (target.GetUseParallelAsyncIO())
                        {
                            fprintf(stderr, "ERROR: -si conflicts with -p\n");
                            fOk = false;
                        }

                        if (!targetHasMultipleThreads)
                        {
                            fprintf(stderr, "WARNING: single-threaded test, -si ignored\n");
                        }
                    }
                    else
                    {
                        if (targetHasMultipleThreads && !target.GetThreadStrideInBytes())
                        {
                            fprintf(stderr, "WARNING: target access pattern will not be sequential, consider -si\n");
                        }

                        if (!targetHasMultipleThreads && target.GetThreadStrideInBytes())
                        {
                            fprintf(stderr, "ERROR: -T has no effect unless multiple threads per target are used\n");
                            fOk = false;
                        }
                    }
                }

                // Distribution ranges are only applied to random loads. Note validation failure in the sequential case.
                if (!target.GetDistribution().Validate(target.GetBlockSizeInBytes()))
                {
                    fOk = false;
                }

                if (target.GetRandomDataWriteBufferSize() > 0)
                {
                    if (target.GetRandomDataWriteBufferSize() < target.GetBlockSizeInBytes())
                    {
                        fprintf(stderr, "ERROR: custom write buffer (-Z) is smaller than the block size. Write buffer size: %I64u block size: %u\n",
                            target.GetRandomDataWriteBufferSize(),
                            target.GetBlockSizeInBytes());
                        fOk = false;
                    }
                }

                if (target.GetMemoryMappedIoMode() == MemoryMappedIoMode::On)
                {
                    if (timeSpan.GetCompletionRoutines())
                    {
                        fprintf(stderr, "ERROR: completion routines (-x) can't be used with memory mapped IO (-Sm)\n");
                        fOk = false;
                    }
                    if (timeSpan.GetUseIoRing())
                    {
                        fprintf(stderr, "ERROR: IoRing (-u) can't be used with memory mapped IO (-Sm)\n");
                        fOk = false;
                    }
                    if (target.GetCacheMode() == TargetCacheMode::DisableOSCache)
                    {
                        fprintf(stderr, "ERROR: unbuffered IO (-Su or -Sh) can't be used with memory mapped IO (-Sm)\n");
                        fOk = false;
                    }
                }

                if (target.GetBypassIoMode() != BypassIoMode::Undefined)
                {
                    if (target.GetCacheMode() != TargetCacheMode::DisableOSCache)
                    {
                        fprintf(stderr, "ERROR: BypassIO (-SY or -Sy) requires unbuffered IO (-Su)\n");
                        fOk = false;
                    }
                    if (target.GetWriteRatio() > 0)
                    {
                        fprintf(stderr, "ERROR: BypassIO (-SY or -Sy) can't be used with write operations (-w)\n");
                        fOk = false;
                    }
                    if (target.GetMemoryMappedIoMode() == MemoryMappedIoMode::On)
                    {
                        fprintf(stderr, "ERROR: BypassIO (-SY or -Sy) can't be used with memory mapped IO (-Sm)\n");
                        fOk = false;
                    }
                }

                if (target.GetMemoryMappedIoMode() == MemoryMappedIoMode::Off &&
                    target.GetMemoryMappedIoFlushMode() != MemoryMappedIoFlushMode::Undefined)
                {
                    fprintf(stderr, "ERROR: memory mapped flush mode (-N) can only be specified with memory mapped IO (-Sm)\n");
                    fOk = false;
                }

                if (GetProfileOnly() == false)
                {
                    auto sPath = target.GetPath();

                    if (sPath[0] == TEMPLATE_TARGET_PREFIX)
                    {
                        fprintf(stderr, "ERROR: template target '%s' was not substituted - all template targets must be substituted to run a profile\n", sPath.c_str());
                        fOk = false;
                    }
                }

                // Note that this error is only possible with -f or XML. The -Bbase:length form is immune.
                if (target.GetMaxFileSize() && target.GetMaxFileSize() <= target.GetBaseFileOffsetInBytes())
                {
                    fprintf(stderr, "ERROR: maximum (-f) target offset must be greater than base (-B)\n");
                    fOk = false;
                }

                // If we know there is only a single target specification (the parameters which apply to targets) shared
                // across the one or more targets, we can stop. In practical terms this is the command line case - for
                // XML we don't know, and do need to keep going. This early exit lets us avoid repeating the same sets
                // of error messages per each target we would otherwise loop over.
                //
                // If we ever did target property validation (say, v. its size) we'd want to divide out the validations
                // into parameter-only v. parameter/property cases for similar reasons.
                if (fSingleSpec)
                {
                    break;
                }
            }

            // Cross-target validation: detect conflicting BypassIO modes for the
            // same target file. This can only happen in XML profiles (command line
            // produces a single target spec). When the same file appears in multiple
            // <Target> sections, handle deduplication causes them to share one handle,
            // and only the first target's BypassIO setting is applied. Detect and
            // reject this to prevent silent misconfiguration.
            if (!fSingleSpec)
            {
                map<string, BypassIoMode> bypassIoByPath;
                for (const auto& target : timeSpan.GetTargets())
                {
                    auto result = bypassIoByPath.emplace(target.GetPath(), target.GetBypassIoMode());
                    if (!result.second && result.first->second != target.GetBypassIoMode())
                    {
                        fprintf(stderr, "ERROR: conflicting BypassIO modes for target '%s' -- all targets sharing a file must use the same BypassIO mode\n",
                            target.GetPath().c_str());
                        fOk = false;
                    }
                }
            }
        }
    }

    return fOk;
}

bool ThreadParameters::AllocateAndFillBufferForTarget(Target& target)
{
    bool fOk = true;
    BYTE *pDataBuffer = nullptr;
    DWORD requestCount = target.GetRequestCount();
    size_t cbDataBuffer;
    DWORD dwAlignment = pTimeSpan->GetEffectiveBufferSeparation();

    // Use global request count
    if (pTimeSpan->GetThreadCount() != 0 &&
        pTimeSpan->GetRequestCount() != 0) {

        requestCount = pTimeSpan->GetRequestCount();
    }

    // Create separate read & write buffers so the write content doesn't get overriden by reads
    cbDataBuffer = (size_t) target.GetBlockSizeInBytes() * requestCount * 2;
    if (target.GetUseLargePages())
    {
        size_t cbMinLargePage = GetLargePageMinimum();
        size_t cbRoundedSize = (cbDataBuffer + cbMinLargePage - 1) & ~(cbMinLargePage - 1);
        pDataBuffer = (BYTE*)VirtualAlloc(nullptr, cbRoundedSize, MEM_COMMIT | MEM_RESERVE | MEM_LARGE_PAGES, PAGE_READWRITE);
    }
    else
    {
        // Effective buffer separation was finalized before IO generation began
        pDataBuffer = AllocateAlignedBuffer(cbDataBuffer, dwAlignment);
    }

    fOk = (pDataBuffer != nullptr);

    if (fOk && Diagnostics::GetVerbose())
    {
        string sMode;
        if (target.GetUseLargePages())
        {
            sMode = "large page";
        }
        else if (dwAlignment > 0)
        {
            sMode = "thread optimized (" + Util::GetSizeKMGT(dwAlignment) + ")";
        }
        else
        {
            sMode = "system default";
        }
        Diagnostics::PrintVerbose("thread %3u: IO buffer     %16p  %-26s  '%s'\n",
            ulThreadNo, pDataBuffer, sMode.c_str(), target.GetPath().c_str());
    }

    //fill buffer (useful only for write tests)
    if (fOk && target.GetWriteRatio() > 0)
    {
        if (target.GetZeroWriteBuffers())
        {
            memset(pDataBuffer, 0, cbDataBuffer);
        }
        else
        {
            for (size_t i = 0; i < cbDataBuffer; i++)
            {
                pDataBuffer[i] = (BYTE)(i % 256);
            }
        }
    }

    if (fOk)
    {
        vpDataBuffers.push_back(pDataBuffer);
        vulReadBufferSize.push_back(cbDataBuffer / 2);
    }

    // If buffer separation is active and this target has a shared write source
    // buffer (allocated on the setup thread), create a per-thread separated copy.
    // Large page targets are excluded since large pages use a different allocation
    // strategy that is incompatible with VirtualAlloc2 alignment.
    if (fOk && dwAlignment > 0 &&
        target.GetRandomDataWriteBufferSize() > 0 &&
        !target.GetUseLargePages())
    {
        size_t cb = static_cast<size_t>(target.GetRandomDataWriteBufferSize());
        BYTE *pSeparated = AllocateAlignedBuffer(cb, dwAlignment);
        if (pSeparated != nullptr)
        {
            memcpy(pSeparated, target.GetRandomDataWriteBuffer(), cb);
            target.SetRandomDataWriteBuffer(pSeparated);

            Diagnostics::PrintVerbose("thread %3u: write source  %16p  %-26s  '%s'\n",
                ulThreadNo, pSeparated,
                ("thread optimized (" + Util::GetSizeKMGT(dwAlignment) + ")").c_str(),
                target.GetPath().c_str());
        }
        else
        {
            fOk = false;
        }
    }

    return fOk;
}

BYTE* ThreadParameters::GetReadBuffer(size_t iTarget, size_t iRequest)
{
    return vpDataBuffers[iTarget] + (iRequest * vTargets[iTarget].GetBlockSizeInBytes());
}

BYTE* ThreadParameters::GetWriteBuffer(size_t iTarget, size_t iRequest)
{
    BYTE *pBuffer = nullptr;

    Target& target(vTargets[iTarget]);
    size_t cb = static_cast<size_t>(target.GetRandomDataWriteBufferSize());
    if (cb == 0)
    {
        pBuffer = vpDataBuffers[iTarget] + vulReadBufferSize[iTarget] + (iRequest * vTargets[iTarget].GetBlockSizeInBytes());

        //
        // This is a very efficient algorithm for generating random content at
        // run-time.  When tested in a single-threaded, CPU limited environment
        // with 4K random writes, doing memset to fill the buffer got 112K IOPS,
        // this algorithm got 111K IOPS.  Using a static buffer got 118K IOPS.
        // This was tested with a 64-bit diskspd.exe.  With a 32-bit version it
        // may be more efficient to do 32-bit operations.
        //

        if (pTimeSpan->GetRandomWriteData() &&
            !target.GetZeroWriteBuffers())
        {
            pRand->RandBuffer(pBuffer, vTargets[iTarget].GetBlockSizeInBytes(), true);
        }
    }
    else
    {
        pBuffer = target.GetRandomDataWriteBuffer(pRand);
    }
    return pBuffer;
}

bool ThreadParameters::InitializeMappedViewForTarget(Target& target, DWORD DesiredAccess)
{
    bool fOk = true;
    DWORD dwProtect = PAGE_READWRITE;

    if (DesiredAccess == GENERIC_READ)
    {
        dwProtect = PAGE_READONLY;
    }

    HANDLE hFile = CreateFileMapping(target.GetMappedViewFileHandle(), NULL, dwProtect, 0, 0, NULL);
    fOk = (hFile != NULL);
    if (fOk)
    {
        DWORD dwDesiredAccess = FILE_MAP_WRITE;

        if (DesiredAccess == GENERIC_READ)
        {
            dwDesiredAccess = FILE_MAP_READ;
        }

        BYTE *mapView = (BYTE*) MapViewOfFile(hFile, dwDesiredAccess, 0, 0, 0);
        fOk = (mapView != NULL);
        if (fOk)
        {
            target.SetMappedView(mapView);
        }
        else
        {
            fprintf(stderr, "FATAL ERROR: Could not map view for target '%s'. Error code: 0x%x\n", target.GetPath().c_str(), GetLastError());
        }
    }
    else
    {
        fprintf(stderr, "FATAL ERROR: Could not create a file mapping for target '%s'. Error code: 0x%x\n", target.GetPath().c_str(), GetLastError());
    }
    return fOk;
}

DWORD ThreadParameters::GetTotalRequestCount() const
{
    DWORD cRequests = 0;

    if (pTimeSpan->GetRequestCount() != 0 &&
        pTimeSpan->GetThreadCount() != 0)
    {
        cRequests = pTimeSpan->GetRequestCount();
    }
    else
    {
        for (const auto& t : vTargets)
        {
            cRequests += t.GetRequestCount();
        }
    }

    return cRequests;
}

DWORD ThreadParameters::GetTargetRequestCount(const Target& target) const
{
    DWORD cRequests = 0;

    if (pTimeSpan->GetRequestCount() != 0 &&
        pTimeSpan->GetThreadCount() != 0)
    {
        cRequests = pTimeSpan->GetRequestCount();
    }
    else
    {
        cRequests = target.GetRequestCount();
    }

    return cRequests;
}

size_t ThreadParameters::GetTargetBufferLength(const Target& target) const
{
    // Create separate read & write buffers so the write content doesn't get overriden by reads
    return (size_t)target.GetBlockSizeInBytes() * GetTargetRequestCount(target) * 2;
}

IoRing::IoRing() :
    _tp(NULL),
    _hIoRing(NULL),
    _useRegBuffer(false),
    _pBufferInfo(NULL),
    _bufferCount(0)
{
}

//
// Initialize IoRing for this thread
//
HRESULT IoRing::Initialize(ThreadParameters* pThreadParameters)
{
    HRESULT hr = S_OK;
    IORING_CREATE_FLAGS createFlags = {IORING_CREATE_REQUIRED_FLAGS_NONE, IORING_CREATE_SKIP_BUILDER_PARAM_CHECKS};
    IORING_BUFFER_INFO* bufferInfo = NULL;
    IORING_CQE cqe = {0};

    _tp = pThreadParameters;
    _useRegBuffer = _tp->pTimeSpan->GetUseRegBuffer();

    if (s_pfnCreateIoRing == nullptr)
    {
        fprintf(stderr, "FATAL ERROR: IoRing APIs not loaded\n");
        return E_FAIL;
    }

    // Create IoRing handle
    hr = s_pfnCreateIoRing(IORING_VERSION_3, createFlags, _tp->GetTotalRequestCount(), 0, &_hIoRing);

    if (!SUCCEEDED(hr))
    {
        fprintf(stderr, "FATAL ERROR: Could not create IoRing in thread %u (hresult: 0x%08x)\n", _tp->ulThreadNo, hr);
        goto cleanup;
    }

    // Register buffers
    if (_tp->pTimeSpan->GetUseRegBuffer())
    {
        UINT32 count = (UINT32)_tp->vpDataBuffers.size();
        bufferInfo = new IORING_BUFFER_INFO[count];

        if (bufferInfo == NULL)
        {
            hr = E_OUTOFMEMORY;
            fprintf(stderr, "FATAL ERROR: Could not allocate memory for RegBuffers in thread %u\n", _tp->ulThreadNo);
            goto cleanup;
        }

        // Build array of buffer descriptors for registration
        for (UINT32 i = 0; i < count; i++)
        {
            size_t length = _tp->GetTargetBufferLength(_tp->vTargets[i]);

            if (length > MAXUINT32)
            {
                fprintf(stderr,
                        "FATAL ERROR: Buffer too long (0x%I64x) to register with IoRing (max length: 0x%X), in thread %u\n",
                        (UINT64)length,
                        MAXUINT32,
                        _tp->ulThreadNo);

                hr = HRESULT_FROM_WIN32(ERROR_BUFFER_OVERFLOW);
                goto cleanup;
            }

            bufferInfo[i].Address = _tp->vpDataBuffers[i];
            bufferInfo[i].Length = (UINT32)length;
        }

        // Build the buffer registration operation
        hr = s_pfnBuildIoRingRegisterBuffers(_hIoRing, count, bufferInfo, 0);

        if (!SUCCEEDED(hr))
        {
            fprintf(stderr,
                    "FATAL ERROR: Could not build register buffer request for IoRing, in thread %u (hresult: 0x%08x)\n",
                    _tp->ulThreadNo,
                    hr);

            goto cleanup;
        }

        // Submit the registration operation and wait for completion
        hr = s_pfnSubmitIoRing(_hIoRing, 1, INFINITE, NULL);

        if (!SUCCEEDED(hr))
        {
            fprintf(stderr,
                    "FATAL ERROR: Could not submit register buffer request to IoRing, in thread %u (hresult: 0x%08x)\n",
                    _tp->ulThreadNo,
                    hr);

            goto cleanup;
        }

        // Retrieve the completion entry for the registration operation
        hr = s_pfnPopIoRingCompletion(_hIoRing, &cqe);

        if (hr != S_OK)
        {
            fprintf(stderr,
                    "FATAL ERROR: Could not pop the completion entry for IoRing buffer registration, in thread %u (hresult: 0x%08x)\n",
                    _tp->ulThreadNo,
                    hr);

            // Convert empty completion queue as error
            if (hr == S_FALSE)
            {
                hr = E_UNEXPECTED;
            }

            goto cleanup;
        }

        // Check the registration operation result
        hr = cqe.ResultCode;
        if (!SUCCEEDED(hr))
        {
            fprintf(stderr,
                    "FATAL ERROR: IoRing buffer registration failed, in thread %u (hresult: 0x%08x)\n",
                    _tp->ulThreadNo,
                    hr);

            goto cleanup;
        }

        _bufferCount = count;
        _pBufferInfo = bufferInfo;
        bufferInfo = NULL;
    }

cleanup:
    if (bufferInfo != NULL)
    {
        delete[] bufferInfo;
    }

    return hr;
}

//
// Get buffer reference for read operation
//
IORING_BUFFER_REF IoRing::GetReadBufferRef(UINT32 iTarget, UINT32 iRequest)
{
    if (_useRegBuffer)
    {
        return IoRingBufferRefFromIndexAndOffset(iTarget,
                                                 iRequest * _tp->vTargets[iTarget].GetBlockSizeInBytes());
    }

    return IoRingBufferRefFromPointer(_tp->GetReadBuffer(iTarget, iRequest));
}

//
// Get buffer reference for write operation
//
IORING_BUFFER_REF IoRing::GetWriteBufferRef(UINT32 iTarget, UINT32 iRequest)
{
    if (_useRegBuffer)
    {
        return IoRingBufferRefFromIndexAndOffset(iTarget,
                                                 (UINT32)_tp->vulReadBufferSize[iTarget]
                                                    + (iRequest * _tp->vTargets[iTarget].GetBlockSizeInBytes()));
    }

    return IoRingBufferRefFromPointer(_tp->GetWriteBuffer(iTarget, iRequest));
}

void EtwResultParser::ParseResults(vector<Results> vResults)
{
    if (TraceLoggingProviderEnabled(g_hEtwProvider,
                                    TRACE_LEVEL_NONE,
                                    DISKSPD_TRACE_INFO))
    {
        for (size_t ullResults = 0; ullResults < vResults.size(); ullResults++)
        {
            const Results& results = vResults[ullResults];
            for (size_t ullThread = 0; ullThread < results.vThreadResults.size(); ullThread++)
            {
                const ThreadResults& threadResults = results.vThreadResults[ullThread];
                for (const auto& targetResults : threadResults.vTargetResults)
                {
                    if (targetResults.ullReadIOCount)
                    {
                        _WriteResults(IOOperation::ReadIO, targetResults, ullThread);
                    }
                    if (targetResults.ullWriteIOCount)
                    {
                        _WriteResults(IOOperation::WriteIO, targetResults, ullThread);
                    }
                }
            }
        }
    }
}

void EtwResultParser::_WriteResults(IOOperation type, const TargetResults& targetResults, size_t ullThread)
{
    UINT64 ullIOCount = (type == IOOperation::ReadIO) ? targetResults.ullReadIOCount : targetResults.ullWriteIOCount;
    UINT64 ullBytesCount = (type == IOOperation::ReadIO) ? targetResults.ullReadBytesCount : targetResults.ullWriteBytesCount;

    TraceLoggingWrite(g_hEtwProvider,
                      "Statistics",
                      TraceLoggingLevel((TRACE_LEVEL_NONE)),
                      TraceLoggingString((type == IOOperation::ReadIO) ? "Read" : "Write", "IO Type"),
                      TraceLoggingUInt64(ullThread, "Thread"),
                      TraceLoggingUInt64(ullBytesCount, "Bytes"),
                      TraceLoggingUInt64(ullIOCount, "IO Count"),
                      TraceLoggingString(targetResults.sPath.c_str(), "Path"),
                      TraceLoggingUInt64(targetResults.ullFileSize, "File Size"));
}
