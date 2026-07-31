#ifndef VIT_TOOLS_PACKAGE_MANAGER_H
#define VIT_TOOLS_PACKAGE_MANAGER_H

#include <string>
#include <vector>

namespace vit {

class PackageManager {
public:
    static bool initProject(const std::string& projectName);
    static bool addPackage(const std::string& packageDescriptor);
    static bool installDependencies();

private:
    static bool ensureDirectoryExists(const std::string& path);
};

} // namespace vit

#endif // VIT_TOOLS_PACKAGE_MANAGER_H
