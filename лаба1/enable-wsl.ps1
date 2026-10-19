$ErrorActionPreference = 'Stop'
$report = Join-Path $PSScriptRoot 'wsl-setup.txt'
bcdedit /set hypervisorlaunchtype auto | Out-File $report -Encoding utf8
if ($LASTEXITCODE -ne 0) { throw 'Failed to enable hypervisor launch' }
'Hypervisor launch set to Auto. Restart Windows before retrying WSL2 installation.' | Out-File $report -Append -Encoding utf8
bcdedit /enum | Out-File $report -Append -Encoding utf8
