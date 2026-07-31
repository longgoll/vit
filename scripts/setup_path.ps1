# Helper script to add VIT compiler to User PATH environment variable
# Usage: Run in PowerShell (as Administrator or standard user)

$vitBinDir = Join-Path $PSScriptRoot "..\build\Debug" | Resolve-Path -ErrorAction SilentlyContinue

if (-not $vitBinDir -or -not (Test-Path "$vitBinDir\vit.exe")) {
    Write-Host "Error: vit.exe not found at $vitBinDir. Please build the project first using CMake." -ForegroundColor Red
    exit 1
}

$userPath = [Environment]::GetEnvironmentVariable("PATH", "User")

if ($userPath -split ';' -contains $vitBinDir.Path) {
    Write-Host "VIT compiler path ($vitBinDir) is already in your User PATH!" -ForegroundColor Green
} else {
    $newPath = "$userPath;$($vitBinDir.Path)"
    [Environment]::SetEnvironmentVariable("PATH", $newPath, "User")
    Write-Host "Successfully added $($vitBinDir.Path) to your User PATH!" -ForegroundColor Green
    Write-Host "Please restart your terminal to start using 'vit' from anywhere." -ForegroundColor Yellow
}
