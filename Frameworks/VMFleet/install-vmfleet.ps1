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

param(
    $source = $(throw "Must specify the source directory for the vmfleet scripts")
    )

$col = get-clustersharedvolume |? Name -match '(collect)'
if ($col -eq $null) {
    write-error "The collect CSV is not present"
    return
}

$fqsrc = gi $source
if ($fqsrc -eq $null) {
    write-error "The source directory for the vmfleet scripts is not accessible"
}

# update the csv mounts to their normalized names and install scripts and directory structure
# remove internet-download blocks on ps1 scripting, if present
& $fqsrc\update-csv.ps1 -renamecsvmounts
$control = 'C:\ClusterStorage\collect\control'

cp -r $fqsrc $control
dir $control\*.ps1 |% { Unblock-File $_ }

mkdir $control\result
mkdir $control\flag
mkdir $control\tools


# put the control directory onto the path
if (-not ([System.Environment]::GetEnvironmentVariable("path") -split ';' |? { $_ -eq $control })) {
    $newpath = [System.Environment]::GetEnvironmentVariable("path") + ";$control"
    [System.Environment]::SetEnvironmentVariable("path",
                                                 $newpath,
                                                 [System.EnvironmentVariableTarget]::Process)
    [System.Environment]::SetEnvironmentVariable("path",
                                                 $newpath,
                                                 [System.EnvironmentVariableTarget]::User)
}

# disable the csv balancer so that motion is under fleet control
(get-cluster).CsvBalancer = 0

# finally set fleet pause and touch the base run file so that we ensure
# the fleet will start with it.
set-pause
(gi $control\run.ps1).IsReadOnly = $false
(gi $control\run.ps1).LastWriteTime = (Get-Date)