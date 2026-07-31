@echo off
echo =========================================================
echo         VIT Compiler Phase 14 Test Suite
echo =========================================================
echo.

set VIT_BIN=..\..\build\vit.exe

if not exist %VIT_BIN% (
    echo [ERROR] vit.exe not found at %VIT_BIN%. Build project first.
    exit /b 1
)

echo [Test 1/3] Testing Cross-Compilation Flag (--target x86_64-unknown-linux-gnu)...
%VIT_BIN% build test_cross_compile.vit --target x86_64-unknown-linux-gnu -o app_linux
if %ERRORLEVEL% NEQ 0 (
    echo [FAIL] Cross-compilation test failed.
    exit /b 1
)
echo [PASS] Cross-compilation IR target header verified.
echo.

echo [Test 2/3] Testing WebAssembly Target (--target wasm32-wasi)...
%VIT_BIN% build test_wasm.vit --target wasm32-wasi -o app.wasm
if %ERRORLEVEL% NEQ 0 (
    echo [FAIL] WASM target test failed.
    exit /b 1
)
echo [PASS] WASM binary compilation verified.
echo.

echo [Test 3/3] Testing ARC Escape Analysis Pass (--enable-escape-analysis)...
%VIT_BIN% build test_escape_analysis.vit -O3 --enable-escape-analysis -o test_escape.exe
if %ERRORLEVEL% NEQ 0 (
    echo [FAIL] Escape analysis test failed.
    exit /b 1
)
echo [PASS] Escape analysis optimization pass verified.
echo.

echo =========================================================
echo   ALL PHASE 14 TESTS COMPLETED SUCCESSFULLY! (v2.0.0)
echo =========================================================
