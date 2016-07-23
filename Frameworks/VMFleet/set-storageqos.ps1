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

# apply given QoS policy (by name) to all VMs on specified nodes
param (
    $policyname = $null,
    [object[]] $node = $(get-clusternode |? State -eq Up)
)

if ($policyname -ne $null) {

    # QoS policy must exist, else error out
    $qosp = get-storageqospolicy -name $policyname
    if ($qosp -eq $null) {
        # cmdlet error sufficient
        return
    }
    $id = $qosp.PolicyId

} else {

    # clears QoS policy
    $id = $null
}

icm $node {

    # note: set-vhdqos should be replaced with set-vmharddiskdrive
    get-vm |% { get-vmharddiskdrive $_ |% { Set-VMHardDiskDrive -QoSPolicyID $using:id -VMHardDiskDrive $_ }}
}