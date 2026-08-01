@echo off
echo ===================================================
echo     VIT Phase 15 - Extreme Performance Test Suite  
echo ===================================================
echo.

cd /d "%~dp0..\.."

echo [1/3] Building Vit Compiler (Release)...
cmake -B build
cmake --build build --config Release
if %ERRORLEVEL% NEQ 0 (
    echo [ERROR] Failed to build Vit Compiler.
    exit /b %ERRORLEVEL%
)

echo.
echo [2/3] Compiling and Running C Runtime Unit Tests...
C:\Users\luuho\AppData\Local\Microsoft\WinGet\Packages\BrechtSanders.WinLibs.POSIX.UCRT_Microsoft.Winget.Source_8wekyb3d8bbwe\mingw64\bin\gcc.exe -O3 -Iinclude -Isrc test\Phase15\test_runtime_c.c src\runtime\memory_rt.c src\runtime\async_iouring_rt.c src\runtime\http_parser_simd.c src\runtime\net_rt.c -lws2_32 -o test\Phase15\test_runtime_c.exe
if %ERRORLEVEL% EQU 0 (
    .\test\Phase15\test_runtime_c.exe
) else (
    echo [ERROR] Failed to compile test_runtime_c.c
    exit /b %ERRORLEVEL%
)

echo.
echo [3/3] Testing Vit Compiler with LTO, PGO ^& March Native Flags...
.\build\vit.exe build test\Phase15\test_extreme_perf.vit --lto=thin -march=native -O3 -o test\Phase15\test_extreme_perf.exe
if %ERRORLEVEL% EQU 0 (
    .\test\Phase15\test_extreme_perf.exe
)

echo.
echo [COMPLETE] Phase 15 Extreme Performance Tests Executed.
