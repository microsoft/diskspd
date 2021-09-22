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

param(
    [string[]] $group = "*",
    [int] $number = 0
    )

icm (get-clusternode |? State -eq Up) {

    $n = $using:number

    # failed is an unclean offline tbd root causes (can usually be recovered)

    $using:group |% {

        # sorted list of vms by vm number
        $vms = @(Get-ClusterGroup |? OwnerNode -eq $env:COMPUTERNAME |? GroupType -eq VirtualMachine |? Name -like "vm-$_-*" | sort -Property @{ Expression = { $null = $_.Name -match '-(\d+)$'; [int] $matches[1] }})

        # start limited number, if specified, else all
        $(if ($n -gt 0 -and $vms.Count -gt $n) {
            $vms[0..($n - 1)]
          } else {
            $vms
          }) |? {
            $_.State -eq 'Offline' -or $_.State -eq 'Failed'
        } | Start-ClusterGroup
    }
} | ft -AutoSize