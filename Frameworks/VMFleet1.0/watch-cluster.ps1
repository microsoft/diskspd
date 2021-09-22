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
    $Cluster = ".",
    $SampleInterval = 2,
    [ValidateSet("CSV FS","SSB Cache","SBL","SBL Local","SBL Remote","SBL*","S2D BW","Hyper-V LCPU","SMB SRV","SMB Transport","*")]
    [string[]] $Sets = "CSV FS",
    $Log = $null
)

if ($null -ne $log) {
    del -Force $log -ErrorAction SilentlyContinue
}

function write-log(
    [string[]] $str
    )
{
    if ($null -ne $log) {
        $str |% {
            "$(get-date) $_" | Out-File -Append -FilePath $log -Width 9999 -Encoding ascii
        }
    }
}

# display name
# ctr name
# display order
# format string
# scalar multiplier

class CounterColumn {

    [string] $displayname
    [string] $setname
    [string[]] $ctrname
    [int] $width
    [string] $fmt
    [decimal] $multiplier
    [ValidateSet("Average","AverageAggregate","Sum")][string] $aggregate
    [boolean] $divider

    CounterColumn(
        [string] $displayname,
        [string] $setname,
        [string[]] $ctrname,
        [int] $width,
        [string] $fmt,
        [decimal] $multiplier,
        [string] $aggregate,
        [boolean] $divider
    )
    {
        $this.displayname = $displayname
        $this.setname = $setname
        $this.ctrname = $ctrname
        $this.width = $width
        $this.fmt = $fmt
        $this.multiplier = $multiplier
        $this.aggregate = $aggregate
        $this.divider = $divider

        if ($this.width -lt $this.displayname.length + 1) {
            $this.width = $this.displayname.length + 1
        }
    }
}

class CounterColumnSet {

    [CounterColumn[]] $columns
    [string[]] $counters
    [string] $topfmt
    [string] $linfmt
    [string] $name

    [string] $totalline
    [string[]] $nodelines

    CounterColumnSet($name)
    {
        $this.columns = $null
        $this.name = $name
    }

    [void] Add([CounterColumn] $c)
    {
        $this.columns += $c
    }

    [void] Seal()
    {
        # assemble the top-line fmt and per-line fmt strings
        # top-line is just strings/width
        # per-line adds the (likely numeric) format specifier
        $n = 1
        $this.topfmt = $this.linfmt = "{0,-16}"
        foreach ($col in $this.columns) {
            $str = ''
            if ($col.divider) {
                $str += '| '
            }
            $str += "{$n,-$($col.width)"
            $this.topfmt += "$str}"
            $this.linfmt += "$($str):$($col.fmt)}"

            $n += 1
        }

        # assemble the list of counter instances that will be needed
        # note that some may be internally aggregated (i.e., total = read + write)
        # in cases where a counterset does not provide an explicit total
        $this.counters = ($this.columns |% {
            $s = $_.setname
            $_.ctrname |% { "\$($s)(_Total)\$($_)" }
        } | group -NoElement).Name
    }

    [void] DisplayPre(
        [hashtable] $samples,
        [hashtable] $psamples
        )
    {
        # aggregate each column across all live sampled nodes
        $this.totalline =  $this.linfmt -f $(
            "Total"
            foreach ($col in $this.columns) {

                # average aggreate doesn't work across nodes if the base is not consistent
                # for instance: cannot average latency safely, but can average cpu utilization
                if ($col.aggregate -ne 'Average' -or $col.aggregate -eq 'AverageAggregate') {
                    $(foreach ($node in $psamples.keys) {
                        get-samples $psamples $node $col
                    }) | get-aggregate $col
                } else {
                    $null
                }
            }
        )

        # individual nodes
        # flags downed/non-responsive nodes by noting which are not
        # present in the processed samples
        $this.nodelines = foreach ($node in $samples.keys | sort) {
            if ($psamples.ContainsKey($node)) {
                $this.linfmt -f $(
                    $node
                    foreach ($col in $this.columns) {
                        $s = get-samples $psamples $node $col
                        if ($null -ne $s) {
                            $a = $s | get-aggregate $col
                            $a
                        } else {
                            "-"
                        }
                    }
                )
            } else {
                $this.topfmt -f $(
                    $node
                    0..($this.columns.count - 1) |% { "-" }
                )
            }
        }
    }

    [void] Display()
    {
        write-host ($this.topfmt -f (,$this.name + $this.columns.displayname))
        write-host -fore green $this.totalline
        $this.nodelines |% { write-host $_ }
    }
}

function get-aggregate(
    [CounterColumn] $col
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
            switch -wildcard ($col.aggregate) {
                'Sum' {
                    #write-host $col.displayname $col.multipler $v
                    $col.multiplier * $v
                }
                'Average*' {
                    #write-host $col.displayname $col.multiplier $v $n
                    $col.multiplier * $v / $n
                }
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
    [CounterColumn] $col
    )
{
    foreach ($i in $col.ctrname) {

        $k = "$($col.setname)+$($i)"

        if ($h.ContainsKey($node)) {
            if ($h[$node].ContainsKey($k)) {
                $h[$node][$k]
            } else {
                write-log "missing $node[$k] : $($h[$node].Keys.Count) total keys"
            }
        } else {
            write-log "missing $node"
        }
    }
}

$allctrs = @()

###
$c = [CounterColumnSet]::new("CSV FS")
$c.Add([CounterColumn]::new("IOPS", "Cluster CSVFS", @("Reads/sec","Writes/sec"), 12, '#,#', 1, 'Sum', $false))
$c.Add([CounterColumn]::new("Reads", "Cluster CSVFS", @("Reads/sec"), 12, '#,#', 1, 'Sum',  $false))
$c.Add([CounterColumn]::new("Writes", "Cluster CSVFS", @("Writes/sec"), 12, '#,#', 1, 'Sum',  $false))

$c.Add([CounterColumn]::new("BW (MB/s)", "Cluster CSVFS", @("Read Bytes/sec","Write Bytes/sec"), 13, '#,#', 0.000001, 'Sum', $true))
$c.Add([CounterColumn]::new("Read", "Cluster CSVFS", @("Read Bytes/sec"), 8, '#,#', 0.000001, 'Sum',  $false))
$c.Add([CounterColumn]::new("Write", "Cluster CSVFS", @("Write Bytes/sec"), 8, '#,#', 0.000001, 'Sum', $false))

$c.Add([CounterColumn]::new("Read Lat (ms)", "Cluster CSVFS", @("Avg. sec/Read"), 15, '0.000', 1000, 'Average', $true))
$c.Add([CounterColumn]::new("Write", "Cluster CSVFS", @("Avg. sec/Write"), 8, '0.000', 1000, 'Average', $false))

$c.Add([CounterColumn]::new("Read QAvg", "Cluster CSVFS", @("Avg. Read Queue Length"), 11, '0.000', 1, 'Average', $true))
$c.Add([CounterColumn]::new("Write", "Cluster CSVFS", @("Avg. Write Queue Length"), 8, '0.000', 1, 'Average', $false))

$c.Seal()
$allctrs += $c

###
$c = [CounterColumnSet]::new("SSB Cache")

$c.Add([CounterColumn]::new("Hit/Sec", "Cluster Storage Hybrid Disks", @("Cache Hit Reads/Sec"), 12, '#,#', 1, 'Sum', $false))
$c.Add([CounterColumn]::new("Miss/Sec", "Cluster Storage Hybrid Disks", @("Cache Miss Reads/Sec"), 12, '#,#', 1, 'Sum', $false))
$c.Add([CounterColumn]::new("Remap/Sec" ,"Cluster Storage Cache Stores", @("Page ReMap/sec"), 12, '#,#', 1, 'Sum', $false))

$c.Add([CounterColumn]::new("Cache (MB/s)", "Cluster Storage Hybrid Disks", @("Cache Populate Bytes/sec","Cache Write Bytes/sec"), 13, '#,#', 0.000001, 'Sum', $true))
$c.Add([CounterColumn]::new("RdPop", "Cluster Storage Hybrid Disks", @("Cache Populate Bytes/sec"), 8, '#,#', 0.000001, 'Sum', $false))
$c.Add([CounterColumn]::new("WrPop", "Cluster Storage Hybrid Disks", @("Cache Write Bytes/sec"), 8, '#,#', 0.000001, 'Sum', $false))

$c.Add([CounterColumn]::new("Destage (MB/s)", "Cluster Storage Cache Stores", @("Destage Bytes/sec"), 15, '#,#', 0.000001, 'Sum', $true))
$c.Add([CounterColumn]::new("Update", "Cluster Storage Cache Stores", @("Update Bytes/sec"), 7, '#,#', 0.000001, 'Sum', $false))

$c.Add([CounterColumn]::new("Total (Pgs)", "Cluster Storage Cache Stores", @("Cache Pages"), 11, '0.00E+0', 1, 'Sum', $true))
$c.Add([CounterColumn]::new("Standby", "Cluster Storage Cache Stores", @("Cache Pages StandBy"), 9, '0.00E+0', 1, 'Sum', $false))
$c.Add([CounterColumn]::new("L0", "Cluster Storage Cache Stores", @("Cache Pages StandBy L0"), 9, '0.00E+0', 1, 'Sum', $false))
$c.Add([CounterColumn]::new("L1", "Cluster Storage Cache Stores", @("Cache Pages StandBy L1"), 9, '0.00E+0', 1, 'Sum', $false))
$c.Add([CounterColumn]::new("L2", "Cluster Storage Cache Stores", @("Cache Pages StandBy L2"), 9, '0.00E+0', 1, 'Sum', $false))
$c.Add([CounterColumn]::new("Dirty", "Cluster Storage Cache Stores", @("Cache Pages Dirty"), 9, '0.00E+0', 1, 'Sum', $false))

$c.Seal()
$allctrs += $c

###

foreach ($subset in '','Local','Remote') {
    $name = 'SBL'
    $prefix = ''
    if ($subset.Length) {
        $name += " $subset"
        $prefix = "$($subset): "
    }

    $c = [CounterColumnSet]::new($name)

    $c.Add([CounterColumn]::new("IOPS", "Cluster Disk Counters", @(($prefix + "Read/sec"),($prefix + "Writes/sec")), 12, '#,#', 1, 'Sum', $false))
    $c.Add([CounterColumn]::new("Reads", "Cluster Disk Counters", @($prefix + "Read/sec"), 12, '#,#', 1, 'Sum', $false))
    $c.Add([CounterColumn]::new("Writes", "Cluster Disk Counters", @($prefix + "Writes/sec"), 12, '#,#', 1, 'Sum', $false))

    $c.Add([CounterColumn]::new("BW (MB/s)", "Cluster Disk Counters", @(($prefix + "Read - Bytes/sec"),($prefix + "Write - Bytes/sec")), 13, '#,#', 0.000001, 'Sum', $true))
    $c.Add([CounterColumn]::new("Read", "Cluster Disk Counters", @($prefix + "Read - Bytes/sec"), 8, '#,#', 0.000001, 'Sum', $false))
    $c.Add([CounterColumn]::new("Write", "Cluster Disk Counters", @($prefix + "Write - Bytes/sec"), 8, '#,#', 0.000001, 'Sum', $false))

    $c.Add([CounterColumn]::new("Read Lat (ms)", "Cluster Disk Counters", @($prefix + "Read Latency"), 15, '0.000', 1000, 'Average', $true))
    $c.Add([CounterColumn]::new("Write", "Cluster Disk Counters", @($prefix + "Write Latency"), 8, '0.000', 1000, 'Average', $false))

    $c.Add([CounterColumn]::new("Read QAvg", "Cluster Disk Counters", @($prefix + "Read Avg. Queue Length"), 11, '0.000', 1, 'Average', $true))
    $c.Add([CounterColumn]::new("Write", "Cluster Disk Counters", @($prefix + "Write Avg. Queue Length"), 8, '0.000', 1, 'Average', $false))

    $c.Seal()
    $allctrs += $c
}

###
$c = [CounterColumnSet]::new("SMB SRV")

$c.Add([CounterColumn]::new("IOPS", "SMB Server Shares", @("Data Requests/sec"), 12, '#,#', 1, 'Sum', $false))
$c.Add([CounterColumn]::new("Reads", "SMB Server Shares", @("Read Requests/sec"), 12, '#,#', 1, 'Sum', $false))
$c.Add([CounterColumn]::new("Writes", "SMB Server Shares", @("Write Requests/sec"), 12, '#,#', 1, 'Sum', $false))

$c.Add([CounterColumn]::new("Data BW (MB/s)", "SMB Server Shares", @("Data Bytes/sec"), 13, '#,#', 0.000001, 'Sum', $true))
$c.Add([CounterColumn]::new("Read", "SMB Server Shares", @("Read Bytes/sec"), 8, '#,#', 0.000001, 'Sum', $false))
$c.Add([CounterColumn]::new("Write", "SMB Server Shares", @("Write Bytes/sec"), 8, '#,#', 0.000001, 'Sum', $false))

$c.Add([CounterColumn]::new("Total BW (MB/s)", "SMB Server Shares", @("Transferred Bytes/sec"), 13, '#,#', 0.000001, 'Sum', $true))
$c.Add([CounterColumn]::new("Rcv", "SMB Server Shares", @("Received Bytes/sec"), 8, '#,#', 0.000001, 'Sum', $false))
$c.Add([CounterColumn]::new("Snd", "SMB Server Shares", @("Sent Bytes/sec"), 8, '#,#', 0.000001, 'Sum', $false))

$c.Seal()
$allctrs += $c

##
$c = [CounterColumnSet]::new("S2D BW")

$c.Add([CounterColumn]::new("CSV (MB/s)", "Cluster CSVFS", @("Read Bytes/sec","Write Bytes/sec"), 10, '#,#', 0.000001, 'Sum', $false))
$c.Add([CounterColumn]::new("Read", "Cluster CSVFS", @("Read Bytes/sec"), 8, '#,#', 0.000001, 'Sum', $false))
$c.Add([CounterColumn]::new("Write", "Cluster CSVFS", @("Write Bytes/sec"), 8 ,'#,#', 0.000001, 'Sum', $false))

$c.Add([CounterColumn]::new("SBL (MB/s)", "Cluster Disk Counters", @("Read - Bytes/sec","Write - Bytes/sec"), 10, '#,#', 0.000001, 'Sum', $true))
$c.Add([CounterColumn]::new("Read", "Cluster Disk Counters", @("Read - Bytes/sec"), 8, '#,#', 0.000001, 'Sum', $false))
$c.Add([CounterColumn]::new("Write", "Cluster Disk Counters", @("Write - Bytes/sec"), 8, '#,#', 0.000001, 'Sum', $false))

$c.Add([CounterColumn]::new("Cache (MB/s)", "Cluster Storage Hybrid Disks", @("Cache Hit Read Bytes/sec","Cache Write Bytes/sec"), 12, '#,#', 0.000001, 'Sum', $true))
$c.Add([CounterColumn]::new("Read", "Cluster Storage Hybrid Disks", @("Cache Hit Read Bytes/sec"), 8, '#,#', 0.000001, 'Sum', $false))
$c.Add([CounterColumn]::new("Write", "Cluster Storage Hybrid Disks", @("Cache Write Bytes/sec"), 8, '#,#', 0.000001, 'Sum', $false))

$c.Add([CounterColumn]::new("Disk (MB/s)", "Cluster Storage Hybrid Disks", @("Disk Read Bytes/sec","Disk Write Bytes/sec"), 11, '#,#', 0.000001, 'Sum', $true))
$c.Add([CounterColumn]::new("Read", "Cluster Storage Hybrid Disks", @("Disk Read Bytes/sec"), 8, '#,#', 0.000001, 'Sum', $false))
$c.Add([CounterColumn]::new("Write", "Cluster Storage Hybrid Disks", @("Disk Write Bytes/sec"), 8, '#,#', 0.000001, 'Sum', $false))

$c.Seal()
$allctrs += $c

##
$c = [CounterColumnSet]::new("Hyper-V LCPU")
$c.Add([CounterColumn]::new("Logical Total%", "Hyper-V Hypervisor Logical Processor", @("% Total Run Time"), 8, "0.00", 1, 'AverageAggregate', $false))
$c.Add([CounterColumn]::new("Guest%", "Hyper-V Hypervisor Logical Processor", @("% Guest Run Time"), 8, "0.00", 1, 'AverageAggregate', $false))
$c.Add([CounterColumn]::new("Hypervisor%", "Hyper-V Hypervisor Logical Processor", @("% Hypervisor Run Time"), 13, "0.00", 1, 'AverageAggregate', $false))

$c.Add([CounterColumn]::new("Root Total%", "Hyper-V Hypervisor Root Virtual Processor", @("% Total Run Time"), 12, "0.00", 1, 'AverageAggregate', $true))
$c.Add([CounterColumn]::new("Guest%", "Hyper-V Hypervisor Root Virtual Processor", @("% Guest Run Time"), 8, "0.00", 1, 'AverageAggregate', $false))
$c.Add([CounterColumn]::new("Hypervisor%", "Hyper-V Hypervisor Root Virtual Processor", @("% Hypervisor Run Time"), 12, "0.00", 1, 'AverageAggregate', $false))
$c.Add([CounterColumn]::new("Remote%", "Hyper-V Hypervisor Root Virtual Processor", @("% Remote Run Time"), 7, "0.00", 1, 'AverageAggregate', $false))

$c.Seal()
$allctrs += $c

##
$c = [CounterColumnSet]::new("SMB Transport")
$c.add([CounterColumn]::new("Read IOPS", "SMB Client Shares", @("Read Requests/sec"), 11, "#,#", 1, 'Sum', $false))
$c.add([CounterColumn]::new("Write", "SMB Client Shares", @("Write Requests/sec"), 8, "#,#", 1, 'Sum', $false))

$c.add([CounterColumn]::new("RDMA Read", "SMB Client Shares", @("Read Requests transmitted via SMB Direct/sec"), 11, "#,#", 1, 'Sum', $true))
$c.add([CounterColumn]::new("Write", "SMB Client Shares", @("Write Requests transmitted via SMB Direct/sec"), 8, "#,#", 1, 'Sum', $false))

$c.Seal()
$allctrs += $c

##
if ($Sets.Count -eq 1 -and $Sets[0] -eq '*') {
    $ctrs = $allctrs
} else {
    $ctrs = $Sets |% {
        $s = $_
        $allctrs |? { $_.name -like $s } # allows the SBL* wildcard
    }
}

function start-sample(
    [CounterColumnSet[]] $ctrs,
    [int] $SampleInterval
    )
{
    # clear any previous job instance
    Get-Job -Name watch-cluster -ErrorAction SilentlyContinue | Stop-Job
    Get-Job -Name watch-cluster -ErrorAction SilentlyContinue | Remove-Job

    # flatten list of counters and uniquify for the total counter set
    # some display counter sets may repeat specific values (which is fine)
    $counters = ($ctrs.counters |% { $_ |% { $_ }} | group -NoElement).Name

    icm -AsJob -JobName watch-cluster (Get-ClusterNode -Cluster $Cluster) {

        # extract countersamples, the object does not survive transfer between powershell sessions
        # extract as a list, not as the individual counters
        get-counter -Continuous -SampleInterval $using:SampleInterval $using:ctrs.counters |% {
            ,$_.countersamples
        }
    }
}

# start the first sample job and allow frame draw the first time through
$j = start-sample $ctrs $SampleInterval
$downtime = $null
$skipone = $false
$loops = 0
$restart = $false

# hash of most recent samples/node
$samples = @{}
Get-ClusterNode -Cluster $Cluster |% { $samples[$_.Name] = $null }

while ($true) {

    if (-not $restart) {
        Start-Sleep -Seconds $SampleInterval

        # sleep again if needed to prime the sample pipeline;
        # there are no samples if we just restarted the sampling jobs
        if ($skipone) {
            $skipone = $false
            continue
        }

        # receive updates into the per-node hash
        foreach ($child in $j.ChildJobs) {
            $samples[$child.Location] = $child | receive-job -ErrorAction SilentlyContinue
        }

        # null out downed nodes and remember first time we saw one drop out
        $down = 0
        $j.ChildJobs |? State -ne Running |% {
            $samples[$_.Location] = $null
            $down += 1
        }
        if ($down -and $null -eq $downtime) {
            $downtime = get-date
        }

        # if everything is down, we will attempt restart
        if ($down -eq $j.ChildJobs.Count) {
            $restart = $true
            break
        }
    }

    # if explicit restart is required, or it has been 30 seconds with a downed node, restart the jobs to retry
    if ($restart -or ($null -ne $downtime -and ((get-date)-$downtime).totalseconds -gt 30)) {
        $j | stop-job
        $j | remove-job
        $j = start-sample $ctrs $SampleInterval

        # force gc to clear out accumulated job state quickly
        [system.gc]::Collect()

        $downtime = $null
        $skipone = $true
        $restart = $false
        continue
    }

    # now process samples into per-node hashes of set/ctr containing lists of the
    # cooked values acrosss the (possible) multiple instances
    $psamples = @{}
    foreach ($node in $samples.keys) {

        if ($samples[$node]) {

            $nsamples = @{}

            # flatten samples - if we are lagging, we'll have a list
            # of consecutive (increasing by timestamp) samples
            # we could try to be more efficient by dumping all but the
            # final sample, but later ...
            $samples[$node] |% { $_ } |% {

                ($setinst,$ctr) = $($_.path -split '\\')[3..4]
                $set = ($setinst -split '\(')[0]

                $k = "$set+$ctr"
                $nsamples[$k] = $_.cookedvalue
            }

            $psamples[$node] = $nsamples
        }
    }

    # post-process the samples into the counterset, then clear and dump
    $ctrs.DisplayPre($samples, $psamples)
    Clear-Host
    $drawsep = $false
    $ctrs |% {
        if ($drawsep) {
            write-host -fore Green $('-'*20)
        }
        $drawsep = $true
        $_.Display()

    }

    # restart the jobs every so many loops to prevent resource growth
    $loops += 1
    if ($loops -gt 100) {
        $loops = 0
        $restart = $true
    }
}