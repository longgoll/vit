#ifndef VIT_DIAGNOSTIC_PRINTER_H
#define VIT_DIAGNOSTIC_PRINTER_H

#include <string>

namespace vit {

class DiagnosticPrinter {
public:
    static void printError(
        const std::string& errorType,
        const std::string& message,
        const std::string& filePath,
        const std::string& sourceCode,
        size_t line,
        size_t column,
        const std::string& hint = ""
    );
};

} // namespace vit

#endif // VIT_DIAGNOSTIC_PRINTER_H
