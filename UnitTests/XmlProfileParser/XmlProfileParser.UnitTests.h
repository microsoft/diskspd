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
#include "WexTestClass.h"
#include <string>
using namespace std;

namespace UnitTests
{
    BEGIN_MODULE()
        MODULE_PROPERTY(L"Feature", L"XmlProfileParser")
    END_MODULE()

    MODULE_SETUP(ModuleSetup);
    MODULE_CLEANUP(ModuleCleanup);

    class XmlProfileParserUnitTests : public WEX::TestClass<XmlProfileParserUnitTests>
    {
    public:
        TEST_CLASS(XmlProfileParserUnitTests)

        TEST_CLASS_SETUP(ClassSetup);
        TEST_CLASS_CLEANUP(ClassCleanup);

        TEST_METHOD_SETUP(MethodSetup);
        TEST_METHOD_CLEANUP(MethodCleanup);

        TEST_METHOD(Test_ParseFile);
        TEST_METHOD(Test_ParseGroupAffinity);
        TEST_METHOD(Test_ParseNonGroupAffinity);
        TEST_METHOD(Test_ParseFilePrecreateFilesUseMaxSize);
        TEST_METHOD(Test_ParseFilePrecreateFilesOnlyFilesWithConstantSizes);
        TEST_METHOD(Test_ParseFilePrecreateFilesOnlyFilesWithConstantOrZeroSizes);
        TEST_METHOD(Test_ParseFileWriteBufferContentSequential);
        TEST_METHOD(Test_ParseFileWriteBufferContentZero);
        TEST_METHOD(Test_ParseFileWriteBufferContentRandom);
        TEST_METHOD(Test_ParseFileWriteBufferContentRandomNoFilePath);
        TEST_METHOD(Test_ParseFileWriteBufferContentRandomWithFilePath);
        TEST_METHOD(Test_ParseFileGlobalRequestCount);

        // TODO: test what happens when parameters have suffixes (e.g. 1M)
    private:
        string _sTempFilePath;
        HMODULE _hModule;
    };
}

