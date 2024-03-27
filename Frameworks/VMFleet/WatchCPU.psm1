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

Set-StrictMode -Version 3.0

function Watch-FleetCPU
{
    [CmdletBinding()]
    param(
        [Parameter()]
        [string]
        $ComputerName = $env:COMPUTERNAME,

        [Parameter()]
        [switch]
        $Guest,

        [Parameter()]
        [int]
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
        [string] $label,
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
        $lines += '-' * $width

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

            $lines += $o -join ''
        }

        # trailing comments (horizontal scale name)
        $lines += center-pad $label $width

        $lines
    }

    # minimum clip, the vertical height available for the cpu core bars
    $minClip = 10

    # these are the valid divisions, in units of percentage width.
    # they must evenly divide 100% and either 10% or 20% for scale markings.
    # determine which is the best fit based on window width.

    $div = 0
    $divs = 1,2,4,5
    foreach ($i in $divs) {
        if ((div-to-width $i) -le [console]::WindowWidth) {
            $div = $i
            break
        }
    }

    # if nothing fit ... ask for minimum
    # in practice this is not possible, but we check anyway
    if ($div -eq 0) {
        Write-Error "Window width must be at least $(div-to-width $divs[-1]) columns"
        return
    }

    $width = div-to-width $div

    # which processor counterset should we use?
    # pi is only the root partition if hv is active; when hv is active:
    #     hvlp is the host physical processors
    #     hvvp is the guest virtual processors
    # via ctrs, hv is active iff hvlp is present and has multiple instances
    $cs = Get-Counter -ComputerName $ComputerName -ListSet 'Hyper-V Hypervisor Logical Processor' -ErrorAction SilentlyContinue
    $hvactive = $null -ne $cs -and $cs.CounterSetType -eq [Diagnostics.PerformanceCounterCategoryType]::MultiInstance

    if ($Guest -and -not $hvactive) {
        Write-Error "Hyper-V is not active on $ComputerName"
        return
    }

    if ($hvactive) {
        if ($Guest) {
            $cpuset = "\Hyper-V Hypervisor Virtual Processor(*)\% Guest Run Time"
            $legend = get-legend "Percent Guest VCPU Utilization" $width $div
        } else {
            $cpuset = '\Hyper-V Hypervisor Logical Processor(*)\% Total Run Time'
            $legend = get-legend "Percent Host LP Utilization" $width $div
        }
    } else {
        $cpuset = '\Processor Information(*)\% Processor Time'
        $legend = get-legend "Percent Processor Utilization" $width $div
    }

    # processor performance counter (turbo/speedstep)
    # this is used to normalize the total cpu utilization (can be > 100%)
    $ppset = '\Processor Information(_Total)\% Processor Performance'

    # account for the constant legend in the window height
    # use the remaining height for the vertical cpu core bars.
    $clip = [console]::WindowHeight - ($legend.Count + 1)

    # insist on a minimum amount of space
    if ($clip -lt $minClip) {
        $minWindowHeight = $minClip + $legend.Count + 1
        Write-Error "Window height must be at least $minWindowHeight lines"
        return
    }

    # set window and buffer size simultaneously so we don't have extra scrollbars
    Clear-Host
    [console]::SetWindowSize($width, [console]::WindowHeight)
    [console]::BufferWidth = [console]::WindowWidth
    [console]::BufferHeight = [console]::WindowHeight

    # common params for Get-Counter
    $gcParam = @{
        SampleInterval = $SampleInterval
        Counter = $cpuset,$ppset
        ErrorAction = 'SilentlyContinue'
    }

    while ($true) {

        # reset measurements
        # these are the vertical height of a cpu bar in each column, e.g. 2 = 2 cpus
        $m = @([int] 0) * $width

        # avoid remoting for the local case
        if ($ComputerName -eq $env:COMPUTERNAME) {
            $ctrs = Get-Counter @gcParam
        } else {
            $ctrs = Get-Counter @gcParam -ComputerName $ComputerName
        }

        # if more than one countersample was returned (ppset + something more), we have data
        if ($null -ne $ctrs -and $ctrs.Countersamples.Count -gt 1)
        {
            # get all specific instances and count them into appropriate measurement bucket
            $ctrs.Countersamples |% {

                # get scaling factor for total utility
                if ($_.Path -like "*$ppset") {
                    $pperf = $_.CookedValue/100
                }

                # a cpu: count into appropriate measurement bucket
                # (ignore total and/or and per-numa total)
                elseif ($_.InstanceName -notlike '*_Total') {
                    $m[[math]::Floor($_.CookedValue/$div)] += 1
                }

                # get total
                #
                elseif ($_.InstanceName -eq '_Total') {
                    $total = $_.CookedValue
                }
            }

            # now produce the bar area as a series of lines
            # work down the veritical altitude of each strip, starting at the clip/top
            $altitude = $clip
            $lines = do {
                $($m |% {

                    # top line - if we are potentially clipped, handle
                    if ($altitude -eq $clip) {

                        # clipped?
                        if ($_ -gt $altitude) { 'x' }
                        # unclipped but at clip?
                        elseif ($_ -eq $altitude) { '*' }
                        # nothing, bar less than altitude
                        else { ' ' }

                    } else {

                        # below top line
                        # >=, output bar
                        if ($_ -ge $altitude) { '*' }
                        # <, nothing
                        else { ' ' }
                    }
                }) -join ''
            } while (--$altitude)

            $totalStr = "{0:0.0}%" -f $total
            $normalStr = "{0:0.0}%" -f ($total*$pperf)

            # move the cursor to indicate average utilization
            # column number is zero based, width is 1-based
            $cpos = [math]::Floor(($width - 1)*$total/100)
        }
        else
        {
            # Center no data message vertically and horizontally in the frame

            $vpre = [math]::Floor($clip/2) - 1
            $vpost = [math]::Floor($clip/2)

            $lines  = @('') * $vpre
            $lines += center-pad "No Data Available" $width
            $lines += @('') * $vpost

            $totalStr = $normalStr = "---"

            # zero cursor
            $cpos = 0
        }

        Clear-Host
        Write-Host -NoNewline ($lines + $legend -join "`n")
        Write-Host -NoNewLine ("`n" + (center-pad "$ComputerName Total: $totalStr Normalized: $normalStr" $width))

        # move the cursor to indicate average utilization
        # column number is zero based, width is 1-based
        [console]::SetCursorPosition($cpos,[console]::CursorTop-$legend.Count)
    }
}