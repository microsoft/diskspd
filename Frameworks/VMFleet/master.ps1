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

param(
    [string] $connectuser,
    [string] $connectpass
    )

$null = net use l: /d

if ($(Get-SmbMapping) -eq $null) {
    New-SmbMapping -LocalPath l: -RemotePath \\169.254.1.1\c$\clusterstorage\collect\control -UserName $connectuser -Password $connectpass -ErrorAction SilentlyContinue
}

# update tooling
cp l:\tools\* c:\run -Force

$run = 'c:\run\run.ps1'
$master = 'c:\run\master.ps1'

$mypause = "l:\flag\pause-$(gc c:\vmspec.txt)"
$mydone = "l:\flag\done-$(gc c:\vmspec.txt)"

$done = $false
$donetouched = $false
$pause = $false

$tick = 0

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

function get-flagfile(
    [string] $flag,
    [switch] $gc = $false
    )
{
    if ($gc) {
        gc "l:\flag\$flag" -ErrorAction SilentlyContinue   
    } else {
        gi "l:\flag\$flag" -ErrorAction SilentlyContinue
    }
}

while ($true) {

    # update master control?
    # assume runners have a simple loop to re-execute on exit
    if (get-newfile l:\master*.ps1 $master -silent:$true) {
        break
    }

    # check and acknowledge pause - only drop flag once
    $pauseepoch = get-flagfile pause -gc:$true
    if ($pauseepoch -ne $null) {

        # pause clears done
        $done = $false

        # drop into pause flagfile if needed
        if ($pause -eq $false) {
            write-host -fore red PAUSE IN FORCE
            $pause = $true
            echo $pauseepoch > $mypause
        } else {
            if ($tick % 10 -eq 0) {
                write-host -fore red -NoNewline '.'
            }
        }

    } elseif ($done) {

        # drop epoch into done flagfile if needed
        if (-not $donetouched) {
            echo $goepoch > $mydone
            $donetouched = $true
        }

        # if go flag is now different, release for the next go around
        if ($goepoch -ne (get-flagfile go -gc:$true)) {
            $done = $false
            write-host -fore cyan `nReleasing from Done
        } else {
            if ($tick % 10 -eq 0) {
                write-host -fore yellow -NoNewline '.'
            }
        }

    } else {

        # clear pause & donetouched flags
        $pause = $false
        $donetouched = $false

        # update run script?
        $null = get-newfile l:\run*.ps1 $run -clean:$true
        $runf = gi $run -ErrorAction SilentlyContinue

        if ($runf -eq $null) {
            write-host -fore yellow no control file found
            sleep 30
            continue
        }

        # update go epoch - a change to this (if present) will be what
        # allows us to proceed past a done flag
        $goepoch = get-flagfile go -gc:$true
        if ($goepoch -ne $null) {
            write-host -fore cyan Go Epoch acknowledged at: $goepoch
        }

        # acknowledge script
        write-host -fore cyan Run Script $runf.lastwritetime "`n$("*"*20)"
        gc $runf
        write-host -fore cyan ("*"*20)

        # launch and monitor pause and new run file
        $j = start-job -arg $run { param($run) & $run }
        while (($jf = wait-job $j -Timeout 1) -eq $null) {

            $halt = $null

            # check pause or new run file: if so, stop and loop
            if (get-flagfile pause) {
                $halt = "pause set"
            }

            if (get-newfile l:\run*.ps1 $run -clean:$true -silent:$true) {
                $halt = "new run file"
            }

            if ($halt -ne $null) {
                write-host -fore yellow STOPPING CURRENT "(reason: $halt)"
                $j | stop-job
                $j | remove-job
                break
            }
        }

        # job finished?
        if ($jf -ne $null) {
            $result = $jf | receive-job

            if ($result -eq "done") {
                write-host -fore yellow DONE CURRENT
                $done = $true
            }

            $jf | remove-job
        }

        write-host -fore cyan ("*"*20)
    }

    # force gc to teardown potentially conflicting handles and enforce min pause
    [system.gc]::Collect()
    sleep 1
    $tick += 1
}