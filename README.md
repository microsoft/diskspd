DiskSpd
=======

DiskSpd is a storage performance tool from the Windows, Windows Server and Cloud Server Infrastructure engineering teams at Microsoft. Please visit <https://github.com/Microsoft/diskspd/wiki> for updated documentation.

In addition to the tool itself, this repository hosts measurement frameworks which utilize DiskSpd. The initial example is [VM Fleet](https://github.com/Microsoft/diskspd/blob/master/Frameworks/VMFleet), used for Windows Server Hyper-Converged environments with Storage Spaces Direct.

This project has adopted the [Microsoft Open Source Code of Conduct](https://opensource.microsoft.com/codeofconduct/). For more information see the [Code of Conduct FAQ](https://opensource.microsoft.com/codeofconduct/faq/) or contact [opencode@microsoft.com](mailto:opencode@microsoft.com) with any additional questions or comments.

Releases
========

The [Releases](https://github.com/Microsoft/diskspd/releases) page includes **pre-compiled binaries (ZIP) and source code** for the most current releases of the DiskSpd tool. The latest update to DiskSpd can always be downloaded from <https://github.com/Microsoft/diskspd/releases/latest/download/DiskSpd.zip> (aka <https://aka.ms/getdiskspd>).

What's New?
===========

## DISKSPD

# DISKSPD 2.1 7/1/2021

* New `-g<n>i` form allowing throughput limit specification in units of IOPS (per specified blocksize)
* New `-rs<pct>` to specify mixed random/sequential operation (pct random); geometric distribution of run lengths
* New `-rd<distribution>` to specify non-uniform IO distributions across target
  * `pct` by target percentage
  * `abs` by absolute offset
* New `-Rp<text|xml>` to show specified parameter set in indicated profile output form; works with -X XML profiles and conventional command line
* XML results/profiles are now indented for ease of review
* Text result output updates
  * now shows values in size units (K/M/G, and now TiB) to two decimals
  * thread stride no longer shown unless specified
  * -F/-O threadpool parameters shown
* XML profiles can now be built more generically
  * XML profiles can be stated in terms of templated target names (*1, *2), replaced in order from command line invocation
  * the command line now allows options alongside -X: -v, -z, -R and -W/-d/-C along with template target specs

# DISKSPD 2.0.21a 9/21/2018

* Added support for memory mapped I/O:
  * New `-Sm` option to enable memory mapped I/O
  * New `-N<vni>` option to specify flush options for memory mapped I/O
* Added support for providing Event Tracing for Windows (ETW) events
* Included a Windows Performance Recorder (WPR) profile to enable ETW tracing
* Added system information to the ResultParser output

# DISKSPD 2.0.20a 2/28/2018

* Changes that may require rebaselining of results:
  * New random number generator that may show an observable decreased cost
  * Switched to 512-byte aligned buffers with the `-Z` option to increase performance
* New `-O` option for specifying the number of outstanding IO requests per thread
* New `-Zr` option for per-IO randomization of write buffer content
* XML: Adds a new `<ThreadTarget>` element to support target weighting schemes
* Enhanced statistics captured from IOPS data
* Added support for validating XML profiles using an in-built XSD
* Added support for handling RAW volumes
* Updated CPU statistics to work on > 64-core systems
* Updated calculation and accuracy of CPU statistics
* Re-enable support for ETW statistics

# DISKSPD 2.0.18a 5/31/2016

* update `/?` example to use `-Sh` v. deprecated `-h`
* fix operation on volumes on GPT partitioned media (<driveletter>:)
* fix IO priority hint to proper stack alignment (if not 8 byte, will fail)
* use iB notation to clarify that text result output is in 2^n units (KiB/MiB/GiB)

# DISKSPD 2.0.17a 5/01/2016

* `-S` is expanded to control write-through independent of OS/software cache. Among other things, this allows buffered write-through to be specified (`-Sbw`).
* XML: adds a new `<WriteThrough>` element to specify write-through
* XML: `<DisableAllCache>` is no longer emitted (still parsed, though), in favor or `<WriteThrough>` and `<DisableOSCache>`
* Text output: OS/software cache and write-through state are now documented separately (adjacent lines)
* Latency histogram now reports to 9-nines (one part in one billion) in both text and XML output
* Error message added for failure to open write-content source file (`-Z<size>,<file>`)

## VM Fleet

VM Fleet is a performance characterization and analyst framework for exploring the storage capabilities of Windows Server Hyper-Converged environments with Storage Spaces Direct.

# VM Fleet 2.1.0.0 4/3/2024

* Support for Arc VM management (only applicable to clusters managed by Arc)
* `Set-FleetRunProfileScript` - produce a free-run script based on one of the defined workload profiles
* `Watch-FleetCPU` - new support for monitoring guest VCPU utilization (-Guest); can handle data outages
* Fix: performance counter handling now manages intermittent data dropouts (per conventional relog.exe)
* Fix: mid-run vm health check now handles the possibility of many vms taking longer than intended runtime to validate; early exit to avoid false failures
* Fix: ignore reboot required indication from cache layer when changing cache behavior; avoid false failure

# VM Fleet 2.0.2.2 12/1/2021

* Fix cluster remoting issue during New-Fleet caused by 2.0.2.1 work
* Use timestamped logging in New-Fleet, simplify and de-colorize default output

# VM Fleet 2.0.2.1 11/9/2021

* Fix cluster remoting issues in Move-Fleet and Get-FleetDataDiskEstimate
* Fix timing issue with Start-FleetSweep; always start from fleet pause to avoid triggering free run
* Use uniqueness to guarantee Start-FleetSweep runs profile in case of repeat

# VM Fleet 2.0.2 11/2/2021

* Windows Server 2019/RS5 host operation now confirmed & supported
* Read cache warmup for HDD capacity systems should now be faster

`Set-FleetPause` will wait for VM responses before completion by default (see -Timeout)

Several minor fixes including:

* Disable Windows Recovery Console in fleet VMs
* Fix: `Show-Fleet` IOPS view now aggregates all VM disk devices
* Fix: clean up leaked/conflicting data collectors and blg automatically

# VM Fleet 2.0 9/22/2021

* major release and rewrite as a first class Powershell module
* original script-based VM Fleet remains available at Frameworks/VMFleet1.0
* see the [documentation](https://github.com/microsoft/diskspd/wiki/VMFleet) in the Wiki

Source Code
===========

The source code for DiskSpd is hosted on GitHub at:

<https://github.com/Microsoft/diskspd>

Any issues with DiskSpd can be reported using the following link:

<https://github.com/Microsoft/diskspd/issues>
