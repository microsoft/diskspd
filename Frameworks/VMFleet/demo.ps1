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

# loop through the set of run scripts
$d = gi C:\ClusterStorage\collect\control
$f = gi $d\run-*.ps1

while ($true) {

    $null = & $d\update-csv.ps1

    foreach ($run in $f) {

        cls

        # touch the current run file to trigger VM updates
        # see the master script
        (gi $run).LastWriteTime = (get-date)

        # tear off the first line of the run script (assume a comment, and trim it)
        # format is a single line, with # standing in for newline for multi-line output
        $comm = gc $run | select -first 1
        write-host -fore green $($comm.trimstart('# ') -replace '#',"`n")
        
        # apply random QoS policy
        $policy = 'SilverVM','GoldVM','PlatinumVM','NoLimit','NoLimit'
        $p = $policy | get-random
        $null = & $d\set-storageqos -policyname $p 2>&1

        write-host -fore yellow Active QoS Policy
        Get-StorageQosPolicy -Name $p | ft -AutoSize

        sleep 60
    }
}

return
