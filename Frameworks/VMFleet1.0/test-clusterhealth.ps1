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
    [switch] $CleanOperationalIssues = $false
    )

function new-namedblock(
    [string] $name,
    [scriptblock] $block,
    [switch] $nullpass = $true,
    [int] $mustbe = -1
    )
{
    new-object psobject -Property @{
        'Name' = $name;
        'Block' = $block;
        'NullPass' = $nullpass;
        'MustBe' = $mustbe
    }
}

function write-blocktitle(
    [string[]] $s
    )
{
    write-host -fore cyan ('*'*20) $s
}

function display-jobs()
{
    BEGIN { $j = @() }
    PROCESS { $j += $_ }
    END {
        # consume job results
        $null = $j | wait-job
        $j | sort -Property Name |% {
            write-blocktitle $_.Name,('({0:F1}s)' -f ($_.PsEndTime - $_.PsBeginTime).TotalSeconds)
            $_ | receive-job
            $_ | remove-job
        }
    }
}

# script block containers for helper functions

$evfns = {

    function get-fltevents(
        [decimal] $timedeltams,
        [string] $provider,
        [string[]] $evid,
        [scriptblock] $flt = { $true },
        [string] $source = $null
        )
    {
        # simple query vs. a provider for a single event within some timedelta of current (in ms)
        # optional addition of a source provider name filter (for system log filtering)
$qstr = @"
<QueryList>
    <Query Id="0" Path="_PROV_">
    <Select Path="_PROV_">*[System[_SOURCE_(_EVENTS_) and TimeCreated[timediff(@SystemTime) &lt;= _MS_]]]</Select>
    </Query>
</QueryList>
"@

$srcstr = @"
Provider[@Name='_SOURCE_'] and 
"@

        $events = ($evid |% {
            "EventID=$_"
        }) -join " or "

        $query = $qstr -replace '_MS_',$timedeltams -replace '_PROV_',$provider -replace '_EVENTS_',$events
        if ($source) {
            $query = $query -replace '_SOURCE_',($srcstr -replace '_SOURCE_',$source)
        } else {
            $query = $query -replace '_SOURCE_',''
        }

        Get-WinEvent -FilterXml $query -ErrorAction SilentlyContinue | Where-Object -FilterScript $flt
    }
}

$fns = {

    function do-clustersymmetry(
        [object] $gather,
        [object[]] $filters,
        [switch] $saygather = $false
        )
    {
        # deserialization note: the scriptblocks on the inputs are downconverted to strings
        # so as a result we must reinstantiate

        # assert all nodes agree on object counts from the provided gather element
        $data = icm (get-clusternode |? State -eq up) ([scriptblock]::create($gather.block))

        if ($saygather) {
            write-host -ForegroundColor yellow ('*'*15) $gather.name
        }

        foreach ($f in $filters) {

            write-host -fore yellow ('*'*10) $f.name
            $r = $data | where-object -FilterScript ([scriptblock]::create($f.block))

            # group results by node
            $nodeg = $r | group -property pscomputername -NoElement

            # regular symmetery
            # if grouping by count (per node) yields more than one element, we know some nodes have different counts
            # i.e., not all are 60: perhaps one is 58, etc.
            # this is always failure.
            if ($nodeg -ne $null -and ($nodeg | group -Property Count | measure).count -ne 1) {
                write-host -ForegroundColor Red Fail
                $nodeg | sort -Property Name,Count | ft -autosize
            } else {
                if ($nodeg -ne $null) {
                    # if no enforced count or count is correct, pass
                    if ($f.mustbe -lt 0 -or $f.mustbe -eq $nodeg[0].Count) {
                        write-host -ForegroundColor Green Pass with $nodeg[0].Count per node
                    } else {
                        # enforced count not correct
                        write-host -ForegroundColor Red Fail - required count of $f.mustbe not consistent on each node
                        $nodeg | sort -Property Name,Count | ft -autosize
                    }
                } elseif ($f.nullpass) {
                    write-host -ForegroundColor Green Pass with none on any node
                } else {
                    write-host -ForegroundColor Red Fail with none on any node
                }
            }
        }
    }
}

# Detect RDMA its type (by manufacturer) so that, if needed, we can assert QoS/Cos for RoCE

$netadapters = Get-NetAdapterRdma |? Enabled | Get-NetAdapter

$rdma = $false
$roce = $false
$rocematch = 'Mellanox'

if ($netadapters) {
    write-host -fore green Detected RDMA adapters: will require RDMA
    $rdma = $true

    # will need a tweak as additional non-Mellanox RoCE arrive (and/or if IB)
    $qroce = $netadapters |? DriverProvider -match $rocematch
    if ($qroce) {
        $drvdesc = $qroce[0].DriverDescription

        write-host -fore green Detected $qroce[0].DriverProvider RDMA adapters: will require RoCE configuration
        write-host -fore green Adapter Description: $drvdesc
        $roce = $true
    }
}

### Basic Health Checks: serialized

$j = @()

$j += start-job -Name 'Basic Health Checks' {

    # nodes up
    $cn = Get-ClusterNode

    if ($cn.count -eq $($cn |? State -eq Up).count) {
        write-host -ForegroundColor Green All cluster nodes Up
    } else {
        write-host -ForegroundColor Red Following cluster nodes not Up:
        $cn |? State -ne Up
    }

    # node uptime
    $o = icm ($cn |? State -eq Up) {
        $w = gwmi win32_operatingsystem
        $w.ConvertToDateTime($w.localdatetime) - $w.ConvertToDateTime($w.lastbootuptime)
    }

    $reboots = $o |? TotalHours -lt 1

    if ($reboots.length -and $reboots.length -ne $o.length) {
        write-host -ForegroundColor Yellow WARNING: $reboots.length nodes have rebooted in the last hour. Ensure that
        write-host -ForegroundColor Yellow `t no unexpected events are occuring in the cluster.
    }

    write-host -ForegroundColor Green Cluster node uptime:
    $o | sort PsComputerName | ft PsComputerName,@{ Label="Uptime"; Expression={"{0}d:{1:00}h:{2:00}m.{3:00}s" -f $_.Days,$_.Hours,$_.Minutes,$_.Seconds}}

    # subsystem check
    $ss = Get-StorageSubSystem |? Model -eq 'Clustered Windows Storage'

    if (($ss | measure).count -ne 1) {
        write-host -ForegroundColor Red Expected single clustered storage subsystem, found:
        $ss | ft -autosize
        return
    }

    $ssuh = $ss |? HealthStatus -ne Healthy

    if ($ssuh) {
        write-host -ForegroundColor Red WARNING: clustered storage subsystem is not healthy
        $ssuh | ft -AutoSize

        write-host -ForegroundColor Red Output of Debug-StorageSubSystem follows
        $ssuh | Debug-StorageSubSystem | fl
    } else {
        write-host -ForegroundColor Green Clustered storage subsystem Healthy
    }

    # pool health
    $p = $ss | Get-StoragePool |? IsPrimordial -ne $true |? HealthStatus -ne Healthy

    if ($p -eq $null) {
        write-host -ForegroundColor Green All pools Healthy
    } else {
        write-host -ForegroundColor Red Following pools not Healthy:
        $p | ft -autosize
    }
}

# vd health

$j += start-job -name 'Virtual Disk Health' {

    $ss = Get-StorageSubSystem |? Model -eq 'Clustered Windows Storage'

    $vd = $ss | Get-VirtualDisk |? HealthStatus -ne Healthy

    if ($vd -eq $null) {
        write-host -fore green All operational virtual disks Healthy
    } else {
        write-host -fore red Following virtual disks not Healthy:
        $vd | ft -autosize
    }
}

# disk state

$j += start-job -name 'Physical Disk Health' {
    $pd = Get-StorageSubSystem |? Model -eq 'Clustered Windows Storage' | Get-PhysicalDisk

    $nonauto = $pd |? Usage -notmatch 'Journal|Auto-Select'
    if ($nonauto) {
        write-host -fore yellow WARNING: there are physical disks which are not auto-select/journal for usage.
        write-host -fore yellow `t It is possible that while storage resilience has been restored from
        write-host -fore yellow `t a failure, it is no longer evenly distributed between cluster nodes.
        write-host -fore yellow `t Consider recovering before doing performance/operational work.
        $nonauto | ft -autosize
    } else {
        write-host -fore green All physical disks are in normal auto-select or journal state
    }
}

# consolidated op issues - should logically split?

$j += start-job -name 'Operational Issues and Storage Jobs' -ArgumentList $CleanOperationalIssues {

    param( $CleanOperationalIssues )

    $ev = icm (get-clusternode) {
        get-winevent -LogName Microsoft-Windows-Storage-Storport/Operational |? Id -eq 502 |% {
            $ex = [xml]$_.ToXml()
            $guid = ($ex.Event.EventData.Data |? Name -eq ClassDeviceGuid).'#text'
            $_ | Add-Member -NotePropertyName DeviceGuid -NotePropertyValue $guid -PassThru
        }
    }
    if ($ev) {
        write-host -fore yellow WARNING: unresponsive device events have been logged by storport.
        write-host -fore yellow `tThese may correspond to retired devices, and should be investigated.
        $ev | ft -autosize PsComputerName,TimeCreated,Id,DeviceGuid,Message

        write-host -fore yellow Corresponding devices by Device GUID:
        $d = Get-StorageSubSystem Cluster* | Get-StoragePool |? IsPrimordial -eq $false | Get-PhysicalDisk
        $ev |% {
            $d |? ObjectId -match "PD:$($_.DeviceGuid)"
        } | ft -AutoSize
    }

    # look for livekernelreport and/or bugcheck dumps

    $dmps = icm (get-clusternode |? State -eq Up) {
        
        $obj = @()

        $obj += dir $($env:windir + "\livekernelreports")
        $obj += dir $($env:windir + "\minidump") -ErrorAction SilentlyContinue
        $obj += dir $($env:windir + "\memory.dmp") -ErrorAction SilentlyContinue

        if ($using:CleanOperationalIssues -and $obj.count -gt 0) {
            $obj | del -Force -Recurse -ErrorAction SilentlyContinue
        }

        $obj

    } | sort -property PsParentPath,LastWriteTime,PsComputerName

    if ($dmps) {
        if ($CleanOperationalIssues) {
            write-host -fore red NOTE: the following failure reports were forcibly removed
        } else {
            write-host -fore yellow WARNING: there are failure reports that may require triage
        }
        
        $dmps | ft -AutoSize
    }

    # Storage Jobs

    $sj = get-storagejob
    if ($sj |? JobState -ne Completed) {
        write-host -ForegroundColor red WARNING: there are active storage jobs running. Investigate the root cause before continuing.
        $sj | ft -autosize
    } else {
        write-host -fore green No storage rebuild or regeneration jobs are active
    }
}

# SMB Connectivity Error Check

$fltblk = {

    param( $ev, $warncol, $warn )

    $r = icm (get-clusternode |? State -eq Up) -ArgumentList @((get-command get-fltevents), $ev) {

        param($fn, $ev)

        set-item -path function:\$($fn.name) -value $fn.definition

        $flttcp = {
            # report tcp (type 1) connectivity events
            $x = [xml] $_.ToXml()
            [int]($x.Event.EventData.Data |? Name -eq 'ConnectionType').'#text' -eq 1
        }

        $fltrdma = {
            # report rdma (type 2) connectivity events
            $x = [xml] $_.ToXml()
            [int]($x.Event.EventData.Data |? Name -eq 'ConnectionType').'#text' -eq 2
        }

        $last5 = (1000*60*5)
        $lasthour = (1000*60*60)
        $lastday = (1000*60*60*24)

        new-object psobject -Property @{
            'RDMA Last5Min' = (get-fltevents -flt $fltrdma -timedeltams $last5 -provider "Microsoft-Windows-SmbClient/Connectivity" -evid $ev).count;
            'RDMA LastHour' = (get-fltevents -flt $fltrdma -timedeltams $lasthour -provider "Microsoft-Windows-SmbClient/Connectivity" -evid $ev).count;
            'RDMA LastDay' = (get-fltevents -flt $fltrdma -timedeltams $lastday -provider "Microsoft-Windows-SmbClient/Connectivity" -evid $ev).count;
            'TCP Last5Min' = (get-fltevents -flt $flttcp -timedeltams $last5 -provider "Microsoft-Windows-SmbClient/Connectivity" -evid $ev).count;
            'TCP LastHour' = (get-fltevents -flt $flttcp -timedeltams $lasthour -provider "Microsoft-Windows-SmbClient/Connectivity" -evid $ev).count;
            'TCP LastDay' = (get-fltevents -flt $flttcp -timedeltams $lastday -provider "Microsoft-Windows-SmbClient/Connectivity" -evid $ev).count;
        }
    }

    $hdr = (($r[0] | gm -MemberType NoteProperty |? Definition -like 'int*').Name | sort)
    $rdmafail = ($r |% { $row = $_; $hdr |? {$_ -like 'RDMA*' } |% { $row.$_ }} | measure -sum).sum -ne 0

    if ($rdmafail) {
        write-host -ForegroundColor $warncol $warn
    }

    $r | sort -Property PsComputerName | ft -Property (@('PsComputerName') + $hdr)
}

$w = @"
WARNING: the SMB Client is receiving RDMA disconnects. This is an error whose root"
`t cause may be PFC/CoS misconfiguration (RoCE) on hosts or switches, physical"
`t issues (ex: bad cable), switch or NIC firmware issues, and will lead to severely"
`t degraded performance. Additional triage is included in other tests."
"@

$j += start-job -name 'SMB Connectivity Error Check - Disconnect Failures' -ArgumentList 30804,([ConsoleColor]'Red'),$w -InitializationScript $evfns $fltblk

$w = @"
WARNING: the SMB Client is receiving RDMA connect errors. This is an error whose root
`t cause may be actual lack of connectivity or fundamental problems with the RDMA
`t network fabric. Please inspect especially if in the Last5 bucket.
"@

$j += start-job -name 'SMB Connectivity Error Check - Connect Failures' -ArgumentList 30803,([ConsoleColor]'Yellow'),$w -InitializationScript $evfns $fltblk

if ($roce) {

    $j += start-job -name 'RoCE: Mellanox Disable Check' -InitializationScript $evfns {

        $r = icm (get-clusternode |? State -eq Up) -ArgumentList (get-command get-fltevents) {

            param($fn)

            set-item -path function:\$($fn.name) -value $fn.definition

            $r = get-fltevents -timedeltams (1000*60*60*24*30) -provider 'System' -source 'mlx4eth63' -evid 35
        }

        if ($r) {

            write-host -ForegroundColor red WARNING: Mellanox indicates that RDMA has been disabled due to mis/non-configuration
            write-host -ForegroundColor red `t of Priority Flow Control at some point in the past 30 days. Unless this has been recently
            write-host -ForegroundColor red `t "corrected," RDMA may be down.
            write-host -ForegroundColor red Most recent event "(System log)" follows
            $r[0] | fl
        } else {
            write-host -ForegroundColor Green Pass
        }
    }

    $j += start-job -name 'RoCE: Mellanox Error Check' {
        
        $r = $null
        $pc = $null

        switch ($using:drvdesc) {
            "Mellanox ConnectX-3 Pro Ethernet Adapter" {
                $pc = @{
                    '\Mellanox Adapter Diagnostic Counters(_Total)\Responder Out-of-order Sequence Received' = 'Out Of Order';
                    '\Mellanox Adapter Traffic Counters(_Total)\Packets Received Bad CRC Error' = 'Rec BadCRC';
                    '\Mellanox Adapter Traffic Counters(_Total)\Packets Received Frame Length Error' = 'Rec FrmLenErr';
                    '\Mellanox Adapter Traffic Counters(_Total)\Packets Received Symbol Error' = 'Rec SymlErr';
                    '\Mellanox Adapter Traffic Counters(_Total)\Packets Received Discarded' = 'Rec Discard';
                    '\Mellanox Adapter Traffic Counters(_Total)\Packets Outbound Discarded' = 'Outbnd Discard'
                    '\Mellanox Adapter Traffic Counters(_Total)\Packets Outbound Errors' = 'Outbnd Err';
                }
            }
            "Mellanox ConnectX-4 Adapter" {
                $pc = @{
                    '\Mellanox WinOF-2 Diagnostics(_Total)\Responder out of order sequence received' = 'Out Of Order';
                    '\Mellanox WinOF-2 Port Traffic(_Total)\Packets Received Bad CRC Error' = 'Rec BadCRC';
                    '\Mellanox WinOF-2 Port Traffic(_Total)\Packets Received Frame Length Error' = 'Rec FrmLenErr';
                    '\Mellanox WinOF-2 Port Traffic(_Total)\Packets Received Symbol Error' = 'Rec SymlErr';
                    '\Mellanox WinOF-2 Port Traffic(_Total)\Packets Received Discarded' = 'Rec Discard';
                    '\Mellanox WinOF-2 Port Traffic(_Total)\Packets Outbound Discarded' = 'Outbnd Discard'
                    '\Mellanox WinOF-2 Port Traffic(_Total)\Packets Outbound Errors' = 'Outbnd Err';
                }
            }
            default {
                write-host -ForegroundColor Red "Unknown adapter type: $($using:drvdesc)"
            }
        }

        # no counters, no results

        if ($pc -ne $null) {

            $r = icm (get-clusternode |? State -eq Up) -ArgumentList $pc {

                param($pc)

                $c = get-counter ($pc.Keys |% { $_ }) -ErrorAction SilentlyContinue
                if ($c) {

                    $o = new-object psobject -Property @{ 'Errors' = $false }
                    $c.CounterSamples | sort -Property Path |% {
                        if ($_.path -match '\\\\[^\\]+(\\.*$)') {
                            $o | Add-Member -NotePropertyName $pc[$matches[1]] -NotePropertyValue $_.CookedValue
                            if ($_.CookedValue -ne 0) { $o.Errors = $true }
                        }
                    }
                    $o
                }
            }
        }

        if ($r.length -ne (get-clusternode |? State -eq Up).length) {

            write-host -ForegroundColor Yellow WARNING: retransmit statistics not available from all nodes. Ensure driver updates applied.
            write-host -ForegroundColor Yellow `t $r.length nodes responded out of $((get-clusternode |? Up).length)

        }

        if ($r |? Errors) {

            write-host -ForegroundColor Red "WARNING: Any non-zero error counters may indicate physical or switch/NIC"
            write-host -ForegroundColor Red "`t issues, likely leading to packet drops and retransmits, which will degrade"
            write-host -ForegroundColor Red "`t performance. At high enough rates they can lead to SMB connection drops."
        } else {
            write-host -ForegroundColor Green "Pass - no errors detected"
        }

        $hdr = @( 'PsComputerName' )
        $hdr += $($pc.Values |% { $_ })

        $r | sort -property PsComputerName | ft -Property $hdr
    }
}

## Begin Symmetry Checks

$totalf = new-namedblock 'Total' { $true }
$totalf_nonull = new-namedblock 'Total' { $true } -nullpass:$false

###
$t = new-namedblock 'Clusport Device Symmetry Check' { gwmi -namespace root\wmi ClusportDeviceInformation }
$f = @($totalf)
$f += ,(new-namedblock 'Disk Type' { $_.DeviceType -eq 0} -nullpass:$false)
# temporarily remove hybrid check - the attribute is not synchronously updated
# and so may be (harmlessly) inaccurate for a period of time in early configuration
#$f += ,(new-namedblock 'Hybrid Media' { $_.DeviceAttribute -band 0x4})
$f += ,(new-namedblock 'Solid/Non-Rotational Media' { $_.DeviceAttribute -band 0x8})
$f += ,(new-namedblock 'Enclosure Type' { $_.DeviceType -eq 1} -nullpass:$false)
$f += ,(new-namedblock 'Virtual' { $_.DeviceAttribute -band 0x1} -nullpass:$true)

$j += start-job -InitializationScript $fns -Name $t.name { do-clustersymmetry $using:t $using:f }

###
$t = new-namedblock 'Physical Disk View Symmetry Check' { Get-StorageSubSystem |? Model -eq 'Clustered Windows Storage' | Get-PhysicalDisk }
$f = @($totalf)

$j += start-job -InitializationScript $fns -Name $t.name { do-clustersymmetry $using:t $using:f }

###
$t = new-namedblock 'Enclosure View Symmetry Check' { Get-StorageSubSystem |? Model -eq 'Clustered Windows Storage' | Get-StorageEnclosure }
$f = @($totalf)

$j += start-job -InitializationScript $fns -Name $t.name { do-clustersymmetry $using:t $using:f }

###
$t = new-namedblock 'SMB SBL Multichannel Symmetry Check' { Get-SmbMultichannelConnection -SmbInstance SBL }
$f = @($totalf)
$f += ,(new-namedblock 'RDMA Capable' { $_.ClientRdmaCapable -and $_.ServerRdmaCapable } -nullpass:$(-not $rdma))
$f += ,(new-namedblock 'Selected & Non-Failed' { $_.Selected -and -not $_.Failed } -nullpass:$(-not $rdma))

$j += start-job -InitializationScript $fns -Name $t.name { do-clustersymmetry $using:t $using:f }

###
$t = new-namedblock 'SMB CSV Multichannel Symmetry Check' { Get-SmbMultichannelConnection -SmbInstance CSV }
$f = @($totalf)
$f += ,(new-namedblock 'RDMA Capable' { $_.ClientRdmaCapable -and $_.ServerRdmaCapable } -nullpass:$(-not $rdma))
$f += ,(new-namedblock 'Selected & Non-Failed' { $_.Selected -and -not $_.Failed } -nullpass:$(-not $rdma))

$j += start-job -InitializationScript $fns -Name $t.name { do-clustersymmetry $using:t $using:f }

###
if ($rdma) {

    # rdma gives us an easy way of identifying a set of adapters to do this test with.
    # it would be good to extend this more generally

    $t = @()
    $t += new-namedblock 'RDMA Adapter IP Check' { Get-NetAdapterRdma |? Enabled | Get-NetAdapter |? HardwareInterface | Get-NetIPAddress -ErrorAction SilentlyContinue |? AddressState -eq 'Preferred' }
    $t += new-namedblock 'RDMA Adapter (Virtual) IP Check' { Get-NetAdapterRdma |? Enabled | Get-NetAdapter |? { -not $_.HardwareInterface } | Get-NetIPAddress -ErrorAction SilentlyContinue |? AddressState -eq 'Preferred' }
    $t += new-namedblock 'RDMA Adapter (Physical) IP Check' { Get-NetAdapterRdma |? Enabled | Get-NetAdapter |? HardwareInterface | Get-NetIPAddress -ErrorAction SilentlyContinue |? AddressState -eq 'Preferred' }
    $f = @($totalf)

    $j += start-job -InitializationScript $fns -Name $t[0].name {
    
        $using:t |% { do-clustersymmetry $_ $using:f -saygather:$true }
    }
}

###
$t = new-namedblock 'RDMA Adapters Symmetry Check' { Get-NetAdapterRdma |? Enabled | Get-NetAdapter }
$f = @($totalf)
$f += ,(new-namedblock 'Operational' { $_.Speed -gt 0 } -nullpass:$(-not $rdma))
$f += ,(new-namedblock 'Up' { $_.ifOperStatus -eq 'Up' } -nullpass:$(-not $rdma))

$j += start-job -InitializationScript $fns -Name $t.name { do-clustersymmetry $using:t $using:f }

###
if ($roce) {
    
    # assert SMB Direct policy defined
    $t = new-namedblock 'RoCE/QoS Configuration for SMB Direct' { Get-NetQosPolicy }
    $f = @($totalf_nonull)
    $f += ,(new-namedblock 'SMB Direct' { $_.NetDirectPort -eq 445 -and $_.PriorityValue -ne 0 } -nullpass:$false)

    $j += start-job -InitializationScript $fns -Name $t.name { do-clustersymmetry $using:t $using:f }
    
    # this is strictly insufficient, should ensure the enabled one is the same as that defined for SMB Direct
    $t = new-namedblock 'RoCE/CoS Definitions' { Get-NetQosFlowControl }
    $f = @($totalf_nonull)
    $f += ,(new-namedblock 'Enabled' { $_.Enabled } -nullpass:$false)
    $f += ,(new-namedblock 'Disabled' { -not $_.Enabled } -nullpass:$false)

    $j += start-job -InitializationScript $fns -Name $t.name { do-clustersymmetry $using:t $using:f }

    <#
    #
    $t = new-namedblock 'RoCE/CoS Applied to RoCE RNICs' { Get-NetAdapterRdma | get-netadapter -Physical -ErrorAction SilentlyContinue |? DriverProvider -match $using:rocematch }
    $f = @($totalf_nonull)
    $f += ,(new-namedblock 'Operational FlowControl Specs' { $_.OperationalFlowControl } -nullpass:$false)
    $f += ,(new-namedblock 'Operational Port/Protocol Classification Specs' { $_.OperationalClassifications } -nullpass:$false)
    $f += ,(new-namedblock 'Operational CoS Traffic Classes' { $_.OperationalTrafficClasses } -nullpass:$false)

    $j += start-job -InitializationScript $fns -Name $t.name { do-clustersymmetry $using:t $using:f }
    #>
}

###
if (Get-StorageFileServer |? SupportsContinuouslyAvailableFileShare) {

    $t = new-namedblock 'SMB Server CA FS Scope Net Interface Symmetry Check' {

        $fs = Get-StorageFileServer |? SupportsContinuouslyAvailableFileShare
        if ($fs) {
            Get-SmbServerNetworkInterface |? ScopeName -eq $fs.FriendlyName
        } else {
            $null
        }
    }
    $f = @()
    $f = ,(new-namedblock 'Rdma Capable' { $_.RdmaCapable } -nullpass:$(-not $rdma))

    $j += start-job -InitializationScript $fns -Name $t.name { do-clustersymmetry $using:t $using:f }
}

# consume job results
$j | display-jobs
