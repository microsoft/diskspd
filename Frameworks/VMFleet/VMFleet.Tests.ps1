Describe "TestModulePresent" {

    BeforeAll {

        $script:e = $null
    }

    if (Test-Path .\VMFleet.psd1)
    {
        It "ShouldLoad-Local" {
            { Import-Module .\VMFleet.psd1 -Force -ErrorVariable script:e } | Should Not Throw
            $script:e | Should BeNullOrEmpty Because "this should be successful"
        }
    }
    else
    {
        It "ModuleExists-Installed" {
            Get-Module VMFleet -ListAvailable -ErrorVariable script:e | Should Not BeNullOrEmpty Because "module should be installed on system"
            $script:e | Should BeNullOrEmpty Because "this should be successful"
        }

        It "ShouldLoad-Installed" {
            { Import-Module VMFleet -Force -ErrorVariable script:e } | Should Not Throw
            $script:e | Should BeNullOrEmpty Because "this should be successful"
        }
    }

    It "ShouldBeLoaded" {
        Get-Module VMFleet -ErrorVariable script:e | Should Not BeNullOrEmpty Because "module should now be loaded"
        $script:e | Should BeNullOrEmpty Because "this should be successful"
    }
}

InModuleScope VMFleet {

    Describe "GetDistributedShift" {

        BeforeAll {

            $a = @('a', 'b', 'c', 'd', 'e')
            $b = @('f', 'g', 'h', 'i', 'j')
            $c = @('k', 'l', 'm', 'n', 'o')
            $d = @('p', 'q', 'r', 's', 't')

            $bad = @('z')

            $a20 = 1..20 |% { "a$_" }
            $b20 = 1..20 |% { "b$_" }
            $c20 = 1..20 |% { "c$_" }
            $d20 = 1..20 |% { "d$_" }
        }

        It "ShouldRequireListOfLists" {
            GetDistributedShift -Group $a -N 1 | Should Throw
        }

        It "ShouldRequireMoreThanOne" {
            GetDistributedShift -Group @($a) -N 1 | Should Throw
        }

        It "ShouldRequireAllSameSize" {
            GetDistributedShift -Group $a,$b,$bad -N 1 | Should Throw
        }

        It "ShouldRequirePositiveRotation" {
            GetDistributedShift -Group $a,$b -N 0 | Should Throw
        }

        It "ShouldNotShiftTooMany" {
            GetDistributedShift -Group $a,$b -N 6 | Should Throw
        }

        #
        # Two group rotations
        #

        It "Rotate2-1" {
            { GetDistributedShift -Group $a,$b -N 1 } | Should Not Throw
            $a1, $b1 = GetDistributedShift -Group $a,$b -N 1
            $a1.Count | Should Be 5
            $b1.Count | Should Be 5

            ($a1 -join '') | Should Be "abcdj"
            ($b1 -join '') | Should Be "fghie"
        }

        It "Rotate2-2" {
            { GetDistributedShift -Group $a,$b -N 2 } | Should Not Throw
            $a1, $b1 = GetDistributedShift -Group $a,$b -N 2
            $a1.Count | Should Be 5
            $b1.Count | Should Be 5

            ($a1 -join '') | Should Be "abcij"
            ($b1 -join '') | Should Be "fghde"
        }

        It "Rotate2-3" {
            { GetDistributedShift -Group $a,$b -N 3 } | Should Not Throw
            $a1, $b1 = GetDistributedShift -Group $a,$b -N 3
            $a1.Count | Should Be 5
            $b1.Count | Should Be 5

            ($a1 -join '') | Should Be "abhij"
            ($b1 -join '') | Should Be "fgcde"
        }

        It "Rotate2-4" {
            { GetDistributedShift -Group $a,$b -N 4 } | Should Not Throw
            $a1, $b1 = GetDistributedShift -Group $a,$b -N 4
            $a1.Count | Should Be 5
            $b1.Count | Should Be 5

            ($a1 -join '') | Should Be "aghij"
            ($b1 -join '') | Should Be "fbcde"
        }

        It "Rotate2-5" {
            { GetDistributedShift -Group $a,$b -N 5 } | Should Not Throw
            $a1, $b1 = GetDistributedShift -Group $a,$b -N 5
            $a1.Count | Should Be 5
            $b1.Count | Should Be 5

            ($a1 -join '') | Should Be "fghij"
            ($b1 -join '') | Should Be "abcde"
        }

        #
        # Three group rotations
        #

        It "Rotate3-1" {
            { GetDistributedShift -Group $a,$b,$c -N 1 } | Should Not Throw
            $a1, $b1, $c1 = GetDistributedShift -Group $a,$b,$c -N 1
            $a1.Count | Should Be 5
            $b1.Count | Should Be 5
            $c1.Count | Should Be 5

            ($a1 -join '') | Should Be "abcdo"
            ($b1 -join '') | Should Be "fghie"
            ($c1 -join '') | Should Be "klmnj"
        }

        It "Rotate3-2" {
            { GetDistributedShift -Group $a,$b,$c -N 2 } | Should Not Throw
            $a1, $b1, $c1 = GetDistributedShift -Group $a,$b,$c -N 2
            $a1.Count | Should Be 5
            $b1.Count | Should Be 5
            $c1.Count | Should Be 5

            ($a1 -join '') | Should Be "abcio"
            ($b1 -join '') | Should Be "fghne"
            ($c1 -join '') | Should Be "klmdj"
        }

        It "Rotate3-3" {
            { GetDistributedShift -Group $a,$b,$c -N 3 } | Should Not Throw
            $a1, $b1, $c1 = GetDistributedShift -Group $a,$b,$c -N 3
            $a1.Count | Should Be 5
            $b1.Count | Should Be 5
            $c1.Count | Should Be 5

            ($a1 -join '') | Should Be "abmio"
            ($b1 -join '') | Should Be "fgcne"
            ($c1 -join '') | Should Be "klhdj"
        }

        It "Rotate3-4" {
            { GetDistributedShift -Group $a,$b,$c -N 4 } | Should Not Throw
            $a1, $b1, $c1 = GetDistributedShift -Group $a,$b,$c -N 4
            $a1.Count | Should Be 5
            $b1.Count | Should Be 5
            $c1.Count | Should Be 5

            ($a1 -join '') | Should Be "agmio"
            ($b1 -join '') | Should Be "flcne"
            ($c1 -join '') | Should Be "kbhdj"
        }

        It "Rotate3-5" {
            { GetDistributedShift -Group $a,$b,$c -N 5 } | Should Not Throw
            $a1, $b1, $c1 = GetDistributedShift -Group $a,$b,$c -N 5
            $a1.Count | Should Be 5
            $b1.Count | Should Be 5
            $c1.Count | Should Be 5

            ($a1 -join '') | Should Be "kgmio"
            ($b1 -join '') | Should Be "alcne"
            ($c1 -join '') | Should Be "fbhdj"
        }

        It "RotateLarge4-1" {
            { GetDistributedShift -Group $a20,$b20,$c20,$d20 -N 1 } | Should Not Throw
            $g = GetDistributedShift -Group $a20,$b20,$c20,$d20 -N 1

            # note last group rotates 1
            $g[0] -join '' | Should Be "a1a2a3a4a5a6a7a8a9a10a11a12a13a14a15a16a17a18a19d20"
            $g[1] -join '' | Should Be "b1b2b3b4b5b6b7b8b9b10b11b12b13b14b15b16b17b18b19a20"
            $g[2] -join '' | Should Be "c1c2c3c4c5c6c7c8c9c10c11c12c13c14c15c16c17c18c19b20"
            $g[3] -join '' | Should Be "d1d2d3d4d5d6d7d8d9d10d11d12d13d14d15d16d17d18d19c20"
        }

        It "RotateLarge4-2" {
            { GetDistributedShift -Group $a20,$b20,$c20,$d20 -N 2 } | Should Not Throw
            $g = GetDistributedShift -Group $a20,$b20,$c20,$d20 -N 2

            # note last group rotates 1, next 2
            $g[0] -join '' | Should Be "a1a2a3a4a5a6a7a8a9a10a11a12a13a14a15a16a17a18c19d20"
            $g[1] -join '' | Should Be "b1b2b3b4b5b6b7b8b9b10b11b12b13b14b15b16b17b18d19a20"
            $g[2] -join '' | Should Be "c1c2c3c4c5c6c7c8c9c10c11c12c13c14c15c16c17c18a19b20"
            $g[3] -join '' | Should Be "d1d2d3d4d5d6d7d8d9d10d11d12d13d14d15d16d17d18b19c20"
        }

        It "RotateLarge4-3" {
            { GetDistributedShift -Group $a20,$b20,$c20,$d20 -N 3 } | Should Not Throw
            $g = GetDistributedShift -Group $a20,$b20,$c20,$d20 -N 3

            # note last group rotates 1, then 2, 3
            $g[0] -join '' | Should Be "a1a2a3a4a5a6a7a8a9a10a11a12a13a14a15a16a17b18c19d20"
            $g[1] -join '' | Should Be "b1b2b3b4b5b6b7b8b9b10b11b12b13b14b15b16b17c18d19a20"
            $g[2] -join '' | Should Be "c1c2c3c4c5c6c7c8c9c10c11c12c13c14c15c16c17d18a19b20"
            $g[3] -join '' | Should Be "d1d2d3d4d5d6d7d8d9d10d11d12d13d14d15d16d17a18b19c20"
        }

        It "RotateLarge4-4" {
            { GetDistributedShift -Group $a20,$b20,$c20,$d20 -N 4 } | Should Not Throw
            $g = GetDistributedShift -Group $a20,$b20,$c20,$d20 -N 4

            # note last group rotates 1, then 2, 3, and back to 1
            $g[0] -join '' | Should Be "a1a2a3a4a5a6a7a8a9a10a11a12a13a14a15a16d17b18c19d20"
            $g[1] -join '' | Should Be "b1b2b3b4b5b6b7b8b9b10b11b12b13b14b15b16a17c18d19a20"
            $g[2] -join '' | Should Be "c1c2c3c4c5c6c7c8c9c10c11c12c13c14c15c16b17d18a19b20"
            $g[3] -join '' | Should Be "d1d2d3d4d5d6d7d8d9d10d11d12d13d14d15d16c17a18b19c20"
        }
    }

    Describe "FilterObject" {

        BeforeAll {
            $o12 = [PSCustomObject]@{
                K1 = 1
                K2 = 2
            }
            $o13 = [PSCustomObject]@{
                K1 = 1
                K2 = 3
            }
        }

        It "PassWithEmpty" {
            $o12 | FilterObject -Filter @{} | Should Not BeNullOrEmpty
        }
        It "PassWithOneK" {
            $o12 | FilterObject -Filter @{ K1 = 1 } | Should Not BeNullOrEmpty
        }
        It "PassWithTwoK" {
            $o12 | FilterObject -Filter @{ K1 = 1; K2 = 2 } | Should Not BeNullOrEmpty
        }
        It "PassWithOneKDiffType" {
            $o12 | FilterObject -Filter @{ K1 = '1' } | Should Not BeNullOrEmpty
        }
        It "PassWithTwoKDiffType" {
            $o12 | FilterObject -Filter @{ K1 = '1'; K2 = '2' } | Should Not BeNullOrEmpty
        }
        It "NotPassMismatchSameType" {
            $o12 | FilterObject -Filter @{ K1 = 2 } | Should BeNullOrEmpty
        }
        It "NotPassMismatchDiffType" {
            $o12 | FilterObject -Filter @{ K1 = '2' } | Should BeNullOrEmpty
        }

        It "ShouldPassMultipleMatch" {
            $r = @($o12,$o12 | FilterObject -Filter @{ K1 = 1 })
            $r.Count | Should Be 2
        }

        It "ShouldPassTheMatch" {
            $r = @($o12,$o13 | FilterObject -Filter @{ K2 = 2 })
            $r.Count | Should Be 1
            $r.K1 | Should Be 1
            $r.K2 | Should Be 2
        }
    }
}

Describe "SetFleetProfile" {

    BeforeAll {

        It "ShouldBeExpectedSqlProfile" {
            { Get-FleetprofileXml -Name SQL } | Should Not Throw
            $x = Get-FleetprofileXml -Name SQL

            $x.Profile.TimeSpans.TimeSpan.Targets.Target[0].Throughput.InnerText | Should Be 1500
            $x.Profile.TimeSpans.TimeSpan.Targets.Target[1].Throughput.InnerText | Should Be 300
        }

        $x = Get-FleetprofileXml -Name SQL

        $oWarmup = $x.Profile.TimeSpans.TimeSpan.Warmup
        $oDuration = $x.Profile.TimeSpans.TimeSpan.Duration
        $oCooldown = $x.Profile.TimeSpans.TimeSpan.Cooldown
    }

    It "ShouldSetByParam" {
        { Set-FleetProfile -ProfileXml $x -Throughput 1000 } | Should Not Throw
        $xn = Set-FleetProfile -ProfileXml $x -Throughput 1000

        # original composition is 1500*4 threads + 300*1 thread = 6300
        # 1000 * 6000/6300 / 4 threads = 238
        # 1000 * 300/6300 / 1 thread = 48
        $xn.Profile.TimeSpans.TimeSpan.Targets.Target[0].Throughput.InnerText | Should Be 238
        $xn.Profile.TimeSpans.TimeSpan.Targets.Target[1].Throughput.InnerText | Should Be 48
    }

    It "ShouldSetByPipeline" {
        { $x | Set-FleetProfile -Throughput 1000 } | Should Not Throw
        $xn = $x | Set-FleetProfile -Throughput 1000

        $xn.Profile.TimeSpans.TimeSpan.Targets.Target[0].Throughput.InnerText | Should Be 238
        $xn.Profile.TimeSpans.TimeSpan.Targets.Target[1].Throughput.InnerText | Should Be 48
    }

    It "ShouldSetUnbounded" {
        { $x | Set-FleetProfile -Throughput 0 } | Should Not Throw
        $xn = $x | Set-FleetProfile -Throughput 0

        $xn.Profile.TimeSpans.TimeSpan.ThreadCount | Should be 5
        $xn.Profile.TimeSpans.TimeSpan.RequestCount | Should be 32

        $xn.Profile.TimeSpans.SelectNodes("TimeSpan/Targets/Target/ThreadsPerFile") | Should BeNullOrEmpty
        $xn.Profile.TimeSpans.SelectNodes("TimeSpan/Targets/Target/RequestCount") | Should BeNullOrEmpty
        $xn.Profile.Timespans.TimeSpan.Targets.Target[1].InterlockedSequential | Should Be 'true'
    }

    It "ShouldSetWarmup" {
        $oWarmup | Should Not Be 33
        { $x | Set-FleetProfile -Warmup 33 } | Should Not Throw
        $xn = $x | Set-FleetProfile -Warmup 33

        $xn.Profile.TimeSpans.TimeSpan.Warmup | Should Be 33
        $xn.Profile.TimeSpans.TimeSpan.Duration | Should Be $oDuration
        $xn.Profile.TimeSpans.TimeSpan.Cooldown | Should Be $oCooldown
    }

    It "ShouldSetDuration" {
        $oDuration | Should Not Be 33
        { $x | Set-FleetProfile -Duration 33 } | Should Not Throw
        $xn = $x | Set-FleetProfile -Duration 33

        $xn.Profile.TimeSpans.TimeSpan.Warmup | Should Be $oWarmup
        $xn.Profile.TimeSpans.TimeSpan.Duration | Should Be 33
        $xn.Profile.TimeSpans.TimeSpan.Cooldown | Should Be $oCooldown
    }

    It "ShouldSetCooldown" {
        $oCooldown | Should Not Be 33
        { $x | Set-FleetProfile -Cooldown 33 } | Should Not Throw
        $xn = $x | Set-FleetProfile -Cooldown 33

        $xn.Profile.TimeSpans.TimeSpan.Warmup | Should Be $oWarmup
        $xn.Profile.TimeSpans.TimeSpan.Duration | Should Be $oDuration
        $xn.Profile.TimeSpans.TimeSpan.Cooldown | Should Be 33
    }
}

Describe "GetFleetProfileXml" {
    It "HasPeak" {
        { $peak = Get-FleetProfileXml -Name Peak -BlockSize 8KB -WriteRatio 0 } | Should Not Throw
        $peak = Get-FleetProfileXml -Name Peak -BlockSize 8KB -WriteRatio 0
        $peak | Should Not BeNullOrEmpty
    }

    It "PeakDoesNotDefineBaseMax" {
        $peak = Get-FleetProfileXml -Name Peak -BlockSize 8KB -WriteRatio 0
        $peak.SelectNodes('Profile/TimeSpans/TimeSpan/Targets/Target/BaseFileOffset') | Should BeNullOrEmpty
        $peak.SelectNodes('Profile/TimeSpans/TimeSpan/Targets/Target/MaxFileSize') | Should BeNullOrEmpty
    }

    It "PeakShouldAcceptBase" {
        $peak = Get-FleetProfileXml -Name Peak -BlockSize 8KB -WriteRatio 0 -BaseOffset 1GB
        $peak.SelectNodes('Profile/TimeSpans/TimeSpan/Targets/Target/MaxFileSize') | Should BeNullOrEmpty
        $n = $peak.SelectNodes('Profile/TimeSpans/TimeSpan/Targets/Target/BaseFileOffset')
        $n | Should Not BeNullOrEmpty
        $n.Count | Should Be 1

        $n.Item(0).InnerText | Should Be ([string] 1GB)
    }

    It "PeakShouldAcceptMax" {
        $peak = Get-FleetProfileXml -Name Peak -BlockSize 8KB -WriteRatio 0 -MaxOffset 2GB
        $peak.SelectNodes('Profile/TimeSpans/TimeSpan/Targets/Target/BaseFileOffset') | Should BeNullOrEmpty
        $n = $peak.SelectNodes('Profile/TimeSpans/TimeSpan/Targets/Target/MaxFileSize')
        $n | Should Not BeNullOrEmpty

        $n.Count | Should Be 1
        $n.Item(0).InnerText | Should Be ([string] 2GB)
    }

    It "PeakShouldAcceptBaseMax" {
        $peak = Get-FleetProfileXml -Name Peak -BlockSize 8KB -WriteRatio 0 -BaseOffset 1GB -MaxOffset 2GB

        $n = $peak.SelectNodes('Profile/TimeSpans/TimeSpan/Targets/Target/BaseFileOffset')
        $n | Should Not BeNullOrEmpty
        $n.Count | Should Be 1
        $n.Item(0).InnerText | Should Be ([string] 1GB)

        $n = $peak.SelectNodes('Profile/TimeSpans/TimeSpan/Targets/Target/MaxFileSize')
        $n | Should Not BeNullOrEmpty
        $n.Count | Should Be 1
        $n.Item(0).InnerText | Should Be ([string] 2GB)
    }

    It "PeakShouldAcceptThreads" {
        $peak = Get-FleetProfileXml -Name Peak -BlockSize 8KB -WriteRatio 0 -ThreadsPerTarget 2

        $n = $peak.SelectNodes('Profile/TimeSpans/TimeSpan/Targets/Target/ThreadsPerFile')
        $n | Should Not BeNullOrEmpty
        $n.Count | Should Be 1
        $n.Item(0).InnerText | Should Be ([string] 2)
    }
}


InModuleScope VMFleet {

    Describe "GetNextSplit" {

        It "ShouldBeLargest@End" {
            $o = [PSCustomObject]@{
                    Value = 0
                    CutoffType =[CutoffType]::No
                },
                [PSCustomObject]@{
                    Value = 5
                    CutoffType =[CutoffType]::No
                },
                [PSCustomObject]@{
                    Value = 15
                    CutoffType =[CutoffType]::No
                },
                [PSCustomObject]@{
                    Value = 40
                    CutoffType =[CutoffType]::No
                }

            $p1, $p2 = GetNextSplit $o 'Value' -OrderBy 'Value'
            $p1.Value | Should Be 40
            $p2.Value | Should Be 15
        }

        It "ShouldBeLargest@Begin" {
            $o = [PSCustomObject]@{
                    Value = 40
                    CutoffType =[CutoffType]::No
                },
                [PSCustomObject]@{
                    Value = 5
                    CutoffType =[CutoffType]::No
                },
                [PSCustomObject]@{
                    Value = 15
                    CutoffType =[CutoffType]::No
                },
                [PSCustomObject]@{
                    Value = 0
                    CutoffType =[CutoffType]::No
                }

            $p1, $p2 = GetNextSplit $o 'Value' -OrderBy 'Value'
            $p1.Value | Should Be 40
            $p2.Value | Should Be 15
        }

        It "ShouldBeFirstOfEqual" {
            $o = [PSCustomObject]@{
                    Value = 0
                    Order = 0
                    CutoffType =[CutoffType]::No
                },
                [PSCustomObject]@{
                    Value = 10
                    Order = 1
                    CutoffType =[CutoffType]::No
                },
                [PSCustomObject]@{
                    Value = 20
                    Order = 2
                    CutoffType =[CutoffType]::No
                },
                [PSCustomObject]@{
                    Value = 30
                    Order = 3
                    CutoffType =[CutoffType]::No
                }

            $p1, $p2 = GetNextSplit $o 'Value' -OrderBy 'Value'
            $p1.Value | Should Be 10
            $p2.Value | Should Be 0
        }

        It "ShouldRespectOrder" {
            $o = [PSCustomObject]@{
                    Value = 0
                    Order = 0
                    CutoffType =[CutoffType]::No
                },
                [PSCustomObject]@{
                    Value = 10
                    Order = 2
                    CutoffType =[CutoffType]::No
                },
                [PSCustomObject]@{
                    Value = 20
                    Order = 3
                    CutoffType =[CutoffType]::No
                },
                [PSCustomObject]@{
                    Value = 30
                    Order = 1
                    CutoffType =[CutoffType]::No
                }

            $p1, $p2 = GetNextSplit $o 'Value' -OrderBy 'Order'
            $p1.Value | Should Be 30
            $p2.Value | Should Be 0
        }

        It "ShouldRespectCutoff" {
            $o = [PSCustomObject]@{
                    Value = 0
                    CutoffType =[CutoffType]::No
                },
                [PSCustomObject]@{
                    Value = 10
                    CutoffType =[CutoffType]::No
                },
                [PSCustomObject]@{
                    Value = 30
                    CutoffType =[CutoffType]::Scale
                },
                [PSCustomObject]@{
                    Value = 60
                    CutoffType =[CutoffType]::Scale
                }

            $p1, $p2 = GetNextSplit $o 'Value' -OrderBy 'Value'
            $p1.Value | Should Be 30
            $p2.Value | Should Be 10
        }

        It "ShouldRespectCutoffOrdered" {
            $o = [PSCustomObject]@{
                    Value = 0
                    CutoffType =[CutoffType]::No
                },
                [PSCustomObject]@{
                    Value = 30
                    CutoffType =[CutoffType]::Scale
                },
                [PSCustomObject]@{
                    Value = 10
                    CutoffType =[CutoffType]::No
                },
                [PSCustomObject]@{
                    Value = 60
                    CutoffType =[CutoffType]::Scale
                }

            $p1, $p2 = GetNextSplit $o 'Value' -OrderBy 'Value'
            $p1.Value | Should Be 30
            $p2.Value | Should Be 10
        }
    }

    Describe "GetUpperAnchor" {

        It "ShouldReturnFinalIfNotCutoff" {
            $o = [PSCustomObject]@{
                Value = 0
                Order = 0
                CutoffType =[CutoffType]::No
            },
            [PSCustomObject]@{
                Value = 10
                Order = 2
                CutoffType =[CutoffType]::No
            },
            [PSCustomObject]@{
                Value = 20
                Order = 3
                CutoffType =[CutoffType]::No
            },
            [PSCustomObject]@{
                Value = 30
                Order = 1
                CutoffType =[CutoffType]::No
            }

            $p1,$p2 = GetUpperAnchor $o -OrderBy 'Order'
            $p1.Value | Should Be 20
            $p2.Value | Should Be 10
        }

        It "ShouldReturnFirstCutoff" {
            $o = [PSCustomObject]@{
                Value = 0
                Order = 0
                CutoffType =[CutoffType]::No
            },
            [PSCustomObject]@{
                Value = 10
                Order = 2
                CutoffType =[CutoffType]::Scale
            },
            [PSCustomObject]@{
                Value = 20
                Order = 3
                CutoffType =[CutoffType]::No
            },
            [PSCustomObject]@{
                Value = 30
                Order = 1
                CutoffType =[CutoffType]::No
            }

            $p1,$p2 = GetUpperAnchor $o -OrderBy 'Order'
            $p1.Value | Should Be 10
            $p2.Value | Should Be 30
        }
    }

    Describe "IsProfileThroughputLimited" {

        It "PeakIsNot" {
            $x = Get-FleetProfileXml -Name Peak -BlockSize 4KB -WriteRatio 0
            IsProfileThroughputLimited -ProfileXml $x | Should Be $false
        }

        It "SqlIs" {
            $x = Get-FleetProfileXml -Name Sql
            IsProfileThroughputLimited -ProfileXml $x | Should Be $true
        }
    }

    Describe "IsProfileSingleTimespan" {

        It "PeakIs" {
            $x = Get-FleetProfileXml -Name Peak -BlockSize 4KB -WriteRatio 0
            IsProfileSingleTimespan -ProfileXml $x | Should Be $true
        }
    }

    Describe "IsProfileSingleTarget" {

        It "PeakIs" {
            $x = Get-FleetProfileXml -Name Peak -BlockSize 4KB -WriteRatio 0
            IsProfileSingleTarget -ProfileXml $x | Should Be $true
        }

        It "SqlIsNot" {
            $x = Get-FleetProfileXml -Name Sql
            IsProfileSingleTarget -ProfileXml $x | Should Be $false
        }
    }

    Describe "GetFleetProfileFootprint" {

        BeforeAll {
            $sql = Get-FleetProfileXml -Name SQL
            $vdi = Get-FleetProfileXml -Name VDI
        }

        It "CheckVDI" {
            $vdi | Should Not BeNullOrEmpty
            $f = GetFleetProfileFootprint -ProfileXml $vdi
            $f.Count | Should Be 1
            $f['*1'].BaseOffset | Should Be 0
            $f['*1'].MaxOffset | Should Be (10GB)
        }

        It "CheckVDIRead" {
            $vdi | Should Not BeNullOrEmpty
            $f = GetFleetProfileFootprint -ProfileXml $vdi -Read
            $f.Count | Should Be 1
            $f['*1'].BaseOffset | Should Be 0
            $f['*1'].MaxOffset | Should Be (8GB)
        }

        It "CheckSQL" {
            $sql | Should Not BeNullOrEmpty
            $f = GetFleetProfileFootprint -ProfileXml $sql
            $f.Count | Should Be 1
            $f['*1'].BaseOffset | Should Be 0
            $f['*1'].MaxOffset | Should Be 0
        }

        It "CheckSQLRead" {
            $sql | Should Not BeNullOrEmpty
            $f = GetFleetProfileFootprint -ProfileXml $sql -Read
            $f.Count | Should Be 1
            $f['*1'].BaseOffset | Should Be (5GB)
            $f['*1'].MaxOffset | Should Be 0
        }
    }

    Describe "TimespanToString" {

        BeforeAll {
            $t0 = [datetime] '7/31/1996 12:00PM'
        }

        It "Seconds" {
            TimespanToString ($t0.AddMilliseconds(1500) - $t0) | Should Be "01.5s"
        }

        It "Minutes" {
            TimespanToString ($t0.AddMinutes(15) - $t0) | Should Be "15m:00.0s"
        }

        It "Hours" {
            TimespanToString ($t0.AddHours(15) - $t0) | Should Be "15h:00m:00.0s"
        }

        It "Days" {
            TimespanToString ($t0.AddDays(15) - $t0) | Should Be "15d.00h:00m:00.0s"
        }
    }
}