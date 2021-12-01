<#
VM Fleet

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

Set-StrictMode -Version 3.0

# Globals @ Module-only scope (others in CommonFunc)

enum PauseState {
    Current
    Stale
}

enum CutoffType {
    No
    Scale
    CPU
    WriteLatency
    ReadLatency
    Surge
}

enum RunType {
    Default
    Baseline
    AnchorSearch
}

enum VolumeType
{
    Mirror
    NestedMirror
    Parity
    MAP
    Collect
}

class VolumeEstimate
{
    [VolumeType] $VolumeType
    [uint64] $MirrorSize
    [string] $MirrorTierName
    [uint64] $ParitySize
    [string] $ParityTierName
    [uint64] $Size

    VolumeEstimate(
        [VolumeType] $VolumeType,
        [uint64] $MirrorSize,
        [string] $MirrorTierName,
        [uint64] $ParitySize,
        [string] $ParityTierName
        )
    {
        $this.VolumeType = $VolumeType
        $this.MirrorSize = $MirrorSize
        $this.MirrorTierName = $MirrorTierName
        $this.ParitySize = $ParitySize
        $this.ParityTierName = $ParityTierName
        $this.Size = $MirrorSize + $ParitySize
    }
}

$vmSwitchName = 'FleetInternal'
$vmSwitchIP = '169.254.1.1'

# Note: this can be directly determined by Get-SmbClientConfiguration. Changes are not expected, but
# since the controlling timeout is in the guest lets simply wire it down. Actual is normally 10s, use
# +2s for margin.
$smbInfoCacheLifetime = 12

#
# Embedded scripts for VM control/pickup
#
# These are installed in the control directory and referenced/run by the VM Fleet.
# The Set verb is used as a meta-comment. These are never executed in context of the module
# and are not exported.
#

#
# The reference manual-edit runner script
#
function Set-FleetBaseScript
{
    # buffer size/alighment, threads/target, outstanding/thread, write%
    $b = 4; $t = 1; $o = 32; $w = 0

    # optional - specify rate limit in iops, translated to bytes/ms for DISKSPD
    # $iops = 500
    if ($null -ne $iops) { $g = $iops * $b * 1KB / 1000 }

    # io pattern, (r)andom or (s)equential (si as needed for multithread)
    $p = 'r'

    # durations of test, cooldown, warmup
    $d = 30*60; $cool = 30; $warm = 60

    $addspec = ''
    $gspec = $null

    # cap -> true to capture xml results, otherwise human text
    $cap = $false

    ### prior to this is template

    if ($null -ne $g) { $gspec = "g$($g)" }
    $result = "result-b$($b)t$($t)o$($o)w$($w)p$($p)$($gspec)"
    if ($addspec.Length) { $result += "-$($addspec)" }
    $result += "-$(Get-Content C:\vmspec.txt).xml"
    $dresult = "l:\result"
    $lresultf = Join-Path "C:\run" $result
    $dresultf = Join-Path $dresult $result

    if (-not (Get-Item $dresultf -ErrorAction SilentlyContinue)) {

        if ($cap) {
            $res = 'xml'
        } else {
            $res = 'text'
        }

        # look for data disk - if present, it will always be disk 1. disk 0 is boot/os.
        # only use it if it is a raw device. we do not have a convention for using a
        # filesystem on it at this time.
        $d1 = Get-Disk -Number 1 -ErrorAction SilentlyContinue
        if ($null -ne $d1 -and $d1.IsBoot -eq $false -and $d1.PartitionStyle -eq 'RAW')
        {
            $target = '#1'
        }
        else
        {
            $target = (Get-ChildItem C:\run\testfile?.dat)
        }

        $gspec = $null
        if ($null -ne $g) { $gspec = "-g$($g)" }
        $o = C:\run\diskspd.exe -Z20M -z -h `-t$t `-o$o $gspec `-b$($b)k `-$($p)$($b)k `-w$w `-W$warm `-C$cool `-d$($d) -D -L `-R$res $target

        if ($cap) {

            # export result and indicate done flag to control

            $o | Out-File $lresultf -Encoding ascii -Width ([int32]::MaxValue)
            [PSCustomObject]@{
                Result = @( $lresultf )
                Stash = $null
            }

        } else {

            #emit to human watcher
            $o | Out-Host
        }

    } else {

        Write-Host -fore green already done $dresultf

        # indicate done flag to control
        # this should only occur if controller does not change variation
        [PSCustomObject]@{
            Result = $null
            Stash = $null
        }
    }

    [system.gc]::Collect()
}

#
# The VM controller script, run by the VMs on start/login
#
function Set-FleetControlScript
{
    param(
        [string] $connectuser,
        [string] $connectpass
        )

    function Get-ProcessDump
    {
        [CmdletBinding()]
        param(
            [Parameter(Mandatory = $true, ValueFromPipeline = $true)]
            [System.Diagnostics.Process]
            $Process,

            [Parameter(Mandatory = $true)]
            [string]
            $DumpFile
        )

        begin
        {
            $wer = [PSObject].Assembly.GetType('System.Management.Automation.WindowsErrorReporting')
            $methods = $wer.GetNestedType('NativeMethods', 'NonPublic')
            $flags = [Reflection.BindingFlags] 'NonPublic, Static'
            $api = $methods.GetMethod('MiniDumpWriteDump', $Flags)

            # Normalize unqualified paths to current working directory
            $dfp = Split-Path $DumpFile -Parent
            if ($dfp.Length -eq 0)
            {
                $DumpFile = Join-Path $PWD.Path $DumpFile
            }
        }

        process
        {
            # Subst in PID to uniquify multiple processes / same name
            $dfs = $DumpFile -split '\.'
            $dfs[-2] += "-$($Process.ID)"
            $df = $dfs -join '.'

            $f = New-Object IO.FileStream($df, [IO.FileMode]::Create)

            $r = $api.Invoke($null, @($Process.Handle,
                                        $Process.Id,
                                        $f.SafeFileHandle,
                                        [UInt32] 2, # FullMemory
                                        [IntPtr]::Zero,
                                        [IntPtr]::Zero,
                                        [IntPtr]::Zero))

            $f.Close()

            if (-not $r)
            {
                $errorCode = [System.Runtime.InteropServices.Marshal]::GetLastWin32Error()

                # Remove any partially written dump file.
                Remove-Item $df -Force -ErrorAction SilentlyContinue

                throw "failed to dump $($Process.Name) PID $($Process.Id), error $errorcode"
            }
            else
            {
                Get-ChildItem $df
            }
        }
    }

    function CopyKeyIf
    {
        param(
            [Parameter(Mandatory = $true, Position = 0)]
            [hashtable]
            $HSource,

            [Parameter(Mandatory = $true, Position = 1)]
            [hashtable]
            $HDest,

            [Parameter(Mandatory = $true, Position = 2)]
            [string]
            $Key
        )

        if ($HSource.ContainsKey($Key))
        {
            $HDest[$Key] = $HSource[$Key]
        }
    }

    function LogOutput
    {
        [CmdletBinding()]
        param(
            [Parameter(Mandatory = $true, Position = 0, ValueFromRemainingArguments)]
            [ValidateNotNullOrEmpty()]
            [string[]]
            $Message,

            [Parameter()]
            [System.ConsoleColor]
            $ForegroundColor,

            [Parameter()]
            [switch]
            $IsVb,

            [Parameter()]
            [switch]
            $NoNewline,

            [Parameter()]
            [switch]
            $CarriageReturn
        )

        $m = ((Get-Date).GetDateTimeFormats('s')[0] + ": $($Message -join ' ')")
        if ($PSBoundParameters.ContainsKey('IsVb'))
        {
            Write-Verbose $m
        }
        else
        {
            $whParam = @{}
            CopyKeyIf $PSBoundParameters $whParam 'ForegroundColor'
            CopyKeyIf $PSBoundParameters $whParam 'NoNewline'
            if ($PSBoundParameters.ContainsKey('CarriageReturn'))
            {
                $m += "`r"
            }
            Write-Host @whParam $m
        }
    }

    function DisableWindowsRE
    {
        #
        # Disable the Windows Recovery console. Under aggresive conditions like repeated start/stop on the part
        # of a runner, Windows RE may get triggered and trap the VMs in the recovery console. While recoverable
        # with Repair-Fleet, lets prevent that since RE doesn't provide value to VM Fleet ops.
        #
        # It would be nice if we could do this during VM provisioning.
        #

        $state = reagentc /info |% {

            if ( $_ -match 'Windows RE status:\s+(\S+)')
            {
                $matches[1]
            }
        }

        if ($state -eq 'Enabled')
        {
            $out = reagentc /disable
            $status = $?
            LogOutput "Disabling Windows RE: $out"
            if (-not $status)
            {
                LogOutput -ForegroundColor Red "Failed to disable Windows RE - VM may enter the recovery console"
            }
        }
    }

    function ValidateSmbMapping
    {
        param(
            [Parameter(Mandatory = $true)]
            $LocalDriveLetter,

            [Parameter(Mandatory = $true)]
            $RemotePath,

            [Parameter()]
            $UserName,

            [Parameter()]
            $Password,

            [Parameter()]
            $TestPath
            )

        $maxAttempt = 10
        $localPath = "$($LocalDriveLetter):"
        if ($PSBoundParameters.ContainsKey('TestPath'))
        {
            # Cannot Join-Path since it will attempt to resolve: this may not exist at time zero
            $testPath = $localPath + '\' + $TestPath
        }
        else
        {
            $testPath = $localPath
        }

        $mapParams = @{
            LocalPath = $localPath
            RemotePath = $RemotePath
        }
        CopyKeyIf $PSBoundParameters $mapParams 'UserName'
        CopyKeyIf $PSBoundParameters $mapParams 'Password'

        function Create
        {
            LogOutput "CREATE SMB Mapping $localPath => $remotePath"
            $null = New-SmbMapping @mapParams
        }

        function Teardown
        {
            LogOutput "REMOVE SMB Mapping $localPath => $remotePath"
            Remove-SmbMapping -LocalPath $localPath -Force -Confirm:$false -UpdateProfile
        }

        for ($nAttempt = 0; $true; $nAttempt +=  1)
        {
            # Indicate connection rebuild attempts (do not emit on first pass - successful validation s.b. silent)
            if ($nAttempt -gt 0)
            {
                LogOutput "VALIDATE SMB Mapping $localPath => $remotePath (attempt $nAttempt)"
            }

            # Throw out after maxAttempts
            if ($nAttempt -ge $maxAttempt)
            {
                LogOutput -ForegroundColor Red "FAIL: SMB connection not recoverable after $maxAttempt tries"
                throw "SMB Connection Failure"
            }

            $mapping = Get-SmbMapping -LocalPath $localPath -ErrorAction SilentlyContinue

            # Create missing mapping
            if ($null -eq $mapping)
            {
                LogOutput "NOT PRESENT SMB Mapping @ $localPath"
                Start-Sleep 1
                Create
                continue
            }

            # Mapping reports bad, teardown and recreate
            if ($mapping.Status -ne 'OK')
            {
                LogOutput "NOT OK SMB Mapping @ $localPath reporting status = $($mapping.Status)"
                Start-Sleep 1
                Teardown
                Create
                continue
            }

            # If test path inaccessible, try teardown/recreate to recover
            if (-not (Test-Path -Path $testPath -ErrorAction Continue))
            {
                LogOutput "ACCESS FAILED to test path @ $testPath"
                Start-Sleep 1
                Teardown
                Create
                continue
            }

            # Indicate connection rebuilt successfully (do not emit on successful validation of existing)
            if ($nAttempt -gt 0)
            {
                LogOutput -ForegroundColor Green "SMB Mapping $localPath => $remotePath connected"
            }

            break
        }
    }

    # Drive mapping for initial/periodic validation
    $mapParam = @{
        LocalDriveLetter = 'L'
        RemotePath = '\\169.254.1.1\c$\clusterstorage\collect'
        TestPath = 'flag\pause'
        UserName = $connectuser
        Password = $connectpass
    }

    # Initial configuration checks
    ValidateSmbMapping @mapParam
    DisableWindowsRE

    # update tooling
    Copy-Item L:\tools\* C:\run -Force

    $run = 'C:\run\run.ps1'
    $control = 'C:\run\control.ps1'
    $stashDir = "C:\run\stash"

    $null = mkdir $stashDir -ErrorAction SilentlyContinue

    $myname = Get-Content C:\vmspec.txt
    $mypause = "L:\flag\pause-$myname"
    $mydone = "L:\flag\done-$myname"

    $pause = $false
    $gowait = $false
    $gowaitf = $false

    function get-newfile {
        param(
            [string]
            $Source,

            [string]
            $Dest,

            [string[]]
            $Exclude,

            [switch]
            $Silent
            )

        $gcParam = @{ Path = $Source }
        if ($PSBoundParameters.ContainsKey('Exclude'))
        {
            $gcParam['Exclude'] = $Exclude
        }
        $sf = Get-ChildItem @gcParam | Sort-Object -Property LastWriteTime -Descending | Select-Object -first 1
        $df = Get-Item $Dest -ErrorAction SilentlyContinue

        # no source?
        if ($null -eq $sf)
        {
            LogOutput -ForegroundColor Green NO source present
            return $null
        }

        # no destination, always take source
        if ($null -eq $df)
        {
            return $sf
        }

        if ($sf.LastWriteTime -gt $df.LastWriteTime)
        {
            $sfh = Get-FileHash $sf
            $dfh = Get-FileHash $df

            # files are different, indicate
            if ($sfh.Hash -ne $dfh.Hash)
            {
                return $sf
            }

            # files are the same but the last write time has moved - ignore, pull
            # up lastwrite on destination

            LogOutput -ForegroundColor Green IGNORE newer last write with no source change
            $df.LastWriteTime = $sf.LastWriteTime
        }

        # Have latest, indicate no new
        return $null
    }

    function get-flagfile(
        [ValidateSet('Go','Pause','Done')]
        [string] $Flag
        )
    {
        switch ($Flag)
        {
            'Go'    { $f = 'L:\flag\go' }
            'Pause' { $f = 'L:\flag\pause' }
            'Done'  { $f = "L:\flag\done-$myname" }
        }

        if (Test-Path $f) {
            [int](Get-Content $f -ErrorAction SilentlyContinue)
        } else {
            0
        }
    }

    # Active (responded to) and Current (content) of respective flag
    $flag = @{}
    foreach ($f in 'pause','go')
    {
        $flag.$f = [pscustomobject] @{ acked = 0; current = 0 }
    }

    # Get acked go flag at time zero; ticks forward during operation
    $flag.go.acked = get-flagfile Done

    while ($true) {

        ValidateSmbMapping @mapParam

        # update control?
        # this passes back out to the launcher which re-executes the controller
        $newf = get-newfile -Source L:\control\control.ps1 -Dest $control -Silent
        if ($null -ne $newf) {
            LogOutput "New $($newf.Name) @ $($newf.LastWriteTime)"
            Copy-Item $newf -Destination $control -Force
            break
        }

        try
        {
            # check go epoch - this can change while paused (say, clear)
            $flag.go.current = get-flagfile Go

            # outside go run, drop ack
            if ($flag.go.current -eq 0 -and $flag.go.acked -ne 0)
            {
                LogOutput "CLEAR Go ENTER FREE RUN"
                Write-Output 0 > $mydone
                $flag.go.acked = 0
            }

            # check and acknowledge pause - only drop flag once
            $flag.pause.current = get-flagfile Pause
            if ($flag.pause.current -ne 0) {

                # drop into pause flagfile if needed
                if ($pause -eq $false -or ($flag.pause.acked -ne $flag.pause.current)) {

                    LogOutput -ForegroundColor Red "PAUSED (flag @ $($flag.pause.current))"
                    Write-Output $flag.pause.current > $mypause
                    $flag.pause.acked = $flag.pause.current

                    $pause = $true
                }

                continue
            }

            # pause is now clear
            $pause = $false
            if ($flag.pause.acked -ne 0)
            {
                LogOutput -ForegroundColor Red "PAUSE RELEASED (flag @ $($flag.pause.acked))"
                $flag.pause.acked = 0

                # Reissue go wait logging as appropriate
                $gowait = $gowaitf = $false
            }

            # if go is zero this is free-running new/existing run file
            # if go is nonzero but we have already acked, we are waiting for a new go
            if ($flag.go.current -ne 0)
            {
                if ($flag.go.current -eq $flag.go.acked)
                {
                    if ($gowait -eq $false)
                    {
                        $gowait = $true
                        LogOutput -ForegroundColor Cyan "WAITING FOR GO"
                    }

                    continue
                }
            }

            # check for update to run script
            $newf = get-newfile -Source L:\control\*.ps1 -Dest $run -Exclude control.ps1
            $runf = Get-Item $run -ErrorAction SilentlyContinue

            if ($null -eq $runf -and $null -eq $newf) {
                LogOutput -ForegroundColor Yellow "NO run file local or from fleet"
                continue
            }

            # if go is nonzero, only move on new run file
            if ($flag.go.current -ne 0)
            {
                if ($null -eq $newf)
                {
                    if ($gowaitf -eq $false)
                    {
                        $gowaitf = $true
                        LogOutput -ForegroundColor Cyan "WAITING on run file for GO $($flag.go.current)"
                    }

                    continue
                }
            }

            # GO check complete, lift squelches
            $gowait = $gowaitf = $false

            # copy new file if available
            if ($null -ne $newf)
            {
                Copy-Item $newf -Destination $run -Force
                $runf = Get-Item $run
                $msg = "NEW $($newf.Name) =>"
            }
            else
            {
                $msg = "EXISTING"
            }
            LogOutput -ForegroundColor Cyan "$msg $($runf.Name) $($runf.lastwritetime)"

            # log start (free/GO)
            if ($flag.go.current -ne 0)
            {
                $msg = "GO $($flag.go.current)"
            }
            else
            {
                $msg = "FREE RUN"
            }
            LogOutput -ForegroundColor Cyan "STARTING $msg"
            LogOutput -ForegroundColor Cyan "BEGIN" ("*"*20)

            # If data disk is present, bring it online/readwrite if needed (initial state).
            # Note that only a single disk is supported at this time.
            # This is delayed so that hot-resize/attach is possible without restarting the VM.
            $d1 = Get-Disk -Number 1 -ErrorAction SilentlyContinue
            if ($null -ne $d1)
            {
                if ($d1.IsOffline)  { $d1 | Set-Disk -IsOffline:$false }
                if ($d1.IsReadOnly) { $d1 | Set-Disk -IsReadOnly:$false }
            }

            # launch and monitor pause and new run file
            $j = start-job -arg $run { param($run) & $run }
            while ($null -eq ($jf = wait-job $j -Timeout 1))
            {
                $halt = $null

                # check pause or new run file: if so, stop and loop
                if (0 -ne (get-flagfile Pause))
                {
                    $halt = 'pause set'

                    # snap diskspd process dumps in case this pause is due to forced
                    # termination as a result of non-responsive / slow result return
                    Remove-Item c:\run\diskspd*.dmp -ErrorAction SilentlyContinue -Force
                    Get-Process diskspd | Get-ProcessDump -DumpFile c:\run\diskspd.dmp |% { LogOutput "DUMP: $($_.FullName)" }
                }
                # unexpected go change?
                elseif ($flag.go.current -ne (get-flagfile Go))
                {
                    $halt = 'go change'
                }
                # normal edit/drop loop (non-go/done control) - new run file
                elseif (get-newfile -Source L:\control\*.ps1 -Dest $run -Exclude control.ps1 -Silent)
                {
                    $halt = 'new run file'
                }
                # new controller dropped
                elseif (get-newfile -Source L:\control\control.ps1 -Dest $control -Silent)
                {
                    $halt = 'new controller'
                }

                if ($null -ne $halt)
                {
                    LogOutput -ForegroundColor Yellow "STOPPING (reason: $halt)"
                    $j | stop-job
                    $j | remove-job

                    # halts end any outstanding GO run
                    if ($flag.go.current -ne 0)
                    {
                        LogOutput -ForegroundColor Yellow "HALT GO $($flag.go.current)"
                        Write-Output $flag.go.current > $mydone
                        $flag.go.acked = $flag.go.current
                    }
                    break
                }
            }

            # job finished?
            if ($null -ne $jf) {
                $result = $jf | receive-job

                if ($null -ne $result)
                {
                    # log free run completion
                    if ($flag.go.current -eq 0)
                    {
                        LogOutput -ForegroundColor Yellow "DONE CURRENT"
                    }
                    # log/ack GO completion
                    else
                    {
                        LogOutput -ForegroundColor Yellow "DONE GO $($flag.go.current)"
                        Write-Output $flag.go.current > $mydone
                    }
                    $flag.go.acked = $flag.go.current

                    # clear prior stash, if any
                    Get-ChildItem $stashDir | Remove-Item -Recurse -Force

                    # validate SMB connection before pushing results out
                    ValidateSmbMapping @mapParam

                    # propagate any results, stashing them locally along
                    # with any other content requested (triage material)
                    foreach ($f in $result.Result)
                    {
                        $null = xcopy /j /y /q $f "l:\result"
                        $null = xcopy /j /y /q $f $stashDir
                        Remove-Item $f -Force
                    }

                    foreach ($f in $result.Stash)
                    {
                        $null = xcopy /j /y /q $f $stashDir
                        Remove-Item $f -Force
                    }
                }

                $jf | remove-job
            }

            LogOutput -ForegroundColor Cyan "END" ("*"*20)
        }
        catch
        {
            $_
        }
        finally
        {
            # force gc to teardown potentially conflicting handles and enforce min pause
            [system.gc]::Collect()
            Start-Sleep 1
        }
    }
}

function Set-SweepTemplateScript
{
    # __Unique__
    # buffer size/alighment, threads/target, outstanding/thread, write%
    $b = __b__; $t = __t__; $o = __o__; $w = __w__

    # optional - specify rate limit in iops _-gNNNi
    $iops = __iops__

    # io pattern, (r)andom or (s)equential (si as needed for multithread)
    $p = '__p__'

    # durations of test, cooldown, warmup
    $d = __d__; $cool = __Cool__; $warm = __Warm__

    # handle autoscaled thread count (1 per LP/VCPU)
    if ($t -eq 0)
    {
        $t = (Get-CimInstance Win32_Processor | Measure-Object -Property NumberOfLogicalProcessors -Sum).Sum
    }

    # sweep template always captures
    $addspec = '-__AddSpec__'
    if ($addspec.Length -eq 1) { $addspec = $null }
    $iopspec = $null
    if ($null -ne $iops) { $iopspec = "iops$($iops)" }
    $result = "result-b$($b)t$($t)o$($o)w$($w)p$($p)$($iopspec)$($addspec)-$(Get-Content C:\vmspec.txt).xml"
    $dresult = "L:\result"
    $lresultf = Join-Path "C:\run" $result
    $dresultf = Join-Path $dresult $result

    ### prior to this is template

    if (-not (Get-Item $dresultf -ErrorAction SilentlyContinue)) {

        # look for data disk - if present, it will always be disk 1. disk 0 is boot/os.
        # only use it if it is a raw device. we do not have a convention for using a
        # filesystem on it at this time.
        $d1 = Get-Disk -Number 1 -ErrorAction SilentlyContinue
        if ($null -ne $d1 -and $d1.IsBoot -eq $false -and $d1.PartitionStyle -eq 'RAW')
        {
            $target = '#1'
        }
        else
        {
            $target = (Get-ChildItem C:\run\testfile?.dat)
        }

        $res = 'xml'
        $gspec = $null
        if ($null -ne $iops) { $gspec = "-g$($iops)i" }

        C:\run\diskspd.exe -Z20M -z -h `-t$t `-o$o $gspec `-b$($b)k `-$($p)$($b)k `-w$w `-W$warm `-C$cool `-d$($d) -D -L `-R$res $target > $lresultf

        [PSCustomObject]@{
            Result = @( $lresultf )
            Stash = $null
        }

    } else {

        write-host -fore green already done $dresultf

        # indicate done flag to control
        # this should only occur if control does not change variation
        [PSCustomObject]@{
            Result = $null
            Stash = $null
        }
    }

    [system.gc]::Collect()
}

function Set-RunProfileTemplateScript
{
    # __Unique__
    $addspec = '__AddSpec__'
    $result = "result"
    if ($addspec.Length) { $result += "-$($addspec)" }
    $result += "-$(Get-Content C:\vmspec.txt).xml"
    $dresult = "L:\result"
    $lresultf = Join-Path "C:\run" $result
    $dresultf = Join-Path $dresult $result

    $prof = '__Profile__'
    $lprof = "C:\run\profile.xml"

    if (-not (Get-Item $dresultf -ErrorAction SilentlyContinue)) {

        # look for data disk - if present, it will always be disk 1. disk 0 is boot/os.
        # only use it if it is a raw device. we do not have a convention for using a
        # filesystem on it at this time.
        $d1 = Get-Disk -Number 1 -ErrorAction SilentlyContinue
        if ($null -ne $d1 -and $d1.IsBoot -eq $false -and $d1.PartitionStyle -eq 'RAW')
        {
            $target = '#1'
        }
        else
        {
            $target = (Get-ChildItem C:\run\testfile?.dat)
        }

        Copy-Item $prof $lprof
        C:\run\diskspd.exe -z `-X$($lprof) $target > $lresultf

        # stash profile for triage and return payload for export
        [PSCustomObject]@{
            Result = @( $lresultf )
            Stash = @( $lprof )
        }
    } else {

        write-host -fore green already done $dresultf

        # indicate done flag to control
        # this should only occur if control does not change variation
        [PSCustomObject]@{
            Result = $null
            Stash = $null
        }
    }

    [system.gc]::Collect()
}

###################

# Common functions for local/remote sessions

$CommonFunc = {

    #
    # Globals
    #

    enum PathType {

        # Paths
        Collect
        Control
        Result
        Tools
        Flag

        # Files
        Pause
        Go
        Done
    }

    $collectVolumeName = 'collect'

    function CreateErrorRecord
    {
        [CmdletBinding()]
        param
        (
            [String]
            $ErrorId,

            [String]
            $ErrorMessage,

            [System.Management.Automation.ErrorCategory]
            $ErrorCategory,

            [Exception]
            $Exception,

            [Object]
            $TargetObject
        )

        if($null -eq $Exception)
        {
            $Exception = New-Object System.Management.Automation.RuntimeException $ErrorMessage
        }

        $errorRecord = New-Object System.Management.Automation.ErrorRecord @($Exception, $ErrorId, $ErrorCategory, $TargetObject)
        return $errorRecord
    }

    # Copy key/value from Source->Destination hash if present
    function CopyKeyIf
    {
        param(
            [Parameter(Mandatory = $true, Position = 0)]
            [hashtable]
            $HSource,

            [Parameter(Mandatory = $true, Position = 1)]
            [hashtable]
            $HDest,

            [Parameter(Mandatory = $true, Position = 2)]
            [string]
            $Key
        )

        if ($HSource.ContainsKey($Key))
        {
            $HDest[$Key] = $HSource[$Key]
        }
    }

    function LogOutput
    {
        [CmdletBinding()]
        param(
            [Parameter(Mandatory = $true, Position = 0, ValueFromRemainingArguments)]
            [ValidateNotNullOrEmpty()]
            [string[]]
            $Message,

            [Parameter()]
            [System.ConsoleColor]
            $ForegroundColor,

            [Parameter()]
            [switch]
            $IsVb,

            [Parameter()]
            [switch]
            $NoNewline,

            [Parameter()]
            [switch]
            $CarriageReturn
        )

        $m = ((Get-Date).GetDateTimeFormats('s')[0] + ": $($Message -join ' ')")
        if ($PSBoundParameters.ContainsKey('IsVb'))
        {
            Write-Verbose $m
        }
        else
        {
            $whParam = @{}
            CopyKeyIf $PSBoundParameters $whParam 'ForegroundColor'
            CopyKeyIf $PSBoundParameters $whParam 'NoNewline'
            if ($PSBoundParameters.ContainsKey('CarriageReturn'))
            {
                $m += "`r"
            }
            Write-Host @whParam $m
        }
    }

    #
    #  Convert an absolute local path to the equivalent remote path via SMB admin shares
    #  ex: C:\foo\bar & scratch -> \\scratch\C$\foo\bar
    #

    function Get-AdminSharePathFromLocal
    {
        [CmdletBinding()]
        param(
            [Parameter()]
            [string]
            $Node,

            [Parameter()]
            [string]
            $LocalPath
        )

        "\\"+$Node+"\"+$LocalPath[0]+"$\"+$LocalPath.Substring(3,$LocalPath.Length-3)
    }
    function Stop-After($step)
    {
        if ($stopafter -eq $step) {
            Write-Host -ForegroundColor Green Stop after $step
            return $true
        }
        return $false
    }

    function Get-AccessNode
    {
        [CmdletBinding()]
        param (
            [Parameter()]
            [string]
            $Cluster = '.'
        )

        $nodes = @(Get-ClusterNode @PsBoundParameters |? State -eq Up)
        if ($nodes.Length -eq 0)
        {
            throw "No nodes of cluster $Cluster are accessible at this time"
        }

        $nodes[0].Name
    }

    #
    # This helper function deals with stable identification of the CSV corresponding
    # to a given virtual disk.
    #
    # In system restore case where a cluster is brought up on previously configured storage
    # the CSV name will not restate the virtualdisk name. By looking at the volume device path
    # seen by CSV and the volume stack we can make the association. At the volume layer the
    # filesystem label will (should) reliably give us the name mapping - and is defensive against
    # potentially renamed CSV mounts.
    #
    # The name is attached to the CSV object as the VDName property.
    #
    function GetMappedCSV
    {
        [CmdletBinding()]
        param(
            [Parameter()]
            [string]
            $Cluster = '.'
            )

        $node = Get-AccessNode @PsBoundParameters
        $csv = Get-ClusterSharedVolume @PsBoundParameters

        # Map of volume path (ex: \?\Volume{9759283a-54c7-40b3-b07d-faef008e1a8d}\) to volume object
        $vh = @{}
        Get-Volume -CimSession $node |? FileSystem -eq CSVFS |% { $vh[$_.Path] = $_ }

        $csv |% {

            # If the CSV maps to a visible volume, attach the filesystemlabel of the respective volume
            # to it. Its volume path us mentioned here under the CSV object.
            $v = $vh[$_.SharedVolumeInfo.Partition.Name]

            if ($null -ne $v)
            {
                $VDName = $v.FileSystemLabel
            }
            else
            {
                $VDName = $null
            }

            # Add and emit
            $_ | Add-Member -NotePropertyName VDName -NotePropertyValue $VDName -PassThru
        }
    }

    function Get-FleetPath
    {
        [CmdletBinding()]
        param (
            [Parameter()]
            [string]
            $Cluster = ".",

            [Parameter()]
            [PathType[]]
            $PathType,

            [Parameter()]
            [switch]
            $Local
        )

        $c = Get-Cluster -Name $Cluster
        if ($null -eq $c)
        {
            return $null
        }

        $accessNode = $null
        if (-not $Local -and $PSBoundParameters.ContainsKey("Cluster"))
        {
            $accessNode = Get-AccessNode -Cluster $Cluster
        }

        $clusterStorage = $c.SharedVolumesRoot
        $collectPath = Join-Path $clusterStorage $collectVolumeName

        foreach ($type in $PathType)
        {
            $path = switch ($type)
            {
                ([PathType]::Collect)   { $collectPath }
                ([PathType]::Control)   { Join-Path $collectPath "control" }

                # Note: these paths should move up to collect at v1.0 (don't bury them in control)

                ([PathType]::Result)    { Join-Path $collectPath "result" }
                ([PathType]::Tools)     { Join-Path $collectPath "tools" }
                ([PathType]::Flag)      { Join-Path $collectPath "flag" }

                ([PathType]::Done)      { Join-Path $collectPath "flag\done" }
                ([PathType]::Go)        { Join-Path $collectPath "flag\go" }
                ([PathType]::Pause)     { Join-Path $collectPath "flag\pause" }
            }

            if ($null -ne $accessNode)
            {
                $path = Get-AdminSharePathFromLocal -Node $accessNode $path
            }

            $path
        }
    }

    function GetLogmanLocal
    {
        [CmdletBinding()]
        param(
            [Parameter(Mandatory = $true)]
            [string]
            $Name
            )

        $raw = logman query $Name
        if ($?)
        {
            $inctrs = $false
            $ctrs = @()
            $props = @{}

            $raw |% {
                if ($inctrs)
                {
                    # Trim leading spaces. Empty terminates the block.
                    $ctr = $_ -replace '^\s+',''
                    if ($ctr.Length -eq 0)
                    {
                        $inctrs = $false
                    }
                    else
                    {
                        $ctrs += $ctr
                    }
                }
                elseif ($_ -match '\S+\:\s+\S+')
                {
                    $kv = $_ -split ':\s+',2
                    # Basic k: v property
                    $props[$kv[0] -replace '\s',''] = $kv[1]
                }
                elseif ($_ -eq 'Counters:')
                {
                    # Begin consuming counter names
                    $inctrs = $true
                }
            }

            $props['Counters'] = $ctrs
            [pscustomobject] $props
        }
    }
}

# Evaluate common block into local session
. $CommonFunc

# Utility functions only for the local session

function TimespanToString
{
    param(
        [timespan] $TimeSpan
    )

    # Autoranging output
    if ($TimeSpan.TotalDays -ge 1)
    {
        $TimeSpan.ToString("dd\d\.hh\h\:mm\m\:ss\.f\s")
    }
    elseif ($TimeSpan.TotalHours -ge 1)
    {
        $TimeSpan.ToString("hh\h\:mm\m\:ss\.f\s")
    }
    elseif ($TimeSpan.TotalMinutes -ge 1)
    {
        $TimeSpan.ToString("mm\m\:ss\.f\s")
    }
    else
    {
        $TimeSpan.ToString("ss\.f\s")
    }
}

# Produce a good-enough parse of logman collector status
function GetLogman
{
    [CmdletBinding()]
    param(
        [Parameter(
            Mandatory = $true
        )]
        [string[]]
        $Computer,

        [Parameter()]
        [string]
        $Name = ""
        )

    $n = "perfctr"
    if ($Name.Length) { $n += "-$Name" }

    Invoke-CommonCommand -ComputerName $Computer -InitBlock $CommonFunc -ScriptBlock {

        GetLogmanLocal -Name $using:n
    }
}

# Start/restart perf collection on given computer(s)/counter set with named blg
function StartLogman
{
    [CmdletBinding()]
    param(
        [Parameter(
            Mandatory = $true
            )]
        [string[]]
        $Computer,

        [Parameter()]
        [string]
        $Name = "",

        [Parameter(
            Mandatory = $true
            )]
        [string[]]
        $Counters,

        [Parameter()]
        [ValidateRange(1,60)]
        [int]
        $SampleInterval = 1
    )

    $n = "perfctr"
    if ($Name.Length) { $n += "-$Name" }

    Invoke-CommonCommand -ComputerName $Computer -InitBlock $CommonFunc -ScriptBlock {

        # clean up any pre-existing collector & corresponding blg due to failed teardown/et.al.
        $existing = GetLogmanLocal -Name $using:n
        if ($null -ne $existing)
        {
            Write-Verbose "logman stop/delete pre-existing $($using:n)"
            if ($existing.Status -eq 'Running')
            {
                $null = logman stop $using:n
            }
            $null = logman delete $using:n
            if (Test-Path $existing.OutputLocation)
            {
                Remove-Item $existing.OutputLocation -Force
            }
        }

        $f = Join-Path $env:TEMP "$($using:n)-$($env:COMPUTERNAME).blg"

        # clean up any stale pre-existing blg
        if (Test-Path $f) { Remove-Item $f -Force }

        $o = logman create counter $using:n -o $f -f bin -si $using:SampleInterval --v -c $using:Counters 2>&1
        $s = $?
        $m = "logman create @ $env:COMPUTERNAME : $o"
        if (-not $s) { Write-Error $m } else { Write-Verbose $m }

        $o = logman start $using:n
        $s = $?
        $m = "logman start @ $env:COMPUTERNAME : $o"
        if (-not $s) { Write-Error $m } else { Write-Verbose $m }
    }
}

# Stop perf collection on given computer(s)/counter set with named blg
# Note that copyout is remoted; assumes $Computer is a cluster node and $Path is CSV
function StopLogman
{
    [CmdletBinding()]
        param(
        [Parameter(
            Mandatory = $true
            )]
        [string[]]
        $Computer,

        [Parameter()]
        [string]
        $Name = "",

        [Parameter()]
        [string] $Path
    )

    $n = "perfctr"
    if ($Name.Length) { $n += "-$Name" }

    Invoke-CommonCommand -ComputerName $Computer -InitBlock $CommonFunc -ScriptBlock {
        $f = Join-Path $env:TEMP "$($using:n)-$($env:COMPUTERNAME).blg"

        $o = logman stop $using:n 2>&1
        $s = $?
        $m = "logman stop @ $env:COMPUTERNAME : $o"
        if (-not $s) { Write-Error $m } else { Write-Verbose $m }

        $o = logman delete $using:n 2>&1
        $s = $?
        $m = "logman delete @ $env:COMPUTERNAME : $o"
        if (-not $s) { Write-Error $m } else { Write-Verbose $m }

        if ($using:Path.Length)
        {
            $o = xcopy /j /y /q $f $using:Path 2>&1
            $s = $?
            $m = "xcopy @ $env:COMPUTERNAME : $o"
            if (-not $s) { Write-Error $m } else { Write-Verbose $m }
        }

        Remove-Item -Force $f
    }
}

#
# Invoke commands with a common initialization block of utility fns.
# When -AsJob, the sessions are persisted onto the parent job object
# for later cleanup on completion.
#
function Invoke-CommonCommand
{
    [CmdletBinding(DefaultParameterSetName = "Default")]
    param(

        [Parameter(ParameterSetName = "Default")]
        [Parameter(ParameterSetName = "AsJob")]
        [Parameter(Position = 0)]
        [string[]]
        $ComputerName = @(),

        [Parameter(ParameterSetName = "Default")]
        [Parameter(ParameterSetName = "AsJob")]
        [scriptblock]
        $InitBlock,

        [Parameter(
            ParameterSetName = "Default",
            Mandatory = $true
            )]
        [Parameter(
            ParameterSetName = "AsJob",
            Mandatory = $true
            )]
        [scriptblock]
        $ScriptBlock,

        [Parameter(ParameterSetName = "Default")]
        [Parameter(ParameterSetName = "AsJob")]
        [Object[]]
        $ArgumentList,

        [Parameter(
            ParameterSetName = "AsJob",
            Mandatory = $true
            )]
        [switch]
        $AsJob,

        [Parameter(ParameterSetName = "AsJob")]
        [string]
        $JobName
    )

    $Sessions = @()
    if ($ComputerName.Count -eq 0)
    {
        $Sessions = New-PSSession -Cn localhost -EnableNetworkAccess
    }
    else
    {
        $Sessions = New-PSSession -ComputerName $ComputerName
    }

    # Guarantee session cleanup if exception thrown w/o either attaching to job
    # for later removal or synch completion of work.
    try
    {
        # Replay the verbose preference onto the seesions so logging is passed
        Invoke-Command -Session $Sessions { $VerbosePreference = $using:VerbosePreference }

        if ($null -ne $InitBlock)
        {
            Invoke-Command -Session $Sessions $InitBlock
        }

        switch($PSCmdlet.ParameterSetName)
        {
            "AsJob"
            {
                Invoke-Command -Session $Sessions -AsJob -JobName $JobName -ScriptBlock $ScriptBlock -ArgumentList $ArgumentList |
                    Add-Member -NotePropertyName ActiveSession -NotePropertyValue $Sessions.Id -PassThru

                # Attached, no cleanup here.
                $Sessions = @()
            }

            default
            {
                Invoke-Command -Session $Sessions -ScriptBlock $ScriptBlock -ArgumentList $ArgumentList
                $Sessions | Remove-PSSession
            }
        }
    }
    finally
    {
        $Sessions | Remove-PSSession
    }
}

function RemoveCommonJobSession
{
    param (
        [Parameter(ValueFromPipeline = $true)]
        [object]
        $j
        )

    # Remove sessions from completed CommonCommand jobs

    process {
        if (Get-Member -InputObject $j ActiveSession)
        {
             Remove-PSSession -Id $j.ActiveSession
        }
    }
}

#######################

function Get-FleetVersion
{
    [CmdletBinding()]
    param (
        [Parameter()]
        [string]
        $Cluster = "."
    )

    $path = Get-FleetPath -PathType Tools @PSBoundParameters

    $vmfleet = Get-Command Get-FleetVersion
    [PSCustomObject]@{
        Component = $vmfleet.Source
        Version = $vmfleet.Version
    }

    $diskspdPath = (Join-Path $path 'diskspd.exe')
    if (Test-Path $diskspdPath)
    {
        $diskspd = Get-Command $diskspdPath
        [PSCustomObject]@{
            Component = $diskspd.Name
            Version = $diskspd.Version
        }
    }
}

function Get-FleetPauseEpoch
{
    [CmdletBinding(DefaultParameterSetName = "ByCluster")]
    param (
        [Parameter(ParameterSetName = "ByCluster")]
        [string]
        $Cluster = ".",

        [Parameter(ParameterSetName = "ByPath")]
        [string]
        $Path
    )

    switch($PSCmdlet.ParameterSetName)
    {
        "ByCluster" { $Path = Get-FleetPath -PathType Pause @PSBoundParameters }
    }

    $ep = Get-Content $Path -ErrorAction SilentlyContinue
    if ($null -eq $ep)
    {
        $ep = 0
    }

    $ep
}

#
# Get-FleetPause
#
function Get-FleetPause
{
    [CmdletBinding(DefaultParameterSetName = "ByCluster")]
    param (
        [Parameter(ParameterSetName = "ByCluster")]
        [string]
        $Cluster = ".",

        [Parameter(ParameterSetName = "ByPath")]
        [string]
        $Path
    )

    $pauseEpoch = Get-FleetPauseEpoch @PSBoundParameters -ErrorAction SilentlyContinue

    # non-0 indicates VMs are paused
    $pauseEpoch -ne 0
}

#
# Show-Pause - text report of pause state
#
function Show-FleetPause
{
    [CmdletBinding(DefaultParameterSetName = "ByCluster")]
    param (
        [Parameter(ParameterSetName = "ByCluster")]
        [string]
        $Cluster = ".",

        [Parameter(ParameterSetName = "ByPath")]
        [string]
        $Path
    )

    $pausePath = Get-FleetPath -PathType Pause @PSBoundParameters
    $pauseEpoch = Get-FleetPauseEpoch -Path $pausePath -ErrorAction SilentlyContinue
    if ($pauseEpoch -eq 0)
    {
        Write-Host -ForegroundColor red "Pause not in force"
        return
    }

    # Now correlate to online vms and see if we agree all online are paused.

    $vms = @(Get-ClusterGroup |? GroupType -eq VirtualMachine |? Name -like 'vm-*' |? State -eq Online)
    $pausedVMs = @($vms | FilterPausedVMs -Path $pausePath)

    if ($pausedVMs.Count -eq $vms.Count)
    {
        Write-Host -ForegroundColor green "OK: All $($vms.count) VMs paused"
    }
    else
    {
        Write-Host -fore red "WARNING: of $($vms.Count), still waiting on $($vms.Count - $pausedVMs.Count) to acknowledge pause"

        Compare-Object $vms $pausedVMs -Property Name -PassThru | Sort-Object -Property Name | Format-Table -AutoSize Name,OwnerNode | Out-Host
    }
}

function FilterPausedVMs
{
    [CmdletBinding()]
    param(
        [Parameter(ParameterSetName = "ByPath")]
        [string]
        $Path,

        [Parameter(ValueFromPipeline = $true)]
        [Microsoft.FailoverClusters.PowerShell.ClusterGroup[]]
        $VM
    )

    begin
    {
        $pauseEpoch = Get-FleetPauseEpoch -Path $Path -ErrorAction SilentlyContinue

        # accumulate hash of pause flags mapped to current/stale state
        $h = @{}

        Get-ChildItem "$($Path)-*" |% {

            $thisPause = Get-Content $_ -ErrorAction SilentlyContinue
            if ($thisPause -eq $pauseEpoch)
            {
                $pauseType = [PauseState]::Current
            }
            else
            {
                $pauseType = [PauseState]::Stale
            }

            if ($_.name -match 'pause-(vm.+)')
            {
                # VM Name
                $h[$matches[1]] = $pauseType
            }
            else
            {
                Write-Warning "malformed pause $($_.Name) present"
            }
        }
    }

    process
    {
        # Pass through all VMs which have responsed to the current pause.
        if ($h[$VM.Name] -eq [PauseState]::Current)
        {
            $VM
        }
    }
}

function Set-FleetPause
{
    [CmdletBinding(DefaultParameterSetName = "ByCluster")]
    param (
        [Parameter(ParameterSetName = "ByCluster")]
        [string]
        $Cluster = ".",

        [Parameter(ParameterSetName = "ByPath")]
        [string]
        $Path,

        [Parameter()]
        [ValidateRange(0,120)]
        [int]
        $Timeout = 120,

        [Parameter()]
        [switch]
        $Force
    )

    # Parameter set specifying cluster only
    $clusterParam = @{}
    CopyKeyIf $PSBoundParameters $clusterParam 'Cluster'

    $pausePath = Get-FleetPath @clusterParam -PathType Pause

    if ((-not $Force) -and (Get-FleetPause -Path $pausePath))
    {
        LogOutput -IsVb "pause is already set @ $((Get-Item $pausePath).LastWriteTime)"
        return
    }

    # if forcing (live check), there may be a nonzero existing pause.
    # ensure the new nonzero pause flag is distinct.
    $currentPause = Get-FleetPauseEpoch -Path $pausePath
    do {
        $newPause = Get-Random -Minimum 1
        if ($newPause -eq $currentPause) { continue }
        Write-Output $newPause > $pausePath
        break
    } until ($false)

    LogOutput -IsVb "pause set @ $((Get-Item $pausePath).LastWriteTime)"

    # Wait for pause to settle?
    if ($Timeout -gt 0)
    {
        $t0 = Get-Date
        $vms = @(Get-ClusterGroup @clusterParam |? GroupType -eq VirtualMachine |? Name -like 'vm-*' |? State -eq Online)

        do {

            $td = (Get-Date) - $t0

            $pausedVMs = @($vms | FilterPausedVMs -Path $pausePath)
            if ($pausedVMs.Count -eq $vms.Count)
            {
                break
            }

            if ($td.TotalSeconds -gt $Timeout)
            {
                Write-Error "Wait for pause ack timed out - only $($pausedVMs.Count)/$($vms.Count) pause responses. Check fleet health with Get-FleetVM -ControlResponse and Repair-Fleet if needed."
                return
            }

            LogOutput -IsVb "WAIT pause ack @ $($pausedVMs.Count)/$($vms.Count) paused"
            Start-Sleep 2

        } while ($true)

        LogOutput -IsVb "pause ack $($vms.Count) total ($('{0:0.0}' -f $td.TotalSeconds)s to ack)"
    }
}

function Clear-FleetPause
{
    [CmdletBinding(DefaultParameterSetName = "ByCluster")]
    param (
        [Parameter(ParameterSetName = "ByCluster")]
        [string]
        $Cluster = ".",

        [Parameter(ParameterSetName = "ByPath")]
        [string]
        $Path
    )

    $pausePath = Get-FleetPath -PathType Pause @PSBoundParameters

    if (-not (Get-FleetPause -Path $pausePath))
    {
        LogOutput -IsVb "pause is already cleared @ $((Get-Item $pausePath).LastWriteTime)"
        return
    }

    Write-Output 0 > $pausePath
    LogOutput -IsVb "pause cleared @ $((Get-Item $pausePath).LastWriteTime)"
}

function Install-Fleet
{
    [CmdletBinding()]
    param (
        [Parameter()]
        [string]
        $Cluster = ".",

        [Parameter()]
        [switch]
        $KeepBlockCache,

        [Parameter()]
        [switch]
        $NoTools,

        [Parameter()]
        [switch]
        $Force
    )

    # Parameter set specifying cluster only
    $clusterParam = @{}
    CopyKeyIf $PSBoundParameters $clusterParam 'Cluster'

    #
    # Ensure there is a collect volume and at least 1 CSV with <nodename> prefix per
    # node of the cluster.
    #

    $csvs = @(GetMappedCSV @clusterParam)
    $nodes = @(Get-ClusterNode @clusterParam)

    # Now accumulate the other potential configuration errors so we can emit all of them before stopping
    $e = $false

    # Ensure there is a collect volume.
    if ($csvs.Count -eq 0 -or $csvs.VDName -notcontains $collectVolumeName)
    {
        $errorObject = CreateErrorRecord -ErrorId "No $($collectVolumeName) CSV found" `
                                        -ErrorMessage "VM Fleet requires one volume named '$($collectVolumeName)' which will hold VM Fleet data/scripts" `
                                        -ErrorCategory ([System.Management.Automation.ErrorCategory]::ObjectNotFound) `
                                        -Exception $null `
                                        -TargetObject $null

        $psCmdlet.WriteError($errorObject)
        $e = $true
    }
    else
    {
        LogOutput -IsVb "collect volume exists"
    }

    # Ensure there is at least one volume per node. Note this is warning since the volume size estimator is neccesarily part
    $h = @{ }
    foreach ($node in $nodes)
    {
        $h[$node] = 0
    }

    foreach ($csv in $csvs)
    {
        foreach ($node in $nodes)
        {
            if ($csv.VDName -match "^$node(?:-.+){0,1}$")
            {
                LogOutput -IsVb "volume $($csv.VDName) matched to node $node"
                $h[$node] += 1
                break
            }
        }
    }

    foreach ($node in $nodes)
    {
        if ($h[$node] -eq 0)
        {
            $errorObject = CreateErrorRecord -ErrorId "No CSV found for node" `
                                            -ErrorMessage "Node $($node): VM Fleet requires at least one volume prefixed with node name which will be used to hold VM data/metadata. Use Get-FleetVolumeEstimate for recommended sizings." `
                                            -ErrorCategory ([System.Management.Automation.ErrorCategory]::ObjectNotFound) `
                                            -Exception $null `
                                            -TargetObject $null

            $psCmdlet.WriteError($errorObject)
            $e = $true
        }
    }

    # Now halt if volume configuration was incomplete/missing.
    if ($e)
    {
        return
    }

    # Install directory structure

    $paths = ($controlPath, $flagPath, $resultPath, $toolsPath) = Get-FleetPath -PathType Control,Flag,Result,Tools @clusterParam
    foreach ($path in $paths)
    {
        if (-not (Test-Path $path))
        {
            LogOutput -IsVb "installing path: $path"
            $null = New-Item -Path $path -ItemType Directory
        }
        else
        {
            LogOutput -IsVb "path exists: $path"
        }
    }

    # Set fleet pause so that VMs come up idle
    Set-FleetPause -Timeout 0

    #
    # Unwrap VM scripts (control, run*)
    #

    $f = Join-Path $controlPath "control.ps1"
    if (-not (Test-Path $f) -or $Force)
    {
        $c = Get-Command Set-FleetControlScript
        if (Test-Path $f) { Remove-Item $f -Force }
        $c.ScriptBlock | Out-File -FilePath "$($f).tmp" -Width ([int32]::MaxValue) -Force
        Move-Item "$($f).tmp" $f
    }

    $f = Join-Path $controlPath "run.ps1"
    if (-not (Test-Path $f) -or $Force)
    {
        $c = Get-Command Set-FleetBaseScript
        if (Test-Path $f) { Remove-Item $f -Force }
        $c.ScriptBlock | Out-File -FilePath "$($f).tmp" -Width ([int32]::MaxValue) -Force
        Move-Item "$($f).tmp" $f
    }

    if (-not $KeepBlockCache)
    {
        LogOutput -IsVb "disabling CSV block cache (recommended)"
        (Get-Cluster @clusterParam).BlockCacheSize = 0
    }

    if (-not $NoTools)
    {
        # DISKSPD

        $diskspdPath = (Join-Path $toolsPath "diskspd.exe")
        $exist = Test-Path $diskspdPath
        if (-not $exist -or $Force)
        {
            if ($exist)
            {
                LogOutput -IsVb "removing prior install of DISKSPD"
                Remove-Item -Force $diskspdPath
            }

            try
            {
                # Temporary ZIP file. Expand-Archive insists on the extension.
                $z = New-TemporaryFile
                $z = Rename-Item $z $($z.BaseName + ".zip") -PassThru

                # Temporary directory for extraction.
                $d = New-TemporaryFile
                Remove-Item $d
                $d = New-Item -ItemType Directory $d

                LogOutput -IsVb "downloading DISKSPD"

                try {

                    $diskspdURI = "https://aka.ms/getdiskspd"

                    # Force TLS1.2. Downlevel TLS defaults may be rejected. Restore after download.
                    $oldTls = [Net.ServicePointManager]::SecurityProtocol
                    [Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12
                    Invoke-WebRequest -Uri $diskspdURI -OutFile $z
                    [Net.ServicePointManager]::SecurityProtocol = $oldTls

                    Expand-Archive $z -DestinationPath $d

                    # Now pull the architecture appropriate diskspd and remove the zip content
                    Copy-Item (Join-Path $d (Join-Path $env:PROCESSOR_ARCHITECTURE "diskspd.exe")) $toolsPath
                }
                catch
                {
                    Write-Warning "Cannot download DISKSPD from public site @ $diskspdURI : please acquire and place @ $toolsPath before running loads requiring it "
                }

            }
            finally
            {
                if (Test-Path $z) { Remove-Item $z -Force }
                if (Test-Path $d ) { Remove-Item $d -Recurse -Force}
            }
        }
        else
        {
            LogOutput -IsVb "DISKSPD already present"
        }
    }

    #
    # Speedbrake: Fleet requires DISKSPD >= 2.1.0.0 for new features
    #

    $v = Get-FleetVersion
    $diskspd = $v |? Component -eq diskspd.exe
    $vmfleet = $v |? Component -eq VMFleet

    if ($null -eq $diskspd)
    {
        Write-Error ("DISKSPD is not present. VM Fleet {0} requires DISKSPD for its core measurement functionality. Please update and, if running, restart the fleet to pick up the tool." -f (
            $vmfleet.Version))
    }
    elseif ($diskspd.Version.CompareTo([Version]'2.1.0.0') -lt 0)
    {
        Write-Error ("The installed DISKSPD is version {0}. VM Fleet {1} requires the functionality of DISKSPD 2.1.0.0. Please update and, if running, restart the fleet to pick up the new version." -f (
            $diskspd.Version,
            $vmfleet.Version))
    }
}

function New-Fleet
{
    [CmdletBinding(DefaultParameterSetName = "ByCluster")]
    param(
        [Parameter(ParameterSetName = "ByCluster")]
        [string]
        $Cluster = ".",

        [Parameter(ParameterSetName = "ByNode", Mandatory = $true)]
        <#
        [ArgumentCompleter({
            param ( $commandName,
                    $parameterName,
                    $wordToComplete,
                    $commandAst,
                    $fakeBoundParameters )
            # Perform calculation of tab completed values here.
        })]
        #>
        [string[]]
        $Node,

        [Parameter(ParameterSetName = "ByCluster")]
        [Parameter(ParameterSetName = "ByNode")]
        [Parameter(Mandatory = $true)]
        [ValidateNotNullOrEmpty()]
        [string]
        $BaseVHD,

        [Parameter(ParameterSetName = "ByCluster")]
        [Parameter(ParameterSetName = "ByNode")]
        [ValidateRange(1, 4096)]
        [int]
        $VMs,

        [Parameter(ParameterSetName = "ByCluster")]
        [Parameter(ParameterSetName = "ByNode")]
        [string[]]
        $Groups = @(),

        [Parameter(ParameterSetName = "ByCluster")]
        [Parameter(ParameterSetName = "ByNode")]
        [Parameter(Mandatory = $true)]
        [ValidateNotNullOrEmpty()]
        [string]
        $AdminPass,

        [Parameter(ParameterSetName = "ByCluster")]
        [Parameter(ParameterSetName = "ByNode")]
        [ValidateNotNullOrEmpty()]
        [string]
        $Admin = 'administrator',

        [Parameter(ParameterSetName = "ByCluster")]
        [Parameter(ParameterSetName = "ByNode")]
        [Parameter(Mandatory = $true)]
        [ValidateNotNullOrEmpty()]
        [string]
        $ConnectPass,

        [Parameter(ParameterSetName = "ByCluster")]
        [Parameter(ParameterSetName = "ByNode")]
        [Parameter(Mandatory = $true)]
        [ValidateNotNullOrEmpty()]
        [string]
        $ConnectUser,

        [Parameter(ParameterSetName = "ByCluster")]
        [Parameter(ParameterSetName = "ByNode")]
        [ValidateSet('CreateVMSwitch','CopyVHD','CreateVM','CreateVMGroup','AssertComplete')]
        [string]
        $StopAfter,

        [Parameter(ParameterSetName = "ByCluster")]
        [Parameter(ParameterSetName = "ByNode")]
        [ValidateSet('Force','Auto','None')]
        [string]
        $Specialize = 'Auto',

        [Parameter(ParameterSetName = "ByCluster")]
        [Parameter(ParameterSetName = "ByNode")]
        [switch]
        $KeepVHD
    )

    # Parameter set specifying cluster only
    $clusterParam = @{}
    CopyKeyIf $PSBoundParameters $clusterParam 'Cluster'

    ##################

    # validate existence of BaseVHD
    if (-not (Test-Path -Path $BaseVHD)) {
        $errorObject = CreateErrorRecord -ErrorId "No BaseVHD" `
                                        -ErrorMessage "The base VHD path $BaseVHD was not found" `
                                        -ErrorCategory ([System.Management.Automation.ErrorCategory]::ObjectNotFound) `
                                        -Exception $null `
                                        -TargetObject $null

        $psCmdlet.WriteError($errorObject)
        return
    }

    # resolve BaseVHD to fully qualified path
    $BaseVHD = (Get-Item $BaseVHD).FullName

    if (@(Get-ClusterNode @clusterParam |? State -ne Up).Count -ne 0) {
        $errorObject = CreateErrorRecord -ErrorId "Cluster nodes not up" `
                                        -ErrorMessage "Some nodes of cluster $Cluster are not up - please address before creating the fleet" `
                                        -ErrorCategory ([System.Management.Automation.ErrorCategory]::ResourceUnavailable) `
                                        -Exception $null `
                                        -TargetObject $null

        $psCmdlet.WriteError($errorObject)
        return
    }

    switch ($psCmdlet.ParameterSetName)
    {
        "ByCluster" { $Nodes = @(Get-ClusterNode @clusterParam) }
    }

    # convert to fixed vhd(x) if needed
    if ((Get-Vhd $BaseVHD).VhdType -ne 'Fixed' -and -not $KeepVHD) {

        # push dynamic vhd to tmppath and place converted at original
        # note that converting a dynamic will leave a sparse hole on refs
        # this is OK, since the copy will not copy the hole
        $f = Get-Item $BaseVHD
        $tmpname = "tmp-$($f.Name)"
        $tmppath = Join-Path $f.DirectoryName $tmpname
        Remove-Item -Force $tmppath -ErrorAction SilentlyContinue
        Rename-Item $f.FullName $tmpname

        LogOutput -IsVb "convert $($f.FullName) to fixed via $tmppath"
        Convert-VHD -Path $tmppath -DestinationPath $f.FullName -VHDType Fixed
        if (-not $?) {
            Rename-Item $tmppath $f.Name

            # allow likely error from Convert-VHD to flow out
            return
        }

        Remove-Item $tmppath
    }

    #
    # Autoscaled VMs (1 per physical core) if not specified
    #

    if (-not $PSBoundParameters.ContainsKey('VMs'))
    {
        $g = @(Get-CimInstance -CimSession $nodes.Name Win32_Processor | Group-Object -Property NumberOfEnabledCore)

        if ($g.Count -eq 0)
        {
            $errorObject = CreateErrorRecord -ErrorId "Cannot query enabled processor cores" `
                                            -ErrorMessage "Cannot query enabled processor core from cluster nodes" `
                                            -ErrorCategory ([System.Management.Automation.ErrorCategory]::ResourceUnavailable) `
                                            -Exception $null `
                                            -TargetObject $null

            $psCmdlet.WriteError($errorObject)
            return
        }

        if ($g.Count -ne 1 -or $g[0].Group.Count % $nodes.Count)
        {
            $errorObject = CreateErrorRecord -ErrorId "Non-uniform processors" `
                                            -ErrorMessage "Cluster $Cluster nodes have processors with differing numbers of enabled processor cores - please verify configuration" `
                                            -ErrorCategory ([System.Management.Automation.ErrorCategory]::ResourceUnavailable) `
                                            -Exception $null `
                                            -TargetObject $null

            $psCmdlet.WriteError($errorObject)
            return
        }

        $VMs = ($g[0].Group.NumberOfEnabledCore | Measure-Object -Sum).Sum / $nodes.Count

        LogOutput -IsVb "Autoscaling number of VMs to $VMs / node"
    }

    # Create the fleet vmswitches with a fixed IP at the base of the APIPA range
    Invoke-CommonCommand $nodes -InitBlock $CommonFunc -ScriptBlock {

        if (-not (Get-VMSwitch -Name $using:vmSwitchName -ErrorAction SilentlyContinue)) {

            $null = New-VMSwitch -name $using:vmSwitchName -SwitchType Internal
            $null = Get-NetAdapter |? DriverDescription -eq 'Hyper-V Virtual Ethernet Adapter' |? Name -eq "vEthernet ($($using:vmSwitchName))" | New-NetIPAddress -PrefixLength 16 -IPAddress $using:vmSwitchIP
            LogOutput "CREATE internal vmswitch $($using:vmSwitchName) @ $($env:COMPUTERNAME)"
        }
    }

    #### STOPAFTER
    if (Stop-After "CreateVMSwitch" $stopafter) {
        return
    }

    # create $vms vms per each csv named as <nodename><group prefix>
    # vm name is vm-<group prefix><$group>-<hostname>-<number>

    # note that this would pass as a shallow copy of the object; this is why they are unpacked
    # into a (flat) custom object
    $csvs = GetMappedCSV @clusterParam |% { [PSCustomObject]@{
        VDName = $_.VDName
        FriendlyVolumeName = $_.SharedVolumeInfo.FriendlyVolumeName
    }}

    Invoke-CommonCommand $nodes -InitBlock $CommonFunc -ArgumentList @(,$csvs) -ScriptBlock {

        param($csvs)

        function ApplySpecialization( $path, $vmspec )
        {
            # all steps here can fail immediately without cleanup

            # error accumulator
            $ok = $true

            # create run directory

            Remove-Item -Recurse -Force z:\run -ErrorAction SilentlyContinue
            mkdir z:\run
            $ok = $ok -band $?
            if (-not $ok) {
                Write-Error "failed run directory creation for $vhdpath"
                return $ok
            }

            # autologon
            $null = reg load 'HKLM\tmp' z:\windows\system32\config\software
            $ok = $ok -band $?
            $null = reg add 'HKLM\tmp\Microsoft\Windows NT\CurrentVersion\WinLogon' /f /v DefaultUserName /t REG_SZ /d $using:admin
            $ok = $ok -band $?
            $null = reg add 'HKLM\tmp\Microsoft\Windows NT\CurrentVersion\WinLogon' /f /v DefaultPassword /t REG_SZ /d $using:adminpass
            $ok = $ok -band $?
            $null = reg add 'HKLM\tmp\Microsoft\Windows NT\CurrentVersion\WinLogon' /f /v AutoAdminLogon /t REG_DWORD /d 1
            $ok = $ok -band $?
            $null = reg add 'HKLM\tmp\Microsoft\Windows NT\CurrentVersion\WinLogon' /f /v Shell /t REG_SZ /d 'powershell.exe -noexit -command C:\users\administrator\launch.ps1'
            $ok = $ok -band $?

            $null = reg unload 'HKLM\tmp'
            $ok = $ok -band $?
            if (-not $ok) {
                Write-Error "failed autologon injection for $vhdpath"
                return $ok
            }

            # scripts

            Copy-Item -Force C:\ClusterStorage\collect\control\control.ps1 z:\run\control.ps1
            $ok = $ok -band $?
            if (-not $ok) {
                Write-Error "failed injection of specd master.ps1 for $vhdpath"
                return $ok
            }

            #
            # Wrapper for the controller script to auto-restart on failure/update.
            # This is the autologin script itself.
            #
            function Set-FleetLauncherScript
            {
                $script = 'C:\run\control.ps1'

                while ($true) {
                    Write-Host -fore Green Launching $script `@ $(Get-Date)
                    try { & $script -connectuser __CONNECTUSER__ -connectpass __CONNECTPASS__ } catch {}
                    Start-Sleep -Seconds 1
                }
            }

            Remove-Item -Force z:\users\administrator\launch.ps1 -ErrorAction SilentlyContinue
            (Get-Command Set-FleetLauncherScript).ScriptBlock |% {
                $_ -replace '__CONNECTUSER__',$using:connectuser -replace '__CONNECTPASS__',$using:connectpass
            } > z:\users\administrator\launch.ps1
            $ok = $ok -band $?
            if (-not $ok) {
                Write-Error "failed injection of launch.ps1 for $vhdpath"
                return $ok
            }

            Write-Output $vmspec > z:\vmspec.txt
            $ok = $ok -band $?
            if (-not $ok) {
                Write-Error "failed injection of vmspec for $vhdpath"
                return $ok
            }

            # load files
            $f = 'z:\run\testfile1.dat'
            if (-not (Get-Item $f -ErrorAction SilentlyContinue)) {
                fsutil file createnew $f (10GB)
                $ok = $ok -band $?
                fsutil file setvaliddata $f (10GB)
                $ok = $ok -band $?
            }
            if (-not $ok) {
                Write-Error "failed creation of initial load file for $vhdpath"
                return $ok
            }

            return $ok
        }

        function SpecializeVhd( $vhdpath )
        {
            $vhd = (Get-Item $vhdpath)
            $vmspec = $vhd.BaseName

            # mount vhd and its largest partition
            $o = Mount-VHD $vhd -NoDriveLetter -Passthru
            if ($null -eq $o) {
                Write-Error "failed mount for $vhdpath"
                return $false
            }
            $p = Get-Disk -number $o.DiskNumber | Get-Partition | Sort-Object -Property size -Descending | Select-Object -first 1
            $p | Add-PartitionAccessPath -AccessPath Z:

            $ok = ApplySpecialization Z: $vmspec

            Remove-PartitionAccessPath -AccessPath Z: -InputObject $p
            Dismount-VHD -DiskNumber $o.DiskNumber

            return $ok
        }

        foreach ($csv in $csvs) {

            if ($($using:groups).Length -eq 0) {
                $groups = @( 'base' )
            } else {
                $groups = $using:groups
            }

            # identify the CSvs for which this node should create its VMs
            # the trailing characters (if any) are the group prefix
            if ($csv.VDName -notmatch "^$env:COMPUTERNAME(?:-.+){0,1}") {
                continue
            }

            foreach ($group in $groups) {

                if ($csv.VDName -match "^$env:COMPUTERNAME-([^-]+)$") {
                    $g = $group+$matches[1]
                } else {
                    $g = $group
                }

                foreach ($vm in 1..$using:vms) {

                    $stop = $false

                    $newvm = $false
                    $name = ("vm-$g-$env:COMPUTERNAME-{0:000}" -f $vm)

                    $placePath = $csv.FriendlyVolumeName
                    $vmPath = Join-Path $placePath $name
                    $vhdPath = Join-Path $vmPath "$name.vhdx"

                    # if the vm cluster group exists, we are already deployed
                    if (-not (Get-ClusterGroup -Name $name -ErrorAction SilentlyContinue)) {

                        if (-not $stop) {
                            $stop = Stop-After "AssertComplete"
                        }

                        if ($stop) {

                            LogOutput -ForegroundColor Red "vm $name not deployed"

                        } else {

                            LogOutput "CREATE vm $name @ path $vmPath"

                            # always specialize new vms
                            $newvm = $true

                            # We're on the canonical node to create the vm; if the vm exists, tear it down and refresh
                            $o = Get-VM -Name $name -ErrorAction SilentlyContinue

                            if ($null -ne $o) {

                                # interrupted between vm creation and role creation; redo it
                                LogOutput "REMOVE vm $name for re-creation"

                                if ($o.State -ne 'Off') {
                                    Stop-VM -Name $name -TurnOff -Force -Confirm:$false
                                }
                                Remove-VM -Name $name -Force -Confirm:$false
                            }

                            # scrub and re-create the vm metadata path and vhd
                            if (Test-Path $vmPath)
                            {
                                Remove-Item -Recurse $vmPath
                            }

                            $null = New-Item -ItemType Directory $vmPath
                            Copy-Item $using:BaseVHD $vhdPath

                            #### STOPAFTER
                            if (-not $stop) {
                                $stop = Stop-After "CopyVHD" $using:stopafter
                            }

                            if (-not $stop) {

                                # Create A1 VM. use set-vmfleet to alter fleet sizing post-creation.
                                # Do not monitor the internal switch connection; this allows live migration
                                # despite the internal switch.
                                $o = New-VM -VHDPath $vhdPath -Generation 2 -SwitchName $using:vmSwitchName -Path $placePath -Name $name
                                if ($null -ne $o)
                                {
                                    $o | Set-VM -ProcessorCount 1 -MemoryStartupBytes 1.75GB -StaticMemory
                                    $o | Get-VMNetworkAdapter | Set-VMNetworkAdapter -NotMonitoredInCluster $true
                                }
                            }

                            #### STOPAFTER
                            if (-not $stop) {
                                $stop = Stop-After "CreateVM" $using:stopafter
                            }

                            if (-not $stop) {

                                # Create clustered vm role (don't emit) and assign default owner node.
                                # Swallow the (expected) warning that will be referring to the internal
                                # vmswitch as being a local-only resource. Since we replicate the vswitch
                                # with the same IP on each cluster node, it does work.
                                #
                                # If an error occurs, it will emit per normal.
                                $null = $o | Add-ClusterVirtualMachineRole -WarningAction SilentlyContinue
                                Set-ClusterOwnerNode -Group $o.VMName -Owners $env:COMPUTERNAME
                            }
                        }

                    } else {
                        LogOutput -ForegroundColor Green "vm $name already deployed"
                    }

                    #### STOPAFTER
                    if (-not $stop) {
                        $stop = Stop-After "CreateVMGroup" $using:stopafter
                    }

                    if (-not $stop -or ($using:specialize -eq 'Force')) {
                        # specialize as needed
                        # auto only specializes new vms; force always; none skips it
                        if (($using:specialize -eq 'Auto' -and $newvm) -or ($using:specialize -eq 'Force')) {
                            LogOutput "SPECIALIZE vm $name"
                            if (-not (SpecializeVhd $vhdPath)) {
                                LogOutput -ForegroundColor red "Failed specialize of vm $name @ $vhdPath, halting."
                            }
                        } else {
                            LogOutput -ForegroundColor green skip specialize $vhdPath
                        }
                    }
                }
            }
        }
    }
}

function Remove-Fleet
{
    [CmdletBinding()]
    param(
        [Parameter()]
        [string]
        $Cluster = ".",

        [Parameter()]
        [string[]]
        $VMs
        )

    # Parameter set specifying cluster only
    $clusterParam = @{}
    CopyKeyIf $PSBoundParameters $clusterParam 'Cluster'

    if ($vms)
    {
        LogOutput -IsVb "Removing VM Fleet content for: $vms"
    }
    else
    {
        LogOutput -IsVb "Removing VM Fleet"
    }

    # stop and remove clustered vm roles
    Get-ClusterGroup @clusterParam |? GroupType -eq VirtualMachine |? {

        if ($vms)
        {
            $_.Name -in $vms
        }
        else
        {
            $_.Name -like 'vm-*'
        }
    } |% {

        LogOutput -IsVb "Removing ClusterGroup for $($_.Name)"
        $null = $_ | Stop-ClusterGroup
        $_ | Remove-ClusterGroup -RemoveResources -Force
    }

    # Capture flag file path for the installation
    $flagPath = Get-FleetPath -PathType Flag @clusterParam

    # remove all vms
    Invoke-CommonCommand (Get-ClusterNode @clusterParam) -InitBlock $CommonFunc -ScriptBlock {

        Get-VM |? {
            if ($using:vms)
            {
                $_.Name -in $using:vms
            }
            else
            {
                $_.Name -like 'vm-*'
            }
        } |% {

            LogOutput -IsVb "Removing VM for $($_.Name) @ $($env:COMPUTERNAME)"
            $_ | Remove-VM -Confirm:$false -Force

            # remove hosting directory and any flag files associated with this VM
            Remove-Item $_.Path -Recurse -Force
            $f = Join-Path $using:flagPath "*-$($_.Name)"
            if (Test-Path $f)
            {
                Remove-Item $f
            }
        }

        # do not remove the internal switch if teardown is partial
        if ($null -eq $using:vms)
        {
            $switch = Get-VMSwitch -SwitchType Internal
            if ($null -ne $switch)
            {
                LogOutput -IsVb "Removing Internal VMSwitch @ $($env:COMPUTERNAME)"
                $switch | Remove-VMSwitch -Confirm:$False -Force
            }
        }
    }

    # Fallback to clear remnant content due to earlier errors/etc.

    # Now delete content from csvs corresponding to the cluster nodes
    $csv = GetMappedCSV @clusterParam

    Get-ClusterNode |% {
        $csv |? VDName -match "$($_.Name)(-.+)?"
    } |% {

        Get-ChildItem -Directory $_.sharedvolumeinfo.friendlyvolumename |? {
            if ($vms)
            {
                $_.Name -in $vms
            }
            else
            {
                $_.Name -like 'vm-*'
            }
        } |% {

            LogOutput -IsVb "Removing CSV content for $($_.BaseName) @ $($_.FullName)"

            # remove hosting directory and any flag files associated with this VM
            Remove-Item $_.FullName -Recurse -Force
            $f = Join-Path $flagPath "*-$($_.Name)"
            if (Test-Path $f)
            {
                Remove-Item $f
            }
        }
    }
}

function Start-Fleet
{
    [CmdletBinding(DefaultParameterSetName = "ByPercent")]
    param(
        [Parameter(ParameterSetName = "ByNumber")]
        [Parameter(ParameterSetName = "ByPercent")]
        [string]
        $Cluster = ".",

        [Parameter(ParameterSetName = "ByNumber")]
        [Parameter(ParameterSetName = "ByPercent")]
        [Parameter()]
        [string[]]
        $Group = @("*"),

        [Parameter(ParameterSetName = "ByNumber", Mandatory = $true)]
        [ValidateRange(1, [uint32]::MaxValue)]
        [int]
        $Number,

        [Parameter(ParameterSetName = "ByPercent")]
        [ValidateRange(1, 100)]
        [double]
        $Percent = 100
        )

    # Parameter set specifying cluster only
    $clusterParam = @{}
    CopyKeyIf $PSBoundParameters $clusterParam 'Cluster'

    $node = @(Get-ClusterNode @clusterParam)
    $numNode = $node.Count

    # Pause and clear fleet run state so that VMs arrive as a coordinated group
    Set-FleetPause @clusterParam -Timeout 0
    Clear-FleetRunState @clusterParam

    Invoke-CommonCommand ($node |? State -eq Up) -InitBlock $CommonFunc -ScriptBlock {

        $n = $using:Number
        $pct = $using:Percent

        $using:Group |% {

            # Get group vms - assume symmetry per node
            $vms = @(Get-ClusterGroup |? GroupType -eq VirtualMachine |? Name -like "vm-$_-*")

            #
            # Start number/percentage is keyed by VM number label, not dependent on current
            # position of VMs within the cluster. For instance, starting 5 VMs/node will start
            # vm-etc-001, 002, 003, 004, and 005 wherever they are.
            #
            # This allows composition with Move-Fleet and partial redistribution of VMs.
            #

            # Start subset up to VM #n?
            $(if (0 -ne $n)
            {
                $vms |? OwnerNode -eq $env:COMPUTERNAME |? { $n -ge [int] ($_.Name -split '-')[-1] }

                LogOutput -IsVb "Starting up to VM $n"
            }
            # Start subset up to VM #n as defined by %age?
            elseif ($pct -ne 100)
            {
                $n = [int] ($vms.Count * $pct / 100 / $using:numNode)
                if ($n -eq 0) { $n = 1 }

                $vms |? OwnerNode -eq $env:COMPUTERNAME |? { $n -ge [int] ($_.Name -split '-')[-1] }

                LogOutput -IsVb ("Starting {0:P1} ({1}) VMs / node" -f (
                    ($pct/100),
                    $n))
            }
            # Start all
            else
            {
                $vms |? OwnerNode -eq $env:COMPUTERNAME
            }) |? {

                # failed is an unclean offline tbd root causes (can usually be recovered with a restart)
                $_.State -eq 'Offline' -or $_.State -eq 'Failed'

            } | Start-ClusterGroup |% { LogOutput "Start $($_.Name)" }
        }
    }
}

function Stop-Fleet
{
    [CmdletBinding()]
    param(
        [Parameter()]
        [string]
        $Cluster = ".",

        [Parameter()]
        [string[]]
        $Group = @('*')
        )

    # Parameter set specifying cluster only
    $clusterParam = @{}
    CopyKeyIf $PSBoundParameters $clusterParam 'Cluster'

    $Node = Get-ClusterNode @clusterParam |? State -eq Up

    # Pause and clear fleet run state so that VMs depart as a coordinated group
    Set-FleetPause @clusterParam
    Clear-FleetRunState @clusterParam

    Invoke-CommonCommand $Node -InitBlock $CommonFunc -ScriptBlock {

        foreach ($g in $using:Group)
        {
            #
            # Get all non-offline VMs to work on. Note that the State property is dynamically evaluated on reference.
            # This list only needs to be acquired once. Note that we do shutdown by resource (the VM) while start
            # must go by group to ensure the complete group (including the VM configuration resource) is brought up
            #

            $vms = Get-ClusterResource |? OwnerNode -eq $env:COMPUTERNAME |? ResourceType -eq 'Virtual Machine' |? OwnerGroup -like "vm-$g-*" |? State -ne 'Offline'

            try
            {
                #
                # Put all VMs in OfflineAction = Shutdown
                # https://docs.microsoft.com/en-us/previous-versions/windows/desktop/mscs/virtual-machines-offlineaction
                #

                $vms | Set-ClusterParameter -Name OfflineAction -Value 2
                $vms | Stop-ClusterResource |% { LogOutput "Shutdown $($_.OwnerGroup)" }

                #
                # Use TurnOff on remaining nonresponsive VMs
                #

                $vms |? State -ne 'Offline' | Set-ClusterParameter -Name OfflineAction -Value 0
                $vms |? State -ne 'Offline' | Stop-ClusterResource |% { LogOutput "TurnOff non-responsive $($_.OwnerGroup)" }
            }
            finally
            {
                #
                # Restore all VMs to OfflineAction - Save (default)
                #

                $vms | Set-ClusterParameter -Name OfflineAction -Value 1
            }
        }
    }
}

function Repair-Fleet
{
    [CmdletBinding()]
    param (
        [Parameter()]
        [string]
        $Cluster = '.',

        [Parameter()]
        [switch]
        $BringUp
        )

    # Parameter set specifying cluster only
    $clusterParam = @{}
    CopyKeyIf $PSBoundParameters $clusterParam 'Cluster'

    $vms = Get-FleetVM @clusterParam -ControlResponse

    #
    # In BringUp mode try to start any group which is Offline
    #

    $startVM = @()
    if ($BringUp)
    {
        $startVM = @($vms |? ClusterState -eq 'Offline')

        if ($startVM.Count)
        {
            $startGroups = foreach ($vm in $startVM)
            {
                Get-ClusterGroup @clusterParam -Name $vm.Name
            }

            $startGroups | Start-ClusterGroup |% { LogOutput "Start $($_.Name)" }
        }

        $expectedOffline = 0
    }

    #
    # Count offline VMs so that we can check that our actions don't create more.
    #
    else
    {
        $expectedOffline = @($vms |? ClusterState -eq 'Offline').Count
    }

    #
    # Now restart all VMs which are not Offline. This rolls up the VM, heartbeat and control response issues.
    # Any offline VMs we decided to start for Bringup are continuing to arrive as this is done.
    #

    $repairVM = @($vms |? State -ne Offline |? State -ne Ok)

    #
    # If there are no VMs to restart and none that we already started (=> need control checks), done.
    #

    if ($repairVM.Count -eq 0 -and $startVM.Count -eq 0)
    {
        return
    }

    if ($repairVM.Count)
    {
        #
        # Get all VM resources  to work on. Note that the State property is dynamically evaluated on reference.
        # This list only needs to be acquired once. Note that we do shutdown by resource (the VM) while start
        # must go by group to ensure the complete group (including the VM configuration resource) is brought up
        #

        $vms = Get-ClusterResource @clusterParam |? ResourceType -eq 'Virtual Machine' |? OwnerGroup -in $repairVM.Name

        try
        {
            #
            # Put all VMs in OfflineAction = Shutdown
            # https://docs.microsoft.com/en-us/previous-versions/windows/desktop/mscs/virtual-machines-offlineaction
            #

            $vms | Set-ClusterParameter -Name OfflineAction -Value 2
            $vms | Stop-ClusterResource |% { LogOutput "Repair Shutdown $($_.OwnerGroup)" }

            #
            # Use TurnOff on remaining nonresponsive VMs
            #

            $vms |? State -ne 'Offline' | Set-ClusterParameter -Name OfflineAction -Value 0
            $vms |? State -ne 'Offline' | Stop-ClusterResource |% { LogOutput "Repair TurnOff non-responsive $($_.OwnerGroup)" }
        }
        finally
        {
            #
            # Restore all VMs to OfflineAction - Save (default)
            #

            $vms | Set-ClusterParameter -Name OfflineAction -Value 1
        }

        #
        # Now get the cluster groups to turn on.
        #

        $vms = Get-ClusterGroup @clusterParam |? GroupType -eq 'VirtualMachine' |? Name -in $repairVM.Name
        $vms | Start-ClusterGroup |% { LogOutput "Repair Start $($_.Name)" }
    }

    #
    # Wait for VM hearbeats (NoContact) to arrive. The health check assumes any non-OK lower level state
    # is indeed fatal and control response is not needed; we know some VMs are restarting and may not be
    # reporting to HyperV yet. Wait for Hyperv.
    #

    $await = 5
    do {

        Start-Sleep 10
        $vms = Get-FleetVM

        if (@($vms |? State -eq NoContact).Count -eq 0)
        {
            break
        }

    } while (--$await)

    #
    # Now get the full health including control response for final validation
    #

    $await = 5

    do {

        $vms = Get-FleetVM -ControlResponse

        #
        # If we've maintained the expected number of offline VMs and the balance are OK, repair has been
        # successful. Otherwise throw the assertion.
        #

        $nowOffline = @($vms |? ClusterState -eq 'Offline').Count
        $nowOk = @($vms |? State -eq 'Ok').Count

        if ($nowOffline -eq $expectedOffline -and
            $nowOk -eq ($vms.Count - $expectedOffline))
        {
            return
        }

        LogOutput "Fleet repair re-probe, pausing 5s"
        Start-Sleep 5

    } while (--$await)

    throw "Fleet repair failed: $($vms.Count) total VMs, $nowOffline offline (expected $expectedOffline), $nowOk Ok (expected $($vms.Count - $expectedOffline))"
}

function Get-FleetVM
{
    [CmdletBinding()]
    param (
        [Parameter()]
        [string]
        $Cluster = ".",

        [Parameter()]
        [string]
        $Group = '*',

        [Parameter()]
        [switch]
        $ControlResponse,

        [Parameter()]
        [uint32]
        $Timeout = 60
        )

    # Parameter set specifying cluster only
    $clusterParam = @{}
    CopyKeyIf $PSBoundParameters $clusterParam 'Cluster'

    #
    # Start timeout; only takes effect after first control response check.
    #

    $t0 = Get-Date

    $clusterView = @{}
    $vmView = @{}
    Get-ClusterGroup @clusterParam |? GroupType -eq VirtualMachine           |? Name -like "vm-$Group-*" |% { $clusterView[$_.Name] = $_ }
    Get-VM -CimSession @(Get-ClusterNode @clusterParam |? State -eq Up).Name |? Name -like "vm-$Group-*" |% { $vmView[$_.Name] = $_ }

    # Union into a composite vm health/state object
    # Pass 1 through cluster groups. Pass 2 through VMs to detect those
    # not attached as cluster groups (not expected).
    #
    # Accumulate in hash for return.

    $h = @{}
    $ok = 0

    foreach ($cvm in $clusterView.Values)
    {
        if ($vmView.Contains($cvm.Name)) {
            $vmState = $vmView[$cvm.Name].State
        }
        else
        {
            $vmState = 'Unknown'
        }

        if ($vmView.Contains($cvm.Name)) {
            $vmHeartbeat = $vmView[$cvm.Name].Heartbeat
            $vmId = $vmView[$cvm.Name].VMId.Guid
        }
        else
        {
            $vmHeartbeat = 'Unknown'
            $vmId = $null
        }

        # Aggregate state is the union of cluster, vm and heartbeat in that priority order
        if ($cvm.State -ne 'Online')
        {
            $state = $cvm.State
        }
        elseif ($vmState -ne 'Running')
        {
            $state = $vmState
        }
        elseif ($vmHeartbeat -like 'Ok*')
        {
            $state = 'Ok'
            ++$ok
        }
        else
        {
            $state = $vmHeartbeat
        }

        $h[$cvm.Name] = [pscustomobject] @{
            Name = $cvm.Name
            VMId = $vmid
            OwnerNode = $cvm.OwnerNode
            State = $state
            ClusterState = $cvm.State
            VMState = $vmState
            Heartbeat = $vmHeartbeat
            ControlResponse = 'Unknown'
            };
    }

    foreach ($vvm in $vmView.Values)
    {
        # Already visited by cluster view?
        if ($clusterView.Contains($vvm.Name)) { continue }

        $h[$vvm.Name] = [pscustomobject] @{
            Name = $vvm.Name
            VMId = $vvm.VMId.Guid
            OwnerNode = $vvm.ComputerName
            State = "NotClustered"
            ClusterState = "NotClustered"
            VMState = $vvm.State
            Heartbeat = $vvm.Heartbeat
            ControlResponse = 'Unknown'
            };
    }

    #
    # If there are no Ok VMs at the higher layer health checks (group, vm, heartbeat),
    # no control response work to perform - we alredy know they are all failed. If there
    # are OK VMs and we are asked to, then perform the control respone check. The basic
    # reason this is opt in is that it *is* currently disruptive to running operations.
    # It is conceivable that we should have a way for the control loop to chirp out without
    # relying on the pause indication, but deferred for now.
    #

    if ($ok -ne 0 -and $ControlResponse)
    {
        #
        # Fill filter hash for all Ok VMs; these are the ones relevant to the
        # pause flag live check.
        #

        $checkPause = @{}
        foreach ($vm in $h.Values)
        {
            if ($vm.State -eq 'Ok')
            {
                $checkPause["pause-$($vm.Name)"] = $true
            }
        }

        $flagPath = Get-FleetPath @clusterParam -PathType Flag

        #
        # Force pause response from all VM and wait (under timeout) for the responses.
        # Note the residual possibility that a given VM has never paused and will now
        # create its pause for the first time - must enumerate on each check.
        #

        Set-FleetPause @clusterParam -Force -Timeout 0
        $pauseEpoch = Get-FleetPauseEpoch @clusterParam

        do {

            Start-Sleep -Seconds 1

            # File -> Value map of responses
            $pauseValues = @{}
            Get-ChildItem $flagPath\pause-* |? { $checkPause[$_.BaseName] } |% { $pauseValues[$_.BaseName] = Get-Content $_ }

            # If we have responses from all VMs ...
            if ($pauseValues.Count -eq $checkPause.Count)
            {
                # Now group responses - if there is one ...
                $pauseGroup = @($pauseValues.Values | Group-Object -AsHashTable)

                if ($pauseGroup.Count -eq 1)
                {
                    # Crack value enumerator to comparable value.
                    $pauseValue = $pauseGroup.Values |% { $_ }

                    # If it is the same as the current epoch, we are done.
                    if ($pauseValue -eq $pauseEpoch)
                    {
                        break
                    }
                }
            }

        } while (((Get-Date) - $t0).TotalSeconds -lt $Timeout)

        #
        # Now roll the pause values (Ok or leave unmodifed/Unknown) onto the VM control response.
        #

        foreach ($k in $pauseValues.Keys)
        {
            if ($pauseValues[$k] -eq $pauseEpoch)
            {
                $r = 'Ok'
            }
            else
            {
                $r = 'NoControlResponse'
            }

            $h[$k -replace 'pause-',''].ControlResponse = $r
        }

        #
        # ControlResponse overrides Ok on final State response.
        # VMs which are already non-Ok continue to carry that indication.
        #

        foreach ($k in $h.Keys)
        {
            if ($h[$k].ControlResponse -ne 'Ok' -and $h[$k].State -eq 'Ok')
            {
                $h[$k].State = 'NoControlResponse'
            }
        }
    }

    $h.Values
}

function Show-Fleet
{
    [CmdletBinding()]
    param (
        [Parameter()]
        [string]
        $Cluster = ".",

        [Parameter()]
        [string]
        $Group = '*'
        )

    # Parameter set specifying cluster only
    $clusterParam = @{}
    CopyKeyIf $PSBoundParameters $clusterParam 'Cluster'

    $vm = Get-FleetVM @PSBoundParameters

    ###########################################################
    write-host -fore green By State
    $vm | Group-Object -Property State,ClusterState,VMState,HeartBeat | Sort-Object -Property Name |`
        Format-Table -AutoSize -Property @{ Label = 'Count'; Expression = { $_.Count }},
                                         @{ Label = 'State'; Expression = { $_.Group[0].State }},
                                         @{ Label = 'ClusterState'; Expression = { $_.Group[0].ClusterState }},
                                         @{ Label = 'VMState'; Expression = { $_.Group[0].VMState }},
                                         @{ Label = 'Heartbeat'; Expression = { $_.Group[0].Heartbeat }}

    ###########################################################
    write-host -fore green By Host
    $vm | Group-Object -Property OwnerNode,State,ClusterState,VMState,HeartBeat | Sort-Object -Property Name |`
        Format-Table -AutoSize -Property @{ Label = 'Count'; Expression = { $_.Count }},
                                         @{ Label = 'OwnerNode'; Expression = { $_.Group[0].OwnerNode }},
                                         @{ Label = 'State'; Expression = { $_.Group[0].State }},
                                         @{ Label = 'ClusterState'; Expression = { $_.Group[0].ClusterState }},
                                         @{ Label = 'VMState'; Expression = { $_.Group[0].VMState }},
                                         @{ Label = 'Heartbeat'; Expression = { $_.Group[0].Heartbeat }}

    ###########################################################
    write-host -fore green By Group
    $vm |% {
        if ($_.Name -match "^vm-([^-]+)-") {
            $_ | Add-Member -NotePropertyName Group -NotePropertyValue $matches[1] -PassThru
        }
    } | Group-Object -Property Group,State,ClusterState,VMState,HeartBeat| Sort-Object -Property Name | `
        Format-Table -AutoSize -Property @{ Label = 'Count'; Expression = { $_.Count }},
                                         @{ Label = 'Group'; Expression = { $_.Group[0].Group }},
                                         @{ Label = 'State'; Expression = { $_.Group[0].State }},
                                         @{ Label = 'ClusterState'; Expression = { $_.Group[0].ClusterState }},
                                         @{ Label = 'VMState'; Expression = { $_.Group[0].VMState }},
                                         @{ Label = 'Heartbeat'; Expression = { $_.Group[0].Heartbeat }}

    ###########################################################
    write-host -fore green By IOPS
    # build 5 orders of log steps
    $logstep = 10,20,50
    $log = 1
    $logs = 1..5 |% {

        $logstep |% { $_ * $log }
        $log *= 10
    }

    # build log step names; 0 is the > range catchall
    $lognames = @{}
    foreach ($step in $logs) {

        if ($step -eq $logs[0]) {
            $lognames[$step] = "< $step"
        } else {
            $lognames[$step] = "$pstep - $($step - 1)"
        }
        $pstep = $step
    }
    $lognames[0] = "> $($logs[-1])"

    # now bucket up VMs by flow rates
    $qosbuckets = @{}
    $qosbuckets[0] = 0
    $logs |% {
        $qosbuckets[$_] = 0
    }

    $node = Get-AccessNode @clusterParam

    # Note: group all vm disks together and bucket sum of IOPS
    Get-StorageQoSFlow -CimSession $node | Group-Object -Property InitiatorName |% {

        $initiatorIops = ($_.Group | Measure-Object -Sum InitiatorIops).Sum
        $found = $false
        foreach ($step in $logs) {

            if ($initiatorIops -lt $step) {
                $qosbuckets[$step] += 1;
                $found = $true
                break
            }
        }
        # if not bucketed, it is greater than range
        if (-not $found) {
            $qosbuckets[0] += 1
        }
    }

    # find min/max buckets with nonzero counts, by $logs index
    # this lets us present a continuous range, with interleaved zeroes
    $bmax = -1
    $bmin = -1
    foreach ($i in 0..($logs.Count - 1)) {
        if ($qosbuckets[$logs[$i]]) {

            if ($bmin -lt 0) {
                $bmin = $i
            }
            $bmax = $i
        }
    }
    # raise max if we have > range
    if ($qosbuckets[0]) {
        $bmax = $logs.Count - 1
    }
    $range = @($logs[$bmin..$bmax])
    # add > range if needed, at end
    if ($qosbuckets[0]) {
        $range += 0
    }

    $(foreach ($i in $range) {

        New-Object -TypeName psobject -Property @{
            Count = $qosbuckets[$i];
            IOPS = $lognames[$i]
        }
    }) | Format-Table Count,IOPS -AutoSize
}

function Move-Fleet
{
    [CmdletBinding(DefaultParameterSetName = "Default")]
    param(
        [Parameter(ParameterSetName = 'Default')]
        [Parameter(ParameterSetName = 'ByPercent')]
        [string]
        $Cluster = ".",

        [Parameter(ParameterSetName = 'Default')]
        [Parameter(ParameterSetName = 'ByPercent')]
        [switch]
        $KeepCSV,

        [Parameter(ParameterSetName = 'Default')]
        [Parameter(ParameterSetName = 'ByPercent')]
        [switch]
        $KeepVM,

        [Parameter(ParameterSetName = 'Default')]
        [Parameter(ParameterSetName = 'ByPercent')]
        [switch]
        $ShiftCSV,

        [Parameter(ParameterSetName = 'Default')]
        [Parameter(ParameterSetName = 'ByPercent')]
        [ValidateRange(0,100)]
        [uint32]
        $DistributeVMPercent = 0,

        [Parameter(ParameterSetName = 'ByPercent', Mandatory = $true)]
        [ValidateRange(1,100)]
        [uint32]
        $VMPercent = 100,

        [Parameter(ParameterSetName = 'Default')]
        [switch]
        $Running
    )

    # Parameter set specifying cluster only
    $clusterParam = @{}
    CopyKeyIf $PSBoundParameters $clusterParam 'Cluster'

    # Note that the collect volume is never affected by fleet CSV movements
    $nodes = @(Get-ClusterNode @clusterParam | Sort-Object -Property Name)
    $nodesLive = @($nodes |? State -eq Up)

    if ($nodes.Count -le 1 -or $nodesLive.Count -le 1)
    {
        Write-Error "Cluster only contains a single node ($($nodes.Count)) or less than two live nodes ($($nodesLive.Count) - movement not possible"
        return
    }

    $csv = GetMappedCSV @clusterParam |? VDName -ne $collectVolumeName

    if ($ShiftCSV)
    {
        # Shift rotates all csv's node ownership by one node, in lexical order of
        # current node owner name. This is useful for forcing all-out-of-position ops
        # quickly without VM movement.
        LogOutput "Shifting CSV owners"

        # CSV object's OwnerNode is dynamically computed. Build a hash of new OwnerNodes
        # for the CSV in pass 1 and move in pass 2.
        $moveh = @{}
        foreach ($n in 0..($nodes.Length - 1))
        {
            $moveTo = ($n + 1) % $nodes.Length
            $moveh[$nodes[$moveTo].Name] = $csv |? { $_.OwnerNode.Name -eq $nodes[$n].Name }
        }

        foreach ($n in $moveh.Keys)
        {
            $moveh[$n] | Move-ClusterSharedVolume $n @clusterParam
        }
    }

    # Re-home all CSV unless declined
    elseif (-not $KeepCSV)
    {
        LogOutput "Aligning CSVs"

        # move all csvs named by node names back to their named node
        $nodes |% {
            $node = $_
            $csv |? VDName -match "$($node.Name)(?:-.+){0,1}" |? OwnerNode -ne $node.Name |% { $_ | Move-ClusterSharedVolume $node.Name @clusterParam }
        }
    }

    if (-not $KeepVM)
    {
        if ($DistributeVMPercent -gt 0)
        {
            LogOutput "Distributing VMs ($DistributeVMPercent%)"
        }
        else
        {
            LogOutput "Aligning VMs"
        }

        #
        # Build aligned ownership lists in order of node list, in lexical name
        # order to take advantage of the stable distribution shift. Shifting by 60%
        # will build on the moves for 50%, and so forth.
        #

        $vms = Get-ClusterGroup @clusterParam |? GroupType -eq VirtualMachine

        #
        # Only rotate online VMs? This allows one model of composing partial startup with rotation - start
        # given number in-place and apply rotation to those.
        #

        if ($Running)
        {
            $vms = $vms |? State -eq Online

            if ($vms.Count -eq 0)
            {
                Write-Error "No VMs currently running, no rotation performed"
            }
        }

        $groups = [collections.arraylist]@()
        $count = 0
        foreach ($node in $nodes)
        {
            $arr = [collections.arraylist]@()
            foreach ($vm in $vms | Sort-Object -Property Name)
            {
                if ($vm.Name -match "vm-.*-$($node.Name)-\d+$")
                {
                    $null = $arr.Add($vm)
                }
            }
            if ($count -eq 0)
            {
                $count = $arr.Count
            }
            elseif ($count -ne $arr.Count)
            {
                Write-Error "Node $node only has $($arr.Count) aligned VMs created for it. Please check that all nodes had the same number of VMs created (New-Fleet -VMs)."
            }
            $null = $groups.Add($arr)
        }

        if($VMPercent -lt 100)
        {
            #
            # How many VMs per node to consider in the rotation (partial start cases, without -Running).
            # Discard upper portion of VMs/node. Note this is/must be consistent with start-fleet,
            # get-fleetdatadiskestimate and other vm-percentage calculations.
            #

            $n = [int] ($VMPercent * $groups[0].Count /100 )
            if ($n -eq 0) { $n = 1 }

            # No trim if rounding took it back to 100%.

            if ($n -ne $groups[0].Count)
            {
                foreach ($g in $groups)
                {
                    $g.RemoveRange($n, $g.Count - $n)
                }
            }
        }

        if ($DistributeVMPercent -gt 0)
        {
            #
            # How many VMs to distribute (at least 1 VM/node)
            #

            $n = [int] ($DistributeVMPercent * $groups[0].Count / 100)
            if ($n -eq 0) { $n = 1 }

            $groups = GetDistributedShift -Group $groups -N $n
        }

        try
        {
            #
            # Strip out VMs already in the correct location and farm out movement
            # to the respective host node.
            #

            $j = @()
            for ($i = 0; $i -lt $groups.Count; ++$i)
            {
                $moveList = @(foreach ($vm in $groups[$i])
                {
                    if ($vm.OwnerNode -ne $nodes[$i])
                    {
                        $vm
                    }
                })

                if ($moveList.Count -eq 0)
                {
                    continue
                }

                $target = $nodes[$i].Name
                LogOutput Moving $moveList.Count VMs to $target

                #
                # Carefully quote the movelist so that it passes as a single parameter (list of a single element, a list)
                #

                $j += Invoke-CommonCommand -AsJob -InitBlock $CommonFunc -ArgumentList @(,$moveList) -ScriptBlock {

                    [CmdletBinding()]
                    param(
                        [object[]]
                        $vms
                    )

                    foreach ($vm in $vms)
                    {
                        LogOutput "Moving $($vm.Name): $($vm.OwnerNode) -> $using:target"

                        $expectedState = $vm.State

                        try
                        {
                            # The default move type is live, but live does not degenerately handle offline vms yet.
                            # Discard the warning stream - if an issue occurs the cluster will offer a generic comment
                            # about credential remoting (in addition to the error) which is not going to be relevant in
                            # our cases.
                            if ($vm.State -eq 'Offline')
                            {
                                $null = Move-ClusterVirtualMachineRole @using:clusterParam -Name $vm.Name -Node $using:target -MigrationType Quick -ErrorAction Stop 3> $null
                            }
                            else
                            {
                                $null = Move-ClusterVirtualMachineRole @using:clusterParam -Name $vm.Name -Node $using:target -ErrorAction Stop 3> $null
                            }
                        }
                        catch
                        {
                            #
                            # Workaround false failure of VM quick/offline migration - sink the error if VM appears to land, otherwise propagate it.
                            #

                            $checkVM = Get-ClusterGroup @using:clusterParam -Name $vm.Name

                            $await = 0
                            if ($expectedState -eq 'Offline')
                            {
                                $await = 5
                                do {

                                    if ($checkVM.State -eq $expectedState -and $checkVM.OwnerNode -eq $using:target)
                                    {
                                        LogOutput "Verified VM quick migration on retry $(5 - $await + 1) $($vm.Name) -> $using:target"
                                        break
                                    }

                                    if (-not --$await)
                                    {
                                        break
                                    }

                                    Start-Sleep 1
                                    $checkVM = Get-ClusterGroup @using:clusterParam -Name $vm.Name

                                } while ($true)
                            }

                            if ($await -eq 0)
                            {
                                if ($checkVM.State -ne $expectedState)      { LogOutput "Verify $($vm.Name) $($checkVM.State) != $expectedState" }
                                if ($checkVM.OwnerNode -ne $using:target)   { LogOutput "Verify $($vm.Name) $($checkVM.OwnerNode) != $($using:target)" }
                                LogOutput -ForegroundColor Red "Error moving $($vm.Name)"

                                $errorObject = CreateErrorRecord -ErrorId $_.FullyQualifiedErrorId `
                                    -ErrorMessage $null `
                                    -ErrorCategory $_.CategoryInfo.Category `
                                    -Exception $_.Exception `
                                    -TargetObject $_.TargetObject

                                $psCmdlet.WriteError($errorObject)
                            }
                        }
                    }
                }
            }

            #
            # And await completion (errors flow through)
            #

            $jobErrors = @()
            while ($true)
            {
                $done = @($j | Wait-Job -Timeout 1)
                try {
                    $j | Receive-Job
                } catch {
                    $jobErrors += $_
                }
                if ($done.Count -eq $j.Count)
                {
                    break
                }
            }
        }
        finally
        {
            $j | RemoveCommonJobSession
            $j | Remove-Job
        }

        if ($jobErrors.Count)
        {
            $jobErrors |% { Write-Error -ErrorRecord $_ }
        }
    }
}

function Set-FleetPowerScheme
{
    param (
        [Parameter()]
        [string]
        $Cluster = ".",

        [Parameter(Mandatory = $true)]
        [ValidateSet("Balanced","HighPerformance","PowerSaver")]
        [string]
        $Scheme
        )

    $node = Get-ClusterNode -Cluster $Cluster |? State -eq Up

    switch ($Scheme)
    {
        "Balanced"          { $guid  = '381b4222-f694-41f0-9685-ff5bb260df2e' }
        "HighPerformance"   { $guid = '8c5e7fda-e8bf-4a96-9a85-a6e23a8c635c' }
        "PowerSaver"        { $guid = 'a1841308-3541-4fab-bc81-f71556f20b4a' }
    }

    Invoke-CommonCommand -ComputerName $node -ScriptBlock {
        powercfg /setactive $using:guid
    }
}

function Get-FleetPowerScheme
{
    param (
        [Parameter()]
        [string]
        $Cluster = "."
        )

    $node = Get-ClusterNode -Cluster $Cluster |? State -eq Up

    $scheme = Invoke-CommonCommand -ComputerName $node -ScriptBlock {
        powercfg /getactivescheme
    }

    # Map/Normalize names off of GUID (i8ln?)
    $normalScheme = foreach ($s in $scheme)
    {
        $null = $s -match 'GUID:\s+(\S+)'

        switch ($matches[1])
        {
            '381b4222-f694-41f0-9685-ff5bb260df2e' { $name = "Balanced" }
            '8c5e7fda-e8bf-4a96-9a85-a6e23a8c635c' { $name = "HighPerformance" }
            'a1841308-3541-4fab-bc81-f71556f20b4a' { $name = "PowerSaver" }

            default {
                Write-Error "Unnown running power scheme guid $($matches[1]) - please provide feedback to VM Fleet and normalize to known (Balanced, High Performance or Power Saver - see powercfg /list)"
                return
            }
        }

        [pscustomobject] @{ Name = $name; Node = $s.PSComputerName }
    }

    $g = @($normalScheme | Group-Object -Property Name )

    if ($g.Count -eq 1)
    {
        return $g[0].Name
    }

    foreach ($s in $normalScheme)
    {
        Write-Warning "Node $($s.Node): $($s.Name)"
    }

    Write-Error "Inconsistent power schemes running in cluster - please verify intended configuration"
}

function Set-FleetQoS
{
    # apply given QoS policy (by name) to all VMs on specified nodes
    [CmdletBinding(DefaultParameterSetName = "ByCluster")]
    param (
        [Parameter(ParameterSetName = "ByCluster")]
        [string]
        $Cluster = ".",

        [Parameter(ParameterSetName = "ByNode", Mandatory = $true)]
        [string[]]
        $Node,

        [Parameter()]
        [string]
        $Name
        )

    if ($PSCmdlet.ParameterSetName -eq 'ByCluster')
    {
        $Node = Get-ClusterNode -Cluster $Cluster |? State -eq Up
    }

    if ($PSBoundParameters.ContainsKey('Name'))
    {
        # QoS policy must exist, else error out
        $qosp = Get-StorageQoSPolicy -Name $Name -CimSession $Node[0]
        if ($null -eq $qosp)
        {
            # cmdlet error sufficient
            return
        }

        $id = $qosp.PolicyId
        LogOutput -IsVb "applying QoS Policy: $Name (PolicyID $id)"
    }
    else
    {
        # clears QoS policy
        $id = $null
        LogOutput -IsVb "removing QoS Policy"
    }

    Invoke-Command $Node {

        Get-VM |% { Get-VMHardDiskDrive $_ |% { Set-VMHardDiskDrive -QoSPolicyID $using:id -VMHardDiskDrive $_ }}
    }
}

#
# Dynamic ValidateSet should be used when available - not present in inbox PS5.1
# This would replace the large ValidateSet with a simple [ValidateSet([VMSpec])]
#

# class VMSpec : System.Management.Automation.IValidateSetValuesGenerator {
class VMSpec {

    [int] $ProcessorCount
    [int64] $MemoryStartupBytes
    [string] $Alias

    VMSpec(
        [int]
        $ProcessorCount,

        [int64]
        $MemoryStartupBytes,

        [string]
        $Alias
    )
    {
        $this.ProcessorCount = $ProcessorCount
        $this.MemoryStartupBytes = $MemoryStartupBytes
        $this.Alias = $Alias
    }

    <#
    [String[]] GetValidValues() {

        return $script:VMSpecs.Keys
    }
    #>
}

$VMSpecs = @{}
$VMSpecs['A1v2'] = [VMSpec]::new(1, 2GB, $null)
$VMSpecs['A2v2'] = [VMSpec]::new(2, 4GB, $null)
$VMSpecs['A4v2'] = [VMSpec]::new(4, 8GB, $null)
$VMSpecs['A8v2'] = [VMSpec]::new(8, 16GB, $null)

$VMSpecs['A2mv2'] = [VMSpec]::new(2, 16GB, $null)
$VMSpecs['A4mv2'] = [VMSpec]::new(4, 32GB, $null)
$VMSpecs['A8mv2'] = [VMSpec]::new(8, 64GB, $null)

# example of an alias when vcpu/mem are the same
# $VMSpecs['D1v2'] = [VMSpec]::new(0, 0, 'A1v2')

#>

function Get-FleetComputeTemplate
{
    [CmdletBinding()]
    param(
        [Parameter(Mandatory = $true)]
        [ValidateSet(
            'A1v2','A2mv2','A2v2','A4mv2','A4v2','A8mv2','A8v2'
            )]
        [string]
        $ComputeTemplate
        )

    $spec = $VMSpecs[$ComputeTemplate]
    if ($spec.ProcessorCount -eq 0) {
        $spec = $VMSpec[$spec.Alias]
    }

    $spec
}

function Set-Fleet
{
    [CmdletBinding(DefaultParameterSetName = "FullSpecStatic")]
    param(
        [Parameter(ParameterSetName = 'DataDisk')]
        [Parameter(ParameterSetName = 'FullSpecDynamic')]
        [Parameter(ParameterSetName = 'FullSpecStatic')]
        [string]
        $Cluster = ".",

        [Parameter(ParameterSetName = 'FullSpecDynamic', Mandatory = $true)]
        [Parameter(ParameterSetName = 'FullSpecStatic', Mandatory = $true)]
        [int]
        $ProcessorCount,

        [Parameter(ParameterSetName = 'FullSpecDynamic', Mandatory = $true)]
        [Parameter(ParameterSetName = 'FullSpecStatic', Mandatory = $true)]
        [int64]
        $MemoryStartupBytes,

        [Parameter(ParameterSetName = 'FullSpecDynamic', Mandatory = $true)]
        [int64]
        $MemoryMaximumBytes,

        [Parameter(ParameterSetName = 'FullSpecDynamic', Mandatory = $true)]
        [int64]
        $MemoryMinimumBytes,

        # Note: ValidateSet MUST be kept in line with definitions.
        # This can become dyanmic if/when we can move forward to PS 6.1/later
        # where the dynamic ValidateSet capability can be used.
        [Parameter(ParameterSetName = 'SizeSpec')]
        [ValidateSet(
            'A1v2','A2mv2','A2v2','A4mv2','A4v2','A8mv2','A8v2'
            )]
        [string]
        $ComputeTemplate = 'A1v2',

        [Parameter(ParameterSetName = 'DataDisk', Mandatory = $true)]
        [uint64]
        $DataDiskSize,

        [Parameter(ParameterSetName = 'DataDisk')]
        [switch]
        $Running
    )

    $nodes = @(Get-ClusterNode -Cluster $Cluster |? State -eq Up)

    # Fill parameters
    $p = @{}

    switch ($PSCmdlet.ParameterSetName) {

        'FullSpecDynamic'
        {
            $p['DynamicMemory'] = $true

            $p['ProcessorCount'] = $ProcessorCount
            $p['MemoryMaximumBytes'] = $MemoryMaximumBytes
            $p['MemoryMinimumBytes'] = $MemoryMinimumBytes
            $p['MemoryStartupBytes'] = $MemoryStartupBytes

            LogOutput -IsVb "DynamicMemory: applying ProcessorCount $ProcessorCount Memory[ Start $('{0:N2}' -f ($MemoryStartupBytes/1GB))GB : $('{0:N2}' -f ($MemoryMinimumBytes/1GB))GB - $('{0:N2}' -f ($MemoryMaximumBytes/1GB))GB ]"
        }

        'FullSpecStatic'
        {
            $p['StaticMemory'] = $true

            $p['ProcessorCount'] = $ProcessorCount
            $p['MemoryStartupBytes'] = $MemoryStartupBytes

            LogOutput -IsVb "StaticMemory: applying ProcessorCount $ProcessorCount Memory[ $('{0:N2}' -f ($MemoryStartupBytes/1GB))GB ]"
        }

        'SizeSpec'
        {
            $spec = Get-FleetComputeTemplate -ComputeTemplate $ComputeTemplate

            $p['ProcessorCount'] = $spec.ProcessorCount
            $p['StaticMemory'] = $true
            $p['MemoryStartupBytes'] = $spec.MemoryStartupBytes

            LogOutput -IsVb "StaticMemory: applying $ComputeTemplate compute template: ProcessorCount $($spec.ProcessorCount) Memory[ $('{0:N2}' -f ($spec.MemoryStartupBytes/1GB))GB ]"
        }
    }

    if ($PSCmdlet.ParameterSetName -ne 'DataDisk')
    {
        Invoke-Command $nodes -ArgumentList $p {

            param($p)

            foreach ($vm in Get-VM)
            {
                # Match off home node name from VM. This identifies the VM directory location.
                if ($vm.Name -notmatch 'vm-[^-]+-(.*)-(\d+)')
                {
                    # not a fleet VM
                    continue
                }

                if ($vm.State -ne 'Off')
                {
                    Write-Error "Cannot set VM properties on VMs which are not offline: $($vm.Name) ($($vm.State))"
                }
                else
                {
                    $vm | Set-VM @p
                    continue
                }
            }
        }
    }
    else
    {
        #
        # This needs to run without remoting. The cluster cmdlets embedded within the HyperV set are not reliably
        # executable in remote sessions due to credential delegation when not using CredSSP. Specifically,
        # Update-ClusterVirtualMachineConfiguration is invoked to update the clustered role when disks are modifieed.
        #

        $vms = @(Get-VM -CimSession $nodes.Name |? Name -like 'vm-*')
        for ($i = 0; $i -lt $vms.Count; ++$i)
        {
            $vm = $vms[$i]

            $pcomplete = [int](100*$i/($vms.Count))
            Write-Progress -Id 0 -Activity "Updating VM Data Disks" -PercentComplete (100*$i/($vms.Count)) -CurrentOperation $vm.Name -Status "$pcomplete% Complete:"

            # Skip non-running VMs?
            if ($Running -and $vm.State -ne 'Running')
            {
                continue
            }
            # Explicitly work on non-running VMs? Skip running VMs.
            elseif ($PSBoundParameters.ContainsKey('Running') -and -not $Running -and $vm.State -eq 'Running')
            {
                continue
            }

            #
            # ControllerNumber/Location 0/0 is the boot/OS VHD - that is not modified.
            # Order the list so that the boot/OS VHD will always be the first, data the second.
            #

            $vhd = @($vm | Get-VMHardDiskDrive | Sort-Object -Property ControllerNumber,ControllerLocation)

            if ($vhd.Count)
            {
                # Only expect exactly one or zero data disks at this time. Do not operate if additional
                # devices have been added.
                if ($vhd.Count -gt 2)
                {
                    Write-Error "Unexpected number of VM hard disks ($($vhd.Count)) attached to VM $($vm.Name): expecting at most a single data disk"
                    continue
                }

                function Remove
                {
                    Write-Progress -Id 1 -ParentId 0 -Activity "Detaching VM Data Disk from VM"
                    $path = $vhd[1].Path
                    $vhd[1] | Remove-VMHardDiskDrive
                    Write-Progress -Id 1 -ParentId 0 -Activity "Removing VM Data Disk"
                    Remove-Item $path -Force
                    Write-Progress -Id 1 -ParentId 0 -Activity "Removing VM Data Disk" -Completed
                }

                function Create
                {
                    $path = Join-Path $vm.Path "data.vhdx"

                    if ($null -eq $path)
                    {
                        continue
                    }

                    # Scrub stale/detached vhdx
                    if (Test-Path $path)
                    {
                        Remove-Item $path -Force
                    }

                    Write-Progress -Id 1 -ParentId 0 -Activity "Creating VM Data Disk"
                    # Allow VHD objects for newly created VHDX to appear on the pipeline. Note that HCI is 4KN, do not create
                    # the default 512e layout.
                    New-VHD -Path $path -SizeBytes $DataDiskSize -Fixed -LogicalSectorSize 4096

                    Write-Progress -Id 1 -ParentId 0 -Activity "Extending VM Data Disk Valid Data Length"
                    # Push valid data - this could be parameterized to expose the true fresh-out-of-box
                    # case of an empty/sparsed VHDX. This does not replace pre-seasoning though it does
                    # eliminate the first level of FS effects.
                    # Redirect stdout, allow stderr to flow out.
                    fsutil file setvaliddata $path (Get-Item $path).Length > $null

                    Write-Progress -Id 1 -ParentId 0 -Activity "Creating VM Data Disk" -Completed
                    $vm | Add-VMHardDiskDrive -Path $path
                }

                # Remove?
                if  ($DataDiskSize -eq 0)
                {
                    # No data device?
                    if ($vhd.Count -eq 1)
                    {
                        LogOutput "Confirmed no data disk attached to $($vm.Name)"
                        continue
                    }

                    LogOutput "Removing data disk from $($vm.Name)"
                    Remove
                }

                # Create data VHD?
                elseif ($vhd.Count -eq 1)
                {
                    LogOutput "Creating data disk for $($vm.Name)"
                    Create
                }

                # Resize existing data VHD. Note that this is destructive - we do not attempt to manage
                # a first-class resize up/down of the disk and its contained partitions/filesystems.
                # We assume for now that any disk level initialization will be repeated by the calling
                # framework, as notified by new vhd objects being returned in the pipeline.
                else
                {
                    $vhdx = Get-VHD -ComputerName $vm.ComputerName -VMId $vm.Id |? Path -eq $vhd[1].Path
                    if ($null -eq $vhdx)
                    {
                        Write-Error "Could not find VHDX on VM ($vm.Name) corresponding to $($vhd[1].Path)"
                    }
                    elseif ($vhdx.Size -ne $DataDiskSize)
                    {
                        # Native Resize-VHD does not apply to fixed VHDX - Remove/Create new
                        LogOutput "Resizing (remove/create) data disk for $($vm.Name)"
                        Remove
                        Create
                    }
                }
            }
        }

        Write-Progress -Id 0 -Activity "Updating VM Data Disks" -Completed
    }
}

function Get-FleetDisk
{
    [CmdletBinding()]
    param(
        [Parameter()]
        [string]
        $Cluster = '.',

        [Parameter()]
        [switch]
        $OsDisk,

        [Parameter()]
        [switch]
        $All
        )

    $nodes = @(Get-ClusterNode -Cluster $Cluster |? State -eq Up)
    foreach ($vhd in Get-VM -CimSession $nodes.Name |? Name -like 'vm-*' | Get-VMHardDiskDrive)
    {
        # If not including OS disk, only emit non-boot devices (boot is always at 0/0)
        if ($All -or
                 ($OsDisk -and ($vhd.ControllerLocation -eq 0 -and $vhd.ControllerNumber -eq 0)) -or
            (-not $OsDisk -and ($vhd.ControllerLocation -ne 0 -or $vhd.ControllerNumber -ne 0)))
        {
            $vhd
        }
    }
}

function Get-FleetVolumeEstimate
{
    [CmdletBinding()]
    param(
        [Parameter()]
        [string]
        $Cluster = '.',

        [Parameter()]
        [uint64]
        $CollectVolumeSize = (200GB),

        [Parameter()]
        [ValidateRange(1,50)]
        [uint32]
        $MAPMirrorPercentage = 20
        )

    $pool = @(Get-StorageSubsystem -CimSession $Cluster |? AutomaticClusteringEnabled | Get-StoragePool |? IsPrimordial -eq $false)

    if ($pool.Count -eq 0)
    {
        Write-Error "No pool found"
        return
    }

    if ($pool.Count -ne 1)
    {
        Write-Error "Unexpected number of pools found ($($pool.Count)) - please verify environment configuration"
        return
    }

    $pool = $pool[0]
    $poolFree = $pool.Size - $pool.AllocatedSize

    #
    # Get relevant m/p storage tiers. Subtle point - 2019 documented creating nested tiers manually w/o On<MediaType>.
    # So we search for NestedMirror* (w/wo mediatype) and *Parity*, the latter so we get [Nested]ParityOn<MediaType> as well
    # as NestedParity. Since we demand only one matching we're safe against odd cases.
    #

    $tierM = @($pool | Get-StorageTier |? ResiliencySettingName -eq Mirror |? FriendlyName -like 'MirrorOn*')
    $tierNM = @($pool | Get-StorageTier |? ResiliencySettingName -eq Mirror |? FriendlyName -like 'NestedMirror*')
    $tierP = @($pool | Get-StorageTier |? ResiliencySettingName -eq Parity |? FriendlyName -like '*Parity*')

    if ($tierM.Count -eq 0)
    {
        Write-Error "No per-media type mirror resiliency tier (MirrorOn<Type>) found for storage pool `'$($pool.FriendlyName)`' - please verify environment configuration"
        return
    }

    if ($tierM.Count -gt 1)
    {
        Write-Error "Multiple per-media type mirror resiliency tiers (MirrorOn<Type>) found for storage pool `'$($pool.FriendlyName)`' - estimation not supported"
        return
    }
    else
    {
        $tierM = $tierM[0]
    }

    if ($tierNM.Count -gt 1)
    {
        Write-Error "Multiple per-media type nested mirror resiliency tiers (NestedMirrorOn<Type>) found for storage pool `'$($pool.FriendlyName)`' - estimation not supported"
        return
    }
    elseif ($tierNM.Count -ne 0)
    {
        $tierNM = $tierNM[0]
    }
    else
    {
        $tierNM = $null
    }

    if ($tierP.Count -gt 1)
    {
        Write-Error "Multiple per-media type parity resiliency tiers (MirrorOn<Type>) found for storage pool `'$($pool.FriendlyName)`' - estimation not supported"
    }

    if ($tierP.Count -eq 0)
    {
        Write-Warning "No per-media type parity resiliency tier (ParityOn<Type>) found for storage pool `'$($pool.FriendlyName)`' - parity estimation skipped"
        $tierP = $null
    }
    else
    {
        $tierP = $tierP[0]
    }

    # Mirror efficiency is simply number of copies.
    $pEff = 0
    $mEff = 1 / $tierM.NumberOfDataCopies
    if ($null -ne $tierNM) { $nmEff = 1 / $tierNM.NumberOfDataCopies }

    if ($null -ne $tierP)
    {
        # Estimate parity tier efficiency based on reported max v. free space.
        $tierPEst = Get-StorageTierSupportedSize -InputObject $tierP
        $pEff = $tierPEst.TierSizeMax / $poolFree
    }

    # Determine reserve capacity per default guidance - one capacity device per node up to 4 total in cluster.
    # For estimation take the largest such device - if there is variance it should be within a very narrow
    # band (e.g., all 10TB devices plus or minus some small amount).
    $nodeCount = @(Get-ClusterNode -Cluster $Cluster).Count
    $capDevice = $pool | Get-PhysicalDisk -CimSession $Cluster |? Usage -eq Auto-Select | Sort-Object -Property Size -Descending | Select-Object -First 1

    if ($null -eq $capDevice)
    {
        Write-Error "Could not determine capacity device size for storage pool `'$($pool.FriendlyName)`' - please verify environment configuration and validate that there are Auto-Select devices reported"
    }

    if ($nodeCount -gt 4)
    {
        $capReserve = $capDevice.Size * 4
    }
    else
    {
        $capReserve = $capDevice.Size * $nodeCount
    }

    LogOutput -IsVb ("Pool {0:0.00}TiB total {1:0.00}TiB free: {2:0.00}TiB capacity devices in {3:0} node cluster => {4:0.00}TiB rebuild reserve" -f (
        ($pool.Size/1TB),
        ($poolFree/1TB),
        ($capDevice.Size/1TB),
        ($nodeCount),
        ($capReserve/1TB)))

    $nmVb = $null
    if ($null -ne $tierNM) { $nmVb = ", {0:P1} nested mirror efficiency" -f $nmEff }

    LogOutput -IsVb ("Using {0:P1} mirror efficiency$nmVb, {1:P1} parity efficiency and {2}:{3} MAP mirror:parity ratio" -f (
        $mEff,
        $pEff,
        $MAPMirrorPercentage,
        (100-$MAPMirrorPercentage)))

    #
    # Now roll up to the usable free space in the pool. This is the actual (current) free space plus the space
    # taken by existing (if any) non-infrastructure volumes, the only one of which is ClusterPerformanceHistory.
    #

    $collect = $null

    # Add back the footprint of existing infra volumes (if any). It might be best to validate that all of the
    # ones other than (potentiall existing) collect do indeed match to nodes per the fleet naming convention,
    # but there should be no other volumes present.
    $pool | Get-VirtualDisk |? FriendlyName -notin @('ClusterPerformanceHistory') |% {

        $poolFree += $_.FootprintOnPool
        if ($_.FriendlyName -eq 'collect')
        {
            $collect = $_
        }
    }

    # If an explicit collect size is provided, use it.
    # If the collect volume already exists, warn if the provided (if any) collect size estimate does not match.
    # If an explicit collect size is not provided, use the existing volume if present.
    if ($PSBoundParameters.ContainsKey('CollectVolumeSize'))
    {
        if ($null -ne $collect -and
            $CollectVolumeSize -ne $collect.Size)
        {
            Write-Warning ("A collect volume already exists but is not the same size as specified - {0:0.00}GiB != {1:0.00}GiB. The estimates will assume this is the desired size and that the existing volume would be deleted and recreated following the new specification. If this is not the intent, rerun without specifying the size to base the estimates on the existing volume." -f (
                ($collect.Size/1GB),
                ($CollectVolumeSize/1GB)))
        }

        # Per spec; existing volume (if any) was already accounted in usable space
        LogOutput -IsVb ("Collect: volume size specified is {0:0.00}GiB" -f ($CollectVolumeSize/1GB))
        $poolFree -= $CollectVolumeSize / $mEff
    }
    else
    {
        if ($null -ne $collect)
        {
            # Use existing volume; take it back out of usable space
            LogOutput -IsVb ("Collect: using existing volume ({0:0.00}GiB)" -f ($collect.Size/1GB))
            $poolFree -= $collect.FootprintOnPool
        }
        else
        {
            # Per default spec
            LogOutput -IsVb ("Collect: using default volume size ({0:0.00}GiB)" -f ($CollectVolumeSize/1GB))
            $poolFree -= $CollectVolumeSize / $mEff
        }
    }

    # Finally, remove the repair reserve
    $poolFree -= $capReserve

    #
    # Now estimate mirror and MAP volume tier sizes to arrive within a near margin of the usable free space.
    #

    function RoundSize
    {
        param( [uint64] $size )

        # Basic precision constrol:
        # Truncate TiB/GiB to nearest 10GiB, nearest 5GiB if < 500GiB
        $sizeGB = [uint32] ($size/1GB)
        if ($sizeGB -lt 500)
        {
            $sizeGB -= $sizeGB % 5
        }
        else
        {
            $sizeGB -= $sizeGB % 10
        }

        return $sizeGB * 1GB
    }

    # Collect volume estimate, if it does not already exist or an overriding size was specified.
    if ($null -eq $collect -or $PSBoundParameters.ContainsKey('CollectVolumeSize'))
    {
        [VolumeEstimate]::new([VolumeType]::Collect, $CollectVolumeSize, $tierm.FriendlyName, 0, $null)
    }

    # Mirror
    $vsizeM = RoundSize (($poolFree * $mEff)/$nodeCount)
    [VolumeEstimate]::new([VolumeType]::Mirror, $vsizeM, $tierM.FriendlyName, 0, $null)

    # Nested Mirror?
    if ($null -ne $tierNM)
    {
        $vsizeM = RoundSize (($poolFree * $nmEff)/$nodeCount)
        [VolumeEstimate]::new([VolumeType]::NestedMirror, $vsizeM, $tierNM.FriendlyName, 0, $null)
    }

    if ($null -ne $tierP)
    {
        # MAP
        # This is derived by solving the system of equations
        #    p/peff + m/meff = poolFree
        #    p/m = the given ratio of parity to mirror capacity
        # Note we can substitute for p (or m) in terms of the ratio; the solution
        # flows from there. MAP on environments with nested mirror defined are
        # presumed to use that tier definition for MAP.

        if ($null -ne $tierNM)
        {
            $mEffToUse = $nmEff
            $mTierName = $tierNM.FriendlyName
        }
        else
        {
            $mEffToUse = $mEff
            $mTierName = $tierM.FriendlyName
        }

        $pmRatio = (100 - $MAPMirrorPercentage)/$MAPMirrorPercentage
        $vsizeM = [uint64] ($poolFree / ($pmRatio/$pEff + 1/$mEffToUse))
        $vsizeP = RoundSize ($pmRatio * $vsizeM / $nodeCount)
        $vsizeM = RoundSize ($vsizeM / $nodeCount)
        [VolumeEstimate]::new([VolumeType]::MAP, $vsizeM, $mTierName, $vsizeP, $tierP.FriendlyName)
    }
}

function Get-FleetDataDiskEstimate
{
    [CmdletBinding(DefaultParameterSetName = 'Default')]
    param(
        [Parameter(ParameterSetName = 'Default')]
        [Parameter(ParameterSetName = 'ByPercent')]
        [Parameter(ParameterSetName = 'ByVMs')]
        [string]
        $Cluster = '.',

        [Parameter(ParameterSetName = 'Default')]
        [Parameter(ParameterSetName = 'ByPercent')]
        [Parameter(ParameterSetName = 'ByVMs')]
        [double]
        [ValidateRange(10,1000)]
        $CachePercent,

        [Parameter(ParameterSetName = 'Default')]
        [Parameter(ParameterSetName = 'ByPercent')]
        [Parameter(ParameterSetName = 'ByVMs')]
        [ValidateRange(1,100)]
        [double]
        $CapacityPercent,

        [Parameter(ParameterSetName = 'Default')]
        [Parameter(ParameterSetName = 'ByPercent')]
        [Parameter(ParameterSetName = 'ByVMs')]
        [switch]
        $Higher,

        [Parameter(ParameterSetName = 'ByVMs', Mandatory = $true)]
        [uint32]
        $VMs,

        [Parameter(ParameterSetName = 'Default')]
        [switch]
        $Running,

        [Parameter(ParameterSetName = 'ByPercent', Mandatory = $true)]
        [ValidateRange(1,100)]
        [double]
        $VMPercent
        )

        $clusterParam = @{}
        CopyKeyIf $PSBoundParameters $clusterParam 'Cluster'

        if (-not $PSBoundParameters.ContainsKey('CachePercent') -and
            -not $PSBoundParameters.ContainsKey('CapacityPercent'))
        {
            throw "One or both of CachePercent or CapacityPercent must be specified."
        }

        $pool = @(Get-StorageSubsystem -CimSession $Cluster |? AutomaticClusteringEnabled | Get-StoragePool |? IsPrimordial -eq $false)

        if ($pool.Count -eq 0)
        {
            Write-Error "No pool found"
            return
        }

        if ($pool.Count -ne 1)
        {
            Write-Error "Unexpected number of pools found ($($pool.Count)) - please verify environment configuration"
            return
        }

        $cacheDevice = @($pool | Get-PhysicalDisk -CimSession $Cluster |? Usage -eq Journal)

        if ($cacheDevice.Count -eq 0 -and -not $PSBoundParameters.ContainsKey('CapacityPercent'))
        {
            Write-Error "There are no identifiable cache devices (Usage = Journal) - please verify environment configuration"
            return
        }

        if ($cacheDevice |? HealthStatus -ne Healthy)
        {
            Write-Warning "There are cache devices that are not healthy - please verify environment before proceeding"
        }

        if ($cacheDevice.Count -ne 0)
        {
            $cacheCapacity = ($cacheDevice | Measure-Object -Property Size -Sum).Sum
        }
        else
        {
            $cacheCapacity = 0
        }

        $nodes = @(Get-ClusterNode @clusterParam)

        if ($nodes |? State -ne Up)
        {
            Write-Warning "There are nodes which are not currently up - please verify environment before proceeding"
        }

        ##
        # Get available space
        ##

        $availableCapacity = 0
        $totalVolumeCapacity = 0

        #
        # Get total/data space available in fleet volumes
        #

        $nodes = Get-ClusterNode @clusterParam
        $volumes = Get-Volume -CimSession $Cluster

        foreach ($volume in $volumes)
        {
            foreach ($node in $nodes)
            {
                if ($volume.FileSystemLabel -match "^$node(?:-.+){0,1}$")
                {
                    LogOutput -IsVb "volume $($volume.FileSystemLabel) matched to node $node"
                    $availableCapacity += $volume.SizeRemaining
                    $totalVolumeCapacity += $volume.Size
                    break
                }
            }
        }

        #
        # Get size of current data disks (if any) to account for reusable space. Note this is
        # physical used capacity, not logical capacity presented to VM.
        #

        $curDataDiskCapacity = 0
        $dataDisk = @(Get-FleetDisk @clusterParam)

        if ($dataDisk.Count)
        {
            #
            # Note that disk paths are local to the cluster.
            # En-list list of paths to pass as single list parameter.
            #

            $curDataDiskCapacity = Invoke-Command -ComputerName $Cluster -ArgumentList @(,@($dataDisk.Path)) {

                param(
                    [string[]]
                    $paths
                    )

                $s = 0
                foreach ($p in $paths)
                {
                    $s += (Get-Item $p).Length
                }
                $s
            }
        }

        #
        # Add in current utilization - available for use (resize)
        #

        $availableCapacity += $curDataDiskCapacity

        #
        # Now validate and convert VM count from VMs/node to total VMs.
        # This is consistent with New-Fleet.
        #

        if (-not $PSBoundParameters.ContainsKey('VMs'))
        {
            $vmGroups = @(Get-ClusterGroup @clusterParam |? GroupType -eq VirtualMachine |? Name -like 'vm-*')

            #
            # Limit estimate to VMs currently running? Allows for at-a-distance dynamic resizing of fleet (e.g., start
            # a fraction, resize per-vm workingset so that they cover)
            # Limit to a percentage of configured VMs? Not dependent on VMs being started yet.
            #

            $VMs = $totalVMs = $vmGroups.Count
            if ($VMs % $nodes.Count)
            {
                Write-Error "There are an uneven number of Fleet VMs: $VMs VMs for $($nodes.Count) nodes - please verify environment configuration"
                return
            }
            elseif ($VMs -eq 0)
            {
                Write-Error "No Fleet VMs found - please verify environment configuration"
                return
            }

            if ($Running)
            {
                $vmGroups = @($vmGroups |? State -eq Online)
                $VMs = $vmGroups.Count

                if ($VMs -eq 0)
                {
                    Write-Error "No running Fleet VMs found - please verify environment configuration"
                    return
                }

                LogOutput -IsVb "Estimate based on running VM count of $VMs of the $totalVMs total"
            }
            elseif ($PSBoundParameters.ContainsKey('VMPercent'))
            {
                # Note this is in absolute numbers of VMs. Round to an even number of VMs/node, and take 0 upward to 1/node.
                $VMs = [int] ($VMPercent * $VMs / 100)
                $VMs -= $VMs % $nodes.Count
                if ($VMs -eq 0) { $VMs = $nodes.Count }

                LogOutput -IsVb "Estimate based on $VMs VMs, $VMPercent% of the $totalVMs total VMs (evenly distributed per node)"
            }
            else
            {
                LogOutput -IsVb "Estimate based on the $totalVMs total VMs"
            }
        }
        else
        {
            $VMs *= $nodes.Count
        }

        ##
        # Generate estimates - one or two depending on call method
        ##

        $cacheEst = $null
        $capEst = $null

        #
        # Truncate to the nearest 512MiB. Coerce the multiplier result back to uint64.
        #

        $trunc = 512MB
        if ($PSBoundParameters.ContainsKey('CachePercent') -and $cacheCapacity -ne 0)
        {
            $c = ([uint64]($CachePercent * $cacheCapacity)) / $VMs / 100
            $c -= $c % $trunc

            if ($c * $VMs -ge $availableCapacity)
            {
                Write-Warning "There is is not enough capacity to reach the specified percentage of cache capacity - please verify environment before proceeding. Truncating to available capacity."

                $c = $availableCapacity / $VMs
                $c -= $c % $trunc
            }

            $cacheEst = $c
        }

        if ($PSBoundParameters.ContainsKey('CapacityPercent'))
        {
            $c = ([uint64]($CapacityPercent * $availableCapacity)) / $VMs / 100
            $c -= $c % $trunc

            $capEst = $c
        }

        ##
        # Pick higher (or lower) - $null is excluded automatically
        ##

        if ($Higher)
        {
            $dataDiskCapacity = ($cacheEst, $capEst | Measure-Object -Maximum).Maximum
        }
        else
        {
            $dataDiskCapacity = ($cacheEst, $capEst | Measure-Object -Minimum).Minimum
        }

        if ($null -ne $cacheEst)
        {
            LogOutput -IsVb ("Cache percentage of {0:P1} yielded {1:0.0}GiB/VM" -f (
                ($CachePercent/100),
                ($cacheEst/1GB)))
        }

        if ($null -ne $capEst)
        {
            LogOutput -IsVb ("Capacity percentage of {0:P1} yielded {1:0.0}GiB/VM" -f (
                ($CapacityPercent/100),
                ($capEst/1GB)))
        }

        if ($null -ne $cacheEst -and $null -ne $capEst)
        {
            if ($Higher)
            {
                LogOutput -IsVb "Chose HIGHER estimate"
            }
            else
            {
                LogOutput -IsVb "Chose LOWER estimate"
            }
        }

        ##
        # Informational output
        ##

        LogOutput -IsVb ("Estimated data disk is {0:0.0}GiB/VM " -f (
            ($dataDiskCapacity/1GB)))
        LogOutput -IsVb ("Environment: {0} nodes, $VMs total VMs" -f (
            $nodes.Count))
        LogOutput -IsVb ("Available for data disks: {0:0.00}TiB ({1:P1}) of total {2:0.00}TiB volume capacity - {3:0.00}TiB is in use (OS disks, VM metadata, other content)" -f (
            ($availableCapacity/1TB),
            ($availableCapacity/$totalVolumeCapacity),
            ($totalVolumeCapacity/1TB),
            (($totalVolumeCapacity - $availableCapacity)/1TB)))

        if ($cacheCapacity -ne 0)
        {
            LogOutput -IsVb ("Cache (v. Available): {0:0.00}TiB cache capacity, available volume capacity is {1:0.00}x cache, cache is {2:P1} of available)" -f (
                ($cacheCapacity/1TB),
                ($availableCapacity/$cacheCapacity),
                ($cacheCapacity/$availableCapacity)))
            LogOutput -IsVb ("Cache (v. Total): {0:0.00}TiB cache capacity, total volume capacity is {1:0.00}x cache, cache is {2:P1} of total)" -f (
                ($cacheCapacity/1TB),
                ($totalVolumeCapacity/$cacheCapacity),
                ($cacheCapacity/$totalVolumeCapacity)))
        }

        # Result is uint64
        return $dataDiskCapacity
}

# a single named Variable with a set of values
class Variable {

    [int] $_ordinal
    [object[]] $_list
    [string] $_label

    Variable([string] $label, [object[]] $list)
    {
        $this._list = $list
        $this._ordinal = 0
        $this._label = $label
    }

    # current value of the Variable ("red"/"blue"/"green")
    [object] value()
    {
        if ($null -ne $this._list -and $this._list.Count -gt 0)
        {
            return $this._list[$this._ordinal]
        }
        else
        {
            return $null
        }
    }

    # label/name of the Variable ("color")
    [object] label()
    {
        return $this._label
    }

    # increment to the next member, return whether a logical carry
    # has occured (overflow)
    [bool] increment()
    {
        # empty list passes through
        if ($null -eq $this._list -or $this._list.Count -eq 0)
        {
            return $true
        }

        # non-empty list, increment
        $this._ordinal += 1
        if ($this._ordinal -eq $this._list.Count)
        {
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

# a set of Variables which allows enumeration of their combinations
# this behaves as a numbering system indexing the respective Variables
# order is not specified
class VariableSet {

    [hashtable] $_set = @{}

    VariableSet([Variable[]] $list)
    {
        $list |% { $this._set[$_.label()] = $_ }
        $this._set.Values |% { $_.reset() }
    }

    # increment the enumeration
    # returns true if the enumeration is complete
    [bool] increment()
    {
        $carry = $false
        foreach ($v in $this._set.Values)
        {
            # if the Variable returns the carry flag, increment
            # the next, and so forth
            $carry = $v.increment()
            if (-not $carry) { break }
        }

        # done if the most significant carried over
        return $carry
    }

    # enumerator of all Variables
    [object] getset()
    {
        return $this._set.Values
    }

    # return value of specific Variable
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

            # only produce labels for non-null, non-empty (as string) values
            if ($this._set.Contains($lookstr) -and $null -ne $this._set[$lookstr].value())
            {
                $vstr = [string] $this._set[$lookstr].value()
                if ($vstr.Length) { "$pfx$str" + $vstr }
            }
        }) -join $null
    }
}

function GetDoneFlags
{
    [CmdletBinding()]
    param(
        [Parameter(
            Mandatory = $true
        )]
        [string]
        $DonePath,

        [Parameter(
            Mandatory = $true
        )]
        [int]
        $GoEpoch,

        [Parameter(
            Mandatory = $true
        )]
        [Microsoft.FailoverClusters.PowerShell.ClusterGroup[]]
        $VM,

        [Parameter()]
        [double]
        $Timeout = 120,

        [Parameter()]
        [switch]
        $AssertNone,

        [Parameter()]
        [string]
        $Tag = ''
    )

    # Hash of VMs to wait on. false indicates not done, true done. Prepopulate to filter flags.
    $vmDone = @{}
    foreach ($v in $VM)
    {
        $vmDone[$v.Name] = $false
    }

    $done = 0
    $tries = 0
    $t0 = Get-Date
    $t1 = $t2 = $null

    do {

        if ($null -ne $t1) {
            Start-Sleep 2
        }

        # increment attempts and capture start of the iteration
        $tries += 1
        $t1 = Get-Date

        # count number of done flags which agree with completion of the current go epoch
        Get-ChildItem "$DonePath-*" |% {

            # Now split the VM name from the flag file name (done-vm-xyz); ignore malformed,
            # ones which are not in our expected set, and ones already done.
            $names = $_.Name -split '-',2
            if ($names.Count -eq 2 -and
                $vmDone.ContainsKey($names[1]) -and
                $vmDone[$names[1]] -eq 0)
            {
                $thisDone = [int](Get-Content $_.FullName -ErrorAction SilentlyContinue)
                if ($thisDone -eq $GoEpoch) {

                    # if asserting that none should be complete, this is an error!
                    if ($AssertNone) {
                        LogOutput -ForegroundColor Red "ERROR: $($names[1]) is already done"
                    }

                    $done += 1
                    $vmDone[$names[1]] = $true
                }
            }
        }

        # color status message per condition
        if (($AssertNone -and $done -ne 0) -or $done -ne $VM.Count) {
            $color = [System.ConsoleColor]::Yellow
        } else {
            $color = [System.ConsoleColor]::Green
        }

        $t2 = Get-Date
        $itdur = $t2 - $t1
        $totdur = $t2 - $t0

        LogOutput -ForegroundColor $color $Tag CHECK $tries : "$done/$($VM.Count)" done $('({0:F2}s, total {1:F2}s)' -f $itdur.TotalSeconds, $totdur.TotalSeconds)

    # loop if not asserting, not all vms are done, and still have timeout to use
    } while ((-not $AssertNone) -and $done -ne $VM.Count -and $totdur.TotalSeconds -lt $Timeout)

    # asserting that none should be complete?
    if ($AssertNone)
    {
        return ($done -eq 0)
    }

    # return incomplete run failure
    elseif ($done -ne $VM.Count) {
        LogOutput -ForegroundColor Red ABORT: only received "$done/$($VM.Count)" completions in $tries attempts

        # log out VMs not done
        $en = $vmDone.GetEnumerator()
        while ($en.MoveNext())
        {
            if ($en.Value -eq $false) {
                LogOutput -ForegroundColor Yellow NOT DONE: $en.Key
            }
        }

        return $false
    }

    # all worked!
    return $true
}

function ClearDoneFlags
{
    [CmdletBinding()]
    param(
        [Parameter(
            Mandatory = $true
        )]
        [string]
        $DonePath
    )

    Get-ChildItem "$DonePath-*" | Remove-Item -Force
}

function GetProfileDuration
{
    [CmdletBinding()]
    param(
        [Parameter(
            ParameterSetName = "ByProfilePath",
            Mandatory = $true
        )]
        [string]
        $ProfilePath,

        [Parameter(
            ParameterSetName = "ByProfileXML",
            Mandatory = $true
        )]
        [xml]
        $ProfileXml,

        [switch]
        $Total,

        [switch]
        $Warmup,

        [switch]
        $Duration,

        [switch]
        $Cooldown
    )

    if ($PSCmdlet.ParameterSetName -eq "ByProfilePath")
    {
        $ProfileXml = [xml] (Get-Content $ProfilePath)
    }

    if ($null -eq $ProfileXml)
    {
        Write-Error "XML profile not readable"
        return
    }

    $timeSpans = $ProfileXml.SelectNodes("Profile/TimeSpans/TimeSpan")

    if ($null -eq $timeSpans)
    {
        Write-Error "XML Profile does not contain TimeSpan elements - verify this is a profile"
        return
    }

    $w = $d = $c = 0
    $timeSpans |% {
        if ($_.Item("Warmup"))   { $w += [int] $_.Warmup }
        if ($_.Item("Duration")) { $d += [int] $_.Duration }
        if ($_.Item("Cooldown")) { $c += [int] $_.Cooldown }
    }

    if ($PSBoundParameters.ContainsKey('Total'))
    {
        return $w + $d + $c
    }

    if ($PSBoundParameters.ContainsKey('Warmup')) { $w }
    if ($PSBoundParameters.ContainsKey('Duration')) { $d }
    if ($PSBoundParameters.ContainsKey('Cooldown')) { $c }
}

function Clear-FleetRunState
{
    [CmdletBinding()]
    param(
        [Parameter()]
        [string]
        $Cluster = "."
    )

    #############
    $clusterParam = @{}
    CopyKeyIf $PSBoundParameters $clusterParam 'Cluster'

    # Go/Done Paths
    ($goPath, $donePath) = Get-FleetPath -PathType Go,Done @clusterParam

    # Scrub, reset go counter to zero
    Get-ChildItem "$donePath-*" | Remove-Item -Force
    Write-Output 0 > $goPath
}

function Start-FleetRun
{
    [CmdletBinding()]
    param(
        [Parameter()]
        [string]
        $Cluster = ".",

        [Parameter()]
        [string]
        $AddSpec = "",

        [Parameter(
            ParameterSetName = "ByProfilePath",
            Mandatory = $true
        )]
        [string]
        $ProfilePath,

        [Parameter(
            ParameterSetName = "ByProfileXml",
            Mandatory = $true
        )]
        [xml]
        $ProfileXml,

        [Parameter(ParameterSetName = "ByProfilePath")]
        [Parameter(ParameterSetName = "ByProfileXml")]
        [string]
        $RunFile = "sweep.ps1",

        [Parameter(ParameterSetName = "ByProfilePath")]
        [Parameter(ParameterSetName = "ByProfileXml")]
        [string]
        $RunProfileFile = "sweep.xml",

        [Parameter(
            ParameterSetName = "ByPrePlaced",
            Mandatory = $true
        )]
        [switch]
        $PrePlaced,

        [Parameter(
            ParameterSetName = "ByPrePlaced",
            Mandatory = $true
        )]
        [int]
        $Duration,

        [Parameter()]
        [switch]
        $Async,

        [Parameter()]
        [string[]]
        $pc = $null,

        [Parameter()]
        [ValidateRange(1,60)]
        [int]
        $SampleInterval = 1
    )

    #
    # Core utility to put a given single run/iteration into the fleet with the
    # go/done response mechanism.
    #
    #   * ByPrePlaced - a pre-placed run script (*.ps1) is assumed, and the expected
    #       duration must be provded.
    #   * ByProfilePath - a pre-placed DISKSPD profile is specified. A template run
    #       script is emitted to run.
    #

    #############
    $clusterParam = @{}
    CopyKeyIf $PSBoundParameters $clusterParam 'Cluster'

    # Async is not compatible with performance counter capture. Consider supporting fully external management
    # of async runs (including logman/done flag checks) for future.
    if ($PSBoundParameters.ContainsKey('pc') -and $Async)
    {
        Write-Error "Performance counter captures (pc) cannot be done for Async runs"
        return
    }

    # Paths including node-local CSV path to results (for blg drop)
    ($controlPath, $goPath, $donePath) = Get-FleetPath -PathType Control,Go,Done @clusterParam
    $resultPathLocal = Get-FleetPath -PathType Result -Local @clusterParam

    # Get expected run duration in the implicit cases
    switch ($psCmdlet.ParameterSetName)
    {
        "ByPrePlaced" { break }
        "ByProfileXml"
        {
            $Duration = GetProfileDuration -ProfileXml $ProfileXml -Total
        }
        "ByProfilePath"
        {
            $Duration = GetProfileDuration -ProfilePath $ProfilePath -Total
        }
    }

    if ($null -eq $Duration -or $Duration -eq 0)
    {
        Write-Error "Run duration could not be determined, cannot continue"
        return
    }

    # Place profile in the control area?
    switch ($psCmdlet.ParameterSetName)
    {
        "ByPrePlaced"
        {
            # Pause for SMB dir/info cache to expire and surface preplaced file.
            # Dropping go flag will release the run.
            Start-Sleep $smbInfoCacheLifetime
            break
        }
        "ByProfileXml"
        {
            $runProfileFilePath = Join-Path $controlPath $RunProfileFile
            Remove-Item $runProfileFilePath -Force -ErrorAction SilentlyContinue
            $ProfileXml | Convert-FleetXmlToString | Out-File $runProfileFilePath -Encoding ascii -Width ([int32]::MaxValue) -Force
        }
        "ByProfilePath"
        {
            $runProfileFilePath = Join-Path $controlPath $RunProfileFile
            Remove-Item $runProfileFilePath -Force -ErrorAction SilentlyContinue
            Copy-Item $ProfilePath $runProfileFilePath -Force
        }
    }

    # Get nodes for performance counter capture (if needed)
    if ($null -ne $pc)
    {
        $nodes = Get-ClusterNode @clusterParam |? State -eq Up | Sort-Object -Property Name
    }

    # VMs expected to participate
    $vms = @(Get-ClusterGroup @clusterParam |? GroupType -eq VirtualMachine |? Name -like "vm-*" |? State -eq Online)

    if (Get-FleetPause @clusterParam)
    {
        $clearPause = $true
    }
    else
    {
        $clearPause = $false
    }

    # Kick off go iteration
    $goepoch = [int](Get-Content $goPath -ErrorAction SilentlyContinue)
    $goepoch += 1

    LogOutput "START Go Epoch: $goepoch"
    Write-Output $goepoch > $goPath

    # Create templated run script to inject this profile?
    if ($psCmdlet.ParameterSetName -ne "ByPrePlaced")
    {
         # Pause for SMB dir/info cache to expire and surface go flag.
         # Runner will wait for new content - note unique guid.
         Start-Sleep $smbInfoCacheLifetime

        $vmprof = "L:\control\$RunProfileFile"
        $guid = [System.Guid]::NewGuid().Guid

        $f = Join-Path $controlPath $RunFile
        $(Get-Command Set-RunProfileTemplateScript).ScriptBlock |% {

            # apply subsitutions to produce a new run file
            $line = $_
            $line = $line -replace '__AddSpec__',$AddSpec
            $line = $line -replace '__Profile__',$vmprof
            $line = $line -replace '__Unique__',$guid
            $line

        } | Out-File "$($f).tmp" -Encoding ascii -Width ([int32]::MaxValue)
        if (Test-Path $f) { Remove-Item $f -Force }
        Rename-Item "$($f).tmp" $RunFile
    }

    # capture time zero
    $t0 = get-date

    # release pause
    if ($clearPause)
    {
        LogOutput "CLEAR pause at Go"
        Clear-FleetPause @clusterParam
    }

    # arm flag to guarantee teardown of logman instances on exception
    $stopLogman = $false

    try
    {
        # start performance counter capture
        if ($null -ne $pc)
        {
            StartLogman -Computer $nodes -Name $AddSpec -Counters $pc -SampleInterval $SampleInterval
            $stopLogman = $true
        }

        ######

        # If total run is more than ffMin seconds, sleep to ffCheckAt and look for false completions
        # for up to ffCheckWindow seconds. Controlling the check duration is important in case it
        # takes an unexpectedly long time to go through the done flags, regardless of reason.

        $fastFailMin = 60
        $fastFailCheckAt = 20
        $fastFailCheckWindow = 15

        if ($Duration -gt $fastFailMin)
        {
            # Account for time taken to this point (logman/clear) to arrive at the check time
            $td = (Get-Date) - $t0
            $remainingSleep = $fastFailCheckAt - $td.TotalSeconds

            if ($remainingSleep -gt 0)
            {
                LogOutput SLEEP TO RUN CHECK "($('{0:0.00}' -f $remainingSleep) seconds)"
                Start-Sleep -Milliseconds ($remainingSleep*1000)
                $td = (Get-Date) - $t0
            }

            LogOutput RUN CHECK Go Epoch: $goepoch

            # Calculate effective time to check flags: how far into the timeout interval
            # are we already. Note that this may wind up missing early completion if the run
            # is very short. TBD positive successful completion ack as opposed to job completion.
            $fastFailCheckTimeout = $fastFailCheckWindow - ($td.TotalSeconds - $fastFailCheckAt)

            if ($fastFailCheckTimeout -gt 0)
            {
                if (-not (GetDoneFlags -DonePath $donePath -GoEpoch $goepoch -VM $vms -AssertNone -Tag 'RUN' -Timeout $fastFailCheckTimeout))
                {
                    throw "Unexpected early completion of load, please check profile and virtual machines for errors"
                }
            }
            LogOutput -ForegroundColor green RUN CHECK PASS Go Epoch: $goepoch
        }

        # If this is an async run, done here.
        # Consider exposing logman and done flag check for fully external async management of runs.
        if ($Async)
        {
            return
        }

        # Sleep for the remaining interval
        $td = (Get-Date) - $t0
        $remainingsleep = $Duration - $td.TotalSeconds

        if ($remainingsleep -gt 0)
        {
            LogOutput SLEEP TO END "($('{0:0.00}' -f $remainingsleep) seconds)"
            Start-Sleep -Milliseconds ($remainingsleep*1000)
        }

        # Stop performance counter capture
        # Do this before polling done flags, which can take time
        if ($stopLogman)
        {
            StopLogman -Computer $nodes -Name $AddSpec -Path $resultPathLocal
            $stopLogman = $false
        }

        if (-not (GetDoneFlags -DonePath $donePath -GoEpoch $goepoch -VM $vms -Tag COMPLETION))
        {
            throw "Load did not fully complete, please check profile and virtual machines for errors"
        }
    }
    catch
    {
        # Pause fleet under error conditions

        Set-FleetPause
        throw
    }
    finally
    {
        # Stop and abandon capture if normal termination did not occur (do not pull blg back)
        if ($stopLogman)
        {
            StopLogman -Computer $nodes -Name $AddSpec
        }

        if (-not $Async)
        {
            # Work around two issues for callers:
            #
            #   * conflicting handles on the XML result files (sleep)
            #   * delegated xcopy in StopLogman appears to leave references that
            #     hold up access to BLG until the CSV is bounced.
            #
            # TBD track these down. Overlap the 20s wait for XML handles with the CSV move.
            #
            # Async runs will need to manage this (if needed) directly if/when full external
            # management is supported.

            $t0 = Get-Date
            $null = Get-ClusterSharedVolume @clusterParam |? Name -match $collectVolumeName | Move-ClusterSharedVolume

            $td = (Get-Date) - $t0
            $tms = 20*1000 - $td.TotalMilliseconds
            if ($tms -gt 0)
            {
                Start-Sleep -Milliseconds $tms
            }
        }
    }
}

function Start-FleetSweep
{
    [CmdletBinding()]
    param(
        [Parameter()]
        [string]
        $Cluster = ".",

        [Parameter()]
        [string]
        $AddSpec,

        [Parameter()]
        [string]
        $RunTemplate,

        [Parameter()]
        [string]
        $RunFile = "sweep.ps1",

        [Parameter()]
        [string[]]
        $LabelTemplate = @('b','t','o','w','p','iops','-$addspec'),

        [Parameter(Mandatory =$true)]
        [int[]]
        $b,

        [Parameter(Mandatory =$true)]
        [int[]]
        $t,

        [Parameter(Mandatory =$true)]
        [int[]]
        $o,

        [Parameter(Mandatory =$true)]
        [int[]]
        $w,

        [Parameter()]
        [ValidateSet('r','s','si')]
        [string[]]
        $p = 'r',

        [Parameter()]
        [int[]]
        $iops = $null,

        [Parameter()]
        [ValidateRange(1,[int]::MaxValue)]
        [int]
        $d = 30,

        [Parameter()]
        [ValidateRange(0,[int]::MaxValue)]
        [int]
        $Warm = 60,

        [Parameter()]
        [ValidateRange(0,[int]::MaxValue)]
        [int]
        $Cool = 30,

        [Parameter()]
        [string[]]
        $pc = $null,

        [Parameter()]
        [ValidateRange(1,60)]
        [int]
        $SampleInterval = 1
        )

    #############
    $clusterParam = @{}
    CopyKeyIf $PSBoundParameters $clusterParam 'Cluster'

    # Paths including node-local CSV path to results (for blg drop)
    ($controlPath, $goPath, $donePath) = Get-FleetPath -PathType Control,Go,Done @clusterParam

    # Convert RunFile to fully qualified. Any type of non-plain filename is not allowed: this will
    # always be placed in the control path.
    if ($RunFile.Contains('\'))
    {
        Write-Error "$RunFile must be a plain filename, to be created in the control path ($controlPath)"
        return
    }
    $RunFilePath = Join-Path $controlPath $RunFile

    function NewRunFile
    {
        param(
            [VariableSet]
            $vs,

            [string]
            $runtemplate
            )

        $guid = [System.Guid]::NewGuid().Guid

        # Use template script if offered, else builtin
        $(if ($runtemplate.Length -gt 0)
        {
            Get-Content $runtemplate
        }
        else
        {
            (Get-Command Set-SweepTemplateScript).ScriptBlock
        }) |% {

            # apply current subsitutions to produce a new run file

            $line = $_

            foreach ($v in $vs.getset()) {
                # non-null goes in as is, null goes in as evaluatable $null
                if ($null -ne $v.value()) {
                    $vsub = $v.value()
                } else {
                    $vsub = '$null'
                }
                $line = $line -replace "__$($v.label())__",$vsub
            }

            $line = $line -replace '__Unique__',$guid
            $line

        } | Out-File "$($RunFilePath).tmp" -Encoding ascii -Width ([int32]::MaxValue)
        if (Test-Path $RunFilePath) { Remove-Item $RunFilePath -Force }
        Rename-Item "$($RunFilePath).tmp" $RunFile
    }

    function ShowRun(
        [VariableSet] $vs
        )
    {
        # show current substitions (minus the underscore bracketing)
        LogOutput -ForegroundColor green RUN PARAMETERS
        foreach ($v in $vs.getset()) {
                if ($null -ne $v.value()) {
                    $vsub = $v.value()
                } else {
                    $vsub = '$null'
                }
                LogOutput -ForegroundColor green "`t$($v.label()) = $($vsub)"
        }
    }

    function GetRunDuration(
        [VariableSet] $vs
        )
    {
        $vs.get('d') + $vs.get('Warm') + $vs.get('Cool')
    }

    #############

    # Start paused so that when run state is cleared VMs do not enter free run
    # and begin executing the first run file for this sweep pass.
    Set-FleetPause @clusterParam
    Clear-FleetRunState @clusterParam

    # construct the Variable list describing the sweep

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
    $v += [Variable]::new('b',$b)
    $v += [Variable]::new('t',$t)
    $v += [Variable]::new('o',$o)
    $v += [Variable]::new('w',$w)
    $v += [Variable]::new('p',$p)
    $v += [Variable]::new('iops',$iops)
    $v += [Variable]::new('d',$d)
    $v += [Variable]::new('Warm',$warm)
    $v += [Variable]::new('Cool',$cool)
    $v += [Variable]::new('AddSpec',$addspec)

    $sweep = [VariableSet]::new($v)

    try
    {
        do
        {
            Write-Host -ForegroundColor Cyan ('-'*20)

            ShowRun $sweep
            NewRunFile $sweep $RunTemplate

            Start-FleetRun -PrePlaced @clusterParam -AddSpec ($sweep.label($LabelTemplate)) -Duration (GetRunDuration $sweep) -pc $pc -SampleInterval $SampleInterval -ErrorVariable e

            if ($e.Count)
            {
                return
            }

        } while (-not $sweep.increment())
    }
    finally
    {
        # Set pause to quiesce the system.
        # Remove the sweep runfile.
        if (-not (Get-FleetPause))
        {
            Set-FleetPause @clusterParam
        }

        if (Test-Path $RunFilePath)
        {
            Remove-Item $RunFilePath -Force
        }
    }
}

function Start-FleetWriteWarmup
{
    [CmdletBinding()]
    param(
        [Parameter()]
        [string]
        $Cluster = '.',

        [Parameter(Mandatory = $true)]
        [uint64]
        $VMWorkingSetSize
        )

    $nodes = Get-ClusterNode -Cluster $Cluster |? State -eq Up

    $vm = @(Get-FleetVM -Cluster $Cluster |? State -eq Ok)
    if ($vm.Count -eq 0)
    {
        throw 'No running VMs found'
    }

    # Write warmup is a sequential workingset rewrite. One example of similar issues addressed in public
    # specification is in the SNIA Solid State Performance Test Specification's "Workload Independent
    # Pre-Conditioning" (see v 2.0.2 sec 3.3 and references throughout). Although VM Fleet is operating
    # at an aggregate level, the same princple applies the the virtual storage stack.
    #
    # In general VM Fleet will avoid most known reasons to need to do this, at least for the sake of
    # its current goal of measuring conditions in long-term running state: it does not use block cloning
    # or leave workingsets in sparse states (currently), for instance.
    #
    # However, a major reason for VM Fleet to handle this is on parity volumes: Spaces tracks whether parity
    # stripes have been written so that it can calculate parity incrementally v. requiring a full stripe
    # read/write. The tracking for this also allows the read path to optimize away the reads, similar to
    # sparse/valid data length management. If portions of the workingset wind up being initially allocated
    # v. parity storage, reads to that content would be optimized away until a write workload came in and
    # hit it. While certainly an interesting condition to evaluate, it is indeed a distinct and shorter term
    # operating condition.

    LogOutput "WRITE WARMUP: START"

    #
    # Now warm up. Build a write profile with one thread of sequential IO.
    #

    $t0 = $null
    $warmupRun = 3600 * 24 * 7
    $blockSize = 128KB

    $dynamicProfile = Get-FleetProfileXml -Name Peak -ThreadsPerTarget 1 -WriteRatio 100 -BlockSize $blockSize -Alignment $blockSize -RequestCount 8 -Sequential -Duration $warmupRun

    $t0 = Get-Date
    Start-FleetRun -ProfileXml $dynamicProfile -Async

    $pc = @('\Cluster CSVFS(_Total)\Write Bytes/sec',
            '\Cluster Storage Hybrid Disks(_Total)\Disk Write Bytes/sec'
            )

    $sampleInterval = 15

    # Track total CSV write bytes to enforce minimum traversal of the logical workingset.
    # The physical workingset traversal (and disk write bytes) are informative, only.
    $totalws = $vm.Count * $VMWorkingSetSize
    $totalwb = [uint64] 0

    # Traverse 2.2x since we don't have direct visibility into each VM's progress
    $traversews = 2.2 * $totalws

    LogOutput ("WRITE WARMUP: $($vm.Count) VMs @ {0:0.00}GiB/VM => {1:0.00}TiB total workingset to traverse" -f (
        ($VMWorkingSetSize/1GB),
         ($traversews/1TB)))

    try
    {
        do
        {
            # Aggregate samples across the cluster - note that all counters are currently unique
            # by counter name, just use that.
            $samples = Get-Counter $pc -ComputerName $nodes -SampleInterval $sampleInterval
            $samplesAgg = @{}
            $samples.CounterSamples |% {

                # Counter name is fifth element when split by seperator: \\machine\set(instance)\ctr
                $ctr = ($_.Path -split '\\')[4]
                if ($samplesAgg.ContainsKey($ctr))
                {
                    $samplesAgg.$ctr += $_.CookedValue
                }
                else
                {
                    $samplesAgg.$ctr = $_.CookedValue
                }
            }

            $csvwb = [uint64] ($samplesAgg."Write Bytes/sec")
            $dwb = [uint64] ($samplesAgg."Disk Write Bytes/sec")

            $totalwb += $csvwb * $sampleInterval

            LogOutput ("Write rate: {0:0.00}GiB/s, total {1:0.00}TiB {2:P1} complete (physical: {3:0.00}GiB/s, {4:0.0}x amplification)" -f (
                ($csvwb/1GB),
                ($totalwb/1TB),
                ($totalwb/$traversews),
                ($dwb/1GB),
                ($dwb/$csvwb)))

        } while ($traversews -gt $totalwb)
    }
    finally
    {
        # Halt the warmup (throw/normal completion)
        Set-FleetPause
    }

    $td = (Get-Date) - $t0
    LogOutput "WRITE WARMUP COMPLETE: took $(TimespanToString $td)"
}

function Start-FleetReadCacheWarmup
{
    [CmdletBinding()]
    param(
        [Parameter()]
        [string]
        $Cluster = '.',

        [Parameter(Mandatory = $true)]
        [xml]
        $ProfileXml,

        [Parameter(Mandatory = $true)]
        [uint64]
        $VMWorkingSetSize
        )

    $sess = New-CimSession -ComputerName $Cluster
    $s2d = Get-ClusterS2D -CimSession $sess
    $nodes = Get-ClusterNode -Cluster $Cluster |? State -eq Up

    # Cache disabled is probably an unexpected condition, but also requires no warmup.
    if ($s2d.CacheState -ne 'Enabled')
    {
        LogOutput "READ CACHE WARMUP: cache not enabled, no warmup to perform"
        return
    }

    # Get read workingset of the profile - if there is none, there is no warmup.
    # More than one named target is unsupported for now.
    $readWorkingSet = GetFleetProfileFootprint -ProfileXml $ProfileXml -Read
    if ($readWorkingSet.Count -eq 0)
    {
        LogOutput "READ CACHE WARMUP: profile does not define a read workingset, no warmup to perform"
        return
    }
    if ($readWorkingSet.Count -gt 1)
    {
        throw "Read Cache Warmup does not currently support more than one target/profile ($($readWorkingset.Count) found)"
    }

    $pool = @(Get-StorageSubsystem -CimSession $sess |? AutomaticClusteringEnabled | Get-StoragePool |? IsPrimordial -eq $false)
    $poold = @($pool | Get-PhysicalDisk -CimSession $sess)

    # Compare capacity devices against cache mode - any operating with read cache require warmup
    $capg = @($poold |? Usage -ne Journal | Group-Object -NoElement -Property MediaType)
    $cached = @($poold |? Usage -eq Journal)

    if ($capg.Count)
    {
        $reasons = 0
        foreach ($type in $capg.Name)
        {
            switch ($type)
            {
                'HDD' { if ($s2d.CacheModeHDD -ne 'WriteOnly') { $reasons += 1 }}
                'SSD' { if ($s2d.CacheModeSSD -ne 'WriteOnly') { $reasons += 1 }}
            }
        }

        if ($reasons -eq 0)
        {
            LogOutput "READ CACHE WARMUP: capacity devices are not caching reads, no warmup to perform"
            return
        }
    }
    else
    {
        return
    }

    if ($cached.Count -eq 0)
    {
        throw 'No cache devices found'
    }

    $vm = @(Get-FleetVM -Cluster $Cluster |? State -eq Ok)
    if ($vm.Count -eq 0)
    {
        throw 'No running VMs found'
    }

    # Now reason about total cache capacity and the profile's read workingset footprint across the running VMs.
    #
    # Produce a warmup profile covering up to 60% of the cache on that footprint. In cache rich environments
    # this may span the entire fleet disk capacity. Example:
    #
    #   * 10 TiB cache capacity                        -> 6 TiB potential warmup
    #   * 48 VMs                                       -> ~128 GiB potential warmup/VM
    #   * data disk of 512 GiB
    #   * profile's read IO base of 5 GiB -> 100% span -> 507GiB read workingset
    #   * => generate warmup profile with IO base of 5 GiB -> max of 133 GiB (128 GiB total warmup)

    $warmPct = 0.6
    $cacheCapacity = ($cached | Measure-Object -Property AllocatedSize -Sum).Sum
    $cacheCapacityVM = $warmPct * $cacheCapacity / $vm.Count

    LogOutput "READ CACHE WARMUP: START"
    LogOutput -IsVb ("$($cached.Count) cache devices totaling {0:0.00} TiB capacity" -f ($cacheCapacity/1TB))
    LogOutput -IsVb ("$($vm.Count) VMs => {0:0.00} GiB cache available per VM (at {1:P1} target of total)" -f
        (($cacheCapacityVM/1GB),
         $warmPct))

    $readBase = $readWorkingSet.Values.BaseOffset
    $readMax = $readWorkingSet.Values.MaxOffset
    if ($readMax -eq 0)
    {
        $readMax = $VMWorkingSetSize
    }

    LogOutput -IsVb ("Profile read range: [{0:0.00}GiB - {1:0.00}GiB] ({2:0.00}GiB total) of total {3:0.00}GiB VM workingset" -f
        (($readBase/1GB),
         ($readMax/1GB),
         (($readMax - $readBase)/1GB),
         ($VMWorkingSetSize/1GB)))

    if (($readMax - $readBase) -gt $cacheCapacityVM)
    {
        $readMax = $readBase + $cacheCapacityVM
        LogOutput -IsVb ("Limit to warmable:  [{0:0.00}GiB - {1:0.00}GiB] ({2:0.00}GiB total)" -f
            (($readBase/1GB),
             ($readMax/1GB),
             (($readMax - $readBase)/1GB)))
    }
    else
    {
        LogOutput -IsVb "Full range warmable"
    }

    #
    # Now warm up. Build read profile with two threads, each thread warming half of the target: one on even numbered
    # blocks and one on odd via one thread starting at ThreadStride 1/2 way around the target + blocksize.
    # This sidesteps sequential IO detection. Blocksize must be small enough to meet cache criteria even when L1
    # age is under threshold goal (just > ~1 day).
    #
    # Time is a placeholder; in practice, the run will be failed if we loop the workingset more times than expected
    # without converging. An explicitly indefinite runtime would be preferable.
    #

    $t0 = $null
    $warmupRun = 3600 * 24 * 7
    $blockSize = 56KB

    # Now align the base/max to our warmup block size. The difference between this and the actual range is not material
    # at the scale of GiB of workingset; if this assumption does not hold it could be reasonable to autoscale the blocksize
    # to better fit the (assumedly very small) workingset size. Base up, max down.

    $readBaseAligned = $readBase + $blockSize - 1
    $readBaseAligned -= $readBaseAligned % $blockSize
    $readMaxAligned = $readMax - ($readMax % $blockSize)

    $threadStride = $blockSize + $blockSize*([uint64](($readMaxAligned - $readBaseAligned)/$blockSize))/2

    # Too small?
    if ($readMaxAligned -le $readBaseAligned -or
        $threadStride -le $readBaseAligned -or
        $threadStride -ge $readMaxAligned)
    {
        LogOutput "READ CACHE WARMUP COMPLETE: minimal workingset"
        return
    }

    $dynamicProfile = Get-FleetProfileXml -Name Peak -BaseOffset $readBaseAligned -MaxOffset $readMaxAligned -ThreadsPerTarget 2 -WriteRatio 0 -BlockSize $blockSize -Alignment ($blockSize*2) -ThreadStride $threadStride -RequestCount 8 -Sequential -Duration $warmupRun

    try
    {
        # Trigger temporary change to cache on first read miss, avoiding time which might be
        # required to pass the cache aging and multiple-hit requirements, among others.
        # Sink warnings (2019 does not support this).
        SetCacheBehavior -Cluster $Cluster -PopulateOnFirstMiss 3>$null

        $t0 = Get-Date
        Start-FleetRun -ProfileXml $dynamicProfile -Async

        $pc = @('\Cluster Storage Hybrid Disks(_Total)\Cache Hit Reads/sec',
                '\Cluster Storage Hybrid Disks(_Total)\Cache Miss Reads/sec',
                '\Cluster Storage Hybrid Disks(_Total)\Cache Populate Bytes/sec',
                '\Cluster Storage Hybrid Disks(_Total)\Disk Reads/sec',
                '\Cluster Storage Hybrid Disks(_Total)\Disk Read Bytes/sec'
                )

        $sampleInterval = 15

        # Track total disk read bytes to enforce minimum traversal of the working set
        # after reaching potential completion criteria.
        $totalws = $vm.Count * ($readMax - $readBase)
        $pendingCompletion = $false
        $completeMsg = $null

        $totaldrb = [uint64] 0
        $completedrb = [uint64] 0
        $anyWarmup = $false

        # AtComp/MinMax workingset traversals
        #
        #   * make AtComp traverses once completion condition triggers
        #   * make at least Min traverses if any warmup begins (any step sees non-completion)
        #   * make at most Max traverses, then flag inability to complete
        #
        # Note AtComp takes precedence over Min if completion triggers on the first step. This
        # allows fast(est) verification of an already warmed system.

        $traverseAtComp = 1.2 * $totalws
        $traverseMin = 2.0 * $totalws
        $traverseMax = 20.0 * $totalws

        LogOutput ("READ CACHE WARMUP: $($vm.Count) VMs @ {0:0.00}GiB/VM => {1:0.00}TiB total workingset" -f (
            (($readMax - $readBase)/1GB),
            ($totalws/1TB)))

        do
        {
            # Aggregate samples across the cluster - note that all counters are currently unique
            # by counter name, just use that.
            $samples = Get-Counter $pc -ComputerName $nodes -SampleInterval $sampleInterval
            $samplesAgg = @{}
            $samples.CounterSamples |% {

                # Counter name is fifth element when split by seperator: \\machine\set(instance)\ctr
                $ctr = ($_.Path -split '\\')[4]
                if ($samplesAgg.ContainsKey($ctr))
                {
                    $samplesAgg.$ctr += $_.CookedValue
                }
                else
                {
                    $samplesAgg.$ctr = $_.CookedValue
                }
            }

            $cpb = $samplesAgg."Cache Populate Bytes/sec"
            $chr = $samplesAgg."Cache Hit Reads/sec"
            $cmr = $samplesAgg."Cache Miss Reads/sec"
            $dr = $samplesAgg."Disk Reads/sec"
            $drbt = [uint64] ($samplesAgg."Disk Read Bytes/sec" * $sampleInterval)

            if ($pendingCompletion)
            {
                $completedrb += $drbt
            }
            $totaldrb += $drbt
            $wsMsg = $null

            # Miss rate hitting near actual zero?
            if ([int][math]::Floor($cmr) -eq 0)
            {
                if (-not $pendingCompletion)
                {
                    LogOutput "PENDING COMPLETION: reached zero cache miss rate"
                    $pendingCompletion = $true
                }
                elseif ($completedrb -gt $traverseAtComp -and
                        ($anyWarmup -eq $false -or $totaldrb -gt $traverseMin))
                {
                    $completeMsg = ("reached zero cache miss rate at {0:0.00}TiB ws traversal" -f (
                        ($totaldrb/1TB)))
                    break
                }
            }
            # Sustaining hit:miss ratio > 100 for an entire traversal
            elseif ($chr/$cmr -gt 100)
            {
                if (-not $pendingCompletion)
                {
                    LogOutput "PENDING COMPLETION: hit:miss ratio > 100"
                    $pendingCompletion = $true
                }
                elseif ($completedrb -gt $traverseAtComp -and
                        ($anyWarmup -eq $false -or $totaldrb -gt $traverseMin))
                {
                    $completeMsg = ("hit:miss ratio > 100 at {0:0.00}TiB ws traversal" -f (
                        ($totaldrb/1TB)))
                    break
                }
            }
            elseif ($pendingCompletion)
            {
                # Not meeting criteria. Reset fuse.
                $completedrb = [uint64] 0
                $pendingCompletion = $false
                $wsMsg = ', resetting completion checks due to high miss rate'

                $anyWarmup = $true
            }

            # Inform workingset state
            if ($null -eq $wsMsg)
            {
                if ($pendingCompletion)
                {
                    $m = $null
                    $toGo = 0
                    $compToGo = 0
                    $minToGo = 0
                    if ($traverseAtComp -ge $completedrb) { $compToGo = $traverseAtComp - $completedrb }
                    if ($traverseMin    -ge $totaldrb)    { $minToGo = $traverseMin - $totaldrb }

                    # Per completion condition, min must be satisfied if any warmup condition occured.
                    # Note whether this is in play or not for diagnostics (fast completion).
                    if ($anyWarmup)
                    {
                        if ($minToGo -gt $compToGo) { $toGo = $minToGo } else { $toGo = $compToGo }
                    }
                    else
                    {
                        $toGo = $compToGo
                        $m = ' (minimal warmup)'
                    }
                    $wsMsg = (", {0:0.00}TiB to complete ws traversal$m" -f (($toGo)/1TB))
                }
                else
                {
                    $wsMsg = (', {0:0.00}TiB ws traversed' -f ($totaldrb/1TB))
                }
            }

            LogOutput ("Read IOPS: hit {0:N0} miss {1:N0} => {2:N0} total (populate @ {3:0.0} MiB/s$wsMsg)" -f
                ($chr, $cmr, $dr, ($cpb/1MB)))

            # If warmup has traversed the maximum and completion is not triggered, give up - something must have
            # gone wrong with the cachable workingset estimation.
            if ($totaldrb -gt $traverseMax -and -not $pendingCompletion)
            {
                throw "FAIL: read cache warmup has not converged"
            }

        } while ($true)
    }
    finally
    {
        # Halt the warmup (throw/normal completion)
        Set-FleetPause

        # Clear cache behavior change
        # Sink warnings (2019 does not support this).
        SetCacheBehavior -Cluster $Cluster -PopulateOnFirstMiss:$false 3>$null
    }

    $td = (Get-Date) - $t0
    LogOutput "READ CACHE WARMUP COMPLETE: $completeMsg, took $(TimespanToString $td)"
}

function GetIndexOfSample
{
    [CmdletBinding()]
    param(
        [Parameter()]
        [Microsoft.PowerShell.Commands.GetCounter.PerformanceCounterSampleSet[]]
        $pc,

        [Parameter()]
        [int]
        $Second
    )

    # Return the index of the element in the counter list which covers the indicated
    # second-hand mark. E.g., if we have samples at 0, 5, 10, 15 and 20s and are asked
    # to find 12s, we return 2 (the index for 10).
    #
    # This is used to adapt to jittery sample gathering, which can vary significantly
    # from the intended sample interval.

    $i = 1
    for (; $i -lt $pc.Count; ++$i)
    {
        $d = $pc[$i].Timestamp - $pc[0].Timestamp
        if ($d.TotalSeconds -gt $Second)
        {
            return $i
        }
    }

    return $i - 1
}

function Get-PerformanceCounter
{
    [CmdletBinding()]
    param(
        [Parameter(
            Mandatory = $true
            )]
        [System.IO.FileInfo]
        $Blg,

        [Parameter(
            Mandatory = $true
        )]
        [int]
        $Warmup,

        [Parameter(
            Mandatory = $true
        )]
        [int]
        $Duration,

        [Parameter()]
        [switch]
        $SumProduct,

        [Parameter(
            Mandatory = $true
            )]
        [string[]]
        $Ctrs
    )

    $pc = @(Import-Counter -Path $Blg -Counter ($Ctrs |% { "\\*\$_" }) -ErrorVariable e)

    # PDH_INVALID_DATA = c0000bc6
    # If we see this on countersamples ($pc[nnn].CounterSamples[mmm].Status) it may be reasonable to retry the run a limited
    # number of times. In order to address this, will need module-scoped ErrorActionPreference = Stop to avoid incrementalist
    # modification like Measure-FleetCoreWorkload currently does. And this becomes a try/catch/rethrow as opposed to assuming
    # EAP = Continue as it does now.
    if ($e.Count -ne 0)
    {
        Write-Error "error $Blg : $Ctrs"
        $e
        return $null
    }

    # Extract measured duration (exclude warmup)
    $t0 = GetIndexOfSample $pc $Warmup
    $t1 = GetIndexOfSample $pc ($Warmup + $Duration)

    # LogOutput -IsVb "counter bounds: $Blg warm $Warm d $d indices $t0 - $t1 $('{0:N2}' -f ($pc[$t1].Timestamp - $pc[$t0].Timestamp).TotalSeconds)"

    # !sumproduct: return average of individual counters, multiple values
    # sumproduct : return average of sumproduct of counters, single value
    #
    # The result of each branch is that we have an list of lists (one or more)
    # which are then individually averaged into the output. In matrix terms,
    # this is transposing the list from time-major order (the samples) to
    # counter-major order.

    if ($SumProduct)
    {
        $vals = [object[]]::new(1)

        foreach ($s in $pc[$t0 .. $t1])
        {
            $x = 1
            $s.CounterSamples.CookedValue |% { $x *= $_ }
            $vals[0] += ,$x
        }
    }
    else
    {
        $nctrs = $pc[0].CounterSamples.Count
        $vals = [object[]]::new($nctrs)

        foreach ($s in $pc[$t0 .. $t1])
        {
            for ($i = 0; $i -lt $nctrs; ++$i)
            {
                $vals[$i] += ,$s.CounterSamples[$i].CookedValue
            }
        }
    }

    # Average each counter strip (sumproduct or individual) to output
    $vals |% {

        ($_ | Measure-Object -Average).Average
    }
}

function ItemToDecimal
{
    [CmdletBinding()]
    param(
        [Parameter()]
        [object]
        $item
    )

    if ($null -ne $item)
    {
        return [decimal] $item."#text"
    }
    else
    {
        return [decimal] 0
    }
}

# Post-process DISKSPD results per VM
function ReduceDISKSPDResultToRowResult
{
    [CmdletBinding()]
    param(
        [Parameter(
            ValueFromPipeline = $true,
            Mandatory = $true
            )]
        [System.IO.FileInfo]
        $ResultFile
    )

    # Row object is a collection of NoteProperty constructed by parsing various properties
    # from the result XML onto a new, flat object.
    #
    # Latency: Read/Write @ Average, Median/90/99th ptile
    # IOPS
    # Result filename

    begin {
        $rw = @("Read", "Write")
        $ms = "Milliseconds"
        $av = "Average"

        $badFile = @()
    }

    process {

        try {
            $x = [xml](Get-Content $ResultFile)
        }
        catch
        {
            $badFile += $ResultFile
        }

        # Use ordered table to produce a predictable column layout if exported.
        # Result File + Average IOPS

        $props = [ordered] @{ Result = $ResultFile.BaseName }

        # Lack of target result elements indicates execution failure; TBD reporting.
        $timeSpan = $x.SelectNodes('Results/TimeSpan')
        $target = $x.SelectNodes('Results/TimeSpan/Thread/Target')
        if ($target.Count)
        {
            $rio =($target | Measure-Object -Sum ReadCount).Sum
            $wio =($target | Measure-Object -Sum WriteCount).Sum
            $rb =($target | Measure-Object -Sum ReadBytes).Sum
            $wb =($target | Measure-Object -Sum WriteBytes).Sum

            $totalTime = ($timeSpan | Measure-Object -Sum TestTimeSeconds).Sum

            $props["IOPS"] = [decimal] (($rio + $wio) / $totalTime)
            $props["AverageReadIOPS"] = [decimal] ($rio / $totalTime)
            $props["AverageWriteIOPS"] = [decimal] ($wio / $totalTime)
            $props["AverageReadBW"] = [decimal] ($rb / $totalTime)
            $props["AverageWriteBW"] = [decimal] ($wb / $totalTime)
        }
        else
        {
            $props["IOPS"] = [decimal] 0
            $props["AverageReadIOPS"] = [decimal] 0
            $props["AverageWriteIOPS"] = [decimal] 0
            $props["AverageReadBW"] = [decimal] 0
            $props["AverageWriteBW"] = [decimal] 0
        }

        # Latencies, Average and PTile
        foreach ($type in $rw)
        {
            foreach ($ptile in @(50, 90, 99))
            {
                $tile = $x.Results.TimeSpan.Latency.Bucket[$ptile]

                # ReadMilliseconds ... per ptile bucket
                $prop = $type + $ms
                $props[$prop + $ptile] = ItemToDecimal $tile.Item($prop)
            }

            # AverageReadMillisconds
            # Encode as Average|type, omit $ms
            $prop = $av + $type + $ms
            $props[$prop] = ItemToDecimal $x.Results.TimeSpan.Latency.Item($prop)
        }

        [PsCustomObject] $props
    }

    end {

        if ($badFile.Count)
        {
            throw "Invalid XML results, cannot post process: $($badFile.Name -join ', ')"
        }
    }
}
function ReduceRowResultToSummary
{
    [CmdletBinding()]
    param(
        [Parameter(
        ValueFromPipeline = $true,
            Mandatory = $true
            )]
        [System.Object]
        $RowObject
    )

    # Row object is a collection of NoteProperty.
    # Single summary property bag is composed of each decimal property reduced to aggregate by type.
    #
    #   * *IOPS are summed
    #   * Average[Type]Milliseconds are SumProduct averaged with Average[Type]IOPS
    #   * others are averaged
    #
    # This does presume column name schema ReduceDISKSPDResultToRowResult, which could be better
    # expressed dynamically.

    begin {
        $rows = @()
        $cols = @()

        $colAverage = @()
        $colSumProduct = @()
        $colSum = @()
    }

    process {
        # Get column schema (all decimal properties)
        if ($cols.Count -eq 0)
        {
            $cols = @(@($RowObject | Get-Member |? MemberType -eq NoteProperty |? Definition -like decimal*).Name)

            $colAverage = @($cols -notlike "*IOPS" -notlike "*BW" -notlike "Average*Milliseconds")
            $colSumProduct = @($cols -like "Average*Milliseconds")
            $colSum = @($cols -like "*IOPS")
            $colSum += @($cols -like "*BW")
        }

        $rows += $RowObject
    }

    end {
        # Build property bag, simple averages/sums first.
        $props = @{}
        $rows | Measure-Object -Property $colAverage -Average |% { $props[$_.Property] = $_.Average }
        $rows | Measure-Object -Property $colSum -Sum         |% { $props[$_.Property] = $_.Sum }

        # The product column is always a sum aggregate; note aggregate sum is already available in the
        # property bag. Compute running sumproduct and divide the aggregate to produce the average.
        foreach ($col in $colSumProduct)
        {
            # Average[Type]Milliseconds * Average[Type]IOPS
            $pcol = $col -replace "Milliseconds","IOPS"
            $sumproduct = ($rows |% { $_.$col * $_.$pcol } | Measure-Object -Sum).Sum

            # Handle hard zeros (e.g., no reads, no writes)
            if ($props[$pcol] -ne 0)
            {
                $props[$col] = $sumproduct / $props[$pcol]
            }
            else
            {
                $props[$col] = 0
            }
        }

        $props
    }
}

function ProcessDISKSPDResult
{
    [CmdletBinding()]
    param(
        [Parameter(
            Mandatory = $true
            )]
        [System.IO.DirectoryInfo]
        $ResultDirectory,

        [Parameter()]
        [string]
        $AddSpec = "",

        [Parameter()]
        [string]
        $PerVMResultFile = "",

        [Parameter(
            Mandatory = $true
        )]
        [int]
        $Warmup,

        [Parameter(
            Mandatory = $true
        )]
        [int]
        $Duration
    )

    # common parameter set for counter interval
    $intervalParam = @{}
    CopyKeyIf $PSBoundParameters $intervalParam 'Warmup'
    CopyKeyIf $PSBoundParameters $intervalParam 'Duration'

    # Patterns for perfctr and VM result files
    $perfctrPat = (Join-Path $ResultDirectory "perfctr")
    if ($PSBoundParameters.ContainsKey('AddSpec')) { $perfctrPat += "-$AddSpec"}
    $perfctrPat += "-*.blg"

    $resultPat = (Join-Path $ResultDirectory "result")
    if ($PSBoundParameters.ContainsKey('AddSpec')) { $resultPat += "-$AddSpec"}
    $resultPat += "-*.xml"

    # Process a directory collecting DISKSPD results + Performance Counters to a property bag

    # Row reduction - table of per-vm results, optionally dropped to TSV
    $row = @(Get-ChildItem $resultPat | ReduceDISKSPDResultToRowResult)

    if ($row.Count -eq 0)
    {
        Write-Error "No results found in directory: $ResultDirectory"
        return
    }

    if ($PerVMResultFile.Length)
    {
        $row | Export-Csv -NoTypeInformation -Delimiter "`t" -Path (Join-Path $ResultDirectory $PerVMResultFile)
    }

    #
    # Now summarize per-vm rows to property bag and add performance counter properties for final return
    #

    $props = $row | ReduceRowResultToSummary

    # get central seconds for the performance counters

    # get average cpu utilization for each node
    # get average of all nodes

    $avcpu = $(Get-ChildItem $perfctrPat |% {

        # SumProduct of total run time and processor performance is the normalized total CPU utilization
        # Note each ctr% is in the range (decimal) 0-100, so we must remove the extra factor of 100 to
        # wind up with a value 0-100
        ($cpu) = Get-PerformanceCounter @intervalParam -Blg $_ -SumProduct -Ctrs '\Hyper-V Hypervisor Logical Processor(_Total)\% Total Run Time','\Processor Information(_Total)\% Processor Performance'
        $cpu/100

    } | Measure-Object -Average).Average

    # Get average CSV data for central seconds, all nodes
    # Note we must SumProduct average latency and iops per node to get total latency
    # and then divide by total iops to get whole-cluster average.

    $t_csvr = 0
    $t_csvw = 0
    $t_csvrb = 0
    $t_csvwb = 0
    $t_csvrlat = 0
    $t_csvwlat = 0

    Get-ChildItem $perfctrPat |% {

        # units of average total seconds of latency/second
        ($csvrlat) = Get-PerformanceCounter @intervalParam -Blg $_ -SumProduct -Ctrs '\Cluster CSVFS(_Total)\avg. sec/read','\Cluster CSVFS(_Total)\reads/sec'
        ($csvwlat) = Get-PerformanceCounter @intervalParam -Blg $_ -SumProduct -Ctrs '\Cluster CSVFS(_Total)\avg. sec/write','\Cluster CSVFS(_Total)\writes/sec'

        # average iops
        ($csvr, $csvw, $csvrb, $csvwb) = Get-PerformanceCounter @intervalParam -Blg $_ -Ctrs '\Cluster CSVFS(_Total)\reads/sec','\Cluster CSVFS(_Total)\writes/sec','\Cluster CSVFS(_Total)\read bytes/sec','\Cluster CSVFS(_Total)\write bytes/sec'

        # LogOutput -IsVb ("$($_.BaseName) : aggregate csv read latency/s {0:N3} csv write latency/s {1:N3} csv read/s {2:N0} csv write/s {3:N0}" -f $csvrlat, $csvwlat, $csvr, $csvw)

        # aggregate totals across cluster: iops and aggregate latency/second
        $t_csvr += $csvr
        $t_csvw += $csvw
        $t_csvrb += $csvrb
        $t_csvwb += $csvwb
        $t_csvrlat += $csvrlat
        $t_csvwlat += $csvwlat
    }

    # convert total latency to average latency/io
    if ($t_csvr -ne 0)
    {
        $t_csvrlat /= $t_csvr
    }
    if ($t_csvw -ne 0)
    {
        $t_csvwlat /= $t_csvw
    }

    #
    # Add to bag and return
    # Normalize latency to milliseconds for consistency with DISKSPD
    #
    $props["AverageCPU"] = $avcpu
    $props["AverageCSVReadMilliseconds"] = $t_csvrlat * 1000
    $props["AverageCSVWriteMilliseconds"] = $t_csvwlat * 1000
    $props["AverageCSVReadIOPS"] = $t_csvr
    $props["AverageCSVWriteIOPS"] = $t_csvw
    $props["AverageCSVReadBW"] = $t_csvrb
    $props["AverageCSVWriteBW"] = $t_csvwb
    $props["AverageCSVIOPS"] = $t_csvr + $t_csvw

    return $props
}

function ConvertToOrderedObject
{
    [CmdletBinding(DefaultParameterSetName = "Properties")]
    param(
        [Parameter(
            ParameterSetName = "Properties",
            Mandatory = $true
        )]
        [hashtable]
        $Properties,

        [Parameter(
            ParameterSetName = "InputObject",
            Mandatory = $true
        )]
        [PSCustomObject]
        $InputObject,

        [Parameter()]
        [string[]]
        $LeftColumn
    )

    # This wraps conversion of a property bag or a generic custom object so that
    # the enumeration order of the properties on the resulting new custom object is
    # defined for later operations (Export-CSV in particular).
    #
    # Insertion order to the ordered hashtable defines the result.

    $oh = [ordered] @{}

    switch ($PSCmdlet.ParameterSetName)
    {
        "Properties"
        {
            # specific columns in defined order ("left" in csv column sense)
            foreach ($k in $LeftColumn)
            {
                $oh[$k] = $Properties[$k]
            }

            # remainder lexically
            foreach ($k in $Properties.Keys | Sort-Object)
            {
                if (-not $oh.Contains($k))
                {
                    $oh[$k] = $Properties[$k]
                }
            }
            break
        }

        "InputObject"
        {
            # left columns
            foreach ($k in $LeftColumn)
            {
                $oh[$k] = $InputObject.$k
            }

            # all note properties
            foreach ($k in ($InputObject | Get-Member |? MemberType -eq NoteProperty).Name | Sort-Object)
            {
                if (-not $oh.Contains($k))
                {
                    $oh[$k] = $InputObject.$k
                }
            }
            break
        }
    }

    [PSCustomObject] $oh
}

function IsWithin(
    [double]
    $Base,

    [double]
    $Target,

    [ValidateRange(1,50)]
    [double]
    $Percentage
    )
{
    return ([math]::abs($Target/$Base - 1) -le ($Percentage/100))
}

function IsAbove(
    [double]
    $Base,

    [double]
    $Target,

    [ValidateRange(1,50)]
    [double]
    $Percentage
    )
{
    return (($Target/$Base - 1) -gt ($Percentage/100))
}

function Measure-FleetCoreWorkload
{
    [CmdletBinding()]
    param(
        [Parameter()]
        [string]
        $Cluster = ".",

        [Parameter()]
        [string]
        $ResultDirectory,

        [Parameter()]
        [string]
        $LogFile = "coreworkload.log",

        # Note: passing of an [ordered] table is not possible
        # with a type specification. Type is manually validated,
        # allowing for a plain unordered hashtable as well.
        [Parameter()]
        $KeyColumn,

        [Parameter()]
        [uint32]
        $Warmup = 300,

        [Parameter()]
        [switch]
        $SkipSddcDiagInfo,

        [Parameter()]
        [switch]
        $UseStorageQos
    )

    # Remoting parametersets
    $clusterParam = @{}
    CopyKeyIf $PSBoundParameters $clusterParam 'Cluster'

    $runParam = @{}
    CopyKeyIf $PSBoundParameters $runParam 'Cluster'
    CopyKeyIf $PSBoundParameters $runParam 'UseStorageQos'

    # If SddcDiagInfo is not skipped, absence of the command is an error
    if (-not $SkipSddcDiagInfo -and -not (Get-Command Get-SddcDiagnosticInfo))
    {
        throw "The PrivateCloud.DiagnosticInfo module must be present in order for SddcDiagnosticInfo to be gathered (Get-SddcDiagnosticInfo). Please install the module and retry, or use -SkipSddcDiagInfo to omit."
    }

    $resultPath = Get-FleetPath -PathType Result @clusterParam

    # place result files in result directory unless a qualified path is provided
    if (-not $LogFile.Contains('\')) {
        $LogFile = Join-Path $resultPath $LogFile
    }

    $myKeyColumn = $null
    if ($PSBoundParameters.ContainsKey('KeyColumn'))
    {
        # Clone KeyColumn to allow non-modifying insert of ordered workload name / metadata columns
        $myKeyColumn = [ordered] @{}
        if ($KeyColumn.GetType().Name -eq 'OrderedDictionary')
        {
            foreach ($k in $KeyColumn.Keys)
            {
                $myKeyColumn[$k] = $KeyColumn[$k]
            }
        }
        elseif ($KeyColumn.GetType().Name -eq 'Hashtable')
        {
            # provide lexical order of unordered input columns
            foreach ($k in $KeyColumn.Keys | Sort-Object)
            {
                $myKeyColumn[$k] = $KeyColumn[$k]
            }
        }
        else
        {
            Write-Error "KeyColumn must be a regular unordered or [ordered] hashtable."
            return
        }
    }
    else
    {
        $myKeyColumn = @{}
    }

    $runParam.KeyColumn = $myKeyColumn

    # Inner loop warmup is min of specified or 30s. This is the time spent warming the
    # search iterations during workload sweeps after full warmup has occured.
    if ($Warmup -lt 30)
    {
        $SearchWarmup = $Warmup
    }
    else
    {
        $SearchWarmup = 30
    }

    #
    # Apply values first to keycolumns and then for running configuration (iff required for run - see runners)
    #

    function ApplyVMTemplate
    {
        param(
            [string]
            $ComputeTemplate,

            [double]
            $VMPercent = 100,

            [switch]
            $KeyColumn
        )

        if ($KeyColumn)
        {
            $spec = Get-FleetComputeTemplate -ComputeTemplate $ComputeTemplate
            $myKeyColumn.ComputeTemplate = $ComputeTemplate
            $myKeyColumn.ProcessorCount = $spec.ProcessorCount
            $myKeyColumn.MemoryStartupBytes = $spec.MemoryStartupBytes
            $myKeyColumn.FleetVMPercent = $VMPercent
        }
        else
        { Set-Fleet @clusterParam -ComputeTemplate $myKeyColumn.ComputeTemplate }
    }

    function ApplyVMDataDisk
    {
        param(
            [switch]
            $KeyColumn
        )

        if ($KeyColumn)
        {
            $dataDiskSize = Get-FleetDataDiskEstimate @clusterParam -CachePercent 200 -CapacityPercent 30 -VMPercent $myKeyColumn.FleetVMPercent -Verbose

            # Opt to use internal load file if disk size would be smaller.
            # This can happen in storage limited/compute rich builds.
            if ($dataDiskSize -le 10GB)
            {
                $dataDiskSize = 0
            }
            $myKeyColumn.DataDiskBytes = $dataDiskSize
        }
        else
        {
            $d = Set-Fleet -DataDiskSize $myKeyColumn.DataDiskBytes -Running

            # If data disks were newly created, execute write warmup.
            if ($null -ne $d)
            {
                Start-FleetWriteWarmup -VMWorkingSetSize $myKeyColumn.DataDiskBytes
            }
        }
    }

    function StartVM
    {
        if ($state.ContainsKey('startVM')) { return }
        $state.startVM = $true

        AlignVM
        ApplyVMTemplate $myKeyColumn.ComputeTemplate $myKeyColumn.FleetVMPercent
        Start-Fleet @clusterParam -Percent $myKeyColumn.FleetVMPercent
        ApplyVMDataDisk
    }

    function AlignVM
    {
        param(
            [switch]
            $IfStarted
        )

        if ($IfStarted -and -not $state.ContainsKey('startVM')) { return  }

        Move-Fleet @clusterParam -DistributeVMPercent (100 - $myKeyColumn.VMAlignmentPct) -VMPercent $myKeyColumn.FleetVMPercent
        Repair-Fleet @clusterParam
    }

    function ReadWarmup
    {
        param(
            [xml]
            $ProfileXml
        )

        if ($myKeyColumn.DataDiskBytes -eq 0) {
            $sz = 10GB
        }
        else
        {
            $sz = $myKeyColumn.DataDiskBytes
        }

        Start-FleetReadCacheWarmup @clusterParam -ProfileXml $ProfileXml -VMWorkingSetSize $sz -Verbose
    }

    #
    # Per VM-configuration runners
    #
    # VM start/move and (re)configuration is delayed until we discover a result which has not been
    # produced yet. This allows for low impact & idempotent restart - for instance, if all
    # results with a given configuration have already been produced. This becomes important
    # since the common case for restart will be that an error was encountered and we want to
    # restart at the existing configuration (after validating, quickly, that all prior are done).
    #
    # Move must be evaluated every time alignment logically changes. Move only needs to happen if
    # VMs are started, however.
    #

    function RunInner1VCPUMatrix
    {
        $once = $true
        foreach ($w in 0, 10, 30)
        {
            $myKeyColumn.Workload = "General4KWriteRatio$w"
            $ProfileXml = Get-FleetProfileXml -Name General -Warmup $Warmup -Duration 60 -Cooldown 30 -WriteRatio $w -ThreadsPerTarget 1 -BlockSize 4KB -Random -RandomRatio 50 -RequestCount 32
            if (-not (Test-FleetResultRun @clusterParam -KeyColumn $myKeyColumn)) {
                StartVM
                if ($once)
                {
                    $once = $false
                    ReadWarmup -ProfileXml $ProfileXml
                }
                Start-FleetResultRun @runParam -ProfileXml $ProfileXml -CpuSweep -SearchWarmup $SearchWarmup
            }
        }

        # Peak with 5GB workingset
        $myKeyColumn.Workload = "Peak4kRead"
        $ProfileXml = Get-FleetProfileXml -Name Peak -Warmup $Warmup -Duration 60 -Cooldown 30 -MaxOffset 5GB -WriteRatio 0 -ThreadsPerTarget 1 -BlockSize 4KB -Random -RequestCount 32
        if (-not (Test-FleetResultRun @clusterParam -KeyColumn $myKeyColumn)) {
            StartVM
            ReadWarmup -ProfileXml $ProfileXml
            Start-FleetResultRun @runParam -ProfileXml $ProfileXml
        }

        $myKeyColumn.Workload = "Vdi"
        $ProfileXml = Get-FleetProfileXml -Name Vdi -Warmup $Warmup -Duration 60 -Cooldown 30
        if (-not (Test-FleetResultRun @clusterParam -KeyColumn $myKeyColumn)) {
            StartVM
            ReadWarmup -ProfileXml $ProfileXml
            Start-FleetResultRun @runParam -ProfileXml $ProfileXml -CpuSweep -SearchWarmup $SearchWarmup -CutoffAverageCPU 40 -CutoffAverageLatencyMs 3
        }
    }

    function RunInner4VCPUMatrix
    {
        $myKeyColumn.Workload = "Sql"
        $ProfileXml = Get-FleetProfileXml -Name Sql -Warmup $Warmup -Duration 60 -Cooldown 30
        if (-not (Test-FleetResultRun @clusterParam -KeyColumn $myKeyColumn)) {
            StartVM
            ReadWarmup -ProfileXml $ProfileXml
            Start-FleetResultRun @runParam -ProfileXml $ProfileXml -CpuSweep -SearchWarmup $SearchWarmup -CutoffAverageCPU 40 -CutoffAverageLatencyMs 3
        }
    }

    try
    {
        $originalEAP = $null
        $originalPowerScheme = $null

        Start-Transcript -Path $LogFile -Append

        $originalEAP = $ErrorActionPreference
        $ErrorActionPreference = [System.Management.Automation.ActionPreference]::Stop

        # Show versioning up top; delay exporting until creation of the final packaging to
        # avoid interference from result sweeps/etc.
        Get-FleetVersion @clusterParam |% {
            "Version: $($_.Component) $($_.Version)"
        }

        $originalPowerScheme = Get-FleetPowerScheme @clusterParam
        Set-FleetPowerScheme @clusterParam -Scheme HighPerformance
        $myKeyColumn.PowerScheme = 'HighPerformance'

        #
        # VM Config @ A1v2
        #

        $state = @{}
        Stop-Fleet @clusterParam
        ApplyVMTemplate -KeyColumn A1v2 100
        ApplyVMDataDisk -KeyColumn

        foreach ($alignment in 100,70)
        {
            $myKeyColumn.VMAlignmentPct = $alignment
            AlignVM -IfStarted
            RunInner1VCPUMatrix
        }

        #
        # VM Config @ A4v2
        #

        $state = @{}
        Stop-Fleet @clusterParam
        ApplyVMTemplate -KeyColumn A4v2 25
        ApplyVMDataDisk -KeyColumn

        foreach ($alignment in 100,70)
        {
            $myKeyColumn.VMAlignmentPct = $alignment
            AlignVM -IfStarted
            RunInner4VCPUMatrix
        }
    }
    finally
    {
        if ($null -ne $originalEAP)         { $ErrorActionPreference = $originalEAP }
        if ($null -ne $originalPowerScheme) { Set-FleetPowerScheme @clusterParam -Scheme $originalPowerScheme }
        Stop-Transcript
    }

    #
    # Add Get-Sddc?
    #

    if (-not $SkipSddcDiagInfo -and -not (Test-Path (Join-Path $resultPath "HealthTest*")))
    {
        LogOutput "Gathering SddcDiagnosticInfo to document running environment"

        # Six hours should be sufficient for all usual purposes, slim down capture size
        Get-SddcDiagnosticInfo @clusterParam -ZipPrefix (Join-Path $resultPath "HealthTest") -HoursOfEvents 6
    }

    #
    # Add additional metadata for the run (versioning)
    #

    Get-FleetVersion | Export-Clixml (Join-Path $resultPath "version.xml")

    #
    # Now create the final packaging
    #

    if ($PSBoundParameters.ContainsKey('ResultDirectory'))
    {
        $finalResult = (Join-Path $ResultDirectory "result-coreworkload-")
    }
    else
    {
        # Note: file in same directory as the result directory (...\collect\result-coreworkload-xyz.zip)
        $finalResult = $resultPath + "-coreworkload-"
    }

    $finalResult += (Get-Date).ToString('yyyyMMdd-HHmm')
    $finalResult += ".zip"

    LogOutput "Compressing final result archive: $finalResult"
    Compress-Archive -Path $resultPath\* -DestinationPath $finalResult -ErrorAction Stop

    #
    # Bounce CSV to workaround very itermittent issue where a latent handle prevents BLG deletion.
    # TBD root cause.
    #

    $null = Get-ClusterSharedVolume @clusterParam |? Name -match $collectVolumeName | Move-ClusterSharedVolume

    LogOutput "Clearing result directory: $resultPath"
    Remove-Item $resultPath\* -Recurse

    LogOutput -ForegroundColor Green "DONE: final result archive $finalResult"
}

function Get-FleetResultLog
{
    [CmdletBinding()]
    param(
        [string]
        $ResultLog,

        # Note: passing of an [ordered] table is not possible
        # with a type specification. Type is manually validated,
        # allowing for a plain unordered hashtable as well.
        [Parameter()]
        $KeyColumn
    )

    function Typeify
    {
        param(
            [Parameter(ValueFromPipeline = $true)]
            $InputObject
        )

        process {

            # Convert all properties which appear to be numeric to actual numerics.
            # Use 64bit integers for lack of a better estimation of a safe default
            # type that will not overflow if/when operated on.
            #
            # Convert all properties which appear to be booleans to actual booleans.

            foreach ($prop in (Get-Member -InputObject $InputObject -MemberType NoteProperty).Name)
            {
                switch -regex ($InputObject.$prop)
                {
                    '^\d+$' {
                        $InputObject.$prop = [uint64] $InputObject.$prop
                    }
                    '^-\d+$' {
                        $InputObject.$prop = [int64] $InputObject.$prop
                    }
                    '^-?\d+\.\d+$' {
                        $InputObject.$prop = [double] $InputObject.$prop
                    }
                    '^True$' {
                        $InputObject.$prop = $true
                    }
                    '^False$' {
                        $InputObject.$prop = $false
                    }
                }
            }

            $InputObject
        }
    }

    if ($PSBoundParameters.ContainsKey('KeyColumn') -and
        (-not ($KeyColumn.GetType().Name -eq 'OrderedDictionary' -or
               $KeyColumn.GetType().Name -eq 'Hashtable')))
    {
        Write-Error "KeyColumn must be a regular unordered or [ordered] hashtable."
        return
    }

    if ($PSBoundParameters.ContainsKey('KeyColumn'))
    {
        Import-Csv $ResultLog -Delimiter "`t" | FilterObject -Filter $KeyColumn | Typeify
    }
    else
    {
        Import-Csv $ResultLog -Delimiter "`t" | Typeify
    }
}

function Test-FleetResultRun
{
    [CmdletBinding()]
    param(
        [Parameter()]
        [string]
        $Cluster = ".",

        [Parameter()]
        [string]
        $ResultLog = "result-log.tsv",

        # Note: passing of an [ordered] table is not possible
        # with a type specification. Type is manually validated,
        # allowing for a plain unordered hashtable as well.
        [Parameter(Mandatory = $true)]
        $KeyColumn
        )

    # Remoting parametersets
    $clusterParam = @{}
    CopyKeyIf $PSBoundParameters $clusterParam 'Cluster'

    if (-not $ResultLog.Contains('\')) {

        # Note we only need the resultpath if not fuilly qualified
        $resultPath = Get-FleetPath -PathType Result @clusterParam
        $ResultLog = Join-Path $resultPath $ResultLog
    }

    if (-not (Test-Path $ResultLog))
    {
        return $false
    }

    $n = 0
    Get-FleetResultLog -ResultLog $ResultLog -KeyColumn $KeyColumn |% { $n += 1 }

    # If any - there may be a ? if we match multiple (incomplete keys?), but there may be utility there.
    return ($n -ne 0)
}

function Start-FleetResultRun
{
    [CmdletBinding(DefaultParameterSetName = 'Default')]
    param(
        [Parameter(ParameterSetName = 'Default')]
        [Parameter(ParameterSetName = 'CpuSweep')]
        [Parameter(ParameterSetName = 'CpuSweepCombinedLatency')]
        [string]
        $Cluster = ".",

        [Parameter(ParameterSetName = 'Default', Mandatory = $true)]
        [Parameter(ParameterSetName = 'CpuSweep', Mandatory = $true)]
        [Parameter(ParameterSetName = 'CpuSweepCombinedLatency', Mandatory = $true)]
        [xml]
        $ProfileXml,

        [Parameter(ParameterSetName = 'Default')]
        [Parameter(ParameterSetName = 'CpuSweep')]
        [Parameter(ParameterSetName = 'CpuSweepCombinedLatency')]
        [string]
        $ResultFile = "result.tsv",

        [Parameter(ParameterSetName = 'Default')]
        [Parameter(ParameterSetName = 'CpuSweep')]
        [Parameter(ParameterSetName = 'CpuSweepCombinedLatency')]
        [string]
        $ResultLog = "result-log.tsv",

        # Note: passing of an [ordered] table is not possible
        # with a type specification. Type is manually validated,
        # allowing for a plain unordered hashtable as well.
        [Parameter(ParameterSetName = 'Default')]
        [Parameter(ParameterSetName = 'CpuSweep')]
        [Parameter(ParameterSetName = 'CpuSweepCombinedLatency')]
        $KeyColumn,

        [Parameter(ParameterSetName = 'Default')]
        [Parameter(ParameterSetName = 'CpuSweep')]
        [Parameter(ParameterSetName = 'CpuSweepCombinedLatency')]
        [string]
        $RunLabel,

        [Parameter(ParameterSetName = 'CpuSweep', Mandatory = $true)]
        [Parameter(ParameterSetName = 'CpuSweepCombinedLatency', Mandatory = $true)]
        [switch]
        $CpuSweep,

        [Parameter(ParameterSetName = 'CpuSweep')]
        [Parameter(ParameterSetName = 'CpuSweepCombinedLatency')]
        [uint32]
        $SearchWarmup = 0,

        [Parameter(ParameterSetName = 'CpuSweep')]
        [Parameter(ParameterSetName = 'CpuSweepCombinedLatency')]
        [switch]
        $UseStorageQos,

        [Parameter(ParameterSetName = 'CpuSweep')]
        [Parameter(ParameterSetName = 'CpuSweepCombinedLatency')]
        [ValidateRange(1,100)]
        [double]
        $CutoffAverageCPU,

        # Latency cutoffs - ranges are arbitrarily chosen to loosely bracket
        # reasonable values : 10us - 1s
        [Parameter(ParameterSetName = 'CpuSweep')]
        [ValidateRange(0.01,1000)]
        [double]
        $CutoffAverageReadLatencyMs,

        [Parameter(ParameterSetName = 'CpuSweep')]
        [ValidateRange(0.01,1000)]
        [double]
        $CutoffAverageWriteLatencyMs,

        [Parameter(ParameterSetName = 'CpuSweepCombinedLatency', Mandatory = $true)]
        [ValidateRange(0.01,1000)]
        [double]
        $CutoffAverageLatencyMs
        )

    # Remoting parametersets
    $clusterParam = @{}
    CopyKeyIf $PSBoundParameters $clusterParam 'Cluster'

    #
    # Validate
    #
    # CpuSweep only applies to a single timespan; we do not currently reason about averaging results
    # through multiple timespans.
    #

    if ($CpuSweep -and -not (IsProfileSingleTimespan -ProfileXml $ProfileXml))
    {
        Write-Error "Profile for a CpuSweep can only contain a single timespan"
        return
    }

    # Produce ordered set of key columns
    $orderedKeyColumn = [ordered] @{}
    if ($PSBoundParameters.ContainsKey('KeyColumn'))
    {
        if ($KeyColumn.GetType().Name -eq 'OrderedDictionary')
        {
            $orderedKeyColumn = $KeyColumn
        }
        elseif ($KeyColumn.GetType().Name -eq 'Hashtable')
        {
            $orderedKeyColumn = [ordered] @{}
            foreach ($k in $KeyColumn.Keys | Sort-Object)
            {
                $orderedKeyColumn[$k] = $KeyColumn[$k]
            }
        }
        else
        {
            Write-Error "KeyColumn must be a regular unordered or [ordered] hashtable."
            return
        }
    }

    # Already complete?
    if ($orderedKeyColumn.Count -and (Test-FleetResultRun @clusterParam -ResultLog $ResultLog -KeyColumn $KeyColumn))
    {
        LogOutput -ForegroundColor Cyan "Run already measured for $($orderedKeyColumn.Keys |% { $_, $orderedKeyColumn[$_] -join '=' })"
        return
    }

    $accessNode = Get-AccessNode @clusterParam
    $cimParam = @{ CimSession = $accessNode }

    $resultPath = Get-FleetPath -PathType Result @clusterParam

    # place result files in result directory unless a qualified path is provided
    if (-not $ResultFile.Contains('\')) {
        $ResultFile = Join-Path $resultPath $ResultFile
    }
    if (-not $ResultLog.Contains('\')) {
        $ResultLog = Join-Path $resultPath $ResultLog
    }

    #
    # Expected counter set for post-processing.
    #

    $pc = @('\Hyper-V Hypervisor Logical Processor(_Total)\% Total Run Time',
            '\Processor Information(_Total)\% Processor Performance',
            '\Cluster CSVFS(_Total)\reads/sec',
            '\Cluster CSVFS(_Total)\avg. sec/read',
            '\Cluster CSVFS(_Total)\writes/sec',
            '\Cluster CSVFS(_Total)\avg. sec/write',
            '\Cluster CSVFS(_Total)\read bytes/sec',
            '\Cluster CSVFS(_Total)\write bytes/sec')

    # Get timing for post processing of data
    $Warmup, $Duration = GetProfileDuration -ProfileXml $ProfileXml -Warmup -Duration

    # Limit qos infill search, done if the next best gap is closer than %age
    $qosWindow = 8

    # Limit qos adaptive upper anchor search, done if the gap closes within this %age of upper cutoff
    $qosAdaptiveWindow = 2

    # Percent variance of result IOPS <=> QoS target which will result in a scale/surge cutoff
    $qosScaleWindow = 1.5

    function GetIterationFiles
    {
        param(
            [string]
            $resultPath
            )

        # per-vm tsv for iteration
        Get-ChildItem $resultPath\* -File -Include result-vm-*.tsv
        # all other, excluding main tsv and log
        Get-ChildItem $resultPath\* -File -Exclude *.tsv,*.log
    }

    # Dynamic scope wrapper for inner loop running the iteration itself
    function DoIteration
    {
        param(
            [int]
            $QoS = 0,

            [switch]
            $IsSearch,

            [switch]
            $IsAdaptive,

            [switch]
            $IsBaseline
        )

        #
        # Measure once. This functions as the short-circuit for the fixed
        # QoS bracketing of the sweep. Baselines are simply that. Non
        # baselines should not consider the baseline, which will be at
        # QoS "zero" (and should not prevent an unconstrained run of a
        # baselined profile).
        #
        # This is also where we decide on the filename labeling for this run
        # (if we make it).
        #

        if ($IsBaseline)
        {
            $prior = @($results |? RunType -eq ([RunType]::Baseline))
            $addspec = "baseline"
        }
        else
        {
            $prior = @($results |? RunType -ne ([RunType]::Baseline) |? QOSperVM -eq $QoS)
            $addspec = "qos$qos"
        }

        if ($prior.Count -eq 1)
        {
            LogOutput "Result already captured"
            return $prior
        }
        elseif ($prior.Count -ne 0)
        {
            throw "Unexpected multiple measurements @ QoS $QoS ($($prior.Count) total) in $ResultFile - please inspect"
        }

        # If this is a sweep and not an initial throughput baseline, apply QoS.
        # Note sweeps can start with a profile that has no throughput constraints
        # as well as ones that do (the latter are "baselines", the former are simply
        # unconstrained).
        if ($CpuSweep -and -not $IsBaseline)
        {
            if ($UseStorageQos)
            {
                Set-StorageQosPolicy -Name SweepQoS -MaximumIops $QoS @cimParam
            }
            else
            {
                $ProfileXml = $ProfileXml | Set-FleetProfile -Throughput $QoS -ThroughputUnit IOPS
            }

            if ($IsSearch -and $SearchWarmup)
            {
                $ProfileXml = $ProfileXml | Set-FleetProfile -Warmup $SearchWarmup
                $Warmup = $SearchWarmup
            }
        }

        if ($VerbosePreference)
        {
            $ProfileXml | Convert-FleetXmlToString |% { LogOutput -IsVb $_ }
        }

        # Use thread autoscaling?
        Start-FleetRun @clusterParam -pc $pc -SampleInterval 1 -ProfileXml $ProfileXml -AddSpec $addspec

        # Get result property bag and add sweep metadata columns, export.
        $props = ProcessDISKSPDResult -ResultDirectory $resultPath -Warmup $Warmup -d $Duration -AddSpec $addspec -PerVMResultFile "result-vm-$addspec.tsv"

        #
        # Gather results for this iteration
        #

        if ($RunLabel.Length)
        {
            $dest = Join-Path $resultPath $RunLabel
            $null = mkdir $dest -ErrorAction SilentlyContinue
            if (-not (Test-Path $dest))
            {
                throw "Could not create run result gather directory $dest"
            }
            GetIterationFiles $resultPath | Move-Item -Destination $dest

            LogOutput Gathered results to $dest
        }

        # Insert key columns as metadata for the result table, ending with our QOS.
        # Note that all results in a given result file MUST share the same key columns.

        foreach ($k in $orderedKeyColumn.Keys)
        {
            $props[$k] = $orderedKeyColumn[$k]
        }

        #
        # Evaluate cutoffs
        #

        $cutoff = [CutoffType]::No

        # Surge is a special case where tooling has malfunctioned/errored in some way to run faster than target
        if ($PSBoundParameters.ContainsKey('QoS')     -and $QoS -ne 0 -and      (IsAbove ($QoS * $vms) $props.IOPS $qosScaleWindow))
        {
            LogOutput -ForegroundColor Red ('CUTOFF SURGE: {0:N0} IOPS is > {1:P1} above target {2:N0} IOPS (QoS {3:N0} from {4:N0} VMs)' -f ($props.IOPS,($qosScaleWindow/100),($QoS*$vms),$QoS,$vms))
            $cutoff = [CutoffType]::Surge
        }

        # Fail to scale - since we checked surge, this will always be slower-than target
        elseif ($PSBoundParameters.ContainsKey('QoS') -and $QoS -ne 0 -and -not (IsWithin ($QoS * $vms) $props.IOPS $qosScaleWindow))
        {
            LogOutput -ForegroundColor Cyan ('CUTOFF SCALE: {0:N0} IOPS is > {1:P1} off of target {2:N0} IOPS (QoS {3:N0} from {4:N0} VMs)' -f ($props.IOPS,($qosScaleWindow/100),($QoS*$vms),$QoS,$vms))
            $cutoff = [CutoffType]::Scale
        }

        # CPU utilizati0on cutoff
        elseif ($CutoffAverageCPU -ne 0 -and $CutoffAverageCPU -lt $props.AverageCPU)
        {
            LogOutput -ForegroundColor Cyan ('CUTOFF CPU: {0:P1} CPU utilization is > {1:P1} cutoff limit' -f (($props.AverageCPU/100),($CutoffAverageCPU/100)))
            $cutoff = [CutoffType]::CPU
        }

        #
        # Single shared latency cutoff
        #

        elseif ($CutoffAverageLatencyMs -ne 0 -and
                $CutoffAverageLatencyMs -lt $props.AverageReadMilliseconds)
        {
            LogOutput -ForegroundColor Cyan ('CUTOFF COMBINED LATENCY: read latency {0:N3}ms > {1:N3}ms cutoff limit' -f ($props.AverageReadMilliseconds,$CutoffAverageLatencyMs))
            $cutoff = [CutoffType]::ReadLatency
        }

        elseif ($CutoffAverageLatencyMs -ne 0 -and
                $CutoffAverageLatencyMs -lt $props.AverageWriteMilliseconds)
        {
            LogOutput -ForegroundColor Cyan ('CUTOFF COMBINED LATENCY: write latency {0:N3}ms > {1:N3}ms cutoff limit' -f ($props.AverageWriteMilliseconds,$CutoffAverageLatencyMs))
            $cutoff = [CutoffType]::WriteLatency
        }

        #
        # Individual latency cutoffs
        #

        elseif ($CutoffAverageReadLatencyMs -ne 0 -and
                $CutoffAverageReadLatencyMs -lt $props.AverageReadMilliseconds)
        {
            LogOutput -ForegroundColor Cyan ('CUTOFF READ LATENCY: read latency {0:N3}ms > {1:N3}ms cutoff limit' -f ($props.AverageReadMilliseconds,$CutoffAverageReadLatencyMs))
            $cutoff = [CutoffType]::ReadLatency
        }

        elseif ($CutoffAverageWriteLatencyMs -ne 0 -and
                $CutoffAverageWriteLatencyMs -lt $props.AverageWriteMilliseconds)
        {
            LogOutput -ForegroundColor Cyan ('CUTOFF WRITE LATENCY: write latency {0:N3}ms > {1:N3}ms cutoff limit' -f ($props.AverageWriteMilliseconds,$CutoffAverageWriteLatencyMs))
            $cutoff = [CutoffType]::WriteLatency
        }

        # Add in metadata columns
        # CAREFUL: [switch] parameters are a distinct object
        if ($IsBaseline.IsPresent)
        {
            $props.RunType = [RunType]::Baseline
        }
        elseif ($IsAdaptive.IsPresent)
        {
            $props.RunType = [RunType]::AnchorSearch
        }
        else
        {
            $props.RunType = [RunType]::Default
        }
        $props.CutoffType = $cutoff
        $props.QOSperVM = $QoS
        $props.VMCount = $vms

        # Add in optional column(s)
        $optColumn = @()

        if ($RunLabel.Length)
        {
            $props.RunLabel = $RunLabel
            $optColumn += 'RunLabel'
        }

        # Add to result file and in-memory table, moving metadata (visuals) to the left. Swallow the index of the addded result.
        $o = ConvertToOrderedObject -Properties $props -LeftColumn ($optColumn + $orderedKeyColumn.Keys + 'RunType' + 'CutoffType' + 'QOSperVM' + 'VMCount' + 'IOPS')
        $o | Export-Csv -NoTypeInformation -Delimiter "`t" -Path $ResultFile -Append -ErrorAction Stop

        $null = $results.Add($o)

        # return current result
        return $o
    }

    function LogToComplete
    {
        # No run label => no log (single result run)
        if ($RunLabel.Length -eq 0) { return }

        $props = @{}

        foreach ($k in $orderedKeyColumn.Keys)
        {
            $props[$k] = $orderedKeyColumn[$k]
        }

        $props.RunLabel = $RunLabel
        $props.Complete = 1

        # RunLabel on the far left
        ConvertToOrderedObject -Properties $props -LeftColumn (@('RunLabel') + $orderedKeyColumn.Keys) |
            Export-Csv -NoTypeInformation -Delimiter "`t" -Path $ResultLog -Append -ErrorAction Stop
    }

    try
    {
        # if using in-stack qos mechanism, make qos policy and apply to vms
        if ($UseStorageQos)
        {
            $qosName = 'SweepQoS'
            Get-StorageQosPolicy -Name $qosName @cimParam -ErrorAction SilentlyContinue | Remove-StorageQosPolicy -Confirm:$false
            $null = New-StorageQosPolicy -Name $qosName -PolicyType Dedicated @cimParam
            Set-FleetQos -Name $qosName @clusterParam
        }
        else
        {
            # clear any applied in-stack qos
            Set-FleetQos @clusterParam
        }

        #
        # Accumulate iteration result objects for fit. Preload from existing data (if any) so that
        # we do not repeat steps. Note that empty key column is a null filter, passing everything.
        #

        $results = [collections.arraylist] @()
        if (Test-Path $ResultFile)
        {
            Get-FleetResultLog -ResultLog $ResultFile -KeyColumn $orderedKeyColumn |% { $null = $results.Add($_) }
        }

        #
        # Use pre-existing iteration's runlabel else generate new guid-based label if none specified.
        #

        if ($results.Count)
        {
            if (Get-Member -InputObject $results[0] -Name RunLabel)
            {
                $RunLabel = $results[0].RunLabel
            }
        }
        elseif (-not $PSBoundParameters.ContainsKey('RunLabel') -and $orderedKeyColumn.Count)
        {
            $RunLabel = [System.Guid]::NewGuid().Guid
        }

        #
        # Log run keys if present
        #

        if ($orderedKeyColumn.Count)
        {
            LogOutput -ForegroundColor Cyan "Starting run for $($orderedKeyColumn.Keys |% { $_, $orderedKeyColumn[$_] -join '=' })"
        }

        #
        # Count the number of online vms in the configuration so we can reason about IOPS -> QoS relationships.
        #

        $vms = @(Get-ClusterGroup @clusterParam |? GroupType -eq VirtualMachine |? Name -like 'vm-*' |? State -eq Online).Count

        #
        # Baseline throughput limited profiles else find unconstrained/default upper anchor.
        # A baselined run can start a cpusweep loop, following up with the adaptive search to find its upper anchor.
        # Unconstrained will start with normal infill.
        #

        $sweepAdaptive = $false
        $doUnconstrained = $false
        $paramUnconstrained = @{ QoS = 0 }

        if (IsProfileThroughputLimited -ProfileXml $ProfileXml)
        {
            $o = DoIteration -IsBaseline
            LogOutput -ForegroundColor Cyan ("RESULT: baseline {0:N0} IOPS @ {1:P1} CPU => {2:N0} IOPS/VM from {3:N0} VMs" -f ($o.IOPS,($o.AverageCPU/100),($o.IOPS/$vms),$vms))

            # If this is a CPU Sweep, set up for the upper anchor search.
            # Single target throughput limited profiles can be run unconstrained (QoS 0) to find the upper anchor.
            # Multi-target throughput limited profiles require the adaptive anchor search.
            #
            # We'll always show the baseline above on restart (may be complete). Then, if a default/infill result
            # has not been logged we know that the anchor search is needed or potentially incomplete and the loop
            # should start on that side. Conversely, if there is a default/infill we know it is complete.
            if ($CpuSweep)
            {
                if(IsProfileSingleTarget -ProfileXml $ProfileXml)
                {
                    # Unconstrained run here is part of the search (speedy warmup - assume baseline run/restarted after)
                    $doUnconstrained = $true
                    $paramUnconstrained.IsSearch = $true
                }
                elseif (@($results |? RunType -eq [RunType]::Default).Count -eq 0)
                {
                    $sweepAdaptive = $true
                }
            }
        }

        # Default case: take the profile and run it wide open.
        else
        {
            $doUnconstrained = $true
        }

        # Note: if this is a CPU Sweep, this may be the second run of a throughput limited profile (above)
        if ($doUnconstrained)
        {
            $o = DoIteration @paramUnconstrained
            LogOutput -ForegroundColor Cyan ("RESULT: unconstrained {0:N0} IOPS @ {1:P1} CPU => {2:N0} IOPS/VM from {3:N0} VMs" -f ($o.IOPS,($o.AverageCPU/100),($o.IOPS/$vms),$vms))
        }

        # If not a sweep, this completes the run.
        if (-not $CpuSweep) {
            LogOutput -ForegroundColor Green "Single run, complete"
            LogToComplete
            return
        }

        #
        # Pair of points defining the current place in the sweep. Becomes valid after there are two or more measured points.
        #   Adaptive: uppermost points defining the adaptive search (upper may represent cutoff)
        #   Infill: bracketing the current split
        #

        $p1 = $p2 = $null
        $last = $false

        do {

            #
            # Adaptive sweep for upper anchor
            #
            # Scale up until cutoff is reached, then iteratively split the resulting gap until within
            # the given tolerance.
            #

            if ($sweepAdaptive)
            {
                $p2, $p1 = GetUpperAnchor $results -OrderBy 'IOPS'

                # Upper (single or of two) measurement, not cutoff
                if ($p2.CutoffType -eq [CutoffType]::No)
                {
                    $qos = 5 * $p2.IOPS / $vms
                    LogOutput -ForegroundColor Cyan ("ADAPTIVE UP: scaling up to QoS {0:N0} => {1:N0} IOPS from {2:N0} VMs" -f ($qos,($qos*$vms),$vms))
                }

                # Upper is cutoff (single or of two). Split in between to try to close the cutoff gap.
                else
                {
                    # Determine lower side of split (either p1 or 0)
                    $lower = 0
                    if ($null -ne $p1)
                    {
                        $lower = [int] $p1.IOPS
                    }

                    # If upper points are already within the window, we are done now. Note that one will be the most recently measured.
                    if ($lower -ne 0 -and (IsWithin $p1.IOPS $p2.IOPS $qosAdaptiveWindow))
                    {
                        LogOutput -ForegroundColor Green ("ADAPTIVE COMPLETE: gap between {1:N0} and {2:N0} IOPS is within {0:P1}" -f (($qosAdaptiveWindow/100),$p1.IOPS,$p2.IOPS))
                        $sweepAdaptive = $false
                        continue
                    }

                    $tgtIOPS = [int] (($p2.IOPS + $lower) / 2)
                    $qos = $tgtIOPS / $vms

                    # Edge case where split bottoms out at an unsplittable small integer (like [2-3] with large percentage delta)
                    if ($tgtIOPS -eq $lower)
                    {
                        LogOutput -ForegroundColor Green ("ADAPTIVE COMPLETE: unsplittable gap between [{0:N0} - {1:N0}] IOPS" -f ($lower,$p2.IOPS))
                        $sweepAdaptive = $false
                        continue
                    }

                    LogOutput -ForegroundColor Cyan ("ADAPTIVE: splitting [{3:N0} - {4:N0} IOPS] to QoS {0:N0} => {1:N0} IOPS from {2:N0} VMs" -f ($qos,($qos*$vms),$vms,$lower,$p2.IOPS))

                    # If the p2 cutoff is within the window of the target, we can pull the adaptive sweep down now.
                    #
                    # There are two basoc possibilities: the system will scale onto this target or it will undershoot.
                    # Regardless of how it might undershoot (triggering the scale cutoff or not) there is no need
                    # to reason about continuing. If the undershoot did not trigger the scale cutoff and the result
                    # stayed outside the adaptive cutoff w.r.t. p2 we would continue pounding away. This occurs when p1
                    # gets close to the actual scaling limit.
                    #
                    # Regardless of the result we'll get we do want this target attempt to try to dial in the upper
                    # anchor, but we know ahead of time (now) that will complete the search.

                    if (IsWithin $tgtIOPS $p2.IOPS $qosAdaptiveWindow)
                    {
                        LogOutput -ForegroundColor Green ("ADAPTIVE COMPLETING: gap between {1:N0} and next target {2:N0} IOPS (QoS {3:N0}) is within {0:P1}" -f (($qosAdaptiveWindow/100),$p1.IOPS,$tgtIOPS,$qos))
                        $sweepAdaptive = $false
                    }
                }
            }

            #
            # Infill - anchor identified, filling in the sweep coverage.
            #
            # Target next QOS based on dividing the largest gap in Average CPU achieved: either splitting
            # the IOPS for those measured CPU utilizations OR toward zero, if that is the largest gap in IOPS
            # targeted so far.
            #

            else
            {
                # At anchor; halve IOPS/vm to begin sweep.
                if ($results.Count -eq 1)
                {
                    $qos = $results.IOPS / 2 / $vms
                    LogOutput -ForegroundColor Cyan ("START: halving anchor to QoS {0:N0} => target {1:N0} IOPS from {2:N0} VMs" -f $qos,($qos*$vms),$vms)
                }

                # Contine sweep at best available split.
                else
                {
                    $p2, $p1 = GetNextSplit $results 'AverageCPU' -OrderBy 'IOPS'
                    $minIOPS = ($results.IOPS | Measure-Object -Minimum).Minimum

                    # Determine lower side of split (either p1 or 0, if p2 wound up being a cutoff)
                    $lower = 0
                    if ($null -ne $p1)
                    {
                        $lower = [int] $p1.IOPS
                    }

                    # Is gap based on AverageCPU split larger than the minimum?
                    if ($null -eq $p1 -or $minIOPS -lt ($p2.IOPS - $p1.IOPS))
                    {
                        # Stop if this best split has met the infill goal window
                        if (IsWithin $p2.IOPS $lower $qosWindow)
                        {
                            LogOutput -ForegroundColor Yellow ("STOP: largest measurement gap, between {1:N0} and {2:N0} IOPS, would result in next QOS < +/-{0:P1}" -f (($qosWindow/100),$lower,$p2.IOPS))
                            break
                        }

                        # Average and scale IOPS -> VM QOS
                        $tgtIOPS = [int] (($p2.IOPS + $lower) / 2)

                        # Edge case where split bottoms out at an unsplittable small integer (like [2-3] with large percentage delta)
                        if ($tgtIOPS -eq $lower)
                        {
                            LogOutput -ForegroundColor Green ("STOP: unsplittable gap between [{0:N0} - {1:N0}] IOPS" -f ($lower,$p2.IOPS))
                            break
                        }

                        $qos = $tgtIOPS / $vms
                        LogOutput -ForegroundColor Cyan ("CONTINUE: next target at QoS {0:N0} => {1:N0} IOPS from {2:N0} VMs, splitting prior measurments @ {3:N0} & {4:N0} IOPS/VM" -f ($qos,$tgtIOPS,$vms,($lower/$vms),($p2.IOPS/$vms)))

                        # Similar to the adaptive case, if the split would logically close within the measurement window, this is now the last measurement.
                        # Indicate this so that regardless of result (say it does not scale) we complete after this next iteration.
                        if (IsWithin $tgtIOPS $p2.IOPS $qosWindow)
                        {
                            LogOutput -ForegroundColor Green ("INFILL COMPLETING: gap between {1:N0} and next target {2:N0} IOPS (QoS {3:N0}) is within {0:P1}" -f (($qosWindow/100),$p2.IOPS,$tgtIOPS,$qos))
                            $last = $true
                        }
                    }
                    else
                    {
                        $qos = $minIOPS / 2 / $vms
                        LogOutput -ForegroundColor Cyan ("CONTINUE: next target at QoS {0:N0} => {1:N0} IOPS from {2:N0} VMs, halving minimum so far @ {3:N0} IOPS/VM" -f ($qos,($qos*$vms),$vms,($minIOPS/$vms)))
                    }
                }
            }

            # Coerce to integer for equality comparisons
            $qos = [int] $qos
            $tgtIOPS = $qos * $vms

            # Bottomed out QoS
            if ($qos -eq 0) { break }

            # Final speedbrake - if we land on a prior QoS measurement, done with this phase of the loop.
            if ($results |? QOSperVM -eq $qos)
            {
                LogOutput -ForegroundColor Cyan "STOP: result already captured at QoS $qos"
                if ($sweepAdaptive) {
                    $sweepAdaptive = $false
                    continue
                }
                else
                {
                    break
                }
            }

            #
            # Iteration
            #

            $o = DoIteration -QoS $qos -IsSearch -IsAdaptive:$sweepAdaptive
            LogOutput -ForegroundColor Cyan ("RESULT: {0:N0} IOPS @ {1:P1} CPU => {2:N0} IOPS/VM from {3:N0} VMs v. target {4:N0} IOPS/VM" -f ($o.IOPS,($o.AverageCPU/100),($o.IOPS/$vms),$vms,$qos))

            #
            # If we have a surge result (workload went past window above throughput limit) there is a tool/systemic issue affecting
            # results that will need external triage. Stop here. This is an error condition and not a normal cutoff.
            #

            if ($o.CutoffType -eq [CutoffType]::Surge)
            {
                break
            }

        } until ($last)

        # Log completion. Note that a thrown error will bypass and not log completion (try again?), and sweep terminating
        # cases (e.g., in-window, unphysical QoS) that trigger before exhaustion are indeed (also) completion.
        LogOutput -ForegroundColor Green "Sweep, complete"
        LogToComplete
    }
    catch
    {
        if ($RunLabel.Length)
        {
            # If this is part of a labeled run, stash any iteration file content in error directory for analysis.
            # Replace prior error content, if any.
            $errd = (Join-Path $resultPath "error")
            if (Test-Path $errd) { Remove-Item $errd -Recurse }
            $null = mkdir $errd
            $itf = @(GetIterationFiles $resultPath)
            if ($itf.Count)
            {
                LogOutput "Iteration file content saved to $errd for triage"

                # Force continue - we don't want a stacked error trying to move content to lose
                # the error which threw us into this condition.
                $itf | Move-Item -Destination $errd -ErrorAction Continue
            }
        }

        throw
    }
    finally
    {
        # Clear QoS if used in this sweep
        if ($UseStorageQos)
        {
            Set-FleetQos @clusterParam
        }
    }
}

function Show-FleetCpuSweep
{
    [CmdletBinding()]
    param(
        [Parameter(Mandatory = $true)]
        [string]
        $ResultFile,

        [Parameter()]
        [int]
        $SigFigs = 5
        )

    function GetSigFigs(
        [double]$Value,
        [int]$SigFigs
        )
    {
        $log = [math]::Ceiling([math]::log10([math]::abs($value)))
        $decimalpt = $sigfigs - $log

        # if all sigfigs are above the decimal point, round off to
        # appropriate power of 10
        if ($decimalpt -lt 0) {
            $pow = [math]::Abs($decimalpt)
            $decimalpt = 0
            $value = [math]::Round($value/[math]::Pow(10,$pow))*[math]::pow(10,$pow)
        }

        "{0:F$($decimalpt)}" -f $value
    }

    function ShowReport
    {
        param(
            [Parameter()]
            [string]
            $Header,

            [Parameter()]
            [object[]]
            $Data
        )

        Write-Host -ForegroundColor Cyan ("-"*20)
        Write-Host "Variation: $Header"

        # Special case of a single measured run, no sweep
        if ($Data.Count -eq 1)
        {
            Write-Host ("IOPS = {0} at AverageCPU {1:P2}" -f (GetSigFigs $Data.IOPS $SigFigs),($Data.AverageCPU/100))
            Write-Host "Single measurement of the workload."
            return
        }

        if ($Data.Count -eq 2)
        {
            Write-Host "Two measurements of the workload. TBD analysis statement."
            return
        }

        $fit = Get-FleetPolynomialFit -Order 2 -X $Data.AverageCPU -Y $Data.IOPS

        # sign of the linear/quad terms for the equation display
        # the constant term falls out of the plain print
        $sign = @()
        if ( $fit.a[1] -ge 0) { $sign += '+ ' } else { $sign += '' }
        if ( $fit.a[2] -ge 0) { $sign += '+ ' } else { $sign += '' }

        Write-Host ("IOPS = {0} {1}{2}(AverageCPU) {3}{4}(AverageCPU^2)" -f (GetSigFigs $fit.a[0] $SigFigs),$sign[0],(GetSigFigs $fit.a[1] $SigFigs),$sign[1],(GetSigFigs $fit.a[2] $SigFigs))
        Write-Host ("Number of measurements = {1}`nR^2 goodness of fit {0:P2}" -f $fit.r2,$Data.Count)

        # Use quadtratic form to estimate zero intercept (~idle cpu utilization)
        # If the discriminant is < 0 something unreasonable happened (a complex solution)
        # Note that the usual form will be an inverted parabola (a[2] < 0); provide the
        # smallest positive root.

        $discriminant = [math]::pow($fit.a[1],2) - 4 * $fit.a[0] * $fit.a[2]
        if ($discriminant -lt 0)
        {
            Write-Host -ForegroundColor Red "There is no solution for AverageCPU = 0; check results and system conditions for unexpected behavior"
        }
        else
        {
            $discriminant = [math]::Sqrt($discriminant)
            $r0 = (-$fit.a[1] - $discriminant) / (2*$fit.a[2])
            $r1 = (-$fit.a[1] + $discriminant) / (2*$fit.a[2])
            $r = $null
            if ($r0 -ge 0 -and $r0 -lt $r1)
            {
                $r = $r0
            }
            if ($r1 -ge 0)
            {
                $r = $r1
            }

            if ($null -eq $r)
            {
                Write-Host -ForegroundColor Red ("Unexpected solutions for AverageCPU @ IOPS = 0 ({0:N2}, {1:N2}); check results and system conditions for unexpected behavior" -f $r0,$r1)
            }
            else
            {
                Write-Host ("Estimated background AverageCPU (CPU @ IOPS = 0): {0:P1}" -f ($r/100))
            }
        }
    }

    # Determine result grouping. We assume that the result file metadata columns end at QOS.
    # To the left are arbitrary metadata columns provided by a runner (workload, alignment, etc.).
    # Dynamically determine these metadata columns and group the results by them, producing a set of
    # results to apply fits to. Some may be single values, not swept by QOS.

    $header = @((Get-Content $ResultFile -TotalCount 1) -split "`t" -replace '"','')

    if ($null -eq $header)
    {
        Write-Error "Could not read the table header from the result file - is this a CPU sweep result?"
        return
    }

    $splitIdx = $header.IndexOf('QOSperVM')
    if ($splitIdx -eq -1)
    {
        Write-Error "Result file does not have a QOSperVM column - is this a CPU sweep result?"
        return
    }

    Write-Host @"
CPU Sweep Report

The following models are quadratic fits to the measured results at the
given write ratios.

Take care that these formulae are only used to reason about the region
where these values have a working relationship:

    AverageCPU > background and < 100%

The second order term (AverageCPU^2) should generally be small negative
value indicating IOPS flattening as the system approaches saturation.

Use R^2 (coefficient of determination) as a quality check for the fit.
Values close to 100% mean that the data is good fit. If R^2 is significantly
less than 100%, the fit is not good and a closer look at system behavior may
be required.
"@

    if ($splitIdx -eq 0)
    {
        # single un-named data set

        ShowReport -Header '' -Data (Get-FleetResultLog $ResultFile)
        return
    }

    # Columns to the left of QOS minus internal metadata, forming the properties to group by.
    $header = [collections.arraylist] $header[0..($splitIdx - 1)]
    $header.Remove('RunType')
    $header.Remove('CutoffType')

    # Process all groupings
    Get-FleetResultLog $ResultFile | Group-Object -Property $header |% {

        $desc = $(foreach ($col in $header)
        {
            $col,$_.Group[0].$col -join ' = '
        }) -join "`n`t"

        ShowReport -Header $desc -Data $_.Group
    }
}

function SolveLU
{
    param(
        [double[,]]
        $m,

        [double[]]
        $b
    )

    # m must be square

    if ($m.Rank -ne 2)
    {
        Write-Error "matrix m is not two dimensional: rank $($m.Rank)"
        return
    }
    if ($m.GetLength(0) -ne $m.GetLength(1))
    {
        Write-Error "matrix m is not square: $($m.GetLength(0)) x $($m.GetLength(1))"
        return
    }
    if ($m.GetLength(0) -ne $b.GetLength(0))
    {
        Write-Error "vector b is not of the same dimension as m: $($b.GetLength(0)) != $($m.GetLength(0))"
        return
    }
    $n = $m.GetLength(0)

    # decompose matrix LU = L+U-I
    $lu = [double[,]]::new($n, $n)
    $sum = [double] 0
    for ($i = 0; $i -lt $n; ++$i)
    {
        for ($j = $i; $j -lt $n; ++$j)
        {
            $sum = [double] 0
            for ($k = 0; $k -lt $i; ++$k)
            {
                $sum += $lu[$i, $k] * $lu[$k, $j]
            }
            $lu[$i, $j] = $m[$i, $j] - $sum
        }

        for ($j = $i + 1; $j -lt $n; ++$j)
        {
            $sum = [double] 0
            for ($k = 0; $k -lt $i; ++$k)
            {
                $sum += $lu[$j, $k] * $lu[$k, $i]
            }
            $lu[$j, $i] = (1 / $lu[$i, $i]) * ($m[$j, $i] - $sum)
        }
    }

    # find Ly = b
    $y = [double[]]::new($n)
    for ($i = 0; $i -lt $n; ++$i)
    {
        $sum = [double] 0
        for ($k = 0; $k -lt $i; ++$k)
        {
            $sum += $lu[$i, $k] * $y[$k]
        }
        $y[$i] = $b[$i] - $sum
    }

    # find Ux = y
    $x = [double[]]::new($n)
    for ($i = $n - 1; $i -ge 0; --$i)
    {
        $sum = [double] 0
        for ($k = $i + 1; $k -lt $n; ++$k)
        {
            $sum += $lu[$i, $k] * $x[$k]
        }
        $x[$i] = (1 / $lu[$i, $i]) * ($y[$i] - $sum)
    }

    ,$x
}

function Get-FleetPolynomialFit
{
    [CmdletBinding()]
    param(
        [Parameter(
            Mandatory = $true
        )]
        [int]
        $Order,

        [Parameter(
            Mandatory = $true
        )]
        [double[]]
        $X,

        [Parameter(
            Mandatory = $true
        )]
        [double[]]
        $Y
    )

    #
    # Return coefficients of least-squares fit for a polynomial of order Order
    # over the input X/Y vectors. Return vector a is in ascending order:
    #
    #    a0 + a1 x + a2 x^2 ... = y
    #
    # System of equations ma = b from polynominal residual function
    # Due in part to discussion @
    #   https://mathworld.wolfram.com/LeastSquaresFittingPolynomial.html
    #   https://en.wikipedia.org/wiki/LU_decomposition
    # Additional reading: https://neutrium.net/mathematics/least-squares-fitting-of-a-polynomial/
    #
    if ($Order -ge $X.Count)
    {
        Write-Error "Polynomial of order $Order requires at least $($Order + 1) values to fit: $($X.Count) provided"
        return $null
    }
    if ($Y.Count -ne $X.Count)
    {
        Write-Error "Ranks of X ($($X.Count)) and Y ($($Y.Count)) input vectors not equal"
        return $null
    }

    # for all col
    $m = [double[,]]::new($Order + 1, $Order + 1)
    for ($j = 0; $j -le $Order; ++$j)
    {
        # for all rows below diagonal
        for ($i = $j; $i -le $Order; ++$i)
        {
            # i, j
            $m[$i, $j] = ($X |% { [math]::Pow($_, $i + $j) } | Measure-Object -Sum).Sum
            # mirror over the diagonal: j, i = i, j
            $m[$j, $i] = $m[$i, $j]
        }
    }

    $b = [double[]]::new($Order + 1)
    for ($j = 0; $j -le $Order; ++$j)
    {
        for ($i = 0; $i -lt $X.Count; ++$i)
        {
            $b[$j] += [math]::Pow($X[$i], $j) * $Y[$i]
        }
    }

    $a = SolveLU $m $b

    # calculate residual r2 of the fit
    $ssres = [double] 0
    $sstot = [double] 0
    $yMean = ($Y | Measure-Object -Average).Average
    for ($i = 0; $i -lt $X.Count; ++$i)
    {
        $yFit = [double] 0
        for ($j = 0; $j -lt $a.Count; ++$j)
        {
            $yFit += $a[$j]*[math]::Pow($X[$i], $j)
        }

        $ssres += [math]::pow($Y[$i] - $yFit, 2)
        $sstot += [math]::pow($Y[$i] - $yMean, 2)
    }

    # If sstot is zero this means all measurements are exactly the average value.
    # In other words, it is a contstant and the non-constant coefficients are/should
    # all be zero with a perfect fit.
    if ($sstot -eq 0)
    {
        $r2 = 1
    }
    else
    {
        $r2 = 1 - ($ssres/$sstot)
    }


    [PsCustomObject] @{ a = $a; r2 = $r2 }
}

function Use-FleetPolynomialFit
{
    [CmdletBinding()]
    param(
        [Parameter(Mandatory = $true)]
        [double[]]
        $A,

        [Parameter(Mandatory = $true)]
        [double]
        $X
    )

    # Apply the polynomial function defined by the vector of coefficients A
    # at the value X.
    #
    # Y = A0 + A1 X + A2 X^2 + ...

    $accX = [double] 1
    $accY = [double] 0
    foreach ($coeff in $A)
    {
        $accY += $accX * $coeff
        $accX *= $X
    }

    $accY
}

function GetNextSplit
{
    param(
        [Parameter(Mandatory = $true)]
        [object[]]
        $P,

        [Parameter(Mandatory = $true)]
        [string]
        $V,

        [Parameter(Mandatory = $true)]
        [string]
        $OrderBy
    )

    # This the adjacent pair of measured points in P with the largest gap in V when ordered by given field.

    $P = $P | Sort-Object -Property $OrderBy

    # Scan for the pair of values with the largest difference.
    # Stop if lower result met cutoff conditions - we are willing to interpolate downward from
    # a cutoff, but not above one.
    $nextIdx = -1
    $nextDelta = 0
    for ($i = 0; $i -lt $P.Count - 1 -and $P[$i].CutoffType -eq [CutoffType]::No; ++$i)
    {
        $thisDelta = [math]::abs($P[$i + 1].$V - $P[$i].$V)
        if ($thisDelta -gt $nextDelta)
        {
            $nextIdx = $i
            $nextDelta = $thisDelta
        }
    }

    # Return the pair
    if ($nextIdx -ge 0)
    {
        return $P[$nextIdx + 1],$P[$nextIdx]
    }
    else
    {
        return $P[0]
    }
}

function GetUpperAnchor
{
    param(
        [Parameter(Mandatory = $true)]
        [object[]]
        $P,

        [Parameter(Mandatory = $true)]
        [string]
        $OrderBy
    )

    # Single/none

    if ($P.Count -eq 0) { return }
    if ($P.Count -eq 1) { return $P[0] }

    # This the adjacent pair of points in P which, when ordered by the given field, the higher of
    # which first hit a cutoff or is the highest value.
    #
    # Return is in descending order (high then low). Return may be none (if no points), one if the
    # first point is already at cutoff OR single point present, or two otherwise. If caller says
    # $h,$l = GetUpperAnchor then h/l will be $null as appropriate to the case.

    $P = $P | Sort-Object -Property $OrderBy

    $prior = $null
    foreach ($pt in $P)
    {
        if ($pt.CutoffType -ne [CutoffType]::No) { return $pt,$prior }
        $prior = $pt
    }

    return $P[-1],$P[-2]
}

function GetDistributedShift
{
    [CmdletBinding()]
    param(
        [Parameter(Mandatory = $true)]
        [ValidateNotNullOrEmpty()]
        [object[]]
        $Group,

        [Parameter(Mandatory = $true)]
        [uint32]
        $N
    )

    foreach ($g in $Group)
    {
        if ($g.GetType().Name -ne "Object[]" -and $g.GetType().Name -ne "ArrayList")
        {
            Write-Error "Group input must be list of lists"
            return
        }
    }

    if ($Group.Count -le 1)
    {
        Write-Error "Need two or more groups for distributed shift"
        return
    }

    $counts = @($Group[0].Count)
    $e = $false
    foreach ($g in $Group[1..($Group.Count - 1)])
    {
        $counts += $g.Count
        if ($g.Count -ne $counts[0])
        {
            $e = $true
        }
    }

    if ($e)
    {
        Write-Error "Invalid groups for distributed shift - all must be same size ($($counts -join ' '))"
        return
    }

    if ($N -eq 0 -or $N -gt $Group[0].Count)
    {
        Write-Error "Invalid number of VMs/node to rotate - must be 0 < N < $($Group[0].Count)"
        return
    }

    #
    # Distributed Shift: take the N VMs from the end of each ordered set per node and logically rotate
    # the resulting groups by an increasing number of positions (1 <= rotation < #groups).
    #
    #   1: abcde
    #   2: fghij
    #   3: klmno
    #
    # => N = 1
    #          1 <= positions rotated
    #   1: abcdo
    #   2: fghie
    #   3: klmnj
    #
    # => N = 2
    #         21 <= positions rotated
    #   1: abcio
    #   2: fghne
    #   3: klmdj
    #
    # => N = 3
    #        121 <= positions rotated
    #   1: abmio
    #   2: fgcne
    #   3: klhdj
    #

    #
    # Build arrays with the bottom elements which will not move.
    #

    $arr = @()

    foreach ($g in $Group)
    {
        if ($N -lt $g.Count)
        {
            $arr += ,[collections.arraylist]@($g[0..($g.Count - $N - 1)])
        }
        else
        {
            $arr += ,[collections.arraylist]@()
        }
    }

    #
    # Now move across the groups.
    #
    # This is reversed since rotation amount starts at 1 at the end of the
    # array and increases with lower indices. The shift starts in the middle
    # and works to the end so we do not modify relative positions (and element
    # at index 5 is always at index 5 of a rotation, regardless of how many
    # VMs/node are rotated).
    #
    # Example: when distributing four groups at N = 6 we vary
    # between rotating by 1, 2 and 3. This is what it looks
    # like in index order
    #
    #    3 2 1 3 2 1    <- rotation in column
    #    0 1 2 3 4 5    <- array index
    #

    $rot = 1 + ($N-1)%($arr.Count-1)
    for ($srcIdx = -$N; $srcIdx -lt 0; ++$srcIdx)
    {
        #
        # Move across groups in the groups, rotating elements
        #

        for($src = 0; $src -lt $arr.Count; ++$src)
        {
            $dst = ($src + $rot) % $arr.Count
            $null = $arr[$dst].Add($Group[$src][$srcIdx])
        }

        # Roll rotation back to max at zero. Reversed since rotation
        # is moving in the forward direction through the array.
        --$rot
        if (-not $rot) { $rot = $arr.Count - 1 }
    }

    $arr
}

function FilterObject
{
    [CmdletBinding()]
    param(
        [Parameter(ValueFromPipeline = $true)]
        $InputObject,

        [Parameter(Mandatory = $true)]
        [hashtable]
        $Filter
        )

    #
    # Utility to filter pipeline objects with an AND equality test
    # of all property/values provided in the filter. Note empty
    # filter passes every element.
    #

    process
    {
        $pass = $true

        foreach ($k in $Filter.Keys)
        {
            if ($_.$k -ne $Filter.$k)
            {
                $pass = $false
                break
            }
        }

        if ($pass) { $_ }
    }
}

function SetCacheBehavior
{
    [CmdletBinding()]
    param(
        [Parameter()]
        [string]
        $Cluster = ".",

        [switch]
        $PopulateOnFirstMiss
    )

    $property = @{
        Path = 'HKLM:\system\CurrentControlSet\Services\clusbflt\Parameters'
        Name = 'CacheFlags'
    }

    # Cache behavior modifications are set per node, not at the actual cluster level. This exposes
    # a risk that if nodes are down or later added that the behaviors will vary across the cluster.
    # This is not desirable, and is at the caller's risk to manage.
    $nodes = @(Get-ClusterNode -Cluster $Cluster |? State -eq Up)

    foreach ($node in $nodes)
    {
        $s = New-PSSession -ComputerName $node

        try
        {
            $v = Invoke-Command -Session $s { Get-ItemProperty @using:property -ErrorAction SilentlyContinue }
            if ($null -eq $v)
            {
                $currentValue = [uint32] 0
            }
            else
            {
                $currentValue = [uint32] $v.CacheFlags
            }
            $newValue = $currentValue

            #
            # Parameters
            #

            if ($PSBoundParameters.ContainsKey('PopulateOnFirstMiss'))
            {
                $mask = [uint32] 0x80000

                if ($PopulateOnFirstMiss) { $newValue = $newValue -bor $mask }
                                     else { $newValue = $newValue -band -bnot $mask }
            }

            #
            # Apply
            #

            # Do nothing?
            if ($currentValue -eq $newValue)
            {
                continue
            }

            $r = Invoke-Command -session $s {

                # Get control object. If RefreshRegParams is not available, issue warning.
                # This is only available with Server 2022+.
                $dm = Get-WmiObject -namespace "root\wmi" ClusBfltDeviceMethods
                if ($null -eq $dm -or -not (Get-Member -InputObject $dm -Name RefreshRegParams))
                {
                    Write-Warning "S2D Cache on $($env:COMPUTERNAME) does not support dynamic cache behavior refresh"
                    return $null
                }

                Set-ItemProperty @using:property -Value ([uint32] $using:newValue) -Type ([Microsoft.Win32.RegistryValueKind]::DWord)

                # Trigger refresh - object returned indicating reboot required (or not)
                $dm.RefreshRegParams()

                # If value is now zero, remove it to keep the registry clean
                if ($using:newValue -eq 0)
                {
                    Remove-ItemProperty @using:property
                }
            }

            # Reboot should never be required when updating cache behavior flags, but check
            if ($null -ne $r -and $r.fRebootRequired)
            {
                Write-Error "Unexpected reboot required when updating cache behavior flags: $node"
            }
        }
        finally
        {
            Remove-PSSession -Session $s
        }
    }
}