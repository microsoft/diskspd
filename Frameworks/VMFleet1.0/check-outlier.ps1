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

param (
    [int] $interval = 10
    )

class RunningStat {

    # This is an implementation of an online (add one value at a time) method to calculate
    # up to the fourth central moment (mean, variance, skewness and kurtosis) of a series
    # of decimal values.
    #
    # Implementation is due to https://en.wikipedia.org/wiki/Algorithms_for_calculating_variance
    # which is in turn due http://people.xiph.org/~tterribe/notes/homs.html by Timothy Terriberry,
    # also cited by Pebay: http://prod.sandia.gov/techlib/access-control.cgi/2008/086212.pdf
    #
    # Note: skewness appears to match Excel calculations. Kurtosis does NOT at this time, and
    # should be used with caution.

    [decimal] $n;
    [decimal] $M1;
    [decimal] $M2;
    [decimal] $M3;
    [decimal] $M4;
    [decimal] $Min;
    [decimal] $Max;
    
    RunningStat()
    {
        $this.M1 = 0
        $this.M2 = 0
        $this.M3 = 0
        $this.M4 = 0
        $this.Min = 0
        $this.Max = 0
    }

    [void] Clear()
    {
        $this.M1 = 0
        $this.M2 = 0
        $this.M3 = 0
        $this.M4 = 0
        $this.Min = 0
        $this.Max = 0
    }

    [void] Add([decimal] $v)
    {
        $n1 = $this.n
        $this.n += 1
        $delta = $v - $this.M1
        $deltan = $delta / $this.n
        $deltan2 = $deltan * $deltan
        $term1 = $delta * $deltan * $n1
        $this.M1 += $deltan
        $this.M4 += $term1 * $deltan2 * (($this.n * $this.n) - (3 * $this.n) + 3) + (6 * $deltan2 * $this.M2) - (4 * $deltan * $this.M3)
        $this.M3 += $term1 * $deltan * ($this.n - 2) - (3 * $deltan * $this.M2)
        $this.M2 += $term1

        if ($n1 -eq 0 -or $v -gt $this.Max) { $this.Max = $v }
        if ($n1 -eq 0 -or $v -lt $this.Min) { $this.Min = $v }
    }

    [decimal] Min()
    {
        return $this.Min
    }

    [decimal] Max()
    {
        return $this.Max
    }

    [decimal] Mean()
    {
        return $this.M1
    }

    [decimal] Variance()
    {
        return $this.M2/($this.n - 1)
    }

    [decimal] StdDev()
    {
        return [math]::Sqrt($this.Variance())
    }

    [decimal] Skew()
    {
        return [math]::Sqrt($this.n) * $this.M3 / [math]::Pow($this.M2, 1.5)
    }

    [decimal] Kurtosis()
    {
        return (($this.n * $this.M4) / ($this.M2 * $this.M2)) - 3
    }
}

# populate a hash by sbl device number
$dev = gwmi -Namespace root\wmi ClusPortDeviceInformation
$devhash = @{}
$dev |% { $devhash[[int]$_.devicenumber] = $_ }

function get-outliers(
    [string[]] $paths
    )
{
    # filters and labels for the sigma buckets
    $sigflt = @(@(">5", '$sig -gt 5'),
      @("4-5", '$sig -gt 4 -and $sig -le 5'),
      @("3-4", '$sig -gt 3 -and $sig -le 4')) |% {

        new-object -TypeName psobject -Property @{
            Label = $_[0];
            Test= $_[1];
        }           
    }

    $ctrs = (get-counter -ComputerName (get-clusternode |? State -eq Up) -SampleInterval $interval -Counter $($paths |% { "\Cluster Disk Counters(*)\$_" })).countersamples |? { $_.instancename -ne "_total" }
    $stat = [RunningStat]::new()

    foreach ($path in $paths) {

        # select out the specific subset of the counters (read, write, etc.)
        $ctr = $ctrs |? { $_.Path -like "*$path" }

        $stat.Clear()
        $ctr.CookedValue |% { $stat.Add($_) }
        $stddev = $stat.StdDev()
        $mean = $stat.Mean()

        # hash of flagged devices
        $flagged = @{}

        write-host -ForegroundColor Green ("-"*20)
        write-host -ForegroundColor green Sample: $path
        write-host Number of measured devices: $ctr.count
        write-host Average of $("$path : {0:F3}ms" -f ($mean*1000))
        write-host Standard Deviation of $("{0:F3}ms" -f ($stddev*1000))

        # enumerate from highest to lowest deviation of merit
        foreach ($sigma in $sigflt) {

            # get new outliers at this deviation
            $outlier = $ctr | sort -property InstanceName |? {
                if ($stddev -eq 0) { $sig = 0 } else { $sig = (($_.CookedValue -$mean)/$stddev) }
                -not $flagged[$_.InstanceName] -and (iex $sigma.Test)
            }

            if ($outlier) {

                write-host -fore Yellow $sigma.Label sigma : $outlier.count total
                $outlier |% {

                    # remember we flagged this device already
                    $flagged[$_.InstanceName] = 1

                    $thisdev = $devhash[[int]$_.InstanceName]

                    # parse source node
                    # \\foo\... -> 0 1 2=foo 3 ...
                    $sourcenode = ($_.Path -split '\\')[2]

                    write-host -ForegroundColor Red $sourcenode SSB Device: $_.InstanceName`n`tAverage of $path $("{0:F3}ms ({1:F1} sigma)" -f ($_.CookedValue * 1000),(($_.CookedValue -$mean)/$stddev))
                    write-host -ForegroundColor Red "`tConnected Node:" $thisdev.ConnectedNode
                    write-host -ForegroundColor Red "`tConnected Node Local PhysicalDisk Number:" $thisdev.ConnectedNodeDeviceNumber
                    write-host -ForegroundColor Red "`tModel | SerialNumber: $($thisdev.ProductId)` | $($thisdev.SerialNumber)"
                }
            }
        }
    }
}

get-outliers -paths "Remote: Read Latency","Local: Read Latency","Remote: Write Latency","Local: Write Latency"