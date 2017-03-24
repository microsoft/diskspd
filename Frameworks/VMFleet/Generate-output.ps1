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


<#
How to run the file.
generate-output | Export-Csv <location of .csv file>
#>

             param (
             )
                $Files = Get-ChildItem 'C:\ClusterStorage\Collect\control\result\' | select -ExpandProperty fullname
                $result = @()

                foreach ($xmlFile in $Files) {
                $XmlObject = [xml](Get-Content "$($xmlFile)")
                $duration = $XmlObject.Results.TimeSpan.TestTimeSeconds
                $IO = ($XmlObject.Results.TimeSpan.Thread.Target | measure -sum -Property IOCount).Sum
                $RC = ($XmlObject.Results.TimeSpan.Thread.Target | measure -sum -Property ReadCount).Sum
                $WC = ($XmlObject.Results.TimeSpan.Thread.Target | measure -sum -Property WriteCount).Sum
                $access = $XmlObject.Results.Profile.TimeSpans.TimeSpan.Targets.Target.SequentialScan
                if ($access = "true") {
                    $accessMethod = "Sequential"
                } else {
                    $accessMethod = "Random"
                }
                [double]$TotIOPS =  $($IO)/$($duration)
                [double]$ReadIOPS = $($RC)/$($duration)
                [double]$WriteIOPS = $($WC)/$($duration)
                [double]$WritePercent = $($WriteIOPS) / $($TotIOPS) * 100
                $output = [pscustomobject]@{
                    VMName = $XmlObject.Results.System.ComputerName;
                    BlockSize = $XmlObject.Results.Profile.TimeSpans.TimeSpan.Targets.Target.BlockSize
                    TestTime = $XmlObject.Results.TimeSpan.TestTimeSeconds
                    ThreadCount = $XmlObject.Results.TimeSpan.ThreadCount
                    ProcCount = $XmlObject.Results.TimeSpan.ProcCount
                    AccessMethod = $accessMethod
                    QueueDepth = $XmlObject.Results.Profile.TimeSpans.TimeSpan.Targets.Target.RequestCount
                    CPUUtil = $XmlObject.Results.TimeSpan.CpuUtilization.Average.UsagePercent
                    IOCount = ($XmlObject.Results.TimeSpan.Thread.Target | measure -sum -Property IOCount).Sum
                    ReadCount = ($XmlObject.Results.TimeSpan.Thread.Target | measure -sum -Property ReadCount).Sum
                    WriteCount = ($XmlObject.Results.TimeSpan.Thread.Target | measure -sum -Property WriteCount).Sum
                    TotalIOPS = [double]$TotIOPS
                    ReadIOPS =  [double]$ReadIOPS
                    WriteIOPS = [double]$WriteIOPS
                    Latency = $XmlObject.Results.TimeSpan.Latency.AverageTotalMilliseconds
                    WritePercent = [double]$WritePercent
                }
            $result += $output
            }
            $result

