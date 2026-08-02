#ifndef VIT_JIT_ENGINE_H
#define VIT_JIT_ENGINE_H

#include "codegen/NativeCompiler.h"
#include <string>
#include <vector>

namespace vit {

class JITEngine {
private:
    std::string detectLLIRunner();
    std::string getOrBuildRuntimeArchive(NativeCompiler& compiler);

public:
    JITEngine();

    bool isJITAvailable();
    int executeIR(const std::string& llvmIR, const std::string& sourceFilePath, const NativeCompileOptions& options = NativeCompileOptions());
};

} // namespace vit

#endif // VIT_JIT_ENGINE_H
