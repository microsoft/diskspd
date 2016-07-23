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

param([int] $interval = 60)

$cnodes = (Get-ClusterNode).Count
$cvms = (Get-ClusterGroup |? GroupType -eq 'VirtualMachine').Count/$cnodes

# loop through the set of run scripts
$f = gi C:\clusterstorage\collect\control\run-demo-*.ps1

# qos policies to loop
$policy = 'SilverVM','GoldVM','PlatinumVM',$null,$null

while ($true) {

    $null = & update-csv.ps1

    foreach ($run in $f) {

        cls

        # touch the current run file to trigger VM updates
        # see the master script
        (gi $run).LastWriteTime = (get-date)

        # pull the show-me line of the run script (comment, trimmed)
        # format is a single line, with # standing in for newline for multi-line output
        # substitute in the configuration's node and vms/node count
        # TBD: autodetect vd configuration
        $comm = (gc $run | sls '^#-#').Line

        write-host -fore green $($comm.trimstart('#- ') -replace '__CVMS__',$cvms -replace '__CNODES__',$cnodes -replace '#',"`n")
        
        # apply random QoS policy
        $p = $policy[(0..($policy.count - 1) | Get-Random)]
        $null = & set-storageqos.ps1 -policyname $p 2>&1

        write-host -fore yellow `nActive QoS Policy
        if ($p) {
            Get-StorageQosPolicy -Name $p | ft -AutoSize
        } else {
            write-host NONE
        }

        sleep $interval
    }
}