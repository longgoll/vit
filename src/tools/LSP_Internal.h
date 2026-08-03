#pragma once
// Internal shared types and helpers for LSP implementation.
// Included by LSP.cpp and LSP_Handlers.cpp.

#include "lexer/Token.h"
#include <string>
#include <vector>
#include <sstream>

namespace vit {

struct LSPSymbolInfo {
    std::string name;
    int kind; // 3=Function, 5=Field, 6=Variable, 7=Class, 13=Enum, 14=Keyword, 22=Struct
    std::string detail;
    std::string typeName;
    std::string documentation;
    size_t line = 0; // 0-indexed
    size_t col  = 0; // 0-indexed
    std::vector<std::pair<std::string, std::string>> fields; // name, type
    std::vector<std::string> variants;
};

// Defined in LSP.cpp
std::string escapeJson(const std::string& str);
std::vector<LSPSymbolInfo> extractDocumentSymbols(const std::string& content);

} // namespace vit
