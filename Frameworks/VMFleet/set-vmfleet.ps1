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

param( [Parameter(ParameterSetName = 'FullSpec', Mandatory = $true)]
        [int] $ProcessorCount,
       [Parameter(ParameterSetName = 'FullSpec', Mandatory = $true)]
        [int64] $MemoryStartupBytes,
       [Parameter(ParameterSetName = 'FullSpec')]
        [int64] $MemoryMaximumBytes = 0,
       [Parameter(ParameterSetName = 'FullSpec')]
        [int64] $MemoryMinimumBytes = 0,
       [Parameter(ParameterSetName = 'FullSpec')]
        [switch]$DynamicMemory = $true,
       [Parameter(ParameterSetName = 'SizeSpec', Mandatory = $true)]
        [ValidateSet('A0','A1','A1v2','A2','A2mv2','A2v2','A3','A4','A4mv2','A4v2','A5','A6','A7','A8mv2','A8v2','D1','D11','D11v2','D12','D12v2','D13','D13v2','D14','D14v2','D1
5v2','D1v2','D2','D2v2','D3','D3v2','D4','D4v2','D5v2','DS11','DS11v2','DS12','DS12v2','DS13','DS13v2','DS14','DS14v2','DS15v2')]
        [string]$Compute = 'A1'
    )

# to regenerate validateset
# ($vmsize.keys | sort |% { "'$_'" }) -join ','

# c = compute cores
# m = memory
# a = alias for another (ex: d -> ds, d -> dv2)
# note that alias is only chased once

 $vmsize = @{

    # general purpose table
    'A0' = @{ 'c' = 1; 'm' = 0.75GB; };
    'A1' = @{ 'c' = 1; 'm' = 1.75GB; };
    'A2' = @{ 'c' = 2; 'm' = 3.5GB; };
    'A3' = @{ 'c' = 4; 'm' = 7GB; };
    'A4' = @{ 'c' = 8; 'm' = 14GB; };
    'A5' = @{ 'c' = 2; 'm' = 14GB };
    'A6' = @{ 'c' = 4; 'm' = 28GB };
    'A7' = @{ 'c' = 8; 'm' = 56GB };

    'A1v2' = @{ 'c' = 1; 'm' = 2GB; };
    'A2v2' = @{ 'c' = 2; 'm' = 4GB; };
    'A4v2' = @{ 'c' = 4; 'm' = 8GB; };
    'A8v2' = @{ 'c' = 8; 'm' = 16GB; };
    'A2mv2' = @{ 'c' = 2; 'm' = 16GB; }
    'A4mv2' = @{ 'c' = 4; 'm' = 32GB; };
    'A8mv2' = @{ 'c' = 8; 'm' = 64GB; };

    'D1' = @{ 'c' = 1; 'm' = 3.5GB };
    'D2' = @{ 'c' = 2; 'm' = 7GB };
    'D3' = @{ 'c' = 4; 'm' = 14GB };
    'D4' = @{ 'c' = 8; 'm' = 28GB };

    'D1v2' = @{ 'a' = 'D1' }
    'D2v2' = @{ 'a' = 'D2' }
    'D3v2' = @{ 'a' = 'D3' }
    'D4v2' = @{ 'a' = 'D4' }
    'D5v2' = @{ 'c' = 16; 'm' = 56GB };

    # memory optimized table (just the d's)
    'D11' = @{ 'c' = 2; 'm' = 14GB };
    'D12' = @{ 'c' = 4; 'm' = 28GB };
    'D13' = @{ 'c' = 8; 'm' = 56GB };
    'D14' = @{ 'c' = 16; 'm' = 112GB };

    'DS11' = @{ 'a' = 'D11' };
    'DS12' = @{ 'a' = 'D12' };
    'DS13' = @{ 'a' = 'D13' };
    'DS14' = @{ 'a' = 'D14' };

    'D11v2' = @{ 'a' = 'D11' };
    'D12v2' = @{ 'a' = 'D12' };
    'D13v2' = @{ 'a' = 'D13' };
    'D14v2' = @{ 'a' = 'D14' };
    'D15v2' = @{ 'c' = 20; 'm' = 140GB };

    'DS11v2' = @{ 'a' = 'D11v2' };
    'DS12v2' = @{ 'a' = 'D12v2' };
    'DS13v2' = @{ 'a' = 'D13v2' };
    'DS14v2' = @{ 'a' = 'D14v2' };
    'DS15v2' = @{ 'a' = 'D15v2' };
}

$g = Get-ClusterGroup |? GroupType -eq VirtualMachine | group -Property OwnerNode -NoElement

icm $g.Name {

    # import vmsize hash (cannot $using[$using])
    $vmsize = $using:vmsize

    Get-ClusterGroup |? GroupType -eq VirtualMachine |? OwnerNode -eq $env:COMPUTERNAME |% {

        $g = $_

        switch ($using:PSCmdlet.ParameterSetName) {

            'FullSpec' {

                if ($using:MemoryMaximumBytes -eq 0) {
                    $MemoryMaximumBytes = $MemoryStartupBytes
                } else {
                    $MemoryMaximumBytes = $using:MemoryMaximumBytes
                }
                if ($using:MemoryMinimumBytes -eq 0) {
                    $MemoryMinimumBytes = $MemoryStartupBytes
                } else {
                    $MemoryMinimumBytes = $using:MemoryMinimumBytes
                }

                $memswitch = '-StaticMemory'
                $dynamicMemArg = ""
                if ($using:DynamicMemory) {
                    $memswitch = '-DynamicMemory'
                    $dynamicMemArg += "-MemoryMinimumBytes $MemoryMinimumBytes -MemoryMaximumBytes $MemoryMaximumBytes"
                }

                if ($g.State -ne 'Offline') {
                    write-host -ForegroundColor Yellow Cannot alter VM sizing on running VMs "($($_.Name))"
                } else {
                    iex "Set-VM -ComputerName $($g.OwnerNode) -Name $($g.Name) -ProcessorCount $using:ProcessorCount -MemoryStartupBytes $using:MemoryStartupBytes $dynamicMemArg $memswitch"
                }
            }

            'SizeSpec' {
                $a = $vmsize[$using:compute].a
                if ($a -eq $null) {
                    $a = $using:compute
                }

                Set-VM -ComputerName $($g.OwnerNode) -Name $($g.Name) -ProcessorCount $vmsize[$a].c -MemoryStartupBytes $vmsize[$a].m -StaticMemory
            }       
        }
    }
}
