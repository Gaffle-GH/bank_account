param(
    [Parameter(Mandatory = $true, Position = 0)]
    [string]$Platform,

    [Parameter(Position = 1)]
    [string]$Version = "dev"
)

$ErrorActionPreference = "Stop"
$Root = Split-Path -Parent (Split-Path -Parent $MyInvocation.MyCommand.Path)
Set-Location $Root

$Staging = Join-Path "dist" "bank-account-$Platform-$Version"
$Archive = Join-Path "dist" "bank-account-$Platform-$Version.zip"

New-Item -ItemType Directory -Force -Path "dist" | Out-Null
if (Test-Path $Staging) {
    Remove-Item -Recurse -Force $Staging
}
New-Item -ItemType Directory -Force -Path (Join-Path $Staging "web") | Out-Null

if ($Platform -ne "windows") {
    throw "package-release.ps1 only supports windows packaging"
}

foreach ($name in @("account.exe", "account_gui.exe", "account_web.exe")) {
    $source = Join-Path "bin" $name
    if (-not (Test-Path $source)) {
        throw "Missing $source"
    }
    Copy-Item $source $Staging
}

Copy-Item "web/index.html", "web/styles.css", "web/app.js" (Join-Path $Staging "web")
Copy-Item "LICENSE" $Staging
Copy-Item "scripts/RUN.txt" $Staging -ErrorAction SilentlyContinue
if (-not (Test-Path (Join-Path $Staging "RUN.txt"))) {
    @"
Bank Account Program
====================

Keep this folder together. The programs look for the web/ folder next to them.

Windows
-------
  account_gui.exe   Native window (FLTK)
  account_web.exe   Open http://127.0.0.1:8765 in your browser
  account.exe       Terminal menu version

Account files are saved as NAME_accountINFOCARD.txt in this folder.
"@ | Set-Content -Path (Join-Path $Staging "RUN.txt") -Encoding UTF8
}

if (Test-Path $Archive) {
    Remove-Item -Force $Archive
}
Compress-Archive -Path $Staging -DestinationPath $Archive
Write-Host "Created $Archive"
