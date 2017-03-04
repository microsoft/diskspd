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

# stop and remove clustered vm roles
Get-ClusterGroup |? GroupType -eq VirtualMachine | Stop-ClusterGroup
Get-ClusterGroup |? GroupType -eq VirtualMachine | Remove-ClusterGroup -RemoveResources -Force

# remove all vms
icm (Get-ClusterNode) {
    Get-VM | Remove-VM -Confirm:$false -Force
}

# delete all vm content from csv
$csv = Get-ClusterSharedVolume

# handle restore cases by mapping the csv to the friendly name of the volume
# don't rely on the csv name to contain this data

$vh = @{}
Get-Volume |? FileSystem -eq CSVFS |% { $vh[$_.Path] = $_ }

$csv |% {
    $v = $vh[$_.SharedVolumeInfo.Partition.Name] 
    if ($v -ne $null) {
        $_ | Add-Member -NotePropertyName VDName -NotePropertyValue $v.FileSystemLabel
    }
}

Get-ClusterNode |% {
    $csv |? VDName -match "$($_.Name)(-.+)?"
} |% {

    del -Recurse -Force "$($_.sharedvolumeinfo.friendlyvolumename)\vm*"
}
