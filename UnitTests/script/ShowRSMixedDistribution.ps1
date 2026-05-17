#
# This command will take a wpr trace of a diskspd run and post-process it for random/sequential runs and
# lengths of sequential runs - multiple threads/threads are handled. Its report can be used as a check on -rs
# distributions and type-homogeneity of sequential runs (read and write-only).
#
# Take care to understand the comment on homogeneity. Turning this into the basis of a fully automated check
# would be useful.
#
# Probability of a false homogeneous flag is related to the load file size, number of total IOs, random%,
# read/write% and IO size/alignment. Discounting the final block of the file:
#
# p = 2 * Prandom * (1 / (File Size / IO Alignment)) * Pread * (1 - Pread)
# ... Probability of a random IO occuring sequential to the prior IO
# ... * probability of first being a read and second being a write
# ... * and in the other order, * 2
#
# For example, for a 10MiB file with 8K IOs we have 1280 distinct offsets
# which can be chosen. With a 50/50 r/s mix and a 70:30 r/w mix we have
# p = 2 * 0.5 * (1/1280) * 0.7 * 0.3 =~ 0.000164063
#
# Over ~220,000 IOs we expect 220000*p =~ 36 IOs to false flag. In a direct
# experiment we saw 35. It would be good to establish 5-sigma bounds on the
# expectation and then consider automating a check.
#
#
<#
    Total IOs: 219316

    IO Distributions
    Random

    Thread  Read Write Total
    ------  ---- ----- -----
    27440 38327 16340 54667
    28232 38531 16527 55058


    Sequential

    Thread  Read Write Total
    ------  ---- ----- -----
    27440 38186 16198 54384
    28232 38636 16571 55207


    Length Distribution

    Thread     2    3    4    5   6   7   8   9 10 11 12 13 14 15 16 17 18 19 20
    ------     -    -    -    -   -   -   -   - -- -- -- -- -- -- -- -- -- -- --
    27440 13776 6673 3338 1682 882 462 194 116 62 26 10  7  3
    28232 13695 6956 3421 1724 842 445 212 120 53 29 13 11  3     1


    Homogeneous Check: 35 IOs were found
#>

[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)]
    [string]
    $ETLFile,

    [Parameter(Mandatory = $true)]
    [string[]]
    $LoadFile,

    [switch]
    $HomogeneousSequential
    )

Set-StrictMode -Version 3.0

if (-not (Get-Command wpaexporter))
{
    Write-Error "wpaexporter not on the path; please make the command available to this tool"
    return
}

#
# Profile producing the filtered File IO event table. Take this content, save to a .wpaProfile and
# import to WPA to modify/inspect.
#
# Grouping: File Path / Thread / Process (Name)
# Data Col: Event Type / Start (s) / Size (B) / Offset
#
# Original filter stated: [File Path]:~=&quot;foo.bin&quot;
# Replaced with __FILEFILTER__ for dynamic subst
#

$wpaprof = @"
<?xml version="1.0" encoding="utf-8"?>
<WpaProfileContainer xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" xmlns:xsd="http://www.w3.org/2001/XMLSchema" Version="2" xmlns="http://tempuri.org/SerializableElement.xsd">
  <Content xsi:type="WpaProfile2">
    <Sessions>
      <Session Index="0">
        <FileReferences />
      </Session>
    </Sessions>
    <Views>
      <View Guid="7882feeb-60c3-4546-bbc7-8de65122a627" IsVisible="true" Title="Analysis">
        <Graphs>
          <Graph Guid="e2a7d340-0dbd-4f24-8b26-e5388eeaf562" LayoutStyle="All" Color="#FF008000" GraphHeight="125.00000000000006" IsMinimized="false" IsShown="true" IsExpanded="false">
            <Preset Name="Count by Type" GraphChartType="StackedBars" BarGraphIntervalCount="50" IsThreadActivityTable="false" GraphColumnCount="10" KeyColumnCount="3" LeftFrozenColumnCount="5" RightFrozenColumnCount="9" InitialFilterQuery="[Process (Name)]:~=&quot;diskspd.exe&quot; __FILEFILTER__ ([Event Type]:=&quot;Read&quot; OR [Event Type]:=&quot;Write&quot;)" InitialFilterShouldKeep="true" InitialExpansionQuery="[Process (Name)]:=&quot;diskspd.exe&quot;" GraphFilterColumnGuid="0f0fd63e-942e-441d-829b-3f921965cc9a" GraphFilterTopValue="0" GraphFilterThresholdValue="0">
              <MetadataEntries>
                <MetadataEntry Guid="26d231f4-e6e3-4ea0-b8f1-3c3cfacf067b" Name="Start" ColumnMetadata="StartTime" />
              </MetadataEntries>
              <HighlightEntries />
              <Columns>
                <Column Guid="0b043700-c82e-4d03-bc9c-ae7138093b47" Name="File Path" SortPriority="2" Width="500" IsVisible="true" />
                <Column Guid="71a12a2f-ac40-4906-a979-5760738e4ad7" Name="Thread" SortPriority="14" TextAlignment="Right" Width="69" IsVisible="true" />
                <Column Guid="01cb52ab-5bb1-4cb6-9cff-e50ce2836e39" Name="Process" SortPriority="1" Width="150" IsVisible="true">
                  <ProcessOptionsParameter Mode="NoPid" />
                </Column>
                <Column Guid="db4da22c-8331-4243-9094-44f05969de01" Name="Event Type" SortPriority="3" Width="89" IsVisible="true" />
                <Column Guid="26d231f4-e6e3-4ea0-b8f1-3c3cfacf067b" Name="Start" AggregationMode="Min" SortOrder="Ascending" SortPriority="0" TextAlignment="Right" Width="93" IsVisible="true">
                  <DateTimeTimestampOptionsParameter DateTimeEnabled="false" />
                </Column>
                <Column Guid="8b8d220b-a24d-4773-81cb-6c6caebb084f" Name="Size" AggregationMode="Sum" SortPriority="19" TextAlignment="Right" Width="60" CellFormat="B" IsVisible="true" />
                <Column Guid="562233cc-2feb-4798-8fe9-bccdd1713c79" Name="Offset" SortPriority="20" TextAlignment="Right" Width="60" IsVisible="true" />
              </Columns>
            </Preset>
          </Graph>
        </Graphs>
        <SessionIndices>
          <SessionIndex>0</SessionIndex>
        </SessionIndices>
      </View>
    </Views>
    <ModifiedGraphs>
      <GraphSchema Guid="e2a7d340-0dbd-4f24-8b26-e5388eeaf562">
        <ModifiedPresets />
        <PersistedPresets>
          <Preset Name="Count by Type" GraphChartType="StackedBars" BarGraphIntervalCount="50" IsThreadActivityTable="false" GraphColumnCount="10" KeyColumnCount="3" LeftFrozenColumnCount="5" RightFrozenColumnCount="9" InitialFilterQuery="[Process (Name)]:~=&quot;diskspd.exe&quot; __FILEFILTER__ ([Event Type]:=&quot;Read&quot; OR [Event Type]:=&quot;Write&quot;)" InitialFilterShouldKeep="true" InitialExpansionQuery="[Process (Name)]:=&quot;diskspd.exe&quot;" GraphFilterColumnGuid="0f0fd63e-942e-441d-829b-3f921965cc9a" GraphFilterTopValue="0" GraphFilterThresholdValue="0">
            <MetadataEntries>
              <MetadataEntry Guid="26d231f4-e6e3-4ea0-b8f1-3c3cfacf067b" Name="Start" ColumnMetadata="StartTime" />
            </MetadataEntries>
            <HighlightEntries />
            <Columns>
              <Column Guid="0b043700-c82e-4d03-bc9c-ae7138093b47" Name="File Path" SortPriority="2" Width="500" IsVisible="true" />
              <Column Guid="71a12a2f-ac40-4906-a979-5760738e4ad7" Name="Thread" SortPriority="14" TextAlignment="Right" Width="69" IsVisible="true" />
              <Column Guid="01cb52ab-5bb1-4cb6-9cff-e50ce2836e39" Name="Process" SortPriority="1" Width="150" IsVisible="true">
                <ProcessOptionsParameter Mode="NoPid" />
              </Column>
              <Column Guid="db4da22c-8331-4243-9094-44f05969de01" Name="Event Type" SortPriority="3" Width="89" IsVisible="true" />
              <Column Guid="26d231f4-e6e3-4ea0-b8f1-3c3cfacf067b" Name="Start" AggregationMode="Min" SortOrder="Ascending" SortPriority="0" TextAlignment="Right" Width="93" IsVisible="true">
                <DateTimeTimestampOptionsParameter DateTimeEnabled="false" />
              </Column>
              <Column Guid="8b8d220b-a24d-4773-81cb-6c6caebb084f" Name="Size" AggregationMode="Sum" SortPriority="19" TextAlignment="Right" Width="60" CellFormat="B" IsVisible="true" />
              <Column Guid="562233cc-2feb-4798-8fe9-bccdd1713c79" Name="Offset" SortPriority="20" TextAlignment="Right" Width="60" IsVisible="true" />
            </Columns>
          </Preset>
        </PersistedPresets>
      </GraphSchema>
    </ModifiedGraphs>
  </Content>
</WpaProfileContainer>
"@

# File IO Table column -> type map
$typeMap = @{
    'Thread' = 'ULong';
    'Start (s)' = 'TimeStamp_s';
    'Size (B)' = 'Ulong';
    'Offset' = 'ULongLong'
}
    
function GetMappableProperties
{
    [CmdletBinding()]
    param(
        [Parameter(Mandatory = $true)]
        [hashtable]
        $TypeMap
    )

    $mappableTypes = @("ULong","ConstOne","ULongLong","TimeStamp_s")

    foreach ($prop in $TypeMap.Keys) {
        if ($TypeMap[$prop] -in $mappableTypes) {
            $prop
        }
    }
}

function ApplyTypeMap
{
    [CmdletBinding()]
    param(
        [Parameter(Mandatory = $true)]
        [hashtable]
        $TypeMap,

        [Parameter(ValueFromPipeline = $true)]
        [object]
        $InputObject
    )

    begin {
        $propertyNames = GetMappableProperties $TypeMap
    }

    process {

        foreach ($property in $propertyNames) {

            $tmpp = $InputObject.$property
            $tmp = $null

            switch ($typeMap[$property]) {
                "ULong" {
                    # knock out commas from n,nnn,nnn forms
                    $tmpp = $tmpp -replace ',',''
                    if ([uint32]::TryParse($tmpp,[ref]$tmp)) { $InputObject.$property = $tmp }
                }
                "ConstOne" {
                    if ([uint32]::TryParse($tmpp,[ref]$tmp)) { $InputObject.$property = $tmp }
                }
                "ULongLong" {
                    # knock out commas from n,nnn,nnn forms
                    $tmpp = $tmpp -replace ',',''
                    if ([uint64]::TryParse($tmpp,[ref]$tmp)) { $InputObject.$property = $tmp }
                }
                "TimeStamp_s" {
                    if ([decimal]::TryParse($tmpp,[ref]$tmp)) { $InputObject.$property = $tmp }
                }
                default { }
            }
        }

        $InputObject
    }
}

function AddEmptyIf
{
    param(
        [hashtable]
        $Hash,

        [object]
        $Key,

        [object]
        $Set
        )

    if (-not $Hash.Contains($Key)) {
        $Hash[$Key] = $Set
    }
}

# per thread last IO offset and type
$lastOffset = @{}
$lastType = @{}

# counts of random/sequential IO and sequential run lengths > 1, per thread
$rndio = @{}
$seqio = @{}
$slens = @{}
$slen = @{}
$numio = 0

$homogeneousck = 0

try {

    $t = New-TemporaryFile
    Remove-Item $t
    $null = New-Item -ItemType Directory $t

    #
    # Produce the file filter & dynamic profile
    #

    $filefilter = @()
    
    foreach ($l in $LoadFile) {
        $filefilter += "[File Path]:~=&quot;$l&quot;"
    }

    $filefilter = "(" + ($filefilter -join 'OR') + ")"
    
    # Insert into profile for wpaexporter
    $wpaprof -split "`n" |% {
        $_ -replace '__FILEFILTER__',$filefilter
    } | Out-File $t\profile.wpaProfile -Encoding ascii -Width 9999

    $capture = wpaexporter -profile $t\profile.wpaProfile -outputformat csv -outputfolder $t -i $ETLFile 2>&1
    if ($?)
    {
        $capture
        return
    }

    # only parse event rows, not aggregate
    # offset is a convenient value in a full event row
    Import-Csv $t\File_I_O_Count_by_Type.csv |? Offset -ne '' | ApplyTypeMap -TypeMap $typeMap |% {

        $numio += 1
        $ev = $_
    
        if ($ev.Offset -ne $lastOffset[$ev.Thread] + $ev.'Size (B)') {
            AddEmptyIf $rndio $ev.Thread @{}
            $rndio[$ev.Thread][$ev."Event Type"] += 1
    
            if ($slen[$ev.Thread] -gt 1)
            {
                # Populate length hash if this is the first run.
                AddEmptyIf $slens $ev.Thread @{}
                $slens[$ev.Thread][$slen[$ev.Thread]] += 1
                $slen[$ev.Thread] = 0
            }
    
            $slen[$ev.Thread] = 1
        } else {
            # assert homogeneous sequential runs
            # NOTE: with moderate file sizes we EXPECT some fraction of random IOs to be sequential
            #  to the previous IO AND if this IO is of different type, we would flag it here.
            if ($HomogeneousSequential -and $ev."Event Type" -ne $lastType[$ev.Thread]) {
                Write-Verbose "failed homogeneous check $ev"
                $homogeneousck += 1
            }
    
            AddEmptyIf $seqio $ev.Thread @{}
            $seqio[$ev.Thread][$ev."Event Type"] += 1
    
            $slen[$ev.Thread] += 1
        }
    
        $lastOffset[$ev.Thread] = $ev.Offset
        $lastType[$ev.Thread] = $ev."Event Type"
    }

} finally {

    Remove-Item -Recurse -Force $t
}

function FlattenHash
{
    param(
        [hashtable] $Hash,
        [string]$KeyName,
        [object[]]$Property,
        [switch] $Total
    )

    # object per primary key
    $Hash.Keys |% { New-Object PsObject -Property @{ $KeyName = $_ }} |% {

        # note: total only aggregates as numerical type
        $t = 0

        # flatten the k/v pairs as properties of the object
        foreach ($p in $Property) {
            $_ | Add-Member -MemberType NoteProperty -Name $p -Value $Hash[$_.$KeyName][$p]
            if ($Total) {
                $t += $Hash[$_.$KeyName][$p]
            }
        }
        if ($Total) {
            $_ | Add-Member -MemberType NoteProperty -Name Total -Value $t
        }
        $_
    }
}
Write-Host "Total IOs: $numio`n"
Write-Host "IO Distributions"
Write-Host "Random"
FlattenHash $rndio Thread Read,Write -Total | ft Thread,Read,Write,Total
Write-Host "Sequential"
FlattenHash $seqio Thread Read,Write -Total | ft Thread,Read,Write,Total

Write-Host "Length Distribution"
FlattenHash $slens Thread (2..20) | ft (,'Thread'+(2..20 |% { [string]$_}))

if ($HomogeneousSequential) {
    Write-Host "Homogeneous Check: $homogeneousck IOs were found"
}