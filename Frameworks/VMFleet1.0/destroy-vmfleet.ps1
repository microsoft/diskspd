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
    [string[]] $vms = $null
    )

if ($vms) {
    write-host -ForegroundColor Yellow "Destroying VM Fleet content for: $vms"
} else {
    write-host -ForegroundColor Yellow "Destroying VM Fleet"
}

# stop and remove clustered vm roles
write-host -ForegroundColor Green "Removing VM ClusterGroups"
Get-ClusterGroup |? GroupType -eq VirtualMachine |? {
    if ($vms) {
        $_.Name -in $vms
    } else {
        $_.Name -like 'vm-*'
    }
} |% {
    write-host "Removing ClusterGroup for $($_.Name)"
    $_ | Stop-ClusterGroup
    $_ | Remove-ClusterGroup -RemoveResources -Force
}

# remove all vms
write-host -ForegroundColor Green "Removing VMs"
icm (Get-ClusterNode) {
    Get-VM |? {
        if ($using:vms) {
            $_.Name -in $using:vms
        } else {
            $_.Name -like 'vm-*'
        }
    } |% {
        write-host "Removing VM for $($_.Name) @ $($env:COMPUTERNAME)"
        $_ | Remove-VM -Confirm:$false -Force
    }

    # do not remove the internal switch if teardown is partial
    if ($using:vms -eq $null) {
        write-host -ForegroundColor Green "Removing Internal VMSwitch"
        Get-VMSwitch -SwitchType Internal | Remove-VMSwitch -Confirm:$False -Force
    }
}

# handle restore cases by mapping the csv to the friendly name of the volume
# don't rely on the csv name to contain this data
$csv = Get-ClusterSharedVolume

$vh = @{}
Get-Volume |? FileSystem -eq CSVFS |% { $vh[$_.Path] = $_ }

$csv |% {
    $v = $vh[$_.SharedVolumeInfo.Partition.Name]
    if ($v -ne $null) {
        $_ | Add-Member -NotePropertyName VDName -NotePropertyValue $v.FileSystemLabel
    }
}

# now delete content from csvs corresponding to the cluster nodes
write-host -ForegroundColor Green "Removing CSV content for VMs"
Get-ClusterNode |% {
    $csv |? VDName -match "$($_.Name)(-.+)?"
} |% {
    dir -Directory $_.sharedvolumeinfo.friendlyvolumename |? {
        if ($vms) {
            $_.Name -in $vms
        } else {
            $_.Name -like 'vm-*'
        }
    } |% {
        write-host "Removing CSV content for $($_.BaseName) @ $($_.FullName)"
        del -Recurse -Force $_.FullName
    }
}