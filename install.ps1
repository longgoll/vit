# Vit & Vito Ecosystem One-Line PowerShell Installer for Windows
# Usage: iwr -useb https://raw.githubusercontent.com/longgoll/vit/main/install.ps1 | iex
# Or run locally: .\install.ps1

$ErrorActionPreference = "Stop"

Write-Host "============================================================" -ForegroundColor Cyan
Write-Host "     ⚡ Vit & Vito Language & Framework Installer (v2.0)    " -ForegroundColor Cyan
Write-Host "============================================================" -ForegroundColor Cyan
Write-Host ""

$installDir = Get-Location
$binDir = Join-Path $installDir "bin"
$vitExe = Join-Path $binDir "vit.exe"

if (-not (Test-Path $vitExe)) {
    # If installed from outside workspace, locate root
    $scriptRoot = $PSScriptRoot
    if ($scriptRoot) {
        $binDir = Join-Path $scriptRoot "bin"
        $vitExe = Join-Path $binDir "vit.exe"
    }
}

if (-not (Test-Path $vitExe)) {
    Write-Host "[Installer] Error: vit.exe not found in $binDir" -ForegroundColor Red
    Write-Host "[Installer] Please ensure you are running installer in the Vit directory or download the release bundle." -ForegroundColor Yellow
    exit 1
}

Write-Host "[1/3] Setting VIT_HOME environment variable..." -ForegroundColor Green
[Environment]::SetEnvironmentVariable("VIT_HOME", $installDir.Path, "User")

Write-Host "[2/3] Configuring User PATH environment variable..." -ForegroundColor Green
$userPath = [Environment]::GetEnvironmentVariable("PATH", "User")
if ($userPath -split ';' -notcontains $binDir) {
    [Environment]::SetEnvironmentVariable("PATH", "$userPath;$binDir", "User")
    Write-Host "      Added $binDir to User PATH." -ForegroundColor Cyan
} else {
    Write-Host "      $binDir is already in User PATH." -ForegroundColor Yellow
}

Write-Host "[3/3] Running Vit Toolchain Self-Diagnostics..." -ForegroundColor Green
& "$vitExe" setup

Write-Host ""
Write-Host "============================================================" -ForegroundColor Green
Write-Host " 🎉 Vit & Vito installed successfully!" -ForegroundColor Green
Write-Host "============================================================" -ForegroundColor Green
Write-Host " Quick Start Commands:" -ForegroundColor White
Write-Host "   vit --version            Check installed version" -ForegroundColor Gray
Write-Host "   vit run main.vit         Execute a Vit file instantly" -ForegroundColor Gray
Write-Host "   vit init my-app          Create a new Vit/Vito project" -ForegroundColor Gray
Write-Host ""
Write-Host " Note: Please restart your terminal window to apply PATH changes." -ForegroundColor Yellow
