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
    std::vector<std::string> relativeCandidatePaths = {
        exeDir + "\\tools\\clang.exe",
        exeDir + "\\tools\\clang\\bin\\clang.exe",
        exeDir + "\\tools\\bin\\clang.exe",
        exeDir + "\\..\\tools\\clang.exe",
        exeDir + "\\..\\tools\\clang\\bin\\clang.exe",
        exeDir + "\\..\\tools\\bin\\clang.exe",
        exeDir + "\\..\\..\\tools\\clang.exe",
        exeDir + "\\..\\..\\tools\\clang\\bin\\clang.exe",
        exeDir + "\\..\\..\\tools\\bin\\clang.exe"
    };

    for (const auto& path : relativeCandidatePaths) {
        std::ifstream f(path);
        if (f.good()) {
            return "\"" + path + "\"";
        }
    }

    // 2. Check if 'clang' is directly available in system PATH
    int res = std::system("clang --version > NUL 2>&1");
    if (res == 0) {
        return "clang";
    }

    // 3. Check common standard installation paths on Windows
    std::vector<std::string> standardCandidatePaths = {
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

    return "";
}

bool NativeCompiler::isClangAvailable() const {
    return !clangExecutablePath.empty();
}

const std::string& NativeCompiler::getClangPath() const {
    return clangExecutablePath;
}

bool NativeCompiler::compileIRToExecutable(const std::string& irFilePath, const std::string& outputExePath, const std::string& optLevel) {
    if (!isClangAvailable()) {
        std::cerr << "\n[VIT Error] 'clang' compiler was not found on your system PATH or bundled toolchain.\n";
        std::cerr << "  Run '.\\scripts\\bundle_tools.ps1' or 'winget install LLVM.LLVM' to set it up.\n\n";
        return false;
    }

    std::string cmd = clangExecutablePath + " " + optLevel + " \"" + irFilePath + "\" -o \"" + outputExePath + "\"";

#ifdef _WIN32
    // Run initial attempt quietly (suppress stderr/stdout) so fallback won't spam misleading clang errors
    std::string quietCmd = "\"" + cmd + " > NUL 2>&1\"";
    int exitCode = std::system(quietCmd.c_str());
    if (exitCode != 0) {
        std::vector<std::string> minGwLibPaths = {
            "C:\\msys64\\ucrt64\\lib",
            "C:\\msys64\\mingw64\\lib",
            "C:\\Users\\User\\AppData\\Local\\Microsoft\\WinGet\\Packages\\BrechtSanders.WinLibs.POSIX.UCRT_Microsoft.Winget.Source_8wekyb3d8bbwe\\mingw64\\x86_64-w64-mingw32\\lib",
            "C:\\Users\\User\\AppData\\Local\\Microsoft\\WinGet\\Packages\\BrechtSanders.WinLibs.POSIX.UCRT_Microsoft.Winget.Source_8wekyb3d8bbwe\\mingw64\\lib\\gcc\\x86_64-w64-mingw32\\16.1.0"
        };
        std::string fallbackCmd = clangExecutablePath + " " + optLevel + " -fuse-ld=lld --target=x86_64-w64-mingw32 -Wno-override-module";
        for (const auto& libPath : minGwLibPaths) {
            fallbackCmd += " -L\"" + libPath + "\"";
        }
        fallbackCmd += " \"" + irFilePath + "\" -o \"" + outputExePath + "\"";
        std::string fallbackSystemCmd = "\"" + fallbackCmd + "\"";
        exitCode = std::system(fallbackSystemCmd.c_str());
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
#ifdef _WIN32
    std::string runCmd = "\"" + exePath + "\"";
    int exitCode = std::system(runCmd.c_str());
#else
    int exitCode = std::system(exePath.c_str());
#endif
    return exitCode;
}

} // namespace vit
