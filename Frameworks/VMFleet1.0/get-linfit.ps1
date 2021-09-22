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
    [string[]]$idxcol,
    [switch]$zerointercept = $false
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

function do-linfit(
    [string] $xcol,
    [string] $ycol,
    [string] $key
    )
{
    BEGIN
    {
        $sumxy = [decimal]0
        $sumx2 = [decimal]0
        $sumx = [decimal]0
        $sumy = [decimal]0

        $n = 0

        $pipe = @()
    }

    PROCESS
    {
        $x = [decimal]$_.$xcol
        $y = [decimal]$_.$ycol

        $sumxy += $x*$y
        $sumx2 += $x*$x
        $sumx += $x
        $sumy += $y

        $n += 1

        # accumulate pipeline for second pass
        $pipe += $_
    }

    END
    {
        if ($n -eq 0) {
            Write-Error "ERROR: no measurements matched"
            return
        }

        # perform requested fit

        $a = 0
        $b = 0

        if ($zerointercept) {

            $a = 0
            $b = ($sumxy/$sumx2)

        } else {

            $b = ($sumxy - (($sumx*$sumy)/$n)) / ($sumx2 - (($sumx*$sumx)/$n))
            $a = ($sumy - $b*$sumx)/$n
        }

        # calculate r2 (coefficient of determination) with respect to the fit
        $meany = $sumy/$n

        # total sum of squares
        $sstot = [decimal]0
        $pipe |% {
            $v = [decimal]$_.$ycol - $meany
            $sstot += $v*$v
        }

        # residual sum of squares
        $ssres = [decimal]0
        $pipe |% {
            $v = [decimal]$_.$ycol - ($a + $b*[decimal]$_.$xcol)
            $ssres += $v*$v
        }

        $r2 = 1 - ($ssres/$sstot)

        new-object -TypeName psobject -Property @{ 'A' = $a; 'B' = $b; 'R2' = $r2; 'N' = $n; 'Key' = $key; 'X' = $xcol; 'Y' = $ycol }
    }
}

# process the results into a hash keyed by the index columns
$h = @{}
import-csv -Path $csvfile -Delimiter $delimeter |% {

    $key = ""
    foreach ($col in $idxcol) {
        $key += "$col $($_.$col)"
    }

    $h[$key] += ,$_
}

$h.Keys |% {

    $h[$_] | do-linfit -xcol $xcol -ycol $ycol -key $_
}