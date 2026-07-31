# Script to set up a bundled portable Clang/LLVM toolchain inside project's tools/ directory
# This allows 'vit' to be distributed as a standalone portable package with zero dependencies.

$projectRoot = Join-Path $PSScriptRoot ".." | Resolve-Path
$toolsDir = Join-Path $projectRoot "tools"
$clangTargetDir = Join-Path $toolsDir "clang"
$clangBinTarget = Join-Path $clangTargetDir "bin"
$vitBinDir = Join-Path $projectRoot "build\Debug"

Write-Host "===========================================" -ForegroundColor Cyan
Write-Host "  VIT Compiler - Bundled Toolchain Setup   " -ForegroundColor Cyan
Write-Host "===========================================" -ForegroundColor Cyan
Write-Host ""

if (-not (Test-Path $toolsDir)) {
    New-Item -ItemType Directory -Path $toolsDir -Force | Out-Null
}

# 1. Automatic PATH configuration
$userPath = [Environment]::GetEnvironmentVariable("PATH", "User")
if ($userPath -split ';' -notcontains $vitBinDir) {
    [Environment]::SetEnvironmentVariable("PATH", "$userPath;$vitBinDir", "User")
    Write-Host "[Auto PATH] Added $vitBinDir to User PATH environment variable." -ForegroundColor Green
} else {
    Write-Host "[Auto PATH] $vitBinDir is already in User PATH." -ForegroundColor Yellow
}

# 2. Check if portable clang already exists in tools/clang/bin
if (Test-Path "$clangBinTarget\clang.exe") {
    Write-Host "[Success] Portable Clang toolchain is already bundled in: $clangTargetDir" -ForegroundColor Green
    exit 0
}

# 3. Check existing system installations
$systemClangPaths = @(
    "C:\Program Files\LLVM\bin\clang.exe",
    "C:\LLVM\bin\clang.exe",
    "C:\Program Files (x86)\LLVM\bin\clang.exe"
)

$foundSystemClang = $null
foreach ($p in $systemClangPaths) {
    if (Test-Path $p) {
        $foundSystemClang = $p
        break
    }
}

if ($foundSystemClang) {
    $sourceBinDir = Split-Path $foundSystemClang -Parent
    Write-Host "[Found LLVM] Existing installation detected at: $sourceBinDir" -ForegroundColor Green
    Write-Host "[Bundling] Copying LLVM toolchain into portable tools directory ($clangBinTarget)..." -ForegroundColor Yellow

    if (-not (Test-Path $clangBinTarget)) {
        New-Item -ItemType Directory -Path $clangBinTarget -Force | Out-Null
    }

    Copy-Item -Path "$sourceBinDir\*" -Destination $clangBinTarget -Recurse -Force
    Write-Host "[Success] Portable Clang toolchain bundled into: $clangTargetDir" -ForegroundColor Green
} else {
    Write-Host "[Notice] Downloading portable LLVM toolchain directly from official release..." -ForegroundColor Yellow
    $installerPath = "$env:TEMP\LLVM-19.1.0-win64.exe"
    
    if (-not (Test-Path $installerPath)) {
        Write-Host "[Downloading] LLVM-19.1.0-win64.exe (~350MB)..." -ForegroundColor Cyan
        curl.exe -L -o $installerPath "https://github.com/llvm/llvm-project/releases/download/llvmorg-19.1.0/LLVM-19.1.0-win64.exe"
    }
    
    Write-Host "[Extracting] Unpacking portable LLVM into $clangTargetDir..." -ForegroundColor Yellow
    Unblock-File $installerPath -ErrorAction SilentlyContinue
    cmd.exe /c "`"$installerPath`" /S /D=$clangTargetDir"
    
    if (Test-Path "$clangBinTarget\clang.exe") {
        Write-Host "[Success] Portable Clang toolchain successfully setup in $clangTargetDir!" -ForegroundColor Green
    } else {
        Write-Host "[Error] Failed to setup Clang toolchain. Please check installer." -ForegroundColor Red
    }
}
