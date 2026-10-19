$ErrorActionPreference = 'Stop'
$report = Join-Path $PSScriptRoot 'wsl-diagnostic.txt'
Get-WindowsOptionalFeature -Online -FeatureName VirtualMachinePlatform | Format-List FeatureName,State | Out-File $report -Encoding utf8
Get-WindowsOptionalFeature -Online -FeatureName Microsoft-Windows-Subsystem-Linux | Format-List FeatureName,State | Out-File $report -Append -Encoding utf8
bcdedit /enum | Out-File $report -Append -Encoding utf8
Get-CimInstance Win32_ComputerSystem | Select-Object HypervisorPresent | Format-List | Out-File $report -Append -Encoding utf8
