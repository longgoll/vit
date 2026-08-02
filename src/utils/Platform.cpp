#include "utils/Platform.h"
#include <cstdlib>
#include <filesystem>
#include <fstream>

#ifdef _WIN32
#include <windows.h>
#endif

namespace vit {
namespace utils {

std::string Platform::getExeDir() {
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

std::string Platform::normalizePath(const std::string& path, bool useBackslash) {
    std::string result = path;
    for (char &c : result) {
        if (useBackslash) {
            if (c == '/') c = '\\';
        } else {
            if (c == '\\') c = '/';
        }
    }
    return result;
}

std::vector<std::string> Platform::getLocalAppDataToolCandidates() {
    std::vector<std::string> candidates;
    const char* localAppData = std::getenv("LOCALAPPDATA");
    if (localAppData && localAppData[0] != '\0') {
        std::string base = std::string(localAppData) + "\\Microsoft\\WinGet\\Packages";
        candidates.push_back(base + "\\BrechtSanders.WinLibs.POSIX.UCRT_Microsoft.Winget.Source_8wekyb3d8bbwe\\mingw64\\bin\\gcc.exe");
        candidates.push_back(base + "\\BrechtSanders.WinLibs.POSIX.UCRT_Microsoft.Winget.Source_8wekyb3d8bbwe\\mingw64\\bin\\clang.exe");
    }
    const char* userProfile = std::getenv("USERPROFILE");
    if (userProfile && userProfile[0] != '\0') {
        std::string base = std::string(userProfile) + "\\AppData\\Local\\Microsoft\\WinGet\\Packages";
        candidates.push_back(base + "\\BrechtSanders.WinLibs.POSIX.UCRT_Microsoft.Winget.Source_8wekyb3d8bbwe\\mingw64\\bin\\gcc.exe");
        candidates.push_back(base + "\\BrechtSanders.WinLibs.POSIX.UCRT_Microsoft.Winget.Source_8wekyb3d8bbwe\\mingw64\\bin\\clang.exe");
    }
    return candidates;
}

} // namespace utils
} // namespace vit
