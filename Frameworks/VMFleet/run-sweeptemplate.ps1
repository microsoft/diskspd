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
$b = __b__; $t = __t__; $o = __o__; $w = __w__

# optional - specify rate limit in iops, translated to bytes/ms for DISKSPD
$iops = __iops__
if ($iops -ne $null) { $g = $iops * $b * 1KB / 1000 }

# io pattern, (r)andom or (s)equential (si as needed for multithread)
$p = '__p__'

# durations of test, cooldown, warmup
$d = __d__; $cool = __Cool__; $warm = __Warm__

# sweep template always captures
$addspec = '__AddSpec__'
$gspec = $null
if ($g -ne $null) { $gspec = "g$($g)" }
$result = "result-b$($b)t$($t)o$($o)w$($w)p$($p)$($gspec)-$($addspec)-$(gc c:\vmspec.txt).xml"
$dresult = "l:\result"
$lresultf = join-path "c:\run" $result
$dresultf = join-path $dresult $result

### prior to this is template

if (-not (gi $dresultf -ErrorAction SilentlyContinue)) {

    $res = 'xml'
    $gspec = $null
    if ($g -ne $null) { $gspec = "-g$($g)" }

    C:\run\diskspd.exe -Z20M -z -h `-t$t `-o$o $gspec `-b$($b)k `-$($p)$($b)k `-w$w `-W$warm `-C$cool `-d$($d) -D -L `-R$res (dir C:\run\testfile?.dat) > $lresultf

    # export result and indicate done flag to master
    # use unbuffered copy to force IO downstream
    xcopy /j $lresultf $dresult
    del $lresultf
    Write-Output "done"

} else {

    write-host -fore green already done $dresultf

    # indicate done flag to master
    # this should only occur if controller does not change variation
    Write-Output "done"
}

[system.gc]::Collect()
[string](get-date)