#ifndef VIT_NATIVE_COMPILER_H
#define VIT_NATIVE_COMPILER_H

#include <string>

namespace vit {

struct NativeCompileOptions {
    std::string optLevel = "-O0";
    std::string targetTriple = "";
    std::string ltoMode = ""; // "", "thin", "full"
    std::string pgoMode = ""; // "", "generate", "use"
    std::string pgoPath = "";
    bool marchNative = false;
    bool enableEscapeAnalysis = false;
    bool verbose = false;
};

class NativeCompiler {
private:
    std::string clangExecutablePath;

    std::string detectClang();

public:
    NativeCompiler();

    bool isClangAvailable() const;
    const std::string& getClangPath() const;

    bool compileIRToExecutable(const std::string& irFilePath, const std::string& outputExePath, const std::string& optLevel = "-O0", const std::string& targetTriple = "");
    bool compileIRWithOptions(const std::string& irFilePath, const std::string& outputExePath, const NativeCompileOptions& options);
    int runExecutable(const std::string& exePath);
};

} // namespace vit

#endif // VIT_NATIVE_COMPILER_H
