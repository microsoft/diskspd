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

[string](get-date)

# buffer size/alighment, threads/target, outstanding/thread, write%
$b = 4; $t = 1; $o = 8; $w = 10

# optional - specify rate limit in iops, translated to bytes/ms for DISKSPD
$iops = 500
if ($iops -ne $null) { $g = $iops * $b * 1KB / 1000 }

# io pattern, (r)andom or (s)equential (si as needed for multithread)
$p = 'r'

# durations of test, cooldown, warmup
$d = 30*60; $cool = 30; $warm = 60

$addspec = 'base'
$gspec = $null
if ($g -ne $null) { $gspec = "g$($g)" }
$result = "result-b$($b)t$($t)o$($o)w$($w)p$($p)$($gspec)-$($addspec)-$(gc c:\vmspec.txt).xml"
$dresult = "l:\result"
$lresultf = join-path "c:\run" $result
$dresultf = join-path $dresult $result

# cap -> true to capture xml results, otherwise human text
$cap = $false

### prior to this is template

if (-not (gi $dresultf -ErrorAction SilentlyContinue)) {

    if ($cap) {
        $res = 'xml'
    } else {
        $res = 'text'
    }

    $gspec = $null
    if ($g -ne $null) { $gspec = "-g$($g)" }
    $o = C:\run\diskspd.exe -Z20M -z -h `-t$t `-o$o $gspec `-b$($b)k `-$($p)$($b)k `-w$w `-W$warm `-C$cool `-d$($d) -D -L `-R$res (dir C:\run\testfile?.dat)

    if ($cap) {

        # export result and indicate done flag to master
        # use unbuffered copy to force IO downstream

        $o | Out-File $lresultf -Encoding ascii -Width 9999
        xcopy /j $lresultf $dresult
        del $lresultf
        Write-Output "done"

    } else {

        #emit to human watcher
        $o | Out-Host
    }

} else {

    write-host -fore green already done $dresultf

    # indicate done flag to master
    # this should only occur if controller does not change variation
    Write-Output "done"
}

[system.gc]::Collect()
[string](get-date)