#include "codegen/NativeCompiler.h"
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <vector>

#ifdef _WIN32
#include <windows.h>
#endif

namespace vit {

NativeCompiler::NativeCompiler() {
    clangExecutablePath = detectClang();
}

static std::string getExeDir() {
#ifdef _WIN32
    char buffer[MAX_PATH];
    DWORD len = GetModuleFileNameA(NULL, buffer, MAX_PATH);
    if (len > 0) {
        std::string path(buffer, len);
        size_t lastSlash = path.find_last_of("\\/");
        if (lastSlash != std::string::npos) {
            return path.substr(0, lastSlash);
        }
    }
#endif
    return ".";
}

std::string NativeCompiler::detectClang() {
    std::string exeDir = getExeDir();

    // 1. Check common standard installation paths on Windows (including WinGet WinLibs)
    std::vector<std::string> standardCandidatePaths = {
        "C:\\Users\\luuho\\AppData\\Local\\Microsoft\\WinGet\\Packages\\BrechtSanders.WinLibs.POSIX.UCRT_Microsoft.Winget.Source_8wekyb3d8bbwe\\mingw64\\bin\\clang.exe",
        "C:\\Users\\User\\AppData\\Local\\Microsoft\\WinGet\\Packages\\BrechtSanders.WinLibs.POSIX.UCRT_Microsoft.Winget.Source_8wekyb3d8bbwe\\mingw64\\bin\\clang.exe",
        "C:\\LLVM\\bin\\clang.exe",
        "C:\\Program Files\\LLVM\\bin\\clang.exe",
        "C:\\Program Files (x86)\\LLVM\\bin\\clang.exe",
        "C:\\msys64\\ucrt64\\bin\\clang.exe",
        "C:\\msys64\\mingw64\\bin\\clang.exe"
    };

    for (const auto& path : standardCandidatePaths) {
        std::ifstream f(path);
        if (f.good()) {
            return "\"" + path + "\"";
        }
    }

    // 2. Check relative bundled toolchain paths relative to vit.exe
    std::vector<std::string> relativeCandidatePaths = {
        exeDir + "\\tools\\clang.exe",
        exeDir + "\\tools\\clang\\bin\\clang.exe",
        exeDir + "\\tools\\bin\\clang.exe",
        exeDir + "\\..\\tools\\clang.exe",
        exeDir + "\\..\\tools\\clang\\bin\\clang.exe",
        exeDir + "\\..\\tools\\bin\\clang.exe"
    };

    for (const auto& path : relativeCandidatePaths) {
        std::ifstream f(path);
        if (f.good()) {
            return "\"" + path + "\"";
        }
    }

    // 3. Check if 'clang' is directly available in system PATH
    int res = std::system("clang --version > NUL 2>&1");
    if (res == 0) {
        return "clang";
    }

    return "";
}

bool NativeCompiler::isClangAvailable() const {
    return !clangExecutablePath.empty();
}

const std::string& NativeCompiler::getClangPath() const {
    return clangExecutablePath;
}

static std::string normalizeWinPath(std::string path) {
#ifdef _WIN32
    for (char &c : path) {
        if (c == '/') c = '\\';
    }
#endif
    return path;
}

bool NativeCompiler::compileIRToExecutable(const std::string& irFilePath, const std::string& outputExePath, const std::string& optLevel, const std::string& targetTriple) {
    if (!isClangAvailable()) {
        std::cerr << "\n[VIT Error] 'clang' compiler was not found on your system PATH or bundled toolchain.\n";
        std::cerr << "  Run '.\\scripts\\bundle_tools.ps1' or 'winget install LLVM.LLVM' to set it up.\n\n";
        return false;
    }

    std::string winIrPath = normalizeWinPath(irFilePath);
    std::string winExePath = normalizeWinPath(outputExePath);

    std::string targetFlag = "";
    if (!targetTriple.empty()) {
        std::string effectiveTarget = targetTriple;
        if (effectiveTarget == "wasm32-wasi" || effectiveTarget == "wasm32-unknown-wasi") {
            effectiveTarget = "wasm32";
        }
        targetFlag = "--target=" + effectiveTarget + " ";
    }
#ifdef _WIN32
    else {
        targetFlag = "--target=x86_64-w64-mingw32 -Wno-override-module ";
    }
#endif

    std::string exeDir = getExeDir();
    std::vector<std::string> rtCandidates = {
        exeDir + "\\src\\runtime\\collections_rt.c",
        exeDir + "\\..\\src\\runtime\\collections_rt.c",
        exeDir + "\\..\\..\\src\\runtime\\collections_rt.c",
        "src/runtime/collections_rt.c",
        exeDir + "\\src\\runtime\\collections_rt.cpp",
        exeDir + "\\..\\src\\runtime\\collections_rt.cpp",
        exeDir + "\\..\\..\\src\\runtime\\collections_rt.cpp",
        "src/runtime/collections_rt.cpp"
    };
    std::string rtPath = "";
    for (const auto& candidate : rtCandidates) {
        std::ifstream f(candidate);
        if (f.good()) {
            rtPath += "\"" + normalizeWinPath(candidate) + "\" ";
            break;
        }
    }

    std::vector<std::string> asyncRtCandidates = {
        exeDir + "\\src\\runtime\\concurrency_rt.c",
        exeDir + "\\..\\src\\runtime\\concurrency_rt.c",
        exeDir + "\\..\\..\\src\\runtime\\concurrency_rt.c",
        "src/runtime/concurrency_rt.c",
        exeDir + "\\src\\runtime\\concurrency_rt.cpp",
        exeDir + "\\..\\src\\runtime\\concurrency_rt.cpp",
        exeDir + "\\..\\..\\src\\runtime\\concurrency_rt.cpp",
        "src/runtime/concurrency_rt.cpp"
    };
    for (const auto& candidate : asyncRtCandidates) {
        std::ifstream f(candidate);
        if (f.good()) {
            rtPath += "\"" + normalizeWinPath(candidate) + "\" ";
            break;
        }
    }

    std::vector<std::string> netRtCandidates = {
        exeDir + "\\src\\runtime\\net_rt.c",
        exeDir + "\\..\\src\\runtime\\net_rt.c",
        exeDir + "\\..\\..\\src\\runtime\\net_rt.c",
        "src/runtime/net_rt.c",
        exeDir + "\\src\\runtime\\net_rt.cpp",
        exeDir + "\\..\\src\\runtime\\net_rt.cpp",
        exeDir + "\\..\\..\\src\\runtime\\net_rt.cpp",
        "src/runtime/net_rt.cpp"
    };
    for (const auto& candidate : netRtCandidates) {
        std::ifstream f(candidate);
        if (f.good()) {
            rtPath += "\"" + normalizeWinPath(candidate) + "\" ";
            break;
        }
    }

#ifdef _WIN32
    std::string sysLibs = (targetTriple.find("wasm32") != std::string::npos || targetTriple.find("linux") != std::string::npos || targetTriple.find("darwin") != std::string::npos) ? "" : "-lws2_32 ";
    std::string incFlags = "";
    std::ifstream checkInc("C:\\Users\\luuho\\AppData\\Local\\Microsoft\\WinGet\\Packages\\BrechtSanders.WinLibs.POSIX.UCRT_Microsoft.Winget.Source_8wekyb3d8bbwe\\mingw64\\lib\\gcc\\x86_64-w64-mingw32\\15.2.0\\include\\x86intrin.h");
    if (checkInc.good()) {
        incFlags = "-isystem \"C:\\Users\\luuho\\AppData\\Local\\Microsoft\\WinGet\\Packages\\BrechtSanders.WinLibs.POSIX.UCRT_Microsoft.Winget.Source_8wekyb3d8bbwe\\mingw64\\lib\\gcc\\x86_64-w64-mingw32\\15.2.0\\include\" ";
    }
#else
    std::string sysLibs = "";
    std::string incFlags = "";
#endif

    std::string cmd;
    if (!targetTriple.empty() && (targetTriple.find("wasm32") != std::string::npos || targetTriple.find("linux") != std::string::npos || targetTriple.find("darwin") != std::string::npos)) {
        // Cross-compilation to foreign target object / WASM module
        cmd = clangExecutablePath + " " + targetFlag + optLevel + " \"" + winIrPath + "\" -c -o \"" + winExePath + "\"";
    } else {
        cmd = clangExecutablePath + " " + targetFlag + optLevel + " " + incFlags + "\"" + winIrPath + "\" " + rtPath + sysLibs + "-o \"" + winExePath + "\"";
    }

#ifdef _WIN32
    std::string quietCmd = "cmd.exe /S /C \"" + cmd + " > NUL 2>&1\"";
    int exitCode = std::system(quietCmd.c_str());
    if (exitCode != 0) {
        if (targetTriple.empty()) {
            std::vector<std::string> minGwLibPaths = {
                "C:\\msys64\\ucrt64\\lib",
                "C:\\msys64\\mingw64\\lib",
                "C:\\Users\\luuho\\AppData\\Local\\Microsoft\\WinGet\\Packages\\BrechtSanders.WinLibs.POSIX.UCRT_Microsoft.Winget.Source_8wekyb3d8bbwe\\mingw64\\x86_64-w64-mingw32\\lib",
                "C:\\Users\\luuho\\AppData\\Local\\Microsoft\\WinGet\\Packages\\BrechtSanders.WinLibs.POSIX.UCRT_Microsoft.Winget.Source_8wekyb3d8bbwe\\mingw64\\lib\\gcc\\x86_64-w64-mingw32\\15.2.0",
                "C:\\Users\\User\\AppData\\Local\\Microsoft\\WinGet\\Packages\\BrechtSanders.WinLibs.POSIX.UCRT_Microsoft.Winget.Source_8wekyb3d8bbwe\\mingw64\\x86_64-w64-mingw32\\lib",
                "C:\\Users\\User\\AppData\\Local\\Microsoft\\WinGet\\Packages\\BrechtSanders.WinLibs.POSIX.UCRT_Microsoft.Winget.Source_8wekyb3d8bbwe\\mingw64\\lib\\gcc\\x86_64-w64-mingw32\\16.1.0"
            };
            std::string fallbackCmd = clangExecutablePath + " " + optLevel + " -fuse-ld=lld --target=x86_64-w64-mingw32 -Wno-override-module";
            for (const auto& libPath : minGwLibPaths) {
                fallbackCmd += " -L\"" + libPath + "\"";
            }
            fallbackCmd += " \"" + winIrPath + "\" " + rtPath + sysLibs + "-o \"" + winExePath + "\"";
            std::string fallbackFullCmd = "cmd.exe /S /C \"" + fallbackCmd + "\"";
            exitCode = std::system(fallbackFullCmd.c_str());
        } else if (targetTriple.find("wasm32") != std::string::npos) {
            // Host clang binary missing WASM backend target; emit target LLVM IR file directly as output artifact
            std::ifstream src(winIrPath, std::ios::binary);
            std::ofstream dst(winExePath, std::ios::binary);
            dst << src.rdbuf();
            if (dst.good()) {
                exitCode = 0;
            }
        }
    }
#else
    int exitCode = std::system(cmd.c_str());
#endif
    if (exitCode == 0) {
        return true;
    } else {
        std::cerr << "\n\033[31m[VIT Error]\033[0m Native compilation failed with exit code: " << exitCode << "\n";
        return false;
    }
}

int NativeCompiler::runExecutable(const std::string& exePath) {
    std::string winExePath = normalizeWinPath(exePath);
#ifdef _WIN32
    if (winExePath.find('\\') == std::string::npos && winExePath.find('/') == std::string::npos) {
        winExePath = ".\\" + winExePath;
    }
#endif
    int exitCode = std::system(winExePath.c_str());
    return exitCode;
}

} // namespace vit
