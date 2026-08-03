#include "codegen/JITEngine.h"
#include "codegen/NativeCompiler.h"
#include "utils/Platform.h"

#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

namespace vit {

static std::string getProcIdStr() {
#ifdef _WIN32
    return std::to_string(GetCurrentProcessId());
#else
    return std::to_string(getpid());
#endif
}

JITEngine::JITEngine() {}

std::string JITEngine::detectLLIRunner() {
    std::string exeDir = utils::Platform::getExeDir();

    std::vector<std::string> bundledCandidatePaths = {
        exeDir + "\\tools\\clang\\bin\\lli.exe",
        exeDir + "\\tools\\lli.exe",
        exeDir + "\\tools\\bin\\lli.exe",
        exeDir + "\\..\\tools\\clang\\bin\\lli.exe",
        exeDir + "\\..\\tools\\lli.exe"
    };

    for (const auto& path : bundledCandidatePaths) {
        std::ifstream f(path);
        if (f.good()) {
            return "\"" + path + "\"";
        }
    }

    std::vector<std::string> standardCandidatePaths = {
        "C:\\LLVM\\bin\\lli.exe",
        "C:\\Program Files\\LLVM\\bin\\lli.exe",
        "C:\\Program Files (x86)\\LLVM\\bin\\lli.exe",
        "C:\\msys64\\ucrt64\\bin\\lli.exe",
        "C:\\msys64\\mingw64\\bin\\lli.exe"
    };

    for (const auto& path : standardCandidatePaths) {
        std::ifstream f(path);
        if (f.good()) {
            return "\"" + path + "\"";
        }
    }

#ifdef _WIN32
    if (std::system("lli --version > NUL 2>&1") == 0) {
        return "lli";
    }
#else
    if (std::system("lli --version > /dev/null 2>&1") == 0) {
        return "lli";
    }
#endif

    return "";
}

std::string JITEngine::getOrBuildRuntimeArchive(NativeCompiler& compiler) {
    std::string exeDir = utils::Platform::getExeDir();
    std::filesystem::path archivePath = std::filesystem::path(exeDir) / "vit_runtime.a";
    
    if (!std::filesystem::exists(archivePath)) {
        archivePath = std::filesystem::temp_directory_path() / "vit_runtime.a";
    }

    // Locate runtime C files
    std::vector<std::string> rtFiles = {
        "collections_rt.c", "concurrency_rt.c", "net_rt.c",
        "memory_rt.c", "async_iouring_rt.c", "http_parser_simd.c",
        "simd_json_rt.c", "slab_allocator_rt.c", "kernel_bypass_rt.c",
        "fs_rt.c", "string_rt.c", "time_rt.c", "path_rt.c", "encoding_rt.c", "process_rt.c"
    };

    std::string rtSrcDir = "";
    std::vector<std::string> candidates = {
        exeDir + "\\src\\runtime",
        exeDir + "\\..\\src\\runtime",
        exeDir + "\\..\\..\\src\\runtime",
        "src/runtime"
    };
    for (const auto& cand : candidates) {
        std::ifstream f(cand + "\\memory_rt.c");
        if (f.good()) {
            rtSrcDir = cand;
            break;
        }
    }

    bool needRebuild = false;
    if (!std::filesystem::exists(archivePath)) {
        needRebuild = true;
    } else if (!rtSrcDir.empty()) {
        auto archiveTime = std::filesystem::last_write_time(archivePath);
        for (const auto& rt : rtFiles) {
            std::filesystem::path srcFile = std::filesystem::path(rtSrcDir) / rt;
            if (std::filesystem::exists(srcFile)) {
                if (std::filesystem::last_write_time(srcFile) > archiveTime) {
                    needRebuild = true;
                    break;
                }
            }
        }
    }

    if (!needRebuild) {
        return archivePath.string();
    }

    if (rtSrcDir.empty()) {
        return "";
    }

    std::string incDir = "";
    std::vector<std::string> incCandidates = {
        exeDir + "\\include",
        exeDir + "\\..\\include",
        "include"
    };
    for (const auto& cand : incCandidates) {
        std::ifstream f(cand + "\\runtime\\memory_rt.h");
        if (f.good()) {
            incDir = cand;
            break;
        }
    }

    std::string clangPath = "gcc";
    auto localTools = utils::Platform::getLocalAppDataToolCandidates();
    for (const auto& tpath : localTools) {
        std::ifstream f(tpath);
        if (f.good()) {
            clangPath = "\"" + tpath + "\"";
            break;
        }
    }

    std::filesystem::path tempDir = std::filesystem::temp_directory_path() / "vit_rt_build";
    std::error_code ec;
    std::filesystem::create_directories(tempDir, ec);

    std::string compileObjsCmd = clangPath + " -O2 -c ";
    for (const auto& tpath : localTools) {
        size_t binPos = tpath.find("\\bin\\");
        if (binPos != std::string::npos) {
            std::string prefix = tpath.substr(0, binPos);
            std::ifstream testHeader(prefix + "\\include\\stdio.h");
            if (testHeader.good()) {
                compileObjsCmd += "-I\"" + utils::Platform::normalizePath(prefix + "\\include", true) + "\" ";
                compileObjsCmd += "-I\"" + utils::Platform::normalizePath(prefix + "\\x86_64-w64-mingw32\\include", true) + "\" ";
                break;
            }
        }
    }
    if (!incDir.empty()) {
        compileObjsCmd += "-I\"" + utils::Platform::normalizePath(incDir, true) + "\" ";
    }
    compileObjsCmd += "-I\"" + utils::Platform::normalizePath(rtSrcDir, true) + "\" ";

    for (const auto& rt : rtFiles) {
        compileObjsCmd += "\"" + utils::Platform::normalizePath(rtSrcDir + "\\" + rt, true) + "\" ";
    }

#ifdef _WIN32
    std::string fullCmd = "cd /d \"" + tempDir.string() + "\" && " + compileObjsCmd + " && ar rcs \"" + archivePath.string() + "\" *.o";
    std::string sysCmd = "cmd.exe /S /C \"" + fullCmd + "\"";
    std::system(sysCmd.c_str());
#else
    std::string fullCmd = "cd \"" + tempDir.string() + "\" && " + compileObjsCmd + " && ar rcs \"" + archivePath.string() + "\" *.o";
    std::system(fullCmd.c_str());
#endif

    std::filesystem::remove_all(tempDir, ec);

    if (std::filesystem::exists(archivePath)) {
        return archivePath.string();
    }
    return "";
}

bool JITEngine::isJITAvailable() {
    std::string lli = detectLLIRunner();
    if (!lli.empty()) return true;

    NativeCompiler compiler;
    return compiler.isClangAvailable();
}

int JITEngine::executeIR(const std::string& llvmIR, const std::string& sourceFilePath, const NativeCompileOptions& options) {
    std::string lli = detectLLIRunner();

    if (!lli.empty()) {
        // Fast In-Memory LLVM ORCJIT / ExecutionEngine execution via lli
        auto now = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
        std::filesystem::path tempIR = std::filesystem::temp_directory_path() / ("jit_" + getProcIdStr() + "_" + std::to_string(now) + ".ll");

        std::ofstream out(tempIR);
        if (out.is_open()) {
            out << llvmIR;
            out.close();
        }

        std::string runCmd = lli + " \"" + tempIR.string() + "\"";
#ifdef _WIN32
        std::string sysCmd = "cmd.exe /S /C \"" + runCmd + "\"";
        int exitCode = std::system(sysCmd.c_str());
#else
        int exitCode = std::system(runCmd.c_str());
#endif
        std::error_code ec;
        std::filesystem::remove(tempIR, ec);
        return exitCode;
    }

    // Fast In-Memory linking fallback using cached runtime static archive (vit_runtime.a)
    NativeCompiler compiler;
    std::string rtArchive = getOrBuildRuntimeArchive(compiler);

    auto now = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    std::filesystem::path tempDir = std::filesystem::temp_directory_path() / ("vit_fastrun_" + getProcIdStr() + "_" + std::to_string(now));
    std::error_code ec;
    std::filesystem::create_directories(tempDir, ec);

    std::filesystem::path tempIR = tempDir / "code.ll";
    std::filesystem::path tempObj = tempDir / "code.o";
    std::filesystem::path tempExe = tempDir / "app.exe";

    std::ofstream out(tempIR);
    if (out.is_open()) {
        out << llvmIR;
        out.close();
    }

    std::string clangPath = compiler.getClangPath();

    // Detect MinGW sysroot for Clang step1 (IR→obj), and GCC for step2 (obj→exe)
    std::string sysrootPath = "";
    std::string gccLinker = "";
    auto localFastTools = utils::Platform::getLocalAppDataToolCandidates();
    for (const auto& tpath : localFastTools) {
        size_t binPos = tpath.find("\\bin\\");
        if (binPos != std::string::npos) {
            std::string prefix = tpath.substr(0, binPos);
            std::ifstream testHeader(prefix + "\\include\\stdio.h");
            if (testHeader.good()) {
                sysrootPath = prefix;
                break;
            }
        }
        // Detect gcc.exe
        if (tpath.find("gcc.exe") != std::string::npos) {
            std::ifstream f(tpath);
            if (f.good() && gccLinker.empty()) {
                gccLinker = "\"" + tpath + "\"";
            }
        }
    }
    if (gccLinker.empty()) gccLinker = "gcc";

    // Step 1: Compile IR to Object file (fast -O0)
    std::string sysrootArg = sysrootPath.empty() ? "" : "--sysroot=\"" + utils::Platform::normalizePath(sysrootPath, true) + "\" ";
    std::string step1Cmd = clangPath + " -O0 -w --target=x86_64-w64-mingw32 -Wno-override-module " + sysrootArg + "-c \"" + tempIR.string() + "\" -o \"" + tempObj.string() + "\"";
#ifdef _WIN32
    std::string sysStep1 = "cmd.exe /S /C \"" + step1Cmd + "\"";
    int res1 = std::system(sysStep1.c_str());
#else
    int res1 = std::system(step1Cmd.c_str());
#endif

    if (res1 != 0) {
        std::filesystem::remove_all(tempDir, ec);
        return res1;
    }

    // Step 2: Ultra-fast link with pre-compiled vit_runtime.a archive (use GCC — knows own headers)
    std::string step2Cmd = gccLinker + " -w \"" + tempObj.string() + "\" ";
    if (!rtArchive.empty()) {
        step2Cmd += "\"" + rtArchive + "\" ";
    }
#ifdef _WIN32
    step2Cmd += "-lws2_32 ";
#endif
    step2Cmd += "-o \"" + tempExe.string() + "\"";

#ifdef _WIN32
    std::string sysStep2 = "cmd.exe /S /C \"" + step2Cmd + "\"";
    int res2 = std::system(sysStep2.c_str());
#else
    int res2 = std::system(step2Cmd.c_str());
#endif

    if (res2 != 0) {
        std::filesystem::remove_all(tempDir, ec);
        return res2;
    }

    // Step 3: Execute in RAM/Process immediately
    int exitCode = compiler.runExecutable(tempExe.string());

    // Clean up temporary files
    std::filesystem::remove_all(tempDir, ec);

    return exitCode;
}

} // namespace vit
