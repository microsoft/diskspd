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
    $ComputerName = $env:COMPUTERNAME,
    $SampleInterval = 2
    )

function div-to-width(
    [int] $div
    )
{
    # 0 - 100 scale
    # ex: 4 -> 100/4 = 25 buckets + 1 more for == 100
    1+100/$div
}

function center-pad(
    [string] $s,
    [int] $width
    )
{
    if ($width -le $s.length) {
        $s
    } else {
        (' ' * (($width - $s.length)/2) + $s)
    }
}

function get-legend(
    [int] $width,
    [int] $div
    )
{
    # now produce the scale, a digit at a time in vertical orientation
    # at each multiple of 10% which aligns with a measurement bucket.
    # the width is the width of the measured values
    #
    # 0 5 1
    #   0 0
    #     0

    $lines = @()
    $lines += ,('-' * $width)

    foreach ($dig in 0..2) {
         $o = foreach ($pos in 0..($width - 1)) {

            $val = $pos * $div
            if ($val % 10 -eq 0) {
                switch ($dig) {
                    0 { if ($val -eq 100) { 1 } else { $val / 10 }}
                    1 { if ($val -ne 0) { 0 } else { ' ' }}
                    2 { if ($val -eq 100) { 0 } else { ' ' }}
                }
            } else { ' ' }
        }

        $lines += ,($o -join '')
    }

    # trailing comments (horizontal scale name)
    'Percent CPU Utilization' |% {
        $lines += ,(center-pad $_ $width)
    }

    $lines
}

# these are the valid divisions, in units of percentage width.
# they must evenly divide 100% and either 10% or 20% for scale markings.
# determine which is the best fit based on window width.

$div = 0
foreach ($i in 1,2,4,5) {
    if ((div-to-width $i) -le [console]::WindowWidth) {
        $div = $i
        break
    }
}

# if nothing fit, widen to 4% divisions

if ($div -eq 0) {
    $div = 4
}

$width = div-to-width $div

# get the constant legend; use the remaining height for the vertical cpu core bars.
# note total height includes variable label line at bottom (instance + aggregagte)
$legend = get-legend $width $div
$clip = [console]::WindowHeight - $legend.Count - 1

# insist on a clip no lower than 10

if ($clip -lt 10) {
    $clip = 10
}

# set window and buffer size simultaneously so we don't have extra scrollbars
cls
[console]::SetWindowSize($width,$clip + $legend.Count + 1)
[console]::BufferWidth = [console]::WindowWidth
[console]::BufferHeight = [console]::WindowHeight

# scale divisions at x%
# this should evenly divide 100%
$m = [array]::CreateInstance([int],$width)

# which processor counterset should we use?
# pi is only the root partition if hv is active
# hvlp is the host physical processors when hv is active
# via ctrs, hv is active iff hvlp is present and has multiple instances
$cset = get-counter -ComputerName $ComputerName -ListSet 'Hyper-V Hypervisor Logical Processor' -ErrorAction SilentlyContinue
if ($cs -ne $null -and $cs.CounterSetType -eq [Diagnostics.PerformanceCounterCategoryType]::MultiInstance) {
    $cpuset = '\Hyper-V Hypervisor Logical Processor(*)\% Total Run Time'
} else {
    $cpuset = '\Processor Information(*)\% Processor Time'
}

# processor performance counter (turbo/speedstep)
$ppset = '\Processor Information(_Total)\% Processor Performance'

while ($true) {

    # reset measurements & the lines to output
    $lines = @()
    foreach ($i in 0..($m.length - 1)) {
        $m[$i] = 0
    }

    # avoid remoting for the local case
    if ($ComputerName -eq $env:COMPUTERNAME) {
        $samp = (get-counter -SampleInterval $SampleInterval -Counter $cpuset,$ppset).Countersamples
    } else {
        $samp = (get-counter -SampleInterval $SampleInterval -Counter $cpuset,$ppset -ComputerName $ComputerName).Countersamples
    }

    # get all specific instances and count them into appropriate measurement bucket
    $samp |% {
    
        if ($_.Path -like "*$ppset") { # scaling factor for total utility
            $pperf = $_.CookedValue/100
        } elseif ($_.InstanceName -notlike '*_Total') { # per cpu: ignore total and per-numa total
            $m[[math]::Floor($_.CookedValue/$div)] += 1
        } elseif ($_.InstanceName -eq '_Total') { # get total
            $total = $_.CookedValue
        }
    }

    # work down the veritical altitude of each strip, starting at vclip
    $altitude = $clip
    do {
        $lines += ,($($m |% {

            # if we are potentially clipped, handle
            if ($altitude -eq $clip) {

                # clipped?
                # unclipped but at clip?
                # nothing, less than altitude

                if ($_ -gt $altitude) { 'x' }
                elseif ($_ -eq $altitude) { '*' }
                else { ' ' }

            } else {
                # normal
                # >=, output
                # <, nothing
                if ($_ -ge $altitude) { '*' }
                else { ' ' }
            }
        }) -join '')
    } while (--$altitude)

    cls
    write-host -NoNewline ($lines + $legend -join "`n")
    write-host -NoNewLine ("`n" + (center-pad ("{2} Total: {0:0.0}% Normalized: {1:0.0}%" -f $total,($total*$pperf),$ComputerName) $width))

    # move the cursor to indicate average utilization
    # column number is zero based, width is 1-based
    [console]::SetCursorPosition([math]::Floor(($width - 1)*$total/100),[console]::CursorTop-$legend.Count)

}