# Script to package Vit Engine & Vito Framework into a Standalone Release Zip Archive
# Usage: .\scripts\pack_release.ps1

$ErrorActionPreference = "Stop"

$projectRoot = Split-Path -Parent $PSScriptRoot
$distDir = Join-Path $projectRoot "dist"
$stageDir = Join-Path $distDir "vit-windows-amd64"
$zipOutput = Join-Path $distDir "vit-v2.0.0-windows-amd64.zip"

Write-Host "============================================================" -ForegroundColor Cyan
Write-Host "  Packaging Standalone Vit and Vito Release Bundle          " -ForegroundColor Cyan
Write-Host "============================================================" -ForegroundColor Cyan
Write-Host ""

if (Test-Path $distDir) {
    Remove-Item -Recurse -Force $distDir -ErrorAction SilentlyContinue
}

New-Item -ItemType Directory -Path $stageDir -Force | Out-Null
New-Item -ItemType Directory -Path "$stageDir\bin" -Force | Out-Null
New-Item -ItemType Directory -Path "$stageDir\std" -Force | Out-Null

Write-Host "[1/4] Copying compiled executables (vit.exe, vit-lsp.exe)..." -ForegroundColor Green
Copy-Item -Path "$projectRoot\bin\vit.exe" -Destination "$stageDir\bin\vit.exe" -Force
Copy-Item -Path "$projectRoot\bin\vit-lsp.exe" -Destination "$stageDir\bin\vit-lsp.exe" -Force

Write-Host "[2/4] Copying standard library files..." -ForegroundColor Green
Copy-Item -Path "$projectRoot\std\*" -Destination "$stageDir\std" -Recurse -Force

if (Test-Path "$projectRoot\tools") {
    Write-Host "[3/4] Copying bundled toolchain..." -ForegroundColor Green
    Copy-Item -Path "$projectRoot\tools" -Destination "$stageDir\tools" -Recurse -Force
}

Copy-Item -Path "$projectRoot\install.ps1" -Destination "$stageDir\install.ps1" -Force
Copy-Item -Path "$projectRoot\README.md" -Destination "$stageDir\README.md" -Force

Write-Host "[4/4] Creating ZIP Archive: $zipOutput..." -ForegroundColor Green
Compress-Archive -Path "$stageDir\*" -DestinationPath $zipOutput -Force

$zipSize = (Get-Item $zipOutput).Length / 1MB
Write-Host ""
Write-Host "============================================================" -ForegroundColor Green
Write-Host " Standalone Release Bundle Created Successfully!" -ForegroundColor Green
Write-Host " Archive Path: $zipOutput" -ForegroundColor White
Write-Host " Package Size: $([math]::Round($zipSize, 2)) MB" -ForegroundColor Yellow
Write-Host "============================================================" -ForegroundColor Green
