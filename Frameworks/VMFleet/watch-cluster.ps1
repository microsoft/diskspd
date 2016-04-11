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
    $SampleInterval = 2
)

# display name
# ctr name
# display order
# format string
# scalar multiplier

function new-ctr(
    [hashtable] $ctrs,
    [string] $displayname,
    [string] $setname,
    [string[]] $ctrname,
    [int] $order,
    [int] $width,
    [string] $fmt,
    [decimal] $multipler,
    [string] $aggregate,
    [boolean] $divider = $false
    )
{
    $o = New-Object psobject -Property @{
        'DisplayName' = $displayname;
        'SetName' = $setname;
        'CtrName' = $ctrname;
        'Order' = $order;
        'Width' = $width;
        'Fmt' = $fmt;
        'Multiplier' = $multipler;
        'Aggregate' = $aggregate;
        'Divider' = $divider
    }

    $ctrs[$o.DisplayName] = $o
}

function get-aggregate(
    $ctr
    )
{
    BEGIN {
        $n = 0
        $v = 0
    }
    PROCESS {
        $n += 1
        $v += $_

    }
    END {
        if ($n -gt 0) {
            switch ($ctr.aggregate) {
                'Sum' { $ctr.multiplier * $v }
                'Average' { $ctr.multiplier * $v / $n }
                default { throw "Internal Error: bad aggregate for $ctr" }
            }
        } else {
            $null
        }
    }
}

# get samples out of per-node hash of ctr hashes
function get-samples(
    [hashtable] $h,
    [string] $node,
    $col
    )
{
    foreach ($i in $col.CtrName) {
        $h[$node]["$($col.SetName)+$($i)"]
    }
}

$ctrs = @{}
new-ctr $ctrs "IOPS" "Cluster CSVFS" @("Reads/sec","Writes/sec") 1 12 '#,#' 1 'Sum'
new-ctr $ctrs "Reads" "Cluster CSVFS" @("Reads/sec") 2 12 '#,#' 1 'Sum'
new-ctr $ctrs "Writes" "Cluster CSVFS" @("Writes/sec") 3 12 '#,#' 1 'Sum'

new-ctr $ctrs "BW (MB/s)" "Cluster CSVFS" @("Read Bytes/sec","Write Bytes/sec") 4 12 '#,#' 0.000001 'Sum' $true
new-ctr $ctrs "Read" "Cluster CSVFS" @("Read Bytes/sec") 5 8 '#,#' 0.000001 'Sum'
new-ctr $ctrs "Write" "Cluster CSVFS" @("Write Bytes/sec") 6 8 '#,#' 0.000001 'Sum'

new-ctr $ctrs "Read Lat (ms)" "Cluster CSVFS" @("Avg. sec/Read") 7 15 '0.000' 1000 'Average' $true
new-ctr $ctrs "Write Lat" "Cluster CSVFS" @("Avg. sec/Write") 8 15 '0.000' 1000 'Average'


$order = $ctrs.values | sort -property order

$query = $ctrs.keys |? { $ctrs[$_].CtrName.Count -eq 1 } |% { "\$($ctrs[$_].SetName)(_Total)\$($ctrs[$_].CtrName[0])" } 
$cfmt = ,"{0,-16}" + ($order |% { if ($_.divider) { '| ' }; "{$($_.order),-$($_.width)}"}) -join ''
$vfmt = ,"{0,-16}" + ($order |% { if ($_.divider) { '| ' }; "{$($_.order),-$($_.width):$($_.fmt)}"}) -join ''

###

# clear any previous job instance
Get-Job -Name watch-cluster -ErrorAction SilentlyContinue | Stop-Job
Get-Job -Name watch-cluster -ErrorAction SilentlyContinue | Remove-Job

$j = icm -AsJob -JobName watch-cluster (Get-ClusterNode |? State -eq Up) -arg @(,$query,$SampleInterval) {

    param($query,$SampleInterval)

    # extract countersamples, the object does not survive transfer between powershell sessions
    # extract as a list, not as the individual counters
    get-counter -Continuous -SampleInterval $SampleInterval $($query |% { $_ -replace 'COMPUTERNAME',$env:COMPUTERNAME }) |% {
        ,$_.countersamples
    }
}

# hash of most recent samples/node
$samples = @{}

while ($true) {

    sleep $SampleInterval

    # receive updates into the per-node hash
    # this automatically discards all but the latest sample from a given node
    # if multiple are received due to skew
    $j | receive-job |% {
        $samples[$_.PsComputerName] = $_
    }

    # now process samples into per-node hashes of set/ctr containing lists of the
    # cooked values acrosss the (possible) multiple instances
    $psamples = @{}

    foreach ($node in $samples.keys) {

        $nsamples = @{}
        $samples[$node] |% {

            ($setinst,$ctr) = $($_.path -split '\\')[3..4]
            $set = ($setinst -split '\(')[0]
            $nsamples["$set+$ctr"] += ,$_.cookedvalue
        }

        $psamples[$node] = $nsamples
    }

    
    # aggregate each column across all nodes
    $totline =  $vfmt -f $(
        "Total"
        foreach ($col in $order) {

            # average aggreate doesn't work across nodes (and/or make this optin)
            if ($col.aggregate -ne 'Average') {
                $(foreach ($node in $psamples.keys) {
                    get-samples $psamples $node $col

                }) | get-aggregate $col
            } else {
                $null
            }
        }
    )

    # individual nodes
    $lines = foreach ($node in $psamples.keys | sort) {
        $vfmt -f $(
            $node
            foreach ($col in $order) {
                get-samples $psamples $node $col | get-aggregate $col
            }
        )
    }

    # clear and dump headers/lines
    cls

    write-host ($cfmt -f (,"" + $order.DisplayName))
    write-host -fore green $totline
    $lines |% { write-host $_ }
}
