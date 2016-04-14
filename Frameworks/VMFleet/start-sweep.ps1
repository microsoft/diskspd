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
    [string] $addspec = "base"
    )

$vms = (get-clustergroup |? GroupType -eq VirtualMachine |? Name -like "vm-*" |? State -ne Offline).count

$go = "c:\clusterstorage\collect\control\go"
$done = "c:\clusterstorage\collect\control\done-*"
$runtemplate = "c:\clusterstorage\collect\control\run-sweeptemplate.ps1"
$runfile = "c:\clusterstorage\collect\control\run-sweep.ps1"

$timeout = 60
$checkpause = $true

# ensure we start a new go epoch
$goepoch = 0
$gocontent = gc $go -ErrorAction SilentlyContinue
if ($gocontent -eq '0') {
    $goepoch = 1
}

function new-runfile()
{
    # apply current subsitutions to produce a new run file
    gc $runtemplate |% {

        $line = $_

        foreach ($subst in $sweep.keys) {
            $line = $line -replace $subst,$sweep[$subst]
        }

        $line

    } | out-file $runfile -Encoding ascii -Width 9999
}

function show-run()
{
    # show current substitions (minus the underscore bracketing)
    write-host -fore green RUN SPEC `@ (get-date)
    foreach ($subst in $sweep.keys | sort) {
        $p = $subst -replace '_',''
        write-host -fore green "`t$p = $($sweep[$subst])"
    }
}

function get-runduration()
{
    $sweep['__d__'] + $sweep['__Warm__'] + $sweep['__Cool__']
}

function get-doneflags(
    [switch] $assertnone = $false
    )
{
    $assert = $false

    $tries = 0
    do {

        if ($tries -gt 0) {
            sleep 1
        }

        # increment attempts
        $tries += 1

        # capture start of the iteration
        $t0 = (get-date)

        # count number of done flags which agree with completion of the current go epoch
        $good = 0
        dir $done |% {
            $thisdone = gc $_ -ErrorAction SilentlyContinue
            if ($thisdone -eq $goepoch) {
                # if asserting that none should be complete, this would be an error!
                if ($assertnone) {
                    write-host -fore red ERROR: $_.basename is already done
                    $assert = $true
                }
                $good += 1
            }
        }

        # color status message per condition
        if ($assert -or $good -ne $vms) {
            $color = 'yellow'
        } else {
            $color = 'green'
        }

        $t1 = (get-date)
        $tdur = $t1 - $t0

        write-host -fore $color done check iteration $tries with "$good/$vms" `@ $t1 "($('{0:F2}' -f $tdur.totalseconds) seconds)"

    # loop if not asserting, not all vms are done, and still have timeout to use
    } while ((-not $assertnone) -and $good -ne $vms -and $tries -lt $timeout)

    # return assertion status?
    if ($assertnone) {
        return (-not $assert)
    }

    # return incomplete run failure
    if ($good -ne $vms) {
        write-host -fore red ABORT: only received "$good/$vms" completions in $timeout attempts `@ (get-date)
        return $false
    }

    # all worked!
    return $true
}

function set-runparam(
    [string] $name,
    $value
    )
{
    # apply substitution if spec'd
    if ($value -ne $null) {
        $sweep["__$($name)__"] = $value
    }
}

function do-run(
    $b = $null,
    $t = $null,
    $o = $null,
    $w = $null,
    $p = $null,
    $d = $null,
    $Warm = $null,
    $Cool = $null,
    $AddSpec = $null
    )
{
    # apply specified run parameters. note null is ignored.
    set-runparam b $b
    set-runparam t $t
    set-runparam o $o
    set-runparam w $w
    set-runparam p $p
    set-runparam d $d
    set-runparam Warm $Warm
    set-runparam Cool $Cool
    set-runparam AddSpec $AddSpec
    show-run

    write-host -fore yellow Generating new runfile `@ (get-date)
    new-runfile

    # if we do not have a pause to clear, need the sleep here since go
    # will release the fleet.
    # smb fileinfo cache +5 seconds (so that fleet will see updated timestamp)
    if (-not ($checkpause -and (check-pause -isactive:$true))) {
        $script:checkpause = $false
        sleep 15
    }

    write-host START Go Epoch: $goepoch `@ (get-date)
    echo $goepoch > $go

    # release any active pause on first loop
    if ($checkpause) {
        write-host CLEAR PAUSE `@ (get-date)

        # same sleep need, pause will release the fleet
        $script:checkpause = $false
        sleep 15
        
        # capture time zero prior to clear - can take time
        $t0 = get-date

        clear-pause
    } else {
    
        # capture time zero
        $t0 = get-date
    }

    # sleep half, check for false done if possible (clear can take time/short runs), continue
    $sleep = get-runduration

    $t1 = get-date
    $td = $t1 - $t0

    $remainingsleep = $sleep/2 - $td.TotalSeconds
    if ($remainingsleep -gt 0) {
        write-host SLEEP TO MID-RUN "($('{0:F2}' -f $remainingsleep) seconds)" `@ (get-date)
        sleep $remainingsleep
    }

    if ($td.TotalSeconds -lt ($sleep - 5)) {
        write-host MID-RUN CHECK Go Epoch: $goepoch `@ (get-date)
        # check for early completions, assert none are done yet
        if (-not (get-doneflags -assertnone:$true)) {
            return $false
        }
        write-host -fore green MID-RUN CHECK PASS Go Epoch: $goepoch `@ (get-date)
    }

    # capture time and sleep for the remaining interval
    $t1 = get-date
    $td = $t1 - $t0

    $remainingsleep = $sleep - $td.TotalSeconds
    if ($remainingsleep -gt 0) {
        write-host SLEEP TO END "($('{0:F2}' -f $remainingsleep) seconds)" `@ (get-date)
        sleep $remainingsleep
    }

    if (-not (get-doneflags)) {
        return $false
    }

    # advance go epoch
    $script:goepoch += 1

    return $true
}

############################
############################
## Modify from here down
############################
############################

# substitutions
# keys match substitution markers in $runtemplate.
# add keys as needed, use $null as placeholder for
# values which will be iterated in loops.
$sweep = @{
    '__b__' = $null;
    '__t__' = $null;
    '__o__' = $null;
    '__w__' = $null;
    '__p__' = 'r';
    '__d__' = 10;
    '__Warm__' = 10;
    '__Cool__' = 10;
    '__AddSpec__' = $addspec;
}

foreach ($b in 8) {
foreach ($t in 1) {
foreach ($o in 1,2,4,8,16,24,32,48,64) {
foreach ($w in 0) {

    $r = do-run -b $b -t $t -o $o -w $w

    if (-not $r) {
        return
    }

}}}}