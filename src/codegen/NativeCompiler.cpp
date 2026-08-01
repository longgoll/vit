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

    // 1. Check relative bundled toolchain paths relative to vit.exe
    std::vector<std::string> bundledCandidatePaths = {
        exeDir + "\\tools\\clang\\bin\\clang.exe",
        exeDir + "\\tools\\clang.exe",
        exeDir + "\\tools\\bin\\clang.exe",
        exeDir + "\\..\\tools\\clang\\bin\\clang.exe",
        exeDir + "\\..\\tools\\clang.exe",
        exeDir + "\\..\\tools\\bin\\clang.exe",
        exeDir + "\\..\\..\\tools\\clang\\bin\\clang.exe",
        exeDir + "\\..\\..\\tools\\clang.exe"
    };

    for (const auto& path : bundledCandidatePaths) {
        std::ifstream f(path);
        if (f.good()) {
            return "\"" + path + "\"";
        }
    }

    // 2. Check common standard installation paths on Windows
    std::vector<std::string> standardCandidatePaths = {
        "C:\\LLVM\\bin\\clang.exe",
        "C:\\Program Files\\LLVM\\bin\\clang.exe",
        "C:\\Program Files (x86)\\LLVM\\bin\\clang.exe",
        "C:\\msys64\\ucrt64\\bin\\clang.exe",
        "C:\\msys64\\mingw64\\bin\\clang.exe",
        "C:\\Users\\luuho\\AppData\\Local\\Microsoft\\WinGet\\Packages\\BrechtSanders.WinLibs.POSIX.UCRT_Microsoft.Winget.Source_8wekyb3d8bbwe\\mingw64\\bin\\gcc.exe"
    };

    for (const auto& path : standardCandidatePaths) {
        std::ifstream f(path);
        if (f.good()) {
            return "\"" + path + "\"";
        }
    }

    if (std::system("clang --version > NUL 2>&1") == 0) {
        return "clang";
    }
    if (std::system("gcc --version > NUL 2>&1") == 0) {
        return "gcc";
    }

    return "";
}

static std::string detectGCC() {
    std::vector<std::string> candidates = {
        "C:\\Users\\luuho\\AppData\\Local\\Microsoft\\WinGet\\Packages\\BrechtSanders.WinLibs.POSIX.UCRT_Microsoft.Winget.Source_8wekyb3d8bbwe\\mingw64\\bin\\gcc.exe",
        "C:\\Users\\User\\AppData\\Local\\Microsoft\\WinGet\\Packages\\BrechtSanders.WinLibs.POSIX.UCRT_Microsoft.Winget.Source_8wekyb3d8bbwe\\mingw64\\bin\\gcc.exe",
        "C:\\msys64\\ucrt64\\bin\\gcc.exe",
        "C:\\msys64\\mingw64\\bin\\gcc.exe"
    };
    for (const auto& path : candidates) {
        std::ifstream f(path);
        if (f.good()) {
            return "\"" + path + "\"";
        }
    }
    if (std::system("gcc --version > NUL 2>&1") == 0) {
        return "gcc";
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

static void addRtCandidate(std::string& rtPath, const std::string& exeDir, const std::string& filename) {
    std::vector<std::string> candidates = {
        exeDir + "\\src\\runtime\\" + filename,
        exeDir + "\\..\\src\\runtime\\" + filename,
        exeDir + "\\..\\..\\src\\runtime\\" + filename,
        "src/runtime/" + filename
    };
    for (const auto& candidate : candidates) {
        std::ifstream f(candidate);
        if (f.good()) {
            rtPath += "\"" + normalizeWinPath(candidate) + "\" ";
            break;
        }
    }
}

bool NativeCompiler::compileIRToExecutable(const std::string& irFilePath, const std::string& outputExePath, const std::string& optLevel, const std::string& targetTriple) {
    NativeCompileOptions opts;
    opts.optLevel = optLevel;
    opts.targetTriple = targetTriple;
    return compileIRWithOptions(irFilePath, outputExePath, opts);
}

bool NativeCompiler::compileIRWithOptions(const std::string& irFilePath, const std::string& outputExePath, const NativeCompileOptions& options) {
    if (!isClangAvailable()) {
        std::cerr << "\n[VIT Error] 'clang' or 'gcc' compiler was not found on your system PATH or bundled toolchain.\n";
        return false;
    }

    std::string winIrPath = normalizeWinPath(irFilePath);
    std::string winExePath = normalizeWinPath(outputExePath);
    std::string winObjPath = winExePath + ".o";

    std::string targetFlag = "";
    if (!options.targetTriple.empty()) {
        std::string effectiveTarget = options.targetTriple;
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
    std::string rtPath = "";
    addRtCandidate(rtPath, exeDir, "collections_rt.c");
    addRtCandidate(rtPath, exeDir, "concurrency_rt.c");
    addRtCandidate(rtPath, exeDir, "net_rt.c");
    addRtCandidate(rtPath, exeDir, "memory_rt.c");
    addRtCandidate(rtPath, exeDir, "async_iouring_rt.c");
    addRtCandidate(rtPath, exeDir, "http_parser_simd.c");

    std::string gccPath = detectGCC();
    std::string linkerBinary = gccPath.empty() ? clangExecutablePath : gccPath;
    bool isGCCLinker = (linkerBinary == gccPath);

    if (isGCCLinker) {
        targetFlag = ""; // GCC uses native Win32/MinGW toolchain flags
    }

    // LTO, PGO & CPU Native flags
    std::string extraOptFlags = "";
    if (options.ltoMode == "thin" || options.ltoMode == "full") {
        extraOptFlags += isGCCLinker ? "-flto " : "-flto=thin -fuse-ld=lld ";
    }

    if (options.pgoMode == "generate") {
        std::string profPath = options.pgoPath.empty() ? "default.profraw" : options.pgoPath;
        extraOptFlags += "-fprofile-generate=\"" + profPath + "\" ";
    } else if (options.pgoMode == "use") {
        std::string profPath = options.pgoPath.empty() ? "default.profdata" : options.pgoPath;
        extraOptFlags += "-fprofile-use=\"" + profPath + "\" ";
    }

    if (options.marchNative) {
        extraOptFlags += "-march=native -mtune=native ";
    }

    std::string incFlags = "";
    std::vector<std::string> incCandidates = {
        exeDir + "\\include",
        exeDir + "\\..\\include",
        exeDir + "\\..\\..\\include",
        "include"
    };
    for (const auto& candidate : incCandidates) {
        std::ifstream f(candidate + "\\runtime\\memory_rt.h");
        if (f.good()) {
            incFlags += "-I\"" + normalizeWinPath(candidate) + "\" ";
            break;
        }
    }

    std::vector<std::string> srcCandidates = {
        exeDir + "\\src",
        exeDir + "\\..\\src",
        exeDir + "\\..\\..\\src",
        "src"
    };
    for (const auto& candidate : srcCandidates) {
        std::ifstream f(candidate + "\\runtime\\net_rt.h");
        if (f.good()) {
            incFlags += "-I\"" + normalizeWinPath(candidate) + "\" ";
            break;
        }
    }

#ifdef _WIN32
    std::string sysLibs = (options.targetTriple.find("wasm32") != std::string::npos || options.targetTriple.find("linux") != std::string::npos || options.targetTriple.find("darwin") != std::string::npos) ? "" : "-lws2_32 ";
#else
    std::string sysLibs = "";
#endif

    // Foreign target or WASM build
    if (!options.targetTriple.empty() && (options.targetTriple.find("wasm32") != std::string::npos || options.targetTriple.find("linux") != std::string::npos || options.targetTriple.find("darwin") != std::string::npos)) {
        std::string cmd = clangExecutablePath + " " + targetFlag + options.optLevel + " " + extraOptFlags + " \"" + winIrPath + "\" -c -o \"" + winExePath + "\"";
#ifdef _WIN32
        std::string fullCmd = "cmd.exe /S /C \"" + cmd + "\"";
        return std::system(fullCmd.c_str()) == 0;
#else
        return std::system(cmd.c_str()) == 0;
#endif
    }

    // Step 1: Compile LLVM IR (.ll) to object file (.o) using Clang
    std::string step1OptFlags = (isGCCLinker && !options.ltoMode.empty()) ? "" : extraOptFlags;
    std::string step1TargetFlag = options.targetTriple.empty() ? "--target=x86_64-w64-mingw32 -Wno-override-module " : ("--target=" + options.targetTriple + " ");
    std::string compileObjCmd = clangExecutablePath + " " + step1TargetFlag + options.optLevel + " " + step1OptFlags + " \"" + winIrPath + "\" -c -o \"" + winObjPath + "\"";
#ifdef _WIN32
    std::string step1Cmd = "cmd.exe /S /C \"" + compileObjCmd + "\"";
    int res1 = std::system(step1Cmd.c_str());
#else
    int res1 = std::system(compileObjCmd.c_str());
#endif

    if (res1 != 0) {
        std::cerr << "\n\033[31m[VIT Error]\033[0m LLVM IR compilation to object file failed.\n";
        return false;
    }

    // Step 2: Link object file + C runtime files into executable using GCC or Clang
    std::string linkCmd = linkerBinary + " " + targetFlag + options.optLevel + " " + extraOptFlags + incFlags + "\"" + winObjPath + "\" " + rtPath + sysLibs + "-o \"" + winExePath + "\"";
#ifdef _WIN32
    std::string step2Cmd = "cmd.exe /S /C \"" + linkCmd + "\"";
    int res2 = std::system(step2Cmd.c_str());
#else
    int res2 = std::system(linkCmd.c_str());
#endif

    if (res2 == 0) {
        return true;
    } else {
        std::cerr << "\n\033[31m[VIT Error]\033[0m Linking executable failed with exit code: " << res2 << "\n";
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
