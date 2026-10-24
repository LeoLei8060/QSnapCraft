[CmdletBinding()]
param(
    [string]$ApplicationPath = '',
    [int]$Cycles = 30,
    [int]$WarmupCycles = 3
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

$repoRoot = [System.IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
if ([string]::IsNullOrWhiteSpace($ApplicationPath)) {
    $projectText = Get-Content (Join-Path $repoRoot 'CMakeLists.txt') -Raw
    if ($projectText -notmatch 'project\(QSnapCraft VERSION ([0-9]+\.[0-9]+\.[0-9]+)') {
        throw "Cannot read the QSnapCraft version from CMakeLists.txt."
    }
    $ApplicationPath = Join-Path $repoRoot "dist\QSnapCraft-$($Matches[1])-win64\QSnapCraft.exe"
}
$ApplicationPath = [System.IO.Path]::GetFullPath($ApplicationPath)
if (-not (Test-Path $ApplicationPath)) {
    throw "Packaged QSnapCraft.exe was not found at $ApplicationPath."
}
if (Get-Process QSnapCraft -ErrorAction SilentlyContinue) {
    throw "Close existing QSnapCraft processes before running the soak test."
}
if ($Cycles -le 0 -or $WarmupCycles -lt 0) {
    throw "Cycles must be positive and WarmupCycles must not be negative."
}

Add-Type -AssemblyName System.Windows.Forms

$applicationDirectory = Split-Path $ApplicationPath
$app = Start-Process -FilePath $ApplicationPath -WorkingDirectory $applicationDirectory -WindowStyle Hidden -PassThru
try {
    Start-Sleep -Milliseconds 1800
    if ($app.HasExited) {
        throw "QSnapCraft exited during startup with code $($app.ExitCode)."
    }

    for ($index = 0; $index -lt $WarmupCycles; ++$index) {
        [System.Windows.Forms.SendKeys]::SendWait('{F1}')
        Start-Sleep -Milliseconds 220
        [System.Windows.Forms.SendKeys]::SendWait('{ESC}')
        Start-Sleep -Milliseconds 180
    }
    Start-Sleep -Milliseconds 1000
    $app.Refresh()
    $startHandles = $app.HandleCount
    $startMemory = $app.WorkingSet64

    for ($index = 0; $index -lt $Cycles; ++$index) {
        [System.Windows.Forms.SendKeys]::SendWait('{F1}')
        Start-Sleep -Milliseconds 220
        [System.Windows.Forms.SendKeys]::SendWait('{ESC}')
        Start-Sleep -Milliseconds 180
        if ($app.HasExited) {
            throw "QSnapCraft exited during soak cycle $($index + 1)."
        }
    }

    Start-Sleep -Milliseconds 1800
    $app.Refresh()
    $endHandles = $app.HandleCount
    $endMemory = $app.WorkingSet64
    Write-Host "CYCLES=$Cycles"
    Write-Host "HANDLE_DELTA=$($endHandles - $startHandles)"
    Write-Host "WORKING_SET_DELTA_MB=$([math]::Round(($endMemory - $startMemory) / 1MB, 1))"
} finally {
    if (-not $app.HasExited) {
        Stop-Process -Id $app.Id
    }
}
