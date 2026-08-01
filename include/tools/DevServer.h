#pragma once

#include <string>

namespace vit {

class DevServer {
public:
    static int run(const std::string& targetPath, int argc, char* argv[]);
};

} // namespace vit
