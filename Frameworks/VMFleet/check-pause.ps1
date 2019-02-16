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

# script to check the state/health of pause in the fleet
#
# isactive - whether there is an active pause (independent of health)
# !isactive - whether there is an active pause and all VMs have responded to it
#
# true/false return

param(
    [switch] $isactive = $false
    )

$pause = "C:\ClusterStorage\collect\control\flag\pause"
$pauseepoch = gc $pause -ErrorAction SilentlyContinue

if ($pauseepoch -eq $null) {
    write-host -fore red Pause not in force
    return $false
}

# if only performing the pause active check, done
if ($isactive) {
    return $true
}

# accumulate hash of pause flags mapped to current/stale state
$h = @{}

dir $pause-* |% {

    $thispause = gc $_ -ErrorAction SilentlyContinue
    if ($thispause -eq $pauseepoch) {
        $pausetype = 'Current'
    } else {
        $pausetype = 'Stale'
    }

    if ($_.name -match 'pause-(.+)\+(vm.+)') {
        # 1 is CSV, 2 is VM name
        $h[$matches[2]] = $pausetype
    } else {
        write-host -fore red ERROR: malformed pause $_.name present
    }
}

# now correlate to online vms and see if we agree all online are paused.
# note that if we shutdown some vms and then check pause the current flags
# will be higher than online, so we need to verify individually to not
# spoof ourselves.

$vms = get-clustergroup |? GroupType -eq VirtualMachine |? Name -like 'vm-*' |? State -eq Online

$pausedvms = $vms |? { $h[$_.Name] -eq 'Current' }

if ($pausedvms.Count -eq $vms.Count) {
    write-host -fore green OK: All $vms.count VMs paused
} else {
    write-host -fore red WARNING: of "$($vms.Count)," still waiting on ($vms.Count - $pausedvms.Count) to acknowledge pause

    compare-object $vms $pausedvms -Property Name -PassThru | sort -Property Name | ft -AutoSize Name,OwnerNode | Out-Host
    return $false
}

return $true