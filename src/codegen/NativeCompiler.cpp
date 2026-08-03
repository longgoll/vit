#include "codegen/NativeCompiler.h"
#include "utils/Platform.h"
#include <cstdlib>
#include <filesystem>
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

std::string NativeCompiler::detectClang() {
    static std::string cachedClangPath = "";
    static bool isDetected = false;
    if (isDetected) return cachedClangPath;

    std::string exeDir = utils::Platform::getExeDir();

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
            cachedClangPath = "\"" + path + "\"";
            isDetected = true;
            return cachedClangPath;
        }
    }

    auto localAppDataCandidates = utils::Platform::getLocalAppDataToolCandidates();
    std::vector<std::string> standardCandidatePaths = localAppDataCandidates;
    standardCandidatePaths.push_back("C:\\msys64\\ucrt64\\bin\\clang.exe");
    standardCandidatePaths.push_back("C:\\msys64\\mingw64\\bin\\clang.exe");
    standardCandidatePaths.push_back("C:\\LLVM\\bin\\clang.exe");
    standardCandidatePaths.push_back("C:\\Program Files\\LLVM\\bin\\clang.exe");

    for (const auto& path : standardCandidatePaths) {
        std::ifstream f(path);
        if (f.good()) {
            cachedClangPath = "\"" + path + "\"";
            isDetected = true;
            return cachedClangPath;
        }
    }

    if (std::system("clang --version > NUL 2>&1") == 0) {
        cachedClangPath = "clang";
        isDetected = true;
        return cachedClangPath;
    }
    if (std::system("gcc --version > NUL 2>&1") == 0) {
        cachedClangPath = "gcc";
        isDetected = true;
        return cachedClangPath;
    }

    isDetected = true;
    cachedClangPath = "";
    return cachedClangPath;
}

static std::string detectGCC() {
    static std::string cachedGCCPath = "";
    static bool isDetected = false;
    if (isDetected) return cachedGCCPath;

    std::vector<std::string> candidates = {
        "C:\\Users\\User\\AppData\\Local\\Microsoft\\WinGet\\Packages\\BrechtSanders.WinLibs.POSIX.UCRT_Microsoft.Winget.Source_8wekyb3d8bbwe\\mingw64\\bin\\gcc.exe",
        "C:\\msys64\\ucrt64\\bin\\gcc.exe",
        "C:\\msys64\\mingw64\\bin\\gcc.exe"
    };
    auto localAppDataCandidates = utils::Platform::getLocalAppDataToolCandidates();
    candidates.insert(candidates.end(), localAppDataCandidates.begin(), localAppDataCandidates.end());

    for (const auto& path : candidates) {
        std::ifstream f(path);
        if (f.good()) {
            cachedGCCPath = "\"" + path + "\"";
            isDetected = true;
            return cachedGCCPath;
        }
    }
    cachedGCCPath = "gcc";
    isDetected = true;
    return cachedGCCPath;
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

    std::string exeDir = utils::Platform::getExeDir();
    std::string rtPath = "";
    addRtCandidate(rtPath, exeDir, "collections_rt.c");
    addRtCandidate(rtPath, exeDir, "concurrency_rt.c");
    addRtCandidate(rtPath, exeDir, "net_rt.c");
    addRtCandidate(rtPath, exeDir, "memory_rt.c");
    addRtCandidate(rtPath, exeDir, "async_iouring_rt.c");
    addRtCandidate(rtPath, exeDir, "http_parser_simd.c");
    addRtCandidate(rtPath, exeDir, "simd_json_rt.c");
    addRtCandidate(rtPath, exeDir, "slab_allocator_rt.c");
    addRtCandidate(rtPath, exeDir, "kernel_bypass_rt.c");
    addRtCandidate(rtPath, exeDir, "fs_rt.c");
    addRtCandidate(rtPath, exeDir, "string_rt.c");
    addRtCandidate(rtPath, exeDir, "time_rt.c");
    addRtCandidate(rtPath, exeDir, "path_rt.c");
    addRtCandidate(rtPath, exeDir, "encoding_rt.c");
    addRtCandidate(rtPath, exeDir, "process_rt.c");

    std::string gccPath = detectGCC();
    std::string linkerBinary = gccPath.empty() ? clangExecutablePath : gccPath;
    bool isGCCLinker = (linkerBinary == gccPath);

    if (isGCCLinker) {
        targetFlag = ""; // GCC uses native Win32/MinGW toolchain flags
    }

    // LTO, PGO & CPU Native flags
    std::string extraOptFlags = "";
    if (options.optLevel == "-O3" || options.optLevel == "-O2") {
        extraOptFlags += isGCCLinker ? "-funroll-loops -ftree-vectorize " : "-funroll-loops -fvectorize ";
    }

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

    std::string sysLibs = (options.targetTriple.find("wasm32") != std::string::npos || options.targetTriple.find("linux") != std::string::npos || options.targetTriple.find("darwin") != std::string::npos) ? "" : "-lws2_32 ";

    // Detect MinGW sysroot for Clang to find C standard headers (stdio.h, stddef.h etc.)
    std::string sysrootFlag = "";
    auto localToolCandidates = utils::Platform::getLocalAppDataToolCandidates();
    for (const auto& tpath : localToolCandidates) {
        size_t binPos = tpath.find("\\bin\\");
        if (binPos != std::string::npos) {
            std::string prefix = tpath.substr(0, binPos);
            std::ifstream testHeader(prefix + "\\include\\stdio.h");
            if (testHeader.good()) {
                sysrootFlag = "--sysroot=\"" + normalizeWinPath(prefix) + "\" ";
                incFlags += "-I\"" + normalizeWinPath(prefix + "\\include") + "\" ";
                incFlags += "-I\"" + normalizeWinPath(prefix + "\\x86_64-w64-mingw32\\include") + "\" ";
                break;
            }
        }
    }

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

    // Step 1: Compile LLVM IR (.ll) to object file (.o)
    // When using GCC linker, use GCC itself (it understands LLVM IR via -x ir or direct pass)
    // Actually GCC does not accept LLVM IR; we still need clang for step1, but with correct sysroot.
    // If GCC is available, use it with -x assembler fallback. But LLVM IR needs clang.
    // Key fix: pass the correct --sysroot or include dirs to clang when invoking step1.
    std::string step1Compiler = clangExecutablePath;
    std::string step1OptFlags = (isGCCLinker && !options.ltoMode.empty()) ? "" : extraOptFlags;
    std::string step1TargetFlag = options.targetTriple.empty() ? "--target=x86_64-w64-mingw32 -Wno-override-module " : ("--target=" + options.targetTriple + " ");
    std::string compileObjCmd = step1Compiler + " " + step1TargetFlag + sysrootFlag + options.optLevel + " " + step1OptFlags + " \"" + winIrPath + "\" -c -o \"" + winObjPath + "\"";
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
    std::string linkCmd = linkerBinary + " " + targetFlag + sysrootFlag + options.optLevel + " " + extraOptFlags + incFlags + "\"" + winObjPath + "\" " + rtPath + sysLibs + "-o \"" + winExePath + "\"";
#ifdef _WIN32
    std::string step2Cmd = "cmd.exe /S /C \"" + linkCmd + "\"";
    int res2 = std::system(step2Cmd.c_str());
#else
    int res2 = std::system(linkCmd.c_str());
#endif

    // Clean up intermediate object file
    std::filesystem::remove(winObjPath);

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
