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
    $outfile = "result-cputarget.tsv",
    $cputargets = $(throw "please specify a set of cpu percentage targets"),
    $addspec = ""
    )

$result = "C:\ClusterStorage\collect\control\result"

# count the number of vms in the configuration
$vms = (Get-ClusterResource |? ResourceType -eq 'Virtual Machine' | measure).Count

# target cpu utilization and qos to +/- given percentage
$cputargetwindow = 5
$qoswindow = 5

# clean result file and set column headers
del -Force $outfile -ErrorAction SilentlyContinue
'WriteRatio','QOS','AVCPU','IOPS','AVRLat','AVWLat' -join "`t" > $outfile

# make qos policy and reset
Get-StorageQosPolicy -Name SweepQos -ErrorAction SilentlyContinue | Remove-StorageQosPolicy -Confirm:$false
New-StorageQosPolicy -Name SweepQoS -MinimumIops $qos -MaximumIops $qos -PolicyType Dedicated
set-storageqos -policyname SweepQoS

function is-within(
    $value,
    $target,
    $percentage
    )
{
    ($value -ge ($target - ($target*($percentage/100))) -and
     $value -le ($target + ($target*($percentage/100))))
}

function get-pc(
    [string] $blg,
    [int] $center,
    [string] $ctr
)
{
    # get central n samples of a performance counter's sample
    $pc = Import-Counter -Path $blg -Counter "\\*\$ctr"

    $t0 = ($pc.length - $center)/2
    $t1 = $t0 + $center - 1
    ($pc[$t0 .. $t1].CounterSamples.CookedValue | measure -Average).Average
}

$pc = @('\Hyper-V Hypervisor Logical Processor(_Total)\% Total Run Time',
        '\Processor Information(_Total)\% Processor Performance',
        '\Cluster CSVFS(_Total)\reads/sec',
        '\Cluster CSVFS(_Total)\avg. sec/read',
        '\Cluster CSVFS(_Total)\writes/sec',
        '\Cluster CSVFS(_Total)\avg. sec/write')

# limit the number of attempts per sweep (mix) to 4 per targeted cpu util
$sweeplimit = ($cputargets.count * 4)

foreach ($w in 0,10,30) {

    # track measured qos points, starting at given value
    $h = @{}
    $qosinitial = $qos = 400

    foreach ($cputarget in $cputargets) {

        # move the qos window using previous run information
        if ($cputarget -ne $cputargets[0]) {
            $nextqos = [int](($cputarget*$iops/$avcpu)/$vms)
            $qos = $nextqos
        }

        write-host -ForegroundColor Cyan Starting outer loop with CPU target $cputarget and initial QoS $qos

        do {

            # failsafes
            if ($h[$qos]) { write-host -ForegroundColor Red already measured $qos; break }
            if ($h.Keys.Count -ge $sweeplimit) { write-host -ForegroundColor Red $sweeplimit tries giving up; break }
   
            Set-StorageQosPolicy -Name SweepQoS -MaximumIops $qos
            write-host -fore Cyan Starting loop with QoS target $qos

            $curaddspec = "$($addspec)w$($w)qos$qos"
            start-sweep.ps1 -addspec $curaddspec -b 4 -o 32 -t 1 -w $w -p r -d 60 -warm 15 -cool 15 -pc $pc

            # HACKHACK bounce collect
            Get-ClusterSharedVolume |? { $_.SharedVolumeInfo.FriendlyVolumeName -match 'collect' } | Move-ClusterSharedVolume
            sleep 1

            # get average IOPS at DISKSPD

            $iops = $(dir $result\*.xml |% {
                $x = [xml](gc $_)
                ($x.Results.TimeSpan.Iops.Bucket | measure -Property Total -Average).Average
            } | measure -Sum).Sum

            # get average cpu utilization for central 60 seconds of each node
            # get average of all nodes

            $avcpu = $(dir $result\*.blg |% {

                $center = 60
                $trt = get-pc $_ $center '\Hyper-V Hypervisor Logical Processor(_Total)\% Total Run Time'
                $ppc = get-pc $_ $center '\Processor Information(_Total)\% Processor Performance'
                $trt*$ppc/100


            } | measure -Average).Average

            # get average latency for central 60 seconds, all nodes
            # note we must aggregate the product of av latency and iops per node to get total
            # latency, and then divide by total iops to get whole-cluster average.

            $csvrtotal = 0
            $csvwtotal = 0

            ($avrlat,$avwlat) = $(dir $result\*.blg |% {

                $csvrlat = get-pc $_ $center '\Cluster CSVFS(_Total)\avg. sec/read'
                $csvwlat = get-pc $_ $center '\Cluster CSVFS(_Total)\avg. sec/write'
                $csvr = get-pc $_ $center '\Cluster CSVFS(_Total)\reads/sec'
                $csvw = get-pc $_ $center '\Cluster CSVFS(_Total)\writes/sec'

                $csvrtotal += $csvr

                $csvwtotal += $csvw

                [pscustomobject] @{ 'avrtime' = $csvrlat*$csvr; 'avwtime' = $csvwlat*$csvw }

            } | measure -Sum -Property avrtime,avwtime).Sum

            $avrlat /= $csvrtotal
            $avwlat /= $csvwtotal

            # capture results
            $w,$qos,$avcpu,$iops,$avrlat,$avwlat -join "`t" >> $outfile

            # archive results
            compress-archive -Path $(dir $result\* -Exclude *.zip) -DestinationPath $result\$curaddspec.zip
            dir $result\* -Exclude *.zip | del
            write-host Archived results to $result\$curaddspec.zip

            # stop within targetwindow% of cpu (+/- % of target)
            if (is-within $avcpu $cputarget $cputargetwindow) {
                write-host -ForegroundColor Green "Stopping in target window at $('{0:N2}' -f $avcpu) with QoS $qos"
                break
            }
    
            # assume cpu and qos have a linear relationship - extrapolate to target
            # could do a direct linear fit of measurements so far
            $nextqos = [int]($cputarget*$qos/$avcpu)

            # stop if next qos target is within qoswindow% of any previous measurement
            $inwindow = $false
            foreach ($previous in $h.keys) {

                if (is-within $nextqos $previous $qoswindow) {
                    $inwindow = $true
                    break
                }
            }

            if ($inwindow) {
                write-host -ForegroundColor Yellow "Stopping in window of prior measurement at $('{0:N2}' -f $avcpu) with QoS $qos"
                break
            }

            # stop if next qos target is less than initial
            if ($nextqos -lt $qosinitial) {
                write-host -ForegroundColor Red "Stopping with underflow targeting $nextqos less than initial $qosinitial"
                break
            }

            write-host -ForegroundColor Cyan "Loop acheived $('{0:N2}' -f $avcpu) @ QoS $qos v. target $cputarget - next loop targeting QoS $nextqos"

            # record this datapoint as measured, move along to the next
            $h[$qos] = 1
            $qos = $nextqos

        } while ($true)
    }
}

set-storageqos -policyname $null