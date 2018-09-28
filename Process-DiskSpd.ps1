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

function process-result(
    )
{
    PROCESS {

        log-host -fore green $_

        if ((Get-Content $_ -first 1) -ne '<Results>') {

            write-host -ForegroundColor Red ERROR: $_ does not appear to be an DiskSpd XML result. Make sure `-Rxml was specified.

        } else {

            # count valid results
            $script:valid += 1

            $x = [xml](Get-Content $_)

            $lf = $_.fullname -replace '.xml','.lat.tsv'

            if (-not [io.file]::Exists($lf)) {
                get-latency $x > $lf
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

            $o = @{
                'ResultFileName' = $_.BaseName;
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

            $ls = $script:l |% {
                $b = $h[$_];
                if ($b.ReadMilliseconds) { $v = $b.ReadMilliseconds } else { $v = "" }
                $o["LatRead$_"] = $v
                if ($b.WriteMilliseconds) { $v = $b.WriteMilliseconds } else { $v = "" }
                $o["LatWrite$_"] = $v
            }

            # Parse additional fields from filename? other? here
            $add = @()
            if ($_.BaseName -match "-wc(\d+)wicpw(\d+)vm(\d+)") {
                $add = $matches[1..3]
            } elseif ($_.BaseName -match "-basevm(\d+)") {
                $add = @('base','base',$matches[1])
            } else {
                $add = @('na','na','na')
            }

            if ($add.count -gt 0) {
                write-host adding fields ($add -join ' : ')
            }

            # Place any addons. user to rename manually for the moment.
            $n = 1
            $add |% {
                $o["Add$n"] = $_
                $n++
            }

            # emit for export
            $o
        }
    }
}

#########################
#
$delim = "`t"
$l = @(); foreach ($i in 25,50,75,90,95,99,99.9,99.99,99.999,100) { $l += ,[string]$i }

$files = dir (join-path $xmlresultpath *.xml)

#########################
# get schema and process/emit column headers
$valid = 0
$scratch = $files[0] | process-result

# if no valid results found, stop now
if ($valid -eq 0) {

    write-host -ForegroundColor Red ERROR: `"$xmlresultpath`"` may not contain DISKSPD result files
    return
}

# lexically order the column schema and emit with bounding quotes per field
$fields = $scratch.Keys | sort

# column headers to output
$($fields |% { "`"$_`"" }) -join "$delim" | Out-File -FilePath $outfile

#########################
# now process all files for rows
$valid = 0
$files | process-result |% {
    $row = $_
    $($fields |% { "`"$($row[$_])`"" }) -join "$delim"
} | Out-File -FilePath $outfile -Append

# if some invalid files were found, warn
if ($valid -ne $files.count) {

    write-host -ForegroundColor Red WARNING: `"$xmlresultpath`"` contained $($files.count - $valid) non-DISKSPD xml files
    return
}