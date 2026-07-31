@echo off
echo ========================================================
echo               VIT Phase 13 Test Suite
echo ========================================================

set VIT=..\..\bin\vit.exe

echo.
echo [1/5] Testing Package Manager: 'vit init test-app'...
%VIT% init test-app
if errorlevel 1 (
    echo [FAIL] 'vit init' failed!
    exit /b 1
)

cd test-app
echo.
echo [2/5] Testing Package Manager: 'vit add github.com/user/vit-http'...
..\..\..\bin\vit.exe add github.com/user/vit-http
if errorlevel 1 (
    echo [FAIL] 'vit add' failed!
    cd ..
    exit /b 1
)

echo.
echo [3/5] Testing Package Manager: 'vit install'...
..\..\..\bin\vit.exe install
if errorlevel 1 (
    echo [FAIL] 'vit install' failed!
    cd ..
    exit /b 1
)
cd ..

echo.
echo [4/5] Testing Formatter: 'vit fmt test_fmt_sample.vit'...
%VIT% fmt test_fmt_sample.vit
if errorlevel 1 (
    echo [FAIL] 'vit fmt' failed!
    exit /b 1
)

echo.
echo [5/5] Testing Linter: 'vit lint test_lint_sample.vit'...
%VIT% lint test_lint_sample.vit
if errorlevel 1 (
    echo [FAIL] 'vit lint' failed!
    exit /b 1
)

echo.
echo ========================================================
echo               ALL PHASE 13 TESTS PASSED!
echo ========================================================
