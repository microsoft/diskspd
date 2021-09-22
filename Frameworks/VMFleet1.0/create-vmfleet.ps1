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
    [string]$BaseVHD = $(throw "please specify a base vhd"),
    [int]$VMs = $(throw "please specify a number of vms per node csv"),
    [string[]]$Groups = @(),
    [string]$AdminPass = $(throw 'need admin password for autologin'),
    [string]$Admin = 'administrator',
    [string]$ConnectPass = $(throw 'need password for loopback host connection'),
    [string]$ConnectUser = $(throw 'need username for loopback host connection'),
    [validateset('CreateVMSwitch','CopyVHD','CreateVM','CreateVMGroup','AssertComplete')][string]$StopAfter,
    [validateset('Force','Auto','None')][string]$Specialize = 'Auto',
    [switch]$FixedVHD = $true,
    [string[]]$Nodes = @()
    )

function Stop-After($step)
{
    if ($stopafter -eq $step) {
        write-host -ForegroundColor Green Stop after $step
        return $true
    }
    return $false
}

##################

# validate existence of basevhd
if (!(test-path -path $BaseVHD)) {
    throw "Base VHD $BaseVHD not found"
}

if (Get-ClusterNode |? State -ne Up) {
    throw "not all cluster nodes are up; please address before creating vmfleet"
}

# if no nodes specified, use the entire cluster
if ($nodes.count -eq 0) {
    $nodes = Get-ClusterNode
}

# convert to fixed vhd(x) if needed
if ((get-vhd $basevhd).VhdType -ne 'Fixed' -and $fixedvhd) {
                                    
    # push dynamic vhd to tmppath and place converted at original
    # note that converting a dynamic will leave a sparse hole on refs
    # this is OK, since the copy will not copy the hole
    $f = gi $basevhd
    $tmpname = "tmp-$($f.Name)"
    $tmppath = join-path $f.DirectoryName $tmpname
    del -Force $tmppath -ErrorAction SilentlyContinue
    ren $f.FullName $tmpname

    write-host -ForegroundColor Yellow "convert $($f.FullName) to fixed via $tmppath"
    convert-vhd -Path $tmppath -DestinationPath $f.FullName -VHDType Fixed
    if (-not $?) {
        ren $tmppath $f.Name
        throw "ERROR: could not convert $($f.fullname) to fixed vhdx"
    }
    
    del $tmppath
}

# Create the fleet vmswitches with a fixed IP at the base of the APIPA range
icm $nodes {

    if (-not (Get-VMSwitch -Name Internal -ErrorAction SilentlyContinue)) {
    
        New-VMSwitch -name Internal -SwitchType Internal
        Get-NetAdapter |? DriverDescription -eq 'Hyper-V Virtual Ethernet Adapter' |? Name -eq 'vEthernet (Internal)' | New-NetIPAddress -PrefixLength 16 -IPAddress '169.254.1.1'
    }
} | ft -AutoSize

#### STOPAFTER
if (Stop-After "CreateVMSwitch") {
    return
}

# create $vms vms per each csv named as <nodename><group prefix>
# vm name is vm-<group prefix><$group>-<hostname>-<number>

icm $nodes -ArgumentList $stopafter,(Get-Command Stop-After) {

    param( [string]$stopafter,
           $fn )

    set-item -Path function:\$($fn.name) -Value $fn.definition
    # workaround evaluation bug and make $stopafter evaluate in the session
    $null = $stopafter

    function apply-specialization( $path )
    {
        # all steps here can fail immediately without cleanup

        # error accumulator
        $ok = $true

        # create run directory

        del -Recurse -Force z:\run -ErrorAction SilentlyContinue
        mkdir z:\run
        $ok = $ok -band $?
        if (-not $ok) {
            Write-Error "failed run directory creation for $vhdpath"
            return $ok
        }

        # autologon
        $null = reg load 'HKLM\tmp' z:\windows\system32\config\software
        $ok = $ok -band $?
        $null = reg add 'HKLM\tmp\Microsoft\Windows NT\CurrentVersion\WinLogon' /f /v DefaultUserName /t REG_SZ /d $using:admin
        $ok = $ok -band $?
        $null = reg add 'HKLM\tmp\Microsoft\Windows NT\CurrentVersion\WinLogon' /f /v DefaultPassword /t REG_SZ /d $using:adminpass
        $ok = $ok -band $?
        $null = reg add 'HKLM\tmp\Microsoft\Windows NT\CurrentVersion\WinLogon' /f /v AutoAdminLogon /t REG_DWORD /d 1
        $ok = $ok -band $?
        $null = reg add 'HKLM\tmp\Microsoft\Windows NT\CurrentVersion\WinLogon' /f /v Shell /t REG_SZ /d 'powershell.exe -noexit -command c:\users\administrator\launch.ps1'
        $ok = $ok -band $?
        $null = [gc]::Collect()
        $ok = $ok -band $?
        $null = reg unload 'HKLM\tmp'
        $ok = $ok -band $?
        if (-not $ok) {
            Write-Error "failed autologon injection for $vhdpath"
            return $ok
        }

        # scripts

        copy -Force C:\ClusterStorage\collect\control\master.ps1 z:\run\master.ps1
        $ok = $ok -band $?
        if (-not $ok) {
            Write-Error "failed injection of specd master.ps1 for $vhdpath"
            return $ok
        }

        del -Force z:\users\administrator\launch.ps1 -ErrorAction SilentlyContinue
        gc C:\ClusterStorage\collect\control\launch-template.ps1 |% { $_ -replace '__CONNECTUSER__',$using:connectuser -replace '__CONNECTPASS__',$using:connectpass } > z:\users\administrator\launch.ps1
        $ok = $ok -band $?
        if (-not $ok) {
            Write-Error "failed injection of launch.ps1 for $vhdpath"
            return $err
        }

        echo $vmspec > z:\vmspec.txt
        $ok = $ok -band $?
        if (-not $ok) {
            Write-Error "failed injection of vmspec for $vhdpath"
            return $ok
        }

        # load files
        $f = 'z:\run\testfile1.dat'
        if (-not (gi $f -ErrorAction SilentlyContinue)) {
            fsutil file createnew $f (10GB)
            $ok = $ok -band $?
            fsutil file setvaliddata $f (10GB)
            $ok = $ok -band $?
        }
        if (-not $ok) {
            Write-Error "failed creation of initial load file for $vhdpath"
            return $ok
        }

        return $ok
    }

    function specialize-vhd( $vhdpath )
    {
        $vhd = (gi $vhdpath)
        $vmspec = $vhd.Directory.Name,$vhd.BaseName -join '+'

        # mount vhd and its largest partition
        $o = Mount-VHD $vhd -NoDriveLetter -Passthru
        if ($o -eq $null) {
            Write-Error "failed mount for $vhdpath"
            return $false
        }
        $p = Get-Disk -number $o.DiskNumber | Get-Partition | sort -Property size -Descending | select -first 1
        $p | Add-PartitionAccessPath -AccessPath Z:

        $ok = apply-specialization Z:

        Remove-PartitionAccessPath -AccessPath Z: -InputObject $p
        Dismount-VHD -DiskNumber $o.DiskNumber

        return $ok
    }

    $csvs = Get-ClusterSharedVolume

    # handle restore cases by mapping the csv to the friendly name of the volume
    # don't rely on the csv name to contain this data

    $vh = @{}
    Get-Volume |? FileSystem -eq CSVFS |% { $vh[$_.Path] = $_ }

    $csvs |% {
        $v = $vh[$_.SharedVolumeInfo.Partition.Name] 
        if ($v -ne $null) {
            $_ | Add-Member -NotePropertyName VDName -NotePropertyValue $v.FileSystemLabel
        }
    }

    foreach ($csv in $csvs) {

        if ($($using:groups).Length -eq 0) {
            $groups = @( 'base' )
        } else {
            $groups = $using:groups
        }

        # identify the CSvs for which this node should create its VMs
        # the trailing characters (if any) are the group prefix
        if ($csv.VDName -match "^$env:COMPUTERNAME(?:-.+){0,1}") {

            foreach ($group in $groups) {

                if ($csv.VDName -match "^$env:COMPUTERNAME-([^-]+)$") {
                    $g = $group+$matches[1]
                } else {
                    $g = $group
                }

                foreach ($vm in 1..$using:vms) {

                    $stop = $false

                    $newvm = $false
                    $name = "vm-$g-$env:COMPUTERNAME-$vm"
                    $path = Join-Path $csv.SharedVolumeInfo.FriendlyVolumeName $name

                    # place vhdx in subdirectory, per scvmm layout defaults
                    # $vhd = $path+".vhdx"
                    $vhd = join-path $path "$name.vhdx"

                    # if the vm cluster group exists, we are already deployed
                    if (-not (Get-ClusterGroup -Name $name -ErrorAction SilentlyContinue)) {

                        if (-not $stop) {
                            $stop = Stop-After "AssertComplete"
                        }

                        if ($stop) {

                            Write-Host -ForegroundColor Red "vm $name not deployed"

                        } else {

                            Write-Host -ForegroundColor Yellow "create vm $name @ metadata path $path with vhd $vhd"

                            # create vm if not already done
                            # note that when restarting interrupted creation, the vm could have moved elsewhere
                            # under cluster control.

                            $o = Get-ClusterGroup |? GroupType -eq VirtualMachine |? Name -eq $name

                            if (-not $o) {

                                # force re-specialization
                                $newvm = $true

                                # if the cluster group doesn't exist, we're on the canonical node to create the vm
                                # if the vm exists, tear it down and refresh

                                $o = get-vm -Name $name -ErrorAction SilentlyContinue

                                if ($o) {

                                    # interrupted between vm creation and role creation; redo it
                                    write-host "REMOVING vm $name for re-creation"

                                    if ($o.State -ne 'Off') {
                                        Stop-VM -Name $name -Force -Confirm:$false
                                    }
                                    Remove-VM -Name $name -Force -Confirm:$false

                                } else {

                                    # scrub and re-create the vm metadata path and vhd
                                    rmdir -ErrorAction SilentlyContinue -Recurse $path
                                    $null = mkdir -ErrorAction SilentlyContinue $path

                                    cp $using:basevhd $vhd
                                }

                                #### STOPAFTER
                                if (-not $stop) {
                                    $stop = Stop-After "CopyVHD"
                                }

                                if (-not $stop) {
                                    $o = New-VM -VHDPath $vhd -Generation 2 -SwitchName Internal -Path $path -Name $name

                                    # create A1 VM. use set-vmfleet to alter fleet sizing post-creation.
                                    $o | Set-VM -ProcessorCount 1 -MemoryStartupBytes 1.75GB -StaticMemory

                                    # do not monitor the internal switch connection; this allows live migration
                                    $o | Get-VMNetworkAdapter| Set-VMNetworkAdapter -NotMonitoredInCluster $true
                                }
                            }

                            #### STOPAFTER
                            if (-not $stop) {
                                $stop = Stop-After "CreateVM"
                            }

                            if (-not $stop) {
                                
                                # create clustered vm role and assign default owner node
                                $o | Add-ClusterVirtualMachineRole
                                Set-ClusterOwnerNode -Group $o.VMName -Owners $env:COMPUTERNAME
                            }
                        }

                    } else {
                        Write-Host -ForegroundColor Green "vm $name already deployed"
                    }

                    #### STOPAFTER
                    if (-not $stop) {
                        $stop = Stop-After "CreateVMGroup"
                    }

                    if (-not $stop -or ($using:specialize -eq 'Force')) {
                        # specialize as needed
                        # auto only specializes new vms; force always; none skips it
                        if (($using:specialize -eq 'Auto' -and $newvm) -or ($using:specialize -eq 'Force')) {
                            write-host -fore yellow specialize $vhd
                            if (-not (specialize-vhd $vhd)) {
                                write-host -fore red "Failed specialize of $vhd, halting."
                            }
                        } else {
                            write-host -fore green skip specialize $vhd
                        }
                    }
                }
            }
        }
    }
}
