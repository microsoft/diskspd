DiskSpd
=======

DISKSPD is a storage load generator / performance test tool from the Microsoft Windows, Windows Server and Cloud Server Infrastructure Engineering teams. Please see the included documentation (docx and pdf formats).

What's New?
===========

In addition DISKSPD itself, this repo hosts measurement frameworks which use DISKSPD. The initial example is the ***VM Fleet*** being used for Windows Server 2016 Hyper-Converged Storage Spaces Direct work. Look for these under the *Frameworks* directory.

DISKSPD 2.0.16b 2/22/2016

* -ag is now default (round robin group-aware affinity)
* new -ag# for group-aware thread->core affinity assignment
* -Sr : remote cache mode
* -Sh : equivalent to -h, all cache modes collapsed under -S
* `<ProcessorTopology>` (under `<System>`) element in XML results shows Processor Group topology of the system the test executed on
* `<RunTime>` (under `<System>`) element shows run start time in GMT
* -ft : specifies `FILE_ATTRIBUTE_TEMPORARY_FILE` on open (note: work in progress, effect of this attribute is not fully lit up yet)

VM Fleet 0.4 4/14/2016

* bugfix: master.ps1 now takes connection credentials as parameters; this allows master to be edited on the fly, it is no longer templated
* credential templating moves to launch.ps1, which is now generated into the VMs
* pause handling now uses an epoch ask/response from the VMs
* check-pause uses the epoch to directly test whether a given VM has indeed acknowledged the current pause request, and is definitive with respect to all running VMs
* new automated sweep mechanic


Compiling / Source
=========

Compilation is supported with Visual Studio and Visual Studio Express. Use the Visual Studio solution file inside the diskspd_vs directory.

Source code is hosted at the following repo, if you did not clone it directly:

[https://github.com/microsoft/diskspd](https://github.com/microsoft/diskspd "https://github.com/microsoft/diskspd")

A binary release is hosted by Microsoft at the following location:

[http://aka.ms/diskspd](http://aka.ms/diskspd "http://aka.ms/diskspd")