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

# io pattern, (r)andom or (s)equential (si as needed for multithread)
$p = '__p__'

# durations of test, cooldown, warmup
$d = __d__; $cool = __Cool__; $warm = __Warm__

# cap -> true to capture xml results, otherwise human text
$addspec = '__AddSpec__'
$result =  "l:\result\result-b$($b)t$($t)o$($o)w$($w)p$($p)-$($addspec)-$(gc c:\vmspec.txt).xml"

### prior to this is template

if (-not (gi $result -ErrorAction SilentlyContinue)) {

    $res = 'xml'
    $o = C:\run\diskspd.exe -h `-t$t `-o$o `-b$($b)k `-$($p)$($b)k `-w$w `-W$warm `-C$cool `-d$($d) -D -L `-R$res (dir C:\run\testfile?.dat)

    # emit result and indicate done flag to master
    # this will nominally squelch re-execution
    $o | Out-File $result -Encoding ascii -Width 9999
    Write-Output "done"

} else {

    write-host -fore green already done $result

    # indicate done flag to master
    # this should only occur if controller does not change variation
    Write-Output "done"
}

[system.gc]::Collect()

[string](get-date)