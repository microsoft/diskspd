<#
DISKSPD - VM Fleet

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
#>

$pause = "C:\ClusterStorage\collect\control\pause"

$o = dir "$pause-*" |% {
    if ($_.name -match 'pause-(.+)\+(vm.+)') {
        new-object psobject -Property @{
            'Node' = $matches[1];
            'VM' = $matches[2]
        }
    }
}

$g = $o | group -Property Node -NoElement | sort -property Count,Name

write-host -fore green Total Pauses Active
$g

$gc = $g | group -Property Count

# all nodes should agree on number of pauses accepted
if ($gc.length -gt 1 -or $gc.count -ne (get-clusternode).length) {
    write-host -fore red `n Problem
} else {
    write-host -fore green `n Pause OK
}
