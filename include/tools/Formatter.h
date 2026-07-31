#ifndef VIT_TOOLS_FORMATTER_H
#define VIT_TOOLS_FORMATTER_H

#include <string>

namespace vit {

class Formatter {
public:
    static bool formatFile(const std::string& filePath);
    static std::string formatCode(const std::string& code);
};

} // namespace vit

#endif // VIT_TOOLS_FORMATTER_H
