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
    [string] $zip = $(throw "please provide zip filename for compressed logs"),
    [ValidateSet("*","HyperV","FailoverClustering","SMB")][string[]] $set = "*",
    [int] $timespan = 0
    )

# convert minutes to milliseconds
if ($timespan -gt 0) {
    $timespan *= 60*1000
}

$logs = icm (get-clusternode) {

    $map = @{
        "HyperV" = "Microsoft-Windows-Hyper-V";
        "FailoverClustering" = "Microsoft-Windows-FailoverClustering"
        "SMB" = "Microsoft-Windows-SMB"
    }

    if ($using:set -eq "*") {
        $lset = $map.Keys |% { $_ }
    } else {
        $lset = $using:set
    }

$q = @"
/q:"*[System[TimeCreated[timediff(@SystemTime) <= __TIME__]]]"
"@

    if ($using:timespan -gt 0) {
        $timefilt = $q -replace '__TIME__',$using:timespan
    } else {
        $timefilt = $null
    }

    $lset |% {

        # get logs and normalize to legal filenames
        $prov = wevtutil el | sls $map[$_]
        $provf = $prov -replace '/','-'

        foreach ($i in 0..($prov.Count - 1)) {

            $localpath = "$($provf[$i])--$env:computername.evtx"
            del -Force $localpath -ErrorAction SilentlyContinue
            wevtutil epl $prov[$i] "c:\$localpath" $timefilt
            write-output "\\$env:computername\c$\$localpath"
        }
    }
}

compress-archive -Path $logs -DestinationPath $zip
del -force $logs