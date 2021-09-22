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

#
# Template Profiles
#
# Template profiles state througput in relative units per target per timespan.
# If a timespan contains two targets, one with 1 relative IOPS and one with 9
# relative IOPS, applying an aggregate IOPS throughput of 100 would result in
# 10 & 90 respectively.
#
# Throughput MUST be in template in order to apply throughput limits.
#

# Note: random/stridesize elements MUST be removed if -Random will be allowed for
# a profile.

#
# Peak/General: a single thread high-QD unbuffered/writethrough single target template
# with latency/iops measured. 10MiB random write data source. All substitutions
# allowed.
#
# Peak and General differ in distribution. General is across a non-uniform distribution
# of the entire available workingset, while Peak has no built-in distribution. This allows
# Peak to be instantiated with a reduced workingset (-BaseOffset/-MaxOffset) for a uniformly
# loaded small workingset.
#
#
# Derived from: -t1 -b4k -o32 -w0 -r -Suw -D -L -Z10m *1
# Random element removed, also BaseFileOffset/MaxFileSize
# Profile specific required parameters override WriteRatio, BlockSize
#

$PeakProfile = @"
<Profile>
  <Progress>0</Progress>
  <ResultFormat>xml</ResultFormat>
  <Verbose>false</Verbose>
  <TimeSpans>
    <TimeSpan>
      <CompletionRoutines>false</CompletionRoutines>
      <MeasureLatency>true</MeasureLatency>
      <CalculateIopsStdDev>true</CalculateIopsStdDev>
      <DisableAffinity>false</DisableAffinity>
      <Duration></Duration>
      <Warmup></Warmup>
      <Cooldown></Cooldown>
      <IoBucketDuration>1000</IoBucketDuration>
      <RandSeed>0</RandSeed>
      <Targets>
        <Target>
          <Path>*1</Path>
          <BlockSize>4096</BlockSize>
          <DisableOSCache>true</DisableOSCache>
          <WriteThrough>true</WriteThrough>
          <WriteBufferContent>
            <Pattern>random</Pattern>
            <RandomDataSource>
              <SizeInBytes>10485760</SizeInBytes>
            </RandomDataSource>
          </WriteBufferContent>
          <ThreadStride>0</ThreadStride>
          <RequestCount>32</RequestCount>
          <WriteRatio>0</WriteRatio>
          <Throughput>0</Throughput>
          <ThreadsPerFile>1</ThreadsPerFile>
          <IOPriority>3</IOPriority>
        </Target>
      </Targets>
    </TimeSpan>
  </TimeSpans>
</Profile>
"@

$GeneralProfile = @"
<Profile>
  <Progress>0</Progress>
  <ResultFormat>xml</ResultFormat>
  <Verbose>false</Verbose>
  <TimeSpans>
    <TimeSpan>
      <CompletionRoutines>false</CompletionRoutines>
      <MeasureLatency>true</MeasureLatency>
      <CalculateIopsStdDev>true</CalculateIopsStdDev>
      <DisableAffinity>false</DisableAffinity>
      <Duration></Duration>
      <Warmup></Warmup>
      <Cooldown></Cooldown>
      <IoBucketDuration>1000</IoBucketDuration>
      <RandSeed>0</RandSeed>
      <Targets>
        <Target>
          <Path>*1</Path>
          <BlockSize>4096</BlockSize>
          <DisableOSCache>true</DisableOSCache>
          <WriteThrough>true</WriteThrough>
          <WriteBufferContent>
            <Pattern>random</Pattern>
            <RandomDataSource>
              <SizeInBytes>10485760</SizeInBytes>
            </RandomDataSource>
          </WriteBufferContent>
          <Distribution>
            <Percent>
              <Range IO="95">5</Range>
              <Range IO="4">10</Range>
            </Percent>
          </Distribution>
          <ThreadStride>0</ThreadStride>
          <RequestCount>32</RequestCount>
          <WriteRatio>0</WriteRatio>
          <Throughput>0</Throughput>
          <ThreadsPerFile>1</ThreadsPerFile>
          <IOPriority>3</IOPriority>
        </Target>
      </Targets>
    </TimeSpan>
  </TimeSpans>
</Profile>
"@

# VDI Profile: DiskSpd parameters that mimic a VDI workload
#
# Derived from:
#   (write component) -t1 -o8 -b32k -w100 -g6i -rs20 -rdpct95/5:4/10 -Z10m -f10g *1
#   (read component)  -t1 -o8 -b8k -w0 -g6i -rs80 -rdpct95/5:4/10 -f8g *1


$VDIProfile = @"
<Profile>
  <Progress>0</Progress>
  <ResultFormat>xml</ResultFormat>
  <Verbose>false</Verbose>
  <TimeSpans>
    <TimeSpan>
      <CompletionRoutines>false</CompletionRoutines>
      <MeasureLatency>true</MeasureLatency>
      <CalculateIopsStdDev>true</CalculateIopsStdDev>
      <DisableAffinity>false</DisableAffinity>
      <Duration></Duration>
      <Warmup></Warmup>
      <Cooldown>0</Cooldown>
      <IoBucketDuration>1000</IoBucketDuration>
      <RandSeed>0</RandSeed>
      <Targets>
        <Target>
          <Path>*1</Path>
          <BlockSize>32768</BlockSize>
          <BaseFileOffset>0</BaseFileOffset>
          <MaxFileSize>10737418240</MaxFileSize>
          <DisableOSCache>true</DisableOSCache>
          <WriteThrough>true</WriteThrough>
          <WriteBufferContent>
            <Pattern>random</Pattern>
            <RandomDataSource>
              <SizeInBytes>10485760</SizeInBytes>
            </RandomDataSource>
          </WriteBufferContent>
          <Random>32768</Random>
          <RandomRatio>20</RandomRatio>
          <Distribution>
            <Percent>
              <Range IO="95">5</Range>
              <Range IO="4">10</Range>
            </Percent>
          </Distribution>
          <ThreadStride>0</ThreadStride>
          <RequestCount>8</RequestCount>
          <WriteRatio>100</WriteRatio>
          <Throughput unit="IOPS">6</Throughput>
          <ThreadsPerFile>1</ThreadsPerFile>
          <IOPriority>3</IOPriority>
        </Target>
        <Target>
          <Path>*1</Path>
          <BlockSize>8192</BlockSize>
          <BaseFileOffset>0</BaseFileOffset>
          <MaxFileSize>8589934592</MaxFileSize>
          <DisableOSCache>true</DisableOSCache>
          <WriteThrough>true</WriteThrough>
          <WriteBufferContent>
            <Pattern>sequential</Pattern>
          </WriteBufferContent>
          <Random>8192</Random>
          <RandomRatio>80</RandomRatio>
          <Distribution>
            <Percent>
              <Range IO="95">5</Range>
              <Range IO="4">10</Range>
            </Percent>
          </Distribution>
          <ThreadStride>0</ThreadStride>
          <RequestCount>8</RequestCount>
          <WriteRatio>0</WriteRatio>
          <Throughput unit="IOPS">6</Throughput>
          <ThreadsPerFile>1</ThreadsPerFile>
          <IOPriority>3</IOPriority>
        </Target>
      </Targets>
    </TimeSpan>
  </TimeSpans>
</Profile>
"@

# SQL Profile: DiskSpd parameters that mimic an SQL workload with log transaction
#
# Derived from:
#   (OLTP) -t4 -o32 -r -b8k -g1500i -w30 -rdpct95/5:4/10 -B5g -Z10m *1
#   (Log) -t1 -o1 -s -b32k -g300i -w100 -f5g -Z10m *1

$SQLProfile = @"
<Profile>
  <Progress>0</Progress>
  <ResultFormat>xml</ResultFormat>
  <Verbose>false</Verbose>
  <TimeSpans>
    <TimeSpan>
      <CompletionRoutines>false</CompletionRoutines>
      <MeasureLatency>true</MeasureLatency>
      <CalculateIopsStdDev>true</CalculateIopsStdDev>
      <DisableAffinity>false</DisableAffinity>
      <Duration></Duration>
      <Warmup></Warmup>
      <Cooldown></Cooldown>
      <IoBucketDuration>1000</IoBucketDuration>
      <RandSeed>0</RandSeed>
      <Targets>
        <Target>
          <Path>*1</Path>
          <BlockSize>8192</BlockSize>
          <BaseFileOffset>5368709120</BaseFileOffset>
          <MaxFileSize>0</MaxFileSize>
          <DisableOSCache>true</DisableOSCache>
          <WriteThrough>true</WriteThrough>
          <WriteBufferContent>
            <Pattern>random</Pattern>
            <RandomDataSource>
              <SizeInBytes>10485760</SizeInBytes>
            </RandomDataSource>
          </WriteBufferContent>
          <Random>8192</Random>
          <Distribution>
            <Percent>
              <Range IO="95">5</Range>
              <Range IO="4">10</Range>
            </Percent>
          </Distribution>
          <ThreadStride>0</ThreadStride>
          <RequestCount>32</RequestCount>
          <WriteRatio>30</WriteRatio>
          <Throughput unit="IOPS">1500</Throughput>
          <ThreadsPerFile>4</ThreadsPerFile>
          <IOPriority>3</IOPriority>
        </Target>
        <Target>
          <Path>*1</Path>
          <BlockSize>32768</BlockSize>
          <BaseFileOffset>0</BaseFileOffset>
          <MaxFileSize>5368709120</MaxFileSize>
          <DisableOSCache>true</DisableOSCache>
          <WriteThrough>true</WriteThrough>
          <WriteBufferContent>
            <Pattern>random</Pattern>
            <RandomDataSource>
              <SizeInBytes>10485760</SizeInBytes>
            </RandomDataSource>
          </WriteBufferContent>
          <StrideSize>32768</StrideSize>
          <ThreadStride>0</ThreadStride>
          <RequestCount>1</RequestCount>
          <WriteRatio>100</WriteRatio>
          <Throughput unit="IOPS">300</Throughput>
          <ThreadsPerFile>1</ThreadsPerFile>
          <IOPriority>3</IOPriority>
        </Target>
      </Targets>
    </TimeSpan>
  </TimeSpans>
</Profile>
"@

#
# Requires - parameters which must be provided
# AllowsRandSeq - compatibility with -Random/-Sequential rewrite rule
# Compatibility - list of parameters for compatibility check
# Compatible - provides sense of the Compatibility list for simpler bulk inclusion/exclusion
#   true    - an inclusion list; parameters not mentioned are not allowed (empty = none allowed)
#   false   - an exclusion list; parameters mentioned are not allowed (empty = all allowed)
#

$FleetProfiles = @{
    Peak = @{
        Profile = $PeakProfile
        AllowRandSeq = $true
        Requires = @('WriteRatio', 'BlockSize')
        Compatibility = $null
        Compatible = $false
    }

    General = @{
        Profile = $GeneralProfile
        AllowRandSeq = $true
        Requires = @('WriteRatio', 'BlockSize')
        Compatibility = $null
        Compatible = $false
    }

    VDI = @{
        Profile = $VDIProfile
        AllowRandSeq = $false
        Requires = $null
        Compatibility = $null
        Compatible = $true
    }

    SQL = @{
        Profile = $SQLProfile
        AllowRandSeq = $false
        Requires = $null
        Compatibility = $null
        Compatible = $true
    }
}

function Get-FleetProfileXml
{
    [CmdletBinding()]
    param(
        [Parameter()]
        [string]
        $Name,

        [Parameter()]
        [uint32]
        $Warmup = 300,

        [Parameter()]
        [uint32]
        $Duration = 60,

        [Parameter()]
        [uint32]
        $Cooldown = 30,

        [ValidateRange(0, 100)]
        [uint32]
        $WriteRatio,

        [Parameter()]
        [uint32]
        $ThreadsPerTarget,

        [Parameter()]
        [uint32]
        $BlockSize,

        [Parameter()]
        [uint32]
        $Alignment,

        [Parameter()]
        [switch]
        $Random,

        [Parameter()]
        [switch]
        $Sequential,

        [ValidateRange(0, 100)]
        [uint32]
        $RandomRatio,

        [Parameter()]
        [uint32]
        $RequestCount,

        [Parameter()]
        [uint64]
        $ThreadStride,

        [Parameter()]
        [uint64]
        $BaseOffset,

        [Parameter()]
        [uint64]
        $MaxOffset,

        [Parameter()]
        [ValidateRange(1,60)]
        [uint32]
        $IoBucketDurationSeconds
    )

    $x = $null

    #
    # Get base profile and validate profile-specific paramters
    #

    if (-not $PSBoundParameters.ContainsKey('Name'))
    {
        Write-Error "Available profiles: $(@($FleetProfiles.Keys | Sort-Object) -join ', ')"
        return
    }
    elseif (-not $FleetProfiles.ContainsKey($Name))
    {
        Write-Error "Unknown profile $Name; available: $(@($FleetProfiles.Keys | Sort-Object) -join ', ')"
        return
    }

    #
    # Switch validation
    #

    if ($PsBoundParameters.ContainsKey('Sequential') -and $PsBoundParameters.ContainsKey('Random'))
    {
        Write-Error "Random and Sequential cannot be specified together"
        return
    }

    #
    # Requires check
    #

    $err = @()
    foreach ($arg in $FleetProfiles[$Name].Requires)
    {
        if (-not $PSBoundParameters.ContainsKey($arg))
        {
            $err += ,$arg
        }
    }

    if ($err.Count)
    {
        Write-Error "$Name profile requires specification of $($err -join ', ')"
        return
    }

    #
    # Incompatible check
    #

    if (-not ($FleetProfiles[$Name].Compatible))
    {
        foreach ($arg in $FleetProfiles[$Name].Compatibility)
        {
            if ($PSBoundParameters.ContainsKey($arg))
            {
                $err += ,$arg
            }
        }
    }

    #
    # Compatible check
    #

    else
    {
        # Always-on core parameters + required list + compatible list
        $baseParameters = @('Name', 'Warmup', 'Duration', 'Cooldown')

        foreach ($arg in $PSBoundParameters.Keys)
        {
            if ($baseParameters -notcontains $arg -and
                $FleetProfiles[$Name].Requires -notcontains $arg -and
                $FleetProfiles[$Name].Compatibility -notcontains $arg)
            {
                $err += ,$arg
            }
        }
    }

    if ($err.Count)
    {
        Write-Error "$Name profile does not allow specification of $($err -join ', ')"
        return
    }

    #
    # Now load/modify profile.
    #

    $x = [xml] $FleetProfiles[$Name].Profile

    #
    # Perform replacements @ TimeSpan
    #

    foreach ($timeSpan in $x.SelectNodes("Profile/TimeSpans/TimeSpan"))
    {
        $timeSpan.Warmup = [string] $Warmup
        $timeSpan.Duration = [string] $Duration
        $timeSpan.Cooldown = [string] $Cooldown

        if ($PSBoundParameters.ContainsKey('IoBucketDurationSeconds'))
        {
            $timeSpan.IoBucketDuration = [string] ($IoBucketDurationSeconds * 1000)
        }

        # Autoscale to best clock fit >1000 n-second IOPS bucket datapoints.
        else
        {
            $timeSpan.IoBucketDuration = [string] ((FitClockRate -TotalSeconds $Duration -Samples 1000) * 1000)
        }
    }

    #
    # Perform replacements @ Target
    #

    foreach ($targetSet in $x.SelectNodes("Profile/TimeSpans/TimeSpan/Targets"))
    {
        $targets = $targetSet.SelectNodes("Target")

        foreach ($target in $targets)
        {
            #
            # 1:1 Common Substitutions
            #

            $TargetSubstitutions = @{
                BlockSize = 'BlockSize'
                RequestCount = 'RequestCount'
                ThreadsPerTarget = 'ThreadsPerFile'
                ThreadStride = 'ThreadStride'
                WriteRatio = 'WriteRatio'
                MaxOffset = 'MaxFileSize'
                BaseOffset = 'BaseFileOffset'
            }

            foreach ($s in $TargetSubstitutions.Keys)
            {
                if ($PSBoundParameters.ContainsKey($s))
                {
                    SetSingleNode -Xml $x -ParentNode $target -Node $TargetSubstitutions[$s] -Value ([string] $PSBoundParameters[$s])
                }
            }

            #
            # Target Random/Sequential/Alignment
            #

            if ($FleetProfiles[$Name].AllowRandSeq)
            {
                # Inherit default buffer alignment from buffer size
                if (-not $PSBoundParameters.ContainsKey('Alignment'))
                {
                    $Alignment = $BlockSize
                }

                # Random or RandomRatio
                if ($Random -or $PsBoundParameters.ContainsKey('RandomRatio'))
                {
                    $e = $x.CreateNode([xml.xmlnodetype]::Element, 'Random', '')
                    $e.InnerText = [string] $Alignment
                    $null = $target.AppendChild($e)

                    if ($PsBoundParameters.ContainsKey('RandomRatio'))
                    {
                        $e = $x.CreateNode([xml.xmlnodetype]::Element, 'RandomRatio', '')
                        $e.InnerText = [string] $RandomRatio
                        $null = $target.AppendChild($e)
                    }
                }

                # Sequential (default if neither specified)
                elseif ($Sequential -or -not $Random)
                {
                    $e = $x.CreateNode([xml.xmlnodetype]::Element, 'StrideSize', '')
                    $e.InnerText = [string] $Alignment
                    $null = $target.AppendChild($e)

                    #
                    # Remove any distribution attached to the target - not compatibile (or relevant);
                    # sequential is sequential.
                    #

                    $dists = $target.SelectNodes("Distribution")
                    foreach ($dist in $dists)
                    {
                        $null = $target.RemoveChild($dist)
                    }
                }
            }
        }
    }

    $x
}

function Set-FleetProfile
{
    [CmdletBinding()]
    param(
        [Parameter(ValueFromPipeline = $true, Mandatory = $true)]
        [xml]
        $ProfileXml,

        [Parameter()]
        [uint32]
        $Throughput,

        [Parameter()]
        [ValidateSet("IOPS","BPMS")]
        [string]
        $ThroughputUnit = "IOPS",

        [Parameter()]
        [uint32]
        $Warmup,

        [Parameter()]
        [uint32]
        $Duration,

        [Parameter()]
        [uint32]
        $Cooldown
    )

    process
    {
        $x = $ProfileXml.Clone()

        foreach ($timeSpan in $x.SelectNodes("Profile/TimeSpans/TimeSpan"))
        {
            if ($PSBoundParameters.ContainsKey('Warmup'))
            {
                $timeSpan.Warmup = [string] $Warmup
            }

            if ($PSBoundParameters.ContainsKey('Cooldown'))
            {
                $timeSpan.Cooldown = [string] $Cooldown
            }

            if ($PSBoundParameters.ContainsKey('Duration'))
            {
                $timeSpan.Duration = [string] $Duration
            }

            if ($PSBoundParameters.ContainsKey('Throughput'))
            {
                # Fail if timespan is in threadpool form - DISKSPD does not support
                # throughput in this case.

                $t = $timeSpan.SelectSingleNode("ThreadCount")
                if ($null -ne $t -and $t.InnerText -ne 0)
                {
                    throw "Cannot set bounded throughput on profile using thread pool (-F/TimeSpan ThreadCount)"
                }

                foreach ($targetSet in $timeSpan.SelectNodes("Targets"))
                {
                    #
                    # Pass 1 - Total
                    #

                    $total = [uint32] 0
                    $nTargets = 0
                    foreach ($target in $targetSet.Target)
                    {
                        $thisTput = 0
                        $e = $target.SelectSingleNode("Throughput")
                        if ($null -ne $e)
                        {
                            $thisTput = [uint32] $e.InnerText

                            $e = $target.SelectSingleNode("ThreadsPerFile")
                            if ($null -eq $e -or ([uint32] $e.InnerText) -eq 0)
                            {
                                throw "ThreadsPerFile is not present - zero or absent - to scale Throughput (target $nTargets)"
                            }

                            $thisTput *= [uint32] $e.InnerText
                        }

                        if (($total -ne 0 -and $thisTput -eq 0) -or
                            ($total -eq 0 -and $thisTput -ne 0 -and $nTargets -ne 0))
                        {
                            throw "Cannot set throughput on profile with a combination of bounded/unbounded targets (target $nTargets)"
                        }

                        $total += $thisTput
                        ++$nTargets
                    }

                    # If there is no throughout specified (unbounded) and no ratio specified
                    # in the profile, we are done - already unbounded.

                    if ($Throughput -eq 0 -and $total -eq 0) { continue }

                    #
                    # Pass 2a - Distribute nonzero by ratio, as well as zero to single target
                    #

                    if ($Throughput -ne 0 -or $targets.Count -eq 1)
                    {
                        foreach ($target in $targetSet.Target)
                        {

                            # Distribute equally
                            if ($total -eq 0)
                            {
                                SetSingleNode -Xml $x -ParentNode $target -Node 'Throughput' -Value ([int] ($Throughput / $nTargets))
                                $e = $target.SelectSingleNode("Throughput")
                            }

                            # Distribute proportionally
                            # Note: in absolute terms the numerator $thisTput should be scaled by #threads to arrive at
                            # a fraction the new throughput, but then we would immediatly divide #threads out of the result
                            # to arrive at per thread throughput; this can be avoided.
                            else
                            {
                                $e = $target.SelectSingleNode("Throughput")
                                $thisTput = [uint32] $e.InnerText
                                $e.InnerText = [string] [int] ($Throughput * ($thisTput / $total))
                            }

                            $e.SetAttribute('unit', $ThroughputUnit)
                        }
                    }

                    # Pass 2b - Distribute "zero" across multiple by converting target threads to pool
                    #           with weighted ratio of throughputs on targets. Set interlocked sequential
                    #           on sequential targets unless a nonzero threadstride is already present.
                    #           This can result in a close approximation of unbounded result on the tput
                    #           limited specification without needing dynamic scale-up, but should be used
                    #           with caution.
                    else
                    {
                        # Loop targets to count threads, move tputs to weights and set interlocked sequential.

                        $nThreads = 0
                        foreach ($target in $targetSet.Target)
                        {
                            # Count threads
                            $e = $target.SelectSingleNode("ThreadsPerFile")
                            if ($null -eq $e -or $e.InnerText -eq 0)
                            {
                                throw "Invalid -t/ThreadsPerFile specification (absent or zero) converting to unbounded form"
                            }
                            $thisThreads = [uint32] $e.InnerText
                            $nThreads += $thisThreads

                            # Remove ThreadsPerFile/RequestCount @ Target
                            $null = $target.RemoveChild($e)
                            $e = $target.SelectSingleNode("RequestCount")
                            if ($null -ne $e) { $null = $target.RemoveChild($e) }

                            # Move Throuhput to Weight. Note - Throughput guaranteed to exist or we would have
                            # returned at the 0/0 check. Weight scales by number of threads on the target.
                            $e = $target.SelectSingleNode("Throughput")
                            $thisTput = [uint32] $e.InnerText
                            $null = $target.RemoveChild($e)

                            SetSingleNode -Xml $x -ParentNode $target -Node 'Weight' -Value ($thisTput * $thisThreads)

                            # Sequential target without ThreadStride? Move to interlocked.
                            $e = $target.SelectSingleNode("StrideSize")
                            if ($null -ne $e)
                            {
                                $e = $target.SelectSingleNode("ThreadStride")
                                if ($null -eq $e -or $e.InnerText -eq 0)
                                {
                                    SetSingleNode -Xml $x -ParentNode $target -Node 'InterlockedSequential' -Value 'true'
                                }
                            }
                        }

                        # Now set TimeSpan level thread pool with high request count

                        SetSingleNode -Xml $x -ParentNode $timeSpan -Node 'ThreadCount' -Value $nThreads
                        SetSingleNode -Xml $x -ParentNode $timeSpan -Node 'RequestCount' -Value 32
                    }
                }
            }
        }

        $x
    }
}
function GetFleetProfileFootprint
{
    [CmdletBinding()]
    param(
        [Parameter(ValueFromPipeline = $true, Mandatory = $true)]
        [xml]
        $ProfileXml,

        [Parameter()]
        [switch]
        $Read
        )

    $r = @{}

    # Capture min base offset and max max offset across timespans for each target.
    # Note that 0 max offset indicates it is not bounded (bounded by target size).

    foreach ($target in $ProfileXml.SelectNodes("Profile/TimeSpans/TimeSpan/Targets/Target"))
    {
        # Skip write-only target specs if only interested in read workingset
        if ($Read -and $target.WriteRatio -eq '100')
        {
            continue
        }

        $bo = [uint64](GetSingleNode $target 'BaseFileOffset')
        $mo = [uint64](GetSingleNode $target 'MaxFileSize')

        $node = $r[$target.Path]

        if ($null -ne $node)
        {
            if ($node.BaseOffset -gt $bo)
            {
                $r[$target.Path].BaseOffset = $bo
            }

            if ($mo -eq 0)
            {
                $r[$target.Path].MaxOffset = 0
            }
            elseif ($node.MaxOffset -ne 0 -and $node.MaxOffset -lt $mo)
            {
                $r[$target.Path].MaxOffset = $mo
            }
        }
        else
        {
            $r[$target.Path] = [pscustomobject] @{
                BaseOffset = $bo
                MaxOffset = $mo
            }
        }
    }

    $r
}

function Convert-FleetXmlToString
{
    [CmdletBinding()]
    param(
        [Parameter(ValueFromPipeline = $true, Mandatory = $true)]
        [object]
        $InputObject
    )

    process {

        switch ($InputObject.GetType().FullName)
        {
            "System.Xml.XmlDocument"    { break }
            "System.Xml.XmlElement"     { break }
            default {
                throw "Unknown object type $_ - must be XmlDocument or XmlElement"
            }
        }

        $sw = [System.IO.StringWriter]::new()
        $xo = [System.Xml.XmlTextWriter]::new($sw)
        $xo.Formatting = [System.Xml.Formatting]::Indented

        $InputObject.WriteTo($xo)
        $sw.ToString()
    }
}

function FitClockRate
{
    param(
        [ValidateRange(1,(1000*60*60))]
        [uint32]
        $TotalSeconds,

        [uint32]
        $Samples
        )

    # Perform a best-fit for the longest interval which produces at least $Samples over
    # the given $TotalSeconds time period and evenly divide minutes/hours. E.g.:
    #   1..60 |? { 60 % $_ -eq 0 })
    #
    # Range is sanity capped at 1000h.

    $div = 1,2,3,4,5,6,10,12,15,20,30,60
    $last = 1

    # First multiple is seconds, second produces minutes.

    foreach ($mul in (1,60))
    {
        foreach ($d in $div)
        {
            # Skip 60s and roll over to 1 minute.
            # 60 is only used for minutes. (e.g., 1hr)
            if ($mul -eq 1 -and $d -eq 60 ) { break }

            if (($TotalSeconds / ($d * $mul)) -lt $Samples)
            {
                return $last
            }

            $last = $d * $mul
        }
    }

    return $last
}
function SetSingleNode
{
    param(
        [xml]
        $Xml,

        [xml.xmlelement]
        $ParentNode,

        [string]
        $Node,

        [string]
        $Value
    )

    # Set/Create child node to given value

    $e = $ParentNode.SelectSingleNode($Node)
    if ($null -ne $e)
    {
        $e.InnerText = $Value
    }
    else
    {
        $e = $Xml.CreateNode([xml.xmlnodetype]::Element, $Node, '')
        $e.InnerText = $Value
        $null = $ParentNode.AppendChild($e)
    }
}

function GetSingleNode
{
    param(
        [xml.xmlelement]
        $ParentNode,

        [string]
        $Node
    )

    # Gete child node, if present

    $e = $ParentNode.SelectSingleNode($Node)
    if ($null -ne $e)
    {
        return $e.InnerText
    }

    return $null
}

function IsProfileSingleTimespan
{
    param(
        [xml]
        $ProfileXml
    )

    $ts = @($ProfileXml.SelectNodes("Profile/TimeSpans/TimeSpan"))
    return ($ts.Count -eq 1)
}

function IsProfileThroughputLimited
{
    param(
        [xml]
        $ProfileXml
    )

    $tputs = @($ProfileXml.SelectNodes("Profile/TimeSpans/TimeSpan/Targets/Target/Throughput"))

    foreach ($t in $tputs)
    {
        if (([uint32]$t.InnerText) -ne 0)
        {
            return $true
        }
    }

    return $false
}

function IsProfileSingleTarget
{
    param(
        [xml]
        $ProfileXml
    )

    $tgts = @($ProfileXml.SelectNodes("Profile/TimeSpans/TimeSpan/Targets/Target"))
    return ($tgts.Count -eq 1)
}