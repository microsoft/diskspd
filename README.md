DiskSpd
=======

DISKSPD is a storage load generator / performance test tool from the Microsoft Windows, Windows Server and Cloud Server Infrastructure Engineering teams. Please see the included documentation (docx and pdf formats).

This project has adopted the [Microsoft Open Source Code of Conduct](https://opensource.microsoft.com/codeofconduct/). For more information see the [Code of Conduct FAQ](https://opensource.microsoft.com/codeofconduct/faq/) or contact [opencode@microsoft.com](mailto:opencode@microsoft.com) with any additional questions or comments.

What's New?
===========

In addition DISKSPD itself, this repo hosts measurement frameworks which use DISKSPD. The initial example is the ***VM Fleet*** being used for Windows Server 2016 Hyper-Converged Storage Spaces Direct work. Look for these under the *Frameworks* directory.

## DISKSPD ##

DISKSPD 2.0.18a 5/31/2016

***IN PROGRESS***

* update /? example to use -Sh v. deprecated -h
* fix operation on volumes on GPT partitioned media (<driveletter>:)
* fix IO priority hint to proper stack alignment (if not 8 byte, will fail)
* use iB notation to clarify that text result output is in 2^n units (KiB/MiB/GiB)

DISKSPD 2.0.17 5/01/2016

* -S is expanded to control write-through independent of OS/software cache. Among other things, this allows buffered write-through to be specified (-Sbw).
* XML: adds a new `<WriteThrough>` element to specify write-through
* XML: `<DisableAllCache>` is no longer emitted (still parsed, though), in favor or `<WriteThrough>` and `<DisableOSCache>`
* Text output: OS/software cache and write-through state are now documented separately (adjacent lines)
* Latency histogram now reports to 9-nines (one part in one billion) in both text and XML output
* Error message added for failure to open write-content source file (`-Z<size>,<file>`)

DISKSPD 2.0.16b 2/22/2016

* -ag is now default (round robin group-aware affinity)
* new -ag# for group-aware thread->core affinity assignment
* -Sr : remote cache mode
* -Sh : equivalent to -h, all cache modes collapsed under -S
* `<ProcessorTopology>` (under `<System>`) element in XML results shows Processor Group topology of the system the test executed on
* `<RunTime>` (under `<System>`) element shows run start time in GMT
* -ft : specifies `FILE_ATTRIBUTE_TEMPORARY_FILE` on open (note: work in progress, effect of this attribute is not fully lit up yet)

## VM Fleet ##

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

VM Fleet 0.5 6/22/2016

* start-sweep now supports gathering performance counters from the physical nodes (hv root)
* bugfix: clear-pause needs to loop on the pause file in case of competing access
* run.ps1 and run-sweeptemplate.ps1 updated to show use of random seed and write source buffer (best practice), changes to push result file down with noncached copy
* check-pause comments on the number of (un)paused vms
* update documentation to mention change in QoS policy type names

Compiling / Source
=========

Compilation is supported with Visual Studio and Visual Studio Express. Use the Visual Studio solution file inside the diskspd_vs directory.

Source code is hosted at the following repo, if you did not clone it directly:

[https://github.com/microsoft/diskspd](https://github.com/microsoft/diskspd "https://github.com/microsoft/diskspd")

A binary release is hosted by Microsoft at the following location:

[http://aka.ms/diskspd](http://aka.ms/diskspd "http://aka.ms/diskspd")