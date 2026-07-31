#ifndef VIT_NATIVE_COMPILER_H
#define VIT_NATIVE_COMPILER_H

#include <string>

namespace vit {

class NativeCompiler {
private:
    std::string clangExecutablePath;

    std::string detectClang();

public:
    NativeCompiler();

    bool isClangAvailable() const;
    const std::string& getClangPath() const;

    bool compileIRToExecutable(const std::string& irFilePath, const std::string& outputExePath, const std::string& optLevel = "-O0");
    int runExecutable(const std::string& exePath);
};

} // namespace vit

#endif // VIT_NATIVE_COMPILER_H
