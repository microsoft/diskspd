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

// versioning material. for simplicity in consumption, please ensure that the date string
// parses via the System.Datetime constructor as follows (in Powershell):
//
//      [datetime] "string"
//
// this should result in a valid System.Datetime object, rendered like:
//
//      Monday, June 16, 2014 12:00:00 AM

#define DISKSPD_RELEASE_TAG ""
#define DISKSPD_REVISION    ""

#define DISKSPD_MAJOR       2
#define DISKSPD_MINOR       2
#define DISKSPD_BUILD       0
#define DISKSPD_QFE         0

#define DISKSPD_MAJORMINOR_VER_STR(x,y,z) #x "." #y "." #z
#define DISKSPD_MAJORMINOR_VERSION_STRING(x,y,z) DISKSPD_MAJORMINOR_VER_STR(x,y,z)
#define DISKSPD_MAJORMINOR_VERSION_STR DISKSPD_MAJORMINOR_VERSION_STRING(DISKSPD_MAJOR, DISKSPD_MINOR, DISKSPD_BUILD)

#define DISKSPD_NUMERIC_VERSION_STRING DISKSPD_MAJORMINOR_VERSION_STR DISKSPD_REVISION DISKSPD_RELEASE_TAG
#define DISKSPD_DATE_VERSION_STRING "2024/6/3"
