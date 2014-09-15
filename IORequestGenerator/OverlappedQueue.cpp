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

#include "OverlappedQueue.h"
#include <assert.h>

OverlappedQueue::OverlappedQueue(void) :
    _pHead(nullptr),
    _pTail(nullptr),
    _cItems(0)
{
}

void OverlappedQueue::Add(OVERLAPPED *pOverlapped)
{
    pOverlapped->Internal = NULL;
    if (_pHead == nullptr)
    {
        assert(_pTail == nullptr);
        _pHead = pOverlapped;
    }
    else
    {
        assert(_pTail != nullptr);
        _pTail->Internal = (ULONG_PTR)pOverlapped;
    }
    _pTail = pOverlapped;
    _cItems++;
}

bool OverlappedQueue::IsEmpty(void) const
{
    return (_pHead == nullptr);
}

OVERLAPPED *OverlappedQueue::Remove(void)
{
    assert(!IsEmpty());

    OVERLAPPED *pOverlapped = _pHead;
    _pHead = (OVERLAPPED *)pOverlapped->Internal;
    if (_pHead == nullptr)
    {
        _pTail = nullptr;
    }
    _cItems--;
    return pOverlapped;
}

size_t OverlappedQueue::GetCount() const
{
    return _cItems;
}