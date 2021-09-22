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

##
## netsh advfirewall firewall set rule group="Performance Logs and Alerts" new enable=yes
##
[CmdletBinding( DefaultParameterSetName = "BySeconds" )]
param(

    [Parameter(
        ParameterSetName                = 'BySeconds',
        ValueFromPipeline               = $false,
        ValueFromPipelineByPropertyName = $false,
        Mandatory                       = $false)]
    [ValidateRange(1,[int]::MaxValue)]
    [int] $Seconds = 1,

    [Parameter(
        ParameterSetName                = 'ByStart',
        ValueFromPipeline               = $false,
        ValueFromPipelineByPropertyName = $false,
        Mandatory                       = $false)]
    [switch] $Start,
    
    [Parameter(
        ParameterSetName                = 'ByStop',
        ValueFromPipeline               = $false,
        ValueFromPipelineByPropertyName = $false,
        Mandatory                       = $false)]
    [switch] $Stop = $false,

    <# --------- common #>

    [Parameter(
        ParameterSetName                = 'BySeconds',
        Mandatory                       = $false)]
    [Parameter(
        ParameterSetName                = 'ByStart',
        Mandatory                       = $false)]
    [Parameter(
        ParameterSetName                = 'ByStop',
        Mandatory                       = $false)]
    [string] $Cluster = ".",

    <# --------- start-time parameters #>

    [Parameter(
        ParameterSetName                = 'BySeconds',
        Mandatory                       = $false)]
    [Parameter(
        ParameterSetName                = 'ByStart',
        Mandatory                       = $false)]
    [ValidateSet('PhysicalDisk','CPU','SMB','SMBD','SSB','SSB Cache','CSVFS','Storage','Spaces')]
    [string[]] $Set = '*',

    [Parameter(
        ParameterSetName                = 'BySeconds',
        Mandatory                       = $false)]
    [Parameter(
        ParameterSetName                = 'ByStart',
        Mandatory                       = $false)]
    [string] $AddSpec,

    [Parameter(
        ParameterSetName                = 'BySeconds',
        Mandatory                       = $false)]
    [Parameter(
        ParameterSetName                = 'ByStart',
        Mandatory                       = $false)]
    [int] $SampleInterval = 1,

    <# --------- stop-time parameters #>

    [Parameter(
        ParameterSetName                = 'BySeconds',
        Mandatory                       = $false)]
    [Parameter(
        ParameterSetName                = 'ByStop',
        Mandatory                       = $false)]
    [switch] $Force = $false,

    [Parameter(
        ParameterSetName                = 'BySeconds',
        Mandatory                       = $true)]
    [Parameter(
        ParameterSetName                = 'ByStop',
        Mandatory                       = $true)]
    [string] $Destination
    )

if ($psCmdlet.ParameterSetName -ne 'ByStart' -and
    (gi -ErrorAction SilentlyContinue $Destination)) {

    if (-not $force) {
        Write-Error "$Destination already exists, please delete or use -Force to overwrite"
        return
    } else {
        del -ErrorAction SilentlyContinue $Destination
    }
}

$sets = @{
    'PhysicalDisk' = '\PhysicalDisk(*)\*','+getclusport';
    'CSVFS' = '\Cluster CSVFS(*)\*','\Cluster CSV Volume Cache(*)\*','\Cluster CSV Volume Manager(*)\*','\Cluster CSVFS Block Cache(*)\*','\Cluster CSVFS Direct IO(*)\*','\Cluster CSVFS Redirected IO(*)\*','+getcsv';
    'SSB' = '\Cluster Disk Counters(*)\*','+getclusport';
    'SMB' = '\SMB Client Shares(*)\*','\SMB Server Shares(*)\*';
    'SMBD' = '\SMB Direct Connection(*)\*';
    'SSB Cache' = '\Cluster Storage Hybrid Disks(*)\*','\Cluster Storage Cache Stores(*)\*';
    'Spaces' = '\Storage Spaces Write Cache(*)\*','\Storage Spaces Tier(*)\*','\Storage Spaces Virtual Disk(*)\*'
    'ReFS' = '\ReFS(*)\*';
    'CPU' = '\Hyper-V Hypervisor Logical Processor(*)\*','\Processor Information(*)\*';

    'Storage' = 'PhysicalDisk','CSVFS','SSB','SSB Cache','ReFS','Spaces';
}

$special = @{
    "getclusport" = $false;
    "getcsv" = $false;
}

$cleanup = @()

$pc = @()
if ($Set.count -eq 1 -and $Set[0] -eq '*') {
    # list of all keys
    $pc = $sets.keys |% { $_ }
} else {
    $pc = $Set
}

# repeat expansion (sets in sets, possibly containing special collection rules)
do {
    $expansion = $false
    $pc = $pc |% {
        $n = $_
        switch ($n[0]) {
            # performance counter - passthru
            '\' { $n }
            # special collection (sets variable -> $true)
            '+' {

                if ($special.ContainsKey($n.Substring(1))) {
                    $special[$n.Substring(1)] = $true
                } else {
                    throw "unrecognized special gather command $($n.Substring(1))"
                }
            }
            # set expansion
            default {
                $sets[$n]
                $expansion = $true
            }
        }
    }
} while ($expansion)

# uniq the counters
$pc = ($pc | group -NoElement).Name

# --

function start-logman(
    [string] $name,
    [string[]] $counters,
    [int] $sampleinterval
    )
{
    $computer = $env:COMPUTERNAME
    $f = "c:\perfctr-$name-$computer.blg"

    $null = logman create counter "perfctr-$name" -o $f -f bin -si $sampleinterval --v -c $counters
    $null = logman start "perfctr-$name"
    write-host "performance counters on: $computer"
}

function stop-logman(
    [string] $name
    )
{
    $computer = $env:COMPUTERNAME
    $f = "c:\perfctr-$name-$computer.blg"
    
    $null = logman stop "perfctr-$name"
    $null = logman delete "perfctr-$name"
    write-host "performance counters off: $computer"

    echo $("\\$computer\$f" -replace ':','$')
}

if (-not $Stop) {

    icm (get-clusternode -cluster $Cluster |? State -eq Up) -ArgumentList (get-command start-logman) {

        param($fn)
        set-item -path function:\$($fn.name) -value $fn.definition

        start-logman $using:AddSpec $using:pc -sampleinterval $using:SampleInterval
    }

    if ($Start) {
        write-host -ForegroundColor Yellow INFO: captures started - reinvoke with `-stop to complete capture
        return
    }

    sleep $Seconds

} else {

    write-host -ForegroundColor Yellow INFO: completing previously started capture
}

# now capture all counter files
$f = @()
$f += icm (get-clusternode -cluster $Cluster |? State -eq Up) -ArgumentList (get-command stop-logman) {

    param($fn)
    set-item -path function:\$($fn.name) -value $fn.definition

    stop-logman $using:AddSpec $using:Destination
}

# add all counter blg to the cleanup step
$cleanup += $f

#--
# specials
#--

# make capture directory, and add to cleanup list
# note that all specials are generated into this directory,
# and will be automatically cleaned up when it is deleted
$t = New-TemporaryFile
del $t
$null = md $t
$cleanup += $t

if ($special['getclusport']) {

    get-clusternode -cluster $Cluster |? State -eq Up |% {

        $exp = "$t\clusport-$_.xml"
        gwmi -ComputerName $_ -Namespace root\wmi ClusPortDeviceInformation | export-clixml -Path $exp
        $f += $exp
    }
}

if ($special['getcsv']) {

    $exp = "$t\csv.xml"
    Get-ClusterSharedVolume -cluster $Cluster | export-clixml -Path $exp
    $f += $exp
}

compress-archive -DestinationPath $Destination -Path $f
del -Force -Recurse $cleanup

write-host "performance counters: $Destination"