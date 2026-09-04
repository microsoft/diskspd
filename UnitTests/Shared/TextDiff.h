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

// Line-by-line text comparison utility for unit test diagnostics.
// Reports the first differing line between two multiline strings.

#pragma once

#include <string>
#include <string_view>
#include <sstream>

namespace TextDiff
{
    struct Result
    {
        bool equal;
        size_t lineNumber;        // 1-based line number of first difference (0 if equal)
        std::string expectedLine;
        std::string actualLine;
    };

    // Compare two multiline strings line by line and return the first difference.
    // If the strings are identical, result.equal is true. Otherwise, result contains
    // the 1-based line number and content of the first differing line.
    inline Result FindFirstDifference(const std::string& expected, const std::string& actual)
    {
        if (expected == actual)
        {
            return { true, 0, {}, {} };
        }

        std::istringstream expectedStream(expected);
        std::istringstream actualStream(actual);
        std::string expectedLine, actualLine;
        size_t lineNumber = 0;

        while (true)
        {
            bool hasExpected = !!std::getline(expectedStream, expectedLine);
            bool hasActual = !!std::getline(actualStream, actualLine);
            lineNumber++;

            if (!hasExpected && !hasActual)
            {
                // Strings differ but all parsed lines matched (e.g., trailing newline difference)
                return { false, lineNumber, "<trailing content differs>", "<trailing content differs>" };
            }

            if (!hasExpected || !hasActual || expectedLine != actualLine)
            {
                return { false, lineNumber,
                         hasExpected ? expectedLine : "<end of text>",
                         hasActual ? actualLine : "<end of text>" };
            }
        }
    }

    // Print the first line-by-line difference between two strings, if any.
    // Use before a VERIFY assertion to add diagnostic output on failure.
    inline void PrintFirstDifference(const std::string& expected, const std::string& actual)
    {
        auto diff = FindFirstDifference(expected, actual);
        if (!diff.equal)
        {
            printf("First difference at line %zu\n"
                   "  Expected: |%s| (length %zu)\n"
                   "    Actual: |%s| (length %zu)\n",
                   diff.lineNumber,
                   diff.expectedLine.c_str(), diff.expectedLine.size(),
                   diff.actualLine.c_str(), diff.actualLine.size());
        }
    }
} // namespace TextDiff

// Verify two multiline strings are equal, printing the first line-by-line
// difference on failure for diagnostic purposes. Uses string_view for the
// final comparison to guarantee value comparison without allocation (two
// const char* would otherwise compare pointer addresses in VERIFY_ARE_EQUAL).
#define VERIFY_MULTILINE_EQUAL(expected, actual) \
    do { \
        const auto& e = (expected); \
        const auto& a = (actual); \
        TextDiff::PrintFirstDifference(e, a); \
        VERIFY_ARE_EQUAL(std::string_view(e), std::string_view(a)); \
    } while (0)
