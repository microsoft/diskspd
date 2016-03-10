DiskSpd
=======

DISKSPD is a storage load generator / performance test tool from the Windows/Windows Server and Cloud Server Infrastructure Engineering teams. Please see the included documentation (docx and pdf formats).

What's New?
===========

2.0.16b 2/22/2016

* -ag is now default (round robin group-aware affinity)
* new -ag# for group-aware thread->core affinity assignment
* -Sr : remote cache mode
* -Sh : equivalent to -h, all cache modes collapsed under -S
* `<ProcessorTopology>` (under `<System>`) element in XML results shows Processor Group topology of the system the test executed on
* `<RunTime>` (under `<System>`) element shows run start time in GMT
* -ft : specifies `FILE_ATTRIBUTE_TEMPORARY_FILE` on open (note: work in progress, effect of this attribute is not fully lit up yet)

Compiling / Source
=========

Compilation is supported with Visual Studio and Visual Studio Express. Use the Visual Studio solution file inside the diskspd_vs directory.

Source code is hosted at the following repo, if you did not clone it directly:

[https://github.com/microsoft/diskspd](https://github.com/microsoft/diskspd "https://github.com/microsoft/diskspd")

A binary release is hosted by Microsoft at the following location:

[http://aka.ms/diskspd](http://aka.ms/diskspd "http://aka.ms/diskspd")