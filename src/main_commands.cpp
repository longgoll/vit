// main_commands.cpp — resolveImports + setupPath implementations
// Everything here is used by main() but kept separate for readability.

#include "main_shared.h"
#include "ast/AST.h"
#include "codegen/NativeCompiler.h"
#include "lexer/Lexer.h"
#include "parser/Parser.h"
#include "utils/Platform.h"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <unordered_set>
#include <vector>

#ifdef _WIN32
#include <windows.h>
#endif

using namespace vit;

// ─── Import resolution ────────────────────────────────────────────────────────

void resolveImports(ProgramASTNode* program, const std::string& currentFilePath,
                    std::unordered_set<std::string>& visitedFiles, size_t depth) {
    if (depth > 100) {
        throw ParseError("Exceeded maximum module import depth limit (100)", 1, 1);
    }
    std::string compilerDir = utils::Platform::getExeDir();
    std::vector<std::unique_ptr<StatementNode>> remainingStmts;
    for (auto& stmt : program->getTopLevelStatements()) {
        if (stmt->getType() == NodeType::ImportDecl) {
            auto importNode = static_cast<ImportASTNode*>(stmt.get());
            std::string modPath = importNode->getModulePath();

            std::string currentDir = ".";
            size_t lastSep = currentFilePath.find_last_of("/\\");
            if (lastSep != std::string::npos) {
                currentDir = currentFilePath.substr(0, lastSep);
            }

            std::string cleanModPath = modPath;
            if (cleanModPath.rfind("std/", 0) == 0) {
                cleanModPath = cleanModPath.substr(4);
            } else if (cleanModPath.rfind("std\\", 0) == 0) {
                cleanModPath = cleanModPath.substr(4);
            }

            std::vector<std::string> candidates = {
                modPath,
                modPath + ".vit",
                currentDir + "/" + modPath,
                currentDir + "/" + modPath + ".vit",
                "./" + modPath,
                "./" + modPath + ".vit",
                "../" + modPath,
                "../" + modPath + ".vit",
                "../../" + modPath,
                "../../" + modPath + ".vit",
                "std/" + cleanModPath + ".vit",
                "../std/" + cleanModPath + ".vit",
                "../../std/" + cleanModPath + ".vit",
                compilerDir + "/std/" + cleanModPath + ".vit",
                compilerDir + "/../std/" + cleanModPath + ".vit",
                compilerDir + "/../../std/" + cleanModPath + ".vit"
            };

            std::string resolvedPath = "";
            for (const auto& cand : candidates) {
                std::error_code ecCheck;
                if (std::filesystem::exists(cand, ecCheck)) {
                    std::error_code ec;
                    auto canon = std::filesystem::weakly_canonical(cand, ec);
                    resolvedPath = ec ? cand : canon.string();
                    break;
                }
            }

            if (resolvedPath.empty()) {
                throw ParseError("Could not resolve imported module '" + modPath + "'", 1, 1);
            }

            if (visitedFiles.count(resolvedPath)) {
                continue; // Skip duplicate import (module cache)
            }
            visitedFiles.insert(resolvedPath);

            std::ifstream modFile(resolvedPath);
            std::stringstream modBuf;
            modBuf << modFile.rdbuf();

            Lexer modLexer(modBuf.str());
            Parser modParser(std::move(modLexer));
            auto modAST = modParser.parseProgram();

            resolveImports(modAST.get(), resolvedPath, visitedFiles, depth + 1);

            for (auto& func : modAST->getFunctions()) {
                program->getFunctions().push_back(std::move(func));
            }
            for (auto& topStmt : modAST->getTopLevelStatements()) {
                remainingStmts.push_back(std::move(topStmt));
            }
        } else {
            remainingStmts.push_back(std::move(stmt));
        }
    }
    program->getTopLevelStatements() = std::move(remainingStmts);
}

// ─── setupPath ────────────────────────────────────────────────────────────────

void setupPath() {
    std::cout << "===========================================\n";
    std::cout << "          VIT Compiler Setup               \n";
    std::cout << "===========================================\n\n";

#ifdef _WIN32
    char buffer[MAX_PATH];
    DWORD len = GetModuleFileNameA(NULL, buffer, MAX_PATH);
    if (len > 0) {
        std::string path(buffer, len);
        size_t lastSlash = path.find_last_of("\\/");
        if (lastSlash != std::string::npos) {
            std::string exeDir = path.substr(0, lastSlash);
            std::string escExeDir = exeDir;
            for (size_t pos = 0; (pos = escExeDir.find("'", pos)) != std::string::npos; pos += 2) {
                escExeDir.replace(pos, 1, "''");
            }

            std::cout << "[1/2] Setting up User PATH environment variable...\n";
            std::string psCmd = "powershell -ExecutionPolicy Bypass -Command \"$p=[Environment]::GetEnvironmentVariable('PATH','User'); if ($p -split ';' -notcontains '" + escExeDir + "') { [Environment]::SetEnvironmentVariable('PATH', $p + ';' + '" + escExeDir + "', 'User'); Write-Host '[VIT Setup] Successfully added " + escExeDir + " to User PATH!' -ForegroundColor Green } else { Write-Host '[VIT Setup] " + escExeDir + " is already in User PATH.' -ForegroundColor Yellow }\"";
            std::system(psCmd.c_str());

            std::cout << "\n[2/2] Checking Native Compiler Toolchain (Clang)...\n";
            NativeCompiler compiler;
            if (compiler.isClangAvailable()) {
                std::cout << "[VIT Setup] Native Clang compiler is ready at: " << compiler.getClangPath() << "\n";
            } else {
                std::cout << "[VIT Setup] Clang compiler not found. Running bundled toolchain setup script...\n\n";
                std::vector<std::string> scriptCandidates = {
                    exeDir + "\\scripts\\bundle_tools.ps1",
                    exeDir + "\\..\\scripts\\bundle_tools.ps1",
                    exeDir + "\\..\\..\\scripts\\bundle_tools.ps1"
                };

                std::string foundScript = "";
                for (const auto& scriptPath : scriptCandidates) {
                    if (std::filesystem::exists(scriptPath)) {
                        foundScript = scriptPath;
                        break;
                    }
                }

                if (!foundScript.empty()) {
                    std::string bundleCmd = "powershell -ExecutionPolicy Bypass -File \"" + foundScript + "\"";
                    std::system(bundleCmd.c_str());
                } else {
                    std::cout << "[VIT Setup] Could not find bundle_tools.ps1 script. Installing LLVM via winget...\n";
                    std::string wingetCmd = "powershell -ExecutionPolicy Bypass -Command \"winget install --id LLVM.LLVM --accept-source-agreements --accept-package-agreements\"";
                    std::system(wingetCmd.c_str());
                }
            }

            std::cout << "\n[VIT Setup] Setup complete!\n";
            return;
        }
    }
#endif
    std::cout << "[VIT Setup] Automatic setup is supported on Windows.\n";
}
