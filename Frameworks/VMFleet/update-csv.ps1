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
    [switch]$disableintegrity = $false,
    [switch]$renamecsvmounts = $false,
    [switch]$movecsv = $true,
    [switch]$movevm = $true,
    [switch]$shiftcsv = $false
)

$csv = get-clustersharedvolume

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

if ($disableintegrity) {
    $csv |% {
        dir -r $_.SharedVolumeInfo.FriendlyVolumeName | Set-FileIntegrity -Enable:$false -ErrorAction SilentlyContinue
    }
}

if ($renamecsvmounts) {
    $csv |% {
        if ($_.SharedVolumeInfo.FriendlyVolumeName -match 'Volume\d+$') {
            ren $_.SharedVolumeInfo.FriendlyVolumeName $_.VDName
        }
    }
}

function move-csv(
    $rehome = $true,
    $shift = $false
    )
{
    if ($shift) {

        write-host -fore Yellow Shifting CSV owners

        # rotation order (n0 -> n1, n1 -> n2, ... nn -> n0)
        $nodes = (Get-ClusterNode |? State -eq Up | sort -Property Name).Name
        $nh = @{}
        foreach ($i in 1..($nodes.Length-1)) {
            $nh[$nodes[$i-1]] = $nodes[$i]
        }
        $nh[$nodes[$nodes.Length-1]] = $nodes[0]

        Get-ClusterNode |% {
            $node = $_.Name
            $csv |? VDName -match "$node(?:-.+){0,1}" |% {
                $_ | Move-ClusterSharedVolume $nh[$_.OwnerNode.Name]
            }
        }

    } elseif ($rehome) {

        # write-host -fore Yellow Re-homing CSVs

        # move all csvs named by node names back to their named node
        get-clusternode |? State -eq Up |% {
            $node = $_.Name
            $csv |? VDName -match "$node(?:-.+){0,1}" |? OwnerNode -ne $node |% { $_ | Move-ClusterSharedVolume $node }
        }
    }
}

if ($shiftcsv) {
    # shift rotates all csvs node ownership by one node, in lexical order of
    # current node owner name. this is useful for forcing out-of-position ops.
    move-csv -shift:$true
} elseif ($movecsv) {
    # move puts all csvs back on their home node
    move-csv -rehome:$true
}

if ($movevm) {

    icm (Get-ClusterNode |? State -eq Up) {

        Get-ClusterGroup |? GroupType -eq VirtualMachine |% {

            if ($_.Name -like "vm-*-$env:COMPUTERNAME-*") {
                if ($env:COMPUTERNAME -ne $_.OwnerNode) {
                    write-host -ForegroundColor yellow moving $_.name $_.OwnerNode '->' $env:COMPUTERNAME

                    # the default move type is live, but live does not degenerately handle offline vms yet
                    if ($_.State -eq 'Offline') {
                        Move-ClusterVirtualMachineRole -Name $_.Name -Node $env:COMPUTERNAME -MigrationType Quick
                    } else {
                        Move-ClusterVirtualMachineRole -Name $_.Name -Node $env:COMPUTERNAME
                    }
                } else {
                    # write-host -ForegroundColor green $_.name is on $_.OwnerNode
                }
            }
        }
    }
}
