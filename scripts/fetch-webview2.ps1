$ErrorActionPreference = "Stop"
$Root = Split-Path -Parent (Split-Path -Parent $MyInvocation.MyCommand.Path)
$Deps = Join-Path $Root "deps"
$PackageDir = Join-Path $Deps "Microsoft.Web.WebView2"
$Version = "1.0.2592.51"

New-Item -ItemType Directory -Force -Path $Deps | Out-Null

if (-not (Test-Path (Join-Path $PackageDir "build/native/include/WebView2.h"))) {
    $nuget = Get-Command nuget -ErrorAction SilentlyContinue
    if (-not $nuget) {
        $nugetExe = Join-Path $Deps "nuget.exe"
        if (-not (Test-Path $nugetExe)) {
            Invoke-WebRequest -Uri "https://dist.nuget.org/win-x86-commandline/latest/nuget.exe" `
                -OutFile $nugetExe
        }
        $nuget = Get-Command $nugetExe
    }
    & $nuget.Source install Microsoft.Web.WebView2 -Version $Version -OutputDirectory $Deps | Out-Null
}

$LoaderSrc = Join-Path $PackageDir "build/native/x64/WebView2Loader.dll"
$LoaderDst = Join-Path $Root "bin/WebView2Loader.dll"
if (Test-Path $LoaderSrc) {
    Copy-Item -Force $LoaderSrc $LoaderDst
}

Write-Host "WebView2 SDK ready at $PackageDir"
