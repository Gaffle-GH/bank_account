param(
    [Parameter(Mandatory = $true, Position = 0)]
    [string]$Platform,

    [Parameter(Position = 1)]
    [string]$Version = "dev"
)

$ErrorActionPreference = "Stop"
$Root = Split-Path -Parent (Split-Path -Parent $MyInvocation.MyCommand.Path)
Set-Location $Root

if ($Platform -ne "windows") {
    throw "package-release.ps1 only supports windows packaging"
}

$Staging = Join-Path "dist" "bank-account-$Platform-$Version"
$Archive = Join-Path "dist" "bank-account-$Platform-$Version.zip"

New-Item -ItemType Directory -Force -Path "dist" | Out-Null
if (Test-Path $Staging) {
    Remove-Item -Recurse -Force $Staging
}
New-Item -ItemType Directory -Force -Path $Staging | Out-Null

$App = "BankAccount.exe"
$source = Join-Path "bin" $App
if (-not (Test-Path $source)) {
    throw "Missing $source - run 'make app' first"
}

Copy-Item $source $Staging
$loader = Join-Path "bin" "WebView2Loader.dll"
if (Test-Path $loader) {
    Copy-Item $loader $Staging
}
Copy-Item "LICENSE" $Staging

@"
Bank Account
============

This is a single, self-contained application - the web UI is built in, so
there are no extra files to keep alongside it.

Windows
-------
  Double-click "BankAccount.exe".
  Keep "WebView2Loader.dll" in the same folder as the executable.

Account data is saved into the folder you choose on the login screen
(blank = your Home folder), as NAME_accountINFOCARD.txt.
"@ | Set-Content -Path (Join-Path $Staging "RUN.txt") -Encoding UTF8

if (Test-Path $Archive) {
    Remove-Item -Force $Archive
}
Compress-Archive -Path $Staging -DestinationPath $Archive
Write-Host "Created $Archive"
