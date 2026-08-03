#pragma once
// Shared declarations for main.cpp and main_commands.cpp

#include "ast/AST.h"
#include <string>
#include <unordered_set>

// Import resolver — defined in main_commands.cpp, used in main.cpp
void resolveImports(vit::ProgramASTNode* program, const std::string& currentFilePath,
                    std::unordered_set<std::string>& visitedFiles, size_t depth = 0);

enum class Mode {
    RUN,
    BUILD,
    HELP,
    VERSION,
    SETUP
};
