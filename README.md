DiskSpd
=======

DiskSpd is a storage performance tool from the Windows, Windows Server and Cloud Server Infrastructure engineering teams at Microsoft. Please visit <https://github.com/Microsoft/diskspd/wiki> for updated documentation.

In addition to the tool itself, this repository hosts measurement frameworks which utilize DiskSpd. The initial example is [VM Fleet](https://github.com/Microsoft/diskspd/blob/master/Frameworks/VMFleet) that was used for the Windows Server 2016 Hyper-Converged Storage Spaces Direct work.

This project has adopted the [Microsoft Open Source Code of Conduct](https://opensource.microsoft.com/codeofconduct/). For more information see the [Code of Conduct FAQ](https://opensource.microsoft.com/codeofconduct/faq/) or contact [opencode@microsoft.com](mailto:opencode@microsoft.com) with any additional questions or comments.

Releases
========

The [Releases](https://github.com/Microsoft/diskspd/releases) page includes **pre-compiled binaries (ZIP) and source code** for the most current releases of the DiskSpd tool. The latest update to DiskSpd can always be downloaded from <https://github.com/Microsoft/diskspd/releases/latest/download/DiskSpd.zip> (aka <https://aka.ms/getdiskspd>).

What's New?
===========

## DISKSPD ##

DISKSPD 2.0.21a 9/21/2018

* Added support for memory mapped I/O:
  * New `-Sm` option to enable memory mapped I/O
  * New `-N<vni>` option to specify flush options for memory mapped I/O
* Added support for providing Event Tracing for Windows (ETW) events
* Included a Windows Performance Recorder (WPR) profile to enable ETW tracing
* Added system information to the ResultParser output

DISKSPD 2.0.20a 2/28/2018

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

DISKSPD 2.0.18a 5/31/2016

* update `/?` example to use `-Sh` v. deprecated `-h`
* fix operation on volumes on GPT partitioned media (<driveletter>:)
* fix IO priority hint to proper stack alignment (if not 8 byte, will fail)
* use iB notation to clarify that text result output is in 2^n units (KiB/MiB/GiB)

DISKSPD 2.0.17a 5/01/2016

* `-S` is expanded to control write-through independent of OS/software cache. Among other things, this allows buffered write-through to be specified (`-Sbw`).
* XML: adds a new `<WriteThrough>` element to specify write-through
* XML: `<DisableAllCache>` is no longer emitted (still parsed, though), in favor or `<WriteThrough>` and `<DisableOSCache>`
* Text output: OS/software cache and write-through state are now documented separately (adjacent lines)
* Latency histogram now reports to 9-nines (one part in one billion) in both text and XML output
* Error message added for failure to open write-content source file (`-Z<size>,<file>`)

## VM Fleet ##

VM Fleet 0.9 10/2017 (minor)

* watch-cpu: now provides total normalized cpu utility (accounting for turbo/speedstep)
* sweep-cputarget: now provides average CSV FS read/write latency in the csv

VM Fleet 0.8 6/2017

* get-cluspc: add SMB Client/Server and SMB Direct (not defaulted in Storage group yet)
* test-clusterhealth: flush output pipeline for Debug-StorageSubsystem output
* watch-cluster: restart immediately if all child jobs are no longer running
* watch-cpu: new, visualizer for CPU core utilization distributions

VM Fleet 0.7 3/2017

* create/destroy-vmfleet & update-csv: don't rely on the csv name containing the friendlyname of the vd
* create-vmfleet: err if basevhd inaccessible
* create-vmfleet: simplify call-throughs using $using: syntax
* create-vmfleet: change vhdx layout to match scvmm behavior of seperate directory per VM (important for ReFS MRV)
* create-vmfleet: use A1 VM size by default (1VCPU 1.75GiB RAM)
* start-vmfleet: try starting "failed" vms, usually works
* set-vmfleet: add support for -SizeSpec <Azure Size Specs> for A/D/D2v1 & v2 size specification, for ease of reconfig
* stop-vmfleet: pass in full namelist to allow best-case internal parallelization of shutdown
* sweep-cputarget: use %Processor Performance to rescale utilization and account for Turbo effects
* test-clusterhealth: support cleaning out dumps/triage material to simplify ongoing monitoring (assume they're already gathered/etc.)
* test-clusterhealth: additional triage output for storport unresponsive device events
* test-clusterhealth: additional triage comments on SMB client connectivity events
* test-clusterhealth: new test for Mellanox CX3/CX4 error counters that diagnose fabric issues (bad cable/transceiver/roce specifics/etc.)
* get-log: new triage log gatherer for all hv/clustering/smb event channels
* get-cluspc: new cross-cluster performance counter gatherer
* remove run-<>.ps1 scripts that were replaced with run-demo-<>.ps1
* check-outlier: EXPERIMENTAL way to ferret out outlier devices in the cluster, using average sampled latency

VM Fleet 0.6 7/18/2016

* CPU Target Sweep: a sweep script using StorageQoS and a linear CPU/IOPS model to build an empirical sweep of IOPS as a function of CPU, initially for the three classic small IOPS mixes (100r, 90:10 and 70:30 4K). Includes an analysis script which provides the linear model for each off of the results.
* Update sweep mechanics which allow generalized specification of DISKSPD sweep parameters and host performance counter capture.
* install-vmfleet to automate placement after CSV/VD structure is in place (add path, create dirs, copyin, pause)
* add non-linearity detection to analyze-cputarget
* get-linfit is now a utility script (produces objects describing fits)
* all flag files (pause/go/done) pushed down to control\flag directory
* demo scripting works again and autofills vm/node counts
* watch-cluster handles downed/recovered nodes gracefully
* update-csv now handles node names which are logical prefixes of another (node1, node10)

Source Code
===========

The source code for DiskSpd is hosted on GitHub at:

<https://github.com/Microsoft/diskspd>

Any issues with DiskSpd can be reported using the following link:

<https://github.com/Microsoft/diskspd/issues>
