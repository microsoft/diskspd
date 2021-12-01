<#
VM Fleet

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

#
# Module manifest for module 'VMFleet'
#
# Generated on: 1/30/2021
#

@{

# Script module or binary module file associated with this manifest.
RootModule = 'VMFleet.psm1'

# Version number of this module. Even build# is release, odd pre-release (Mj.Mn.Bd.Rv)
ModuleVersion = '2.0.2.2'

# Supported PSEditions
# CompatiblePSEditions = @()

# ID used to uniquely identify this module
GUID = '753ad5da-01b3-4cc8-a475-16a09a021384'

# Author of this module
Author = 'Microsoft Corporation'

# Company or vendor of this module
CompanyName = 'Microsoft Corporation'

# Copyright statement for this module
Copyright = '(c) 2021 Microsoft Corporation. All rights reserved.'

# Description of the functionality provided by this module
Description = 'VM Fleet is a performance characterization and analyst framework for exploring the storage capabilities of Windows Server Hyper-Converged environments with Storage Spaces Direct'

# Minimum version of the Windows PowerShell engine required by this module
PowerShellVersion = '5.1'

# Name of the Windows PowerShell host required by this module
# PowerShellHostName = ''

# Minimum version of the Windows PowerShell host required by this module
# PowerShellHostVersion = ''

# Minimum version of Microsoft .NET Framework required by this module. This prerequisite is valid for the PowerShell Desktop edition only.
# DotNetFrameworkVersion = ''

# Minimum version of the common language runtime (CLR) required by this module. This prerequisite is valid for the PowerShell Desktop edition only.
# CLRVersion = ''

# Processor architecture (None, X86, Amd64) required by this module
# ProcessorArchitecture = ''

# Modules that must be imported into the global environment prior to importing this module
# RequiredModules = @()

# Assemblies that must be loaded prior to importing this module
# RequiredAssemblies = @()

# Script files (.ps1) that are run in the caller's environment prior to importing this module.
# ScriptsToProcess = @()

# Type files (.ps1xml) to be loaded when importing this module
# TypesToProcess = @()

# Format files (.ps1xml) to be loaded when importing this module
FormatsToProcess = @( 'VMFleet.format.ps1xml' )

# Modules to import as nested modules of the module specified in RootModule/ModuleToProcess
NestedModules = @(
    'Profile.psm1',
    'WatchCluster.psm1',
    'WatchCPU.psm1'
)

# Functions to export from this module, for best performance, do not use wildcards and do not delete the entry, use an empty array if there are no functions to export.
FunctionsToExport = @(
    'Clear-FleetPause',
    'Clear-FleetRunState',
    'Convert-FleetXmlToString',
    'Get-FleetComputeTemplate',
    'Get-FleetDisk',
    'Get-FleetDataDiskEstimate',
    'Get-FleetPath',
    'Get-FleetPause',
    'Get-FleetPolynomialFit',
    'Get-FleetPowerScheme',
    'Get-FleetProfileXml',
    'Get-FleetResultLog',
    'Get-FleetVersion',
    'Get-FleetVM',
    'Get-FleetVolumeEstimate',
    'Install-Fleet',
    'Measure-FleetCoreWorkload',
    'Move-Fleet',
    'New-Fleet',
    'Remove-Fleet',
    'Repair-Fleet',
    'Set-Fleet',
    'Set-FleetPause',
    'Set-FleetPowerScheme',
    'Set-FleetProfile',
    'Set-FleetQoS',
    'Show-Fleet',
    'Show-FleetCpuSweep',
    'Show-FleetPause',
    'Start-Fleet',
    'Start-FleetReadCacheWarmup',
    'Start-FleetResultRun',
    'Start-FleetRun',
    'Start-FleetSweep',
    'Start-FleetWriteWarmup',
    'Stop-Fleet',
    'Test-FleetResultRun',
    'Use-FleetPolynomialFit',
    'Watch-FleetCluster',
    'Watch-FleetCPU'
)

# Cmdlets to export from this module, for best performance, do not use wildcards and do not delete the entry, use an empty array if there are no cmdlets to export.
CmdletsToExport = @(

)

# Variables to export from this module
VariablesToExport = $null

# Aliases to export from this module, for best performance, do not use wildcards and do not delete the entry, use an empty array if there are no aliases to export.
AliasesToExport = @()

# DSC resources to export from this module
# DscResourcesToExport = @()

# List of all modules packaged with this module
# ModuleList = @()

# List of all files packaged with this module
# FileList = @()

# Private data to pass to the module specified in RootModule/ModuleToProcess. This may also contain a PSData hashtable with additional module metadata used by PowerShell.
PrivateData = @{

    PSData = @{

        # Tags applied to this module. These help with module discovery in online galleries.
        # Tags = @()

        # A URL to the license for this module.
        # LicenseUri = ''

        # A URL to the main website for this project.
        ProjectUri = 'https://www.github.com/microsoft/diskspd'

        # A URL to an icon representing this module.
        # IconUri = ''

        # ReleaseNotes of this module
        # ReleaseNotes = ''

    } # End of PSData hashtable

} # End of PrivateData hashtable

# HelpInfo URI of this module
# HelpInfoURI = ''

# Default prefix for commands exported from this module. Override the default prefix using Import-Module -Prefix.
# DefaultCommandPrefix = ''

}