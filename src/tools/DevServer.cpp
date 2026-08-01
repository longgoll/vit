#include "tools/DevServer.h"
#include <iostream>
#include <filesystem>
#include <thread>
#include <chrono>
#include <vector>
#include <unordered_map>

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <signal.h>
#endif

namespace vit {

namespace fs = std::filesystem;

struct ProcessInfo {
#ifdef _WIN32
    PROCESS_INFORMATION pi{};
    bool valid = false;
#else
    pid_t pid = -1;
    bool valid = false;
#endif
};

static ProcessInfo spawnProcess(const std::string& exePath, const std::string& targetFile) {
    ProcessInfo pinfo;
    std::cout << "\033[36m[VIT Dev]\033[0m Launching application: \033[33m" << targetFile << "\033[0m...\n";

#ifdef _WIN32
    std::string cmd = "\"" + exePath + "\" run \"" + targetFile + "\"";
    STARTUPINFOA si{};
    si.cb = sizeof(si);
    
    char cmdBuffer[1024];
    strncpy(cmdBuffer, cmd.c_str(), sizeof(cmdBuffer));

    if (CreateProcessA(NULL, cmdBuffer, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pinfo.pi)) {
        pinfo.valid = true;
    } else {
        std::cerr << "\033[31m[VIT Dev Error]\033[0m Failed to spawn process (" << GetLastError() << ")\n";
    }
#else
    pid_t pid = fork();
    if (pid == 0) {
        // Child
        execl(exePath.c_str(), exePath.c_str(), "run", targetFile.c_str(), (char*)NULL);
        exit(1);
    } else if (pid > 0) {
        pinfo.pid = pid;
        pinfo.valid = true;
    } else {
        std::cerr << "\033[31m[VIT Dev Error]\033[0m Fork failed\n";
    }
#endif
    return pinfo;
}

static void killProcess(ProcessInfo& pinfo) {
    if (!pinfo.valid) return;
#ifdef _WIN32
    TerminateProcess(pinfo.pi.hProcess, 0);
    CloseHandle(pinfo.pi.hProcess);
    CloseHandle(pinfo.pi.hThread);
#else
    kill(pinfo.pid, SIGTERM);
    waitpid(pinfo.pid, NULL, 0);
#endif
    pinfo.valid = false;
}

static std::unordered_map<std::string, fs::file_time_type> getDirectoryTimestamps(const std::string& watchPath) {
    std::unordered_map<std::string, fs::file_time_type> timestamps;
    try {
        if (fs::is_regular_file(watchPath)) {
            timestamps[watchPath] = fs::last_write_time(watchPath);
        } else if (fs::is_directory(watchPath)) {
            for (const auto& entry : fs::recursive_directory_iterator(watchPath)) {
                if (entry.is_regular_file()) {
                    std::string ext = entry.path().extension().string();
                    if (ext == ".vit" || ext == ".json") {
                        timestamps[entry.path().string()] = fs::last_write_time(entry.path());
                    }
                }
            }
        }
    } catch (...) {}
    return timestamps;
}

int DevServer::run(const std::string& targetInput, int argc, char* argv[]) {
    std::string targetFile = targetInput;
    if (targetFile.empty() || targetFile == "dev") {
        if (fs::exists("src/main.vit")) targetFile = "src/main.vit";
        else if (fs::exists("main.vit")) targetFile = "main.vit";
        else targetFile = "src/main.vit";
    }

    std::string watchDir = fs::is_directory("src") ? "src" : (fs::is_regular_file(targetFile) ? fs::path(targetFile).parent_path().string() : ".");
    if (watchDir.empty()) watchDir = ".";

    std::string exePath = argv[0];

    std::cout << "============================================================" << std::endl;
    std::cout << "  ⚡ VIT DevServer (Live-Reload & Hot Watcher)" << std::endl;
    std::cout << "============================================================" << std::endl;
    std::cout << "\033[32m✓\033[0m Entrypoint: \033[36m" << targetFile << "\033[0m" << std::endl;
    std::cout << "\033[32m✓\033[0m Watching directory: \033[36m" << watchDir << "\033[0m" << std::endl;
    std::cout << "\033[33mPress Ctrl+C to stop dev server\033[0m\n" << std::endl;

    ProcessInfo child = spawnProcess(exePath, targetFile);
    auto lastTimestamps = getDirectoryTimestamps(watchDir);

    while (true) {
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        auto currentTimestamps = getDirectoryTimestamps(watchDir);

        bool modified = false;
        std::string modifiedFile;

        for (const auto& [file, time] : currentTimestamps) {
            auto it = lastTimestamps.find(file);
            if (it == lastTimestamps.end() || it->second != time) {
                modified = true;
                modifiedFile = file;
                break;
            }
        }

        if (modified) {
            std::cout << "\n\033[33m[VIT Dev]\033[0m File change detected: \033[36m" << modifiedFile << "\033[0m. Restarting server...\n";
            killProcess(child);
            lastTimestamps = currentTimestamps;
            child = spawnProcess(exePath, targetFile);
        }
    }

    return 0;
}

} // namespace vit
