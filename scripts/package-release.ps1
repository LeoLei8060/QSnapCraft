[CmdletBinding()]
param(
    [string]$BuildDirectory = '',
    [string]$OutputDirectory = '',
    [string]$QtRoot = 'C:\Qt\6.7.2\msvc2019_64',
    [string]$QtSourceRoot = 'C:\Qt\6.7.2\Src',
    [string]$JomDirectory = 'C:\Qt\Tools\QtCreator\bin\jom',
    [string]$VisualStudioVersion = '16.0',
    [switch]$SkipTests
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

$repoRoot = [System.IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
if ([string]::IsNullOrWhiteSpace($BuildDirectory)) {
    $BuildDirectory = Join-Path $repoRoot 'build\Desktop_Qt_6_7_2_MSVC2019_64bit_Release'
}
if ([string]::IsNullOrWhiteSpace($OutputDirectory)) {
    $OutputDirectory = Join-Path $repoRoot 'dist'
}
$BuildDirectory = [System.IO.Path]::GetFullPath($BuildDirectory)
$OutputDirectory = [System.IO.Path]::GetFullPath($OutputDirectory)
$repoPrefix = $repoRoot.TrimEnd('\') + '\'
if (-not $OutputDirectory.StartsWith($repoPrefix, [System.StringComparison]::OrdinalIgnoreCase)) {
    throw "OutputDirectory must stay inside the repository."
}

$projectText = Get-Content (Join-Path $repoRoot 'CMakeLists.txt') -Raw
if ($projectText -notmatch 'project\(QSnapCraft VERSION ([0-9]+\.[0-9]+\.[0-9]+)') {
    throw "Cannot read the QSnapCraft version from CMakeLists.txt."
}
$version = $Matches[1]
$packageName = "QSnapCraft-$version-win64"
$stagingDirectory = Join-Path $OutputDirectory $packageName
$archivePath = Join-Path $OutputDirectory "$packageName.zip"
$hashPath = "$archivePath.sha256"
$windeployqt = Join-Path $QtRoot 'bin\windeployqt.exe'
if (-not (Test-Path $windeployqt)) {
    throw "windeployqt was not found at $windeployqt."
}
$jom = Join-Path $JomDirectory 'jom.exe'
if (-not (Test-Path $jom)) {
    throw "jom.exe was not found at $jom."
}

$vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
if (-not (Test-Path $vswhere)) {
    throw "vswhere.exe was not found."
}
$visualStudioMajor = [int]($VisualStudioVersion.Split('.')[0])
$visualStudioUpperBound = if ($visualStudioMajor -le 16) { '17.0' } else { '18.0' }
$visualStudioPath = & $vswhere -latest -products * `
    -version "[$VisualStudioVersion,$visualStudioUpperBound)" `
    -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 `
    -property installationPath
if ([string]::IsNullOrWhiteSpace($visualStudioPath)) {
    throw "A Visual C++ workload was not found."
}
$vcvars = Join-Path $visualStudioPath 'VC\Auxiliary\Build\vcvars64.bat'
if (-not (Test-Path $vcvars)) {
    throw "vcvars64.bat was not found at $vcvars."
}

function Invoke-DeveloperCommand([string]$Command, [string]$LogPath = '') {
    $line = "call `"$vcvars`" >nul && set `"PATH=$JomDirectory;%PATH%`" && $Command"
    if ([string]::IsNullOrWhiteSpace($LogPath)) {
        & cmd.exe /d /s /c $line
    } else {
        & cmd.exe /d /s /c $line *> $LogPath
    }
    if ($LASTEXITCODE -ne 0) {
        if (-not [string]::IsNullOrWhiteSpace($LogPath) -and (Test-Path $LogPath)) {
            Get-Content $LogPath -Tail 40
        }
        throw "Developer command failed with exit code $LASTEXITCODE."
    }
}

if (-not (Test-Path (Join-Path $BuildDirectory 'CMakeCache.txt'))) {
    $configureCommand = "cmake -S `"$repoRoot`" -B `"$BuildDirectory`" -G `"NMake Makefiles JOM`" " +
        "-DCMAKE_PREFIX_PATH=`"$QtRoot`" -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=ON"
    Invoke-DeveloperCommand -Command $configureCommand `
        -LogPath (Join-Path $env:TEMP 'qsnap-package-configure.log')
}

$buildCommand = "cmake --build `"$BuildDirectory`" --config Release --parallel 4"
Invoke-DeveloperCommand -Command $buildCommand -LogPath (Join-Path $env:TEMP 'qsnap-package-build.log')

if (-not $SkipTests) {
    $previousPath = $env:PATH
    try {
        $env:PATH = (Join-Path $QtRoot 'bin') + ';' + $env:PATH
        & ctest --test-dir $BuildDirectory -C Release --output-on-failure
        if ($LASTEXITCODE -ne 0) {
            throw "Release tests failed."
        }
    } finally {
        $env:PATH = $previousPath
    }
}

if (Test-Path $stagingDirectory) {
    Remove-Item -LiteralPath $stagingDirectory -Recurse -Force
}
New-Item -ItemType Directory -Path $stagingDirectory -Force | Out-Null
& cmake --install $BuildDirectory --config Release --prefix $stagingDirectory
if ($LASTEXITCODE -ne 0) {
    throw "CMake install failed."
}

$applicationPath = Join-Path $stagingDirectory 'QSnapCraft.exe'
if (-not (Test-Path $applicationPath)) {
    throw "Installed QSnapCraft.exe was not found."
}
$deployCommand = "`"$windeployqt`" --release --compiler-runtime --no-translations " +
    "--dir `"$stagingDirectory`" `"$applicationPath`""
Invoke-DeveloperCommand -Command $deployCommand -LogPath (Join-Path $env:TEMP 'qsnap-package-deploy.log')

$licenseDirectory = Join-Path $stagingDirectory 'licenses'
New-Item -ItemType Directory -Path $licenseDirectory -Force | Out-Null
foreach ($licenseName in @('LGPL-3.0-only.txt', 'GPL-3.0-only.txt')) {
    $qtLicense = Join-Path $QtSourceRoot "LICENSES\$licenseName"
    if (-not (Test-Path $qtLicense)) {
        throw "Qt license file was not found at $qtLicense."
    }
    Copy-Item -LiteralPath $qtLicense -Destination $licenseDirectory
}

$sumPath = Join-Path $stagingDirectory 'SHA256SUMS.txt'
$sumLines = Get-ChildItem $stagingDirectory -File -Recurse |
    Where-Object { $_.FullName -ne $sumPath } |
    Sort-Object FullName |
    ForEach-Object {
        $relativePath = $_.FullName.Substring($stagingDirectory.Length).TrimStart('\')
        $hash = (Get-FileHash -LiteralPath $_.FullName -Algorithm SHA256).Hash.ToLowerInvariant()
        "$hash  $relativePath"
    }
$utf8WithoutBom = New-Object System.Text.UTF8Encoding($false)
[System.IO.File]::WriteAllLines($sumPath, [string[]]$sumLines, $utf8WithoutBom)

if (Test-Path $archivePath) {
    Remove-Item -LiteralPath $archivePath -Force
}
Compress-Archive -LiteralPath $stagingDirectory -DestinationPath $archivePath -CompressionLevel Optimal
$archiveHash = (Get-FileHash -LiteralPath $archivePath -Algorithm SHA256).Hash.ToLowerInvariant()
"$archiveHash  $([System.IO.Path]::GetFileName($archivePath))" |
    Set-Content -LiteralPath $hashPath -Encoding ascii

Write-Host "PACKAGE=$archivePath"
Write-Host "SHA256=$archiveHash"
