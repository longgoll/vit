#ifndef VIT_UTILS_PLATFORM_H
#define VIT_UTILS_PLATFORM_H

#include <string>
#include <vector>

namespace vit {
namespace utils {

class Platform {
public:
    // Get absolute directory path where vit executable resides
    static std::string getExeDir();

    // Normalize Windows slashes to backslashes or forward slashes
    static std::string normalizePath(const std::string& path, bool useBackslash = false);

    // Get candidate paths for local tools/compilers (e.g. LLVM / MinGW in LocalAppData)
    static std::vector<std::string> getLocalAppDataToolCandidates();
};

} // namespace utils
} // namespace vit

#endif // VIT_UTILS_PLATFORM_H
