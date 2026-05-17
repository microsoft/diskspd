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

#include "StdAfx.h"
#include "CmdLineParser.h"
#include "CmdLineParser.UnitTests.h"
#include <stdlib.h>

using namespace WEX::TestExecution;
using namespace WEX::Logging;

namespace UnitTests
{
    bool ModuleSetup()
    {
        return true;
    }

    bool ModuleCleanup()
    {
        return true;
    }

    bool CmdLineParserUnitTests::ClassSetup()
    {
        return true;
    }

    bool CmdLineParserUnitTests::ClassCleanup()
    {
        return true;
    }

    bool CmdLineParserUnitTests::MethodSetup()
    {
        return true;
    }

    bool CmdLineParserUnitTests::MethodCleanup()
    {
        return true;
    }

    void CmdLineParserUnitTests::Test_GetSizeInBytes()
    {
        CmdLineParser p;
        UINT64 ullResult = 0;
        VERIFY_IS_TRUE(p._GetSizeInBytes("0", ullResult, nullptr));
        VERIFY_IS_TRUE(ullResult == 0);

        VERIFY_IS_TRUE(p._GetSizeInBytes("1", ullResult, nullptr));
        VERIFY_IS_TRUE(ullResult == 1);

        VERIFY_IS_TRUE(p._GetSizeInBytes("2", ullResult, nullptr));
        VERIFY_IS_TRUE(ullResult == 2);

        VERIFY_IS_TRUE(p._GetSizeInBytes("10", ullResult, nullptr));
        VERIFY_IS_TRUE(ullResult == 10);

        VERIFY_IS_TRUE(p._GetSizeInBytes("4096", ullResult, nullptr));
        VERIFY_IS_TRUE(ullResult == 4096);

        VERIFY_IS_TRUE(p._GetSizeInBytes("123908798324", ullResult, nullptr));
        VERIFY_IS_TRUE(ullResult == 123908798324);

        // _GetSizeInBytes shouldn't modify ullResult on if the input string is incorrect
        ullResult = 9;
        VERIFY_IS_TRUE(p._GetSizeInBytes("10a", ullResult, nullptr) == false);
        VERIFY_IS_TRUE(ullResult == 9);

        // block
        VERIFY_IS_TRUE(p._GetSizeInBytes("1b", ullResult, nullptr));
        VERIFY_IS_TRUE(ullResult == p._dwBlockSize);

        VERIFY_IS_TRUE(p._GetSizeInBytes("3B", ullResult, nullptr));
        VERIFY_IS_TRUE(ullResult == 3 * p._dwBlockSize);

        VERIFY_IS_TRUE(p._GetSizeInBytes("30b", ullResult, nullptr));
        VERIFY_IS_TRUE(ullResult == 30 * p._dwBlockSize);

        VERIFY_IS_TRUE(p._GetSizeInBytes("30 b", ullResult, nullptr) == false);
        VERIFY_IS_TRUE(p._GetSizeInBytes("30 B", ullResult, nullptr) == false);

        // KB
        VERIFY_IS_TRUE(p._GetSizeInBytes("1K", ullResult, nullptr));
        VERIFY_IS_TRUE(ullResult == 1024);

        VERIFY_IS_TRUE(p._GetSizeInBytes("3K", ullResult, nullptr));
        VERIFY_IS_TRUE(ullResult == 3 * 1024);

        VERIFY_IS_TRUE(p._GetSizeInBytes("30K", ullResult, nullptr));
        VERIFY_IS_TRUE(ullResult == 30 * 1024);

        VERIFY_IS_TRUE(p._GetSizeInBytes("30 K", ullResult, nullptr) == false);
        VERIFY_IS_TRUE(p._GetSizeInBytes("30KB", ullResult, nullptr) == false);

        // MB
        VERIFY_IS_TRUE(p._GetSizeInBytes("1M", ullResult, nullptr));
        VERIFY_IS_TRUE(ullResult == 1024 * 1024);

        VERIFY_IS_TRUE(p._GetSizeInBytes("4M", ullResult, nullptr));
        VERIFY_IS_TRUE(ullResult == 4 * 1024 * 1024);

        VERIFY_IS_TRUE(p._GetSizeInBytes("50M", ullResult, nullptr));
        VERIFY_IS_TRUE(ullResult == 50 * 1024 * 1024);

        VERIFY_IS_TRUE(p._GetSizeInBytes("40 M", ullResult, nullptr) == false);
        VERIFY_IS_TRUE(p._GetSizeInBytes("40MB", ullResult, nullptr) == false);

        // GB
        VERIFY_IS_TRUE(p._GetSizeInBytes("1G", ullResult, nullptr));
        VERIFY_IS_TRUE(ullResult == 1024 * 1024 * 1024);

        VERIFY_IS_TRUE(p._GetSizeInBytes("6G", ullResult, nullptr));
        VERIFY_IS_TRUE(ullResult == (UINT64)6 * 1024 * 1024 * 1024);

        VERIFY_IS_TRUE(p._GetSizeInBytes("70G", ullResult, nullptr));
        VERIFY_IS_TRUE(ullResult == (UINT64)70 * 1024 * 1024 * 1024);

        VERIFY_IS_TRUE(p._GetSizeInBytes("70 G", ullResult, nullptr) == false);
        VERIFY_IS_TRUE(p._GetSizeInBytes("70GB", ullResult, nullptr) == false);

        // TB
        VERIFY_IS_TRUE(p._GetSizeInBytes("1T", ullResult, nullptr));
        VERIFY_IS_TRUE(ullResult == (UINT64)1024 * 1024 * 1024 * 1024);

        VERIFY_IS_TRUE(p._GetSizeInBytes("6T", ullResult, nullptr));
        VERIFY_IS_TRUE(ullResult == (UINT64)6 * 1024 * 1024 * 1024 * 1024);

        VERIFY_IS_TRUE(p._GetSizeInBytes("70T", ullResult, nullptr));
        VERIFY_IS_TRUE(ullResult == (UINT64)70 * 1024 * 1024 * 1024 * 1024);

        VERIFY_IS_TRUE(p._GetSizeInBytes("70 T", ullResult, nullptr) == false);
        VERIFY_IS_TRUE(p._GetSizeInBytes("70TB", ullResult, nullptr) == false);
        // check overflows
        // MAXUINT64 == 18446744073709551615
        ullResult = 0;
        VERIFY_IS_TRUE(p._GetSizeInBytes("18446744073709551615", ullResult, nullptr));
        VERIFY_IS_TRUE(ullResult == MAXUINT64);

        // MAXUINT64 + 1
        VERIFY_IS_TRUE(p._GetSizeInBytes("18446744073709551616", ullResult, nullptr) == false);

        // MAXUINT64 / 1024 = 18014398509481983
        ullResult = 0;
        VERIFY_IS_TRUE(p._GetSizeInBytes("18014398509481983K", ullResult, nullptr));
        VERIFY_IS_TRUE(ullResult == (MAXUINT64 >> 10) << 10);
        VERIFY_IS_TRUE(p._GetSizeInBytes("18014398509481984K", ullResult, nullptr) == false);

        // MAXUINT64 / 1024^2 = 17592186044415
        ullResult = 0;
        VERIFY_IS_TRUE(p._GetSizeInBytes("17592186044415M", ullResult, nullptr));
        VERIFY_IS_TRUE(ullResult == (MAXUINT64 >> 20) << 20);
        VERIFY_IS_TRUE(p._GetSizeInBytes("17592186044416M", ullResult, nullptr) == false);

        // MAXUINT64 / 1024^3 = 17179869183
        ullResult = 0;
        VERIFY_IS_TRUE(p._GetSizeInBytes("17179869183G", ullResult, nullptr));
        VERIFY_IS_TRUE(ullResult == (MAXUINT64 >> 30) << 30);
        VERIFY_IS_TRUE(p._GetSizeInBytes("17179869184G", ullResult, nullptr) == false);

        // block
        p._dwBlockSize = 1024;
        ullResult = 0;
        VERIFY_IS_TRUE(p._GetSizeInBytes("18014398509481983b", ullResult, nullptr));
        VERIFY_IS_TRUE(ullResult == (MAXUINT64 >> 10) << 10);
        VERIFY_IS_TRUE(p._GetSizeInBytes("18014398509481984b", ullResult, nullptr) == false);
    }

    void CmdLineParserUnitTests::TestParseCmdLineAssignAffinity()
    {
        CmdLineParser p;
        struct Synchronization s = {};

        // base case
        {
            Profile profile;
            const char *argv[] = { "foo", "-a", "testfile.dat" };
            VERIFY_IS_TRUE(p.ParseCmdLine(_countof(argv), argv, &profile, &s) == true);

            vector<TimeSpan> vSpans(profile.GetTimeSpans());
            VERIFY_ARE_EQUAL(vSpans.size(), (size_t)1);
            VERIFY_IS_TRUE(vSpans[0].GetDisableAffinity() == false);

            const auto& vMasks(vSpans[0].GetAffinityGroupMasks());
            VERIFY_IS_TRUE(vMasks.empty());
        }

        // base case
        {
            Profile profile;
            const char *argv[] = { "foo", "-ag", "testfile.dat" };
            VERIFY_IS_TRUE(p.ParseCmdLine(_countof(argv), argv, &profile, &s) == true);

            vector<TimeSpan> vSpans(profile.GetTimeSpans());
            VERIFY_ARE_EQUAL(vSpans.size(), (size_t)1);
            VERIFY_IS_TRUE(vSpans[0].GetDisableAffinity() == false);

            const auto& vMasks(vSpans[0].GetAffinityGroupMasks());
            VERIFY_IS_TRUE(vMasks.empty());
        }

        // no group spec case
        {
            Profile profile;
            const char *argv[] = { "foo", "-a1,4,6", "testfile.dat" };
            VERIFY_IS_TRUE(p.ParseCmdLine(_countof(argv), argv, &profile, &s) == true);

            vector<TimeSpan> vSpans(profile.GetTimeSpans());
            VERIFY_ARE_EQUAL(vSpans.size(), (size_t)1);
            VERIFY_IS_TRUE(vSpans[0].GetDisableAffinity() == false);

            // CPUs 1,4,6 ascending in group 0 -> merged into one mask
            const auto& vMasks(vSpans[0].GetAffinityGroupMasks());
            VERIFY_ARE_EQUAL(vMasks.size(), (size_t)1);
            VERIFY_ARE_EQUAL(vMasks[0].wGroup, (WORD)0);
            VERIFY_ARE_EQUAL(vMasks[0].mask, (KAFFINITY)0x52);
        }

        // single group spec
        {
            Profile profile;
            const char *argv[] = { "foo", "-ag0,1,4,6", "testfile.dat" };
            VERIFY_IS_TRUE(p.ParseCmdLine(_countof(argv), argv, &profile, &s) == true);

            vector<TimeSpan> vSpans(profile.GetTimeSpans());
            VERIFY_ARE_EQUAL(vSpans.size(), (size_t)1);
            VERIFY_IS_TRUE(vSpans[0].GetDisableAffinity() == false);

            const auto& vMasks(vSpans[0].GetAffinityGroupMasks());
            VERIFY_ARE_EQUAL(vMasks.size(), (size_t)1);
            VERIFY_ARE_EQUAL(vMasks[0].wGroup, (WORD)0);
            VERIFY_ARE_EQUAL(vMasks[0].mask, (KAFFINITY)0x52);
        }

        // multiple group spec
        {
            Profile profile;
            const char *argv[] = { "foo", "-ag0,1,4,6,g1,2,5,7", "testfile.dat" };
            VERIFY_IS_TRUE(p.ParseCmdLine(_countof(argv), argv, &profile, &s) == true);

            vector<TimeSpan> vSpans(profile.GetTimeSpans());
            VERIFY_ARE_EQUAL(vSpans.size(), (size_t)1);
            VERIFY_IS_TRUE(vSpans[0].GetDisableAffinity() == false);

            // group 0 CPUs 1,4,6 -> 0x52; group 1 CPUs 2,5,7 -> 0xA4
            const auto& vMasks(vSpans[0].GetAffinityGroupMasks());
            VERIFY_ARE_EQUAL(vMasks.size(), (size_t)2);
            VERIFY_ARE_EQUAL(vMasks[0].wGroup, (WORD)0);
            VERIFY_ARE_EQUAL(vMasks[0].mask, (KAFFINITY)0x52);
            VERIFY_ARE_EQUAL(vMasks[1].wGroup, (WORD)1);
            VERIFY_ARE_EQUAL(vMasks[1].mask, (KAFFINITY)0xA4);
        }

        // multiple group spec across two instances of -ag
        {
            Profile profile;
            const char *argv[] = { "foo", "-ag0,1,4,6", "-ag1,2,5,7", "testfile.dat" };
            VERIFY_IS_TRUE(p.ParseCmdLine(_countof(argv), argv, &profile, &s) == true);

            vector<TimeSpan> vSpans(profile.GetTimeSpans());
            VERIFY_ARE_EQUAL(vSpans.size(), (size_t)1);
            VERIFY_IS_TRUE(vSpans[0].GetDisableAffinity() == false);

            const auto& vMasks(vSpans[0].GetAffinityGroupMasks());
            VERIFY_ARE_EQUAL(vMasks.size(), (size_t)2);
            VERIFY_ARE_EQUAL(vMasks[0].wGroup, (WORD)0);
            VERIFY_ARE_EQUAL(vMasks[0].mask, (KAFFINITY)0x52);
            VERIFY_ARE_EQUAL(vMasks[1].wGroup, (WORD)1);
            VERIFY_ARE_EQUAL(vMasks[1].mask, (KAFFINITY)0xA4);
        }

        // now various syntax error cases
        // just group spec - NOW LEGAL: whole-group effective
        {
            Profile profile;
            const char *argv[] = { "foo", "-ag0", "testfile.dat" };
            VERIFY_IS_TRUE(p.ParseCmdLine(_countof(argv), argv, &profile, &s) == true);

            vector<TimeSpan> vSpans(profile.GetTimeSpans());
            const auto& vMasks(vSpans[0].GetAffinityGroupMasks());
            VERIFY_ARE_EQUAL(vMasks.size(), (size_t)1);
            VERIFY_ARE_EQUAL(vMasks[0].wGroup, (WORD)0);
            VERIFY_ARE_EQUAL(vMasks[0].mask, (KAFFINITY)0);
        }

        // multiple g
        {
            Profile profile;
            const char *argv[] = { "foo", "-agg", "testfile.dat" };
            VERIFY_IS_FALSE(p.ParseCmdLine(_countof(argv), argv, &profile, &s));
        }

        // group with no number
        {
            Profile profile;
            const char *argv[] = { "foo", "-ag,0,1,2", "testfile.dat" };
            VERIFY_IS_FALSE(p.ParseCmdLine(_countof(argv), argv, &profile, &s));
        }

        // trailing ,
        {
            Profile profile;
            const char *argv[] = { "foo", "-a1,", "testfile.dat" };
            VERIFY_IS_FALSE(p.ParseCmdLine(_countof(argv), argv, &profile, &s));
        }

        // trailing , in group spec form
        {
            Profile profile;
            const char *argv[] = { "foo", "-ag1,0,", "testfile.dat" };
            VERIFY_IS_FALSE(p.ParseCmdLine(_countof(argv), argv, &profile, &s));
        }

        // trailing g in group spec form
        {
            Profile profile;
            const char *argv[] = { "foo", "-ag1,0,g", "testfile.dat" };
            VERIFY_IS_FALSE(p.ParseCmdLine(_countof(argv), argv, &profile, &s));
        }

        // mixed CPU and whole-group in one switch, not allowed
        {
            Profile profile;
            const char *argv[] = { "foo", "-ag1,0,g2", "testfile.dat" };
            VERIFY_IS_FALSE(p.ParseCmdLine(_countof(argv), argv, &profile, &s));
        }

        // junk chars
        {
            Profile profile;
            const char *argv[] = { "foo", "-ax", "testfile.dat" };
            VERIFY_IS_FALSE(p.ParseCmdLine(_countof(argv), argv, &profile, &s));
        }

        // out-of-range processor index (BYTE)
        {
            Profile profile;
            const char *argv[] = { "foo", "-ag0,300", "testfile.dat" };
            VERIFY_IS_FALSE(p.ParseCmdLine(_countof(argv), argv, &profile, &s));
        }

        // out-of-range group index (WORD)
        {
            Profile profile;
            const char *argv[] = { "foo", "-ag70000,1", "testfile.dat" };
            VERIFY_IS_FALSE(p.ParseCmdLine(_countof(argv), argv, &profile, &s));
        }

        // range spec: -ag0,0-2
        {
            Profile profile;
            const char *argv[] = { "foo", "-ag0,0-2", "testfile.dat" };
            VERIFY_IS_TRUE(p.ParseCmdLine(_countof(argv), argv, &profile, &s) == true);

            vector<TimeSpan> vSpans(profile.GetTimeSpans());
            const auto& vMasks(vSpans[0].GetAffinityGroupMasks());
            VERIFY_ARE_EQUAL(vMasks.size(), (size_t)1);
            VERIFY_ARE_EQUAL(vMasks[0].wGroup, (WORD)0);
            VERIFY_ARE_EQUAL(vMasks[0].mask, (KAFFINITY)0x7);
        }

        // range spec with gap: -ag0,0-2,5-7
        {
            Profile profile;
            const char *argv[] = { "foo", "-ag0,0-2,5-7", "testfile.dat" };
            VERIFY_IS_TRUE(p.ParseCmdLine(_countof(argv), argv, &profile, &s) == true);

            vector<TimeSpan> vSpans(profile.GetTimeSpans());
            const auto& vMasks(vSpans[0].GetAffinityGroupMasks());
            VERIFY_ARE_EQUAL(vMasks.size(), (size_t)1);
            VERIFY_ARE_EQUAL(vMasks[0].wGroup, (WORD)0);
            VERIFY_ARE_EQUAL(vMasks[0].mask, (KAFFINITY)0xE7);
        }

        // mixed range and individual: -ag0,0,3-5,7
        {
            Profile profile;
            const char *argv[] = { "foo", "-ag0,0,3-5,7", "testfile.dat" };
            VERIFY_IS_TRUE(p.ParseCmdLine(_countof(argv), argv, &profile, &s) == true);

            vector<TimeSpan> vSpans(profile.GetTimeSpans());
            const auto& vMasks(vSpans[0].GetAffinityGroupMasks());
            VERIFY_ARE_EQUAL(vMasks.size(), (size_t)1);
            VERIFY_ARE_EQUAL(vMasks[0].wGroup, (WORD)0);
            VERIFY_ARE_EQUAL(vMasks[0].mask, (KAFFINITY)0xB9);
        }

        // range across groups: -ag0,0-2,g1,5-7
        {
            Profile profile;
            const char *argv[] = { "foo", "-ag0,0-2,g1,5-7", "testfile.dat" };
            VERIFY_IS_TRUE(p.ParseCmdLine(_countof(argv), argv, &profile, &s) == true);

            vector<TimeSpan> vSpans(profile.GetTimeSpans());
            const auto& vMasks(vSpans[0].GetAffinityGroupMasks());
            VERIFY_ARE_EQUAL(vMasks.size(), (size_t)2);
            VERIFY_ARE_EQUAL(vMasks[0].wGroup, (WORD)0);
            VERIFY_ARE_EQUAL(vMasks[0].mask, (KAFFINITY)0x7);
            VERIFY_ARE_EQUAL(vMasks[1].wGroup, (WORD)1);
            VERIFY_ARE_EQUAL(vMasks[1].mask, (KAFFINITY)0xE0);
        }

        // equivalence: -ag0,0-10,15-20 and -ag0,0-10,g0,15-20 produce same result
        {
            // bits 0-10 = 0x7FF, bits 15-20 = 0x1F8000, total = 0x1F87FF
            Profile p1, p2;
            const char *argv1[] = { "foo", "-ag0,0-10,15-20", "testfile.dat" };
            const char *argv2[] = { "foo", "-ag0,0-10,g0,15-20", "testfile.dat" };
            VERIFY_IS_TRUE(p.ParseCmdLine(_countof(argv1), argv1, &p1, &s) == true);
            VERIFY_IS_TRUE(p.ParseCmdLine(_countof(argv2), argv2, &p2, &s) == true);

            const auto& m1(p1.GetTimeSpans()[0].GetAffinityGroupMasks());
            const auto& m2(p2.GetTimeSpans()[0].GetAffinityGroupMasks());

            // Both should produce a single merged mask entry for group 0
            VERIFY_ARE_EQUAL(m1.size(), (size_t)1);
            VERIFY_ARE_EQUAL(m1[0].wGroup, (WORD)0);
            VERIFY_ARE_EQUAL(m1[0].mask, (KAFFINITY)0x1F87FF);

            VERIFY_ARE_EQUAL(m2.size(), (size_t)1);
            VERIFY_ARE_EQUAL(m2[0].wGroup, (WORD)0);
            VERIFY_ARE_EQUAL(m2[0].mask, (KAFFINITY)0x1F87FF);
        }

        // multi whole-group in one switch: NOW ILLEGAL (use separate -a switches)
        {
            Profile profile;
            const char *argv[] = { "foo", "-ag1,g0", "testfile.dat" };
            VERIFY_IS_FALSE(p.ParseCmdLine(_countof(argv), argv, &profile, &s));
        }

        // multi whole-group across separate -a switches: legal
        {
            Profile profile;
            const char *argv[] = { "foo", "-ag1", "-ag0", "testfile.dat" };
            VERIFY_IS_TRUE(p.ParseCmdLine(_countof(argv), argv, &profile, &s) == true);

            vector<TimeSpan> vSpans(profile.GetTimeSpans());
            const auto& vMasks(vSpans[0].GetAffinityGroupMasks());
            VERIFY_ARE_EQUAL(vMasks.size(), (size_t)2);
            VERIFY_ARE_EQUAL(vMasks[0].wGroup, (WORD)1);
            VERIFY_ARE_EQUAL(vMasks[0].mask, (KAFFINITY)0);
            VERIFY_ARE_EQUAL(vMasks[1].wGroup, (WORD)0);
            VERIFY_ARE_EQUAL(vMasks[1].mask, (KAFFINITY)0);
        }

        // descending CPUs don't merge: -ag0,5,3
        {
            Profile profile;
            const char *argv[] = { "foo", "-ag0,5,3", "testfile.dat" };
            VERIFY_IS_TRUE(p.ParseCmdLine(_countof(argv), argv, &profile, &s) == true);

            vector<TimeSpan> vSpans(profile.GetTimeSpans());
            const auto& vMasks(vSpans[0].GetAffinityGroupMasks());
            VERIFY_ARE_EQUAL(vMasks.size(), (size_t)2);
            VERIFY_ARE_EQUAL(vMasks[0].wGroup, (WORD)0);
            VERIFY_ARE_EQUAL(vMasks[0].mask, (KAFFINITY)0x20);
            VERIFY_ARE_EQUAL(vMasks[1].wGroup, (WORD)0);
            VERIFY_ARE_EQUAL(vMasks[1].mask, (KAFFINITY)0x8);
        }

        // inverted range error: -ag0,5-3
        {
            Profile profile;
            const char *argv[] = { "foo", "-ag0,5-3", "testfile.dat" };
            VERIFY_IS_FALSE(p.ParseCmdLine(_countof(argv), argv, &profile, &s));
        }

        // trailing dash: -ag0,5-
        {
            Profile profile;
            const char *argv[] = { "foo", "-ag0,5-", "testfile.dat" };
            VERIFY_IS_FALSE(p.ParseCmdLine(_countof(argv), argv, &profile, &s));
        }

        // -acs: CoreAware + Span
        {
            Profile profile;
            const char *argv[] = { "foo", "-acs", "testfile.dat" };
            VERIFY_IS_TRUE(p.ParseCmdLine(_countof(argv), argv, &profile, &s) == true);

            vector<TimeSpan> vSpans(profile.GetTimeSpans());
            VERIFY_ARE_EQUAL(vSpans[0].GetAffinityTraversal(), AffinityTraversal::CoreAware);
            VERIFY_ARE_EQUAL(vSpans[0].GetAffinityGroupSpan(), AffinityGroupSpan::Span);
            VERIFY_IS_TRUE(vSpans[0].GetAffinityGroupMasks().empty());
        }

        // -ac with separate -ag spec
        {
            Profile profile;
            const char *argv[] = { "foo", "-ac", "-ag0,1,2", "testfile.dat" };
            VERIFY_IS_TRUE(p.ParseCmdLine(_countof(argv), argv, &profile, &s) == true);

            vector<TimeSpan> vSpans(profile.GetTimeSpans());
            VERIFY_ARE_EQUAL(vSpans[0].GetAffinityTraversal(), AffinityTraversal::CoreAware);
            VERIFY_ARE_EQUAL(vSpans[0].GetAffinityGroupSpan(), AffinityGroupSpan::Fill);
            const auto& vMasks(vSpans[0].GetAffinityGroupMasks());
            VERIFY_ARE_EQUAL(vMasks.size(), (size_t)1);
            VERIFY_ARE_EQUAL(vMasks[0].wGroup, (WORD)0);
            VERIFY_ARE_EQUAL(vMasks[0].mask, (KAFFINITY)0x6);
        }

        // -as with separate whole-group
        {
            Profile profile;
            const char *argv[] = { "foo", "-as", "-ag0", "testfile.dat" };
            VERIFY_IS_TRUE(p.ParseCmdLine(_countof(argv), argv, &profile, &s) == true);

            vector<TimeSpan> vSpans(profile.GetTimeSpans());
            VERIFY_ARE_EQUAL(vSpans[0].GetAffinityTraversal(), AffinityTraversal::Cpu);
            VERIFY_ARE_EQUAL(vSpans[0].GetAffinityGroupSpan(), AffinityGroupSpan::Span);
            const auto& vMasks(vSpans[0].GetAffinityGroupMasks());
            VERIFY_ARE_EQUAL(vMasks.size(), (size_t)1);
            VERIFY_ARE_EQUAL(vMasks[0].mask, (KAFFINITY)0);
        }

        // -acs with multiple -ag specs
        {
            Profile profile;
            const char *argv[] = { "foo", "-acs", "-ag0", "-ag1,0-3", "testfile.dat" };
            VERIFY_IS_TRUE(p.ParseCmdLine(_countof(argv), argv, &profile, &s) == true);

            vector<TimeSpan> vSpans(profile.GetTimeSpans());
            VERIFY_ARE_EQUAL(vSpans[0].GetAffinityTraversal(), AffinityTraversal::CoreAware);
            VERIFY_ARE_EQUAL(vSpans[0].GetAffinityGroupSpan(), AffinityGroupSpan::Span);
            const auto& vMasks(vSpans[0].GetAffinityGroupMasks());
            VERIFY_ARE_EQUAL(vMasks.size(), (size_t)2);
        }

        // default is Cpu, Fill
        {
            Profile profile;
            const char *argv[] = { "foo", "-ag0,0-3", "testfile.dat" };
            VERIFY_IS_TRUE(p.ParseCmdLine(_countof(argv), argv, &profile, &s) == true);

            vector<TimeSpan> vSpans(profile.GetTimeSpans());
            VERIFY_ARE_EQUAL(vSpans[0].GetAffinityTraversal(), AffinityTraversal::Cpu);
            VERIFY_ARE_EQUAL(vSpans[0].GetAffinityGroupSpan(), AffinityGroupSpan::Fill);
        }

        // -aP: FillPFirst efficiency order
        {
            Profile profile;
            const char *argv[] = { "foo", "-aP", "testfile.dat" };
            VERIFY_IS_TRUE(p.ParseCmdLine(_countof(argv), argv, &profile, &s) == true);

            vector<TimeSpan> vSpans(profile.GetTimeSpans());
            VERIFY_ARE_EQUAL(vSpans[0].GetAffinityEfficiencyOrder(), AffinityEfficiencyOrder::FillPFirst);
            VERIFY_ARE_EQUAL(vSpans[0].GetAffinityTraversal(), AffinityTraversal::Cpu);
        }

        // -ae: EFirst efficiency order
        {
            Profile profile;
            const char *argv[] = { "foo", "-ae", "testfile.dat" };
            VERIFY_IS_TRUE(p.ParseCmdLine(_countof(argv), argv, &profile, &s) == true);

            vector<TimeSpan> vSpans(profile.GetTimeSpans());
            VERIFY_ARE_EQUAL(vSpans[0].GetAffinityEfficiencyOrder(), AffinityEfficiencyOrder::EFirst);
            VERIFY_ARE_EQUAL(vSpans[0].GetAffinityTraversal(), AffinityTraversal::Cpu);
        }

        // -aE: FillEFirst efficiency order
        {
            Profile profile;
            const char *argv[] = { "foo", "-aE", "testfile.dat" };
            VERIFY_IS_TRUE(p.ParseCmdLine(_countof(argv), argv, &profile, &s) == true);

            vector<TimeSpan> vSpans(profile.GetTimeSpans());
            VERIFY_ARE_EQUAL(vSpans[0].GetAffinityEfficiencyOrder(), AffinityEfficiencyOrder::FillEFirst);
            VERIFY_ARE_EQUAL(vSpans[0].GetAffinityTraversal(), AffinityTraversal::Cpu);
        }

        // -ap: explicit PFirst
        {
            Profile profile;
            const char *argv[] = { "foo", "-ap", "testfile.dat" };
            VERIFY_IS_TRUE(p.ParseCmdLine(_countof(argv), argv, &profile, &s) == true);

            vector<TimeSpan> vSpans(profile.GetTimeSpans());
            VERIFY_ARE_EQUAL(vSpans[0].GetAffinityEfficiencyOrder(), AffinityEfficiencyOrder::PFirst);
            VERIFY_ARE_EQUAL(vSpans[0].GetAffinityTraversal(), AffinityTraversal::Cpu);
        }

        // -acsE: composed CoreAware + Span + FillEFirst
        {
            Profile profile;
            const char *argv[] = { "foo", "-acsE", "testfile.dat" };
            VERIFY_IS_TRUE(p.ParseCmdLine(_countof(argv), argv, &profile, &s) == true);

            vector<TimeSpan> vSpans(profile.GetTimeSpans());
            VERIFY_ARE_EQUAL(vSpans[0].GetAffinityTraversal(), AffinityTraversal::CoreAware);
            VERIFY_ARE_EQUAL(vSpans[0].GetAffinityGroupSpan(), AffinityGroupSpan::Span);
            VERIFY_ARE_EQUAL(vSpans[0].GetAffinityEfficiencyOrder(), AffinityEfficiencyOrder::FillEFirst);
        }

        // -acs -aE: separate CoreAware+Span + FillEFirst
        {
            Profile profile;
            const char *argv[] = { "foo", "-acs", "-aE", "testfile.dat" };
            VERIFY_IS_TRUE(p.ParseCmdLine(_countof(argv), argv, &profile, &s) == true);

            vector<TimeSpan> vSpans(profile.GetTimeSpans());
            VERIFY_ARE_EQUAL(vSpans[0].GetAffinityTraversal(), AffinityTraversal::CoreAware);
            VERIFY_ARE_EQUAL(vSpans[0].GetAffinityGroupSpan(), AffinityGroupSpan::Span);
            VERIFY_ARE_EQUAL(vSpans[0].GetAffinityEfficiencyOrder(), AffinityEfficiencyOrder::FillEFirst);
        }

        // -aPcs: composed FillPFirst + CoreAware + Span (any order)
        {
            Profile profile;
            const char *argv[] = { "foo", "-aPcs", "testfile.dat" };
            VERIFY_IS_TRUE(p.ParseCmdLine(_countof(argv), argv, &profile, &s) == true);

            vector<TimeSpan> vSpans(profile.GetTimeSpans());
            VERIFY_ARE_EQUAL(vSpans[0].GetAffinityTraversal(), AffinityTraversal::CoreAware);
            VERIFY_ARE_EQUAL(vSpans[0].GetAffinityGroupSpan(), AffinityGroupSpan::Span);
            VERIFY_ARE_EQUAL(vSpans[0].GetAffinityEfficiencyOrder(), AffinityEfficiencyOrder::FillPFirst);
        }

        // -aP -ac: separate FillPFirst + CoreAware
        {
            Profile profile;
            const char *argv[] = { "foo", "-aP", "-ac", "testfile.dat" };
            VERIFY_IS_TRUE(p.ParseCmdLine(_countof(argv), argv, &profile, &s) == true);

            vector<TimeSpan> vSpans(profile.GetTimeSpans());
            VERIFY_ARE_EQUAL(vSpans[0].GetAffinityTraversal(), AffinityTraversal::CoreAware);
            VERIFY_ARE_EQUAL(vSpans[0].GetAffinityGroupSpan(), AffinityGroupSpan::Fill);
            VERIFY_ARE_EQUAL(vSpans[0].GetAffinityEfficiencyOrder(), AffinityEfficiencyOrder::FillPFirst);
        }

        // -aPE: multiple efficiency orders conflict
        {
            Profile profile;
            const char *argv[] = { "foo", "-aPE", "testfile.dat" };
            VERIFY_IS_FALSE(p.ParseCmdLine(_countof(argv), argv, &profile, &s));
        }

        // -asP: Span + FillPFirst
        {
            Profile profile;
            const char *argv[] = { "foo", "-asP", "testfile.dat" };
            VERIFY_IS_TRUE(p.ParseCmdLine(_countof(argv), argv, &profile, &s) == true);

            vector<TimeSpan> vSpans(profile.GetTimeSpans());
            VERIFY_ARE_EQUAL(vSpans[0].GetAffinityTraversal(), AffinityTraversal::Cpu);
            VERIFY_ARE_EQUAL(vSpans[0].GetAffinityGroupSpan(), AffinityGroupSpan::Span);
            VERIFY_ARE_EQUAL(vSpans[0].GetAffinityEfficiencyOrder(), AffinityEfficiencyOrder::FillPFirst);
        }

        // -ase: Span + EFirst
        {
            Profile profile;
            const char *argv[] = { "foo", "-ase", "testfile.dat" };
            VERIFY_IS_TRUE(p.ParseCmdLine(_countof(argv), argv, &profile, &s) == true);

            vector<TimeSpan> vSpans(profile.GetTimeSpans());
            VERIFY_ARE_EQUAL(vSpans[0].GetAffinityTraversal(), AffinityTraversal::Cpu);
            VERIFY_ARE_EQUAL(vSpans[0].GetAffinityGroupSpan(), AffinityGroupSpan::Span);
            VERIFY_ARE_EQUAL(vSpans[0].GetAffinityEfficiencyOrder(), AffinityEfficiencyOrder::EFirst);
        }

        // -apsx: trailing invalid character
        {
            Profile profile;
            const char *argv[] = { "foo", "-apsx", "testfile.dat" };
            VERIFY_IS_FALSE(p.ParseCmdLine(_countof(argv), argv, &profile, &s));
        }

        // -acc: idempotent (c is on/off, repeating is benign)
        {
            Profile profile;
            const char *argv[] = { "foo", "-acc", "testfile.dat" };
            VERIFY_IS_TRUE(p.ParseCmdLine(_countof(argv), argv, &profile, &s) == true);

            vector<TimeSpan> vSpans(profile.GetTimeSpans());
            VERIFY_ARE_EQUAL(vSpans[0].GetAffinityTraversal(), AffinityTraversal::CoreAware);
        }

        // -aup: unset efficiency ordering
        {
            Profile profile;
            const char *argv[] = { "foo", "-aup", "testfile.dat" };
            VERIFY_IS_TRUE(p.ParseCmdLine(_countof(argv), argv, &profile, &s) == true);

            vector<TimeSpan> vSpans(profile.GetTimeSpans());
            VERIFY_ARE_EQUAL(vSpans[0].GetAffinityEfficiencyOrder(), AffinityEfficiencyOrder::Unordered);
        }

        // -aup -ac: unordered composes with core-awareness (separate switches)
        {
            Profile profile;
            const char *argv[] = { "foo", "-aup", "-ac", "testfile.dat" };
            VERIFY_IS_TRUE(p.ParseCmdLine(_countof(argv), argv, &profile, &s) == true);

            vector<TimeSpan> vSpans(profile.GetTimeSpans());
            VERIFY_ARE_EQUAL(vSpans[0].GetAffinityEfficiencyOrder(), AffinityEfficiencyOrder::Unordered);
            VERIFY_ARE_EQUAL(vSpans[0].GetAffinityTraversal(), AffinityTraversal::CoreAware);
        }

        // -aup -aE: conflict (unset + explicit)
        {
            Profile profile;
            const char *argv[] = { "foo", "-aup", "-aE", "testfile.dat" };
            VERIFY_IS_FALSE(p.ParseCmdLine(_countof(argv), argv, &profile, &s));
        }

        // -aE -aup: conflict (explicit + unset, reversed)
        {
            Profile profile;
            const char *argv[] = { "foo", "-aE", "-aup", "testfile.dat" };
            VERIFY_IS_FALSE(p.ParseCmdLine(_countof(argv), argv, &profile, &s));
        }

        // -au alone: error (requires sub-option)
        {
            Profile profile;
            const char *argv[] = { "foo", "-au", "testfile.dat" };
            VERIFY_IS_FALSE(p.ParseCmdLine(_countof(argv), argv, &profile, &s));
        }

        // -aux: unknown -au option
        {
            Profile profile;
            const char *argv[] = { "foo", "-aux", "testfile.dat" };
            VERIFY_IS_FALSE(p.ParseCmdLine(_countof(argv), argv, &profile, &s));
        }
    }

    void CmdLineParserUnitTests::TestParseCmdLine()
    {
        CmdLineParser p;
        Profile profile;
        struct Synchronization s = {};
        const char *argv[] = { "foo", "-b4K", "-w84", "-a1,4,6", "testfile.dat", "testfile2.dat" };
        VERIFY_IS_TRUE(p.ParseCmdLine(_countof(argv), argv, &profile, &s) == true);
        VERIFY_IS_TRUE(profile.GetVerbose() == false);
        VERIFY_IS_TRUE(profile.GetProgress() == 0);
        VERIFY_IS_TRUE(profile.GetCmdLine().compare("foo -b4K -w84 -a1,4,6 testfile.dat testfile2.dat") == 0);
        VERIFY_IS_TRUE(profile.GetEtwEnabled() == false);
        VERIFY_IS_TRUE(profile.GetEtwProcess() == false);
        VERIFY_IS_TRUE(profile.GetEtwThread() == false);
        VERIFY_IS_TRUE(profile.GetEtwImageLoad() == false);
        VERIFY_IS_TRUE(profile.GetEtwDiskIO() == false);
        VERIFY_IS_TRUE(profile.GetEtwMemoryPageFaults() == false);
        VERIFY_IS_TRUE(profile.GetEtwMemoryHardFaults() == false);
        VERIFY_IS_TRUE(profile.GetEtwNetwork() == false);
        VERIFY_IS_TRUE(profile.GetEtwRegistry() == false);
        VERIFY_IS_TRUE(profile.GetEtwUsePagedMemory() == false);
        VERIFY_IS_TRUE(profile.GetEtwUsePerfTimer() == false);
        VERIFY_IS_TRUE(profile.GetEtwUseSystemTimer() == false);
        VERIFY_IS_TRUE(profile.GetEtwUseCyclesCounter() == false);

        vector<TimeSpan> vSpans(profile.GetTimeSpans());
        VERIFY_ARE_EQUAL(vSpans.size(), (size_t)1);
        VERIFY_ARE_EQUAL(vSpans[0].GetDuration(), (UINT32)10);
        VERIFY_ARE_EQUAL(vSpans[0].GetWarmup(), (UINT32)5);
        VERIFY_ARE_EQUAL(vSpans[0].GetCooldown(), (UINT32)0);
        VERIFY_ARE_EQUAL(vSpans[0].GetRandSeed(), (UINT32)0);
        VERIFY_ARE_EQUAL(vSpans[0].GetThreadCount(), (DWORD)0);
        VERIFY_ARE_EQUAL(vSpans[0].GetRequestCount(), (DWORD)0);
        VERIFY_IS_TRUE(vSpans[0].GetDisableAffinity() == false);
        VERIFY_IS_TRUE(vSpans[0].GetCompletionRoutines() == false);
        VERIFY_IS_TRUE(vSpans[0].GetMeasureLatency() == false);
        VERIFY_IS_TRUE(vSpans[0].GetRandomWriteData() == false);
        VERIFY_IS_TRUE(vSpans[0].GetUseIoRing() == false);

        const auto& vMasks(vSpans[0].GetAffinityGroupMasks());
        VERIFY_ARE_EQUAL(vMasks.size(), (size_t)1);
        VERIFY_ARE_EQUAL(vMasks[0].wGroup, (WORD)0);
        VERIFY_ARE_EQUAL(vMasks[0].mask, (KAFFINITY)0x52);

        vector<Target> vTargets(vSpans[0].GetTargets());
        VERIFY_ARE_EQUAL(vTargets.size(), (size_t)2);

        {
            const auto& t = vTargets[0];

            VERIFY_IS_TRUE(t.GetPath().compare("testfile.dat") == 0);
            VERIFY_ARE_EQUAL(t.GetBlockSizeInBytes(), (DWORD)(4 * 1024));
            VERIFY_ARE_EQUAL(t.GetRequestCount(), (DWORD)2);
            VERIFY_IS_TRUE(t.GetRandomRatio() == 0);
            VERIFY_IS_TRUE(t.GetUseInterlockedSequential() == false);
            VERIFY_ARE_EQUAL(t.GetBlockAlignmentInBytes(), t.GetBlockSizeInBytes());
            VERIFY_ARE_EQUAL(t.GetBaseFileOffsetInBytes(), 0);
            VERIFY_IS_TRUE(t.GetUseParallelAsyncIO() == false);
            VERIFY_IS_TRUE(t.GetCacheMode() == TargetCacheMode::Cached);
            VERIFY_IS_TRUE(t.GetWriteThroughMode() == WriteThroughMode::Off);
            VERIFY_IS_TRUE(t.GetMemoryMappedIoMode() == MemoryMappedIoMode::Off);
            VERIFY_IS_TRUE(t.GetMemoryMappedIoFlushMode() == MemoryMappedIoFlushMode::Undefined);
            VERIFY_IS_TRUE(t.GetBypassIoMode() == BypassIoMode::Undefined);
            VERIFY_ARE_EQUAL(t.GetThreadsPerFile(), (DWORD)1);
            VERIFY_ARE_EQUAL(t.GetThreadStrideInBytes(), 0);
            VERIFY_IS_TRUE(t.GetCreateFile() == false);
            VERIFY_ARE_EQUAL(t.GetFileSize(), 0);
            VERIFY_ARE_EQUAL(t.GetMaxFileSize(), 0);
            VERIFY_ARE_EQUAL(t.GetWriteRatio(), (DWORD)84);
            VERIFY_IS_TRUE(t.GetUseBurstSize() == false);
            VERIFY_ARE_EQUAL(t.GetBurstSize(), (DWORD)0);
            VERIFY_ARE_EQUAL(t.GetThinkTime(), (DWORD)0);
            VERIFY_IS_TRUE(t.GetEnableThinkTime() == false);
            VERIFY_IS_TRUE(t.GetSequentialScanHint() == false);
            VERIFY_IS_TRUE(t.GetRandomAccessHint() == false);
            VERIFY_IS_TRUE(t.GetTemporaryFileHint() == false);
            VERIFY_IS_TRUE(t.GetUseLargePages() == false);
            VERIFY_ARE_EQUAL(t.GetThroughputInBytesPerMillisecond(), (DWORD)0);
        }

        {
            const auto& t = vTargets[1];

            VERIFY_IS_TRUE(t.GetPath().compare("testfile2.dat") == 0);
            VERIFY_ARE_EQUAL(t.GetBlockSizeInBytes(), (DWORD)(4 * 1024));
            VERIFY_ARE_EQUAL(t.GetRequestCount(), (DWORD)2);
            VERIFY_IS_TRUE(t.GetRandomRatio() == 0);
            VERIFY_IS_TRUE(t.GetUseInterlockedSequential() == false);
            VERIFY_ARE_EQUAL(t.GetBlockAlignmentInBytes(), t.GetBlockSizeInBytes());
            VERIFY_ARE_EQUAL(t.GetBaseFileOffsetInBytes(), 0);
            VERIFY_IS_TRUE(t.GetUseParallelAsyncIO() == false);
            VERIFY_IS_TRUE(t.GetCacheMode() == TargetCacheMode::Cached);
            VERIFY_IS_TRUE(t.GetWriteThroughMode() == WriteThroughMode::Off);
            VERIFY_IS_TRUE(t.GetMemoryMappedIoMode() == MemoryMappedIoMode::Off);
            VERIFY_IS_TRUE(t.GetMemoryMappedIoFlushMode() == MemoryMappedIoFlushMode::Undefined);
            VERIFY_IS_TRUE(t.GetBypassIoMode() == BypassIoMode::Undefined);
            VERIFY_IS_TRUE(t.GetZeroWriteBuffers() == false);
            VERIFY_ARE_EQUAL(t.GetThreadsPerFile(), (DWORD)1);
            VERIFY_ARE_EQUAL(t.GetThreadStrideInBytes(), 0);
            VERIFY_IS_TRUE(t.GetCreateFile() == false);
            VERIFY_ARE_EQUAL(t.GetFileSize(), 0);
            VERIFY_ARE_EQUAL(t.GetMaxFileSize(), 0);
            VERIFY_ARE_EQUAL(t.GetWriteRatio(), (DWORD)84);
            VERIFY_IS_TRUE(t.GetUseBurstSize() == false);
            VERIFY_ARE_EQUAL(t.GetBurstSize(), (DWORD)0);
            VERIFY_ARE_EQUAL(t.GetThinkTime(), (DWORD)0);
            VERIFY_IS_TRUE(t.GetEnableThinkTime() == false);
            VERIFY_IS_TRUE(t.GetSequentialScanHint() == false);
            VERIFY_IS_TRUE(t.GetRandomAccessHint() == false);
            VERIFY_IS_TRUE(t.GetTemporaryFileHint() == false);
            VERIFY_IS_TRUE(t.GetUseLargePages() == false);
            VERIFY_ARE_EQUAL(t.GetThroughputInBytesPerMillisecond(), (DWORD)0);
        }
    }

    void CmdLineParserUnitTests::TestParseCmdLineBlockSize()
    {
        CmdLineParser p;
        Profile profile;
        struct Synchronization s = {};
        const char *argv[] = { "foo", "-b128K", "-w84", "-a1,4,6", "testfile.dat" };
        VERIFY_IS_TRUE(p.ParseCmdLine(_countof(argv), argv, &profile, &s) == true);
        VERIFY_IS_TRUE(profile.GetVerbose() == false);
        VERIFY_IS_TRUE(profile.GetProgress() == 0);
        VERIFY_IS_TRUE(profile.GetCmdLine().compare("foo -b128K -w84 -a1,4,6 testfile.dat") == 0);
        VERIFY_IS_TRUE(profile.GetEtwEnabled() == false);
        VERIFY_IS_TRUE(profile.GetEtwProcess() == false);
        VERIFY_IS_TRUE(profile.GetEtwThread() == false);
        VERIFY_IS_TRUE(profile.GetEtwImageLoad() == false);
        VERIFY_IS_TRUE(profile.GetEtwDiskIO() == false);
        VERIFY_IS_TRUE(profile.GetEtwMemoryPageFaults() == false);
        VERIFY_IS_TRUE(profile.GetEtwMemoryHardFaults() == false);
        VERIFY_IS_TRUE(profile.GetEtwNetwork() == false);
        VERIFY_IS_TRUE(profile.GetEtwRegistry() == false);
        VERIFY_IS_TRUE(profile.GetEtwUsePagedMemory() == false);
        VERIFY_IS_TRUE(profile.GetEtwUsePerfTimer() == false);
        VERIFY_IS_TRUE(profile.GetEtwUseSystemTimer() == false);
        VERIFY_IS_TRUE(profile.GetEtwUseCyclesCounter() == false);

        vector<TimeSpan> vSpans(profile.GetTimeSpans());
        VERIFY_ARE_EQUAL(vSpans.size(), (size_t)1);
        VERIFY_ARE_EQUAL(vSpans[0].GetDuration(), (UINT32)10);
        VERIFY_ARE_EQUAL(vSpans[0].GetWarmup(), (UINT32)5);
        VERIFY_ARE_EQUAL(vSpans[0].GetCooldown(), (UINT32)0);
        VERIFY_ARE_EQUAL(vSpans[0].GetRandSeed(), (UINT32)0);
        VERIFY_ARE_EQUAL(vSpans[0].GetThreadCount(), (DWORD)0);
        VERIFY_ARE_EQUAL(vSpans[0].GetRequestCount(), (DWORD)0);
        VERIFY_IS_TRUE(vSpans[0].GetDisableAffinity() == false);
        VERIFY_IS_TRUE(vSpans[0].GetCompletionRoutines() == false);
        VERIFY_IS_TRUE(vSpans[0].GetMeasureLatency() == false);
        VERIFY_IS_TRUE(vSpans[0].GetRandomWriteData() == false);
        VERIFY_IS_TRUE(vSpans[0].GetUseIoRing() == false);

        const auto& vMasks(vSpans[0].GetAffinityGroupMasks());
        VERIFY_ARE_EQUAL(vMasks.size(), (size_t)1);
        VERIFY_ARE_EQUAL(vMasks[0].wGroup, (WORD)0);
        VERIFY_ARE_EQUAL(vMasks[0].mask, (KAFFINITY)0x52);

        vector<Target> vTargets(vSpans[0].GetTargets());
        VERIFY_ARE_EQUAL(vTargets.size(), (size_t)1);
        Target t(vTargets[0]);

        VERIFY_IS_TRUE(t.GetPath().compare("testfile.dat") == 0);
        VERIFY_ARE_EQUAL(t.GetBlockSizeInBytes(), (DWORD)(128 * 1024));
        VERIFY_ARE_EQUAL(t.GetRequestCount(), (DWORD)2);
        VERIFY_IS_TRUE(t.GetRandomRatio() == 0);
        VERIFY_IS_TRUE(t.GetUseInterlockedSequential() == false);
        VERIFY_ARE_EQUAL(t.GetBlockAlignmentInBytes(), t.GetBlockSizeInBytes());
        VERIFY_ARE_EQUAL(t.GetBaseFileOffsetInBytes(), 0);
        VERIFY_IS_TRUE(t.GetUseParallelAsyncIO() == false);
        VERIFY_IS_TRUE(t.GetCacheMode() == TargetCacheMode::Cached);
        VERIFY_IS_TRUE(t.GetWriteThroughMode() == WriteThroughMode::Off);
        VERIFY_IS_TRUE(t.GetMemoryMappedIoMode() == MemoryMappedIoMode::Off);
        VERIFY_IS_TRUE(t.GetMemoryMappedIoFlushMode() == MemoryMappedIoFlushMode::Undefined);
        VERIFY_IS_TRUE(t.GetBypassIoMode() == BypassIoMode::Undefined);
        VERIFY_IS_TRUE(t.GetZeroWriteBuffers() == false);
        VERIFY_ARE_EQUAL(t.GetThreadsPerFile(), (DWORD)1);
        VERIFY_ARE_EQUAL(t.GetThreadStrideInBytes(), 0);
        VERIFY_IS_TRUE(t.GetCreateFile() == false);
        VERIFY_ARE_EQUAL(t.GetFileSize(), 0);
        VERIFY_ARE_EQUAL(t.GetMaxFileSize(), 0);
        VERIFY_ARE_EQUAL(t.GetWriteRatio(), (DWORD)84);
        VERIFY_IS_TRUE(t.GetUseBurstSize() == false);
        VERIFY_ARE_EQUAL(t.GetBurstSize(), (DWORD)0);
        VERIFY_ARE_EQUAL(t.GetThinkTime(), (DWORD)0);
        VERIFY_IS_TRUE(t.GetEnableThinkTime() == false);
        VERIFY_IS_TRUE(t.GetSequentialScanHint() == false);
        VERIFY_IS_TRUE(t.GetRandomAccessHint() == false);
        VERIFY_IS_TRUE(t.GetTemporaryFileHint() == false);
        VERIFY_IS_TRUE(t.GetUseLargePages() == false);
        VERIFY_ARE_EQUAL(t.GetThroughputInBytesPerMillisecond(), (DWORD)0);
    }

    void CmdLineParserUnitTests::TestParseCmdLineGroupAffinity()
    {
        CmdLineParser p;
        Profile profile;
        struct Synchronization s = {};
        const char *argv[] = { "foo", "-b128K", "-w84", "-ag", "testfile.dat" };
        VERIFY_IS_TRUE(p.ParseCmdLine(_countof(argv), argv, &profile, &s) == true);
        VERIFY_IS_TRUE(profile.GetVerbose() == false);
        VERIFY_IS_TRUE(profile.GetProgress() == 0);
        VERIFY_IS_TRUE(profile.GetCmdLine().compare("foo -b128K -w84 -ag testfile.dat") == 0);
        VERIFY_IS_TRUE(profile.GetEtwEnabled() == false);
        VERIFY_IS_TRUE(profile.GetEtwProcess() == false);
        VERIFY_IS_TRUE(profile.GetEtwThread() == false);
        VERIFY_IS_TRUE(profile.GetEtwImageLoad() == false);
        VERIFY_IS_TRUE(profile.GetEtwDiskIO() == false);
        VERIFY_IS_TRUE(profile.GetEtwMemoryPageFaults() == false);
        VERIFY_IS_TRUE(profile.GetEtwMemoryHardFaults() == false);
        VERIFY_IS_TRUE(profile.GetEtwNetwork() == false);
        VERIFY_IS_TRUE(profile.GetEtwRegistry() == false);
        VERIFY_IS_TRUE(profile.GetEtwUsePagedMemory() == false);
        VERIFY_IS_TRUE(profile.GetEtwUsePerfTimer() == false);
        VERIFY_IS_TRUE(profile.GetEtwUseSystemTimer() == false);
        VERIFY_IS_TRUE(profile.GetEtwUseCyclesCounter() == false);

        vector<TimeSpan> vSpans(profile.GetTimeSpans());
        VERIFY_ARE_EQUAL(vSpans.size(), (size_t)1);
        //TimeSpan span = vSpans[0];
        VERIFY_ARE_EQUAL(vSpans[0].GetDuration(), (UINT32)10);
        VERIFY_ARE_EQUAL(vSpans[0].GetWarmup(), (UINT32)5);
        VERIFY_ARE_EQUAL(vSpans[0].GetCooldown(), (UINT32)0);
        VERIFY_ARE_EQUAL(vSpans[0].GetRandSeed(), (UINT32)0);
        VERIFY_ARE_EQUAL(vSpans[0].GetThreadCount(), (DWORD)0);
        VERIFY_ARE_EQUAL(vSpans[0].GetRequestCount(), (DWORD)0);
        VERIFY_IS_TRUE(vSpans[0].GetDisableAffinity() == false);
        VERIFY_IS_TRUE(vSpans[0].GetCompletionRoutines() == false);
        VERIFY_IS_TRUE(vSpans[0].GetMeasureLatency() == false);
        VERIFY_IS_TRUE(vSpans[0].GetRandomWriteData() == false);
        VERIFY_IS_TRUE(vSpans[0].GetUseIoRing() == false);

        const auto& vMasks(vSpans[0].GetAffinityGroupMasks());
        VERIFY_ARE_EQUAL(vMasks.size(), (size_t)0);

        vector<Target> vTargets(vSpans[0].GetTargets());
        VERIFY_ARE_EQUAL(vTargets.size(), (size_t)1);
        Target t(vTargets[0]);

        VERIFY_IS_TRUE(t.GetPath().compare("testfile.dat") == 0);
        VERIFY_ARE_EQUAL(t.GetBlockSizeInBytes(), (DWORD)(128 * 1024));
        VERIFY_ARE_EQUAL(t.GetRequestCount(), (DWORD)2);
        VERIFY_IS_TRUE(t.GetRandomRatio() == 0);
        VERIFY_IS_TRUE(t.GetUseInterlockedSequential() == false);
        VERIFY_ARE_EQUAL(t.GetBlockAlignmentInBytes(), t.GetBlockSizeInBytes());
        VERIFY_ARE_EQUAL(t.GetBaseFileOffsetInBytes(), 0);
        VERIFY_IS_TRUE(t.GetUseParallelAsyncIO() == false);
        VERIFY_IS_TRUE(t.GetCacheMode() == TargetCacheMode::Cached);
        VERIFY_IS_TRUE(t.GetWriteThroughMode() == WriteThroughMode::Off);
        VERIFY_IS_TRUE(t.GetMemoryMappedIoMode() == MemoryMappedIoMode::Off);
        VERIFY_IS_TRUE(t.GetMemoryMappedIoFlushMode() == MemoryMappedIoFlushMode::Undefined);
        VERIFY_IS_TRUE(t.GetBypassIoMode() == BypassIoMode::Undefined);
        VERIFY_IS_TRUE(t.GetZeroWriteBuffers() == false);
        VERIFY_ARE_EQUAL(t.GetThreadsPerFile(), (DWORD)1);
        VERIFY_ARE_EQUAL(t.GetThreadStrideInBytes(), 0);
        VERIFY_IS_TRUE(t.GetCreateFile() == false);
        VERIFY_ARE_EQUAL(t.GetFileSize(), 0);
        VERIFY_ARE_EQUAL(t.GetMaxFileSize(), 0);
        VERIFY_ARE_EQUAL(t.GetWriteRatio(), (DWORD)84);
        VERIFY_IS_TRUE(t.GetUseBurstSize() == false);
        VERIFY_ARE_EQUAL(t.GetBurstSize(), (DWORD)0);
        VERIFY_ARE_EQUAL(t.GetThinkTime(), (DWORD)0);
        VERIFY_IS_TRUE(t.GetEnableThinkTime() == false);
        VERIFY_IS_TRUE(t.GetSequentialScanHint() == false);
        VERIFY_IS_TRUE(t.GetRandomAccessHint() == false);
        VERIFY_IS_TRUE(t.GetTemporaryFileHint() == false);
        VERIFY_IS_TRUE(t.GetUseLargePages() == false);
        VERIFY_ARE_EQUAL(t.GetThroughputInBytesPerMillisecond(), (DWORD)0);
    }

    void CmdLineParserUnitTests::TestParseCmdLineBaseMaxTarget()
    {
        CmdLineParser p;
        struct Synchronization s = {};

        {
            Profile profile;
            const char *argv[] = { "foo", "testfile.dat" };
            VERIFY_IS_TRUE(p.ParseCmdLine(_countof(argv), argv, &profile, &s) == true);
            VERIFY_IS_TRUE(profile.GetCmdLine().compare("foo testfile.dat") == 0);

            vector<TimeSpan> vSpans(profile.GetTimeSpans());
            VERIFY_ARE_EQUAL(vSpans.size(), (size_t)1);

            vector<Target> vTargets(vSpans[0].GetTargets());
            VERIFY_ARE_EQUAL(vTargets.size(), (size_t)1);

            // defaults = 0
            const auto& t(vTargets[0]);
            VERIFY_ARE_EQUAL(t.GetBaseFileOffsetInBytes(), (UINT64) 0);
            VERIFY_ARE_EQUAL(t.GetMaxFileSize(), (UINT64) 0);
        }

        {
            // base 5MiB
            Profile profile;
            const char *argv[] = { "foo", "-B5m", "testfile.dat" };
            VERIFY_IS_TRUE(p.ParseCmdLine(_countof(argv), argv, &profile, &s) == true);
            VERIFY_IS_TRUE(profile.GetCmdLine().compare("foo -B5m testfile.dat") == 0);

            vector<TimeSpan> vSpans(profile.GetTimeSpans());
            VERIFY_ARE_EQUAL(vSpans.size(), (size_t)1);

            vector<Target> vTargets(vSpans[0].GetTargets());
            VERIFY_ARE_EQUAL(vTargets.size(), (size_t)1);

            const auto& t(vTargets[0]);
            VERIFY_ARE_EQUAL(t.GetBaseFileOffsetInBytes(), (UINT64)(5 * 1024 * 1024));
            VERIFY_ARE_EQUAL(t.GetMaxFileSize(), (UINT64) 0);
        }

        {
            // base 5MiB, length 1MiB -> 6MiB
            Profile profile;
            const char *argv[] = { "foo", "-B5m:1m", "testfile.dat" };
            VERIFY_IS_TRUE(p.ParseCmdLine(_countof(argv), argv, &profile, &s) == true);
            VERIFY_IS_TRUE(profile.GetCmdLine().compare("foo -B5m:1m testfile.dat") == 0);

            vector<TimeSpan> vSpans(profile.GetTimeSpans());
            VERIFY_ARE_EQUAL(vSpans.size(), (size_t)1);

            vector<Target> vTargets(vSpans[0].GetTargets());
            VERIFY_ARE_EQUAL(vTargets.size(), (size_t)1);

            const auto& t(vTargets[0]);
            VERIFY_ARE_EQUAL(t.GetBaseFileOffsetInBytes(), (UINT64)(5 * 1024 * 1024));
            VERIFY_ARE_EQUAL(t.GetMaxFileSize(), (UINT64)(6 * 1024 * 1024));
        }

        {
            // base 5MiB, max 6MiB
            Profile profile;
            const char *argv[] = { "foo", "-B5m", "-f6m", "testfile.dat" };
            VERIFY_IS_TRUE(p.ParseCmdLine(_countof(argv), argv, &profile, &s) == true);
            VERIFY_IS_TRUE(profile.GetCmdLine().compare("foo -B5m -f6m testfile.dat") == 0);

            vector<TimeSpan> vSpans(profile.GetTimeSpans());
            VERIFY_ARE_EQUAL(vSpans.size(), (size_t)1);

            vector<Target> vTargets(vSpans[0].GetTargets());
            VERIFY_ARE_EQUAL(vTargets.size(), (size_t)1);

            const auto& t(vTargets[0]);
            VERIFY_ARE_EQUAL(t.GetBaseFileOffsetInBytes(), (UINT64)(5 * 1024 * 1024));
            VERIFY_ARE_EQUAL(t.GetMaxFileSize(), (UINT64)(6 * 1024 * 1024));
        }

        {
            // cannot specify -f/-Bb:l together
            Profile profile;
            const char *argv[] = { "foo", "-B5m:1m", "-f6m", "testfile.dat" };
            VERIFY_IS_FALSE(p.ParseCmdLine(_countof(argv), argv, &profile, &s) == true);
        }

        {
            // cannot specify -B twice (2x b:l)
            Profile profile;
            const char *argv[] = { "foo", "-B5m:1m", "-B5m:1m", "testfile.dat" };
            VERIFY_IS_FALSE(p.ParseCmdLine(_countof(argv), argv, &profile, &s) == true);
        }

        {
            // cannot specify -B twice (b:l and b)
            Profile profile;
            const char *argv[] = { "foo", "-B5m:1m", "-B5m", "testfile.dat" };
            VERIFY_IS_FALSE(p.ParseCmdLine(_countof(argv), argv, &profile, &s) == true);
        }

        {
            // cannot specify -B twice (b and b)
            Profile profile;
            const char *argv[] = { "foo", "-B5m", "-B5m", "testfile.dat" };
            VERIFY_IS_FALSE(p.ParseCmdLine(_countof(argv), argv, &profile, &s) == true);
        }

        {
            // cannot specify -f twice (f and f)
            Profile profile;
            const char *argv[] = { "foo", "-f5m", "-f6m", "testfile.dat" };
            VERIFY_IS_FALSE(p.ParseCmdLine(_countof(argv), argv, &profile, &s) == true);
        }

        {
            // cannot specify max twice (b:l and f)
            Profile profile;
            const char *argv[] = { "foo", "-B5m:1m", "-f6m", "testfile.dat" };
            VERIFY_IS_FALSE(p.ParseCmdLine(_countof(argv), argv, &profile, &s) == true);
        }

        {
            // junk after -Bbase
            Profile profile;
            const char *argv[] = { "foo", "-B5mx", "testfile.dat" };
            VERIFY_IS_FALSE(p.ParseCmdLine(_countof(argv), argv, &profile, &s) == true);
        }

        {
            // sep but no length
            Profile profile;
            const char *argv[] = { "foo", "-B5m:", "testfile.dat" };
            VERIFY_IS_FALSE(p.ParseCmdLine(_countof(argv), argv, &profile, &s) == true);
        }

        {
            // sep but junk length
            Profile profile;
            const char *argv[] = { "foo", "-B5m:j", "testfile.dat" };
            VERIFY_IS_FALSE(p.ParseCmdLine(_countof(argv), argv, &profile, &s) == true);
        }

        {
            // sep but bad length spec
            Profile profile;
            const char *argv[] = { "foo", "-B5m:1x", "testfile.dat" };
            VERIFY_IS_FALSE(p.ParseCmdLine(_countof(argv), argv, &profile, &s) == true);
        }

        {
            // sep but extra after length spec
            Profile profile;
            const char *argv[] = { "foo", "-B5m:1mx", "testfile.dat" };
            VERIFY_IS_FALSE(p.ParseCmdLine(_countof(argv), argv, &profile, &s) == true);
        }
    }

    void CmdLineParserUnitTests::TestParseCmdLineHintFlag()
    {
        CmdLineParser p;
        struct Synchronization s = {};

        {
            Profile profile;
            const char *argv[] = { "foo", "-b128K", "-w84", "-fs", "testfile.dat" };
            VERIFY_IS_TRUE(p.ParseCmdLine(_countof(argv), argv, &profile, &s) == true);
            VERIFY_IS_TRUE(profile.GetCmdLine().compare("foo -b128K -w84 -fs testfile.dat") == 0);
            VerifyParseCmdLineAccessHints(profile, false, true, false);
        }

        {
            Profile profile;
            const char *argv[] = { "foo", "-b128K", "-w84", "-fr", "testfile.dat" };
            VERIFY_IS_TRUE(p.ParseCmdLine(_countof(argv), argv, &profile, &s) == true);
            VERIFY_IS_TRUE(profile.GetCmdLine().compare("foo -b128K -w84 -fr testfile.dat") == 0);
            VerifyParseCmdLineAccessHints(profile, true, false, false);
        }

        {
            Profile profile;
            const char *argv[] = { "foo", "-b128K", "-w84", "-ft", "testfile.dat" };
            VERIFY_IS_TRUE(p.ParseCmdLine(_countof(argv), argv, &profile, &s) == true);
            VERIFY_IS_TRUE(profile.GetCmdLine().compare("foo -b128K -w84 -ft testfile.dat") == 0);
            VerifyParseCmdLineAccessHints(profile, false, false, true);
        }

        {
            Profile profile;
            const char *argv[] = { "foo", "-b128K", "-w84", "-frst", "testfile.dat" };
            VERIFY_IS_TRUE(p.ParseCmdLine(_countof(argv), argv, &profile, &s) == true);
            VERIFY_IS_TRUE(profile.GetCmdLine().compare("foo -b128K -w84 -frst testfile.dat") == 0);
            VerifyParseCmdLineAccessHints(profile, true, true, true);
        }
    }

    void CmdLineParserUnitTests::VerifyParseCmdLineAccessHints(Profile &profile, bool RandomAccess, bool SequentialScan, bool TemporaryFile)
    {
        VERIFY_IS_TRUE(profile.GetVerbose() == false);
        VERIFY_IS_TRUE(profile.GetProgress() == 0);
        VERIFY_IS_TRUE(profile.GetEtwEnabled() == false);
        VERIFY_IS_TRUE(profile.GetEtwProcess() == false);
        VERIFY_IS_TRUE(profile.GetEtwThread() == false);
        VERIFY_IS_TRUE(profile.GetEtwImageLoad() == false);
        VERIFY_IS_TRUE(profile.GetEtwDiskIO() == false);
        VERIFY_IS_TRUE(profile.GetEtwMemoryPageFaults() == false);
        VERIFY_IS_TRUE(profile.GetEtwMemoryHardFaults() == false);
        VERIFY_IS_TRUE(profile.GetEtwNetwork() == false);
        VERIFY_IS_TRUE(profile.GetEtwRegistry() == false);
        VERIFY_IS_TRUE(profile.GetEtwUsePagedMemory() == false);
        VERIFY_IS_TRUE(profile.GetEtwUsePerfTimer() == false);
        VERIFY_IS_TRUE(profile.GetEtwUseSystemTimer() == false);
        VERIFY_IS_TRUE(profile.GetEtwUseCyclesCounter() == false);

        vector<TimeSpan> vSpans(profile.GetTimeSpans());
        VERIFY_ARE_EQUAL(vSpans.size(), (size_t)1);
        VERIFY_ARE_EQUAL(vSpans[0].GetDuration(), (UINT32)10);
        VERIFY_ARE_EQUAL(vSpans[0].GetWarmup(), (UINT32)5);
        VERIFY_ARE_EQUAL(vSpans[0].GetCooldown(), (UINT32)0);
        VERIFY_ARE_EQUAL(vSpans[0].GetRandSeed(), (UINT32)0);
        VERIFY_ARE_EQUAL(vSpans[0].GetThreadCount(), (DWORD)0);
        VERIFY_ARE_EQUAL(vSpans[0].GetRequestCount(), (DWORD)0);
        VERIFY_IS_TRUE(vSpans[0].GetDisableAffinity() == false);
        VERIFY_IS_TRUE(vSpans[0].GetCompletionRoutines() == false);
        VERIFY_IS_TRUE(vSpans[0].GetMeasureLatency() == false);
        VERIFY_IS_TRUE(vSpans[0].GetRandomWriteData() == false);
        VERIFY_IS_TRUE(vSpans[0].GetUseIoRing() == false);

        const auto& vMasks(vSpans[0].GetAffinityGroupMasks());
        VERIFY_ARE_EQUAL(vMasks.size(), (size_t)0);

        vector<Target> vTargets(vSpans[0].GetTargets());
        VERIFY_ARE_EQUAL(vTargets.size(), (size_t)1);
        Target t(vTargets[0]);

        VERIFY_IS_TRUE(t.GetPath().compare("testfile.dat") == 0);
        VERIFY_ARE_EQUAL(t.GetBlockSizeInBytes(), (DWORD)(128 * 1024));
        VERIFY_ARE_EQUAL(t.GetRequestCount(), (DWORD)2);
        VERIFY_IS_TRUE(t.GetRandomRatio() == 0);
        VERIFY_IS_TRUE(t.GetUseInterlockedSequential() == false);
        VERIFY_ARE_EQUAL(t.GetBlockAlignmentInBytes(), t.GetBlockSizeInBytes());
        VERIFY_ARE_EQUAL(t.GetBaseFileOffsetInBytes(), 0);
        VERIFY_IS_TRUE(t.GetUseParallelAsyncIO() == false);
        VERIFY_IS_TRUE(t.GetCacheMode() == TargetCacheMode::Cached);
        VERIFY_IS_TRUE(t.GetWriteThroughMode() == WriteThroughMode::Off);
        VERIFY_IS_TRUE(t.GetMemoryMappedIoMode() == MemoryMappedIoMode::Off);
        VERIFY_IS_TRUE(t.GetMemoryMappedIoFlushMode() == MemoryMappedIoFlushMode::Undefined);
        VERIFY_IS_TRUE(t.GetBypassIoMode() == BypassIoMode::Undefined);
        VERIFY_IS_TRUE(t.GetZeroWriteBuffers() == false);
        VERIFY_ARE_EQUAL(t.GetThreadsPerFile(), (DWORD)1);
        VERIFY_ARE_EQUAL(t.GetThreadStrideInBytes(), 0);
        VERIFY_IS_TRUE(t.GetCreateFile() == false);
        VERIFY_ARE_EQUAL(t.GetFileSize(), 0);
        VERIFY_ARE_EQUAL(t.GetMaxFileSize(), 0);
        VERIFY_ARE_EQUAL(t.GetWriteRatio(), (DWORD)84);
        VERIFY_IS_TRUE(t.GetUseBurstSize() == false);
        VERIFY_ARE_EQUAL(t.GetBurstSize(), (DWORD)0);
        VERIFY_ARE_EQUAL(t.GetThinkTime(), (DWORD)0);
        VERIFY_IS_TRUE(t.GetEnableThinkTime() == false);
        VERIFY_IS_TRUE(t.GetSequentialScanHint() == SequentialScan);
        VERIFY_IS_TRUE(t.GetRandomAccessHint() == RandomAccess);
        VERIFY_IS_TRUE(t.GetTemporaryFileHint() == TemporaryFile);
        VERIFY_IS_TRUE(t.GetUseLargePages() == false);
        VERIFY_ARE_EQUAL(t.GetThroughputInBytesPerMillisecond(), (DWORD)0);
    }

    void CmdLineParserUnitTests::TestParseCmdLineDisableAllCacheMode1()
    {
        CmdLineParser p;
        Profile profile;
        struct Synchronization s = {};
        const char *argv[] = { "foo", "-b128K", "-w84", "-h", "testfile.dat" };
        VERIFY_IS_TRUE(p.ParseCmdLine(_countof(argv), argv, &profile, &s) == true);
        VERIFY_IS_TRUE(profile.GetCmdLine().compare("foo -b128K -w84 -h testfile.dat") == 0);

        VerifyParseCmdLineDisableAllCache(profile);
    }

    void CmdLineParserUnitTests::TestParseCmdLineDisableAllCacheMode2()
    {
        CmdLineParser p;
        Profile profile;
        struct Synchronization s = {};
        const char *argv[] = { "foo", "-b128K", "-w84", "-Sh", "testfile.dat" };
        VERIFY_IS_TRUE(p.ParseCmdLine(_countof(argv), argv, &profile, &s) == true);
        VERIFY_IS_TRUE(profile.GetCmdLine().compare("foo -b128K -w84 -Sh testfile.dat") == 0);

        VerifyParseCmdLineDisableAllCache(profile);
    }

    void CmdLineParserUnitTests::VerifyParseCmdLineDisableAllCache(Profile &profile)
    {
        VERIFY_IS_TRUE(profile.GetVerbose() == false);
        VERIFY_IS_TRUE(profile.GetProgress() == 0);

        VERIFY_IS_TRUE(profile.GetEtwEnabled() == false);
        VERIFY_IS_TRUE(profile.GetEtwProcess() == false);
        VERIFY_IS_TRUE(profile.GetEtwThread() == false);
        VERIFY_IS_TRUE(profile.GetEtwImageLoad() == false);
        VERIFY_IS_TRUE(profile.GetEtwDiskIO() == false);
        VERIFY_IS_TRUE(profile.GetEtwMemoryPageFaults() == false);
        VERIFY_IS_TRUE(profile.GetEtwMemoryHardFaults() == false);
        VERIFY_IS_TRUE(profile.GetEtwNetwork() == false);
        VERIFY_IS_TRUE(profile.GetEtwRegistry() == false);
        VERIFY_IS_TRUE(profile.GetEtwUsePagedMemory() == false);
        VERIFY_IS_TRUE(profile.GetEtwUsePerfTimer() == false);
        VERIFY_IS_TRUE(profile.GetEtwUseSystemTimer() == false);
        VERIFY_IS_TRUE(profile.GetEtwUseCyclesCounter() == false);

        vector<TimeSpan> vSpans(profile.GetTimeSpans());
        VERIFY_ARE_EQUAL(vSpans.size(), (size_t)1);
        VERIFY_ARE_EQUAL(vSpans[0].GetDuration(), (UINT32)10);
        VERIFY_ARE_EQUAL(vSpans[0].GetWarmup(), (UINT32)5);
        VERIFY_ARE_EQUAL(vSpans[0].GetCooldown(), (UINT32)0);
        VERIFY_ARE_EQUAL(vSpans[0].GetRandSeed(), (UINT32)0);
        VERIFY_ARE_EQUAL(vSpans[0].GetThreadCount(), (DWORD)0);
        VERIFY_ARE_EQUAL(vSpans[0].GetRequestCount(), (DWORD)0);
        VERIFY_IS_TRUE(vSpans[0].GetDisableAffinity() == false);
        VERIFY_IS_TRUE(vSpans[0].GetCompletionRoutines() == false);
        VERIFY_IS_TRUE(vSpans[0].GetMeasureLatency() == false);
        VERIFY_IS_TRUE(vSpans[0].GetRandomWriteData() == false);
        VERIFY_IS_TRUE(vSpans[0].GetUseIoRing() == false);

        const auto& vMasks(vSpans[0].GetAffinityGroupMasks());
        VERIFY_ARE_EQUAL(vMasks.size(), (size_t)0);

        vector<Target> vTargets(vSpans[0].GetTargets());
        VERIFY_ARE_EQUAL(vTargets.size(), (size_t)1);
        Target t(vTargets[0]);

        VERIFY_IS_TRUE(t.GetPath().compare("testfile.dat") == 0);
        VERIFY_ARE_EQUAL(t.GetBlockSizeInBytes(), (DWORD)(128 * 1024));
        VERIFY_ARE_EQUAL(t.GetRequestCount(), (DWORD)2);
        VERIFY_IS_TRUE(t.GetRandomRatio() == 0);
        VERIFY_IS_TRUE(t.GetUseInterlockedSequential() == false);
        VERIFY_ARE_EQUAL(t.GetBlockAlignmentInBytes(), t.GetBlockSizeInBytes());
        VERIFY_ARE_EQUAL(t.GetBaseFileOffsetInBytes(), 0);
        VERIFY_IS_TRUE(t.GetUseParallelAsyncIO() == false);
        VERIFY_IS_TRUE(t.GetCacheMode() == TargetCacheMode::DisableOSCache);
        VERIFY_IS_TRUE(t.GetWriteThroughMode() == WriteThroughMode::On);
        VERIFY_IS_TRUE(t.GetMemoryMappedIoMode() == MemoryMappedIoMode::Off);
        VERIFY_IS_TRUE(t.GetMemoryMappedIoFlushMode() == MemoryMappedIoFlushMode::Undefined);
        VERIFY_IS_TRUE(t.GetBypassIoMode() == BypassIoMode::Undefined);
        VERIFY_IS_TRUE(t.GetZeroWriteBuffers() == false);
        VERIFY_ARE_EQUAL(t.GetThreadsPerFile(), (DWORD)1);
        VERIFY_ARE_EQUAL(t.GetThreadStrideInBytes(), 0);
        VERIFY_IS_TRUE(t.GetCreateFile() == false);
        VERIFY_ARE_EQUAL(t.GetFileSize(), 0);
        VERIFY_ARE_EQUAL(t.GetMaxFileSize(), 0);
        VERIFY_ARE_EQUAL(t.GetWriteRatio(), (DWORD)84);
        VERIFY_IS_TRUE(t.GetUseBurstSize() == false);
        VERIFY_ARE_EQUAL(t.GetBurstSize(), (DWORD)0);
        VERIFY_ARE_EQUAL(t.GetThinkTime(), (DWORD)0);
        VERIFY_IS_TRUE(t.GetEnableThinkTime() == false);
        VERIFY_IS_TRUE(t.GetSequentialScanHint() == false);
        VERIFY_IS_TRUE(t.GetRandomAccessHint() == false);
        VERIFY_IS_TRUE(t.GetTemporaryFileHint() == false);
        VERIFY_IS_TRUE(t.GetUseLargePages() == false);
        VERIFY_ARE_EQUAL(t.GetThroughputInBytesPerMillisecond(), (DWORD)0);
    }

    void CmdLineParserUnitTests::TestParseCmdLineConflictingCacheModes()
    {
        CmdLineParser p;
        struct Synchronization s = {};

        // conflict bu in either order
        {
            Profile profile;
            const char *argv[] = { "foo", "-Sub", "testfile.dat" };
            VERIFY_IS_FALSE(p.ParseCmdLine(_countof(argv), argv, &profile, &s));
        }

        {
            Profile profile;
            const char *argv[] = { "foo", "-Sbu", "testfile.dat" };
            VERIFY_IS_FALSE(p.ParseCmdLine(_countof(argv), argv, &profile, &s));
        }

        // conflict ru
        {
            Profile profile;
            const char *argv[] = { "foo", "-Sru", "testfile.dat" };
            VERIFY_IS_FALSE(p.ParseCmdLine(_countof(argv), argv, &profile, &s));
        }

        // conflict mu
        {
            Profile profile;
            const char *argv[] = { "foo", "-Smu", "testfile.dat" };
            VERIFY_IS_FALSE(p.ParseCmdLine(_countof(argv), argv, &profile, &s));
        }

        // conflict with -h/-Sb, either order
        {
            Profile profile;
            const char *argv[] = { "foo", "-Sb", "-h", "testfile.dat" };
            VERIFY_IS_FALSE(p.ParseCmdLine(_countof(argv), argv, &profile, &s));
        }

        {
            Profile profile;
            const char *argv[] = { "foo", "-h", "-Sb", "testfile.dat" };
            VERIFY_IS_FALSE(p.ParseCmdLine(_countof(argv), argv, &profile, &s));
        }

        // conflict with -h/-Sm
        {
            Profile profile;
            const char *argv[] = { "foo", "-Sm", "-h", "testfile.dat" };
            VERIFY_IS_FALSE(p.ParseCmdLine(_countof(argv), argv, &profile, &s));
        }

        // multiple with -h/-Suw, either order
        {
            Profile profile;
            const char *argv[] = { "foo", "-Suw", "-h", "testfile.dat" };
            VERIFY_IS_FALSE(p.ParseCmdLine(_countof(argv), argv, &profile, &s));
        }

        {
            Profile profile;
            const char *argv[] = { "foo", "-h", "-Suw", "testfile.dat" };
            VERIFY_IS_FALSE(p.ParseCmdLine(_countof(argv), argv, &profile, &s));
        }

        // multiple with -S/-Su
        {
            Profile profile;
            const char *argv[] = { "foo", "-S", "-Su", "testfile.dat" };
            VERIFY_IS_FALSE(p.ParseCmdLine(_countof(argv), argv, &profile, &s));
        }

        // multiple with -Sb/-Sb (same)
        {
            Profile profile;
            const char *argv[] = { "foo", "-Sb", "-Sb", "testfile.dat" };
            VERIFY_IS_FALSE(p.ParseCmdLine(_countof(argv), argv, &profile, &s));
        }

        // multiple with -Sm/-Sm
        {
            Profile profile;
            const char *argv[] = { "foo", "-Sm", "-Sm", "testfile.dat" };
            VERIFY_IS_FALSE(p.ParseCmdLine(_countof(argv), argv, &profile, &s));
        }

        // invalid option
        {
            Profile profile;
            const char *argv[] = { "foo", "-Sx", "testfile.dat" };
            VERIFY_IS_FALSE(p.ParseCmdLine(_countof(argv), argv, &profile, &s));
        }
    }

    void CmdLineParserUnitTests::TestParseCmdLineDisableAffinity()
    {
        CmdLineParser p;
        Profile profile;
        struct Synchronization s = {};
        const char *argv[] = { "foo", "-b128K", "-w84", "-n", "testfile.dat" };
        VERIFY_IS_TRUE(p.ParseCmdLine(_countof(argv), argv, &profile, &s) == true);
        VERIFY_IS_TRUE(profile.GetVerbose() == false);
        VERIFY_IS_TRUE(profile.GetProgress() == 0);
        VERIFY_IS_TRUE(profile.GetCmdLine().compare("foo -b128K -w84 -n testfile.dat") == 0);
        VERIFY_IS_TRUE(profile.GetEtwEnabled() == false);
        VERIFY_IS_TRUE(profile.GetEtwProcess() == false);
        VERIFY_IS_TRUE(profile.GetEtwThread() == false);
        VERIFY_IS_TRUE(profile.GetEtwImageLoad() == false);
        VERIFY_IS_TRUE(profile.GetEtwDiskIO() == false);
        VERIFY_IS_TRUE(profile.GetEtwMemoryPageFaults() == false);
        VERIFY_IS_TRUE(profile.GetEtwMemoryHardFaults() == false);
        VERIFY_IS_TRUE(profile.GetEtwNetwork() == false);
        VERIFY_IS_TRUE(profile.GetEtwRegistry() == false);
        VERIFY_IS_TRUE(profile.GetEtwUsePagedMemory() == false);
        VERIFY_IS_TRUE(profile.GetEtwUsePerfTimer() == false);
        VERIFY_IS_TRUE(profile.GetEtwUseSystemTimer() == false);
        VERIFY_IS_TRUE(profile.GetEtwUseCyclesCounter() == false);

        vector<TimeSpan> vSpans(profile.GetTimeSpans());
        VERIFY_ARE_EQUAL(vSpans.size(), (size_t)1);
        VERIFY_ARE_EQUAL(vSpans[0].GetDuration(), (UINT32)10);
        VERIFY_ARE_EQUAL(vSpans[0].GetWarmup(), (UINT32)5);
        VERIFY_ARE_EQUAL(vSpans[0].GetCooldown(), (UINT32)0);
        VERIFY_ARE_EQUAL(vSpans[0].GetRandSeed(), (UINT32)0);
        VERIFY_ARE_EQUAL(vSpans[0].GetThreadCount(), (DWORD)0);
        VERIFY_ARE_EQUAL(vSpans[0].GetRequestCount(), (DWORD)0);
        VERIFY_IS_TRUE(vSpans[0].GetDisableAffinity() == true);
        VERIFY_IS_TRUE(vSpans[0].GetCompletionRoutines() == false);
        VERIFY_IS_TRUE(vSpans[0].GetMeasureLatency() == false);
        VERIFY_IS_TRUE(vSpans[0].GetRandomWriteData() == false);
        VERIFY_IS_TRUE(vSpans[0].GetUseIoRing() == false);

        const auto& vMasks(vSpans[0].GetAffinityGroupMasks());
        VERIFY_ARE_EQUAL(vMasks.size(), (size_t)0);

        vector<Target> vTargets(vSpans[0].GetTargets());
        VERIFY_ARE_EQUAL(vTargets.size(), (size_t)1);
        Target t(vTargets[0]);

        VERIFY_IS_TRUE(t.GetPath().compare("testfile.dat") == 0);
        VERIFY_ARE_EQUAL(t.GetBlockSizeInBytes(), (DWORD)(128 * 1024));
        VERIFY_ARE_EQUAL(t.GetRequestCount(), (DWORD)2);
        VERIFY_IS_TRUE(t.GetRandomRatio() == 0);
        VERIFY_IS_TRUE(t.GetUseInterlockedSequential() == false);
        VERIFY_ARE_EQUAL(t.GetBlockAlignmentInBytes(), t.GetBlockSizeInBytes());
        VERIFY_ARE_EQUAL(t.GetBaseFileOffsetInBytes(), 0);
        VERIFY_IS_TRUE(t.GetUseParallelAsyncIO() == false);
        VERIFY_IS_TRUE(t.GetCacheMode() == TargetCacheMode::Cached);
        VERIFY_IS_TRUE(t.GetWriteThroughMode() == WriteThroughMode::Off);
        VERIFY_IS_TRUE(t.GetMemoryMappedIoMode() == MemoryMappedIoMode::Off);
        VERIFY_IS_TRUE(t.GetMemoryMappedIoFlushMode() == MemoryMappedIoFlushMode::Undefined);
        VERIFY_IS_TRUE(t.GetBypassIoMode() == BypassIoMode::Undefined);
        VERIFY_IS_TRUE(t.GetZeroWriteBuffers() == false);
        VERIFY_ARE_EQUAL(t.GetThreadsPerFile(), (DWORD)1);
        VERIFY_ARE_EQUAL(t.GetThreadStrideInBytes(), 0);
        VERIFY_IS_TRUE(t.GetCreateFile() == false);
        VERIFY_ARE_EQUAL(t.GetFileSize(), 0);
        VERIFY_ARE_EQUAL(t.GetMaxFileSize(), 0);
        VERIFY_ARE_EQUAL(t.GetWriteRatio(), (DWORD)84);
        VERIFY_IS_TRUE(t.GetUseBurstSize() == false);
        VERIFY_ARE_EQUAL(t.GetBurstSize(), (DWORD)0);
        VERIFY_ARE_EQUAL(t.GetThinkTime(), (DWORD)0);
        VERIFY_IS_TRUE(t.GetEnableThinkTime() == false);
        VERIFY_IS_TRUE(t.GetSequentialScanHint() == false);
        VERIFY_IS_TRUE(t.GetRandomAccessHint() == false);
        VERIFY_IS_TRUE(t.GetTemporaryFileHint() == false);
        VERIFY_IS_TRUE(t.GetUseLargePages() == false);
        VERIFY_ARE_EQUAL(t.GetThroughputInBytesPerMillisecond(), (DWORD)0);
    }

    void CmdLineParserUnitTests::TestParseCmdLineDisableAffinityConflict()
    {
        CmdLineParser p;
        struct Synchronization s = {};

        // -n cannot be used with -a (explicit masks)
        {
            Profile profile;
            const char *argv[] = { "foo", "-b128K", "-w84", "-n", "-a1,2", "testfile.dat" };
            VERIFY_IS_FALSE(p.ParseCmdLine(_countof(argv), argv, &profile, &s));
        }

        // -n cannot be used with -ac (core-aware traversal)
        {
            Profile profile;
            const char *argv[] = { "foo", "-n", "-ac", "testfile.dat" };
            VERIFY_IS_FALSE(p.ParseCmdLine(_countof(argv), argv, &profile, &s));
        }

        // -n cannot be used with -as (span)
        {
            Profile profile;
            const char *argv[] = { "foo", "-n", "-as", "testfile.dat" };
            VERIFY_IS_FALSE(p.ParseCmdLine(_countof(argv), argv, &profile, &s));
        }

        // -n cannot be used with -aP (efficiency ordering)
        {
            Profile profile;
            const char *argv[] = { "foo", "-n", "-aP", "testfile.dat" };
            VERIFY_IS_FALSE(p.ParseCmdLine(_countof(argv), argv, &profile, &s));
        }

        // -n cannot be used with -ae (efficiency ordering)
        {
            Profile profile;
            const char *argv[] = { "foo", "-n", "-ae", "testfile.dat" };
            VERIFY_IS_FALSE(p.ParseCmdLine(_countof(argv), argv, &profile, &s));
        }

        // -n cannot be used with -aup (efficiency opt-out)
        {
            Profile profile;
            const char *argv[] = { "foo", "-n", "-aup", "testfile.dat" };
            VERIFY_IS_FALSE(p.ParseCmdLine(_countof(argv), argv, &profile, &s));
        }

        // -n cannot be used with -acs (composed core-aware + span)
        {
            Profile profile;
            const char *argv[] = { "foo", "-n", "-acs", "testfile.dat" };
            VERIFY_IS_FALSE(p.ParseCmdLine(_countof(argv), argv, &profile, &s));
        }

        // -n cannot be used with -ap (explicit default efficiency)
        {
            Profile profile;
            const char *argv[] = { "foo", "-n", "-ap", "testfile.dat" };
            VERIFY_IS_FALSE(p.ParseCmdLine(_countof(argv), argv, &profile, &s));
        }

        // reversed order: -ac then -n
        {
            Profile profile;
            const char *argv[] = { "foo", "-ac", "-n", "testfile.dat" };
            VERIFY_IS_FALSE(p.ParseCmdLine(_countof(argv), argv, &profile, &s));
        }

        // reversed order: -aP then -n
        {
            Profile profile;
            const char *argv[] = { "foo", "-aP", "-n", "testfile.dat" };
            VERIFY_IS_FALSE(p.ParseCmdLine(_countof(argv), argv, &profile, &s));
        }

        // -n cannot be used with whole-group affinity
        {
            Profile profile;
            const char *argv[] = { "foo", "-n", "-ag0", "testfile.dat" };
            VERIFY_IS_FALSE(p.ParseCmdLine(_countof(argv), argv, &profile, &s));
        }
    }

    void CmdLineParserUnitTests::TestParseCmdLineVerbose()
    {
        CmdLineParser p;
        Profile profile;
        struct Synchronization s = {};
        const char *argv[] = { "foo", "-b128K", "-w84", "-v", "testfile.dat" };
        VERIFY_IS_TRUE(p.ParseCmdLine(_countof(argv), argv, &profile, &s) == true);
        VERIFY_IS_TRUE(profile.GetVerbose() == true);
        VERIFY_IS_TRUE(profile.GetProgress() == 0);
        VERIFY_IS_TRUE(profile.GetCmdLine().compare("foo -b128K -w84 -v testfile.dat") == 0);
        VERIFY_IS_TRUE(profile.GetEtwEnabled() == false);
        VERIFY_IS_TRUE(profile.GetEtwProcess() == false);
        VERIFY_IS_TRUE(profile.GetEtwThread() == false);
        VERIFY_IS_TRUE(profile.GetEtwImageLoad() == false);
        VERIFY_IS_TRUE(profile.GetEtwDiskIO() == false);
        VERIFY_IS_TRUE(profile.GetEtwMemoryPageFaults() == false);
        VERIFY_IS_TRUE(profile.GetEtwMemoryHardFaults() == false);
        VERIFY_IS_TRUE(profile.GetEtwNetwork() == false);
        VERIFY_IS_TRUE(profile.GetEtwRegistry() == false);
        VERIFY_IS_TRUE(profile.GetEtwUsePagedMemory() == false);
        VERIFY_IS_TRUE(profile.GetEtwUsePerfTimer() == false);
        VERIFY_IS_TRUE(profile.GetEtwUseSystemTimer() == false);
        VERIFY_IS_TRUE(profile.GetEtwUseCyclesCounter() == false);

        vector<TimeSpan> vSpans(profile.GetTimeSpans());
        VERIFY_ARE_EQUAL(vSpans.size(), (size_t)1);
        VERIFY_ARE_EQUAL(vSpans[0].GetDuration(), (UINT32)10);
        VERIFY_ARE_EQUAL(vSpans[0].GetWarmup(), (UINT32)5);
        VERIFY_ARE_EQUAL(vSpans[0].GetCooldown(), (UINT32)0);
        VERIFY_ARE_EQUAL(vSpans[0].GetRandSeed(), (UINT32)0);
        VERIFY_ARE_EQUAL(vSpans[0].GetThreadCount(), (DWORD)0);
        VERIFY_ARE_EQUAL(vSpans[0].GetRequestCount(), (DWORD)0);
        VERIFY_IS_TRUE(vSpans[0].GetDisableAffinity() == false);
        VERIFY_IS_TRUE(vSpans[0].GetCompletionRoutines() == false);
        VERIFY_IS_TRUE(vSpans[0].GetMeasureLatency() == false);
        VERIFY_IS_TRUE(vSpans[0].GetRandomWriteData() == false);
        VERIFY_IS_TRUE(vSpans[0].GetUseIoRing() == false);

        const auto& vMasks(vSpans[0].GetAffinityGroupMasks());
        VERIFY_ARE_EQUAL(vMasks.size(), (size_t)0);

        vector<Target> vTargets(vSpans[0].GetTargets());
        VERIFY_ARE_EQUAL(vTargets.size(), (size_t)1);
        Target t(vTargets[0]);

        VERIFY_IS_TRUE(t.GetPath().compare("testfile.dat") == 0);
        VERIFY_ARE_EQUAL(t.GetBlockSizeInBytes(), (DWORD)(128 * 1024));
        VERIFY_ARE_EQUAL(t.GetRequestCount(), (DWORD)2);
        VERIFY_IS_TRUE(t.GetRandomRatio() == 0);
        VERIFY_IS_TRUE(t.GetUseInterlockedSequential() == false);
        VERIFY_ARE_EQUAL(t.GetBlockAlignmentInBytes(), t.GetBlockSizeInBytes());
        VERIFY_ARE_EQUAL(t.GetBaseFileOffsetInBytes(), 0);
        VERIFY_IS_TRUE(t.GetUseParallelAsyncIO() == false);
        VERIFY_IS_TRUE(t.GetCacheMode() == TargetCacheMode::Cached);
        VERIFY_IS_TRUE(t.GetWriteThroughMode() == WriteThroughMode::Off);
        VERIFY_IS_TRUE(t.GetMemoryMappedIoMode() == MemoryMappedIoMode::Off);
        VERIFY_IS_TRUE(t.GetMemoryMappedIoFlushMode() == MemoryMappedIoFlushMode::Undefined);
        VERIFY_IS_TRUE(t.GetBypassIoMode() == BypassIoMode::Undefined);
        VERIFY_IS_TRUE(t.GetZeroWriteBuffers() == false);
        VERIFY_ARE_EQUAL(t.GetThreadsPerFile(), (DWORD)1);
        VERIFY_ARE_EQUAL(t.GetThreadStrideInBytes(), 0);
        VERIFY_IS_TRUE(t.GetCreateFile() == false);
        VERIFY_ARE_EQUAL(t.GetFileSize(), 0);
        VERIFY_ARE_EQUAL(t.GetMaxFileSize(), 0);
        VERIFY_ARE_EQUAL(t.GetWriteRatio(), (DWORD)84);
        VERIFY_IS_TRUE(t.GetUseBurstSize() == false);
        VERIFY_ARE_EQUAL(t.GetBurstSize(), (DWORD)0);
        VERIFY_ARE_EQUAL(t.GetThinkTime(), (DWORD)0);
        VERIFY_IS_TRUE(t.GetEnableThinkTime() == false);
        VERIFY_IS_TRUE(t.GetSequentialScanHint() == false);
        VERIFY_IS_TRUE(t.GetRandomAccessHint() == false);
        VERIFY_IS_TRUE(t.GetTemporaryFileHint() == false);
        VERIFY_IS_TRUE(t.GetUseLargePages() == false);
        VERIFY_ARE_EQUAL(t.GetThroughputInBytesPerMillisecond(), (DWORD)0);
    }

    void CmdLineParserUnitTests::TestParseCmdLineDisableOSCache()
    {
        CmdLineParser p;
        Profile profile;
        struct Synchronization s = {};
        const char *argv[] = { "foo", "-b128K", "-w84", "-S", "testfile.dat" };
        VERIFY_IS_TRUE(p.ParseCmdLine(_countof(argv), argv, &profile, &s) == true);
        VERIFY_IS_TRUE(profile.GetVerbose() == false);
        VERIFY_IS_TRUE(profile.GetProgress() == 0);
        VERIFY_IS_TRUE(profile.GetCmdLine().compare("foo -b128K -w84 -S testfile.dat") == 0);
        VERIFY_IS_TRUE(profile.GetEtwEnabled() == false);
        VERIFY_IS_TRUE(profile.GetEtwProcess() == false);
        VERIFY_IS_TRUE(profile.GetEtwThread() == false);
        VERIFY_IS_TRUE(profile.GetEtwImageLoad() == false);
        VERIFY_IS_TRUE(profile.GetEtwDiskIO() == false);
        VERIFY_IS_TRUE(profile.GetEtwMemoryPageFaults() == false);
        VERIFY_IS_TRUE(profile.GetEtwMemoryHardFaults() == false);
        VERIFY_IS_TRUE(profile.GetEtwNetwork() == false);
        VERIFY_IS_TRUE(profile.GetEtwRegistry() == false);
        VERIFY_IS_TRUE(profile.GetEtwUsePagedMemory() == false);
        VERIFY_IS_TRUE(profile.GetEtwUsePerfTimer() == false);
        VERIFY_IS_TRUE(profile.GetEtwUseSystemTimer() == false);
        VERIFY_IS_TRUE(profile.GetEtwUseCyclesCounter() == false);

        vector<TimeSpan> vSpans(profile.GetTimeSpans());
        VERIFY_ARE_EQUAL(vSpans.size(), (size_t)1);
        VERIFY_ARE_EQUAL(vSpans[0].GetDuration(), (UINT32)10);
        VERIFY_ARE_EQUAL(vSpans[0].GetWarmup(), (UINT32)5);
        VERIFY_ARE_EQUAL(vSpans[0].GetCooldown(), (UINT32)0);
        VERIFY_ARE_EQUAL(vSpans[0].GetRandSeed(), (UINT32)0);
        VERIFY_ARE_EQUAL(vSpans[0].GetThreadCount(), (DWORD)0);
        VERIFY_ARE_EQUAL(vSpans[0].GetRequestCount(), (DWORD)0);
        VERIFY_IS_TRUE(vSpans[0].GetDisableAffinity() == false);
        VERIFY_IS_TRUE(vSpans[0].GetCompletionRoutines() == false);
        VERIFY_IS_TRUE(vSpans[0].GetMeasureLatency() == false);
        VERIFY_IS_TRUE(vSpans[0].GetRandomWriteData() == false);
        VERIFY_IS_TRUE(vSpans[0].GetUseIoRing() == false);

        const auto& vMasks(vSpans[0].GetAffinityGroupMasks());
        VERIFY_ARE_EQUAL(vMasks.size(), (size_t)0);

        vector<Target> vTargets(vSpans[0].GetTargets());
        VERIFY_ARE_EQUAL(vTargets.size(), (size_t)1);
        Target t(vTargets[0]);

        VERIFY_IS_TRUE(t.GetPath().compare("testfile.dat") == 0);
        VERIFY_ARE_EQUAL(t.GetBlockSizeInBytes(), (DWORD)(128 * 1024));
        VERIFY_ARE_EQUAL(t.GetRequestCount(), (DWORD)2);
        VERIFY_IS_TRUE(t.GetRandomRatio() == 0);
        VERIFY_IS_TRUE(t.GetUseInterlockedSequential() == false);
        VERIFY_ARE_EQUAL(t.GetBlockAlignmentInBytes(), t.GetBlockSizeInBytes());
        VERIFY_ARE_EQUAL(t.GetBaseFileOffsetInBytes(), 0);
        VERIFY_IS_TRUE(t.GetUseParallelAsyncIO() == false);
        VERIFY_IS_TRUE(t.GetCacheMode() == TargetCacheMode::DisableOSCache);
        VERIFY_IS_TRUE(t.GetWriteThroughMode() == WriteThroughMode::Off);
        VERIFY_IS_TRUE(t.GetMemoryMappedIoMode() == MemoryMappedIoMode::Off);
        VERIFY_IS_TRUE(t.GetMemoryMappedIoFlushMode() == MemoryMappedIoFlushMode::Undefined);
        VERIFY_IS_TRUE(t.GetBypassIoMode() == BypassIoMode::Undefined);
        VERIFY_IS_TRUE(t.GetZeroWriteBuffers() == false);
        VERIFY_ARE_EQUAL(t.GetThreadsPerFile(), (DWORD)1);
        VERIFY_ARE_EQUAL(t.GetThreadStrideInBytes(), 0);
        VERIFY_IS_TRUE(t.GetCreateFile() == false);
        VERIFY_ARE_EQUAL(t.GetFileSize(), 0);
        VERIFY_ARE_EQUAL(t.GetMaxFileSize(), 0);
        VERIFY_ARE_EQUAL(t.GetWriteRatio(), (DWORD)84);
        VERIFY_IS_TRUE(t.GetUseBurstSize() == false);
        VERIFY_ARE_EQUAL(t.GetBurstSize(), (DWORD)0);
        VERIFY_ARE_EQUAL(t.GetThinkTime(), (DWORD)0);
        VERIFY_IS_TRUE(t.GetEnableThinkTime() == false);
        VERIFY_IS_TRUE(t.GetSequentialScanHint() == false);
        VERIFY_IS_TRUE(t.GetRandomAccessHint() == false);
        VERIFY_IS_TRUE(t.GetTemporaryFileHint() == false);
        VERIFY_IS_TRUE(t.GetUseLargePages() == false);
        VERIFY_ARE_EQUAL(t.GetThroughputInBytesPerMillisecond(), (DWORD)0);
    }

    void CmdLineParserUnitTests::TestParseCmdLineDisableLocalCache()
    {
        CmdLineParser p;
        Profile profile;
        struct Synchronization s = {};
        const char *argv[] = { "foo", "-b128K", "-w84", "-Sr", "testfile.dat" };
        VERIFY_IS_TRUE(p.ParseCmdLine(_countof(argv), argv, &profile, &s) == true);
        VERIFY_IS_TRUE(profile.GetVerbose() == false);
        VERIFY_IS_TRUE(profile.GetProgress() == 0);
        VERIFY_IS_TRUE(profile.GetCmdLine().compare("foo -b128K -w84 -Sr testfile.dat") == 0);
        VERIFY_IS_TRUE(profile.GetEtwEnabled() == false);
        VERIFY_IS_TRUE(profile.GetEtwProcess() == false);
        VERIFY_IS_TRUE(profile.GetEtwThread() == false);
        VERIFY_IS_TRUE(profile.GetEtwImageLoad() == false);
        VERIFY_IS_TRUE(profile.GetEtwDiskIO() == false);
        VERIFY_IS_TRUE(profile.GetEtwMemoryPageFaults() == false);
        VERIFY_IS_TRUE(profile.GetEtwMemoryHardFaults() == false);
        VERIFY_IS_TRUE(profile.GetEtwNetwork() == false);
        VERIFY_IS_TRUE(profile.GetEtwRegistry() == false);
        VERIFY_IS_TRUE(profile.GetEtwUsePagedMemory() == false);
        VERIFY_IS_TRUE(profile.GetEtwUsePerfTimer() == false);
        VERIFY_IS_TRUE(profile.GetEtwUseSystemTimer() == false);
        VERIFY_IS_TRUE(profile.GetEtwUseCyclesCounter() == false);

        vector<TimeSpan> vSpans(profile.GetTimeSpans());
        VERIFY_ARE_EQUAL(vSpans.size(), (size_t)1);
        VERIFY_ARE_EQUAL(vSpans[0].GetDuration(), (UINT32)10);
        VERIFY_ARE_EQUAL(vSpans[0].GetWarmup(), (UINT32)5);
        VERIFY_ARE_EQUAL(vSpans[0].GetCooldown(), (UINT32)0);
        VERIFY_ARE_EQUAL(vSpans[0].GetRandSeed(), (UINT32)0);
        VERIFY_ARE_EQUAL(vSpans[0].GetThreadCount(), (DWORD)0);
        VERIFY_ARE_EQUAL(vSpans[0].GetRequestCount(), (DWORD)0);
        VERIFY_IS_TRUE(vSpans[0].GetDisableAffinity() == false);
        VERIFY_IS_TRUE(vSpans[0].GetCompletionRoutines() == false);
        VERIFY_IS_TRUE(vSpans[0].GetMeasureLatency() == false);
        VERIFY_IS_TRUE(vSpans[0].GetRandomWriteData() == false);
        VERIFY_IS_TRUE(vSpans[0].GetUseIoRing() == false);

        const auto& vMasks(vSpans[0].GetAffinityGroupMasks());
        VERIFY_ARE_EQUAL(vMasks.size(), (size_t)0);

        vector<Target> vTargets(vSpans[0].GetTargets());
        VERIFY_ARE_EQUAL(vTargets.size(), (size_t)1);
        Target t(vTargets[0]);

        VERIFY_IS_TRUE(t.GetPath().compare("testfile.dat") == 0);
        VERIFY_ARE_EQUAL(t.GetBlockSizeInBytes(), (DWORD)(128 * 1024));
        VERIFY_ARE_EQUAL(t.GetRequestCount(), (DWORD)2);
        VERIFY_IS_TRUE(t.GetRandomRatio() == 0);
        VERIFY_IS_TRUE(t.GetUseInterlockedSequential() == false);
        VERIFY_ARE_EQUAL(t.GetBlockAlignmentInBytes(), t.GetBlockSizeInBytes());
        VERIFY_ARE_EQUAL(t.GetBaseFileOffsetInBytes(), 0);
        VERIFY_IS_TRUE(t.GetUseParallelAsyncIO() == false);
        VERIFY_IS_TRUE(t.GetCacheMode() == TargetCacheMode::DisableLocalCache);
        VERIFY_IS_TRUE(t.GetWriteThroughMode() == WriteThroughMode::Off);
        VERIFY_IS_TRUE(t.GetMemoryMappedIoMode() == MemoryMappedIoMode::Off);
        VERIFY_IS_TRUE(t.GetMemoryMappedIoFlushMode() == MemoryMappedIoFlushMode::Undefined);
        VERIFY_IS_TRUE(t.GetBypassIoMode() == BypassIoMode::Undefined);
        VERIFY_IS_TRUE(t.GetZeroWriteBuffers() == false);
        VERIFY_ARE_EQUAL(t.GetThreadsPerFile(), (DWORD)1);
        VERIFY_ARE_EQUAL(t.GetThreadStrideInBytes(), 0);
        VERIFY_IS_TRUE(t.GetCreateFile() == false);
        VERIFY_ARE_EQUAL(t.GetFileSize(), 0);
        VERIFY_ARE_EQUAL(t.GetMaxFileSize(), 0);
        VERIFY_ARE_EQUAL(t.GetWriteRatio(), (DWORD)84);
        VERIFY_IS_TRUE(t.GetUseBurstSize() == false);
        VERIFY_ARE_EQUAL(t.GetBurstSize(), (DWORD)0);
        VERIFY_ARE_EQUAL(t.GetThinkTime(), (DWORD)0);
        VERIFY_IS_TRUE(t.GetEnableThinkTime() == false);
        VERIFY_IS_TRUE(t.GetSequentialScanHint() == false);
        VERIFY_IS_TRUE(t.GetRandomAccessHint() == false);
        VERIFY_IS_TRUE(t.GetTemporaryFileHint() == false);
        VERIFY_IS_TRUE(t.GetUseLargePages() == false);
        VERIFY_ARE_EQUAL(t.GetThroughputInBytesPerMillisecond(), (DWORD)0);
    }

    void CmdLineParserUnitTests::TestParseCmdLineBufferedWriteThrough()
    {
        CmdLineParser p;
        Profile profile;
        struct Synchronization s = {};
        const char *argv[] = { "foo", "-b128K", "-w84", "-Sbw", "testfile.dat" };
        VERIFY_IS_TRUE(p.ParseCmdLine(_countof(argv), argv, &profile, &s) == true);
        VERIFY_IS_TRUE(profile.GetVerbose() == false);
        VERIFY_IS_TRUE(profile.GetProgress() == 0);
        VERIFY_IS_TRUE(profile.GetCmdLine().compare("foo -b128K -w84 -Sbw testfile.dat") == 0);
        VERIFY_IS_TRUE(profile.GetEtwEnabled() == false);
        VERIFY_IS_TRUE(profile.GetEtwProcess() == false);
        VERIFY_IS_TRUE(profile.GetEtwThread() == false);
        VERIFY_IS_TRUE(profile.GetEtwImageLoad() == false);
        VERIFY_IS_TRUE(profile.GetEtwDiskIO() == false);
        VERIFY_IS_TRUE(profile.GetEtwMemoryPageFaults() == false);
        VERIFY_IS_TRUE(profile.GetEtwMemoryHardFaults() == false);
        VERIFY_IS_TRUE(profile.GetEtwNetwork() == false);
        VERIFY_IS_TRUE(profile.GetEtwRegistry() == false);
        VERIFY_IS_TRUE(profile.GetEtwUsePagedMemory() == false);
        VERIFY_IS_TRUE(profile.GetEtwUsePerfTimer() == false);
        VERIFY_IS_TRUE(profile.GetEtwUseSystemTimer() == false);
        VERIFY_IS_TRUE(profile.GetEtwUseCyclesCounter() == false);

        vector<TimeSpan> vSpans(profile.GetTimeSpans());
        VERIFY_ARE_EQUAL(vSpans.size(), (size_t)1);
        VERIFY_ARE_EQUAL(vSpans[0].GetDuration(), (UINT32)10);
        VERIFY_ARE_EQUAL(vSpans[0].GetWarmup(), (UINT32)5);
        VERIFY_ARE_EQUAL(vSpans[0].GetCooldown(), (UINT32)0);
        VERIFY_ARE_EQUAL(vSpans[0].GetRandSeed(), (UINT32)0);
        VERIFY_ARE_EQUAL(vSpans[0].GetThreadCount(), (DWORD)0);
        VERIFY_ARE_EQUAL(vSpans[0].GetRequestCount(), (DWORD)0);
        VERIFY_IS_TRUE(vSpans[0].GetDisableAffinity() == false);
        VERIFY_IS_TRUE(vSpans[0].GetCompletionRoutines() == false);
        VERIFY_IS_TRUE(vSpans[0].GetMeasureLatency() == false);
        VERIFY_IS_TRUE(vSpans[0].GetRandomWriteData() == false);
        VERIFY_IS_TRUE(vSpans[0].GetUseIoRing() == false);

        const auto& vMasks(vSpans[0].GetAffinityGroupMasks());
        VERIFY_ARE_EQUAL(vMasks.size(), (size_t)0);

        vector<Target> vTargets(vSpans[0].GetTargets());
        VERIFY_ARE_EQUAL(vTargets.size(), (size_t)1);
        Target t(vTargets[0]);

        VERIFY_IS_TRUE(t.GetPath().compare("testfile.dat") == 0);
        VERIFY_ARE_EQUAL(t.GetBlockSizeInBytes(), (DWORD)(128 * 1024));
        VERIFY_ARE_EQUAL(t.GetRequestCount(), (DWORD)2);
        VERIFY_IS_TRUE(t.GetRandomRatio() == 0);
        VERIFY_IS_TRUE(t.GetUseInterlockedSequential() == false);
        VERIFY_ARE_EQUAL(t.GetBlockAlignmentInBytes(), t.GetBlockSizeInBytes());
        VERIFY_ARE_EQUAL(t.GetBaseFileOffsetInBytes(), 0);
        VERIFY_IS_TRUE(t.GetUseParallelAsyncIO() == false);
        VERIFY_IS_TRUE(t.GetCacheMode() == TargetCacheMode::Cached);
        VERIFY_IS_TRUE(t.GetWriteThroughMode() == WriteThroughMode::On);
        VERIFY_IS_TRUE(t.GetMemoryMappedIoMode() == MemoryMappedIoMode::Off);
        VERIFY_IS_TRUE(t.GetMemoryMappedIoFlushMode() == MemoryMappedIoFlushMode::Undefined);
        VERIFY_IS_TRUE(t.GetBypassIoMode() == BypassIoMode::Undefined);
        VERIFY_IS_TRUE(t.GetZeroWriteBuffers() == false);
        VERIFY_ARE_EQUAL(t.GetThreadsPerFile(), (DWORD)1);
        VERIFY_ARE_EQUAL(t.GetThreadStrideInBytes(), 0);
        VERIFY_IS_TRUE(t.GetCreateFile() == false);
        VERIFY_ARE_EQUAL(t.GetFileSize(), 0);
        VERIFY_ARE_EQUAL(t.GetMaxFileSize(), 0);
        VERIFY_ARE_EQUAL(t.GetWriteRatio(), (DWORD)84);
        VERIFY_IS_TRUE(t.GetUseBurstSize() == false);
        VERIFY_ARE_EQUAL(t.GetBurstSize(), (DWORD)0);
        VERIFY_ARE_EQUAL(t.GetThinkTime(), (DWORD)0);
        VERIFY_IS_TRUE(t.GetEnableThinkTime() == false);
        VERIFY_IS_TRUE(t.GetSequentialScanHint() == false);
        VERIFY_IS_TRUE(t.GetRandomAccessHint() == false);
        VERIFY_IS_TRUE(t.GetTemporaryFileHint() == false);
        VERIFY_IS_TRUE(t.GetUseLargePages() == false);
        VERIFY_ARE_EQUAL(t.GetThroughputInBytesPerMillisecond(), (DWORD)0);
    }

    void CmdLineParserUnitTests::VerifyParseCmdLineMappedIO(Profile &profile, MemoryMappedIoFlushMode FlushMode)
    {
        VERIFY_IS_TRUE(profile.GetVerbose() == false);
        VERIFY_IS_TRUE(profile.GetProgress() == 0);

        VERIFY_IS_TRUE(profile.GetEtwEnabled() == false);
        VERIFY_IS_TRUE(profile.GetEtwProcess() == false);
        VERIFY_IS_TRUE(profile.GetEtwThread() == false);
        VERIFY_IS_TRUE(profile.GetEtwImageLoad() == false);
        VERIFY_IS_TRUE(profile.GetEtwDiskIO() == false);
        VERIFY_IS_TRUE(profile.GetEtwMemoryPageFaults() == false);
        VERIFY_IS_TRUE(profile.GetEtwMemoryHardFaults() == false);
        VERIFY_IS_TRUE(profile.GetEtwNetwork() == false);
        VERIFY_IS_TRUE(profile.GetEtwRegistry() == false);
        VERIFY_IS_TRUE(profile.GetEtwUsePagedMemory() == false);
        VERIFY_IS_TRUE(profile.GetEtwUsePerfTimer() == false);
        VERIFY_IS_TRUE(profile.GetEtwUseSystemTimer() == false);
        VERIFY_IS_TRUE(profile.GetEtwUseCyclesCounter() == false);

        vector<TimeSpan> vSpans(profile.GetTimeSpans());
        VERIFY_ARE_EQUAL(vSpans.size(), (size_t)1);
        VERIFY_ARE_EQUAL(vSpans[0].GetDuration(), (UINT32)10);
        VERIFY_ARE_EQUAL(vSpans[0].GetWarmup(), (UINT32)5);
        VERIFY_ARE_EQUAL(vSpans[0].GetCooldown(), (UINT32)0);
        VERIFY_ARE_EQUAL(vSpans[0].GetRandSeed(), (UINT32)0);
        VERIFY_ARE_EQUAL(vSpans[0].GetThreadCount(), (DWORD)0);
        VERIFY_ARE_EQUAL(vSpans[0].GetRequestCount(), (DWORD)0);
        VERIFY_IS_TRUE(vSpans[0].GetDisableAffinity() == false);
        VERIFY_IS_TRUE(vSpans[0].GetCompletionRoutines() == false);
        VERIFY_IS_TRUE(vSpans[0].GetMeasureLatency() == false);
        VERIFY_IS_TRUE(vSpans[0].GetRandomWriteData() == false);
        VERIFY_IS_TRUE(vSpans[0].GetUseIoRing() == false);

        const auto& vMasks(vSpans[0].GetAffinityGroupMasks());
        VERIFY_ARE_EQUAL(vMasks.size(), (size_t)0);

        vector<Target> vTargets(vSpans[0].GetTargets());
        VERIFY_ARE_EQUAL(vTargets.size(), (size_t)1);
        Target t(vTargets[0]);

        VERIFY_IS_TRUE(t.GetPath().compare("testfile.dat") == 0);
        VERIFY_ARE_EQUAL(t.GetBlockSizeInBytes(), (DWORD)(128 * 1024));
        VERIFY_ARE_EQUAL(t.GetRequestCount(), (DWORD)2);
        VERIFY_IS_TRUE(t.GetRandomRatio() == 0);
        VERIFY_IS_TRUE(t.GetUseInterlockedSequential() == false);
        VERIFY_ARE_EQUAL(t.GetBlockAlignmentInBytes(), t.GetBlockSizeInBytes());
        VERIFY_ARE_EQUAL(t.GetBaseFileOffsetInBytes(), 0);
        VERIFY_IS_TRUE(t.GetUseParallelAsyncIO() == false);
        VERIFY_IS_TRUE(t.GetCacheMode() == TargetCacheMode::Cached);
        VERIFY_IS_TRUE(t.GetWriteThroughMode() == WriteThroughMode::Off);
        VERIFY_IS_TRUE(t.GetMemoryMappedIoMode() == MemoryMappedIoMode::On);
        VERIFY_IS_TRUE(t.GetMemoryMappedIoFlushMode() == FlushMode);
        VERIFY_IS_TRUE(t.GetBypassIoMode() == BypassIoMode::Undefined);
        VERIFY_IS_TRUE(t.GetZeroWriteBuffers() == false);
        VERIFY_ARE_EQUAL(t.GetThreadsPerFile(), (DWORD)1);
        VERIFY_ARE_EQUAL(t.GetThreadStrideInBytes(), 0);
        VERIFY_IS_TRUE(t.GetCreateFile() == false);
        VERIFY_ARE_EQUAL(t.GetFileSize(), 0);
        VERIFY_ARE_EQUAL(t.GetMaxFileSize(), 0);
        VERIFY_ARE_EQUAL(t.GetWriteRatio(), (DWORD)84);
        VERIFY_IS_TRUE(t.GetUseBurstSize() == false);
        VERIFY_ARE_EQUAL(t.GetBurstSize(), (DWORD)0);
        VERIFY_ARE_EQUAL(t.GetThinkTime(), (DWORD)0);
        VERIFY_IS_TRUE(t.GetEnableThinkTime() == false);
        VERIFY_IS_TRUE(t.GetSequentialScanHint() == false);
        VERIFY_IS_TRUE(t.GetRandomAccessHint() == false);
        VERIFY_IS_TRUE(t.GetTemporaryFileHint() == false);
        VERIFY_IS_TRUE(t.GetUseLargePages() == false);
        VERIFY_ARE_EQUAL(t.GetThroughputInBytesPerMillisecond(), (DWORD)0);
    }

    void CmdLineParserUnitTests::TestParseCmdLineMappedIO()
    {
        {
            CmdLineParser p;
            Profile profile;
            struct Synchronization s = {};
            const char *argv[] = { "foo", "-b128K", "-w84", "-Sm", "testfile.dat" };
            VERIFY_IS_TRUE(p.ParseCmdLine(_countof(argv), argv, &profile, &s) == true);
            VERIFY_IS_TRUE(profile.GetCmdLine().compare("foo -b128K -w84 -Sm testfile.dat") == 0);
            VerifyParseCmdLineMappedIO(profile, MemoryMappedIoFlushMode::Undefined);
        }

        {
            CmdLineParser p;
            Profile profile;
            struct Synchronization s = {};
            const char *argv[] = { "foo", "-b128K", "-w84", "-Sm", "-Nv", "testfile.dat" };
            VERIFY_IS_TRUE(p.ParseCmdLine(_countof(argv), argv, &profile, &s) == true);
            VERIFY_IS_TRUE(profile.GetCmdLine().compare("foo -b128K -w84 -Sm -Nv testfile.dat") == 0);
            VerifyParseCmdLineMappedIO(profile, MemoryMappedIoFlushMode::ViewOfFile);
        }

        {
            CmdLineParser p;
            Profile profile;
            struct Synchronization s = {};
            const char *argv[] = { "foo", "-b128K", "-w84", "-Sm", "-Nn", "testfile.dat" };
            VERIFY_IS_TRUE(p.ParseCmdLine(_countof(argv), argv, &profile, &s) == true);
            VERIFY_IS_TRUE(profile.GetCmdLine().compare("foo -b128K -w84 -Sm -Nn testfile.dat") == 0);
            VerifyParseCmdLineMappedIO(profile, MemoryMappedIoFlushMode::NonVolatileMemory);
        }

        {
            CmdLineParser p;
            Profile profile;
            struct Synchronization s = {};
            const char *argv[] = { "foo", "-b128K", "-w84", "-Sm", "-Ni", "testfile.dat" };
            VERIFY_IS_TRUE(p.ParseCmdLine(_countof(argv), argv, &profile, &s) == true);
            VERIFY_IS_TRUE(profile.GetCmdLine().compare("foo -b128K -w84 -Sm -Ni testfile.dat") == 0);
            VerifyParseCmdLineMappedIO(profile, MemoryMappedIoFlushMode::NonVolatileMemoryNoDrain);
        }
    }

    void CmdLineParserUnitTests::VerifyParseCmdLineBypassIO(Profile &profile, BypassIoMode BypassIoMode)
    {
        VERIFY_IS_TRUE(profile.GetVerbose() == false);
        VERIFY_IS_TRUE(profile.GetProgress() == 0);

        VERIFY_IS_TRUE(profile.GetEtwEnabled() == false);
        VERIFY_IS_TRUE(profile.GetEtwProcess() == false);
        VERIFY_IS_TRUE(profile.GetEtwThread() == false);
        VERIFY_IS_TRUE(profile.GetEtwImageLoad() == false);
        VERIFY_IS_TRUE(profile.GetEtwDiskIO() == false);
        VERIFY_IS_TRUE(profile.GetEtwMemoryPageFaults() == false);
        VERIFY_IS_TRUE(profile.GetEtwMemoryHardFaults() == false);
        VERIFY_IS_TRUE(profile.GetEtwNetwork() == false);
        VERIFY_IS_TRUE(profile.GetEtwRegistry() == false);
        VERIFY_IS_TRUE(profile.GetEtwUsePagedMemory() == false);
        VERIFY_IS_TRUE(profile.GetEtwUsePerfTimer() == false);
        VERIFY_IS_TRUE(profile.GetEtwUseSystemTimer() == false);
        VERIFY_IS_TRUE(profile.GetEtwUseCyclesCounter() == false);

        vector<TimeSpan> vSpans(profile.GetTimeSpans());
        VERIFY_ARE_EQUAL(vSpans.size(), (size_t)1);
        VERIFY_ARE_EQUAL(vSpans[0].GetDuration(), (UINT32)10);
        VERIFY_ARE_EQUAL(vSpans[0].GetWarmup(), (UINT32)5);
        VERIFY_ARE_EQUAL(vSpans[0].GetCooldown(), (UINT32)0);
        VERIFY_ARE_EQUAL(vSpans[0].GetRandSeed(), (UINT32)0);
        VERIFY_ARE_EQUAL(vSpans[0].GetThreadCount(), (DWORD)0);
        VERIFY_ARE_EQUAL(vSpans[0].GetRequestCount(), (DWORD)0);
        VERIFY_IS_TRUE(vSpans[0].GetDisableAffinity() == false);
        VERIFY_IS_TRUE(vSpans[0].GetCompletionRoutines() == false);
        VERIFY_IS_TRUE(vSpans[0].GetMeasureLatency() == false);
        VERIFY_IS_TRUE(vSpans[0].GetRandomWriteData() == false);
        VERIFY_IS_TRUE(vSpans[0].GetUseIoRing() == false);

        VERIFY_IS_TRUE(vSpans[0].GetAffinityGroupMasks().empty());

        vector<Target> vTargets(vSpans[0].GetTargets());
        VERIFY_ARE_EQUAL(vTargets.size(), (size_t)1);
        Target t(vTargets[0]);

        VERIFY_IS_TRUE(t.GetPath().compare("testfile.dat") == 0);
        VERIFY_ARE_EQUAL(t.GetBlockSizeInBytes(), (DWORD)(128 * 1024));
        VERIFY_ARE_EQUAL(t.GetRequestCount(), (DWORD)2);
        VERIFY_IS_TRUE(t.GetRandomRatio() == 0);
        VERIFY_IS_TRUE(t.GetUseInterlockedSequential() == false);
        VERIFY_ARE_EQUAL(t.GetBlockAlignmentInBytes(), t.GetBlockSizeInBytes());
        VERIFY_ARE_EQUAL(t.GetBaseFileOffsetInBytes(), 0);
        VERIFY_IS_TRUE(t.GetUseParallelAsyncIO() == false);
        VERIFY_IS_TRUE(t.GetCacheMode() == TargetCacheMode::DisableOSCache);
        VERIFY_IS_TRUE(t.GetWriteThroughMode() == WriteThroughMode::Off);
        VERIFY_IS_TRUE(t.GetMemoryMappedIoMode() == MemoryMappedIoMode::Off);
        VERIFY_IS_TRUE(t.GetMemoryMappedIoFlushMode() == MemoryMappedIoFlushMode::Undefined);
        VERIFY_IS_TRUE(t.GetBypassIoMode() == BypassIoMode);
        VERIFY_IS_TRUE(t.GetZeroWriteBuffers() == false);
        VERIFY_ARE_EQUAL(t.GetThreadsPerFile(), (DWORD)1);
        VERIFY_ARE_EQUAL(t.GetThreadStrideInBytes(), 0);
        VERIFY_IS_TRUE(t.GetCreateFile() == false);
        VERIFY_ARE_EQUAL(t.GetFileSize(), 0);
        VERIFY_ARE_EQUAL(t.GetMaxFileSize(), 0);
        VERIFY_ARE_EQUAL(t.GetWriteRatio(), (DWORD)0);
        VERIFY_IS_TRUE(t.GetUseBurstSize() == false);
        VERIFY_ARE_EQUAL(t.GetBurstSize(), (DWORD)0);
        VERIFY_ARE_EQUAL(t.GetThinkTime(), (DWORD)0);
        VERIFY_IS_TRUE(t.GetEnableThinkTime() == false);
        VERIFY_IS_TRUE(t.GetSequentialScanHint() == false);
        VERIFY_IS_TRUE(t.GetRandomAccessHint() == false);
        VERIFY_IS_TRUE(t.GetTemporaryFileHint() == false);
        VERIFY_IS_TRUE(t.GetUseLargePages() == false);
        VERIFY_ARE_EQUAL(t.GetThroughputInBytesPerMillisecond(), (DWORD)0);
    }

    void CmdLineParserUnitTests::TestParseCmdLineBypassIO()
    {
        // Test BypassIO while allowing partial bypass (-Sy)
        {
            CmdLineParser p;
            Profile profile;
            struct Synchronization s = {};
            const char *argv[] = { "foo", "-b128K", "-Su", "-Sy", "testfile.dat" };
            VERIFY_IS_TRUE(p.ParseCmdLine(_countof(argv), argv, &profile, &s) == true);
            VERIFY_IS_TRUE(profile.GetCmdLine().compare("foo -b128K -Su -Sy testfile.dat") == 0);
            VerifyParseCmdLineBypassIO(profile, BypassIoMode::Partial);
        }

        // Test BypassIO (-SY)
        {
            CmdLineParser p;
            Profile profile;
            struct Synchronization s = {};
            const char *argv[] = { "foo", "-b128K", "-Su", "-SY", "testfile.dat" };
            VERIFY_IS_TRUE(p.ParseCmdLine(_countof(argv), argv, &profile, &s) == true);
            VERIFY_IS_TRUE(profile.GetCmdLine().compare("foo -b128K -Su -SY testfile.dat") == 0);
            VerifyParseCmdLineBypassIO(profile, BypassIoMode::Full);
        }
    }

    void CmdLineParserUnitTests::TestParseCmdLineBypassIOConflicts()
    {
        CmdLineParser p;
        struct Synchronization s = {};

        // multiple with -Sy/-Sy
        {
            Profile profile;
            const char *argv[] = { "foo", "-Su", "-Sy", "-Sy", "testfile.dat" };
            VERIFY_IS_FALSE(p.ParseCmdLine(_countof(argv), argv, &profile, &s));
        }

        // multiple with -SY/-SY
        {
            Profile profile;
            const char *argv[] = { "foo", "-Su", "-SY", "-SY", "testfile.dat" };
            VERIFY_IS_FALSE(p.ParseCmdLine(_countof(argv), argv, &profile, &s));
        }

        // conflict Sy/SY
        {
            Profile profile;
            const char *argv[] = { "foo", "-Su", "-Sy", "-SY", "testfile.dat" };
            VERIFY_IS_FALSE(p.ParseCmdLine(_countof(argv), argv, &profile, &s));
        }

        // BypassIO (-SY or -Sy) requires unbuffered IO (-Su)
        {
            Profile profile;
            const char *argv[] = { "foo", "-Sy", "testfile.dat" };
            VERIFY_IS_FALSE(p.ParseCmdLine(_countof(argv), argv, &profile, &s));
        }

        {
            Profile profile;
            const char *argv[] = { "foo", "-SY", "testfile.dat" };
            VERIFY_IS_FALSE(p.ParseCmdLine(_countof(argv), argv, &profile, &s));
        }

        // BypassIO (-SY or -Sy) can't be used with write operations (-w)
        {
            Profile profile;
            const char *argv[] = { "foo", "-Su", "-Sy", "-w50", "testfile.dat" };
            VERIFY_IS_FALSE(p.ParseCmdLine(_countof(argv), argv, &profile, &s));
        }

        {
            Profile profile;
            const char *argv[] = { "foo", "-Su", "-SY", "-w50", "testfile.dat" };
            VERIFY_IS_FALSE(p.ParseCmdLine(_countof(argv), argv, &profile, &s));
        }

        // BypassIO (-SY or -Sy) can't be used with memory mapped IO (-Sm)
        {
            Profile profile;
            const char *argv[] = { "foo", "-Sy", "-Sm", "testfile.dat" };
            VERIFY_IS_FALSE(p.ParseCmdLine(_countof(argv), argv, &profile, &s));
        }

        {
            Profile profile;
            const char *argv[] = { "foo", "-SY", "-Sm", "testfile.dat" };
            VERIFY_IS_FALSE(p.ParseCmdLine(_countof(argv), argv, &profile, &s));
        }
    }

    void CmdLineParserUnitTests::TestParseCmdLineUseCompletionRoutines()
    {
        CmdLineParser p;
        Profile profile;
        struct Synchronization s = {};
        const char *argv[] = { "foo", "-b128K", "-w84", "-x", "testfile.dat" };
        VERIFY_IS_TRUE(p.ParseCmdLine(_countof(argv), argv, &profile, &s) == true);
        VERIFY_IS_TRUE(profile.GetVerbose() == false);
        VERIFY_IS_TRUE(profile.GetProgress() == 0);
        VERIFY_IS_TRUE(profile.GetCmdLine().compare("foo -b128K -w84 -x testfile.dat") == 0);
        VERIFY_IS_TRUE(profile.GetEtwEnabled() == false);
        VERIFY_IS_TRUE(profile.GetEtwProcess() == false);
        VERIFY_IS_TRUE(profile.GetEtwThread() == false);
        VERIFY_IS_TRUE(profile.GetEtwImageLoad() == false);
        VERIFY_IS_TRUE(profile.GetEtwDiskIO() == false);
        VERIFY_IS_TRUE(profile.GetEtwMemoryPageFaults() == false);
        VERIFY_IS_TRUE(profile.GetEtwMemoryHardFaults() == false);
        VERIFY_IS_TRUE(profile.GetEtwNetwork() == false);
        VERIFY_IS_TRUE(profile.GetEtwRegistry() == false);
        VERIFY_IS_TRUE(profile.GetEtwUsePagedMemory() == false);
        VERIFY_IS_TRUE(profile.GetEtwUsePerfTimer() == false);
        VERIFY_IS_TRUE(profile.GetEtwUseSystemTimer() == false);
        VERIFY_IS_TRUE(profile.GetEtwUseCyclesCounter() == false);

        vector<TimeSpan> vSpans(profile.GetTimeSpans());
        VERIFY_ARE_EQUAL(vSpans.size(), (size_t)1);
        VERIFY_ARE_EQUAL(vSpans[0].GetDuration(), (UINT32)10);
        VERIFY_ARE_EQUAL(vSpans[0].GetWarmup(), (UINT32)5);
        VERIFY_ARE_EQUAL(vSpans[0].GetCooldown(), (UINT32)0);
        VERIFY_ARE_EQUAL(vSpans[0].GetRandSeed(), (UINT32)0);
        VERIFY_ARE_EQUAL(vSpans[0].GetThreadCount(), (DWORD)0);
        VERIFY_ARE_EQUAL(vSpans[0].GetRequestCount(), (DWORD)0);
        VERIFY_IS_TRUE(vSpans[0].GetDisableAffinity() == false);
        VERIFY_IS_TRUE(vSpans[0].GetCompletionRoutines() == true);
        VERIFY_IS_TRUE(vSpans[0].GetMeasureLatency() == false);
        VERIFY_IS_TRUE(vSpans[0].GetRandomWriteData() == false);
        VERIFY_IS_TRUE(vSpans[0].GetUseIoRing() == false);

        const auto& vMasks(vSpans[0].GetAffinityGroupMasks());
        VERIFY_ARE_EQUAL(vMasks.size(), (size_t)0);

        vector<Target> vTargets(vSpans[0].GetTargets());
        VERIFY_ARE_EQUAL(vTargets.size(), (size_t)1);
        Target t(vTargets[0]);

        VERIFY_IS_TRUE(t.GetPath().compare("testfile.dat") == 0);
        VERIFY_ARE_EQUAL(t.GetBlockSizeInBytes(), (DWORD)(128 * 1024));
        VERIFY_ARE_EQUAL(t.GetRequestCount(), (DWORD)2);
        VERIFY_IS_TRUE(t.GetRandomRatio() == 0);
        VERIFY_IS_TRUE(t.GetUseInterlockedSequential() == false);
        VERIFY_ARE_EQUAL(t.GetBlockAlignmentInBytes(), t.GetBlockSizeInBytes());
        VERIFY_ARE_EQUAL(t.GetBaseFileOffsetInBytes(), 0);
        VERIFY_IS_TRUE(t.GetUseParallelAsyncIO() == false);
        VERIFY_IS_TRUE(t.GetCacheMode() == TargetCacheMode::Cached);
        VERIFY_IS_TRUE(t.GetWriteThroughMode() == WriteThroughMode::Off);
        VERIFY_IS_TRUE(t.GetMemoryMappedIoMode() == MemoryMappedIoMode::Off);
        VERIFY_IS_TRUE(t.GetMemoryMappedIoFlushMode() == MemoryMappedIoFlushMode::Undefined);
        VERIFY_IS_TRUE(t.GetBypassIoMode() == BypassIoMode::Undefined);
        VERIFY_IS_TRUE(t.GetZeroWriteBuffers() == false);
        VERIFY_ARE_EQUAL(t.GetThreadsPerFile(), (DWORD)1);
        VERIFY_ARE_EQUAL(t.GetThreadStrideInBytes(), 0);
        VERIFY_IS_TRUE(t.GetCreateFile() == false);
        VERIFY_ARE_EQUAL(t.GetFileSize(), 0);
        VERIFY_ARE_EQUAL(t.GetMaxFileSize(), 0);
        VERIFY_ARE_EQUAL(t.GetWriteRatio(), (DWORD)84);
        VERIFY_IS_TRUE(t.GetUseBurstSize() == false);
        VERIFY_ARE_EQUAL(t.GetBurstSize(), (DWORD)0);
        VERIFY_ARE_EQUAL(t.GetThinkTime(), (DWORD)0);
        VERIFY_IS_TRUE(t.GetEnableThinkTime() == false);
        VERIFY_IS_TRUE(t.GetSequentialScanHint() == false);
        VERIFY_IS_TRUE(t.GetRandomAccessHint() == false);
        VERIFY_IS_TRUE(t.GetTemporaryFileHint() == false);
        VERIFY_IS_TRUE(t.GetUseLargePages() == false);
        VERIFY_ARE_EQUAL(t.GetThroughputInBytesPerMillisecond(), (DWORD)0);
    }

    void CmdLineParserUnitTests::VerifyParseCmdLineIoRing(Profile &profile, bool UseRegBuffer, UINT32 IoRingBatchSize)
    {
        VERIFY_IS_TRUE(profile.GetVerbose() == false);
        VERIFY_IS_TRUE(profile.GetProgress() == 0);

        VERIFY_IS_TRUE(profile.GetEtwEnabled() == false);
        VERIFY_IS_TRUE(profile.GetEtwProcess() == false);
        VERIFY_IS_TRUE(profile.GetEtwThread() == false);
        VERIFY_IS_TRUE(profile.GetEtwImageLoad() == false);
        VERIFY_IS_TRUE(profile.GetEtwDiskIO() == false);
        VERIFY_IS_TRUE(profile.GetEtwMemoryPageFaults() == false);
        VERIFY_IS_TRUE(profile.GetEtwMemoryHardFaults() == false);
        VERIFY_IS_TRUE(profile.GetEtwNetwork() == false);
        VERIFY_IS_TRUE(profile.GetEtwRegistry() == false);
        VERIFY_IS_TRUE(profile.GetEtwUsePagedMemory() == false);
        VERIFY_IS_TRUE(profile.GetEtwUsePerfTimer() == false);
        VERIFY_IS_TRUE(profile.GetEtwUseSystemTimer() == false);
        VERIFY_IS_TRUE(profile.GetEtwUseCyclesCounter() == false);

        vector<TimeSpan> vSpans(profile.GetTimeSpans());
        VERIFY_ARE_EQUAL(vSpans.size(), (size_t)1);
        VERIFY_ARE_EQUAL(vSpans[0].GetDuration(), (UINT32)10);
        VERIFY_ARE_EQUAL(vSpans[0].GetWarmup(), (UINT32)5);
        VERIFY_ARE_EQUAL(vSpans[0].GetCooldown(), (UINT32)0);
        VERIFY_ARE_EQUAL(vSpans[0].GetRandSeed(), (UINT32)0);
        VERIFY_ARE_EQUAL(vSpans[0].GetThreadCount(), (DWORD)0);
        VERIFY_ARE_EQUAL(vSpans[0].GetRequestCount(), (DWORD)0);
        VERIFY_IS_TRUE(vSpans[0].GetDisableAffinity() == false);
        VERIFY_IS_TRUE(vSpans[0].GetCompletionRoutines() == false);
        VERIFY_IS_TRUE(vSpans[0].GetMeasureLatency() == false);
        VERIFY_IS_TRUE(vSpans[0].GetRandomWriteData() == false);
        VERIFY_IS_TRUE(vSpans[0].GetUseIoRing() == true);
        VERIFY_ARE_EQUAL(vSpans[0].GetIoRingBatchSize(), IoRingBatchSize);
        VERIFY_IS_TRUE(vSpans[0].GetUseRegBuffer() == UseRegBuffer);

        VERIFY_IS_TRUE(vSpans[0].GetAffinityGroupMasks().empty());

        vector<Target> vTargets(vSpans[0].GetTargets());
        VERIFY_ARE_EQUAL(vTargets.size(), (size_t)1);
        Target t(vTargets[0]);

        VERIFY_IS_TRUE(t.GetPath().compare("testfile.dat") == 0);
        VERIFY_ARE_EQUAL(t.GetBlockSizeInBytes(), (DWORD)(128 * 1024));
        VERIFY_ARE_EQUAL(t.GetRequestCount(), (DWORD)2);
        VERIFY_IS_TRUE(t.GetRandomRatio() == 0);
        VERIFY_IS_TRUE(t.GetUseInterlockedSequential() == false);
        VERIFY_ARE_EQUAL(t.GetBlockAlignmentInBytes(), t.GetBlockSizeInBytes());
        VERIFY_ARE_EQUAL(t.GetBaseFileOffsetInBytes(), 0);
        VERIFY_IS_TRUE(t.GetUseParallelAsyncIO() == false);
        VERIFY_IS_TRUE(t.GetCacheMode() == TargetCacheMode::Cached);
        VERIFY_IS_TRUE(t.GetWriteThroughMode() == WriteThroughMode::Off);
        VERIFY_IS_TRUE(t.GetMemoryMappedIoMode() == MemoryMappedIoMode::Off);
        VERIFY_IS_TRUE(t.GetMemoryMappedIoFlushMode() == MemoryMappedIoFlushMode::Undefined);
        VERIFY_IS_TRUE(t.GetBypassIoMode() == BypassIoMode::Undefined);
        VERIFY_IS_TRUE(t.GetZeroWriteBuffers() == false);
        VERIFY_ARE_EQUAL(t.GetThreadsPerFile(), (DWORD)1);
        VERIFY_ARE_EQUAL(t.GetThreadStrideInBytes(), 0);
        VERIFY_IS_TRUE(t.GetCreateFile() == false);
        VERIFY_ARE_EQUAL(t.GetFileSize(), 0);
        VERIFY_ARE_EQUAL(t.GetMaxFileSize(), 0);
        VERIFY_ARE_EQUAL(t.GetWriteRatio(), (DWORD)84);
        VERIFY_IS_TRUE(t.GetUseBurstSize() == false);
        VERIFY_ARE_EQUAL(t.GetBurstSize(), (DWORD)0);
        VERIFY_ARE_EQUAL(t.GetThinkTime(), (DWORD)0);
        VERIFY_IS_TRUE(t.GetEnableThinkTime() == false);
        VERIFY_IS_TRUE(t.GetSequentialScanHint() == false);
        VERIFY_IS_TRUE(t.GetRandomAccessHint() == false);
        VERIFY_IS_TRUE(t.GetTemporaryFileHint() == false);
        VERIFY_IS_TRUE(t.GetUseLargePages() == false);
        VERIFY_ARE_EQUAL(t.GetThroughputInBytesPerMillisecond(), (DWORD)0);
    }

    void CmdLineParserUnitTests::TestParseCmdLineUseIoRing()
    {
        // Test IoRing (-u)
        {
            CmdLineParser p;
            Profile profile;
            struct Synchronization s = {};
            const char *argv[] = { "foo", "-b128K", "-w84", "-u", "testfile.dat" };
            VERIFY_IS_TRUE(p.ParseCmdLine(_countof(argv), argv, &profile, &s) == true);
            VERIFY_IS_TRUE(profile.GetCmdLine().compare("foo -b128K -w84 -u testfile.dat") == 0);
            VerifyParseCmdLineIoRing(profile, false, 25);
        }

        // Test IoRing with registered buffers (-ub)
        {
            CmdLineParser p;
            Profile profile;
            struct Synchronization s = {};
            const char *argv[] = { "foo", "-b128K", "-w84", "-ub", "testfile.dat" };
            VERIFY_IS_TRUE(p.ParseCmdLine(_countof(argv), argv, &profile, &s) == true);
            VERIFY_IS_TRUE(profile.GetCmdLine().compare("foo -b128K -w84 -ub testfile.dat") == 0);
            VerifyParseCmdLineIoRing(profile, true, 25);
        }

        // Test IoRing with custom batch size (-u50)
        {
            CmdLineParser p;
            Profile profile;
            struct Synchronization s = {};
            const char *argv[] = { "foo", "-b128K", "-w84", "-u50", "testfile.dat" };
            VERIFY_IS_TRUE(p.ParseCmdLine(_countof(argv), argv, &profile, &s) == true);
            VERIFY_IS_TRUE(profile.GetCmdLine().compare("foo -b128K -w84 -u50 testfile.dat") == 0);
            VerifyParseCmdLineIoRing(profile, false, 50);
        }

        // Test IoRing with registered buffers and custom batch size (-ub75)
        {
            CmdLineParser p;
            Profile profile;
            struct Synchronization s = {};
            const char *argv[] = { "foo", "-b128K", "-w84", "-ub75", "testfile.dat" };
            VERIFY_IS_TRUE(p.ParseCmdLine(_countof(argv), argv, &profile, &s) == true);
            VERIFY_IS_TRUE(profile.GetCmdLine().compare("foo -b128K -w84 -ub75 testfile.dat") == 0);
            VerifyParseCmdLineIoRing(profile, true, 75);
        }
    }

    void CmdLineParserUnitTests::TestParseCmdLineIoRingConflicts()
    {
        CmdLineParser p;
        struct Synchronization s = {};

        // -u (IoRing) cannot be used with -x (Completion Routines)
        {
            Profile profile;
            const char *argv[] = { "foo", "-u", "-x", "testfile.dat" };
            VERIFY_IS_FALSE(p.ParseCmdLine(_countof(argv), argv, &profile, &s));
        }

        // -u (IoRing) cannot be used with -Sm (Memory Mapped IO)
        {
            Profile profile;
            const char *argv[] = { "foo", "-u", "-Sm", "testfile.dat" };
            VERIFY_IS_FALSE(p.ParseCmdLine(_countof(argv), argv, &profile, &s));
        }
    }

    void CmdLineParserUnitTests::TestParseCmdLineRandSeed()
    {
        CmdLineParser p;
        Profile profile;
        struct Synchronization s = {};
        const char *argv[] = { "foo", "-b128K", "-w84", "-z1234", "testfile.dat" };
        VERIFY_IS_TRUE(p.ParseCmdLine(_countof(argv), argv, &profile, &s) == true);
        VERIFY_IS_TRUE(profile.GetVerbose() == false);
        VERIFY_IS_TRUE(profile.GetProgress() == 0);
        VERIFY_IS_TRUE(profile.GetCmdLine().compare("foo -b128K -w84 -z1234 testfile.dat") == 0);
        VERIFY_IS_TRUE(profile.GetEtwEnabled() == false);
        VERIFY_IS_TRUE(profile.GetEtwProcess() == false);
        VERIFY_IS_TRUE(profile.GetEtwThread() == false);
        VERIFY_IS_TRUE(profile.GetEtwImageLoad() == false);
        VERIFY_IS_TRUE(profile.GetEtwDiskIO() == false);
        VERIFY_IS_TRUE(profile.GetEtwMemoryPageFaults() == false);
        VERIFY_IS_TRUE(profile.GetEtwMemoryHardFaults() == false);
        VERIFY_IS_TRUE(profile.GetEtwNetwork() == false);
        VERIFY_IS_TRUE(profile.GetEtwRegistry() == false);
        VERIFY_IS_TRUE(profile.GetEtwUsePagedMemory() == false);
        VERIFY_IS_TRUE(profile.GetEtwUsePerfTimer() == false);
        VERIFY_IS_TRUE(profile.GetEtwUseSystemTimer() == false);
        VERIFY_IS_TRUE(profile.GetEtwUseCyclesCounter() == false);

        vector<TimeSpan> vSpans(profile.GetTimeSpans());
        VERIFY_ARE_EQUAL(vSpans.size(), (size_t)1);
        VERIFY_ARE_EQUAL(vSpans[0].GetDuration(), (UINT32)10);
        VERIFY_ARE_EQUAL(vSpans[0].GetWarmup(), (UINT32)5);
        VERIFY_ARE_EQUAL(vSpans[0].GetCooldown(), (UINT32)0);
        VERIFY_ARE_EQUAL(vSpans[0].GetRandSeed(), (UINT32)1234);
        VERIFY_ARE_EQUAL(vSpans[0].GetThreadCount(), (DWORD)0);
        VERIFY_ARE_EQUAL(vSpans[0].GetRequestCount(), (DWORD)0);
        VERIFY_IS_TRUE(vSpans[0].GetDisableAffinity() == false);
        VERIFY_IS_TRUE(vSpans[0].GetCompletionRoutines() == false);
        VERIFY_IS_TRUE(vSpans[0].GetMeasureLatency() == false);
        VERIFY_IS_TRUE(vSpans[0].GetRandomWriteData() == false);
        VERIFY_IS_TRUE(vSpans[0].GetUseIoRing() == false);

        const auto& vMasks(vSpans[0].GetAffinityGroupMasks());
        VERIFY_ARE_EQUAL(vMasks.size(), (size_t)0);

        vector<Target> vTargets(vSpans[0].GetTargets());
        VERIFY_ARE_EQUAL(vTargets.size(), (size_t)1);
        Target t(vTargets[0]);

        VERIFY_IS_TRUE(t.GetPath().compare("testfile.dat") == 0);
        VERIFY_ARE_EQUAL(t.GetBlockSizeInBytes(), (DWORD)(128 * 1024));
        VERIFY_ARE_EQUAL(t.GetRequestCount(), (DWORD)2);
        VERIFY_IS_TRUE(t.GetRandomRatio() == 0);
        VERIFY_IS_TRUE(t.GetUseInterlockedSequential() == false);
        VERIFY_ARE_EQUAL(t.GetBlockAlignmentInBytes(), t.GetBlockSizeInBytes());
        VERIFY_ARE_EQUAL(t.GetBaseFileOffsetInBytes(), 0);
        VERIFY_IS_TRUE(t.GetUseParallelAsyncIO() == false);
        VERIFY_IS_TRUE(t.GetCacheMode() == TargetCacheMode::Cached);
        VERIFY_IS_TRUE(t.GetWriteThroughMode() == WriteThroughMode::Off);
        VERIFY_IS_TRUE(t.GetMemoryMappedIoMode() == MemoryMappedIoMode::Off);
        VERIFY_IS_TRUE(t.GetMemoryMappedIoFlushMode() == MemoryMappedIoFlushMode::Undefined);
        VERIFY_IS_TRUE(t.GetBypassIoMode() == BypassIoMode::Undefined);
        VERIFY_IS_TRUE(t.GetZeroWriteBuffers() == false);
        VERIFY_ARE_EQUAL(t.GetThreadsPerFile(), (DWORD)1);
        VERIFY_ARE_EQUAL(t.GetThreadStrideInBytes(), 0);
        VERIFY_IS_TRUE(t.GetCreateFile() == false);
        VERIFY_ARE_EQUAL(t.GetFileSize(), 0);
        VERIFY_ARE_EQUAL(t.GetMaxFileSize(), 0);
        VERIFY_ARE_EQUAL(t.GetWriteRatio(), (DWORD)84);
        VERIFY_IS_TRUE(t.GetUseBurstSize() == false);
        VERIFY_ARE_EQUAL(t.GetBurstSize(), (DWORD)0);
        VERIFY_ARE_EQUAL(t.GetThinkTime(), (DWORD)0);
        VERIFY_IS_TRUE(t.GetEnableThinkTime() == false);
        VERIFY_IS_TRUE(t.GetSequentialScanHint() == false);
        VERIFY_IS_TRUE(t.GetRandomAccessHint() == false);
        VERIFY_IS_TRUE(t.GetTemporaryFileHint() == false);
        VERIFY_IS_TRUE(t.GetUseLargePages() == false);
        VERIFY_ARE_EQUAL(t.GetThroughputInBytesPerMillisecond(), (DWORD)0);
    }

    void CmdLineParserUnitTests::TestParseCmdLineRandSeedGetTickCount()
    {
        CmdLineParser p;
        Profile profile;
        struct Synchronization s = {};
        const char *argv[] = { "foo", "-b128K", "-w84", "-z", "testfile.dat" };
        VERIFY_IS_TRUE(p.ParseCmdLine(_countof(argv), argv, &profile, &s) == true);
        VERIFY_IS_TRUE(profile.GetVerbose() == false);
        VERIFY_IS_TRUE(profile.GetProgress() == 0);
        VERIFY_IS_TRUE(profile.GetCmdLine().compare("foo -b128K -w84 -z testfile.dat") == 0);
        VERIFY_IS_TRUE(profile.GetEtwEnabled() == false);
        VERIFY_IS_TRUE(profile.GetEtwProcess() == false);
        VERIFY_IS_TRUE(profile.GetEtwThread() == false);
        VERIFY_IS_TRUE(profile.GetEtwImageLoad() == false);
        VERIFY_IS_TRUE(profile.GetEtwDiskIO() == false);
        VERIFY_IS_TRUE(profile.GetEtwMemoryPageFaults() == false);
        VERIFY_IS_TRUE(profile.GetEtwMemoryHardFaults() == false);
        VERIFY_IS_TRUE(profile.GetEtwNetwork() == false);
        VERIFY_IS_TRUE(profile.GetEtwRegistry() == false);
        VERIFY_IS_TRUE(profile.GetEtwUsePagedMemory() == false);
        VERIFY_IS_TRUE(profile.GetEtwUsePerfTimer() == false);
        VERIFY_IS_TRUE(profile.GetEtwUseSystemTimer() == false);
        VERIFY_IS_TRUE(profile.GetEtwUseCyclesCounter() == false);

        vector<TimeSpan> vSpans(profile.GetTimeSpans());
        VERIFY_ARE_EQUAL(vSpans.size(), (size_t)1);
        VERIFY_ARE_EQUAL(vSpans[0].GetDuration(), (UINT32)10);
        VERIFY_ARE_EQUAL(vSpans[0].GetWarmup(), (UINT32)5);
        VERIFY_ARE_EQUAL(vSpans[0].GetCooldown(), (UINT32)0);
        VERIFY_IS_TRUE(vSpans[0].GetRandSeed() != 0);
        VERIFY_ARE_EQUAL(vSpans[0].GetThreadCount(), (DWORD)0);
        VERIFY_ARE_EQUAL(vSpans[0].GetRequestCount(), (DWORD)0);
        VERIFY_IS_TRUE(vSpans[0].GetDisableAffinity() == false);
        VERIFY_IS_TRUE(vSpans[0].GetCompletionRoutines() == false);
        VERIFY_IS_TRUE(vSpans[0].GetMeasureLatency() == false);
        VERIFY_IS_TRUE(vSpans[0].GetRandomWriteData() == false);
        VERIFY_IS_TRUE(vSpans[0].GetUseIoRing() == false);

        const auto& vMasks(vSpans[0].GetAffinityGroupMasks());
        VERIFY_ARE_EQUAL(vMasks.size(), (size_t)0);

        vector<Target> vTargets(vSpans[0].GetTargets());
        VERIFY_ARE_EQUAL(vTargets.size(), (size_t)1);
        Target t(vTargets[0]);

        VERIFY_IS_TRUE(t.GetPath().compare("testfile.dat") == 0);
        VERIFY_ARE_EQUAL(t.GetBlockSizeInBytes(), (DWORD)(128 * 1024));
        VERIFY_ARE_EQUAL(t.GetRequestCount(), (DWORD)2);
        VERIFY_IS_TRUE(t.GetRandomRatio() == 0);
        VERIFY_IS_TRUE(t.GetUseInterlockedSequential() == false);
        VERIFY_ARE_EQUAL(t.GetBlockAlignmentInBytes(), t.GetBlockSizeInBytes());
        VERIFY_ARE_EQUAL(t.GetBaseFileOffsetInBytes(), 0);
        VERIFY_IS_TRUE(t.GetUseParallelAsyncIO() == false);
        VERIFY_IS_TRUE(t.GetCacheMode() == TargetCacheMode::Cached);
        VERIFY_IS_TRUE(t.GetWriteThroughMode() == WriteThroughMode::Off);
        VERIFY_IS_TRUE(t.GetMemoryMappedIoMode() == MemoryMappedIoMode::Off);
        VERIFY_IS_TRUE(t.GetMemoryMappedIoFlushMode() == MemoryMappedIoFlushMode::Undefined);
        VERIFY_IS_TRUE(t.GetBypassIoMode() == BypassIoMode::Undefined);
        VERIFY_IS_TRUE(t.GetZeroWriteBuffers() == false);
        VERIFY_ARE_EQUAL(t.GetThreadsPerFile(), (DWORD)1);
        VERIFY_ARE_EQUAL(t.GetThreadStrideInBytes(), 0);
        VERIFY_IS_TRUE(t.GetCreateFile() == false);
        VERIFY_ARE_EQUAL(t.GetFileSize(), 0);
        VERIFY_ARE_EQUAL(t.GetMaxFileSize(), 0);
        VERIFY_ARE_EQUAL(t.GetWriteRatio(), (DWORD)84);
        VERIFY_IS_TRUE(t.GetUseBurstSize() == false);
        VERIFY_ARE_EQUAL(t.GetBurstSize(), (DWORD)0);
        VERIFY_ARE_EQUAL(t.GetThinkTime(), (DWORD)0);
        VERIFY_IS_TRUE(t.GetEnableThinkTime() == false);
        VERIFY_IS_TRUE(t.GetSequentialScanHint() == false);
        VERIFY_IS_TRUE(t.GetRandomAccessHint() == false);
        VERIFY_IS_TRUE(t.GetTemporaryFileHint() == false);
        VERIFY_IS_TRUE(t.GetUseLargePages() == false);
        VERIFY_ARE_EQUAL(t.GetThroughputInBytesPerMillisecond(), (DWORD)0);
    }

    void CmdLineParserUnitTests::TestParseCmdLineWarmupAndCooldown()
    {
        CmdLineParser p;
        Profile profile;
        struct Synchronization s = {};
        const char *argv[] = { "foo", "-b128K", "-w84", "-C12", "-W345", "testfile.dat" };
        VERIFY_IS_TRUE(p.ParseCmdLine(_countof(argv), argv, &profile, &s) == true);
        VERIFY_IS_TRUE(profile.GetVerbose() == false);
        VERIFY_IS_TRUE(profile.GetProgress() == 0);
        VERIFY_IS_TRUE(profile.GetCmdLine().compare("foo -b128K -w84 -C12 -W345 testfile.dat") == 0);
        VERIFY_IS_TRUE(profile.GetEtwEnabled() == false);
        VERIFY_IS_TRUE(profile.GetEtwProcess() == false);
        VERIFY_IS_TRUE(profile.GetEtwThread() == false);
        VERIFY_IS_TRUE(profile.GetEtwImageLoad() == false);
        VERIFY_IS_TRUE(profile.GetEtwDiskIO() == false);
        VERIFY_IS_TRUE(profile.GetEtwMemoryPageFaults() == false);
        VERIFY_IS_TRUE(profile.GetEtwMemoryHardFaults() == false);
        VERIFY_IS_TRUE(profile.GetEtwNetwork() == false);
        VERIFY_IS_TRUE(profile.GetEtwRegistry() == false);
        VERIFY_IS_TRUE(profile.GetEtwUsePagedMemory() == false);
        VERIFY_IS_TRUE(profile.GetEtwUsePerfTimer() == false);
        VERIFY_IS_TRUE(profile.GetEtwUseSystemTimer() == false);
        VERIFY_IS_TRUE(profile.GetEtwUseCyclesCounter() == false);

        vector<TimeSpan> vSpans(profile.GetTimeSpans());
        VERIFY_ARE_EQUAL(vSpans.size(), (size_t)1);
        VERIFY_ARE_EQUAL(vSpans[0].GetDuration(), (UINT32)10);
        VERIFY_ARE_EQUAL(vSpans[0].GetWarmup(), (UINT32)345);
        VERIFY_ARE_EQUAL(vSpans[0].GetCooldown(), (UINT32)12);
        VERIFY_ARE_EQUAL(vSpans[0].GetRandSeed(), (UINT32)0);
        VERIFY_ARE_EQUAL(vSpans[0].GetThreadCount(), (DWORD)0);
        VERIFY_ARE_EQUAL(vSpans[0].GetRequestCount(), (DWORD)0);
        VERIFY_IS_TRUE(vSpans[0].GetDisableAffinity() == false);
        VERIFY_IS_TRUE(vSpans[0].GetCompletionRoutines() == false);
        VERIFY_IS_TRUE(vSpans[0].GetMeasureLatency() == false);
        VERIFY_IS_TRUE(vSpans[0].GetRandomWriteData() == false);
        VERIFY_IS_TRUE(vSpans[0].GetUseIoRing() == false);

        const auto& vMasks(vSpans[0].GetAffinityGroupMasks());
        VERIFY_ARE_EQUAL(vMasks.size(), (size_t)0);

        vector<Target> vTargets(vSpans[0].GetTargets());
        VERIFY_ARE_EQUAL(vTargets.size(), (size_t)1);
        Target t(vTargets[0]);

        VERIFY_IS_TRUE(t.GetPath().compare("testfile.dat") == 0);
        VERIFY_ARE_EQUAL(t.GetBlockSizeInBytes(), (DWORD)(128 * 1024));
        VERIFY_ARE_EQUAL(t.GetRequestCount(), (DWORD)2);
        VERIFY_IS_TRUE(t.GetRandomRatio() == 0);
        VERIFY_IS_TRUE(t.GetUseInterlockedSequential() == false);
        VERIFY_ARE_EQUAL(t.GetBlockAlignmentInBytes(), t.GetBlockSizeInBytes());
        VERIFY_ARE_EQUAL(t.GetBaseFileOffsetInBytes(), 0);
        VERIFY_IS_TRUE(t.GetUseParallelAsyncIO() == false);
        VERIFY_IS_TRUE(t.GetCacheMode() == TargetCacheMode::Cached);
        VERIFY_IS_TRUE(t.GetWriteThroughMode() == WriteThroughMode::Off);
        VERIFY_IS_TRUE(t.GetMemoryMappedIoMode() == MemoryMappedIoMode::Off);
        VERIFY_IS_TRUE(t.GetMemoryMappedIoFlushMode() == MemoryMappedIoFlushMode::Undefined);
        VERIFY_IS_TRUE(t.GetBypassIoMode() == BypassIoMode::Undefined);
        VERIFY_IS_TRUE(t.GetZeroWriteBuffers() == false);
        VERIFY_ARE_EQUAL(t.GetThreadsPerFile(), (DWORD)1);
        VERIFY_ARE_EQUAL(t.GetThreadStrideInBytes(), 0);
        VERIFY_IS_TRUE(t.GetCreateFile() == false);
        VERIFY_ARE_EQUAL(t.GetFileSize(), 0);
        VERIFY_ARE_EQUAL(t.GetMaxFileSize(), 0);
        VERIFY_ARE_EQUAL(t.GetWriteRatio(), (DWORD)84);
        VERIFY_IS_TRUE(t.GetUseBurstSize() == false);
        VERIFY_ARE_EQUAL(t.GetBurstSize(), (DWORD)0);
        VERIFY_ARE_EQUAL(t.GetThinkTime(), (DWORD)0);
        VERIFY_IS_TRUE(t.GetEnableThinkTime() == false);
        VERIFY_IS_TRUE(t.GetSequentialScanHint() == false);
        VERIFY_IS_TRUE(t.GetRandomAccessHint() == false);
        VERIFY_IS_TRUE(t.GetTemporaryFileHint() == false);
        VERIFY_IS_TRUE(t.GetUseLargePages() == false);
        VERIFY_ARE_EQUAL(t.GetThroughputInBytesPerMillisecond(), (DWORD)0);
    }

    void CmdLineParserUnitTests::TestParseCmdLineDurationAndProgress()
    {
        CmdLineParser p;
        Profile profile;
        struct Synchronization s = {};
        const char *argv[] = { "foo", "-b128K", "-w84", "-d12", "-P345", "testfile.dat" };
        VERIFY_IS_TRUE(p.ParseCmdLine(_countof(argv), argv, &profile, &s) == true);
        VERIFY_IS_TRUE(profile.GetVerbose() == false);
        VERIFY_IS_TRUE(profile.GetProgress() == 345);
        VERIFY_IS_TRUE(profile.GetCmdLine().compare("foo -b128K -w84 -d12 -P345 testfile.dat") == 0);
        VERIFY_IS_TRUE(profile.GetEtwEnabled() == false);
        VERIFY_IS_TRUE(profile.GetEtwProcess() == false);
        VERIFY_IS_TRUE(profile.GetEtwThread() == false);
        VERIFY_IS_TRUE(profile.GetEtwImageLoad() == false);
        VERIFY_IS_TRUE(profile.GetEtwDiskIO() == false);
        VERIFY_IS_TRUE(profile.GetEtwMemoryPageFaults() == false);
        VERIFY_IS_TRUE(profile.GetEtwMemoryHardFaults() == false);
        VERIFY_IS_TRUE(profile.GetEtwNetwork() == false);
        VERIFY_IS_TRUE(profile.GetEtwRegistry() == false);
        VERIFY_IS_TRUE(profile.GetEtwUsePagedMemory() == false);
        VERIFY_IS_TRUE(profile.GetEtwUsePerfTimer() == false);
        VERIFY_IS_TRUE(profile.GetEtwUseSystemTimer() == false);
        VERIFY_IS_TRUE(profile.GetEtwUseCyclesCounter() == false);

        vector<TimeSpan> vSpans(profile.GetTimeSpans());
        VERIFY_ARE_EQUAL(vSpans.size(), (size_t)1);
        VERIFY_ARE_EQUAL(vSpans[0].GetDuration(), (UINT32)12);
        VERIFY_ARE_EQUAL(vSpans[0].GetWarmup(), (UINT32)5);
        VERIFY_ARE_EQUAL(vSpans[0].GetCooldown(), (UINT32)0);
        VERIFY_ARE_EQUAL(vSpans[0].GetRandSeed(), (UINT32)0);
        VERIFY_ARE_EQUAL(vSpans[0].GetThreadCount(), (DWORD)0);
        VERIFY_ARE_EQUAL(vSpans[0].GetRequestCount(), (DWORD)0);
        VERIFY_IS_TRUE(vSpans[0].GetDisableAffinity() == false);
        VERIFY_IS_TRUE(vSpans[0].GetCompletionRoutines() == false);
        VERIFY_IS_TRUE(vSpans[0].GetMeasureLatency() == false);
        VERIFY_IS_TRUE(vSpans[0].GetRandomWriteData() == false);
        VERIFY_IS_TRUE(vSpans[0].GetUseIoRing() == false);

        const auto& vMasks(vSpans[0].GetAffinityGroupMasks());
        VERIFY_ARE_EQUAL(vMasks.size(), (size_t)0);

        vector<Target> vTargets(vSpans[0].GetTargets());
        VERIFY_ARE_EQUAL(vTargets.size(), (size_t)1);
        Target t(vTargets[0]);

        VERIFY_IS_TRUE(t.GetPath().compare("testfile.dat") == 0);
        VERIFY_ARE_EQUAL(t.GetBlockSizeInBytes(), (DWORD)(128 * 1024));
        VERIFY_ARE_EQUAL(t.GetRequestCount(), (DWORD)2);
        VERIFY_IS_TRUE(t.GetRandomRatio() == 0);
        VERIFY_IS_TRUE(t.GetUseInterlockedSequential() == false);
        VERIFY_ARE_EQUAL(t.GetBlockAlignmentInBytes(), t.GetBlockSizeInBytes());
        VERIFY_ARE_EQUAL(t.GetBaseFileOffsetInBytes(), 0);
        VERIFY_IS_TRUE(t.GetUseParallelAsyncIO() == false);
        VERIFY_IS_TRUE(t.GetCacheMode() == TargetCacheMode::Cached);
        VERIFY_IS_TRUE(t.GetWriteThroughMode() == WriteThroughMode::Off);
        VERIFY_IS_TRUE(t.GetMemoryMappedIoMode() == MemoryMappedIoMode::Off);
        VERIFY_IS_TRUE(t.GetMemoryMappedIoFlushMode() == MemoryMappedIoFlushMode::Undefined);
        VERIFY_IS_TRUE(t.GetBypassIoMode() == BypassIoMode::Undefined);
        VERIFY_IS_TRUE(t.GetZeroWriteBuffers() == false);
        VERIFY_ARE_EQUAL(t.GetThreadsPerFile(), (DWORD)1);
        VERIFY_ARE_EQUAL(t.GetThreadStrideInBytes(), 0);
        VERIFY_IS_TRUE(t.GetCreateFile() == false);
        VERIFY_ARE_EQUAL(t.GetFileSize(), 0);
        VERIFY_ARE_EQUAL(t.GetMaxFileSize(), 0);
        VERIFY_ARE_EQUAL(t.GetWriteRatio(), (DWORD)84);
        VERIFY_IS_TRUE(t.GetUseBurstSize() == false);
        VERIFY_ARE_EQUAL(t.GetBurstSize(), (DWORD)0);
        VERIFY_ARE_EQUAL(t.GetThinkTime(), (DWORD)0);
        VERIFY_IS_TRUE(t.GetEnableThinkTime() == false);
        VERIFY_IS_TRUE(t.GetSequentialScanHint() == false);
        VERIFY_IS_TRUE(t.GetRandomAccessHint() == false);
        VERIFY_IS_TRUE(t.GetTemporaryFileHint() == false);
        VERIFY_IS_TRUE(t.GetUseLargePages() == false);
        VERIFY_ARE_EQUAL(t.GetThroughputInBytesPerMillisecond(), (DWORD)0);
    }

    void CmdLineParserUnitTests::TestParseCmdLineUseParallelAsyncIO()
    {
        CmdLineParser p;
        Profile profile;
        struct Synchronization s = {};
        const char *argv[] = { "foo", "-b128K", "-w84", "-p", "testfile.dat" };
        VERIFY_IS_TRUE(p.ParseCmdLine(_countof(argv), argv, &profile, &s) == true);
        VERIFY_IS_TRUE(profile.GetVerbose() == false);
        VERIFY_IS_TRUE(profile.GetProgress() == 0);
        VERIFY_IS_TRUE(profile.GetCmdLine().compare("foo -b128K -w84 -p testfile.dat") == 0);
        VERIFY_IS_TRUE(profile.GetEtwEnabled() == false);
        VERIFY_IS_TRUE(profile.GetEtwProcess() == false);
        VERIFY_IS_TRUE(profile.GetEtwThread() == false);
        VERIFY_IS_TRUE(profile.GetEtwImageLoad() == false);
        VERIFY_IS_TRUE(profile.GetEtwDiskIO() == false);
        VERIFY_IS_TRUE(profile.GetEtwMemoryPageFaults() == false);
        VERIFY_IS_TRUE(profile.GetEtwMemoryHardFaults() == false);
        VERIFY_IS_TRUE(profile.GetEtwNetwork() == false);
        VERIFY_IS_TRUE(profile.GetEtwRegistry() == false);
        VERIFY_IS_TRUE(profile.GetEtwUsePagedMemory() == false);
        VERIFY_IS_TRUE(profile.GetEtwUsePerfTimer() == false);
        VERIFY_IS_TRUE(profile.GetEtwUseSystemTimer() == false);
        VERIFY_IS_TRUE(profile.GetEtwUseCyclesCounter() == false);

        vector<TimeSpan> vSpans(profile.GetTimeSpans());
        VERIFY_ARE_EQUAL(vSpans.size(), (size_t)1);
        VERIFY_ARE_EQUAL(vSpans[0].GetDuration(), (UINT32)10);
        VERIFY_ARE_EQUAL(vSpans[0].GetWarmup(), (UINT32)5);
        VERIFY_ARE_EQUAL(vSpans[0].GetCooldown(), (UINT32)0);
        VERIFY_ARE_EQUAL(vSpans[0].GetRandSeed(), (UINT32)0);
        VERIFY_ARE_EQUAL(vSpans[0].GetThreadCount(), (DWORD)0);
        VERIFY_ARE_EQUAL(vSpans[0].GetRequestCount(), (DWORD)0);
        VERIFY_IS_TRUE(vSpans[0].GetDisableAffinity() == false);
        VERIFY_IS_TRUE(vSpans[0].GetCompletionRoutines() == false);
        VERIFY_IS_TRUE(vSpans[0].GetMeasureLatency() == false);
        VERIFY_IS_TRUE(vSpans[0].GetRandomWriteData() == false);
        VERIFY_IS_TRUE(vSpans[0].GetUseIoRing() == false);

        const auto& vMasks(vSpans[0].GetAffinityGroupMasks());
        VERIFY_ARE_EQUAL(vMasks.size(), (size_t)0);

        vector<Target> vTargets(vSpans[0].GetTargets());
        VERIFY_ARE_EQUAL(vTargets.size(), (size_t)1);
        Target t(vTargets[0]);

        VERIFY_IS_TRUE(t.GetPath().compare("testfile.dat") == 0);
        VERIFY_ARE_EQUAL(t.GetBlockSizeInBytes(), (DWORD)(128 * 1024));
        VERIFY_ARE_EQUAL(t.GetRequestCount(), (DWORD)2);
        VERIFY_IS_TRUE(t.GetRandomRatio() == 0);
        VERIFY_IS_TRUE(t.GetUseInterlockedSequential() == false);
        VERIFY_ARE_EQUAL(t.GetBlockAlignmentInBytes(), t.GetBlockSizeInBytes());
        VERIFY_ARE_EQUAL(t.GetBaseFileOffsetInBytes(), 0);
        VERIFY_IS_TRUE(t.GetUseParallelAsyncIO() == true);
        VERIFY_IS_TRUE(t.GetCacheMode() == TargetCacheMode::Cached);
        VERIFY_IS_TRUE(t.GetWriteThroughMode() == WriteThroughMode::Off);
        VERIFY_IS_TRUE(t.GetMemoryMappedIoMode() == MemoryMappedIoMode::Off);
        VERIFY_IS_TRUE(t.GetMemoryMappedIoFlushMode() == MemoryMappedIoFlushMode::Undefined);
        VERIFY_IS_TRUE(t.GetBypassIoMode() == BypassIoMode::Undefined);
        VERIFY_IS_TRUE(t.GetZeroWriteBuffers() == false);
        VERIFY_ARE_EQUAL(t.GetThreadsPerFile(), (DWORD)1);
        VERIFY_ARE_EQUAL(t.GetThreadStrideInBytes(), 0);
        VERIFY_IS_TRUE(t.GetCreateFile() == false);
        VERIFY_ARE_EQUAL(t.GetFileSize(), 0);
        VERIFY_ARE_EQUAL(t.GetMaxFileSize(), 0);
        VERIFY_ARE_EQUAL(t.GetWriteRatio(), (DWORD)84);
        VERIFY_IS_TRUE(t.GetUseBurstSize() == false);
        VERIFY_ARE_EQUAL(t.GetBurstSize(), (DWORD)0);
        VERIFY_ARE_EQUAL(t.GetThinkTime(), (DWORD)0);
        VERIFY_IS_TRUE(t.GetEnableThinkTime() == false);
        VERIFY_IS_TRUE(t.GetSequentialScanHint() == false);
        VERIFY_IS_TRUE(t.GetRandomAccessHint() == false);
        VERIFY_IS_TRUE(t.GetTemporaryFileHint() == false);
        VERIFY_IS_TRUE(t.GetUseLargePages() == false);
        VERIFY_ARE_EQUAL(t.GetThroughputInBytesPerMillisecond(), (DWORD)0);
    }

    void CmdLineParserUnitTests::TestParseCmdLineUseLargePages()
    {
        CmdLineParser p;
        Profile profile;
        struct Synchronization s = {};
        const char *argv[] = { "foo", "-b128K", "-w84", "-l", "testfile.dat" };
        VERIFY_IS_TRUE(p.ParseCmdLine(_countof(argv), argv, &profile, &s) == true);
        VERIFY_IS_TRUE(profile.GetVerbose() == false);
        VERIFY_IS_TRUE(profile.GetProgress() == 0);
        VERIFY_IS_TRUE(profile.GetCmdLine().compare("foo -b128K -w84 -l testfile.dat") == 0);
        VERIFY_IS_TRUE(profile.GetEtwEnabled() == false);
        VERIFY_IS_TRUE(profile.GetEtwProcess() == false);
        VERIFY_IS_TRUE(profile.GetEtwThread() == false);
        VERIFY_IS_TRUE(profile.GetEtwImageLoad() == false);
        VERIFY_IS_TRUE(profile.GetEtwDiskIO() == false);
        VERIFY_IS_TRUE(profile.GetEtwMemoryPageFaults() == false);
        VERIFY_IS_TRUE(profile.GetEtwMemoryHardFaults() == false);
        VERIFY_IS_TRUE(profile.GetEtwNetwork() == false);
        VERIFY_IS_TRUE(profile.GetEtwRegistry() == false);
        VERIFY_IS_TRUE(profile.GetEtwUsePagedMemory() == false);
        VERIFY_IS_TRUE(profile.GetEtwUsePerfTimer() == false);
        VERIFY_IS_TRUE(profile.GetEtwUseSystemTimer() == false);
        VERIFY_IS_TRUE(profile.GetEtwUseCyclesCounter() == false);

        vector<TimeSpan> vSpans(profile.GetTimeSpans());
        VERIFY_ARE_EQUAL(vSpans.size(), (size_t)1);
        VERIFY_ARE_EQUAL(vSpans[0].GetDuration(), (UINT32)10);
        VERIFY_ARE_EQUAL(vSpans[0].GetWarmup(), (UINT32)5);
        VERIFY_ARE_EQUAL(vSpans[0].GetCooldown(), (UINT32)0);
        VERIFY_ARE_EQUAL(vSpans[0].GetRandSeed(), (UINT32)0);
        VERIFY_ARE_EQUAL(vSpans[0].GetThreadCount(), (DWORD)0);
        VERIFY_ARE_EQUAL(vSpans[0].GetRequestCount(), (DWORD)0);
        VERIFY_IS_TRUE(vSpans[0].GetDisableAffinity() == false);
        VERIFY_IS_TRUE(vSpans[0].GetCompletionRoutines() == false);
        VERIFY_IS_TRUE(vSpans[0].GetMeasureLatency() == false);
        VERIFY_IS_TRUE(vSpans[0].GetRandomWriteData() == false);
        VERIFY_IS_TRUE(vSpans[0].GetUseIoRing() == false);

        const auto& vMasks(vSpans[0].GetAffinityGroupMasks());
        VERIFY_ARE_EQUAL(vMasks.size(), (size_t)0);

        vector<Target> vTargets(vSpans[0].GetTargets());
        VERIFY_ARE_EQUAL(vTargets.size(), (size_t)1);
        Target t(vTargets[0]);

        VERIFY_IS_TRUE(t.GetPath().compare("testfile.dat") == 0);
        VERIFY_ARE_EQUAL(t.GetBlockSizeInBytes(), (DWORD)(128 * 1024));
        VERIFY_ARE_EQUAL(t.GetRequestCount(), (DWORD)2);
        VERIFY_IS_TRUE(t.GetRandomRatio() == 0);
        VERIFY_IS_TRUE(t.GetUseInterlockedSequential() == false);
        VERIFY_ARE_EQUAL(t.GetBlockAlignmentInBytes(), t.GetBlockSizeInBytes());
        VERIFY_ARE_EQUAL(t.GetBaseFileOffsetInBytes(), 0);
        VERIFY_IS_TRUE(t.GetUseParallelAsyncIO() == false);
        VERIFY_IS_TRUE(t.GetCacheMode() == TargetCacheMode::Cached);
        VERIFY_IS_TRUE(t.GetWriteThroughMode() == WriteThroughMode::Off);
        VERIFY_IS_TRUE(t.GetMemoryMappedIoMode() == MemoryMappedIoMode::Off);
        VERIFY_IS_TRUE(t.GetMemoryMappedIoFlushMode() == MemoryMappedIoFlushMode::Undefined);
        VERIFY_IS_TRUE(t.GetBypassIoMode() == BypassIoMode::Undefined);
        VERIFY_IS_TRUE(t.GetZeroWriteBuffers() == false);
        VERIFY_ARE_EQUAL(t.GetThreadsPerFile(), (DWORD)1);
        VERIFY_ARE_EQUAL(t.GetThreadStrideInBytes(), 0);
        VERIFY_IS_TRUE(t.GetCreateFile() == false);
        VERIFY_ARE_EQUAL(t.GetFileSize(), 0);
        VERIFY_ARE_EQUAL(t.GetMaxFileSize(), 0);
        VERIFY_ARE_EQUAL(t.GetWriteRatio(), (DWORD)84);
        VERIFY_IS_TRUE(t.GetUseBurstSize() == false);
        VERIFY_ARE_EQUAL(t.GetBurstSize(), (DWORD)0);
        VERIFY_ARE_EQUAL(t.GetThinkTime(), (DWORD)0);
        VERIFY_IS_TRUE(t.GetEnableThinkTime() == false);
        VERIFY_IS_TRUE(t.GetSequentialScanHint() == false);
        VERIFY_IS_TRUE(t.GetRandomAccessHint() == false);
        VERIFY_IS_TRUE(t.GetTemporaryFileHint() == false);
        VERIFY_IS_TRUE(t.GetUseLargePages() == true);
        VERIFY_ARE_EQUAL(t.GetThroughputInBytesPerMillisecond(), (DWORD)0);
    }

    void CmdLineParserUnitTests::TestParseCmdLineOverlappedCountAndBaseOffset()
    {
        CmdLineParser p;
        Profile profile;
        struct Synchronization s = {};
        const char *argv[] = { "foo", "-b128K", "-w84", "-o123", "-B512k", "testfile.dat" };
        VERIFY_IS_TRUE(p.ParseCmdLine(_countof(argv), argv, &profile, &s) == true);
        VERIFY_IS_TRUE(profile.GetVerbose() == false);
        VERIFY_IS_TRUE(profile.GetProgress() == 0);
        VERIFY_IS_TRUE(profile.GetCmdLine().compare("foo -b128K -w84 -o123 -B512k testfile.dat") == 0);
        VERIFY_IS_TRUE(profile.GetEtwEnabled() == false);
        VERIFY_IS_TRUE(profile.GetEtwProcess() == false);
        VERIFY_IS_TRUE(profile.GetEtwThread() == false);
        VERIFY_IS_TRUE(profile.GetEtwImageLoad() == false);
        VERIFY_IS_TRUE(profile.GetEtwDiskIO() == false);
        VERIFY_IS_TRUE(profile.GetEtwMemoryPageFaults() == false);
        VERIFY_IS_TRUE(profile.GetEtwMemoryHardFaults() == false);
        VERIFY_IS_TRUE(profile.GetEtwNetwork() == false);
        VERIFY_IS_TRUE(profile.GetEtwRegistry() == false);
        VERIFY_IS_TRUE(profile.GetEtwUsePagedMemory() == false);
        VERIFY_IS_TRUE(profile.GetEtwUsePerfTimer() == false);
        VERIFY_IS_TRUE(profile.GetEtwUseSystemTimer() == false);
        VERIFY_IS_TRUE(profile.GetEtwUseCyclesCounter() == false);

        vector<TimeSpan> vSpans(profile.GetTimeSpans());
        VERIFY_ARE_EQUAL(vSpans.size(), (size_t)1);
        VERIFY_ARE_EQUAL(vSpans[0].GetDuration(), (UINT32)10);
        VERIFY_ARE_EQUAL(vSpans[0].GetWarmup(), (UINT32)5);
        VERIFY_ARE_EQUAL(vSpans[0].GetCooldown(), (UINT32)0);
        VERIFY_ARE_EQUAL(vSpans[0].GetRandSeed(), (UINT32)0);
        VERIFY_ARE_EQUAL(vSpans[0].GetThreadCount(), (DWORD)0);
        VERIFY_ARE_EQUAL(vSpans[0].GetRequestCount(), (DWORD)0);
        VERIFY_IS_TRUE(vSpans[0].GetDisableAffinity() == false);
        VERIFY_IS_TRUE(vSpans[0].GetCompletionRoutines() == false);
        VERIFY_IS_TRUE(vSpans[0].GetMeasureLatency() == false);
        VERIFY_IS_TRUE(vSpans[0].GetRandomWriteData() == false);
        VERIFY_IS_TRUE(vSpans[0].GetUseIoRing() == false);

        const auto& vMasks(vSpans[0].GetAffinityGroupMasks());
        VERIFY_ARE_EQUAL(vMasks.size(), (size_t)0);

        vector<Target> vTargets(vSpans[0].GetTargets());
        VERIFY_ARE_EQUAL(vTargets.size(), (size_t)1);
        Target t(vTargets[0]);

        VERIFY_IS_TRUE(t.GetPath().compare("testfile.dat") == 0);
        VERIFY_ARE_EQUAL(t.GetBlockSizeInBytes(), (DWORD)(128 * 1024));
        VERIFY_ARE_EQUAL(t.GetRequestCount(), (DWORD)123);
        VERIFY_IS_TRUE(t.GetRandomRatio() == 0);
        VERIFY_IS_TRUE(t.GetUseInterlockedSequential() == false);
        VERIFY_ARE_EQUAL(t.GetBlockAlignmentInBytes(), t.GetBlockSizeInBytes());
        VERIFY_ARE_EQUAL(t.GetBaseFileOffsetInBytes(), 512 * 1024);
        VERIFY_IS_TRUE(t.GetUseParallelAsyncIO() == false);
        VERIFY_IS_TRUE(t.GetCacheMode() == TargetCacheMode::Cached);
        VERIFY_IS_TRUE(t.GetWriteThroughMode() == WriteThroughMode::Off);
        VERIFY_IS_TRUE(t.GetMemoryMappedIoMode() == MemoryMappedIoMode::Off);
        VERIFY_IS_TRUE(t.GetMemoryMappedIoFlushMode() == MemoryMappedIoFlushMode::Undefined);
        VERIFY_IS_TRUE(t.GetBypassIoMode() == BypassIoMode::Undefined);
        VERIFY_IS_TRUE(t.GetZeroWriteBuffers() == false);
        VERIFY_ARE_EQUAL(t.GetThreadsPerFile(), (DWORD)1);
        VERIFY_ARE_EQUAL(t.GetThreadStrideInBytes(), 0);
        VERIFY_IS_TRUE(t.GetCreateFile() == false);
        VERIFY_ARE_EQUAL(t.GetFileSize(), 0);
        VERIFY_ARE_EQUAL(t.GetMaxFileSize(), 0);
        VERIFY_ARE_EQUAL(t.GetWriteRatio(), (DWORD)84);
        VERIFY_IS_TRUE(t.GetUseBurstSize() == false);
        VERIFY_ARE_EQUAL(t.GetBurstSize(), (DWORD)0);
        VERIFY_ARE_EQUAL(t.GetThinkTime(), (DWORD)0);
        VERIFY_IS_TRUE(t.GetEnableThinkTime() == false);
        VERIFY_IS_TRUE(t.GetSequentialScanHint() == false);
        VERIFY_IS_TRUE(t.GetRandomAccessHint() == false);
        VERIFY_IS_TRUE(t.GetTemporaryFileHint() == false);
        VERIFY_IS_TRUE(t.GetUseLargePages() == false);
        VERIFY_ARE_EQUAL(t.GetThroughputInBytesPerMillisecond(), (DWORD)0);
    }

    void CmdLineParserUnitTests::TestParseCmdLineCreateFileAndMaxFileSize()
    {
        CmdLineParser p;
        Profile profile;
        struct Synchronization s = {};
        const char *argv[] = { "foo", "-b128K", "-w84", "-c23", "-f4M", "testfile.dat" };
        VERIFY_IS_TRUE(p.ParseCmdLine(_countof(argv), argv, &profile, &s) == true);
        VERIFY_IS_TRUE(profile.GetVerbose() == false);
        VERIFY_IS_TRUE(profile.GetProgress() == 0);
        VERIFY_IS_TRUE(profile.GetCmdLine().compare("foo -b128K -w84 -c23 -f4M testfile.dat") == 0);
        VERIFY_IS_TRUE(profile.GetEtwEnabled() == false);
        VERIFY_IS_TRUE(profile.GetEtwProcess() == false);
        VERIFY_IS_TRUE(profile.GetEtwThread() == false);
        VERIFY_IS_TRUE(profile.GetEtwImageLoad() == false);
        VERIFY_IS_TRUE(profile.GetEtwDiskIO() == false);
        VERIFY_IS_TRUE(profile.GetEtwMemoryPageFaults() == false);
        VERIFY_IS_TRUE(profile.GetEtwMemoryHardFaults() == false);
        VERIFY_IS_TRUE(profile.GetEtwNetwork() == false);
        VERIFY_IS_TRUE(profile.GetEtwRegistry() == false);
        VERIFY_IS_TRUE(profile.GetEtwUsePagedMemory() == false);
        VERIFY_IS_TRUE(profile.GetEtwUsePerfTimer() == false);
        VERIFY_IS_TRUE(profile.GetEtwUseSystemTimer() == false);
        VERIFY_IS_TRUE(profile.GetEtwUseCyclesCounter() == false);

        vector<TimeSpan> vSpans(profile.GetTimeSpans());
        VERIFY_ARE_EQUAL(vSpans.size(), (size_t)1);
        VERIFY_ARE_EQUAL(vSpans[0].GetDuration(), (UINT32)10);
        VERIFY_ARE_EQUAL(vSpans[0].GetWarmup(), (UINT32)5);
        VERIFY_ARE_EQUAL(vSpans[0].GetCooldown(), (UINT32)0);
        VERIFY_ARE_EQUAL(vSpans[0].GetRandSeed(), (UINT32)0);
        VERIFY_ARE_EQUAL(vSpans[0].GetThreadCount(), (DWORD)0);
        VERIFY_ARE_EQUAL(vSpans[0].GetRequestCount(), (DWORD)0);
        VERIFY_IS_TRUE(vSpans[0].GetDisableAffinity() == false);
        VERIFY_IS_TRUE(vSpans[0].GetCompletionRoutines() == false);
        VERIFY_IS_TRUE(vSpans[0].GetMeasureLatency() == false);
        VERIFY_IS_TRUE(vSpans[0].GetRandomWriteData() == false);
        VERIFY_IS_TRUE(vSpans[0].GetUseIoRing() == false);

        const auto& vMasks(vSpans[0].GetAffinityGroupMasks());
        VERIFY_ARE_EQUAL(vMasks.size(), (size_t)0);

        vector<Target> vTargets(vSpans[0].GetTargets());
        VERIFY_ARE_EQUAL(vTargets.size(), (size_t)1);
        Target t(vTargets[0]);

        VERIFY_IS_TRUE(t.GetPath().compare("testfile.dat") == 0);
        VERIFY_ARE_EQUAL(t.GetBlockSizeInBytes(), (DWORD)(128 * 1024));
        VERIFY_ARE_EQUAL(t.GetRequestCount(), (DWORD)2);
        VERIFY_IS_TRUE(t.GetRandomRatio() == 0);
        VERIFY_IS_TRUE(t.GetUseInterlockedSequential() == false);
        VERIFY_ARE_EQUAL(t.GetBlockAlignmentInBytes(), t.GetBlockSizeInBytes());
        VERIFY_ARE_EQUAL(t.GetBaseFileOffsetInBytes(), 0);
        VERIFY_IS_TRUE(t.GetUseParallelAsyncIO() == false);
        VERIFY_IS_TRUE(t.GetCacheMode() == TargetCacheMode::Cached);
        VERIFY_IS_TRUE(t.GetWriteThroughMode() == WriteThroughMode::Off);
        VERIFY_IS_TRUE(t.GetMemoryMappedIoMode() == MemoryMappedIoMode::Off);
        VERIFY_IS_TRUE(t.GetMemoryMappedIoFlushMode() == MemoryMappedIoFlushMode::Undefined);
        VERIFY_IS_TRUE(t.GetBypassIoMode() == BypassIoMode::Undefined);
        VERIFY_IS_TRUE(t.GetZeroWriteBuffers() == false);
        VERIFY_ARE_EQUAL(t.GetThreadsPerFile(), (DWORD)1);
        VERIFY_ARE_EQUAL(t.GetThreadStrideInBytes(), 0);
        VERIFY_IS_TRUE(t.GetCreateFile() == true);
        VERIFY_ARE_EQUAL(t.GetFileSize(), 23);
        VERIFY_ARE_EQUAL(t.GetMaxFileSize(), (4 * 1024 * 1024));
        VERIFY_ARE_EQUAL(t.GetWriteRatio(), (DWORD)84);
        VERIFY_IS_TRUE(t.GetUseBurstSize() == false);
        VERIFY_ARE_EQUAL(t.GetBurstSize(), (DWORD)0);
        VERIFY_ARE_EQUAL(t.GetThinkTime(), (DWORD)0);
        VERIFY_IS_TRUE(t.GetEnableThinkTime() == false);
        VERIFY_IS_TRUE(t.GetSequentialScanHint() == false);
        VERIFY_IS_TRUE(t.GetRandomAccessHint() == false);
        VERIFY_IS_TRUE(t.GetTemporaryFileHint() == false);
        VERIFY_IS_TRUE(t.GetUseLargePages() == false);
        VERIFY_ARE_EQUAL(t.GetThroughputInBytesPerMillisecond(), (DWORD)0);
    }

    void CmdLineParserUnitTests::TestParseCmdLineBurstSizeAndThinkTime()
    {
        CmdLineParser p;
        Profile profile;
        struct Synchronization s = {};
        const char *argv[] = { "foo", "-b128K", "-w84", "-i23", "-j567", "testfile.dat" };
        VERIFY_IS_TRUE(p.ParseCmdLine(_countof(argv), argv, &profile, &s) == true);
        VERIFY_IS_TRUE(profile.GetVerbose() == false);
        VERIFY_IS_TRUE(profile.GetProgress() == 0);
        VERIFY_IS_TRUE(profile.GetCmdLine().compare("foo -b128K -w84 -i23 -j567 testfile.dat") == 0);
        VERIFY_IS_TRUE(profile.GetEtwEnabled() == false);
        VERIFY_IS_TRUE(profile.GetEtwProcess() == false);
        VERIFY_IS_TRUE(profile.GetEtwThread() == false);
        VERIFY_IS_TRUE(profile.GetEtwImageLoad() == false);
        VERIFY_IS_TRUE(profile.GetEtwDiskIO() == false);
        VERIFY_IS_TRUE(profile.GetEtwMemoryPageFaults() == false);
        VERIFY_IS_TRUE(profile.GetEtwMemoryHardFaults() == false);
        VERIFY_IS_TRUE(profile.GetEtwNetwork() == false);
        VERIFY_IS_TRUE(profile.GetEtwRegistry() == false);
        VERIFY_IS_TRUE(profile.GetEtwUsePagedMemory() == false);
        VERIFY_IS_TRUE(profile.GetEtwUsePerfTimer() == false);
        VERIFY_IS_TRUE(profile.GetEtwUseSystemTimer() == false);
        VERIFY_IS_TRUE(profile.GetEtwUseCyclesCounter() == false);

        vector<TimeSpan> vSpans(profile.GetTimeSpans());
        VERIFY_ARE_EQUAL(vSpans.size(), (size_t)1);
        VERIFY_ARE_EQUAL(vSpans[0].GetDuration(), (UINT32)10);
        VERIFY_ARE_EQUAL(vSpans[0].GetWarmup(), (UINT32)5);
        VERIFY_ARE_EQUAL(vSpans[0].GetCooldown(), (UINT32)0);
        VERIFY_ARE_EQUAL(vSpans[0].GetRandSeed(), (UINT32)0);
        VERIFY_ARE_EQUAL(vSpans[0].GetThreadCount(), (DWORD)0);
        VERIFY_ARE_EQUAL(vSpans[0].GetRequestCount(), (DWORD)0);
        VERIFY_IS_TRUE(vSpans[0].GetDisableAffinity() == false);
        VERIFY_IS_TRUE(vSpans[0].GetCompletionRoutines() == false);
        VERIFY_IS_TRUE(vSpans[0].GetMeasureLatency() == false);
        VERIFY_IS_TRUE(vSpans[0].GetRandomWriteData() == false);
        VERIFY_IS_TRUE(vSpans[0].GetUseIoRing() == false);

        const auto& vMasks(vSpans[0].GetAffinityGroupMasks());
        VERIFY_ARE_EQUAL(vMasks.size(), (size_t)0);

        vector<Target> vTargets(vSpans[0].GetTargets());
        VERIFY_ARE_EQUAL(vTargets.size(), (size_t)1);
        Target t(vTargets[0]);

        VERIFY_IS_TRUE(t.GetPath().compare("testfile.dat") == 0);
        VERIFY_ARE_EQUAL(t.GetBlockSizeInBytes(), (DWORD)(128 * 1024));
        VERIFY_ARE_EQUAL(t.GetRequestCount(), (DWORD)2);
        VERIFY_IS_TRUE(t.GetRandomRatio() == 0);
        VERIFY_IS_TRUE(t.GetUseInterlockedSequential() == false);
        VERIFY_ARE_EQUAL(t.GetBlockAlignmentInBytes(), t.GetBlockSizeInBytes());
        VERIFY_ARE_EQUAL(t.GetBaseFileOffsetInBytes(), 0);
        VERIFY_IS_TRUE(t.GetUseParallelAsyncIO() == false);
        VERIFY_IS_TRUE(t.GetCacheMode() == TargetCacheMode::Cached);
        VERIFY_IS_TRUE(t.GetWriteThroughMode() == WriteThroughMode::Off);
        VERIFY_IS_TRUE(t.GetMemoryMappedIoMode() == MemoryMappedIoMode::Off);
        VERIFY_IS_TRUE(t.GetMemoryMappedIoFlushMode() == MemoryMappedIoFlushMode::Undefined);
        VERIFY_IS_TRUE(t.GetBypassIoMode() == BypassIoMode::Undefined);
        VERIFY_IS_TRUE(t.GetZeroWriteBuffers() == false);
        VERIFY_ARE_EQUAL(t.GetThreadsPerFile(), (DWORD)1);
        VERIFY_ARE_EQUAL(t.GetThreadStrideInBytes(), 0);
        VERIFY_IS_TRUE(t.GetCreateFile() == false);
        VERIFY_ARE_EQUAL(t.GetFileSize(), 0);
        VERIFY_ARE_EQUAL(t.GetMaxFileSize(), 0);
        VERIFY_ARE_EQUAL(t.GetWriteRatio(), (DWORD)84);
        VERIFY_IS_TRUE(t.GetUseBurstSize() == true);
        VERIFY_ARE_EQUAL(t.GetBurstSize(), (DWORD)23);
        VERIFY_ARE_EQUAL(t.GetThinkTime(), (DWORD)567);
        VERIFY_IS_TRUE(t.GetEnableThinkTime() == true);
        VERIFY_IS_TRUE(t.GetSequentialScanHint() == false);
        VERIFY_IS_TRUE(t.GetRandomAccessHint() == false);
        VERIFY_IS_TRUE(t.GetTemporaryFileHint() == false);
        VERIFY_IS_TRUE(t.GetUseLargePages() == false);
        VERIFY_ARE_EQUAL(t.GetThroughputInBytesPerMillisecond(), (DWORD)0);
    }

    void CmdLineParserUnitTests::TestParseCmdLineTotalThreadCountAndThroughput()
    {
        CmdLineParser p;
        Profile profile;
        struct Synchronization s = {};
        const char *argv[] = { "foo", "-b128K", "-w84", "-F23", "-g567", "testfile.dat" };
        VERIFY_IS_TRUE(p.ParseCmdLine(_countof(argv), argv, &profile, &s) == true);
        VERIFY_IS_TRUE(profile.GetVerbose() == false);
        VERIFY_IS_TRUE(profile.GetProgress() == 0);
        VERIFY_IS_TRUE(profile.GetCmdLine().compare("foo -b128K -w84 -F23 -g567 testfile.dat") == 0);
        VERIFY_IS_TRUE(profile.GetEtwEnabled() == false);
        VERIFY_IS_TRUE(profile.GetEtwProcess() == false);
        VERIFY_IS_TRUE(profile.GetEtwThread() == false);
        VERIFY_IS_TRUE(profile.GetEtwImageLoad() == false);
        VERIFY_IS_TRUE(profile.GetEtwDiskIO() == false);
        VERIFY_IS_TRUE(profile.GetEtwMemoryPageFaults() == false);
        VERIFY_IS_TRUE(profile.GetEtwMemoryHardFaults() == false);
        VERIFY_IS_TRUE(profile.GetEtwNetwork() == false);
        VERIFY_IS_TRUE(profile.GetEtwRegistry() == false);
        VERIFY_IS_TRUE(profile.GetEtwUsePagedMemory() == false);
        VERIFY_IS_TRUE(profile.GetEtwUsePerfTimer() == false);
        VERIFY_IS_TRUE(profile.GetEtwUseSystemTimer() == false);
        VERIFY_IS_TRUE(profile.GetEtwUseCyclesCounter() == false);

        vector<TimeSpan> vSpans(profile.GetTimeSpans());
        VERIFY_ARE_EQUAL(vSpans.size(), (size_t)1);
        VERIFY_ARE_EQUAL(vSpans[0].GetDuration(), (UINT32)10);
        VERIFY_ARE_EQUAL(vSpans[0].GetWarmup(), (UINT32)5);
        VERIFY_ARE_EQUAL(vSpans[0].GetCooldown(), (UINT32)0);
        VERIFY_ARE_EQUAL(vSpans[0].GetRandSeed(), (UINT32)0);
        VERIFY_ARE_EQUAL(vSpans[0].GetThreadCount(), (DWORD)23);
        VERIFY_ARE_EQUAL(vSpans[0].GetRequestCount(), (DWORD)0);
        VERIFY_IS_TRUE(vSpans[0].GetDisableAffinity() == false);
        VERIFY_IS_TRUE(vSpans[0].GetCompletionRoutines() == false);
        VERIFY_IS_TRUE(vSpans[0].GetMeasureLatency() == false);
        VERIFY_IS_TRUE(vSpans[0].GetRandomWriteData() == false);
        VERIFY_IS_TRUE(vSpans[0].GetUseIoRing() == false);

        const auto& vMasks(vSpans[0].GetAffinityGroupMasks());
        VERIFY_ARE_EQUAL(vMasks.size(), (size_t)0);

        vector<Target> vTargets(vSpans[0].GetTargets());
        VERIFY_ARE_EQUAL(vTargets.size(), (size_t)1);
        Target t(vTargets[0]);

        VERIFY_IS_TRUE(t.GetPath().compare("testfile.dat") == 0);
        VERIFY_ARE_EQUAL(t.GetBlockSizeInBytes(), (DWORD)(128 * 1024));
        VERIFY_ARE_EQUAL(t.GetRequestCount(), (DWORD)2);
        VERIFY_IS_TRUE(t.GetRandomRatio() == 0);
        VERIFY_IS_TRUE(t.GetUseInterlockedSequential() == false);
        VERIFY_ARE_EQUAL(t.GetBlockAlignmentInBytes(), t.GetBlockSizeInBytes());
        VERIFY_ARE_EQUAL(t.GetBaseFileOffsetInBytes(), 0);
        VERIFY_IS_TRUE(t.GetUseParallelAsyncIO() == false);
        VERIFY_IS_TRUE(t.GetCacheMode() == TargetCacheMode::Cached);
        VERIFY_IS_TRUE(t.GetWriteThroughMode() == WriteThroughMode::Off);
        VERIFY_IS_TRUE(t.GetMemoryMappedIoMode() == MemoryMappedIoMode::Off);
        VERIFY_IS_TRUE(t.GetMemoryMappedIoFlushMode() == MemoryMappedIoFlushMode::Undefined);
        VERIFY_IS_TRUE(t.GetBypassIoMode() == BypassIoMode::Undefined);
        VERIFY_IS_TRUE(t.GetZeroWriteBuffers() == false);
        VERIFY_ARE_EQUAL(t.GetThreadsPerFile(), (DWORD)1);
        VERIFY_ARE_EQUAL(t.GetThreadStrideInBytes(), 0);
        VERIFY_IS_TRUE(t.GetCreateFile() == false);
        VERIFY_ARE_EQUAL(t.GetFileSize(), 0);
        VERIFY_ARE_EQUAL(t.GetMaxFileSize(), 0);
        VERIFY_ARE_EQUAL(t.GetWriteRatio(), (DWORD)84);
        VERIFY_IS_TRUE(t.GetUseBurstSize() == false);
        VERIFY_ARE_EQUAL(t.GetBurstSize(), (DWORD)0);
        VERIFY_ARE_EQUAL(t.GetThinkTime(), (DWORD)0);
        VERIFY_IS_TRUE(t.GetEnableThinkTime() == false);
        VERIFY_IS_TRUE(t.GetSequentialScanHint() == false);
        VERIFY_IS_TRUE(t.GetRandomAccessHint() == false);
        VERIFY_IS_TRUE(t.GetTemporaryFileHint() == false);
        VERIFY_IS_TRUE(t.GetUseLargePages() == false);
        VERIFY_ARE_EQUAL(t.GetThroughputInBytesPerMillisecond(), (DWORD)567);
    }

    void CmdLineParserUnitTests::TestParseCmdLineRandomIOAlignment()
    {
        CmdLineParser p;
        Profile profile;
        struct Synchronization s = {};
        const char *argv[] = { "foo", "-b128K", "-w84", "-r23M", "testfile.dat" };
        VERIFY_IS_TRUE(p.ParseCmdLine(_countof(argv), argv, &profile, &s) == true);
        VERIFY_IS_TRUE(profile.GetVerbose() == false);
        VERIFY_IS_TRUE(profile.GetProgress() == 0);
        VERIFY_IS_TRUE(profile.GetCmdLine().compare("foo -b128K -w84 -r23M testfile.dat") == 0);
        VERIFY_IS_TRUE(profile.GetEtwEnabled() == false);
        VERIFY_IS_TRUE(profile.GetEtwProcess() == false);
        VERIFY_IS_TRUE(profile.GetEtwThread() == false);
        VERIFY_IS_TRUE(profile.GetEtwImageLoad() == false);
        VERIFY_IS_TRUE(profile.GetEtwDiskIO() == false);
        VERIFY_IS_TRUE(profile.GetEtwMemoryPageFaults() == false);
        VERIFY_IS_TRUE(profile.GetEtwMemoryHardFaults() == false);
        VERIFY_IS_TRUE(profile.GetEtwNetwork() == false);
        VERIFY_IS_TRUE(profile.GetEtwRegistry() == false);
        VERIFY_IS_TRUE(profile.GetEtwUsePagedMemory() == false);
        VERIFY_IS_TRUE(profile.GetEtwUsePerfTimer() == false);
        VERIFY_IS_TRUE(profile.GetEtwUseSystemTimer() == false);
        VERIFY_IS_TRUE(profile.GetEtwUseCyclesCounter() == false);

        vector<TimeSpan> vSpans(profile.GetTimeSpans());
        VERIFY_ARE_EQUAL(vSpans.size(), (size_t)1);
        VERIFY_ARE_EQUAL(vSpans[0].GetDuration(), (UINT32)10);
        VERIFY_ARE_EQUAL(vSpans[0].GetWarmup(), (UINT32)5);
        VERIFY_ARE_EQUAL(vSpans[0].GetCooldown(), (UINT32)0);
        VERIFY_ARE_EQUAL(vSpans[0].GetRandSeed(), (UINT32)0);
        VERIFY_ARE_EQUAL(vSpans[0].GetThreadCount(), (DWORD)0);
        VERIFY_ARE_EQUAL(vSpans[0].GetRequestCount(), (DWORD)0);
        VERIFY_IS_TRUE(vSpans[0].GetDisableAffinity() == false);
        VERIFY_IS_TRUE(vSpans[0].GetCompletionRoutines() == false);
        VERIFY_IS_TRUE(vSpans[0].GetMeasureLatency() == false);
        VERIFY_IS_TRUE(vSpans[0].GetRandomWriteData() == false);
        VERIFY_IS_TRUE(vSpans[0].GetUseIoRing() == false);

        const auto& vMasks(vSpans[0].GetAffinityGroupMasks());
        VERIFY_ARE_EQUAL(vMasks.size(), (size_t)0);

        vector<Target> vTargets(vSpans[0].GetTargets());
        VERIFY_ARE_EQUAL(vTargets.size(), (size_t)1);
        Target t(vTargets[0]);

        VERIFY_IS_TRUE(t.GetPath().compare("testfile.dat") == 0);
        VERIFY_ARE_EQUAL(t.GetBlockSizeInBytes(), (DWORD)(128 * 1024));
        VERIFY_ARE_EQUAL(t.GetRequestCount(), (DWORD)2);
        VERIFY_IS_TRUE(t.GetRandomRatio() == 100);
        VERIFY_IS_TRUE(t.GetUseInterlockedSequential() == false);
        VERIFY_ARE_EQUAL(t.GetBlockAlignmentInBytes(), (23 * 1024 * 1024));
        VERIFY_ARE_EQUAL(t.GetBaseFileOffsetInBytes(), 0);
        VERIFY_IS_TRUE(t.GetUseParallelAsyncIO() == false);
        VERIFY_IS_TRUE(t.GetCacheMode() == TargetCacheMode::Cached);
        VERIFY_IS_TRUE(t.GetWriteThroughMode() == WriteThroughMode::Off);
        VERIFY_IS_TRUE(t.GetMemoryMappedIoMode() == MemoryMappedIoMode::Off);
        VERIFY_IS_TRUE(t.GetMemoryMappedIoFlushMode() == MemoryMappedIoFlushMode::Undefined);
        VERIFY_IS_TRUE(t.GetBypassIoMode() == BypassIoMode::Undefined);
        VERIFY_IS_TRUE(t.GetZeroWriteBuffers() == false);
        VERIFY_ARE_EQUAL(t.GetThreadsPerFile(), (DWORD)1);
        VERIFY_ARE_EQUAL(t.GetThreadStrideInBytes(), 0);
        VERIFY_IS_TRUE(t.GetCreateFile() == false);
        VERIFY_ARE_EQUAL(t.GetFileSize(), 0);
        VERIFY_ARE_EQUAL(t.GetMaxFileSize(), 0);
        VERIFY_ARE_EQUAL(t.GetWriteRatio(), (DWORD)84);
        VERIFY_IS_TRUE(t.GetUseBurstSize() == false);
        VERIFY_ARE_EQUAL(t.GetBurstSize(), (DWORD)0);
        VERIFY_ARE_EQUAL(t.GetThinkTime(), (DWORD)0);
        VERIFY_IS_TRUE(t.GetEnableThinkTime() == false);
        VERIFY_IS_TRUE(t.GetSequentialScanHint() == false);
        VERIFY_IS_TRUE(t.GetRandomAccessHint() == false);
        VERIFY_IS_TRUE(t.GetTemporaryFileHint() == false);
        VERIFY_IS_TRUE(t.GetUseLargePages() == false);
        VERIFY_ARE_EQUAL(t.GetThroughputInBytesPerMillisecond(), (DWORD)0);
    }

    void CmdLineParserUnitTests::TestParseCmdLineStrideSize()
    {
        CmdLineParser p;
        Profile profile;
        struct Synchronization s = {};
        const char *argv[] = { "foo", "-b128K", "-w84", "-s567K", "testfile.dat" };
        VERIFY_IS_TRUE(p.ParseCmdLine(_countof(argv), argv, &profile, &s) == true);
        VERIFY_IS_TRUE(profile.GetVerbose() == false);
        VERIFY_IS_TRUE(profile.GetProgress() == 0);
        VERIFY_IS_TRUE(profile.GetCmdLine().compare("foo -b128K -w84 -s567K testfile.dat") == 0);
        VERIFY_IS_TRUE(profile.GetEtwEnabled() == false);
        VERIFY_IS_TRUE(profile.GetEtwProcess() == false);
        VERIFY_IS_TRUE(profile.GetEtwThread() == false);
        VERIFY_IS_TRUE(profile.GetEtwImageLoad() == false);
        VERIFY_IS_TRUE(profile.GetEtwDiskIO() == false);
        VERIFY_IS_TRUE(profile.GetEtwMemoryPageFaults() == false);
        VERIFY_IS_TRUE(profile.GetEtwMemoryHardFaults() == false);
        VERIFY_IS_TRUE(profile.GetEtwNetwork() == false);
        VERIFY_IS_TRUE(profile.GetEtwRegistry() == false);
        VERIFY_IS_TRUE(profile.GetEtwUsePagedMemory() == false);
        VERIFY_IS_TRUE(profile.GetEtwUsePerfTimer() == false);
        VERIFY_IS_TRUE(profile.GetEtwUseSystemTimer() == false);
        VERIFY_IS_TRUE(profile.GetEtwUseCyclesCounter() == false);

        vector<TimeSpan> vSpans(profile.GetTimeSpans());
        VERIFY_ARE_EQUAL(vSpans.size(), (size_t)1);
        VERIFY_ARE_EQUAL(vSpans[0].GetDuration(), (UINT32)10);
        VERIFY_ARE_EQUAL(vSpans[0].GetWarmup(), (UINT32)5);
        VERIFY_ARE_EQUAL(vSpans[0].GetCooldown(), (UINT32)0);
        VERIFY_ARE_EQUAL(vSpans[0].GetRandSeed(), (UINT32)0);
        VERIFY_ARE_EQUAL(vSpans[0].GetThreadCount(), (DWORD)0);
        VERIFY_ARE_EQUAL(vSpans[0].GetRequestCount(), (DWORD)0);
        VERIFY_IS_TRUE(vSpans[0].GetDisableAffinity() == false);
        VERIFY_IS_TRUE(vSpans[0].GetCompletionRoutines() == false);
        VERIFY_IS_TRUE(vSpans[0].GetMeasureLatency() == false);
        VERIFY_IS_TRUE(vSpans[0].GetRandomWriteData() == false);
        VERIFY_IS_TRUE(vSpans[0].GetUseIoRing() == false);

        const auto& vMasks(vSpans[0].GetAffinityGroupMasks());
        VERIFY_ARE_EQUAL(vMasks.size(), (size_t)0);

        vector<Target> vTargets(vSpans[0].GetTargets());
        VERIFY_ARE_EQUAL(vTargets.size(), (size_t)1);
        Target t(vTargets[0]);

        VERIFY_IS_TRUE(t.GetPath().compare("testfile.dat") == 0);
        VERIFY_ARE_EQUAL(t.GetBlockSizeInBytes(), (DWORD)(128 * 1024));
        VERIFY_ARE_EQUAL(t.GetRequestCount(), (DWORD)2);
        VERIFY_IS_TRUE(t.GetRandomRatio() == 0);
        VERIFY_IS_TRUE(t.GetUseInterlockedSequential() == false);
        VERIFY_ARE_EQUAL(t.GetBlockAlignmentInBytes(), (567 * 1024));
        VERIFY_ARE_EQUAL(t.GetBaseFileOffsetInBytes(), 0);
        VERIFY_IS_TRUE(t.GetUseParallelAsyncIO() == false);
        VERIFY_IS_TRUE(t.GetCacheMode() == TargetCacheMode::Cached);
        VERIFY_IS_TRUE(t.GetWriteThroughMode() == WriteThroughMode::Off);
        VERIFY_IS_TRUE(t.GetMemoryMappedIoMode() == MemoryMappedIoMode::Off);
        VERIFY_IS_TRUE(t.GetMemoryMappedIoFlushMode() == MemoryMappedIoFlushMode::Undefined);
        VERIFY_IS_TRUE(t.GetBypassIoMode() == BypassIoMode::Undefined);
        VERIFY_IS_TRUE(t.GetZeroWriteBuffers() == false);
        VERIFY_ARE_EQUAL(t.GetThreadsPerFile(), (DWORD)1);
        VERIFY_ARE_EQUAL(t.GetThreadStrideInBytes(), 0);
        VERIFY_IS_TRUE(t.GetCreateFile() == false);
        VERIFY_ARE_EQUAL(t.GetFileSize(), 0);
        VERIFY_ARE_EQUAL(t.GetMaxFileSize(), 0);
        VERIFY_ARE_EQUAL(t.GetWriteRatio(), (DWORD)84);
        VERIFY_IS_TRUE(t.GetUseBurstSize() == false);
        VERIFY_ARE_EQUAL(t.GetBurstSize(), (DWORD)0);
        VERIFY_ARE_EQUAL(t.GetThinkTime(), (DWORD)0);
        VERIFY_IS_TRUE(t.GetEnableThinkTime() == false);
        VERIFY_IS_TRUE(t.GetSequentialScanHint() == false);
        VERIFY_IS_TRUE(t.GetRandomAccessHint() == false);
        VERIFY_IS_TRUE(t.GetTemporaryFileHint() == false);
        VERIFY_IS_TRUE(t.GetUseLargePages() == false);
        VERIFY_ARE_EQUAL(t.GetThroughputInBytesPerMillisecond(), (DWORD)0);
    }

    void CmdLineParserUnitTests::TestParseCmdLineThreadsPerFileAndThreadStride()
    {
        CmdLineParser p;
        Profile profile;
        struct Synchronization s = {};
        const char *argv[] = { "foo", "-b128K", "-w84", "-t23", "-T512K", "testfile.dat" };
        VERIFY_IS_TRUE(p.ParseCmdLine(_countof(argv), argv, &profile, &s) == true);
        VERIFY_IS_TRUE(profile.GetVerbose() == false);
        VERIFY_IS_TRUE(profile.GetProgress() == 0);
        VERIFY_IS_TRUE(profile.GetCmdLine().compare("foo -b128K -w84 -t23 -T512K testfile.dat") == 0);
        VERIFY_IS_TRUE(profile.GetEtwEnabled() == false);
        VERIFY_IS_TRUE(profile.GetEtwProcess() == false);
        VERIFY_IS_TRUE(profile.GetEtwThread() == false);
        VERIFY_IS_TRUE(profile.GetEtwImageLoad() == false);
        VERIFY_IS_TRUE(profile.GetEtwDiskIO() == false);
        VERIFY_IS_TRUE(profile.GetEtwMemoryPageFaults() == false);
        VERIFY_IS_TRUE(profile.GetEtwMemoryHardFaults() == false);
        VERIFY_IS_TRUE(profile.GetEtwNetwork() == false);
        VERIFY_IS_TRUE(profile.GetEtwRegistry() == false);
        VERIFY_IS_TRUE(profile.GetEtwUsePagedMemory() == false);
        VERIFY_IS_TRUE(profile.GetEtwUsePerfTimer() == false);
        VERIFY_IS_TRUE(profile.GetEtwUseSystemTimer() == false);
        VERIFY_IS_TRUE(profile.GetEtwUseCyclesCounter() == false);

        vector<TimeSpan> vSpans(profile.GetTimeSpans());
        VERIFY_ARE_EQUAL(vSpans.size(), (size_t)1);
        VERIFY_ARE_EQUAL(vSpans[0].GetDuration(), (UINT32)10);
        VERIFY_ARE_EQUAL(vSpans[0].GetWarmup(), (UINT32)5);
        VERIFY_ARE_EQUAL(vSpans[0].GetCooldown(), (UINT32)0);
        VERIFY_ARE_EQUAL(vSpans[0].GetRandSeed(), (UINT32)0);
        VERIFY_ARE_EQUAL(vSpans[0].GetThreadCount(), (DWORD)0);
        VERIFY_ARE_EQUAL(vSpans[0].GetRequestCount(), (DWORD)0);
        VERIFY_IS_TRUE(vSpans[0].GetDisableAffinity() == false);
        VERIFY_IS_TRUE(vSpans[0].GetCompletionRoutines() == false);
        VERIFY_IS_TRUE(vSpans[0].GetMeasureLatency() == false);
        VERIFY_IS_TRUE(vSpans[0].GetRandomWriteData() == false);
        VERIFY_IS_TRUE(vSpans[0].GetUseIoRing() == false);

        const auto& vMasks(vSpans[0].GetAffinityGroupMasks());
        VERIFY_ARE_EQUAL(vMasks.size(), (size_t)0);

        vector<Target> vTargets(vSpans[0].GetTargets());
        VERIFY_ARE_EQUAL(vTargets.size(), (size_t)1);
        Target t(vTargets[0]);

        VERIFY_IS_TRUE(t.GetPath().compare("testfile.dat") == 0);
        VERIFY_ARE_EQUAL(t.GetBlockSizeInBytes(), (DWORD)(128 * 1024));
        VERIFY_ARE_EQUAL(t.GetRequestCount(), (DWORD)2);
        VERIFY_IS_TRUE(t.GetRandomRatio() == 0);
        VERIFY_IS_TRUE(t.GetUseInterlockedSequential() == false);
        VERIFY_ARE_EQUAL(t.GetBlockAlignmentInBytes(), t.GetBlockSizeInBytes());
        VERIFY_ARE_EQUAL(t.GetBaseFileOffsetInBytes(), 0);
        VERIFY_IS_TRUE(t.GetUseParallelAsyncIO() == false);
        VERIFY_IS_TRUE(t.GetCacheMode() == TargetCacheMode::Cached);
        VERIFY_IS_TRUE(t.GetWriteThroughMode() == WriteThroughMode::Off);
        VERIFY_IS_TRUE(t.GetMemoryMappedIoMode() == MemoryMappedIoMode::Off);
        VERIFY_IS_TRUE(t.GetMemoryMappedIoFlushMode() == MemoryMappedIoFlushMode::Undefined);
        VERIFY_IS_TRUE(t.GetBypassIoMode() == BypassIoMode::Undefined);
        VERIFY_IS_TRUE(t.GetZeroWriteBuffers() == false);
        VERIFY_ARE_EQUAL(t.GetThreadsPerFile(), (DWORD)23);
        VERIFY_ARE_EQUAL(t.GetThreadStrideInBytes(), (512 * 1024));
        VERIFY_IS_TRUE(t.GetCreateFile() == false);
        VERIFY_ARE_EQUAL(t.GetFileSize(), 0);
        VERIFY_ARE_EQUAL(t.GetMaxFileSize(), 0);
        VERIFY_ARE_EQUAL(t.GetWriteRatio(), (DWORD)84);
        VERIFY_IS_TRUE(t.GetUseBurstSize() == false);
        VERIFY_ARE_EQUAL(t.GetBurstSize(), (DWORD)0);
        VERIFY_ARE_EQUAL(t.GetThinkTime(), (DWORD)0);
        VERIFY_IS_TRUE(t.GetEnableThinkTime() == false);
        VERIFY_IS_TRUE(t.GetSequentialScanHint() == false);
        VERIFY_IS_TRUE(t.GetRandomAccessHint() == false);
        VERIFY_IS_TRUE(t.GetTemporaryFileHint() == false);
        VERIFY_IS_TRUE(t.GetUseLargePages() == false);
        VERIFY_ARE_EQUAL(t.GetThroughputInBytesPerMillisecond(), (DWORD)0);
    }

    void CmdLineParserUnitTests::TestParseCmdLineEtwUsePagedMemory()
    {
        CmdLineParser p;
        Profile profile;
        struct Synchronization s = {};
        const char *argv[] = { "foo", "-b128K", "-w84", "-ep", "testfile.dat" };
        VERIFY_IS_TRUE(p.ParseCmdLine(_countof(argv), argv, &profile, &s) == true);
        VERIFY_IS_TRUE(profile.GetVerbose() == false);
        VERIFY_IS_TRUE(profile.GetProgress() == 0);
        VERIFY_IS_TRUE(profile.GetCmdLine().compare("foo -b128K -w84 -ep testfile.dat") == 0);
        VERIFY_IS_TRUE(profile.GetEtwEnabled() == true);
        VERIFY_IS_TRUE(profile.GetEtwProcess() == false);
        VERIFY_IS_TRUE(profile.GetEtwThread() == false);
        VERIFY_IS_TRUE(profile.GetEtwImageLoad() == false);
        VERIFY_IS_TRUE(profile.GetEtwDiskIO() == false);
        VERIFY_IS_TRUE(profile.GetEtwMemoryPageFaults() == false);
        VERIFY_IS_TRUE(profile.GetEtwMemoryHardFaults() == false);
        VERIFY_IS_TRUE(profile.GetEtwNetwork() == false);
        VERIFY_IS_TRUE(profile.GetEtwRegistry() == false);
        VERIFY_IS_TRUE(profile.GetEtwUsePagedMemory() == true);
        VERIFY_IS_TRUE(profile.GetEtwUsePerfTimer() == false);
        VERIFY_IS_TRUE(profile.GetEtwUseSystemTimer() == false);
        VERIFY_IS_TRUE(profile.GetEtwUseCyclesCounter() == false);

        vector<TimeSpan> vSpans(profile.GetTimeSpans());
        VERIFY_ARE_EQUAL(vSpans.size(), (size_t)1);
        VERIFY_ARE_EQUAL(vSpans[0].GetDuration(), (UINT32)10);
        VERIFY_ARE_EQUAL(vSpans[0].GetWarmup(), (UINT32)5);
        VERIFY_ARE_EQUAL(vSpans[0].GetCooldown(), (UINT32)0);
        VERIFY_ARE_EQUAL(vSpans[0].GetRandSeed(), (UINT32)0);
        VERIFY_ARE_EQUAL(vSpans[0].GetThreadCount(), (DWORD)0);
        VERIFY_ARE_EQUAL(vSpans[0].GetRequestCount(), (DWORD)0);
        VERIFY_IS_TRUE(vSpans[0].GetDisableAffinity() == false);
        VERIFY_IS_TRUE(vSpans[0].GetCompletionRoutines() == false);
        VERIFY_IS_TRUE(vSpans[0].GetMeasureLatency() == false);
        VERIFY_IS_TRUE(vSpans[0].GetRandomWriteData() == false);
        VERIFY_IS_TRUE(vSpans[0].GetUseIoRing() == false);

        const auto& vMasks(vSpans[0].GetAffinityGroupMasks());
        VERIFY_ARE_EQUAL(vMasks.size(), (size_t)0);

        vector<Target> vTargets(vSpans[0].GetTargets());
        VERIFY_ARE_EQUAL(vTargets.size(), (size_t)1);
        Target t(vTargets[0]);

        VERIFY_IS_TRUE(t.GetPath().compare("testfile.dat") == 0);
        VERIFY_ARE_EQUAL(t.GetBlockSizeInBytes(), (DWORD)(128 * 1024));
        VERIFY_ARE_EQUAL(t.GetRequestCount(), (DWORD)2);
        VERIFY_IS_TRUE(t.GetRandomRatio() == 0);
        VERIFY_IS_TRUE(t.GetUseInterlockedSequential() == false);
        VERIFY_ARE_EQUAL(t.GetBlockAlignmentInBytes(), t.GetBlockSizeInBytes());
        VERIFY_ARE_EQUAL(t.GetBaseFileOffsetInBytes(), 0);
        VERIFY_IS_TRUE(t.GetUseParallelAsyncIO() == false);
        VERIFY_IS_TRUE(t.GetCacheMode() == TargetCacheMode::Cached);
        VERIFY_IS_TRUE(t.GetWriteThroughMode() == WriteThroughMode::Off);
        VERIFY_IS_TRUE(t.GetMemoryMappedIoMode() == MemoryMappedIoMode::Off);
        VERIFY_IS_TRUE(t.GetMemoryMappedIoFlushMode() == MemoryMappedIoFlushMode::Undefined);
        VERIFY_IS_TRUE(t.GetBypassIoMode() == BypassIoMode::Undefined);
        VERIFY_IS_TRUE(t.GetZeroWriteBuffers() == false);
        VERIFY_ARE_EQUAL(t.GetThreadsPerFile(), (DWORD)1);
        VERIFY_ARE_EQUAL(t.GetThreadStrideInBytes(), 0);
        VERIFY_IS_TRUE(t.GetCreateFile() == false);
        VERIFY_ARE_EQUAL(t.GetFileSize(), 0);
        VERIFY_ARE_EQUAL(t.GetMaxFileSize(), 0);
        VERIFY_ARE_EQUAL(t.GetWriteRatio(), (DWORD)84);
        VERIFY_IS_TRUE(t.GetUseBurstSize() == false);
        VERIFY_ARE_EQUAL(t.GetBurstSize(), (DWORD)0);
        VERIFY_ARE_EQUAL(t.GetThinkTime(), (DWORD)0);
        VERIFY_IS_TRUE(t.GetEnableThinkTime() == false);
        VERIFY_IS_TRUE(t.GetSequentialScanHint() == false);
        VERIFY_IS_TRUE(t.GetRandomAccessHint() == false);
        VERIFY_IS_TRUE(t.GetTemporaryFileHint() == false);
        VERIFY_IS_TRUE(t.GetUseLargePages() == false);
        VERIFY_ARE_EQUAL(t.GetThroughputInBytesPerMillisecond(), (DWORD)0);
    }

    void CmdLineParserUnitTests::TestParseCmdLineEtwPROCESS()
    {
        CmdLineParser p;
        Profile profile;
        struct Synchronization s = {};
        const char *argv[] = { "foo", "-b128K", "-w84", "-ePROCESS", "testfile.dat" };
        VERIFY_IS_TRUE(p.ParseCmdLine(_countof(argv), argv, &profile, &s) == true);
        VERIFY_IS_TRUE(profile.GetVerbose() == false);
        VERIFY_IS_TRUE(profile.GetProgress() == 0);
        VERIFY_IS_TRUE(profile.GetCmdLine().compare("foo -b128K -w84 -ePROCESS testfile.dat") == 0);
        VERIFY_IS_TRUE(profile.GetEtwEnabled() == true);
        VERIFY_IS_TRUE(profile.GetEtwProcess() == true);
        VERIFY_IS_TRUE(profile.GetEtwThread() == false);
        VERIFY_IS_TRUE(profile.GetEtwImageLoad() == false);
        VERIFY_IS_TRUE(profile.GetEtwDiskIO() == false);
        VERIFY_IS_TRUE(profile.GetEtwMemoryPageFaults() == false);
        VERIFY_IS_TRUE(profile.GetEtwMemoryHardFaults() == false);
        VERIFY_IS_TRUE(profile.GetEtwNetwork() == false);
        VERIFY_IS_TRUE(profile.GetEtwRegistry() == false);
        VERIFY_IS_TRUE(profile.GetEtwUsePagedMemory() == false);
        VERIFY_IS_TRUE(profile.GetEtwUsePerfTimer() == false);
        VERIFY_IS_TRUE(profile.GetEtwUseSystemTimer() == false);
        VERIFY_IS_TRUE(profile.GetEtwUseCyclesCounter() == false);

        vector<TimeSpan> vSpans(profile.GetTimeSpans());
        VERIFY_ARE_EQUAL(vSpans.size(), (size_t)1);
        VERIFY_ARE_EQUAL(vSpans[0].GetDuration(), (UINT32)10);
        VERIFY_ARE_EQUAL(vSpans[0].GetWarmup(), (UINT32)5);
        VERIFY_ARE_EQUAL(vSpans[0].GetCooldown(), (UINT32)0);
        VERIFY_ARE_EQUAL(vSpans[0].GetRandSeed(), (UINT32)0);
        VERIFY_ARE_EQUAL(vSpans[0].GetThreadCount(), (DWORD)0);
        VERIFY_ARE_EQUAL(vSpans[0].GetRequestCount(), (DWORD)0);
        VERIFY_IS_TRUE(vSpans[0].GetDisableAffinity() == false);
        VERIFY_IS_TRUE(vSpans[0].GetCompletionRoutines() == false);
        VERIFY_IS_TRUE(vSpans[0].GetMeasureLatency() == false);
        VERIFY_IS_TRUE(vSpans[0].GetRandomWriteData() == false);
        VERIFY_IS_TRUE(vSpans[0].GetUseIoRing() == false);

        const auto& vMasks(vSpans[0].GetAffinityGroupMasks());
        VERIFY_ARE_EQUAL(vMasks.size(), (size_t)0);

        vector<Target> vTargets(vSpans[0].GetTargets());
        VERIFY_ARE_EQUAL(vTargets.size(), (size_t)1);
        Target t(vTargets[0]);

        VERIFY_IS_TRUE(t.GetPath().compare("testfile.dat") == 0);
        VERIFY_ARE_EQUAL(t.GetBlockSizeInBytes(), (DWORD)(128 * 1024));
        VERIFY_ARE_EQUAL(t.GetRequestCount(), (DWORD)2);
        VERIFY_IS_TRUE(t.GetRandomRatio() == 0);
        VERIFY_IS_TRUE(t.GetUseInterlockedSequential() == false);
        VERIFY_ARE_EQUAL(t.GetBlockAlignmentInBytes(), t.GetBlockSizeInBytes());
        VERIFY_ARE_EQUAL(t.GetBaseFileOffsetInBytes(), 0);
        VERIFY_IS_TRUE(t.GetUseParallelAsyncIO() == false);
        VERIFY_IS_TRUE(t.GetCacheMode() == TargetCacheMode::Cached);
        VERIFY_IS_TRUE(t.GetWriteThroughMode() == WriteThroughMode::Off);
        VERIFY_IS_TRUE(t.GetMemoryMappedIoMode() == MemoryMappedIoMode::Off);
        VERIFY_IS_TRUE(t.GetMemoryMappedIoFlushMode() == MemoryMappedIoFlushMode::Undefined);
        VERIFY_IS_TRUE(t.GetBypassIoMode() == BypassIoMode::Undefined);
        VERIFY_IS_TRUE(t.GetZeroWriteBuffers() == false);
        VERIFY_ARE_EQUAL(t.GetThreadsPerFile(), (DWORD)1);
        VERIFY_ARE_EQUAL(t.GetThreadStrideInBytes(), 0);
        VERIFY_IS_TRUE(t.GetCreateFile() == false);
        VERIFY_ARE_EQUAL(t.GetFileSize(), 0);
        VERIFY_ARE_EQUAL(t.GetMaxFileSize(), 0);
        VERIFY_ARE_EQUAL(t.GetWriteRatio(), (DWORD)84);
        VERIFY_IS_TRUE(t.GetUseBurstSize() == false);
        VERIFY_ARE_EQUAL(t.GetBurstSize(), (DWORD)0);
        VERIFY_ARE_EQUAL(t.GetThinkTime(), (DWORD)0);
        VERIFY_IS_TRUE(t.GetEnableThinkTime() == false);
        VERIFY_IS_TRUE(t.GetSequentialScanHint() == false);
        VERIFY_IS_TRUE(t.GetRandomAccessHint() == false);
        VERIFY_IS_TRUE(t.GetTemporaryFileHint() == false);
        VERIFY_IS_TRUE(t.GetUseLargePages() == false);
        VERIFY_ARE_EQUAL(t.GetThroughputInBytesPerMillisecond(), (DWORD)0);
    }

    void CmdLineParserUnitTests::TestParseCmdLineEtwTHREAD()
    {
        CmdLineParser p;
        Profile profile;
        struct Synchronization s = {};
        const char *argv[] = { "foo", "-b128K", "-w84", "-eTHREAD", "testfile.dat" };
        VERIFY_IS_TRUE(p.ParseCmdLine(_countof(argv), argv, &profile, &s) == true);
        VERIFY_IS_TRUE(profile.GetVerbose() == false);
        VERIFY_IS_TRUE(profile.GetProgress() == 0);
        VERIFY_IS_TRUE(profile.GetCmdLine().compare("foo -b128K -w84 -eTHREAD testfile.dat") == 0);
        VERIFY_IS_TRUE(profile.GetEtwEnabled() == true);
        VERIFY_IS_TRUE(profile.GetEtwProcess() == false);
        VERIFY_IS_TRUE(profile.GetEtwThread() == true);
        VERIFY_IS_TRUE(profile.GetEtwImageLoad() == false);
        VERIFY_IS_TRUE(profile.GetEtwDiskIO() == false);
        VERIFY_IS_TRUE(profile.GetEtwMemoryPageFaults() == false);
        VERIFY_IS_TRUE(profile.GetEtwMemoryHardFaults() == false);
        VERIFY_IS_TRUE(profile.GetEtwNetwork() == false);
        VERIFY_IS_TRUE(profile.GetEtwRegistry() == false);
        VERIFY_IS_TRUE(profile.GetEtwUsePagedMemory() == false);
        VERIFY_IS_TRUE(profile.GetEtwUsePerfTimer() == false);
        VERIFY_IS_TRUE(profile.GetEtwUseSystemTimer() == false);
        VERIFY_IS_TRUE(profile.GetEtwUseCyclesCounter() == false);

        vector<TimeSpan> vSpans(profile.GetTimeSpans());
        VERIFY_ARE_EQUAL(vSpans.size(), (size_t)1);
        VERIFY_ARE_EQUAL(vSpans[0].GetDuration(), (UINT32)10);
        VERIFY_ARE_EQUAL(vSpans[0].GetWarmup(), (UINT32)5);
        VERIFY_ARE_EQUAL(vSpans[0].GetCooldown(), (UINT32)0);
        VERIFY_ARE_EQUAL(vSpans[0].GetRandSeed(), (UINT32)0);
        VERIFY_ARE_EQUAL(vSpans[0].GetThreadCount(), (DWORD)0);
        VERIFY_ARE_EQUAL(vSpans[0].GetRequestCount(), (DWORD)0);
        VERIFY_IS_TRUE(vSpans[0].GetDisableAffinity() == false);
        VERIFY_IS_TRUE(vSpans[0].GetCompletionRoutines() == false);
        VERIFY_IS_TRUE(vSpans[0].GetMeasureLatency() == false);
        VERIFY_IS_TRUE(vSpans[0].GetRandomWriteData() == false);
        VERIFY_IS_TRUE(vSpans[0].GetUseIoRing() == false);

        const auto& vMasks(vSpans[0].GetAffinityGroupMasks());
        VERIFY_ARE_EQUAL(vMasks.size(), (size_t)0);

        vector<Target> vTargets(vSpans[0].GetTargets());
        VERIFY_ARE_EQUAL(vTargets.size(), (size_t)1);
        Target t(vTargets[0]);

        VERIFY_IS_TRUE(t.GetPath().compare("testfile.dat") == 0);
        VERIFY_ARE_EQUAL(t.GetBlockSizeInBytes(), (DWORD)(128 * 1024));
        VERIFY_ARE_EQUAL(t.GetRequestCount(), (DWORD)2);
        VERIFY_IS_TRUE(t.GetRandomRatio() == 0);
        VERIFY_IS_TRUE(t.GetUseInterlockedSequential() == false);
        VERIFY_ARE_EQUAL(t.GetBlockAlignmentInBytes(), t.GetBlockSizeInBytes());
        VERIFY_ARE_EQUAL(t.GetBaseFileOffsetInBytes(), 0);
        VERIFY_IS_TRUE(t.GetUseParallelAsyncIO() == false);
        VERIFY_IS_TRUE(t.GetCacheMode() == TargetCacheMode::Cached);
        VERIFY_IS_TRUE(t.GetWriteThroughMode() == WriteThroughMode::Off);
        VERIFY_IS_TRUE(t.GetMemoryMappedIoMode() == MemoryMappedIoMode::Off);
        VERIFY_IS_TRUE(t.GetMemoryMappedIoFlushMode() == MemoryMappedIoFlushMode::Undefined);
        VERIFY_IS_TRUE(t.GetBypassIoMode() == BypassIoMode::Undefined);
        VERIFY_IS_TRUE(t.GetZeroWriteBuffers() == false);
        VERIFY_ARE_EQUAL(t.GetThreadsPerFile(), (DWORD)1);
        VERIFY_ARE_EQUAL(t.GetThreadStrideInBytes(), 0);
        VERIFY_IS_TRUE(t.GetCreateFile() == false);
        VERIFY_ARE_EQUAL(t.GetFileSize(), 0);
        VERIFY_ARE_EQUAL(t.GetMaxFileSize(), 0);
        VERIFY_ARE_EQUAL(t.GetWriteRatio(), (DWORD)84);
        VERIFY_IS_TRUE(t.GetUseBurstSize() == false);
        VERIFY_ARE_EQUAL(t.GetBurstSize(), (DWORD)0);
        VERIFY_ARE_EQUAL(t.GetThinkTime(), (DWORD)0);
        VERIFY_IS_TRUE(t.GetEnableThinkTime() == false);
        VERIFY_IS_TRUE(t.GetSequentialScanHint() == false);
        VERIFY_IS_TRUE(t.GetRandomAccessHint() == false);
        VERIFY_IS_TRUE(t.GetTemporaryFileHint() == false);
        VERIFY_IS_TRUE(t.GetUseLargePages() == false);
        VERIFY_ARE_EQUAL(t.GetThroughputInBytesPerMillisecond(), (DWORD)0);
    }

    void CmdLineParserUnitTests::TestParseCmdLineEtwIMAGE_LOAD()
    {
        CmdLineParser p;
        Profile profile;
        struct Synchronization s = {};
        const char *argv[] = { "foo", "-b128K", "-w84", "-eIMAGE_LOAD", "testfile.dat" };
        VERIFY_IS_TRUE(p.ParseCmdLine(_countof(argv), argv, &profile, &s) == true);
        VERIFY_IS_TRUE(profile.GetVerbose() == false);
        VERIFY_IS_TRUE(profile.GetProgress() == 0);
        VERIFY_IS_TRUE(profile.GetCmdLine().compare("foo -b128K -w84 -eIMAGE_LOAD testfile.dat") == 0);
        VERIFY_IS_TRUE(profile.GetEtwEnabled() == true);
        VERIFY_IS_TRUE(profile.GetEtwProcess() == false);
        VERIFY_IS_TRUE(profile.GetEtwThread() == false);
        VERIFY_IS_TRUE(profile.GetEtwImageLoad() == true);
        VERIFY_IS_TRUE(profile.GetEtwDiskIO() == false);
        VERIFY_IS_TRUE(profile.GetEtwMemoryPageFaults() == false);
        VERIFY_IS_TRUE(profile.GetEtwMemoryHardFaults() == false);
        VERIFY_IS_TRUE(profile.GetEtwNetwork() == false);
        VERIFY_IS_TRUE(profile.GetEtwRegistry() == false);
        VERIFY_IS_TRUE(profile.GetEtwUsePagedMemory() == false);
        VERIFY_IS_TRUE(profile.GetEtwUsePerfTimer() == false);
        VERIFY_IS_TRUE(profile.GetEtwUseSystemTimer() == false);
        VERIFY_IS_TRUE(profile.GetEtwUseCyclesCounter() == false);

        vector<TimeSpan> vSpans(profile.GetTimeSpans());
        VERIFY_ARE_EQUAL(vSpans.size(), (size_t)1);
        VERIFY_ARE_EQUAL(vSpans[0].GetDuration(), (UINT32)10);
        VERIFY_ARE_EQUAL(vSpans[0].GetWarmup(), (UINT32)5);
        VERIFY_ARE_EQUAL(vSpans[0].GetCooldown(), (UINT32)0);
        VERIFY_ARE_EQUAL(vSpans[0].GetRandSeed(), (UINT32)0);
        VERIFY_ARE_EQUAL(vSpans[0].GetThreadCount(), (DWORD)0);
        VERIFY_ARE_EQUAL(vSpans[0].GetRequestCount(), (DWORD)0);
        VERIFY_IS_TRUE(vSpans[0].GetDisableAffinity() == false);
        VERIFY_IS_TRUE(vSpans[0].GetCompletionRoutines() == false);
        VERIFY_IS_TRUE(vSpans[0].GetMeasureLatency() == false);
        VERIFY_IS_TRUE(vSpans[0].GetRandomWriteData() == false);
        VERIFY_IS_TRUE(vSpans[0].GetUseIoRing() == false);

        const auto& vMasks(vSpans[0].GetAffinityGroupMasks());
        VERIFY_ARE_EQUAL(vMasks.size(), (size_t)0);

        vector<Target> vTargets(vSpans[0].GetTargets());
        VERIFY_ARE_EQUAL(vTargets.size(), (size_t)1);
        Target t(vTargets[0]);

        VERIFY_IS_TRUE(t.GetPath().compare("testfile.dat") == 0);
        VERIFY_ARE_EQUAL(t.GetBlockSizeInBytes(), (DWORD)(128 * 1024));
        VERIFY_ARE_EQUAL(t.GetRequestCount(), (DWORD)2);
        VERIFY_IS_TRUE(t.GetRandomRatio() == 0);
        VERIFY_IS_TRUE(t.GetUseInterlockedSequential() == false);
        VERIFY_ARE_EQUAL(t.GetBlockAlignmentInBytes(), t.GetBlockSizeInBytes());
        VERIFY_ARE_EQUAL(t.GetBaseFileOffsetInBytes(), 0);
        VERIFY_IS_TRUE(t.GetUseParallelAsyncIO() == false);
        VERIFY_IS_TRUE(t.GetCacheMode() == TargetCacheMode::Cached);
        VERIFY_IS_TRUE(t.GetWriteThroughMode() == WriteThroughMode::Off);
        VERIFY_IS_TRUE(t.GetMemoryMappedIoMode() == MemoryMappedIoMode::Off);
        VERIFY_IS_TRUE(t.GetMemoryMappedIoFlushMode() == MemoryMappedIoFlushMode::Undefined);
        VERIFY_IS_TRUE(t.GetBypassIoMode() == BypassIoMode::Undefined);
        VERIFY_IS_TRUE(t.GetZeroWriteBuffers() == false);
        VERIFY_ARE_EQUAL(t.GetThreadsPerFile(), (DWORD)1);
        VERIFY_ARE_EQUAL(t.GetThreadStrideInBytes(), 0);
        VERIFY_IS_TRUE(t.GetCreateFile() == false);
        VERIFY_ARE_EQUAL(t.GetFileSize(), 0);
        VERIFY_ARE_EQUAL(t.GetMaxFileSize(), 0);
        VERIFY_ARE_EQUAL(t.GetWriteRatio(), (DWORD)84);
        VERIFY_IS_TRUE(t.GetUseBurstSize() == false);
        VERIFY_ARE_EQUAL(t.GetBurstSize(), (DWORD)0);
        VERIFY_ARE_EQUAL(t.GetThinkTime(), (DWORD)0);
        VERIFY_IS_TRUE(t.GetEnableThinkTime() == false);
        VERIFY_IS_TRUE(t.GetSequentialScanHint() == false);
        VERIFY_IS_TRUE(t.GetRandomAccessHint() == false);
        VERIFY_IS_TRUE(t.GetTemporaryFileHint() == false);
        VERIFY_IS_TRUE(t.GetUseLargePages() == false);
        VERIFY_ARE_EQUAL(t.GetThroughputInBytesPerMillisecond(), (DWORD)0);
    }

    void CmdLineParserUnitTests::TestParseCmdLineEtwDISK_IO()
    {
        CmdLineParser p;
        Profile profile;
        struct Synchronization s = {};
        const char *argv[] = { "foo", "-b128K", "-w84", "-eDISK_IO", "testfile.dat" };
        VERIFY_IS_TRUE(p.ParseCmdLine(_countof(argv), argv, &profile, &s) == true);
        VERIFY_IS_TRUE(profile.GetVerbose() == false);
        VERIFY_IS_TRUE(profile.GetProgress() == 0);
        VERIFY_IS_TRUE(profile.GetCmdLine().compare("foo -b128K -w84 -eDISK_IO testfile.dat") == 0);
        VERIFY_IS_TRUE(profile.GetEtwEnabled() == true);
        VERIFY_IS_TRUE(profile.GetEtwProcess() == false);
        VERIFY_IS_TRUE(profile.GetEtwThread() == false);
        VERIFY_IS_TRUE(profile.GetEtwImageLoad() == false);
        VERIFY_IS_TRUE(profile.GetEtwDiskIO() == true);
        VERIFY_IS_TRUE(profile.GetEtwMemoryPageFaults() == false);
        VERIFY_IS_TRUE(profile.GetEtwMemoryHardFaults() == false);
        VERIFY_IS_TRUE(profile.GetEtwNetwork() == false);
        VERIFY_IS_TRUE(profile.GetEtwRegistry() == false);
        VERIFY_IS_TRUE(profile.GetEtwUsePagedMemory() == false);
        VERIFY_IS_TRUE(profile.GetEtwUsePerfTimer() == false);
        VERIFY_IS_TRUE(profile.GetEtwUseSystemTimer() == false);
        VERIFY_IS_TRUE(profile.GetEtwUseCyclesCounter() == false);

        vector<TimeSpan> vSpans(profile.GetTimeSpans());
        VERIFY_ARE_EQUAL(vSpans.size(), (size_t)1);
        VERIFY_ARE_EQUAL(vSpans[0].GetDuration(), (UINT32)10);
        VERIFY_ARE_EQUAL(vSpans[0].GetWarmup(), (UINT32)5);
        VERIFY_ARE_EQUAL(vSpans[0].GetCooldown(), (UINT32)0);
        VERIFY_ARE_EQUAL(vSpans[0].GetRandSeed(), (UINT32)0);
        VERIFY_ARE_EQUAL(vSpans[0].GetThreadCount(), (DWORD)0);
        VERIFY_ARE_EQUAL(vSpans[0].GetRequestCount(), (DWORD)0);
        VERIFY_IS_TRUE(vSpans[0].GetDisableAffinity() == false);
        VERIFY_IS_TRUE(vSpans[0].GetCompletionRoutines() == false);
        VERIFY_IS_TRUE(vSpans[0].GetMeasureLatency() == false);
        VERIFY_IS_TRUE(vSpans[0].GetRandomWriteData() == false);
        VERIFY_IS_TRUE(vSpans[0].GetUseIoRing() == false);

        const auto& vMasks(vSpans[0].GetAffinityGroupMasks());
        VERIFY_ARE_EQUAL(vMasks.size(), (size_t)0);

        vector<Target> vTargets(vSpans[0].GetTargets());
        VERIFY_ARE_EQUAL(vTargets.size(), (size_t)1);
        Target t(vTargets[0]);

        VERIFY_IS_TRUE(t.GetPath().compare("testfile.dat") == 0);
        VERIFY_ARE_EQUAL(t.GetBlockSizeInBytes(), (DWORD)(128 * 1024));
        VERIFY_ARE_EQUAL(t.GetRequestCount(), (DWORD)2);
        VERIFY_IS_TRUE(t.GetRandomRatio() == 0);
        VERIFY_IS_TRUE(t.GetUseInterlockedSequential() == false);
        VERIFY_ARE_EQUAL(t.GetBlockAlignmentInBytes(), t.GetBlockSizeInBytes());
        VERIFY_ARE_EQUAL(t.GetBaseFileOffsetInBytes(), 0);
        VERIFY_IS_TRUE(t.GetUseParallelAsyncIO() == false);
        VERIFY_IS_TRUE(t.GetCacheMode() == TargetCacheMode::Cached);
        VERIFY_IS_TRUE(t.GetWriteThroughMode() == WriteThroughMode::Off);
        VERIFY_IS_TRUE(t.GetMemoryMappedIoMode() == MemoryMappedIoMode::Off);
        VERIFY_IS_TRUE(t.GetMemoryMappedIoFlushMode() == MemoryMappedIoFlushMode::Undefined);
        VERIFY_IS_TRUE(t.GetBypassIoMode() == BypassIoMode::Undefined);
        VERIFY_IS_TRUE(t.GetZeroWriteBuffers() == false);
        VERIFY_ARE_EQUAL(t.GetThreadsPerFile(), (DWORD)1);
        VERIFY_ARE_EQUAL(t.GetThreadStrideInBytes(), 0);
        VERIFY_IS_TRUE(t.GetCreateFile() == false);
        VERIFY_ARE_EQUAL(t.GetFileSize(), 0);
        VERIFY_ARE_EQUAL(t.GetMaxFileSize(), 0);
        VERIFY_ARE_EQUAL(t.GetWriteRatio(), (DWORD)84);
        VERIFY_IS_TRUE(t.GetUseBurstSize() == false);
        VERIFY_ARE_EQUAL(t.GetBurstSize(), (DWORD)0);
        VERIFY_ARE_EQUAL(t.GetThinkTime(), (DWORD)0);
        VERIFY_IS_TRUE(t.GetEnableThinkTime() == false);
        VERIFY_IS_TRUE(t.GetSequentialScanHint() == false);
        VERIFY_IS_TRUE(t.GetRandomAccessHint() == false);
        VERIFY_IS_TRUE(t.GetTemporaryFileHint() == false);
        VERIFY_IS_TRUE(t.GetUseLargePages() == false);
        VERIFY_ARE_EQUAL(t.GetThroughputInBytesPerMillisecond(), (DWORD)0);
    }

    void CmdLineParserUnitTests::TestParseCmdLineEtwNETWORK()
    {
        CmdLineParser p;
        Profile profile;
        struct Synchronization s = {};
        const char *argv[] = { "foo", "-b128K", "-w84", "-eNETWORK", "testfile.dat" };
        VERIFY_IS_TRUE(p.ParseCmdLine(_countof(argv), argv, &profile, &s) == true);
        VERIFY_IS_TRUE(profile.GetVerbose() == false);
        VERIFY_IS_TRUE(profile.GetProgress() == 0);
        VERIFY_IS_TRUE(profile.GetCmdLine().compare("foo -b128K -w84 -eNETWORK testfile.dat") == 0);
        VERIFY_IS_TRUE(profile.GetEtwEnabled() == true);
        VERIFY_IS_TRUE(profile.GetEtwProcess() == false);
        VERIFY_IS_TRUE(profile.GetEtwThread() == false);
        VERIFY_IS_TRUE(profile.GetEtwImageLoad() == false);
        VERIFY_IS_TRUE(profile.GetEtwDiskIO() == false);
        VERIFY_IS_TRUE(profile.GetEtwMemoryPageFaults() == false);
        VERIFY_IS_TRUE(profile.GetEtwMemoryHardFaults() == false);
        VERIFY_IS_TRUE(profile.GetEtwNetwork() == true);
        VERIFY_IS_TRUE(profile.GetEtwRegistry() == false);
        VERIFY_IS_TRUE(profile.GetEtwUsePagedMemory() == false);
        VERIFY_IS_TRUE(profile.GetEtwUsePerfTimer() == false);
        VERIFY_IS_TRUE(profile.GetEtwUseSystemTimer() == false);
        VERIFY_IS_TRUE(profile.GetEtwUseCyclesCounter() == false);

        vector<TimeSpan> vSpans(profile.GetTimeSpans());
        VERIFY_ARE_EQUAL(vSpans.size(), (size_t)1);
        VERIFY_ARE_EQUAL(vSpans[0].GetDuration(), (UINT32)10);
        VERIFY_ARE_EQUAL(vSpans[0].GetWarmup(), (UINT32)5);
        VERIFY_ARE_EQUAL(vSpans[0].GetCooldown(), (UINT32)0);
        VERIFY_ARE_EQUAL(vSpans[0].GetRandSeed(), (UINT32)0);
        VERIFY_ARE_EQUAL(vSpans[0].GetThreadCount(), (DWORD)0);
        VERIFY_ARE_EQUAL(vSpans[0].GetRequestCount(), (DWORD)0);
        VERIFY_IS_TRUE(vSpans[0].GetDisableAffinity() == false);
        VERIFY_IS_TRUE(vSpans[0].GetCompletionRoutines() == false);
        VERIFY_IS_TRUE(vSpans[0].GetMeasureLatency() == false);
        VERIFY_IS_TRUE(vSpans[0].GetRandomWriteData() == false);
        VERIFY_IS_TRUE(vSpans[0].GetUseIoRing() == false);

        const auto& vMasks(vSpans[0].GetAffinityGroupMasks());
        VERIFY_ARE_EQUAL(vMasks.size(), (size_t)0);

        vector<Target> vTargets(vSpans[0].GetTargets());
        VERIFY_ARE_EQUAL(vTargets.size(), (size_t)1);
        Target t(vTargets[0]);

        VERIFY_IS_TRUE(t.GetPath().compare("testfile.dat") == 0);
        VERIFY_ARE_EQUAL(t.GetBlockSizeInBytes(), (DWORD)(128 * 1024));
        VERIFY_ARE_EQUAL(t.GetRequestCount(), (DWORD)2);
        VERIFY_IS_TRUE(t.GetRandomRatio() == 0);
        VERIFY_IS_TRUE(t.GetUseInterlockedSequential() == false);
        VERIFY_ARE_EQUAL(t.GetBlockAlignmentInBytes(), t.GetBlockSizeInBytes());
        VERIFY_ARE_EQUAL(t.GetBaseFileOffsetInBytes(), 0);
        VERIFY_IS_TRUE(t.GetUseParallelAsyncIO() == false);
        VERIFY_IS_TRUE(t.GetCacheMode() == TargetCacheMode::Cached);
        VERIFY_IS_TRUE(t.GetWriteThroughMode() == WriteThroughMode::Off);
        VERIFY_IS_TRUE(t.GetMemoryMappedIoMode() == MemoryMappedIoMode::Off);
        VERIFY_IS_TRUE(t.GetMemoryMappedIoFlushMode() == MemoryMappedIoFlushMode::Undefined);
        VERIFY_IS_TRUE(t.GetBypassIoMode() == BypassIoMode::Undefined);
        VERIFY_IS_TRUE(t.GetZeroWriteBuffers() == false);
        VERIFY_ARE_EQUAL(t.GetThreadsPerFile(), (DWORD)1);
        VERIFY_ARE_EQUAL(t.GetThreadStrideInBytes(), 0);
        VERIFY_IS_TRUE(t.GetCreateFile() == false);
        VERIFY_ARE_EQUAL(t.GetFileSize(), 0);
        VERIFY_ARE_EQUAL(t.GetMaxFileSize(), 0);
        VERIFY_ARE_EQUAL(t.GetWriteRatio(), (DWORD)84);
        VERIFY_IS_TRUE(t.GetUseBurstSize() == false);
        VERIFY_ARE_EQUAL(t.GetBurstSize(), (DWORD)0);
        VERIFY_ARE_EQUAL(t.GetThinkTime(), (DWORD)0);
        VERIFY_IS_TRUE(t.GetEnableThinkTime() == false);
        VERIFY_IS_TRUE(t.GetSequentialScanHint() == false);
        VERIFY_IS_TRUE(t.GetRandomAccessHint() == false);
        VERIFY_IS_TRUE(t.GetTemporaryFileHint() == false);
        VERIFY_IS_TRUE(t.GetUseLargePages() == false);
        VERIFY_ARE_EQUAL(t.GetThroughputInBytesPerMillisecond(), (DWORD)0);
    }

    void CmdLineParserUnitTests::TestParseCmdLineEtwREGISTRY()
    {
        CmdLineParser p;
        Profile profile;
        struct Synchronization s = {};
        const char *argv[] = { "foo", "-b128K", "-w84", "-eREGISTRY", "testfile.dat" };
        VERIFY_IS_TRUE(p.ParseCmdLine(_countof(argv), argv, &profile, &s) == true);
        VERIFY_IS_TRUE(profile.GetVerbose() == false);
        VERIFY_IS_TRUE(profile.GetProgress() == 0);
        VERIFY_IS_TRUE(profile.GetCmdLine().compare("foo -b128K -w84 -eREGISTRY testfile.dat") == 0);
        VERIFY_IS_TRUE(profile.GetEtwEnabled() == true);
        VERIFY_IS_TRUE(profile.GetEtwProcess() == false);
        VERIFY_IS_TRUE(profile.GetEtwThread() == false);
        VERIFY_IS_TRUE(profile.GetEtwImageLoad() == false);
        VERIFY_IS_TRUE(profile.GetEtwDiskIO() == false);
        VERIFY_IS_TRUE(profile.GetEtwMemoryPageFaults() == false);
        VERIFY_IS_TRUE(profile.GetEtwMemoryHardFaults() == false);
        VERIFY_IS_TRUE(profile.GetEtwNetwork() == false);
        VERIFY_IS_TRUE(profile.GetEtwRegistry() == true);
        VERIFY_IS_TRUE(profile.GetEtwUsePagedMemory() == false);
        VERIFY_IS_TRUE(profile.GetEtwUsePerfTimer() == false);
        VERIFY_IS_TRUE(profile.GetEtwUseSystemTimer() == false);
        VERIFY_IS_TRUE(profile.GetEtwUseCyclesCounter() == false);

        vector<TimeSpan> vSpans(profile.GetTimeSpans());
        VERIFY_ARE_EQUAL(vSpans.size(), (size_t)1);
        VERIFY_ARE_EQUAL(vSpans[0].GetDuration(), (UINT32)10);
        VERIFY_ARE_EQUAL(vSpans[0].GetWarmup(), (UINT32)5);
        VERIFY_ARE_EQUAL(vSpans[0].GetCooldown(), (UINT32)0);
        VERIFY_ARE_EQUAL(vSpans[0].GetRandSeed(), (UINT32)0);
        VERIFY_ARE_EQUAL(vSpans[0].GetThreadCount(), (DWORD)0);
        VERIFY_ARE_EQUAL(vSpans[0].GetRequestCount(), (DWORD)0);
        VERIFY_IS_TRUE(vSpans[0].GetDisableAffinity() == false);
        VERIFY_IS_TRUE(vSpans[0].GetCompletionRoutines() == false);
        VERIFY_IS_TRUE(vSpans[0].GetMeasureLatency() == false);
        VERIFY_IS_TRUE(vSpans[0].GetRandomWriteData() == false);
        VERIFY_IS_TRUE(vSpans[0].GetUseIoRing() == false);

        const auto& vMasks(vSpans[0].GetAffinityGroupMasks());
        VERIFY_ARE_EQUAL(vMasks.size(), (size_t)0);

        vector<Target> vTargets(vSpans[0].GetTargets());
        VERIFY_ARE_EQUAL(vTargets.size(), (size_t)1);
        Target t(vTargets[0]);

        VERIFY_IS_TRUE(t.GetPath().compare("testfile.dat") == 0);
        VERIFY_ARE_EQUAL(t.GetBlockSizeInBytes(), (DWORD)(128 * 1024));
        VERIFY_ARE_EQUAL(t.GetRequestCount(), (DWORD)2);
        VERIFY_IS_TRUE(t.GetRandomRatio() == 0);
        VERIFY_IS_TRUE(t.GetUseInterlockedSequential() == false);
        VERIFY_ARE_EQUAL(t.GetBlockAlignmentInBytes(), t.GetBlockSizeInBytes());
        VERIFY_ARE_EQUAL(t.GetBaseFileOffsetInBytes(), 0);
        VERIFY_IS_TRUE(t.GetUseParallelAsyncIO() == false);
        VERIFY_IS_TRUE(t.GetCacheMode() == TargetCacheMode::Cached);
        VERIFY_IS_TRUE(t.GetWriteThroughMode() == WriteThroughMode::Off);
        VERIFY_IS_TRUE(t.GetMemoryMappedIoMode() == MemoryMappedIoMode::Off);
        VERIFY_IS_TRUE(t.GetMemoryMappedIoFlushMode() == MemoryMappedIoFlushMode::Undefined);
        VERIFY_IS_TRUE(t.GetBypassIoMode() == BypassIoMode::Undefined);
        VERIFY_IS_TRUE(t.GetZeroWriteBuffers() == false);
        VERIFY_ARE_EQUAL(t.GetThreadsPerFile(), (DWORD)1);
        VERIFY_ARE_EQUAL(t.GetThreadStrideInBytes(), 0);
        VERIFY_IS_TRUE(t.GetCreateFile() == false);
        VERIFY_ARE_EQUAL(t.GetFileSize(), 0);
        VERIFY_ARE_EQUAL(t.GetMaxFileSize(), 0);
        VERIFY_ARE_EQUAL(t.GetWriteRatio(), (DWORD)84);
        VERIFY_IS_TRUE(t.GetUseBurstSize() == false);
        VERIFY_ARE_EQUAL(t.GetBurstSize(), (DWORD)0);
        VERIFY_ARE_EQUAL(t.GetThinkTime(), (DWORD)0);
        VERIFY_IS_TRUE(t.GetEnableThinkTime() == false);
        VERIFY_IS_TRUE(t.GetSequentialScanHint() == false);
        VERIFY_IS_TRUE(t.GetRandomAccessHint() == false);
        VERIFY_IS_TRUE(t.GetTemporaryFileHint() == false);
        VERIFY_IS_TRUE(t.GetUseLargePages() == false);
        VERIFY_ARE_EQUAL(t.GetThroughputInBytesPerMillisecond(), (DWORD)0);
    }

    void CmdLineParserUnitTests::TestParseCmdLineEtwMEMORY_PAGE_FAULTS()
    {
        CmdLineParser p;
        Profile profile;
        struct Synchronization s = {};
        const char *argv[] = { "foo", "-b128K", "-w84", "-eMEMORY_PAGE_FAULTS", "testfile.dat" };
        VERIFY_IS_TRUE(p.ParseCmdLine(_countof(argv), argv, &profile, &s) == true);
        VERIFY_IS_TRUE(profile.GetVerbose() == false);
        VERIFY_IS_TRUE(profile.GetProgress() == 0);
        VERIFY_IS_TRUE(profile.GetCmdLine().compare("foo -b128K -w84 -eMEMORY_PAGE_FAULTS testfile.dat") == 0);
        VERIFY_IS_TRUE(profile.GetEtwEnabled() == true);
        VERIFY_IS_TRUE(profile.GetEtwProcess() == false);
        VERIFY_IS_TRUE(profile.GetEtwThread() == false);
        VERIFY_IS_TRUE(profile.GetEtwImageLoad() == false);
        VERIFY_IS_TRUE(profile.GetEtwDiskIO() == false);
        VERIFY_IS_TRUE(profile.GetEtwMemoryPageFaults() == true);
        VERIFY_IS_TRUE(profile.GetEtwMemoryHardFaults() == false);
        VERIFY_IS_TRUE(profile.GetEtwNetwork() == false);
        VERIFY_IS_TRUE(profile.GetEtwRegistry() == false);
        VERIFY_IS_TRUE(profile.GetEtwUsePagedMemory() == false);
        VERIFY_IS_TRUE(profile.GetEtwUsePerfTimer() == false);
        VERIFY_IS_TRUE(profile.GetEtwUseSystemTimer() == false);
        VERIFY_IS_TRUE(profile.GetEtwUseCyclesCounter() == false);

        vector<TimeSpan> vSpans(profile.GetTimeSpans());
        VERIFY_ARE_EQUAL(vSpans.size(), (size_t)1);
        VERIFY_ARE_EQUAL(vSpans[0].GetDuration(), (UINT32)10);
        VERIFY_ARE_EQUAL(vSpans[0].GetWarmup(), (UINT32)5);
        VERIFY_ARE_EQUAL(vSpans[0].GetCooldown(), (UINT32)0);
        VERIFY_ARE_EQUAL(vSpans[0].GetRandSeed(), (UINT32)0);
        VERIFY_ARE_EQUAL(vSpans[0].GetThreadCount(), (DWORD)0);
        VERIFY_ARE_EQUAL(vSpans[0].GetRequestCount(), (DWORD)0);
        VERIFY_IS_TRUE(vSpans[0].GetDisableAffinity() == false);
        VERIFY_IS_TRUE(vSpans[0].GetCompletionRoutines() == false);
        VERIFY_IS_TRUE(vSpans[0].GetMeasureLatency() == false);
        VERIFY_IS_TRUE(vSpans[0].GetRandomWriteData() == false);
        VERIFY_IS_TRUE(vSpans[0].GetUseIoRing() == false);

        const auto& vMasks(vSpans[0].GetAffinityGroupMasks());
        VERIFY_ARE_EQUAL(vMasks.size(), (size_t)0);

        vector<Target> vTargets(vSpans[0].GetTargets());
        VERIFY_ARE_EQUAL(vTargets.size(), (size_t)1);
        Target t(vTargets[0]);

        VERIFY_IS_TRUE(t.GetPath().compare("testfile.dat") == 0);
        VERIFY_ARE_EQUAL(t.GetBlockSizeInBytes(), (DWORD)(128 * 1024));
        VERIFY_ARE_EQUAL(t.GetRequestCount(), (DWORD)2);
        VERIFY_IS_TRUE(t.GetRandomRatio() == 0);
        VERIFY_IS_TRUE(t.GetUseInterlockedSequential() == false);
        VERIFY_ARE_EQUAL(t.GetBlockAlignmentInBytes(), t.GetBlockSizeInBytes());
        VERIFY_ARE_EQUAL(t.GetBaseFileOffsetInBytes(), 0);
        VERIFY_IS_TRUE(t.GetUseParallelAsyncIO() == false);
        VERIFY_IS_TRUE(t.GetCacheMode() == TargetCacheMode::Cached);
        VERIFY_IS_TRUE(t.GetWriteThroughMode() == WriteThroughMode::Off);
        VERIFY_IS_TRUE(t.GetMemoryMappedIoMode() == MemoryMappedIoMode::Off);
        VERIFY_IS_TRUE(t.GetMemoryMappedIoFlushMode() == MemoryMappedIoFlushMode::Undefined);
        VERIFY_IS_TRUE(t.GetBypassIoMode() == BypassIoMode::Undefined);
        VERIFY_IS_TRUE(t.GetZeroWriteBuffers() == false);
        VERIFY_ARE_EQUAL(t.GetThreadsPerFile(), (DWORD)1);
        VERIFY_ARE_EQUAL(t.GetThreadStrideInBytes(), 0);
        VERIFY_IS_TRUE(t.GetCreateFile() == false);
        VERIFY_ARE_EQUAL(t.GetFileSize(), 0);
        VERIFY_ARE_EQUAL(t.GetMaxFileSize(), 0);
        VERIFY_ARE_EQUAL(t.GetWriteRatio(), (DWORD)84);
        VERIFY_IS_TRUE(t.GetUseBurstSize() == false);
        VERIFY_ARE_EQUAL(t.GetBurstSize(), (DWORD)0);
        VERIFY_ARE_EQUAL(t.GetThinkTime(), (DWORD)0);
        VERIFY_IS_TRUE(t.GetEnableThinkTime() == false);
        VERIFY_IS_TRUE(t.GetSequentialScanHint() == false);
        VERIFY_IS_TRUE(t.GetRandomAccessHint() == false);
        VERIFY_IS_TRUE(t.GetTemporaryFileHint() == false);
        VERIFY_IS_TRUE(t.GetUseLargePages() == false);
        VERIFY_ARE_EQUAL(t.GetThroughputInBytesPerMillisecond(), (DWORD)0);
    }

    void CmdLineParserUnitTests::TestParseCmdLineEtwMEMORY_HARD_FAULTS()
    {
        CmdLineParser p;
        Profile profile;
        struct Synchronization s = {};
        const char *argv[] = { "foo", "-b128K", "-w84", "-eMEMORY_HARD_FAULTS", "testfile.dat" };
        VERIFY_IS_TRUE(p.ParseCmdLine(_countof(argv), argv, &profile, &s) == true);
        VERIFY_IS_TRUE(profile.GetVerbose() == false);
        VERIFY_IS_TRUE(profile.GetProgress() == 0);
        VERIFY_IS_TRUE(profile.GetCmdLine().compare("foo -b128K -w84 -eMEMORY_HARD_FAULTS testfile.dat") == 0);
        VERIFY_IS_TRUE(profile.GetEtwEnabled() == true);
        VERIFY_IS_TRUE(profile.GetEtwProcess() == false);
        VERIFY_IS_TRUE(profile.GetEtwThread() == false);
        VERIFY_IS_TRUE(profile.GetEtwImageLoad() == false);
        VERIFY_IS_TRUE(profile.GetEtwDiskIO() == false);
        VERIFY_IS_TRUE(profile.GetEtwMemoryPageFaults() == false);
        VERIFY_IS_TRUE(profile.GetEtwMemoryHardFaults() == true);
        VERIFY_IS_TRUE(profile.GetEtwNetwork() == false);
        VERIFY_IS_TRUE(profile.GetEtwRegistry() == false);
        VERIFY_IS_TRUE(profile.GetEtwUsePagedMemory() == false);
        VERIFY_IS_TRUE(profile.GetEtwUsePerfTimer() == false);
        VERIFY_IS_TRUE(profile.GetEtwUseSystemTimer() == false);
        VERIFY_IS_TRUE(profile.GetEtwUseCyclesCounter() == false);

        vector<TimeSpan> vSpans(profile.GetTimeSpans());
        VERIFY_ARE_EQUAL(vSpans.size(), (size_t)1);
        VERIFY_ARE_EQUAL(vSpans[0].GetDuration(), (UINT32)10);
        VERIFY_ARE_EQUAL(vSpans[0].GetWarmup(), (UINT32)5);
        VERIFY_ARE_EQUAL(vSpans[0].GetCooldown(), (UINT32)0);
        VERIFY_ARE_EQUAL(vSpans[0].GetRandSeed(), (UINT32)0);
        VERIFY_ARE_EQUAL(vSpans[0].GetThreadCount(), (DWORD)0);
        VERIFY_ARE_EQUAL(vSpans[0].GetRequestCount(), (DWORD)0);
        VERIFY_IS_TRUE(vSpans[0].GetDisableAffinity() == false);
        VERIFY_IS_TRUE(vSpans[0].GetCompletionRoutines() == false);
        VERIFY_IS_TRUE(vSpans[0].GetMeasureLatency() == false);
        VERIFY_IS_TRUE(vSpans[0].GetRandomWriteData() == false);
        VERIFY_IS_TRUE(vSpans[0].GetUseIoRing() == false);

        const auto& vMasks(vSpans[0].GetAffinityGroupMasks());
        VERIFY_ARE_EQUAL(vMasks.size(), (size_t)0);

        vector<Target> vTargets(vSpans[0].GetTargets());
        VERIFY_ARE_EQUAL(vTargets.size(), (size_t)1);
        Target t(vTargets[0]);

        VERIFY_IS_TRUE(t.GetPath().compare("testfile.dat") == 0);
        VERIFY_ARE_EQUAL(t.GetBlockSizeInBytes(), (DWORD)(128 * 1024));
        VERIFY_ARE_EQUAL(t.GetRequestCount(), (DWORD)2);
        VERIFY_IS_TRUE(t.GetRandomRatio() == 0);
        VERIFY_IS_TRUE(t.GetUseInterlockedSequential() == false);
        VERIFY_ARE_EQUAL(t.GetBlockAlignmentInBytes(), t.GetBlockSizeInBytes());
        VERIFY_ARE_EQUAL(t.GetBaseFileOffsetInBytes(), 0);
        VERIFY_IS_TRUE(t.GetUseParallelAsyncIO() == false);
        VERIFY_IS_TRUE(t.GetCacheMode() == TargetCacheMode::Cached);
        VERIFY_IS_TRUE(t.GetWriteThroughMode() == WriteThroughMode::Off);
        VERIFY_IS_TRUE(t.GetMemoryMappedIoMode() == MemoryMappedIoMode::Off);
        VERIFY_IS_TRUE(t.GetMemoryMappedIoFlushMode() == MemoryMappedIoFlushMode::Undefined);
        VERIFY_IS_TRUE(t.GetBypassIoMode() == BypassIoMode::Undefined);
        VERIFY_IS_TRUE(t.GetZeroWriteBuffers() == false);
        VERIFY_ARE_EQUAL(t.GetThreadsPerFile(), (DWORD)1);
        VERIFY_ARE_EQUAL(t.GetThreadStrideInBytes(), 0);
        VERIFY_IS_TRUE(t.GetCreateFile() == false);
        VERIFY_ARE_EQUAL(t.GetFileSize(), 0);
        VERIFY_ARE_EQUAL(t.GetMaxFileSize(), 0);
        VERIFY_ARE_EQUAL(t.GetWriteRatio(), (DWORD)84);
        VERIFY_IS_TRUE(t.GetUseBurstSize() == false);
        VERIFY_ARE_EQUAL(t.GetBurstSize(), (DWORD)0);
        VERIFY_ARE_EQUAL(t.GetThinkTime(), (DWORD)0);
        VERIFY_IS_TRUE(t.GetEnableThinkTime() == false);
        VERIFY_IS_TRUE(t.GetSequentialScanHint() == false);
        VERIFY_IS_TRUE(t.GetRandomAccessHint() == false);
        VERIFY_IS_TRUE(t.GetTemporaryFileHint() == false);
        VERIFY_IS_TRUE(t.GetUseLargePages() == false);
        VERIFY_ARE_EQUAL(t.GetThroughputInBytesPerMillisecond(), (DWORD)0);
    }

    void CmdLineParserUnitTests::TestParseCmdLineEtwUsePerfTimer()
    {
        CmdLineParser p;
        Profile profile;
        struct Synchronization s = {};
        const char *argv[] = { "foo", "-b128K", "-w84", "-eq", "testfile.dat" };
        VERIFY_IS_TRUE(p.ParseCmdLine(_countof(argv), argv, &profile, &s) == true);
        VERIFY_IS_TRUE(profile.GetVerbose() == false);
        VERIFY_IS_TRUE(profile.GetProgress() == 0);
        VERIFY_IS_TRUE(profile.GetCmdLine().compare("foo -b128K -w84 -eq testfile.dat") == 0);
        VERIFY_IS_TRUE(profile.GetEtwEnabled() == true);
        VERIFY_IS_TRUE(profile.GetEtwProcess() == false);
        VERIFY_IS_TRUE(profile.GetEtwThread() == false);
        VERIFY_IS_TRUE(profile.GetEtwImageLoad() == false);
        VERIFY_IS_TRUE(profile.GetEtwDiskIO() == false);
        VERIFY_IS_TRUE(profile.GetEtwMemoryPageFaults() == false);
        VERIFY_IS_TRUE(profile.GetEtwMemoryHardFaults() == false);
        VERIFY_IS_TRUE(profile.GetEtwNetwork() == false);
        VERIFY_IS_TRUE(profile.GetEtwRegistry() == false);
        VERIFY_IS_TRUE(profile.GetEtwUsePagedMemory() == false);
        VERIFY_IS_TRUE(profile.GetEtwUsePerfTimer() == true);
        VERIFY_IS_TRUE(profile.GetEtwUseSystemTimer() == false);
        VERIFY_IS_TRUE(profile.GetEtwUseCyclesCounter() == false);

        vector<TimeSpan> vSpans(profile.GetTimeSpans());
        VERIFY_ARE_EQUAL(vSpans.size(), (size_t)1);
        VERIFY_ARE_EQUAL(vSpans[0].GetDuration(), (UINT32)10);
        VERIFY_ARE_EQUAL(vSpans[0].GetWarmup(), (UINT32)5);
        VERIFY_ARE_EQUAL(vSpans[0].GetCooldown(), (UINT32)0);
        VERIFY_ARE_EQUAL(vSpans[0].GetRandSeed(), (UINT32)0);
        VERIFY_ARE_EQUAL(vSpans[0].GetThreadCount(), (DWORD)0);
        VERIFY_ARE_EQUAL(vSpans[0].GetRequestCount(), (DWORD)0);
        VERIFY_IS_TRUE(vSpans[0].GetDisableAffinity() == false);
        VERIFY_IS_TRUE(vSpans[0].GetCompletionRoutines() == false);
        VERIFY_IS_TRUE(vSpans[0].GetMeasureLatency() == false);
        VERIFY_IS_TRUE(vSpans[0].GetRandomWriteData() == false);
        VERIFY_IS_TRUE(vSpans[0].GetUseIoRing() == false);

        const auto& vMasks(vSpans[0].GetAffinityGroupMasks());
        VERIFY_ARE_EQUAL(vMasks.size(), (size_t)0);

        vector<Target> vTargets(vSpans[0].GetTargets());
        VERIFY_ARE_EQUAL(vTargets.size(), (size_t)1);
        Target t(vTargets[0]);

        VERIFY_IS_TRUE(t.GetPath().compare("testfile.dat") == 0);
        VERIFY_ARE_EQUAL(t.GetBlockSizeInBytes(), (DWORD)(128 * 1024));
        VERIFY_ARE_EQUAL(t.GetRequestCount(), (DWORD)2);
        VERIFY_IS_TRUE(t.GetRandomRatio() == 0);
        VERIFY_IS_TRUE(t.GetUseInterlockedSequential() == false);
        VERIFY_ARE_EQUAL(t.GetBlockAlignmentInBytes(), t.GetBlockSizeInBytes());
        VERIFY_ARE_EQUAL(t.GetBaseFileOffsetInBytes(), 0);
        VERIFY_IS_TRUE(t.GetUseParallelAsyncIO() == false);
        VERIFY_IS_TRUE(t.GetCacheMode() == TargetCacheMode::Cached);
        VERIFY_IS_TRUE(t.GetWriteThroughMode() == WriteThroughMode::Off);
        VERIFY_IS_TRUE(t.GetMemoryMappedIoMode() == MemoryMappedIoMode::Off);
        VERIFY_IS_TRUE(t.GetMemoryMappedIoFlushMode() == MemoryMappedIoFlushMode::Undefined);
        VERIFY_IS_TRUE(t.GetBypassIoMode() == BypassIoMode::Undefined);
        VERIFY_IS_TRUE(t.GetZeroWriteBuffers() == false);
        VERIFY_ARE_EQUAL(t.GetThreadsPerFile(), (DWORD)1);
        VERIFY_ARE_EQUAL(t.GetThreadStrideInBytes(), 0);
        VERIFY_IS_TRUE(t.GetCreateFile() == false);
        VERIFY_ARE_EQUAL(t.GetFileSize(), 0);
        VERIFY_ARE_EQUAL(t.GetMaxFileSize(), 0);
        VERIFY_ARE_EQUAL(t.GetWriteRatio(), (DWORD)84);
        VERIFY_IS_TRUE(t.GetUseBurstSize() == false);
        VERIFY_ARE_EQUAL(t.GetBurstSize(), (DWORD)0);
        VERIFY_ARE_EQUAL(t.GetThinkTime(), (DWORD)0);
        VERIFY_IS_TRUE(t.GetEnableThinkTime() == false);
        VERIFY_IS_TRUE(t.GetSequentialScanHint() == false);
        VERIFY_IS_TRUE(t.GetRandomAccessHint() == false);
        VERIFY_IS_TRUE(t.GetTemporaryFileHint() == false);
        VERIFY_IS_TRUE(t.GetUseLargePages() == false);
        VERIFY_ARE_EQUAL(t.GetThroughputInBytesPerMillisecond(), (DWORD)0);
    }

    void CmdLineParserUnitTests::TestParseCmdLineEtwUseSystemTimer()
    {
        CmdLineParser p;
        Profile profile;
        struct Synchronization s = {};
        const char *argv[] = { "foo", "-b128K", "-w84", "-es", "testfile.dat" };
        VERIFY_IS_TRUE(p.ParseCmdLine(_countof(argv), argv, &profile, &s) == true);
        VERIFY_IS_TRUE(profile.GetVerbose() == false);
        VERIFY_IS_TRUE(profile.GetProgress() == 0);
        VERIFY_IS_TRUE(profile.GetCmdLine().compare("foo -b128K -w84 -es testfile.dat") == 0);
        VERIFY_IS_TRUE(profile.GetEtwEnabled() == true);
        VERIFY_IS_TRUE(profile.GetEtwProcess() == false);
        VERIFY_IS_TRUE(profile.GetEtwThread() == false);
        VERIFY_IS_TRUE(profile.GetEtwImageLoad() == false);
        VERIFY_IS_TRUE(profile.GetEtwDiskIO() == false);
        VERIFY_IS_TRUE(profile.GetEtwMemoryPageFaults() == false);
        VERIFY_IS_TRUE(profile.GetEtwMemoryHardFaults() == false);
        VERIFY_IS_TRUE(profile.GetEtwNetwork() == false);
        VERIFY_IS_TRUE(profile.GetEtwRegistry() == false);
        VERIFY_IS_TRUE(profile.GetEtwUsePagedMemory() == false);
        VERIFY_IS_TRUE(profile.GetEtwUsePerfTimer() == false);
        VERIFY_IS_TRUE(profile.GetEtwUseSystemTimer() == true);
        VERIFY_IS_TRUE(profile.GetEtwUseCyclesCounter() == false);

        vector<TimeSpan> vSpans(profile.GetTimeSpans());
        VERIFY_ARE_EQUAL(vSpans.size(), (size_t)1);
        VERIFY_ARE_EQUAL(vSpans[0].GetDuration(), (UINT32)10);
        VERIFY_ARE_EQUAL(vSpans[0].GetWarmup(), (UINT32)5);
        VERIFY_ARE_EQUAL(vSpans[0].GetCooldown(), (UINT32)0);
        VERIFY_ARE_EQUAL(vSpans[0].GetRandSeed(), (UINT32)0);
        VERIFY_ARE_EQUAL(vSpans[0].GetThreadCount(), (DWORD)0);
        VERIFY_ARE_EQUAL(vSpans[0].GetRequestCount(), (DWORD)0);
        VERIFY_IS_TRUE(vSpans[0].GetDisableAffinity() == false);
        VERIFY_IS_TRUE(vSpans[0].GetCompletionRoutines() == false);
        VERIFY_IS_TRUE(vSpans[0].GetMeasureLatency() == false);
        VERIFY_IS_TRUE(vSpans[0].GetRandomWriteData() == false);
        VERIFY_IS_TRUE(vSpans[0].GetUseIoRing() == false);

        const auto& vMasks(vSpans[0].GetAffinityGroupMasks());
        VERIFY_ARE_EQUAL(vMasks.size(), (size_t)0);

        vector<Target> vTargets(vSpans[0].GetTargets());
        VERIFY_ARE_EQUAL(vTargets.size(), (size_t)1);
        Target t(vTargets[0]);

        VERIFY_IS_TRUE(t.GetPath().compare("testfile.dat") == 0);
        VERIFY_ARE_EQUAL(t.GetBlockSizeInBytes(), (DWORD)(128 * 1024));
        VERIFY_ARE_EQUAL(t.GetRequestCount(), (DWORD)2);
        VERIFY_IS_TRUE(t.GetRandomRatio() == 0);
        VERIFY_IS_TRUE(t.GetUseInterlockedSequential() == false);
        VERIFY_ARE_EQUAL(t.GetBlockAlignmentInBytes(), t.GetBlockSizeInBytes());
        VERIFY_ARE_EQUAL(t.GetBaseFileOffsetInBytes(), 0);
        VERIFY_IS_TRUE(t.GetUseParallelAsyncIO() == false);
        VERIFY_IS_TRUE(t.GetCacheMode() == TargetCacheMode::Cached);
        VERIFY_IS_TRUE(t.GetWriteThroughMode() == WriteThroughMode::Off);
        VERIFY_IS_TRUE(t.GetMemoryMappedIoMode() == MemoryMappedIoMode::Off);
        VERIFY_IS_TRUE(t.GetMemoryMappedIoFlushMode() == MemoryMappedIoFlushMode::Undefined);
        VERIFY_IS_TRUE(t.GetBypassIoMode() == BypassIoMode::Undefined);
        VERIFY_IS_TRUE(t.GetZeroWriteBuffers() == false);
        VERIFY_ARE_EQUAL(t.GetThreadsPerFile(), (DWORD)1);
        VERIFY_ARE_EQUAL(t.GetThreadStrideInBytes(), 0);
        VERIFY_IS_TRUE(t.GetCreateFile() == false);
        VERIFY_ARE_EQUAL(t.GetFileSize(), 0);
        VERIFY_ARE_EQUAL(t.GetMaxFileSize(), 0);
        VERIFY_ARE_EQUAL(t.GetWriteRatio(), (DWORD)84);
        VERIFY_IS_TRUE(t.GetUseBurstSize() == false);
        VERIFY_ARE_EQUAL(t.GetBurstSize(), (DWORD)0);
        VERIFY_ARE_EQUAL(t.GetThinkTime(), (DWORD)0);
        VERIFY_IS_TRUE(t.GetEnableThinkTime() == false);
        VERIFY_IS_TRUE(t.GetSequentialScanHint() == false);
        VERIFY_IS_TRUE(t.GetRandomAccessHint() == false);
        VERIFY_IS_TRUE(t.GetTemporaryFileHint() == false);
        VERIFY_IS_TRUE(t.GetUseLargePages() == false);
        VERIFY_ARE_EQUAL(t.GetThroughputInBytesPerMillisecond(), (DWORD)0);
    }

    void CmdLineParserUnitTests::TestParseCmdLineEtwUseCycleCount()
    {
        CmdLineParser p;
        Profile profile;
        struct Synchronization s = {};
        const char *argv[] = { "foo", "-b128K", "-w84", "-ec", "testfile.dat" };
        VERIFY_IS_TRUE(p.ParseCmdLine(_countof(argv), argv, &profile, &s) == true);
        VERIFY_IS_TRUE(profile.GetVerbose() == false);
        VERIFY_IS_TRUE(profile.GetProgress() == 0);
        VERIFY_IS_TRUE(profile.GetCmdLine().compare("foo -b128K -w84 -ec testfile.dat") == 0);
        VERIFY_IS_TRUE(profile.GetEtwEnabled() == true);
        VERIFY_IS_TRUE(profile.GetEtwProcess() == false);
        VERIFY_IS_TRUE(profile.GetEtwThread() == false);
        VERIFY_IS_TRUE(profile.GetEtwImageLoad() == false);
        VERIFY_IS_TRUE(profile.GetEtwDiskIO() == false);
        VERIFY_IS_TRUE(profile.GetEtwMemoryPageFaults() == false);
        VERIFY_IS_TRUE(profile.GetEtwMemoryHardFaults() == false);
        VERIFY_IS_TRUE(profile.GetEtwNetwork() == false);
        VERIFY_IS_TRUE(profile.GetEtwRegistry() == false);
        VERIFY_IS_TRUE(profile.GetEtwUsePagedMemory() == false);
        VERIFY_IS_TRUE(profile.GetEtwUsePerfTimer() == false);
        VERIFY_IS_TRUE(profile.GetEtwUseSystemTimer() == false);
        VERIFY_IS_TRUE(profile.GetEtwUseCyclesCounter() == true);

        vector<TimeSpan> vSpans(profile.GetTimeSpans());
        VERIFY_ARE_EQUAL(vSpans.size(), (size_t)1);
        VERIFY_ARE_EQUAL(vSpans[0].GetDuration(), (UINT32)10);
        VERIFY_ARE_EQUAL(vSpans[0].GetWarmup(), (UINT32)5);
        VERIFY_ARE_EQUAL(vSpans[0].GetCooldown(), (UINT32)0);
        VERIFY_ARE_EQUAL(vSpans[0].GetRandSeed(), (UINT32)0);
        VERIFY_ARE_EQUAL(vSpans[0].GetThreadCount(), (DWORD)0);
        VERIFY_ARE_EQUAL(vSpans[0].GetRequestCount(), (DWORD)0);
        VERIFY_IS_TRUE(vSpans[0].GetDisableAffinity() == false);
        VERIFY_IS_TRUE(vSpans[0].GetCompletionRoutines() == false);
        VERIFY_IS_TRUE(vSpans[0].GetMeasureLatency() == false);
        VERIFY_IS_TRUE(vSpans[0].GetRandomWriteData() == false);
        VERIFY_IS_TRUE(vSpans[0].GetUseIoRing() == false);

        const auto& vMasks(vSpans[0].GetAffinityGroupMasks());
        VERIFY_ARE_EQUAL(vMasks.size(), (size_t)0);

        vector<Target> vTargets(vSpans[0].GetTargets());
        VERIFY_ARE_EQUAL(vTargets.size(), (size_t)1);
        Target t(vTargets[0]);

        VERIFY_IS_TRUE(t.GetPath().compare("testfile.dat") == 0);
        VERIFY_ARE_EQUAL(t.GetBlockSizeInBytes(), (DWORD)(128 * 1024));
        VERIFY_ARE_EQUAL(t.GetRequestCount(), (DWORD)2);
        VERIFY_IS_TRUE(t.GetRandomRatio() == 0);
        VERIFY_IS_TRUE(t.GetUseInterlockedSequential() == false);
        VERIFY_ARE_EQUAL(t.GetBlockAlignmentInBytes(), t.GetBlockSizeInBytes());
        VERIFY_ARE_EQUAL(t.GetBaseFileOffsetInBytes(), 0);
        VERIFY_IS_TRUE(t.GetUseParallelAsyncIO() == false);
        VERIFY_IS_TRUE(t.GetCacheMode() == TargetCacheMode::Cached);
        VERIFY_IS_TRUE(t.GetWriteThroughMode() == WriteThroughMode::Off);
        VERIFY_IS_TRUE(t.GetMemoryMappedIoMode() == MemoryMappedIoMode::Off);
        VERIFY_IS_TRUE(t.GetMemoryMappedIoFlushMode() == MemoryMappedIoFlushMode::Undefined);
        VERIFY_IS_TRUE(t.GetBypassIoMode() == BypassIoMode::Undefined);
        VERIFY_IS_TRUE(t.GetZeroWriteBuffers() == false);
        VERIFY_ARE_EQUAL(t.GetThreadsPerFile(), (DWORD)1);
        VERIFY_ARE_EQUAL(t.GetThreadStrideInBytes(), 0);
        VERIFY_IS_TRUE(t.GetCreateFile() == false);
        VERIFY_ARE_EQUAL(t.GetFileSize(), 0);
        VERIFY_ARE_EQUAL(t.GetMaxFileSize(), 0);
        VERIFY_ARE_EQUAL(t.GetWriteRatio(), (DWORD)84);
        VERIFY_IS_TRUE(t.GetUseBurstSize() == false);
        VERIFY_ARE_EQUAL(t.GetBurstSize(), (DWORD)0);
        VERIFY_ARE_EQUAL(t.GetThinkTime(), (DWORD)0);
        VERIFY_IS_TRUE(t.GetEnableThinkTime() == false);
        VERIFY_IS_TRUE(t.GetSequentialScanHint() == false);
        VERIFY_IS_TRUE(t.GetRandomAccessHint() == false);
        VERIFY_IS_TRUE(t.GetTemporaryFileHint() == false);
        VERIFY_IS_TRUE(t.GetUseLargePages() == false);
        VERIFY_ARE_EQUAL(t.GetThroughputInBytesPerMillisecond(), (DWORD)0);
    }

    void CmdLineParserUnitTests::TestParseCmdLineIOPriority()
    {
        CmdLineParser p;
        Profile profile;
        struct Synchronization s = {};
        const char *argv[] = { "foo", "-b128K", "-w84", "-I2", "testfile.dat" };
        VERIFY_IS_TRUE(p.ParseCmdLine(_countof(argv), argv, &profile, &s) == true);
        VERIFY_IS_TRUE(profile.GetVerbose() == false);
        VERIFY_IS_TRUE(profile.GetProgress() == 0);
        VERIFY_IS_TRUE(profile.GetCmdLine().compare("foo -b128K -w84 -I2 testfile.dat") == 0);
        VERIFY_IS_TRUE(profile.GetEtwEnabled() == false);
        VERIFY_IS_TRUE(profile.GetEtwProcess() == false);
        VERIFY_IS_TRUE(profile.GetEtwThread() == false);
        VERIFY_IS_TRUE(profile.GetEtwImageLoad() == false);
        VERIFY_IS_TRUE(profile.GetEtwDiskIO() == false);
        VERIFY_IS_TRUE(profile.GetEtwMemoryPageFaults() == false);
        VERIFY_IS_TRUE(profile.GetEtwMemoryHardFaults() == false);
        VERIFY_IS_TRUE(profile.GetEtwNetwork() == false);
        VERIFY_IS_TRUE(profile.GetEtwRegistry() == false);
        VERIFY_IS_TRUE(profile.GetEtwUsePagedMemory() == false);
        VERIFY_IS_TRUE(profile.GetEtwUsePerfTimer() == false);
        VERIFY_IS_TRUE(profile.GetEtwUseSystemTimer() == false);
        VERIFY_IS_TRUE(profile.GetEtwUseCyclesCounter() == false);

        vector<TimeSpan> vSpans(profile.GetTimeSpans());
        VERIFY_ARE_EQUAL(vSpans.size(), (size_t)1);
        VERIFY_ARE_EQUAL(vSpans[0].GetDuration(), (UINT32)10);
        VERIFY_ARE_EQUAL(vSpans[0].GetWarmup(), (UINT32)5);
        VERIFY_ARE_EQUAL(vSpans[0].GetCooldown(), (UINT32)0);
        VERIFY_ARE_EQUAL(vSpans[0].GetRandSeed(), (UINT32)0);
        VERIFY_ARE_EQUAL(vSpans[0].GetThreadCount(), (DWORD)0);
        VERIFY_ARE_EQUAL(vSpans[0].GetRequestCount(), (DWORD)0);
        VERIFY_IS_TRUE(vSpans[0].GetDisableAffinity() == false);
        VERIFY_IS_TRUE(vSpans[0].GetCompletionRoutines() == false);
        VERIFY_IS_TRUE(vSpans[0].GetMeasureLatency() == false);
        VERIFY_IS_TRUE(vSpans[0].GetRandomWriteData() == false);
        VERIFY_IS_TRUE(vSpans[0].GetUseIoRing() == false);

        const auto& vMasks(vSpans[0].GetAffinityGroupMasks());
        VERIFY_ARE_EQUAL(vMasks.size(), (size_t)0);

        vector<Target> vTargets(vSpans[0].GetTargets());
        VERIFY_ARE_EQUAL(vTargets.size(), (size_t)1);
        Target t(vTargets[0]);

        VERIFY_IS_TRUE(t.GetPath().compare("testfile.dat") == 0);
        VERIFY_ARE_EQUAL(t.GetBlockSizeInBytes(), (DWORD)(128 * 1024));
        VERIFY_ARE_EQUAL(t.GetRequestCount(), (DWORD)2);
        VERIFY_IS_TRUE(t.GetRandomRatio() == 0);
        VERIFY_IS_TRUE(t.GetUseInterlockedSequential() == false);
        VERIFY_ARE_EQUAL(t.GetBlockAlignmentInBytes(), t.GetBlockSizeInBytes());
        VERIFY_ARE_EQUAL(t.GetBaseFileOffsetInBytes(), 0);
        VERIFY_IS_TRUE(t.GetUseParallelAsyncIO() == false);
        VERIFY_IS_TRUE(t.GetCacheMode() == TargetCacheMode::Cached);
        VERIFY_IS_TRUE(t.GetWriteThroughMode() == WriteThroughMode::Off);
        VERIFY_IS_TRUE(t.GetMemoryMappedIoMode() == MemoryMappedIoMode::Off);
        VERIFY_IS_TRUE(t.GetMemoryMappedIoFlushMode() == MemoryMappedIoFlushMode::Undefined);
        VERIFY_IS_TRUE(t.GetBypassIoMode() == BypassIoMode::Undefined);
        VERIFY_IS_TRUE(t.GetZeroWriteBuffers() == false);
        VERIFY_ARE_EQUAL(t.GetThreadsPerFile(), (DWORD)1);
        VERIFY_ARE_EQUAL(t.GetThreadStrideInBytes(), 0);
        VERIFY_IS_TRUE(t.GetCreateFile() == false);
        VERIFY_ARE_EQUAL(t.GetFileSize(), 0);
        VERIFY_ARE_EQUAL(t.GetMaxFileSize(), 0);
        VERIFY_ARE_EQUAL(t.GetWriteRatio(), (DWORD)84);
        VERIFY_IS_TRUE(t.GetUseBurstSize() == false);
        VERIFY_ARE_EQUAL(t.GetBurstSize(), (DWORD)0);
        VERIFY_ARE_EQUAL(t.GetThinkTime(), (DWORD)0);
        VERIFY_IS_TRUE(t.GetEnableThinkTime() == false);
        VERIFY_IS_TRUE(t.GetSequentialScanHint() == false);
        VERIFY_IS_TRUE(t.GetRandomAccessHint() == false);
        VERIFY_IS_TRUE(t.GetTemporaryFileHint() == false);
        VERIFY_IS_TRUE(t.GetUseLargePages() == false);
        VERIFY_IS_TRUE(t.GetIOPriorityHint() == IoPriorityHintLow);
        VERIFY_ARE_EQUAL(t.GetThroughputInBytesPerMillisecond(), (DWORD)0);
    }

    void CmdLineParserUnitTests::TestParseCmdLineMeasureLatency()
    {
        CmdLineParser p;
        Profile profile;
        struct Synchronization s = {};
        const char *argv[] = { "foo", "-b128K", "-w84", "-L", "testfile.dat" };
        VERIFY_IS_TRUE(p.ParseCmdLine(_countof(argv), argv, &profile, &s) == true);
        VERIFY_IS_TRUE(profile.GetVerbose() == false);
        VERIFY_IS_TRUE(profile.GetProgress() == 0);
        VERIFY_IS_TRUE(profile.GetCmdLine().compare("foo -b128K -w84 -L testfile.dat") == 0);
        VERIFY_IS_TRUE(profile.GetEtwEnabled() == false);
        VERIFY_IS_TRUE(profile.GetEtwProcess() == false);
        VERIFY_IS_TRUE(profile.GetEtwThread() == false);
        VERIFY_IS_TRUE(profile.GetEtwImageLoad() == false);
        VERIFY_IS_TRUE(profile.GetEtwDiskIO() == false);
        VERIFY_IS_TRUE(profile.GetEtwMemoryPageFaults() == false);
        VERIFY_IS_TRUE(profile.GetEtwMemoryHardFaults() == false);
        VERIFY_IS_TRUE(profile.GetEtwNetwork() == false);
        VERIFY_IS_TRUE(profile.GetEtwRegistry() == false);
        VERIFY_IS_TRUE(profile.GetEtwUsePagedMemory() == false);
        VERIFY_IS_TRUE(profile.GetEtwUsePerfTimer() == false);
        VERIFY_IS_TRUE(profile.GetEtwUseSystemTimer() == false);
        VERIFY_IS_TRUE(profile.GetEtwUseCyclesCounter() == false);

        vector<TimeSpan> vSpans(profile.GetTimeSpans());
        VERIFY_ARE_EQUAL(vSpans.size(), (size_t)1);
        VERIFY_ARE_EQUAL(vSpans[0].GetDuration(), (UINT32)10);
        VERIFY_ARE_EQUAL(vSpans[0].GetWarmup(), (UINT32)5);
        VERIFY_ARE_EQUAL(vSpans[0].GetCooldown(), (UINT32)0);
        VERIFY_ARE_EQUAL(vSpans[0].GetRandSeed(), (UINT32)0);
        VERIFY_ARE_EQUAL(vSpans[0].GetThreadCount(), (DWORD)0);
        VERIFY_ARE_EQUAL(vSpans[0].GetRequestCount(), (DWORD)0);
        VERIFY_IS_TRUE(vSpans[0].GetDisableAffinity() == false);
        VERIFY_IS_TRUE(vSpans[0].GetCompletionRoutines() == false);
        VERIFY_IS_TRUE(vSpans[0].GetMeasureLatency() == true);
        VERIFY_IS_TRUE(vSpans[0].GetRandomWriteData() == false);
        VERIFY_IS_TRUE(vSpans[0].GetUseIoRing() == false);

        const auto& vMasks(vSpans[0].GetAffinityGroupMasks());
        VERIFY_ARE_EQUAL(vMasks.size(), (size_t)0);

        vector<Target> vTargets(vSpans[0].GetTargets());
        VERIFY_ARE_EQUAL(vTargets.size(), (size_t)1);
        Target t(vTargets[0]);

        VERIFY_IS_TRUE(t.GetPath().compare("testfile.dat") == 0);
        VERIFY_ARE_EQUAL(t.GetBlockSizeInBytes(), (DWORD)(128 * 1024));
        VERIFY_ARE_EQUAL(t.GetRequestCount(), (DWORD)2);
        VERIFY_IS_TRUE(t.GetRandomRatio() == 0);
        VERIFY_IS_TRUE(t.GetUseInterlockedSequential() == false);
        VERIFY_ARE_EQUAL(t.GetBlockAlignmentInBytes(), t.GetBlockSizeInBytes());
        VERIFY_ARE_EQUAL(t.GetBaseFileOffsetInBytes(), 0);
        VERIFY_IS_TRUE(t.GetUseParallelAsyncIO() == false);
        VERIFY_IS_TRUE(t.GetCacheMode() == TargetCacheMode::Cached);
        VERIFY_IS_TRUE(t.GetWriteThroughMode() == WriteThroughMode::Off);
        VERIFY_IS_TRUE(t.GetMemoryMappedIoMode() == MemoryMappedIoMode::Off);
        VERIFY_IS_TRUE(t.GetMemoryMappedIoFlushMode() == MemoryMappedIoFlushMode::Undefined);
        VERIFY_IS_TRUE(t.GetBypassIoMode() == BypassIoMode::Undefined);
        VERIFY_IS_TRUE(t.GetZeroWriteBuffers() == false);
        VERIFY_ARE_EQUAL(t.GetThreadsPerFile(), (DWORD)1);
        VERIFY_ARE_EQUAL(t.GetThreadStrideInBytes(), 0);
        VERIFY_IS_TRUE(t.GetCreateFile() == false);
        VERIFY_ARE_EQUAL(t.GetFileSize(), 0);
        VERIFY_ARE_EQUAL(t.GetMaxFileSize(), 0);
        VERIFY_ARE_EQUAL(t.GetWriteRatio(), (DWORD)84);
        VERIFY_IS_TRUE(t.GetUseBurstSize() == false);
        VERIFY_ARE_EQUAL(t.GetBurstSize(), (DWORD)0);
        VERIFY_ARE_EQUAL(t.GetThinkTime(), (DWORD)0);
        VERIFY_IS_TRUE(t.GetEnableThinkTime() == false);
        VERIFY_IS_TRUE(t.GetSequentialScanHint() == false);
        VERIFY_IS_TRUE(t.GetRandomAccessHint() == false);
        VERIFY_IS_TRUE(t.GetTemporaryFileHint() == false);
        VERIFY_IS_TRUE(t.GetUseLargePages() == false);
        VERIFY_ARE_EQUAL(t.GetThroughputInBytesPerMillisecond(), (DWORD)0);
    }

    void CmdLineParserUnitTests::TestParseCmdLineZeroWriteBuffers()
    {
        CmdLineParser p;
        Profile profile;
        struct Synchronization s = {};
        const char *argv[] = { "foo", "-b128K", "-w84", "-Z", "testfile.dat" };
        VERIFY_IS_TRUE(p.ParseCmdLine(_countof(argv), argv, &profile, &s) == true);
        VERIFY_IS_TRUE(profile.GetVerbose() == false);
        VERIFY_IS_TRUE(profile.GetProgress() == 0);
        VERIFY_IS_TRUE(profile.GetCmdLine().compare("foo -b128K -w84 -Z testfile.dat") == 0);
        VERIFY_IS_TRUE(profile.GetEtwEnabled() == false);
        VERIFY_IS_TRUE(profile.GetEtwProcess() == false);
        VERIFY_IS_TRUE(profile.GetEtwThread() == false);
        VERIFY_IS_TRUE(profile.GetEtwImageLoad() == false);
        VERIFY_IS_TRUE(profile.GetEtwDiskIO() == false);
        VERIFY_IS_TRUE(profile.GetEtwMemoryPageFaults() == false);
        VERIFY_IS_TRUE(profile.GetEtwMemoryHardFaults() == false);
        VERIFY_IS_TRUE(profile.GetEtwNetwork() == false);
        VERIFY_IS_TRUE(profile.GetEtwRegistry() == false);
        VERIFY_IS_TRUE(profile.GetEtwUsePagedMemory() == false);
        VERIFY_IS_TRUE(profile.GetEtwUsePerfTimer() == false);
        VERIFY_IS_TRUE(profile.GetEtwUseSystemTimer() == false);
        VERIFY_IS_TRUE(profile.GetEtwUseCyclesCounter() == false);

        vector<TimeSpan> vSpans(profile.GetTimeSpans());
        VERIFY_ARE_EQUAL(vSpans.size(), (size_t)1);
        VERIFY_ARE_EQUAL(vSpans[0].GetDuration(), (UINT32)10);
        VERIFY_ARE_EQUAL(vSpans[0].GetWarmup(), (UINT32)5);
        VERIFY_ARE_EQUAL(vSpans[0].GetCooldown(), (UINT32)0);
        VERIFY_ARE_EQUAL(vSpans[0].GetRandSeed(), (UINT32)0);
        VERIFY_ARE_EQUAL(vSpans[0].GetThreadCount(), (DWORD)0);
        VERIFY_ARE_EQUAL(vSpans[0].GetRequestCount(), (DWORD)0);
        VERIFY_IS_TRUE(vSpans[0].GetDisableAffinity() == false);
        VERIFY_IS_TRUE(vSpans[0].GetCompletionRoutines() == false);
        VERIFY_IS_TRUE(vSpans[0].GetMeasureLatency() == false);
        VERIFY_IS_TRUE(vSpans[0].GetRandomWriteData() == false);
        VERIFY_IS_TRUE(vSpans[0].GetUseIoRing() == false);

        const auto& vMasks(vSpans[0].GetAffinityGroupMasks());
        VERIFY_ARE_EQUAL(vMasks.size(), (size_t)0);

        vector<Target> vTargets(vSpans[0].GetTargets());
        VERIFY_ARE_EQUAL(vTargets.size(), (size_t)1);
        Target t(vTargets[0]);

        VERIFY_IS_TRUE(t.GetPath().compare("testfile.dat") == 0);
        VERIFY_ARE_EQUAL(t.GetBlockSizeInBytes(), (DWORD)(128 * 1024));
        VERIFY_ARE_EQUAL(t.GetRequestCount(), (DWORD)2);
        VERIFY_IS_TRUE(t.GetRandomRatio() == 0);
        VERIFY_IS_TRUE(t.GetUseInterlockedSequential() == false);
        VERIFY_ARE_EQUAL(t.GetBlockAlignmentInBytes(), t.GetBlockSizeInBytes());
        VERIFY_ARE_EQUAL(t.GetBaseFileOffsetInBytes(), 0);
        VERIFY_IS_TRUE(t.GetUseParallelAsyncIO() == false);
        VERIFY_IS_TRUE(t.GetCacheMode() == TargetCacheMode::Cached);
        VERIFY_IS_TRUE(t.GetWriteThroughMode() == WriteThroughMode::Off);
        VERIFY_IS_TRUE(t.GetMemoryMappedIoMode() == MemoryMappedIoMode::Off);
        VERIFY_IS_TRUE(t.GetMemoryMappedIoFlushMode() == MemoryMappedIoFlushMode::Undefined);
        VERIFY_IS_TRUE(t.GetBypassIoMode() == BypassIoMode::Undefined);
        VERIFY_IS_TRUE(t.GetZeroWriteBuffers() == true);
        VERIFY_ARE_EQUAL(t.GetThreadsPerFile(), (DWORD)1);
        VERIFY_ARE_EQUAL(t.GetThreadStrideInBytes(), 0);
        VERIFY_IS_TRUE(t.GetCreateFile() == false);
        VERIFY_ARE_EQUAL(t.GetFileSize(), 0);
        VERIFY_ARE_EQUAL(t.GetMaxFileSize(), 0);
        VERIFY_ARE_EQUAL(t.GetWriteRatio(), (DWORD)84);
        VERIFY_IS_TRUE(t.GetUseBurstSize() == false);
        VERIFY_ARE_EQUAL(t.GetBurstSize(), (DWORD)0);
        VERIFY_ARE_EQUAL(t.GetThinkTime(), (DWORD)0);
        VERIFY_IS_TRUE(t.GetEnableThinkTime() == false);
        VERIFY_IS_TRUE(t.GetSequentialScanHint() == false);
        VERIFY_IS_TRUE(t.GetRandomAccessHint() == false);
        VERIFY_IS_TRUE(t.GetTemporaryFileHint() == false);
        VERIFY_IS_TRUE(t.GetUseLargePages() == false);
        VERIFY_ARE_EQUAL(t.GetThroughputInBytesPerMillisecond(), (DWORD)0);
        VERIFY_ARE_EQUAL(t.GetRandomDataWriteBufferSize(), 0);
        VERIFY_ARE_EQUAL(t.GetRandomDataWriteBufferSourcePath(), "");
    }

    void CmdLineParserUnitTests::TestParseCmdLineRandomWriteBuffers()
    {
        CmdLineParser p;
        Profile profile;
        struct Synchronization s = {};
        const char *argv[] = { "foo", "-b128K", "-w84", "-Zr", "testfile.dat" };
        VERIFY_IS_TRUE(p.ParseCmdLine(_countof(argv), argv, &profile, &s) == true);
        VERIFY_IS_TRUE(profile.GetVerbose() == false);
        VERIFY_IS_TRUE(profile.GetProgress() == 0);
        VERIFY_IS_TRUE(profile.GetCmdLine().compare("foo -b128K -w84 -Zr testfile.dat") == 0);
        VERIFY_IS_TRUE(profile.GetEtwEnabled() == false);
        VERIFY_IS_TRUE(profile.GetEtwProcess() == false);
        VERIFY_IS_TRUE(profile.GetEtwThread() == false);
        VERIFY_IS_TRUE(profile.GetEtwImageLoad() == false);
        VERIFY_IS_TRUE(profile.GetEtwDiskIO() == false);
        VERIFY_IS_TRUE(profile.GetEtwMemoryPageFaults() == false);
        VERIFY_IS_TRUE(profile.GetEtwMemoryHardFaults() == false);
        VERIFY_IS_TRUE(profile.GetEtwNetwork() == false);
        VERIFY_IS_TRUE(profile.GetEtwRegistry() == false);
        VERIFY_IS_TRUE(profile.GetEtwUsePagedMemory() == false);
        VERIFY_IS_TRUE(profile.GetEtwUsePerfTimer() == false);
        VERIFY_IS_TRUE(profile.GetEtwUseSystemTimer() == false);
        VERIFY_IS_TRUE(profile.GetEtwUseCyclesCounter() == false);

        vector<TimeSpan> vSpans(profile.GetTimeSpans());
        VERIFY_ARE_EQUAL(vSpans.size(), (size_t)1);
        VERIFY_ARE_EQUAL(vSpans[0].GetDuration(), (UINT32)10);
        VERIFY_ARE_EQUAL(vSpans[0].GetWarmup(), (UINT32)5);
        VERIFY_ARE_EQUAL(vSpans[0].GetCooldown(), (UINT32)0);
        VERIFY_ARE_EQUAL(vSpans[0].GetRandSeed(), (UINT32)0);
        VERIFY_ARE_EQUAL(vSpans[0].GetThreadCount(), (DWORD)0);
        VERIFY_ARE_EQUAL(vSpans[0].GetRequestCount(), (DWORD)0);
        VERIFY_IS_TRUE(vSpans[0].GetDisableAffinity() == false);
        VERIFY_IS_TRUE(vSpans[0].GetCompletionRoutines() == false);
        VERIFY_IS_TRUE(vSpans[0].GetMeasureLatency() == false);
        VERIFY_IS_TRUE(vSpans[0].GetRandomWriteData() == true);
        VERIFY_IS_TRUE(vSpans[0].GetUseIoRing() == false);

        const auto& vMasks(vSpans[0].GetAffinityGroupMasks());
        VERIFY_ARE_EQUAL(vMasks.size(), (size_t)0);

        vector<Target> vTargets(vSpans[0].GetTargets());
        VERIFY_ARE_EQUAL(vTargets.size(), (size_t)1);
        Target t(vTargets[0]);

        VERIFY_IS_TRUE(t.GetPath().compare("testfile.dat") == 0);
        VERIFY_ARE_EQUAL(t.GetBlockSizeInBytes(), (DWORD)(128 * 1024));
        VERIFY_ARE_EQUAL(t.GetRequestCount(), (DWORD)2);
        VERIFY_IS_TRUE(t.GetRandomRatio() == 0);
        VERIFY_IS_TRUE(t.GetUseInterlockedSequential() == false);
        VERIFY_ARE_EQUAL(t.GetBlockAlignmentInBytes(), t.GetBlockSizeInBytes());
        VERIFY_ARE_EQUAL(t.GetBaseFileOffsetInBytes(), 0);
        VERIFY_IS_TRUE(t.GetUseParallelAsyncIO() == false);
        VERIFY_IS_TRUE(t.GetCacheMode() == TargetCacheMode::Cached);
        VERIFY_IS_TRUE(t.GetWriteThroughMode() == WriteThroughMode::Off);
        VERIFY_IS_TRUE(t.GetMemoryMappedIoMode() == MemoryMappedIoMode::Off);
        VERIFY_IS_TRUE(t.GetMemoryMappedIoFlushMode() == MemoryMappedIoFlushMode::Undefined);
        VERIFY_IS_TRUE(t.GetBypassIoMode() == BypassIoMode::Undefined);
        VERIFY_IS_TRUE(t.GetZeroWriteBuffers() == false);
        VERIFY_ARE_EQUAL(t.GetThreadsPerFile(), (DWORD)1);
        VERIFY_ARE_EQUAL(t.GetThreadStrideInBytes(), 0);
        VERIFY_IS_TRUE(t.GetCreateFile() == false);
        VERIFY_ARE_EQUAL(t.GetFileSize(), 0);
        VERIFY_ARE_EQUAL(t.GetMaxFileSize(), 0);
        VERIFY_ARE_EQUAL(t.GetWriteRatio(), (DWORD)84);
        VERIFY_IS_TRUE(t.GetUseBurstSize() == false);
        VERIFY_ARE_EQUAL(t.GetBurstSize(), (DWORD)0);
        VERIFY_ARE_EQUAL(t.GetThinkTime(), (DWORD)0);
        VERIFY_IS_TRUE(t.GetEnableThinkTime() == false);
        VERIFY_IS_TRUE(t.GetSequentialScanHint() == false);
        VERIFY_IS_TRUE(t.GetRandomAccessHint() == false);
        VERIFY_IS_TRUE(t.GetTemporaryFileHint() == false);
        VERIFY_IS_TRUE(t.GetUseLargePages() == false);
        VERIFY_ARE_EQUAL(t.GetThroughputInBytesPerMillisecond(), (DWORD)0);
        VERIFY_ARE_EQUAL(t.GetRandomDataWriteBufferSize(), 0);
        VERIFY_ARE_EQUAL(t.GetRandomDataWriteBufferSourcePath(), "");
    }

    void CmdLineParserUnitTests::TestGetRandomDataWriteBufferData()
    {
        CmdLineParser p;
        UINT64 cb;
        string sPath;

        p._GetRandomDataWriteBufferData("11332", cb, sPath);
        VERIFY_ARE_EQUAL(cb, 11332);
        VERIFY_IS_TRUE(sPath == "");

        cb = 0;
        sPath = "";
        p._GetRandomDataWriteBufferData("11332,", cb, sPath);
        VERIFY_ARE_EQUAL(cb, 11332);
        VERIFY_IS_TRUE(sPath == "");

        cb = 0;
        sPath = "";
        p._GetRandomDataWriteBufferData("11332,x:\\foo\\bar", cb, sPath);
        VERIFY_ARE_EQUAL(cb, 11332);
        VERIFY_IS_TRUE(sPath == "x:\\foo\\bar");

        cb = 0;
        sPath = "";
        p._GetRandomDataWriteBufferData("2M", cb, sPath);
        VERIFY_ARE_EQUAL(cb, 2 * 1024 * 1024);
        VERIFY_IS_TRUE(sPath == "");

        cb = 0;
        sPath = "";
        p._GetRandomDataWriteBufferData("2M,x:\\foo\\bar", cb, sPath);
        VERIFY_ARE_EQUAL(cb, 2 * 1024 * 1024);
        VERIFY_IS_TRUE(sPath == "x:\\foo\\bar");
    }

    void CmdLineParserUnitTests::TestParseCmdLineWriteBufferContentRandomNoFilePath()
    {
        CmdLineParser p;
        Profile profile;
        struct Synchronization s = {};
        const char *argv[] = { "foo", "-b128K", "-w84", "-Z2M", "testfile.dat" };
        VERIFY_IS_TRUE(p.ParseCmdLine(_countof(argv), argv, &profile, &s) == true);

        vector<TimeSpan> vSpans(profile.GetTimeSpans());
        vector<Target> vTargets(vSpans[0].GetTargets());
        VERIFY_ARE_EQUAL(vTargets.size(), (size_t)1);
        Target t(vTargets[0]);

        VERIFY_IS_FALSE(t.GetZeroWriteBuffers());
        VERIFY_ARE_EQUAL(t.GetRandomDataWriteBufferSize(), (size_t)(2 * 1024 * 1024));
        VERIFY_IS_TRUE(t.GetRandomDataWriteBufferSourcePath() == "");
    }

    void CmdLineParserUnitTests::TestParseCmdLineWriteBufferContentRandomWithFilePath()
    {
        CmdLineParser p;
        Profile profile;
        struct Synchronization s = {};
        const char *argv[] = { "foo", "-b128K", "-w84", "-Z3M,x:\\foo\\bar.baz", "testfile.dat" };
        VERIFY_IS_TRUE(p.ParseCmdLine(_countof(argv), argv, &profile, &s) == true);

        vector<TimeSpan> vSpans(profile.GetTimeSpans());
        vector<Target> vTargets(vSpans[0].GetTargets());
        VERIFY_ARE_EQUAL(vTargets.size(), (size_t)1);
        Target t(vTargets[0]);

        VERIFY_IS_FALSE(t.GetZeroWriteBuffers());
        VERIFY_ARE_EQUAL(t.GetRandomDataWriteBufferSize(), (size_t)(3 * 1024 * 1024));
        VERIFY_IS_TRUE(t.GetRandomDataWriteBufferSourcePath() == "x:\\foo\\bar.baz");
    }

    void CmdLineParserUnitTests::TestParseCmdLineInterlockedSequential()
    {
        CmdLineParser p;
        Profile profile;
        struct Synchronization s = {};
        const char *argv[] = { "foo", "-b128K", "-w84", "-si", "testfile.dat" };
        VERIFY_IS_TRUE(p.ParseCmdLine(_countof(argv), argv, &profile, &s) == true);
        VERIFY_IS_TRUE(profile.GetVerbose() == false);
        VERIFY_IS_TRUE(profile.GetProgress() == 0);
        VERIFY_IS_TRUE(profile.GetCmdLine().compare("foo -b128K -w84 -si testfile.dat") == 0);
        VERIFY_IS_TRUE(profile.GetEtwEnabled() == false);
        VERIFY_IS_TRUE(profile.GetEtwProcess() == false);
        VERIFY_IS_TRUE(profile.GetEtwThread() == false);
        VERIFY_IS_TRUE(profile.GetEtwImageLoad() == false);
        VERIFY_IS_TRUE(profile.GetEtwDiskIO() == false);
        VERIFY_IS_TRUE(profile.GetEtwMemoryPageFaults() == false);
        VERIFY_IS_TRUE(profile.GetEtwMemoryHardFaults() == false);
        VERIFY_IS_TRUE(profile.GetEtwNetwork() == false);
        VERIFY_IS_TRUE(profile.GetEtwRegistry() == false);
        VERIFY_IS_TRUE(profile.GetEtwUsePagedMemory() == false);
        VERIFY_IS_TRUE(profile.GetEtwUsePerfTimer() == false);
        VERIFY_IS_TRUE(profile.GetEtwUseSystemTimer() == false);
        VERIFY_IS_TRUE(profile.GetEtwUseCyclesCounter() == false);

        vector<TimeSpan> vSpans(profile.GetTimeSpans());
        VERIFY_ARE_EQUAL(vSpans.size(), (size_t)1);
        VERIFY_ARE_EQUAL(vSpans[0].GetDuration(), (UINT32)10);
        VERIFY_ARE_EQUAL(vSpans[0].GetWarmup(), (UINT32)5);
        VERIFY_ARE_EQUAL(vSpans[0].GetCooldown(), (UINT32)0);
        VERIFY_ARE_EQUAL(vSpans[0].GetRandSeed(), (UINT32)0);
        VERIFY_ARE_EQUAL(vSpans[0].GetThreadCount(), (DWORD)0);
        VERIFY_ARE_EQUAL(vSpans[0].GetRequestCount(), (DWORD)0);
        VERIFY_IS_TRUE(vSpans[0].GetDisableAffinity() == false);
        VERIFY_IS_TRUE(vSpans[0].GetCompletionRoutines() == false);
        VERIFY_IS_TRUE(vSpans[0].GetMeasureLatency() == false);
        VERIFY_IS_TRUE(vSpans[0].GetRandomWriteData() == false);
        VERIFY_IS_TRUE(vSpans[0].GetUseIoRing() == false);

        const auto& vMasks(vSpans[0].GetAffinityGroupMasks());
        VERIFY_ARE_EQUAL(vMasks.size(), (size_t)0);

        vector<Target> vTargets(vSpans[0].GetTargets());
        VERIFY_ARE_EQUAL(vTargets.size(), (size_t)1);
        Target t(vTargets[0]);

        VERIFY_IS_TRUE(t.GetPath().compare("testfile.dat") == 0);
        VERIFY_ARE_EQUAL(t.GetBlockSizeInBytes(), (DWORD)(128 * 1024));
        VERIFY_ARE_EQUAL(t.GetRequestCount(), (DWORD)2);
        VERIFY_IS_TRUE(t.GetRandomRatio() == 0);
        VERIFY_IS_TRUE(t.GetUseInterlockedSequential() == true);
        VERIFY_ARE_EQUAL(t.GetBlockAlignmentInBytes(), t.GetBlockSizeInBytes());
        VERIFY_ARE_EQUAL(t.GetBaseFileOffsetInBytes(), 0);
        VERIFY_IS_TRUE(t.GetUseParallelAsyncIO() == false);
        VERIFY_IS_TRUE(t.GetCacheMode() == TargetCacheMode::Cached);
        VERIFY_IS_TRUE(t.GetWriteThroughMode() == WriteThroughMode::Off);
        VERIFY_IS_TRUE(t.GetMemoryMappedIoMode() == MemoryMappedIoMode::Off);
        VERIFY_IS_TRUE(t.GetMemoryMappedIoFlushMode() == MemoryMappedIoFlushMode::Undefined);
        VERIFY_IS_TRUE(t.GetBypassIoMode() == BypassIoMode::Undefined);
        VERIFY_IS_TRUE(t.GetZeroWriteBuffers() == false);
        VERIFY_ARE_EQUAL(t.GetThreadsPerFile(), (DWORD)1);
        VERIFY_ARE_EQUAL(t.GetThreadStrideInBytes(), 0);
        VERIFY_IS_TRUE(t.GetCreateFile() == false);
        VERIFY_ARE_EQUAL(t.GetFileSize(), 0);
        VERIFY_ARE_EQUAL(t.GetMaxFileSize(), 0);
        VERIFY_ARE_EQUAL(t.GetWriteRatio(), (DWORD)84);
        VERIFY_IS_TRUE(t.GetUseBurstSize() == false);
        VERIFY_ARE_EQUAL(t.GetBurstSize(), (DWORD)0);
        VERIFY_ARE_EQUAL(t.GetThinkTime(), (DWORD)0);
        VERIFY_IS_TRUE(t.GetEnableThinkTime() == false);
        VERIFY_IS_TRUE(t.GetSequentialScanHint() == false);
        VERIFY_IS_TRUE(t.GetRandomAccessHint() == false);
        VERIFY_IS_TRUE(t.GetTemporaryFileHint() == false);
        VERIFY_IS_TRUE(t.GetUseLargePages() == false);
        VERIFY_ARE_EQUAL(t.GetThroughputInBytesPerMillisecond(), (DWORD)0);
    }

    void CmdLineParserUnitTests::TestParseCmdLineInterlockedSequentialWithStride()
    {
        CmdLineParser p;
        Profile profile;
        struct Synchronization s = {};
        const char *argv[] = { "foo", "-b128K", "-w84", "-si567K", "testfile.dat" };
        VERIFY_IS_TRUE(p.ParseCmdLine(_countof(argv), argv, &profile, &s) == true);
        VERIFY_IS_TRUE(profile.GetVerbose() == false);
        VERIFY_IS_TRUE(profile.GetProgress() == 0);
        VERIFY_IS_TRUE(profile.GetCmdLine().compare("foo -b128K -w84 -si567K testfile.dat") == 0);
        VERIFY_IS_TRUE(profile.GetEtwEnabled() == false);
        VERIFY_IS_TRUE(profile.GetEtwProcess() == false);
        VERIFY_IS_TRUE(profile.GetEtwThread() == false);
        VERIFY_IS_TRUE(profile.GetEtwImageLoad() == false);
        VERIFY_IS_TRUE(profile.GetEtwDiskIO() == false);
        VERIFY_IS_TRUE(profile.GetEtwMemoryPageFaults() == false);
        VERIFY_IS_TRUE(profile.GetEtwMemoryHardFaults() == false);
        VERIFY_IS_TRUE(profile.GetEtwNetwork() == false);
        VERIFY_IS_TRUE(profile.GetEtwRegistry() == false);
        VERIFY_IS_TRUE(profile.GetEtwUsePagedMemory() == false);
        VERIFY_IS_TRUE(profile.GetEtwUsePerfTimer() == false);
        VERIFY_IS_TRUE(profile.GetEtwUseSystemTimer() == false);
        VERIFY_IS_TRUE(profile.GetEtwUseCyclesCounter() == false);

        vector<TimeSpan> vSpans(profile.GetTimeSpans());
        VERIFY_ARE_EQUAL(vSpans.size(), (size_t)1);
        VERIFY_ARE_EQUAL(vSpans[0].GetDuration(), (UINT32)10);
        VERIFY_ARE_EQUAL(vSpans[0].GetWarmup(), (UINT32)5);
        VERIFY_ARE_EQUAL(vSpans[0].GetCooldown(), (UINT32)0);
        VERIFY_ARE_EQUAL(vSpans[0].GetRandSeed(), (UINT32)0);
        VERIFY_ARE_EQUAL(vSpans[0].GetThreadCount(), (DWORD)0);
        VERIFY_ARE_EQUAL(vSpans[0].GetRequestCount(), (DWORD)0);
        VERIFY_IS_TRUE(vSpans[0].GetDisableAffinity() == false);
        VERIFY_IS_TRUE(vSpans[0].GetCompletionRoutines() == false);
        VERIFY_IS_TRUE(vSpans[0].GetMeasureLatency() == false);
        VERIFY_IS_TRUE(vSpans[0].GetRandomWriteData() == false);
        VERIFY_IS_TRUE(vSpans[0].GetUseIoRing() == false);

        const auto& vMasks(vSpans[0].GetAffinityGroupMasks());
        VERIFY_ARE_EQUAL(vMasks.size(), (size_t)0);

        vector<Target> vTargets(vSpans[0].GetTargets());
        VERIFY_ARE_EQUAL(vTargets.size(), (size_t)1);
        Target t(vTargets[0]);

        VERIFY_IS_TRUE(t.GetPath().compare("testfile.dat") == 0);
        VERIFY_ARE_EQUAL(t.GetBlockSizeInBytes(), (DWORD)(128 * 1024));
        VERIFY_ARE_EQUAL(t.GetRequestCount(), (DWORD)2);
        VERIFY_IS_TRUE(t.GetRandomRatio() == 0);
        VERIFY_IS_TRUE(t.GetUseInterlockedSequential() == true);
        VERIFY_ARE_EQUAL(t.GetBlockAlignmentInBytes(), (567 * 1024));
        VERIFY_ARE_EQUAL(t.GetBaseFileOffsetInBytes(), 0);
        VERIFY_IS_TRUE(t.GetUseParallelAsyncIO() == false);
        VERIFY_IS_TRUE(t.GetCacheMode() == TargetCacheMode::Cached);
        VERIFY_IS_TRUE(t.GetWriteThroughMode() == WriteThroughMode::Off);
        VERIFY_IS_TRUE(t.GetMemoryMappedIoMode() == MemoryMappedIoMode::Off);
        VERIFY_IS_TRUE(t.GetMemoryMappedIoFlushMode() == MemoryMappedIoFlushMode::Undefined);
        VERIFY_IS_TRUE(t.GetBypassIoMode() == BypassIoMode::Undefined);
        VERIFY_IS_TRUE(t.GetZeroWriteBuffers() == false);
        VERIFY_ARE_EQUAL(t.GetThreadsPerFile(), (DWORD)1);
        VERIFY_ARE_EQUAL(t.GetThreadStrideInBytes(), 0);
        VERIFY_IS_TRUE(t.GetCreateFile() == false);
        VERIFY_ARE_EQUAL(t.GetFileSize(), 0);
        VERIFY_ARE_EQUAL(t.GetMaxFileSize(), 0);
        VERIFY_ARE_EQUAL(t.GetWriteRatio(), (DWORD)84);
        VERIFY_IS_TRUE(t.GetUseBurstSize() == false);
        VERIFY_ARE_EQUAL(t.GetBurstSize(), (DWORD)0);
        VERIFY_ARE_EQUAL(t.GetThinkTime(), (DWORD)0);
        VERIFY_IS_TRUE(t.GetEnableThinkTime() == false);
        VERIFY_IS_TRUE(t.GetSequentialScanHint() == false);
        VERIFY_IS_TRUE(t.GetRandomAccessHint() == false);
        VERIFY_IS_TRUE(t.GetTemporaryFileHint() == false);
        VERIFY_IS_TRUE(t.GetUseLargePages() == false);
        VERIFY_ARE_EQUAL(t.GetThroughputInBytesPerMillisecond(), (DWORD)0);
    }

    void CmdLineParserUnitTests::TestParseCmdLineTotalThreadCountAndTotalRequestCount()
    {
        CmdLineParser p;
        Profile profile;
        struct Synchronization s = {};
        const char *argv[] = { "foo", "-b128K", "-w84", "-F23", "-O8", "testfile.dat" };
        VERIFY_IS_TRUE(p.ParseCmdLine(_countof(argv), argv, &profile, &s) == true);
        VERIFY_IS_TRUE(profile.GetVerbose() == false);
        VERIFY_IS_TRUE(profile.GetProgress() == 0);
        VERIFY_IS_TRUE(profile.GetCmdLine().compare("foo -b128K -w84 -F23 -O8 testfile.dat") == 0);
        VERIFY_IS_TRUE(profile.GetEtwEnabled() == false);
        VERIFY_IS_TRUE(profile.GetEtwProcess() == false);
        VERIFY_IS_TRUE(profile.GetEtwThread() == false);
        VERIFY_IS_TRUE(profile.GetEtwImageLoad() == false);
        VERIFY_IS_TRUE(profile.GetEtwDiskIO() == false);
        VERIFY_IS_TRUE(profile.GetEtwMemoryPageFaults() == false);
        VERIFY_IS_TRUE(profile.GetEtwMemoryHardFaults() == false);
        VERIFY_IS_TRUE(profile.GetEtwNetwork() == false);
        VERIFY_IS_TRUE(profile.GetEtwRegistry() == false);
        VERIFY_IS_TRUE(profile.GetEtwUsePagedMemory() == false);
        VERIFY_IS_TRUE(profile.GetEtwUsePerfTimer() == false);
        VERIFY_IS_TRUE(profile.GetEtwUseSystemTimer() == false);
        VERIFY_IS_TRUE(profile.GetEtwUseCyclesCounter() == false);

        vector<TimeSpan> vSpans(profile.GetTimeSpans());
        VERIFY_ARE_EQUAL(vSpans.size(), (size_t)1);
        VERIFY_ARE_EQUAL(vSpans[0].GetDuration(), (UINT32)10);
        VERIFY_ARE_EQUAL(vSpans[0].GetWarmup(), (UINT32)5);
        VERIFY_ARE_EQUAL(vSpans[0].GetCooldown(), (UINT32)0);
        VERIFY_ARE_EQUAL(vSpans[0].GetRandSeed(), (UINT32)0);
        VERIFY_ARE_EQUAL(vSpans[0].GetThreadCount(), (DWORD)23);
        VERIFY_ARE_EQUAL(vSpans[0].GetRequestCount(), (DWORD)8);
        VERIFY_IS_TRUE(vSpans[0].GetDisableAffinity() == false);
        VERIFY_IS_TRUE(vSpans[0].GetCompletionRoutines() == false);
        VERIFY_IS_TRUE(vSpans[0].GetMeasureLatency() == false);
        VERIFY_IS_TRUE(vSpans[0].GetRandomWriteData() == false);
        VERIFY_IS_TRUE(vSpans[0].GetUseIoRing() == false);

        const auto& vMasks(vSpans[0].GetAffinityGroupMasks());
        VERIFY_ARE_EQUAL(vMasks.size(), (size_t)0);

        vector<Target> vTargets(vSpans[0].GetTargets());
        VERIFY_ARE_EQUAL(vTargets.size(), (size_t)1);
        Target t(vTargets[0]);

        VERIFY_IS_TRUE(t.GetPath().compare("testfile.dat") == 0);
        VERIFY_ARE_EQUAL(t.GetBlockSizeInBytes(), (DWORD)(128 * 1024));
        VERIFY_ARE_EQUAL(t.GetRequestCount(), (DWORD)2);
        VERIFY_IS_TRUE(t.GetRandomRatio() == 0);
        VERIFY_IS_TRUE(t.GetUseInterlockedSequential() == false);
        VERIFY_ARE_EQUAL(t.GetBlockAlignmentInBytes(), t.GetBlockSizeInBytes());
        VERIFY_ARE_EQUAL(t.GetBaseFileOffsetInBytes(), 0);
        VERIFY_IS_TRUE(t.GetUseParallelAsyncIO() == false);
        VERIFY_IS_TRUE(t.GetCacheMode() == TargetCacheMode::Cached);
        VERIFY_IS_TRUE(t.GetWriteThroughMode() == WriteThroughMode::Off);
        VERIFY_IS_TRUE(t.GetMemoryMappedIoMode() == MemoryMappedIoMode::Off);
        VERIFY_IS_TRUE(t.GetMemoryMappedIoFlushMode() == MemoryMappedIoFlushMode::Undefined);
        VERIFY_IS_TRUE(t.GetBypassIoMode() == BypassIoMode::Undefined);
        VERIFY_IS_TRUE(t.GetZeroWriteBuffers() == false);
        VERIFY_ARE_EQUAL(t.GetThreadsPerFile(), (DWORD)1);
        VERIFY_ARE_EQUAL(t.GetThreadStrideInBytes(), 0);
        VERIFY_IS_TRUE(t.GetCreateFile() == false);
        VERIFY_ARE_EQUAL(t.GetFileSize(), 0);
        VERIFY_ARE_EQUAL(t.GetMaxFileSize(), 0);
        VERIFY_ARE_EQUAL(t.GetWriteRatio(), (DWORD)84);
        VERIFY_IS_TRUE(t.GetUseBurstSize() == false);
        VERIFY_ARE_EQUAL(t.GetBurstSize(), (DWORD)0);
        VERIFY_ARE_EQUAL(t.GetThinkTime(), (DWORD)0);
        VERIFY_IS_TRUE(t.GetEnableThinkTime() == false);
        VERIFY_IS_TRUE(t.GetSequentialScanHint() == false);
        VERIFY_IS_TRUE(t.GetRandomAccessHint() == false);
        VERIFY_IS_TRUE(t.GetTemporaryFileHint() == false);
        VERIFY_IS_TRUE(t.GetUseLargePages() == false);
        VERIFY_ARE_EQUAL(t.GetThroughputInBytesPerMillisecond(), (DWORD)0);
    }

    void CmdLineParserUnitTests::TestParseCmdLineThroughput()
    {
        CmdLineParser p;
        struct Synchronization s = {};

        {
            Profile profile;
            const char *argv[] = { "foo", "-b128K", "-g10i", "testfile.dat" };
            VERIFY_IS_TRUE(p.ParseCmdLine(_countof(argv), argv, &profile, &s) == true);
            VERIFY_ARE_EQUAL(profile.GetTimeSpans()[0].GetTargets()[0].GetThroughputInBytesPerMillisecond(), (DWORD)((128*1024*10)/1000));
            VERIFY_IS_TRUE(profile.GetTimeSpans()[0].GetTargets()[0].GetThroughputIOPS() == 10);
        }
        {
            Profile profile;
            const char *argv[] = { "foo", "-b128K", "-g1024", "testfile.dat" };
            VERIFY_IS_TRUE(p.ParseCmdLine(_countof(argv), argv, &profile, &s) == true);
            VERIFY_IS_TRUE(profile.GetTimeSpans()[0].GetTargets()[0].GetThroughputInBytesPerMillisecond() == 1024);
            VERIFY_IS_TRUE(profile.GetTimeSpans()[0].GetTargets()[0].GetThroughputIOPS() == 0);
        }

        // Invalid cases: valid unit on wrong side, no digits, zeroes, bad unit

        {
            Profile profile;
            const char *argv[] = { "foo", "-b128K", "-gi100", "testfile.dat" };
            VERIFY_IS_TRUE(p.ParseCmdLine(_countof(argv), argv, &profile, &s) == false);
        }
        {
            Profile profile;
            const char *argv[] = { "foo", "-b128K", "-g0", "testfile.dat" };
            VERIFY_IS_TRUE(p.ParseCmdLine(_countof(argv), argv, &profile, &s) == false);
        }
        {
            Profile profile;
            const char *argv[] = { "foo", "-b128K", "-g0i", "testfile.dat" };
            VERIFY_IS_TRUE(p.ParseCmdLine(_countof(argv), argv, &profile, &s) == false);
        }
        {
            Profile profile;
            const char *argv[] = { "foo", "-b128K", "-g", "testfile.dat" };
            VERIFY_IS_TRUE(p.ParseCmdLine(_countof(argv), argv, &profile, &s) == false);
        }
        {
            Profile profile;
            const char *argv[] = { "foo", "-b128K", "-gi", "testfile.dat" };
            VERIFY_IS_TRUE(p.ParseCmdLine(_countof(argv), argv, &profile, &s) == false);
        }
        {
            Profile profile;
            const char *argv[] = { "foo", "-b128K", "-g100x", "testfile.dat" };
            VERIFY_IS_TRUE(p.ParseCmdLine(_countof(argv), argv, &profile, &s) == false);
        }
    }

    void CmdLineParserUnitTests::TestParseCmdLineRandomSequentialMixed()
    {
        // Coverage for -rs and combinations of conflicts with -r/-s/-rs

        CmdLineParser p;
        struct Synchronization s = {};

        //
        // -rs cases
        //

        // Isolated

        {
            Profile profile;
            const char *argv[] = { "foo", "-rs50", "testfile.dat" };
            VERIFY_IS_TRUE(p.ParseCmdLine(_countof(argv), argv, &profile, &s) == true);
            VERIFY_ARE_EQUAL(profile.GetTimeSpans()[0].GetTargets()[0].GetRandomRatio(), (UINT32) 50);
        }

        // Combined with -r<value>, in any order

        {
            Profile profile;
            const char *argv[] = { "foo", "-rs50", "-r8k", "testfile.dat" };
            VERIFY_IS_TRUE(p.ParseCmdLine(_countof(argv), argv, &profile, &s) == true);
            VERIFY_ARE_EQUAL(profile.GetTimeSpans()[0].GetTargets()[0].GetRandomRatio(), (UINT32) 50);
            VERIFY_ARE_EQUAL(profile.GetTimeSpans()[0].GetTargets()[0].GetBlockAlignmentInBytes(), 8*1024);
        }
        {
            Profile profile;
            const char *argv[] = { "foo", "-r8k", "-rs50", "testfile.dat" };
            VERIFY_IS_TRUE(p.ParseCmdLine(_countof(argv), argv, &profile, &s) == true);
            VERIFY_ARE_EQUAL(profile.GetTimeSpans()[0].GetTargets()[0].GetRandomRatio(), (UINT32) 50);
            VERIFY_ARE_EQUAL(profile.GetTimeSpans()[0].GetTargets()[0].GetBlockAlignmentInBytes(), 8*1024);
        }

        // Combined with -r, in any order (don't care block size)
        // While the -r in this order has no effect and could be flagged as an error, we don't currently parse to this level

        {
            Profile profile;
            const char *argv[] = { "foo", "-rs50", "-r", "testfile.dat" };
            VERIFY_IS_TRUE(p.ParseCmdLine(_countof(argv), argv, &profile, &s) == true);
            VERIFY_ARE_EQUAL(profile.GetTimeSpans()[0].GetTargets()[0].GetRandomRatio(), (UINT32) 50);
        }
        {
            Profile profile;
            const char *argv[] = { "foo", "-r", "-rs50", "testfile.dat" };
            VERIFY_IS_TRUE(p.ParseCmdLine(_countof(argv), argv, &profile, &s) == true);
            VERIFY_ARE_EQUAL(profile.GetTimeSpans()[0].GetTargets()[0].GetRandomRatio(), (UINT32) 50);
        }

        // Now for conflict cases

        // Combined with -s<value> in any order

        {
            Profile profile;
            const char *argv[] = { "foo", "-rs50", "-s8k", "testfile.dat" };
            VERIFY_IS_TRUE(p.ParseCmdLine(_countof(argv), argv, &profile, &s) == false);
        }
        {
            Profile profile;
            const char *argv[] = { "foo", "-s8k", "-rs50", "testfile.dat" };
            VERIFY_IS_TRUE(p.ParseCmdLine(_countof(argv), argv, &profile, &s) == false);
        }

        // Combined with -s in any order

        {
            Profile profile;
            const char *argv[] = { "foo", "-rs50", "-s", "testfile.dat" };
            VERIFY_IS_TRUE(p.ParseCmdLine(_countof(argv), argv, &profile, &s) == false);
        }
        {
            Profile profile;
            const char *argv[] = { "foo", "-s", "-rs50", "testfile.dat" };
            VERIFY_IS_TRUE(p.ParseCmdLine(_countof(argv), argv, &profile, &s) == false);
        }

        // -r/-s conflict

        {
            Profile profile;
            const char *argv[] = { "foo", "-r", "-s", "testfile.dat" };
            VERIFY_IS_TRUE(p.ParseCmdLine(_countof(argv), argv, &profile, &s) == false);
        }
        {
            Profile profile;
            const char *argv[] = { "foo", "-s", "-r", "testfile.dat" };
            VERIFY_IS_TRUE(p.ParseCmdLine(_countof(argv), argv, &profile, &s) == false);
        }

        // 3-way conflict
        // If it were more important, we could/should enumerate the orderings
        {
            Profile profile;
            const char *argv[] = { "foo", "-rs50", "-s", "-r", "testfile.dat" };
            VERIFY_IS_TRUE(p.ParseCmdLine(_countof(argv), argv, &profile, &s) == false);
        }
        {
            Profile profile;
            const char *argv[] = { "foo", "-s", "-rs50", "-r", "testfile.dat" };
            VERIFY_IS_TRUE(p.ParseCmdLine(_countof(argv), argv, &profile, &s) == false);
        }
        {
            Profile profile;
            const char *argv[] = { "foo", "-s", "-r", "-rs50", "testfile.dat" };
            VERIFY_IS_TRUE(p.ParseCmdLine(_countof(argv), argv, &profile, &s) == false);
        }

        // 3-way with -s<value>
        {
            Profile profile;
            const char *argv[] = { "foo", "-rs50", "-s8k", "-r", "testfile.dat" };
            VERIFY_IS_TRUE(p.ParseCmdLine(_countof(argv), argv, &profile, &s) == false);
        }
        {
            Profile profile;
            const char *argv[] = { "foo", "-s8k", "-rs50", "-r", "testfile.dat" };
            VERIFY_IS_TRUE(p.ParseCmdLine(_countof(argv), argv, &profile, &s) == false);
        }
        {
            Profile profile;
            const char *argv[] = { "foo", "-s8k", "-r", "-rs50", "testfile.dat" };
            VERIFY_IS_TRUE(p.ParseCmdLine(_countof(argv), argv, &profile, &s) == false);
        }

        // 3-way with -r<value>
        {
            Profile profile;
            const char *argv[] = { "foo", "-rs50", "-s", "-r8k", "testfile.dat" };
            VERIFY_IS_TRUE(p.ParseCmdLine(_countof(argv), argv, &profile, &s) == false);
        }
        {
            Profile profile;
            const char *argv[] = { "foo", "-s", "-rs50", "-r8k", "testfile.dat" };
            VERIFY_IS_TRUE(p.ParseCmdLine(_countof(argv), argv, &profile, &s) == false);
        }
        {
            Profile profile;
            const char *argv[] = { "foo", "-s", "-r8k", "-rs50", "testfile.dat" };
            VERIFY_IS_TRUE(p.ParseCmdLine(_countof(argv), argv, &profile, &s) == false);
        }

        // Duplicated specs
        {
            Profile profile;
            const char *argv[] = { "foo", "-s", "-s8k", "testfile.dat" };
            VERIFY_IS_TRUE(p.ParseCmdLine(_countof(argv), argv, &profile, &s) == false);
        }
        {
            Profile profile;
            const char *argv[] = { "foo", "-s8k", "-s", "testfile.dat" };
            VERIFY_IS_TRUE(p.ParseCmdLine(_countof(argv), argv, &profile, &s) == false);
        }
        {
            Profile profile;
            const char *argv[] = { "foo", "-s", "-s", "testfile.dat" };
            VERIFY_IS_TRUE(p.ParseCmdLine(_countof(argv), argv, &profile, &s) == false);
        }
        {
            Profile profile;
            const char *argv[] = { "foo", "-s8k", "-s8k", "testfile.dat" };
            VERIFY_IS_TRUE(p.ParseCmdLine(_countof(argv), argv, &profile, &s) == false);
        }
        {
            Profile profile;
            const char *argv[] = { "foo", "-r", "-r8k", "testfile.dat" };
            VERIFY_IS_TRUE(p.ParseCmdLine(_countof(argv), argv, &profile, &s) == false);
        }
        {
            Profile profile;
            const char *argv[] = { "foo", "-r8k", "-r", "testfile.dat" };
            VERIFY_IS_TRUE(p.ParseCmdLine(_countof(argv), argv, &profile, &s) == false);
        }
        {
            Profile profile;
            const char *argv[] = { "foo", "-r", "-r", "testfile.dat" };
            VERIFY_IS_TRUE(p.ParseCmdLine(_countof(argv), argv, &profile, &s) == false);
        }
        {
            Profile profile;
            const char *argv[] = { "foo", "-r8k", "-r8k", "testfile.dat" };
            VERIFY_IS_TRUE(p.ParseCmdLine(_countof(argv), argv, &profile, &s) == false);
        }
        {
            Profile profile;
            const char *argv[] = { "foo", "-r8k", "-r4k", "testfile.dat" };
            VERIFY_IS_TRUE(p.ParseCmdLine(_countof(argv), argv, &profile, &s) == false);
        }

        // Bounds checks: -rs100 is OK w/wo random alignment and -rs0 is rejected
        {
            Profile profile;
            const char *argv[] = { "foo", "-rs100", "-r", "testfile.dat" };
            VERIFY_IS_TRUE(p.ParseCmdLine(_countof(argv), argv, &profile, &s) == true);
        }
        {
            Profile profile;
            const char *argv[] = { "foo", "-rs100", "-r8k", "testfile.dat" };
            VERIFY_IS_TRUE(p.ParseCmdLine(_countof(argv), argv, &profile, &s) == true);
        }
        {
            Profile profile;
            const char *argv[] = { "foo", "-r", "-rs100", "testfile.dat" };
            VERIFY_IS_TRUE(p.ParseCmdLine(_countof(argv), argv, &profile, &s) == true);
        }
        {
            Profile profile;
            const char *argv[] = { "foo", "-r8k", "-rs100", "testfile.dat" };
            VERIFY_IS_TRUE(p.ParseCmdLine(_countof(argv), argv, &profile, &s) == true);
        }
        {
            Profile profile;
            const char *argv[] = { "foo", "-rs0", "-r", "testfile.dat" };
            VERIFY_IS_TRUE(p.ParseCmdLine(_countof(argv), argv, &profile, &s) == false);
        }
        {
            Profile profile;
            const char *argv[] = { "foo", "-rs0", "-r8k", "testfile.dat" };
            VERIFY_IS_TRUE(p.ParseCmdLine(_countof(argv), argv, &profile, &s) == false);
        }
        {
            Profile profile;
            const char *argv[] = { "foo", "-rs0", "-s", "testfile.dat" };
            VERIFY_IS_TRUE(p.ParseCmdLine(_countof(argv), argv, &profile, &s) == false);
        }
        {
            Profile profile;
            const char *argv[] = { "foo", "-rs0", "-s8k", "testfile.dat" };
            VERIFY_IS_TRUE(p.ParseCmdLine(_countof(argv), argv, &profile, &s) == false);
        }
    }

    void CmdLineParserUnitTests::TestParseCmdLineTargetDistribution()
    {
        // coverage for parsing of cmdline target distributions (percent/absolute)

        CmdLineParser p;
        struct Synchronization s = {};

        //
        // Positive cases - these match the ResultParser UTs
        //

        {
            Profile profile;
            const char *argv[] = { "foo", "-rdpct10/10:10/10:0/10", "-r", "testfile.dat" };
            VERIFY_IS_TRUE(p.ParseCmdLine(_countof(argv), argv, &profile, &s));

            auto t = profile.GetTimeSpans()[0].GetTargets()[0]; // manage object lifetime so target is not destroyed
            auto dt = t.GetDistributionType();
            auto& v = t.GetDistributionRange();

            VERIFY_ARE_EQUAL(dt, DistributionType::Percent);
            VERIFY_ARE_EQUAL(v[0]._src, (UINT64) 0);
            VERIFY_ARE_EQUAL(v[0]._span, (UINT64) 10);
            VERIFY_ARE_EQUAL(v[0]._dst.first, (UINT64) 0);
            VERIFY_ARE_EQUAL(v[0]._dst.second, (UINT64) 10);
            VERIFY_ARE_EQUAL(v[1]._src, (UINT64) 10);           ///
            VERIFY_ARE_EQUAL(v[1]._span, (UINT64) 10);
            VERIFY_ARE_EQUAL(v[1]._dst.first, (UINT64) 10);
            VERIFY_ARE_EQUAL(v[1]._dst.second, (UINT64) 10);
            VERIFY_ARE_EQUAL(v[2]._src, (UINT64) 20);           ///
            VERIFY_ARE_EQUAL(v[2]._span, (UINT64) 0);
            VERIFY_ARE_EQUAL(v[2]._dst.first, (UINT64) 20);
            VERIFY_ARE_EQUAL(v[2]._dst.second, (UINT64) 10);
            VERIFY_ARE_EQUAL(v[3]._src, (UINT64) 20);           ///
            VERIFY_ARE_EQUAL(v[3]._span, (UINT64) 80);
            VERIFY_ARE_EQUAL(v[3]._dst.first, (UINT64) 30);
            VERIFY_ARE_EQUAL(v[3]._dst.second, (UINT64) 70);
        }
        {
            Profile profile;
            const char *argv[] = { "foo", "-rdabs10/1G:10/1G:0/100G", "-r", "testfile.dat" };
            VERIFY_IS_TRUE(p.ParseCmdLine(_countof(argv), argv, &profile, &s));

            auto t = profile.GetTimeSpans()[0].GetTargets()[0];
            auto dt = t.GetDistributionType();
            auto& v = t.GetDistributionRange();

            VERIFY_ARE_EQUAL(dt, DistributionType::Absolute);
            VERIFY_ARE_EQUAL(v[0]._src, (UINT64) 0);
            VERIFY_ARE_EQUAL(v[0]._span, (UINT64) 10);
            VERIFY_ARE_EQUAL(v[0]._dst.first, (UINT64) 0);
            VERIFY_ARE_EQUAL(v[0]._dst.second, (UINT64) 1*GB);
            VERIFY_ARE_EQUAL(v[1]._src, (UINT64) 10);           ///
            VERIFY_ARE_EQUAL(v[1]._span, (UINT64) 10);
            VERIFY_ARE_EQUAL(v[1]._dst.first, (UINT64) 1*GB);
            VERIFY_ARE_EQUAL(v[1]._dst.second, (UINT64) 1*GB);
            VERIFY_ARE_EQUAL(v[2]._src, (UINT64) 20);           ///
            VERIFY_ARE_EQUAL(v[2]._span, (UINT64) 0);
            VERIFY_ARE_EQUAL(v[2]._dst.first, (UINT64) 2*GB);
            VERIFY_ARE_EQUAL(v[2]._dst.second, (UINT64) 100*GB);
            VERIFY_ARE_EQUAL(v[3]._src, (UINT64) 20);           ///
            VERIFY_ARE_EQUAL(v[3]._span, (UINT64) 80);
            VERIFY_ARE_EQUAL(v[3]._dst.first, (UINT64) 102*GB);
            VERIFY_ARE_EQUAL(v[3]._dst.second, (UINT64) 0);
        }

        // valid with mixed load
        {
            Profile profile;
            const char *argv[] = { "foo", "-rdpct90/10", "-rs50", "-r8k", "testfile.dat" };
            VERIFY_IS_TRUE(p.ParseCmdLine(_countof(argv), argv, &profile, &s));
        }

        //
        // Negative cases
        //

        // not valid with sequential load
        {
            Profile profile;
            const char *argv[] = { "foo", "-rdpct90/10", "-s", "testfile.dat" };
            VERIFY_IS_FALSE(p.ParseCmdLine(_countof(argv), argv, &profile, &s));
        }
        {
            Profile profile;
            const char *argv[] = { "foo", "-rdpct90/10", "testfile.dat" };
            VERIFY_IS_FALSE(p.ParseCmdLine(_countof(argv), argv, &profile, &s));
        }

        // no/invalid distribution
        {
            Profile profile;
            const char *argv[] = { "foo", "-rd", "-r", "testfile.dat" };
            VERIFY_IS_FALSE(p.ParseCmdLine(_countof(argv), argv, &profile, &s));
        }
        {
            Profile profile;
            const char *argv[] = { "foo", "-rdfoo", "-r", "testfile.dat" };
            VERIFY_IS_FALSE(p.ParseCmdLine(_countof(argv), argv, &profile, &s));
        }
        {
            Profile profile;
            const char *argv[] = { "foo", "-rdfoo10/1G:10/1G:0/100G", "-r", "testfile.dat" };
            VERIFY_IS_FALSE(p.ParseCmdLine(_countof(argv), argv, &profile, &s));
        }

        // bad ints in first pos
        {
            Profile profile;
            const char *argv[] = { "foo", "-rdpctBAD", "-r", "testfile.dat" };
            VERIFY_IS_FALSE(p.ParseCmdLine(_countof(argv), argv, &profile, &s));
        }
        {
            Profile profile;
            const char *argv[] = { "foo", "-rdabsBAD", "-r", "testfile.dat" };
            VERIFY_IS_FALSE(p.ParseCmdLine(_countof(argv), argv, &profile, &s));
        }

        // good int, no sep
        {
            Profile profile;
            const char *argv[] = { "foo", "-rdpct10", "-r", "testfile.dat" };
            VERIFY_IS_FALSE(p.ParseCmdLine(_countof(argv), argv, &profile, &s));
        }
        {
            Profile profile;
            const char *argv[] = { "foo", "-rdabs10", "-r", "testfile.dat" };
            VERIFY_IS_FALSE(p.ParseCmdLine(_countof(argv), argv, &profile, &s));
        }

        // good int, bad sep
        {
            Profile profile;
            const char *argv[] = { "foo", "-rdpct10[", "-r", "testfile.dat" };
            VERIFY_IS_FALSE(p.ParseCmdLine(_countof(argv), argv, &profile, &s));
        }
        {
            Profile profile;
            const char *argv[] = { "foo", "-rdabs10[", "-r", "testfile.dat" };
            VERIFY_IS_FALSE(p.ParseCmdLine(_countof(argv), argv, &profile, &s));
        }

        // good int, good sep, no int
        {
            Profile profile;
            const char *argv[] = { "foo", "-rdpct10/", "-r", "testfile.dat" };
            VERIFY_IS_FALSE(p.ParseCmdLine(_countof(argv), argv, &profile, &s));
        }
        {
            Profile profile;
            const char *argv[] = { "foo", "-rdabs10/", "-r", "testfile.dat" };
            VERIFY_IS_FALSE(p.ParseCmdLine(_countof(argv), argv, &profile, &s));
        }

        // good int, good sep, bad int
        {
            Profile profile;
            const char *argv[] = { "foo", "-rdpct10/BAD", "-r", "testfile.dat" };
            VERIFY_IS_FALSE(p.ParseCmdLine(_countof(argv), argv, &profile, &s));
        }
        {
            Profile profile;
            const char *argv[] = { "foo", "-rdabs10/BAD", "-r", "testfile.dat" };
            VERIFY_IS_FALSE(p.ParseCmdLine(_countof(argv), argv, &profile, &s));
        }

        // good int, good sep, good int, bad sep
        {
            Profile profile;
            const char *argv[] = { "foo", "-rdpct10/10[", "-r", "testfile.dat" };
            VERIFY_IS_FALSE(p.ParseCmdLine(_countof(argv), argv, &profile, &s));
        }
        {
            Profile profile;
            const char *argv[] = { "foo", "-rdabs10/10g[", "-r", "testfile.dat" };        // detail - abs range > blocksize in order to be OK
            VERIFY_IS_FALSE(p.ParseCmdLine(_countof(argv), argv, &profile, &s));
        }

        // good int, good sep, good int, good sep, no int
        {
            Profile profile;
            const char *argv[] = { "foo", "-rdpct10/10:", "-r", "testfile.dat" };
            VERIFY_IS_FALSE(p.ParseCmdLine(_countof(argv), argv, &profile, &s));
        }
        {
            Profile profile;
            const char *argv[] = { "foo", "-rdabs10/10g:", "-r", "testfile.dat" };        // detail - abs range > blocksize in order to be OK
            VERIFY_IS_FALSE(p.ParseCmdLine(_countof(argv), argv, &profile, &s));
        }

        // good int, good sep, good int, good sep, bad int
        {
            Profile profile;
            const char *argv[] = { "foo", "-rdpct10/10:BAD", "-r", "testfile.dat" };
            VERIFY_IS_FALSE(p.ParseCmdLine(_countof(argv), argv, &profile, &s));
        }
        {
            Profile profile;
            const char *argv[] = { "foo", "-rdabs10/10g:BAD", "-r", "testfile.dat" };     // detail - abs range > blocksize in order to be OK
            VERIFY_IS_FALSE(p.ParseCmdLine(_countof(argv), argv, &profile, &s));
        }

        //
        // Negative cases for IO%
        //

        // a single pct cannot be > 100, and cannot sum > 100
        {
            Profile profile;
            const char *argv[] = { "foo", "-rdpct101/10", "-r", "testfile.dat" };
            VERIFY_IS_FALSE(p.ParseCmdLine(_countof(argv), argv, &profile, &s));
        }
        {
            Profile profile;
            const char *argv[] = { "foo", "-rdpct60/10:60/10", "-r", "testfile.dat" };
            VERIFY_IS_FALSE(p.ParseCmdLine(_countof(argv), argv, &profile, &s));
        }
        {
            Profile profile;
            const char *argv[] = { "foo", "-rdabs101/10g", "-r", "testfile.dat" };
            VERIFY_IS_FALSE(p.ParseCmdLine(_countof(argv), argv, &profile, &s));
        }
        {
            Profile profile;
            const char *argv[] = { "foo", "-rdabs60/10g:60/10g", "-r", "testfile.dat" };
            VERIFY_IS_FALSE(p.ParseCmdLine(_countof(argv), argv, &profile, &s));
        }

        // target% cannot be covered before IO% is covered
        {
            Profile profile;
            const char *argv[] = { "foo", "-rdpct10/50:10/50", "-r", "testfile.dat" };
            VERIFY_IS_FALSE(p.ParseCmdLine(_countof(argv), argv, &profile, &s));
        }

        // target% cannot be > 100
        {
            Profile profile;
            const char *argv[] = { "foo", "-rdpct10/50:10/60", "-r", "testfile.dat" };
            VERIFY_IS_FALSE(p.ParseCmdLine(_countof(argv), argv, &profile, &s));
        }

        //
        // Negative cases for Target%/Size
        //

        // a target%/size cannot be zero (first/second positions)
        {
            Profile profile;
            const char *argv[] = { "foo", "-rdpct10/0", "-r", "testfile.dat" };
            VERIFY_IS_FALSE(p.ParseCmdLine(_countof(argv), argv, &profile, &s));
        }
        {
            Profile profile;
            const char *argv[] = { "foo", "-rdpct60/10:10/0", "-r", "testfile.dat" };
            VERIFY_IS_FALSE(p.ParseCmdLine(_countof(argv), argv, &profile, &s));
        }
        {
            Profile profile;
            const char *argv[] = { "foo", "-rdabs10/0", "-r", "testfile.dat" };
            VERIFY_IS_FALSE(p.ParseCmdLine(_countof(argv), argv, &profile, &s));
        }
        {
            Profile profile;
            const char *argv[] = { "foo", "-rdabs60/10g:10/0", "-r", "testfile.dat" };
            VERIFY_IS_FALSE(p.ParseCmdLine(_countof(argv), argv, &profile, &s));
        }

        //
        // Negative cases for abs
        //

        // abs range must be >= blocksize
        {
            Profile profile;
            const char *argv[] = { "foo", "-rdabs10/10", "-r", "testfile.dat" };
            VERIFY_IS_FALSE(p.ParseCmdLine(_countof(argv), argv, &profile, &s));
        }
    }

    void CmdLineParserUnitTests::TestParseCmdLineResultOutput()
    {
        // Cases for combinations of -R[p][text|xml]

        CmdLineParser p;
        struct Synchronization s = {};

        char *aType[] = { "text", "xml" };
        vector<char *> vType(aType, &aType[0] + _countof(aType));
        char *aProfile[] = { "-R", "-Rp" };
        vector<char *> vProfile(aProfile, &aProfile[0] + _countof(aProfile));

        // combinations of p/text|xml OK
        for (auto ty : vType)
        {
            for (auto pr : vProfile)
            {
                string str = pr;
                str += ty;

                Profile profile;
                const char *argv[] = { "foo", str.c_str(), "testfile.dat" };
                fprintf(stderr, "case: %s\n", str.c_str());
                VERIFY_IS_TRUE(p.ParseCmdLine(_countof(argv), argv, &profile, &s));

                if (pr[2] == 'p')
                {
                    VERIFY_IS_TRUE(profile.GetProfileOnly());
                }
                else
                {
                    VERIFY_IS_FALSE(profile.GetProfileOnly());
                }

                if (*ty == 't')
                {
                    VERIFY_ARE_EQUAL(ResultsFormat::Text, profile.GetResultsFormat());
                }
                else
                {
                    VERIFY_ARE_EQUAL(ResultsFormat::Xml, profile.GetResultsFormat());
                }
            }
        }
    }

    void CmdLineParserUnitTests::TestParseCmdLineTargetPosition()
    {
        // coverage for positioning of targets and parameters

        CmdLineParser p;
        struct Synchronization s = {};

        // in order - this obviously duplicates all the other normal cases but, to
        // document it in place against the negative cases, repeat.
        {
            Profile profile;
            const char *argv[] = { "foo", "-r", "testfile.dat" };
            VERIFY_IS_TRUE(p.ParseCmdLine(_countof(argv), argv, &profile, &s));
        }
        {
            Profile profile;
            const char *argv[] = { "foo", "-r", "testfile.dat" , "testfile2.dat"};
            VERIFY_IS_TRUE(p.ParseCmdLine(_countof(argv), argv, &profile, &s));
        }

        // out of order - parameter follows one or more targets
        {
            Profile profile;
            const char *argv[] = { "foo", "testfile.dat" , "-r"};
            VERIFY_IS_FALSE(p.ParseCmdLine(_countof(argv), argv, &profile, &s));
        }
        {
            Profile profile;
            const char *argv[] = { "foo", "testfile.dat" , "testfile2.dat", "-r"};
            VERIFY_IS_FALSE(p.ParseCmdLine(_countof(argv), argv, &profile, &s));
        }

        // out of order - in between
        {
            Profile profile;
            const char *argv[] = { "foo", "testfile.dat" , "-r", "testfile2.dat"};
            VERIFY_IS_FALSE(p.ParseCmdLine(_countof(argv), argv, &profile, &s));
        }
    }

    void CmdLineParserUnitTests::TestParseCmdLineBufferSeparation()
    {
        CmdLineParser p;
        struct Synchronization s = {};

        // -bsp sets PDECacheLine
        {
            Profile profile;
            const char *argv[] = { "foo", "-bsp", "testfile.dat" };
            VERIFY_IS_TRUE(p.ParseCmdLine(_countof(argv), argv, &profile, &s));
            vector<TimeSpan> v(profile.GetTimeSpans());
            VERIFY_ARE_EQUAL(v[0].GetBufferSeparation(), BufferSeparation::PDECacheLine);
            VERIFY_IS_TRUE(v[0].IsBufferSeparationExplicit());
        }

        // -bsn sets SystemDefault
        {
            Profile profile;
            const char *argv[] = { "foo", "-bsn", "testfile.dat" };
            VERIFY_IS_TRUE(p.ParseCmdLine(_countof(argv), argv, &profile, &s));
            vector<TimeSpan> v(profile.GetTimeSpans());
            VERIFY_ARE_EQUAL(v[0].GetBufferSeparation(), BufferSeparation::SystemDefault);
            VERIFY_IS_TRUE(v[0].IsBufferSeparationExplicit());
        }

        // -bs (no flag) fails
        {
            Profile profile;
            const char *argv[] = { "foo", "-bs", "testfile.dat" };
            VERIFY_IS_FALSE(p.ParseCmdLine(_countof(argv), argv, &profile, &s));
        }

        // -bsx (invalid flag) fails
        {
            Profile profile;
            const char *argv[] = { "foo", "-bsx", "testfile.dat" };
            VERIFY_IS_FALSE(p.ParseCmdLine(_countof(argv), argv, &profile, &s));
        }

        // -bspx (extra chars) fails
        {
            Profile profile;
            const char *argv[] = { "foo", "-bspx", "testfile.dat" };
            VERIFY_IS_FALSE(p.ParseCmdLine(_countof(argv), argv, &profile, &s));
        }

        // -b4k still works (block size)
        {
            Profile profile;
            const char *argv[] = { "foo", "-b4k", "testfile.dat" };
            VERIFY_IS_TRUE(p.ParseCmdLine(_countof(argv), argv, &profile, &s));
            vector<TimeSpan> v(profile.GetTimeSpans());
            vector<Target> vt(v[0].GetTargets());
            VERIFY_ARE_EQUAL(vt[0].GetBlockSizeInBytes(), (DWORD)(4 * 1024));
        }

        // -b4k and -bsp together
        {
            Profile profile;
            const char *argv[] = { "foo", "-b4k", "-bsp", "testfile.dat" };
            VERIFY_IS_TRUE(p.ParseCmdLine(_countof(argv), argv, &profile, &s));
            vector<TimeSpan> v(profile.GetTimeSpans());
            VERIFY_ARE_EQUAL(v[0].GetBufferSeparation(), BufferSeparation::PDECacheLine);
            VERIFY_IS_TRUE(v[0].IsBufferSeparationExplicit());
            vector<Target> vt(v[0].GetTargets());
            VERIFY_ARE_EQUAL(vt[0].GetBlockSizeInBytes(), (DWORD)(4 * 1024));
        }

        // Default (no -bs) should be PDECacheLine but not explicit
        {
            Profile profile;
            const char *argv[] = { "foo", "testfile.dat" };
            VERIFY_IS_TRUE(p.ParseCmdLine(_countof(argv), argv, &profile, &s));
            vector<TimeSpan> v(profile.GetTimeSpans());
            VERIFY_ARE_EQUAL(v[0].GetBufferSeparation(), BufferSeparation::PDECacheLine);
            VERIFY_IS_FALSE(v[0].IsBufferSeparationExplicit());
        }
    }

    void CmdLineParserUnitTests::TestParseCmdLineCompletionDepth()
    {
        CmdLineParser p;
        struct Synchronization s = {};

        // -oc8 sets CompletionDepth to 8
        {
            Profile profile;
            const char *argv[] = { "foo", "-oc8", "testfile.dat" };
            VERIFY_IS_TRUE(p.ParseCmdLine(_countof(argv), argv, &profile, &s));
            vector<TimeSpan> v(profile.GetTimeSpans());
            VERIFY_ARE_EQUAL(v[0].GetCompletionDepth(), (DWORD)8);
            VERIFY_IS_TRUE(v[0].IsCompletionDepthExplicit());
        }

        // -oc1 valid (minimum)
        {
            Profile profile;
            const char *argv[] = { "foo", "-oc1", "testfile.dat" };
            VERIFY_IS_TRUE(p.ParseCmdLine(_countof(argv), argv, &profile, &s));
            vector<TimeSpan> v(profile.GetTimeSpans());
            VERIFY_ARE_EQUAL(v[0].GetCompletionDepth(), (DWORD)1);
        }

        // -oc16 valid (maximum)
        {
            Profile profile;
            const char *argv[] = { "foo", "-oc16", "testfile.dat" };
            VERIFY_IS_TRUE(p.ParseCmdLine(_countof(argv), argv, &profile, &s));
            vector<TimeSpan> v(profile.GetTimeSpans());
            VERIFY_ARE_EQUAL(v[0].GetCompletionDepth(), (DWORD)16);
        }

        // -oc0 invalid
        {
            Profile profile;
            const char *argv[] = { "foo", "-oc0", "testfile.dat" };
            VERIFY_IS_FALSE(p.ParseCmdLine(_countof(argv), argv, &profile, &s));
        }

        // -oc17 invalid (over maximum)
        {
            Profile profile;
            const char *argv[] = { "foo", "-oc17", "testfile.dat" };
            VERIFY_IS_FALSE(p.ParseCmdLine(_countof(argv), argv, &profile, &s));
        }

        // Default (no -oc) is c_defaultCompletionDepth and not explicit
        {
            Profile profile;
            const char *argv[] = { "foo", "testfile.dat" };
            VERIFY_IS_TRUE(p.ParseCmdLine(_countof(argv), argv, &profile, &s));
            vector<TimeSpan> v(profile.GetTimeSpans());
            VERIFY_ARE_EQUAL(v[0].GetCompletionDepth(), c_defaultCompletionDepth);
            VERIFY_IS_FALSE(v[0].IsCompletionDepthExplicit());
        }

        // -oc with -x conflicts (caught during validation in ParseCmdLine)
        {
            Profile profile;
            const char *argv[] = { "foo", "-oc4", "-x", "testfile.dat" };
            VERIFY_IS_FALSE(p.ParseCmdLine(_countof(argv), argv, &profile, &s));
        }
    }

    void CmdLineParserUnitTests::TestParseCmdLineSystemInfoOnly()
    {
        CmdLineParser p;
        struct Synchronization s = {};

        // -Rs (text, default format)
        {
            Profile profile;
            const char *argv[] = { "foo", "-Rs" };
            VERIFY_IS_TRUE(p.ParseCmdLine(_countof(argv), argv, &profile, &s));
            VERIFY_IS_TRUE(profile.GetSystemInformationOnly());
            VERIFY_ARE_EQUAL(ResultsFormat::Text, profile.GetResultsFormat());
        }

        // -Rstext (text, explicit)
        {
            Profile profile;
            const char *argv[] = { "foo", "-Rstext" };
            VERIFY_IS_TRUE(p.ParseCmdLine(_countof(argv), argv, &profile, &s));
            VERIFY_IS_TRUE(profile.GetSystemInformationOnly());
            VERIFY_ARE_EQUAL(ResultsFormat::Text, profile.GetResultsFormat());
        }

        // -Rsxml
        {
            Profile profile;
            const char *argv[] = { "foo", "-Rsxml" };
            VERIFY_IS_TRUE(p.ParseCmdLine(_countof(argv), argv, &profile, &s));
            VERIFY_IS_TRUE(profile.GetSystemInformationOnly());
            VERIFY_ARE_EQUAL(ResultsFormat::Xml, profile.GetResultsFormat());
        }

        // -Rs with extra args fails
        {
            Profile profile;
            const char *argv[] = { "foo", "-Rs", "testfile.dat" };
            VERIFY_IS_FALSE(p.ParseCmdLine(_countof(argv), argv, &profile, &s));
        }

        // -Rs with other parameters fails
        {
            Profile profile;
            const char *argv[] = { "foo", "-Rs", "-d10" };
            VERIFY_IS_FALSE(p.ParseCmdLine(_countof(argv), argv, &profile, &s));
        }
    }
}