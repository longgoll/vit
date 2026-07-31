#ifndef VIT_TOOLS_LINTER_H
#define VIT_TOOLS_LINTER_H

#include <string>
#include <vector>

namespace vit {

struct LintWarning {
    std::string filePath;
    int line;
    int column;
    std::string rule;
    std::string message;
};

class Linter {
public:
    static bool lintFile(const std::string& filePath, std::vector<LintWarning>& warnings);
    static bool lintCode(const std::string& code, const std::string& filePath, std::vector<LintWarning>& warnings);
};

} // namespace vit

#endif // VIT_TOOLS_LINTER_H
