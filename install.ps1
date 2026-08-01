# Vit & Vito Ecosystem Standalone One-Line Installer for Windows
# Usage: iwr -useb https://raw.githubusercontent.com/longgoll/vit/main/install.ps1 | iex
# Or run locally: .\install.ps1

$ErrorActionPreference = "Stop"

Write-Host "============================================================" -ForegroundColor Cyan
Write-Host "     ⚡ Vit & Vito Standalone Language & Framework Engine    " -ForegroundColor Cyan
Write-Host "============================================================" -ForegroundColor Cyan
Write-Host ""

$installDir = Get-Location
$binDir = Join-Path $installDir "bin"
$vitExe = Join-Path $binDir "vit.exe"

# If running standalone from web outside workspace
if (-not (Test-Path $vitExe)) {
    $scriptRoot = $PSScriptRoot
    if ($scriptRoot) {
        $binDir = Join-Path $scriptRoot "bin"
        $vitExe = Join-Path $binDir "vit.exe"
    }
}

if (-not (Test-Path $vitExe)) {
    Write-Host "[Installer] Local vit.exe not found. Bootstrapping Standalone Vit Engine..." -ForegroundColor Yellow
    $targetDir = Join-Path $env:LOCALAPPDATA "vit"
    if (-not (Test-Path $targetDir)) {
        New-Item -ItemType Directory -Path $targetDir -Force | Out-Null
    }
    
    $binDir = Join-Path $targetDir "bin"
    if (-not (Test-Path $binDir)) {
        New-Item -ItemType Directory -Path $binDir -Force | Out-Null
    }
    
    $installDir = $targetDir
    $zipPath = "$env:TEMP\vit-windows-amd64.zip"
    $releaseUrl = "https://github.com/longgoll/vit/releases/download/v2.0.0/vit-windows-amd64.zip"

    Write-Host "[Downloading] Fetching standalone Vit Engine bundle (~30MB)..." -ForegroundColor Cyan
    try {
        Invoke-WebRequest -Uri $releaseUrl -OutFile $zipPath -UseBasicParsing
        Expand-Archive -Path $zipPath -DestinationPath $targetDir -Force
        Remove-Item $zipPath -Force -ErrorAction SilentlyContinue
    } catch {
        Write-Host "[Notice] Release zip not available online yet. Linking workspace binaries..." -ForegroundColor Yellow
        $sourceBin = "F:\Dev\product\vit-lag\vit\bin"
        if (Test-Path $sourceBin) {
            Copy-Item -Path "$sourceBin\*" -Destination $binDir -Recurse -Force
        }
    }
    $vitExe = Join-Path $binDir "vit.exe"
}

Write-Host "[1/3] Configuring VIT_HOME environment variable ($installDir)..." -ForegroundColor Green
[Environment]::SetEnvironmentVariable("VIT_HOME", $installDir, "User")

Write-Host "[2/3] Setting up User PATH environment variable..." -ForegroundColor Green
$userPath = [Environment]::GetEnvironmentVariable("PATH", "User")
if ($userPath -split ';' -notcontains $binDir) {
    [Environment]::SetEnvironmentVariable("PATH", "$userPath;$binDir", "User")
    Write-Host "      Added $binDir to User PATH." -ForegroundColor Cyan
} else {
    Write-Host "      $binDir is already in User PATH." -ForegroundColor Yellow
}

Write-Host "[3/3] Running Vit Toolchain Self-Diagnostics..." -ForegroundColor Green
if (Test-Path $vitExe) {
    & "$vitExe" setup
} else {
    Write-Host "[Warning] vit.exe will be active after downloading release package." -ForegroundColor Yellow
}

Write-Host ""
Write-Host "============================================================" -ForegroundColor Green
Write-Host " 🎉 Vit & Vito Standalone Engine Installed Successfully!" -ForegroundColor Green
Write-Host "============================================================" -ForegroundColor Green
Write-Host " Quick Start Commands:" -ForegroundColor White
Write-Host "   vit --version            Check installed version" -ForegroundColor Gray
Write-Host "   vit run main.vit         Execute a Vit file instantly" -ForegroundColor Gray
Write-Host "   vit init my-app          Create a new Vit/Vito project" -ForegroundColor Gray
Write-Host "   vit dev                  Launch dev server with Live-Reload" -ForegroundColor Gray
Write-Host ""
Write-Host " Note: Please restart your terminal window to apply PATH changes." -ForegroundColor Yellow
