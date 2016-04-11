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

#
# VM Master Script
#
# This script must be executed on autologon by the VM. It establishes a mapping a location containing the Run Script
# and then repeatedly polls/executes the latest one it finds. This allows the runner to inject new run scripts on the
# fly and/or touch pre-existing ones to bump them to the top of the list.
#
# In the initial form, it assumes the VM is connected by an internal switch and it just polls all the node names to
# find the one which will work. This could be improved by using specialization to inject the specific path to a
# configuration file.
#

$null = net use l: /d

if ($(Get-SmbMapping) -eq $null) {
    New-SmbMapping -LocalPath l: -RemotePath \\169.254.1.1\c$\clusterstorage\collect\control -UserName __ADMIN__ -Password __ADMINPASS__ -ErrorAction SilentlyContinue
}

# update tooling
cp l:\tools\* c:\run -Force

$run = 'c:\run\run.ps1'
$master = 'c:\run\master.ps1'

function get-newfile(
    $sourcepat,
    $dest,
    $clean = $false,
    $silent = $false
    )
{
    $sf = dir $sourcepat | sort -Property LastWriteTime -Descending | select -first 1
    $df = gi $dest -ErrorAction SilentlyContinue

    # no source?
    if ($sf -eq $null) {
        write-host -fore green NO source present
        # clean if needed
        if ($clean) {
            rm $dest
        }
        # no new file is an update condition
        $true
    } elseif ($df -eq $null -or $sf.lastwritetime -gt $df.lastwritetime) {
        write-host -fore green NEWER source $sf.fullname '=>' $dest
        # update with newer source file and indicate update condition
        cp $sf.fullname $dest -Force
        $true
    } else {
        if (-not $silent) {
            write-host -fore green NO newer source $sf.lastwritetime $sf.fullname '**' $df.LastWriteTime $df.fullname
        }
        # already have latest, indicate no new
        $false
    }
}

while ($true) {

    # update master control?
    # assume runners have a simple loop to re-execute on exit
    if (get-newfile l:\master*.ps1 $master) {
        break
    }

    # check and acknowledge pause
    if (gi l:\pause -ErrorAction SilentlyContinue) {

        write-host -fore red PAUSE IN FORCE
        echo $null > l:\pause-$(gc c:\vmspec.txt)

    } else {

        # update run script?
        $null = get-newfile l:\run*.ps1 $run -clean:$true
        $runf = gi $run -ErrorAction SilentlyContinue

        if ($runf -eq $null) {
            write-host -fore yellow no control file found
            sleep 30
            continue
        }

        # acknowledge script
        write-host -fore cyan Run Script $runf.lastwritetime "`n$("*"*20)"
        gc $runf
        write-host -fore cyan ("*"*20)

        # launch and monitor pause and new run file
        $j = start-job -arg $run { param($run) & $run }
        while (($jf = wait-job $j -Timeout 1) -eq $null) {

            # check pause or new run file: if so, stop and loop
            if ((gi l:\pause -ErrorAction SilentlyContinue) -or
                (get-newfile l:\run*.ps1 $run -clean:$true -silent:$true)) {

                write-host -fore yellow STOPPING CURRENT
                $j | stop-job
                break
            }
        }

        if ($jf -ne $null) {
            $jf | receive-job
            $jf | remove-job
        }

        write-host -fore cyan ("*"*20)
    }

    # force gc to teardown potentially conflicting handles and enforce min pause
    [system.gc]::Collect()
    sleep 1
}
