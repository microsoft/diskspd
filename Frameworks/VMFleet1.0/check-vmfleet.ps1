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

param ($group = '*')

$g = Get-ClusterGroup |? GroupType -eq VirtualMachine |? Name -like "vm-$group-*"


###########################################################
write-host -fore green State Pivot
$g | group -Property State -NoElement | sort -Property Name | ft -autosize

###########################################################
write-host -fore green Host Pivot
$g | group -Property OwnerNode,State -NoElement | sort -Property Name | ft -autosize

###########################################################
write-host -fore green Group Pivot
$g |% {
    if ($_.Name -match "^vm-([^-]+)-") {
        $_ | add-member -NotePropertyName Group -NotePropertyValue $matches[1] -PassThru
    }
} | group -Property Group,State -NoElement | sort -Property Name | ft -AutoSize

###########################################################
write-host -fore green IOPS Pivot
# build 5 orders of log steps
$logstep = 10,20,50
$log = 1
$logs = 1..5 |% {

    $logstep |% { $_ * $log }
    $log *= 10
}

# build log step names; 0 is the > range catchall
$lognames = @{}
foreach ($step in $logs) {

    if ($step -eq $logs[0]) {
        $lognames[$step] = "< $step"
    } else {
        $lognames[$step] = "$pstep - $($step - 1)"
    }
    $pstep = $step
}
$lognames[0] = "> $($logs[-1])"

# now bucket up VMs by flow rates
$qosbuckets = @{}
$qosbuckets[0] = 0
$logs |% {
    $qosbuckets[$_] = 0
}

Get-StorageQoSFlow |% {
    
    $found = $false
    foreach ($step in $logs) {

        if ($_.InitiatorIops -lt $step) {
            $qosbuckets[$step] += 1;
            $found = $true
            break
        }
    }
    # if not bucketed, it is greater than range
    if (-not $found) {
        $qosbuckets[0] += 1
    }
}

# find min/max buckets with nonzero counts, by $logs index
# this lets us present a continuous range, with interleaved zeroes
$bmax = -1
$bmin = -1
foreach ($i in 0..($logs.Count - 1)) {
    if ($qosbuckets[$logs[$i]]) {
    
        if ($bmin -lt 0) {
            $bmin = $i
        }
        $bmax = $i
    }
}
# raise max if we have > range
if ($qosbuckets[0]) {
    $bmax = $logs.Count - 1
}
$range = @($logs[$bmin..$bmax])
# add > range if needed, at end
if ($qosbuckets[0]) {
    $range += 0
}

$(foreach ($i in $range) {

    New-Object -TypeName psobject -Property @{
        Count = $qosbuckets[$i];
        IOPS = $lognames[$i]
    }    
}) | ft Count,IOPS