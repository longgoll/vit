#include "diagnostics/DiagnosticPrinter.h"
#include <iostream>
#include <sstream>
#include <vector>

namespace vit {

void DiagnosticPrinter::printError(
    const std::string& errorType,
    const std::string& message,
    const std::string& filePath,
    const std::string& sourceCode,
    size_t line,
    size_t column,
    const std::string& hint
) {
    // ANSI Color codes
    const std::string RED = "\033[1;31m";
    const std::string CYAN = "\033[1;36m";
    const std::string GREEN = "\033[1;32m";
    const std::string BOLD = "\033[1m";
    const std::string RESET = "\033[0m";

    std::cerr << "\n" << RED << "[" << errorType << "]" << RESET << " " << BOLD << message << RESET << "\n";
    std::cerr << CYAN << "  --> " << RESET << filePath;
    if (line > 0) {
        std::cerr << ":" << line;
        if (column > 0) {
            std::cerr << ":" << column;
        }
    }
    std::cerr << "\n";

    if (!sourceCode.empty() && line > 0) {
        // Split source code into lines
        std::stringstream ss(sourceCode);
        std::string lineContent;
        size_t currentLineNum = 1;
        std::string targetLine = "";

        while (std::getline(ss, lineContent)) {
            if (currentLineNum == line) {
                targetLine = lineContent;
                break;
            }
            currentLineNum++;
        }

        if (!targetLine.empty()) {
            std::string lineStr = std::to_string(line);
            std::string pad(lineStr.length(), ' ');

            std::cerr << CYAN << "   " << pad << " |\n";
            std::cerr << CYAN << " " << lineStr << " | " << RESET << targetLine << "\n";
            std::cerr << CYAN << "   " << pad << " | " << RED;

            size_t colIndex = (column > 0) ? column - 1 : 0;
            for (size_t i = 0; i < colIndex && i < targetLine.length(); ++i) {
                if (targetLine[i] == '\t') std::cerr << "\t";
                else std::cerr << " ";
            }
            std::cerr << "^^^" << RESET << "\n";
            std::cerr << CYAN << "   " << pad << " |\n" << RESET;
        }
    }

    if (!hint.empty()) {
        std::cerr << GREEN << "  = hint: " << RESET << hint << "\n";
    }
    std::cerr << "\n";
}

} // namespace vit
