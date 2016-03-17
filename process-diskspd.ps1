<#
DISKSPD

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
#>

param(
    [string] $xmlresultpath = ".",
    [string] $outfile = "result.tsv"
    )

function Log-Host(
    $ForegroundColor = 'Green',
    [parameter(Position=0, ValueFromRemainingArguments=$true)] $args
    )
{
    write-host -ForegroundColor $ForegroundColor $(get-date -DisplayHint Time) $args
}

function get-latency( $x ) {

    $x.Results.TimeSpan.Latency.Bucket |% {
        $_.Percentile,$_.ReadMilliseconds,$_.WriteMilliseconds -join "`t"
    }
}

$delim = "`t"
$l = @(); foreach ($i in 25,50,75,90,95,99,99.9,99.99,99.999,100) { $l += ,[string]$i }

$tmp = New-TemporaryFile
$valid = 0

dir (join-path $xmlresultpath *.xml) |% {

    log-host -fore green $_

    if ((Get-Content $_ -first 1) -ne '<Results>') {

        write-host -ForegroundColor Red ERROR: $_ does not appear to be an DiskSpd XML result. Make sure `-Rxml was specified.

    } else {

        # count valid results
        $valid += 1

        $x = [xml](Get-Content $_)

        $lf = $_.fullname -replace '.xml','.lat.tsv'

        if (-not [io.file]::Exists($lf)) {
            get-latency $x > $lf
        }

        # Parse additional fields from filename here
        # XXX update method of adding to the object

        if ($_.BaseName -match "-i(\d+)c(\d+)-integ(\d)-r(\w+)") {
            $add = $matches[1..4]
        }

        $system = $x.Results.System.ComputerName
        $t = $x.Results.TimeSpan.TestTimeSeconds

        # sum read and write iops across all threads and targets
        $ri = ($x.Results.TimeSpan.Thread.Target |
                measure -sum -Property ReadCount).Sum
        $wi = ($x.Results.TimeSpan.Thread.Target |
                measure -sum -Property WriteCount).Sum
        $rb = ($x.Results.TimeSpan.Thread.Target |
                measure -sum -Property ReadBytes).Sum
        $wb = ($x.Results.TimeSpan.Thread.Target |
                measure -sum -Property WriteBytes).Sum

        $riops = $ri / $t
        $wiops = $wi / $t
        $rbw = $rb / $t
        $wbw = $wb / $t

        # avoid undefined/div-by-zero when calculating coefficient of var
        $ricv = $null
        if ($riops -gt 0) { $ricv = $x.Results.TimeSpan.Iops.ReadIopsStdDev / $riops }
        $wicv = $null
        if ($wiops -gt 0) { $wicv = $x.Results.TimeSpan.Iops.WriteIopsStdDev / $wiops }

        if ($ricv -gt 0.10 -or $wicv -gt 0.10) {
            log-host -ForegroundColor red ("ricv {0:F3} wicv {1:F3}" -f $ricv,$wicv)
        } else {
            log-host ("ricv {0:F3} wicv {1:F3}" -f $ricv,$wicv)
        }

        # note that with runs specified on the command line, only a single write ratio,
        # outstanding request count and blocksize can be specified, so sampling the one
        # used for the first thread is sufficient.
        #
        # this script DOES NOT handle complex timespan specifications

        if (($x.Results.Profile.TimeSpans.TimeSpan.Targets.Target.Random | select -first 1) -gt 0) {
            $pat = "Random"
            $align = ($x.Results.Profile.TimeSpans.TimeSpan.Targets.Target.Random | select -first 1)
        } else {
            $pat = "Sequential"
            $align = ($x.Results.Profile.TimeSpans.TimeSpan.Targets.Target.StrideSize | select -first 1)
        }

        # convert to object form / export-csv

        $o = new-object psobject -Property @{
            'Path' = ($x.Results.TimeSpan.Thread.target.path | select -first 1);
            'Affinity' = ($x.Results.Profile.TimeSpans.TimeSpan.DisableAffinity -ne 'true');
            'System' = $system;
            'TestTime' = $t;
            'WriteRatio' = ($x.Results.Profile.TimeSpans.TimeSpan.Targets.Target.WriteRatio |
                select -first 1);
            'Pattern' = $pat;
            'Align' = $align;
            'Threads' = $x.Results.TimeSpan.ThreadCount;
            'Outstanding' = ($x.Results.Profile.TimeSpans.TimeSpan.Targets.Target.RequestCount |
                select -first 1);
            'Block' = ($x.Results.Profile.TimeSpans.TimeSpan.Targets.Target.BlockSize |
                select -first 1);
            'CPU' = $x.Results.TimeSpan.CpuUtilization.Average.UsagePercent;
            'CPUKern' = $x.Results.TimeSpan.CpuUtilization.Average.KernelPercent;
            'CPUHotCore' = ($x.Results.TimeSpan.CpuUtilization.CPU.UsagePercent |% { [decimal] $_ } |
                sort -Descending | select -first 1);
            'CPUHotCoreKern' = ($x.Results.TimeSpan.CpuUtilization.CPU.KernelPercent |% { [decimal] $_ } |
                sort -Descending | select -first 1);
            'ReadIOPS' = $riops;
            'ReadIOPSCoV' = $ricv;
            'ReadBW' = $rbw;
            'ReadBytes' = $rb;
            'LatReadAV' = $x.Results.TimeSpan.Latency.AverageReadMilliseconds;
            'WriteIOPS' = $wiops;
            'WriteIOPSCoV' = $wicv;
            'WriteBW' = $wbw;
            'WriteBytes' = $wb;
            'LatWriteAV' = $x.Results.TimeSpan.Latency.AverageWriteMilliseconds;
            'TotIOPS' = $riops + $wiops;
            'TotBW' = $rbw + $wbw;
        }

        # extract the subset of latency percentiles as specified above in $l
        $h = @{}; $x.Results.TimeSpan.Latency.Bucket |% { $h[$_.Percentile] = $_ }

        $ls = $l |% {
            $b = $h[$_];
            if ($b.ReadMilliseconds) { $v = $b.ReadMilliseconds } else { $v = "" }
            $o | add-member -NotePropertyName "LatRead$_" -NotePropertyValue $v
            if ($b.WriteMilliseconds) { $v = $b.WriteMilliseconds } else { $v = "" }
            $o | add-member -NotePropertyName "LatWrite$_" -NotePropertyValue $v
        }

        # emit for export
        $o
    }
} | Export-Csv -Path $tmp -NoTypeInformation -Delimiter $delim

# if no valid results found, stop now
if ($valid -eq 0) {

    write-host -ForegroundColor Red ERROR: no valid results found in the directory `"$xmlresultpath`"`.
    return
}

###
# export-csv outputs the properties which were not explicitly add-membered in a non-lexical order.
# reorder the columns lexically for visual convenience
###

# extract header and construct hash to hold values
$h = (gc $tmp | select -First 1) -split $delim
$dh = @{}
$h |% { $dh[$_] = @() }

# now read data, insert into hash in order of headers
gc $tmp | select -Skip 1 |% {

    $dat = $_ -split $delim
    foreach ($i in 0..$($dat.count - 1)) {
        $dh[$h[$i]] += $dat[$i]
    }
}

# scrub temporary data file
del $tmp

# verify that all fields have the same number of values
$g = $dh.keys |% { $dh[$_].count } | group
if (($g | measure).count -ne 1) {
    write-host -ForegroundColor Red Internal error: parse to reorder columns did not result in equal `#values per column
    $g
    return
}

# now re-output in lexically sorted columns
$hsrt = $h | sort
$hsrt -join $delim | out-file $outfile
# foreach row ...
foreach ($i in 0..$(([int]($g.name)) - 1)) {
    $($hsrt |% { $dh[$_][$i] }) -join $delim | out-file $outfile -Append
}