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
    [string] $addspec = "base",
    [string] $runtemplate = "c:\clusterstorage\collect\control\run-sweeptemplate.ps1",
    [string] $runfile = "c:\clusterstorage\collect\control\run-sweep.ps1",
    [string[]] $labeltemplate = @('b','t','o','w','p','-$addspec'),
    [Parameter(Mandatory =$true)]
        [int[]] $b,
    [Parameter(Mandatory =$true)]
        [int[]] $t,
    [Parameter(Mandatory =$true)]
        [int[]] $o,
    [Parameter(Mandatory =$true)]
        [int[]] $w,
    [int[]] $iops = $null,
    [ValidateSet('r','s','si')]
        [string[]] $p = 'r',
    [ValidateRange(1,[int]::MaxValue)]
        [int] $d = 60,
    [ValidateRange(0,[int]::MaxValue)]
        [int] $warm = 60,
    [ValidateRange(0,[int]::MaxValue)]
        [int] $cool = 60,
    [string[]] $pc = $null,
    [switch] $midcheck = $false
    )

#############

# a single named variable with a set of values
class variable {

    [int] $_ordinal
    [object[]] $_list
    [string] $_label

    variable([string] $label, [object[]] $list)
    {
        $this._list = $list
        $this._ordinal = 0
        $this._label = $label
    }

    # current value of the variable ("red"/"blue"/"green")
    [object] value()
    {
        if ($this._list.count -gt 0) {
            return $this._list[$this._ordinal]
        } else {
            return $null
        }
    }

    # label/name of the variable ("color")
    [object] label()
    {
        return $this._label
    }

    # increment to the next member, return whether a logical carry
    # has occured (overflow)
    [bool] increment()
    {
        # empty list passes through
        if ($this._list.Count -eq 0) {
            return $true
        }

        # non-empty list, increment
        $this._ordinal += 1
        if ($this._ordinal -eq $this._list.Count) {
            $this._ordinal = 0
            return $true
        }
        return $false
    }

    # back to initial state
    [void] reset()
    {
        $this._ordinal = 0
    }
}

# a set of variables which allows enumeration of their combinations
# this behaves as a numbering system indexing the respective variables
# order is not specified
class variableset {

    [hashtable] $_set = @{}

    variableset([variable[]] $list)
    {
        $list |% { $this._set[$_.label()] = $_ }
        $this._set.Values |% { $_.reset() }
    }

    # increment the enumeration
    # returns true if the enumeration is complete
    [bool] increment()
    {
        $carry = $false
        foreach ($v in $this._set.Values) {

            # if the variable returns the carry flag, increment
            # the next, and so forth
            $carry = $v.increment()
            if (-not $carry) { break }
        }

        # done if the most significant carried over
        return $carry
    }

    # enumerator of all variables
    [object] getset()
    {
        return $this._set.Values
    }

    # return value of specific variable
    [object] get([string]$label)
    {
        return $this._set[$label].value()
    }

    # return a label representing the current value of the set, following the input label template
    # add a leading '-' to get a seperator
    # use a leading '$' to eliminate repetition of label (just produce value)
    [string] label([string[]] $template)
    {
        return $($template |% {

            $str = $_
            $pfx = ''
            $done = $false
            $norep = $false
            do {
                switch ($str[0])
                {
                    '-' {
                        $pfx = '-'
                        $str = $str.TrimStart('-')
                    }
                    '$' {
                        $norep = $true
                        $str = $str.TrimStart('$')
                    }
                    default {
                        $done = $true
                    }
                }
            } while (-not $done)

            $lookstr = $str
            if ($norep) {
                $str = ''
            }

            # only produce labels for non-null values
            if ($this._set[$lookstr].value() -ne $null) {
                "$pfx$str" + $this._set[$lookstr].value()
            }
        }) -join $null
    }
}

#############

function start-logman(
    [string] $computer,
    [string] $name,
    [string[]] $counters
    )
{
    $f = "c:\perfctr-$name-$computer.blg"

    $null = logman create counter "perfctr-$name" -o $f -f bin -si 1 --v -c $counters -s $computer
    $null = logman start "perfctr-$name" -s $computer
    write-host "performance counters on: $computer"
}

function stop-logman(
    [string] $computer,
    [string] $name,
    [string] $path
    )
{
    $f = "c:\perfctr-$name-$computer.blg"
    
    $null = logman stop "perfctr-$name" -s $computer
    $null = logman delete "perfctr-$name" -s $computer
    xcopy /j $f $path
    del -force $f
    write-host "performance counters off: $computer"
}

function new-runfile(
    [variableset] $vs
    )
{
    # apply current subsitutions to produce a new run file
    gc $runtemplate |% {

        $line = $_

        foreach ($v in $vs.getset()) {
            # non-null goes in as is, null goes in as evaluatable $null
            if ($v.value() -ne $null) {
                $vsub = $v.value()
            } else {
                $vsub = '$null'
            }
            $line = $line -replace "__$($v.label())__",$vsub
        }

        $line

    } | out-file $runfile -Encoding ascii -Width 9999
}

function show-run(
    [variableset] $vs
    )
{
    # show current substitions (minus the underscore bracketing)
    write-host -fore green RUN SPEC `@ (get-date)
    foreach ($v in $vs.getset()) {
            if ($v.value() -ne $null) {
                $vsub = $v.value()
            } else {
                $vsub = '$null'
            }
            write-host -fore green "`t$($v.label()) = $($vsub)"
    }
}

function get-runduration(
    [variableset] $vs
    )
{
    $vs.get('d') + $vs.get('Warm') + $vs.get('Cool')
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

function do-run(
    [variableset] $vs
    )
{
    # apply specified run parameters. note null is ignored.
    show-run $vs

    write-host -fore yellow Generating new runfile `@ (get-date)
    new-runfile $vs

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

    # start performance counter capture
    if ($pc -ne $null) {
        $curpclabel = $vs.label($labeltemplate)
        icm (get-clusternode) -ArgumentList (get-command start-logman) {

            param($fn)
            set-item -path function:\$($fn.name) -value $fn.definition

            start-logman $env:COMPUTERNAME $using:curpclabel $using:pc
        }
    }

    ######

    # sleep half, check for false done if possible (clear can take time/short runs), continue
    $sleep = get-runduration $vs

    $t1 = get-date
    $td = $t1 - $t0

    if ($midcheck) {

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

    } else {

        sleep ($sleep - $td.TotalSeconds)
    }

    ######

    # stop performance counter capture
    if ($pc -ne $null) {
        icm (get-clusternode) -ArgumentList (get-command stop-logman) {

            param($fn)
            set-item -path function:\$($fn.name) -value $fn.definition

            stop-logman $env:COMPUTERNAME $using:curpclabel "C:\ClusterStorage\collect\control\result"
        }
    }

    if (-not (get-doneflags)) {
        return $false
    }

    # advance go epoch
    $script:goepoch += 1

    return $true
}

#############

$vms = (get-clustergroup |? GroupType -eq VirtualMachine |? Name -like "vm-*" |? State -ne Offline).count

# spec location of control files
$go = "c:\clusterstorage\collect\control\flag\go"
$done = "c:\clusterstorage\collect\control\flag\done-*"

$timeout = 120
$checkpause = $true

# ensure we start a new go epoch
$goepoch = 0
$gocontent = gc $go -ErrorAction SilentlyContinue
if ($gocontent -eq '0') {
    $goepoch = 1
}

# construct the variable list describing the sweep

############################
############################
## Modify from here down
############################
############################

# add any additional sweep parameters here to match those specified on the command line
# ensure your sweep template script contains substitutable elements for each
#
#      __somename__
#
# bracketed by two underscore characters. Consider adding your new parameters to
# the label template so that result files are well-named and distinguishable.

$v = @()
$v += [variable]::new('b',$b)
$v += [variable]::new('t',$t)
$v += [variable]::new('o',$o)
$v += [variable]::new('w',$w)
$v += [variable]::new('p',$p)
$v += [variable]::new('iops',$iops)
$v += [variable]::new('d',$d)
$v += [variable]::new('Warm',$warm)
$v += [variable]::new('Cool',$cool)
$v += [variable]::new('AddSpec',$addspec)

$sweep = [variableset]::new($v)

do {

    write-host -ForegroundColor Cyan '---'

    $r = do-run $sweep

    if (-not $r) {
        return
    }

} while (-not $sweep.increment())