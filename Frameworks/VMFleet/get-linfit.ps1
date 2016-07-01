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
    $csvfile,
    [string]$delimeter = "`t",
    [string]$xcol = $(throw "please specify column containing x values"),
    [string]$ycol = $(throw "please specify column containing y values"),
    [string]$idxcol,
    [string]$idxval = $null,
    [switch]$zerointercept = $false,
    [int]$sigfigs = 5
    )

# given the input data, produce the linear fit coefficients for
#
#     zerointercept = true: y = bx
#    !zerointercept = true: y = a + bx
#
# specify the column containing the x and y measurements
# a single index column can be used to differentiate rows which should be
# used for the fit (i.e., "test1", "test2", ...).
#
# method: ordinary least squares

function get-sigfigs(
    [decimal]$value,
    [int]$sigfigs
    )
{
    $log = [math]::Ceiling([math]::log10([math]::abs($value)))
    $decimalpt = $sigfigs - $log

    # if all sigfigs are above the decimal point, round off to
    # appropriate power of 10
    if ($decimalpt -lt 0) {
        $pow = [math]::Abs($decimalpt)
        $decimalpt = 0
        $value = [math]::Round($value/[math]::Pow(10,$pow))*[math]::pow(10,$pow)
    }

    "{0:F$($decimalpt)}" -f $value
}

$sumxy = [decimal]0
$sumx2 = [decimal]0
$sumx = [decimal]0
$sumy = [decimal]0

$results = import-csv -Path $csvfile -Delimiter $delimeter |? {

    $idxval -eq $null -or $_.$idxcol -eq $idxval
}

# pass 1 to produce the linear fit parameters
$results |% {

    $x = [decimal]$_.$xcol
    $y = [decimal]$_.$ycol

    $sumxy += $x*$y
    $sumx2 += $x*$x
    $sumx += $x
    $sumy += $y
}

$n = [decimal]$results.count

if ($n -eq 0) {
    Write-Error "ERROR: no measurements matched"
    return
} else {
    write-host "n = $n"
}

# perform requested fit

$a = 0
$b = 0

if ($zerointercept) {

    $a = 0
    $b = ($sumxy/$sumx2)
    $bstr = get-sigfigs $b $sigfigs

    write-host "$ycol = b($xcol)`nb = $bstr"

} else {

    $b = ($sumxy - (($sumx*$sumy)/$n)) / ($sumx2 - (($sumx*$sumx)/$n))
    $a = ($sumy - $b*$sumx)/$n

    $bstr = get-sigfigs $b $sigfigs
    $astr = get-sigfigs $a $sigfigs

    write-host "$ycol = a + b($xcol)`na = $astr`nb = $bstr"
}

# calculate r2 (coefficient of determination) with respect to the fit
$meany = $sumy/$n

# total sum of squares
$sstot = [decimal]0
$results |% {
    $v = [decimal]$_.$ycol - $meany
    $sstot += $v*$v
}

# residual sum of squares
$ssres = [decimal]0
$results |% {
    $v = [decimal]$_.$ycol - ($a + $b*[decimal]$_.$xcol)
    $ssres += $v*$v
}

$r2 = 1 - ($ssres/$sstot)
write-host ("r2 goodness of fit = {0:P2}" -f $r2)