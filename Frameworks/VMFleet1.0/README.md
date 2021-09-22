## VM Fleet ##

These are the historical release comments for VM Fleet 1.0.

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