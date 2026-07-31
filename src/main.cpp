#include "ast/ASTPrinter.h"
#include "codegen/LLVMCodeGen.h"
#include "codegen/NativeCompiler.h"
#include "lexer/Lexer.h"
#include "parser/Parser.h"
#include "semantics/SemanticAnalyzer.h"
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#ifdef _WIN32
#include <windows.h>
#endif

using namespace vit;

const std::string VIT_VERSION = "0.3.0 (Phase 3)";

void printUsage(const char* progName) {
    std::cout << "VIT Language Compiler v" << VIT_VERSION << "\n\n";
    std::cout << "Usage:\n";
    std::cout << "  vit <command> [file] [options]\n";
    std::cout << "  vit <file> [options]         (Shortcut for 'vit run <file>')\n\n";
    std::cout << "Commands:\n";
    std::cout << "  run <file>      Compile and execute a VIT source file immediately\n";
    std::cout << "  build <file>    Compile a VIT source file into a native executable (.exe)\n";
    std::cout << "  setup           Automatically add 'vit' directory to Windows User PATH\n";
    std::cout << "  version         Display compiler version info\n";
    std::cout << "  help            Show this help message\n\n";
    std::cout << "Options:\n";
    std::cout << "  -o <output.exe> Specify output executable file path\n";
    std::cout << "  --emit-ast      Print Abstract Syntax Tree (AST) to console\n";
    std::cout << "  --emit-llvm     Print LLVM IR intermediate representation to console\n";
    std::cout << "  -h, --help      Display this help message\n\n";
    std::cout << "Examples:\n";
    std::cout << "  vit run main.vit\n";
    std::cout << "  vit build main.vit -o output.exe\n";
    std::cout << "  vit setup\n";
    std::cout << "  vit main.vit\n";
}

void printVersion() {
    std::cout << "VIT Compiler v" << VIT_VERSION << "\n";
}

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

            // 1. PATH setup
            std::cout << "[1/2] Setting up User PATH environment variable...\n";
            std::string psCmd = "powershell -ExecutionPolicy Bypass -Command \"$p=[Environment]::GetEnvironmentVariable('PATH','User'); if ($p -split ';' -notcontains '" + exeDir + "') { [Environment]::SetEnvironmentVariable('PATH', $p + ';' + '" + exeDir + "', 'User'); Write-Host '[VIT Setup] Successfully added " + exeDir + " to User PATH!' -ForegroundColor Green } else { Write-Host '[VIT Setup] " + exeDir + " is already in User PATH.' -ForegroundColor Yellow }\"";
            std::system(psCmd.c_str());

            // 2. Native Toolchain setup
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
                    std::ifstream f(scriptPath);
                    if (f.good()) {
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

enum class Mode {
    RUN,
    BUILD,
    HELP,
    VERSION,
    SETUP
};

#ifdef _WIN32
#ifndef ENABLE_VIRTUAL_TERMINAL_PROCESSING
#define ENABLE_VIRTUAL_TERMINAL_PROCESSING 0x0004
#endif
static void enableANSI() {
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hOut != INVALID_HANDLE_VALUE) {
        DWORD dwMode = 0;
        if (GetConsoleMode(hOut, &dwMode)) {
            dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
            SetConsoleMode(hOut, dwMode);
        }
    }
}
#endif

int main(int argc, char* argv[]) {
#ifdef _WIN32
    enableANSI();
#endif

    if (argc < 2) {
        printUsage(argv[0]);
        return 0;
    }

    Mode mode = Mode::RUN; // Default mode if file passed directly
    std::string sourceFilePath;
    std::string outputExePath;
    std::string irFilePath = "output.ll";
    bool emitAST = false;
    bool emitLLVM = false;
    bool customOutput = false;

    std::string firstArg = argv[1];

    int startIndex = 1;
    if (firstArg == "run") {
        mode = Mode::RUN;
        startIndex = 2;
    } else if (firstArg == "build") {
        mode = Mode::BUILD;
        startIndex = 2;
    } else if (firstArg == "setup") {
        setupPath();
        return 0;
    } else if (firstArg == "version" || firstArg == "-v" || firstArg == "--version") {
        printVersion();
        return 0;
    } else if (firstArg == "help" || firstArg == "-h" || firstArg == "--help") {
        printUsage(argv[0]);
        return 0;
    }

    // Parse remaining arguments
    for (int i = startIndex; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "-h" || arg == "--help") {
            printUsage(argv[0]);
            return 0;
        } else if (arg == "-o" && i + 1 < argc) {
            outputExePath = argv[++i];
            customOutput = true;
        } else if (arg == "--emit-ast") {
            emitAST = true;
        } else if (arg == "--emit-llvm") {
            emitLLVM = true;
        } else if (arg[0] != '-') {
            if (sourceFilePath.empty()) {
                sourceFilePath = arg;
            }
        }
    }

    if (sourceFilePath.empty()) {
        std::cerr << "\033[31m[VIT Error]\033[0m No input source file specified.\n\n";
        printUsage(argv[0]);
        return 1;
    }

    // Determine default output binary name if not explicitly set
    if (!customOutput) {
        std::string baseName = sourceFilePath;
        size_t lastDot = baseName.find_last_of(".");
        if (lastDot != std::string::npos) {
            baseName = baseName.substr(0, lastDot);
        }
        outputExePath = baseName + ".exe";
    }

    // Read source code from file
    std::ifstream file(sourceFilePath);
    if (!file.is_open()) {
        std::cerr << "\033[31m[VIT Error]\033[0m Could not open source file '" << sourceFilePath << "'\n";
        return 1;
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string sourceCode = buffer.str();

    if (mode == Mode::BUILD) {
        std::cout << "\033[36m[VIT]\033[0m Compiling " << sourceFilePath << " ...\n";
    }

    try {
        // 1. Lexical & Syntax Analysis
        Lexer lexer(sourceCode);
        Parser parser(std::move(lexer));
        auto programAST = parser.parseProgram();

        if (emitAST) {
            std::cout << "\n--- Abstract Syntax Tree (AST) ---\n";
            ASTPrinter printer(std::cout);
            programAST->accept(&printer);
            std::cout << "-----------------------------------\n";
        }

        // 2. Semantic Analysis (Type & Scope Check)
        SemanticAnalyzer semanticAnalyzer;
        if (!semanticAnalyzer.analyze(programAST.get())) {
            std::cerr << "\n\033[31m[Semantic Error]\033[0m Found "
                      << semanticAnalyzer.getErrors().size() << " error(s):\n";
            for (const auto& err : semanticAnalyzer.getErrors()) {
                std::cerr << "  - " << err << "\n";
            }
            return 1;
        }

        // 3. Code Generation (LLVM IR)
        LLVMCodeGen codeGen;
        std::string llvmIR = codeGen.generateIR(programAST.get());

        if (emitLLVM) {
            std::cout << "\n--- Generated LLVM IR Code ---\n";
            std::cout << llvmIR;
            std::cout << "-------------------------------\n";
        }

        // Save LLVM IR file
        std::ofstream outFile(irFilePath);
        if (outFile.is_open()) {
            outFile << llvmIR;
            outFile.close();
        }

        // 3. Native Binary Compilation
        NativeCompiler nativeCompiler;
        bool compileSuccess = nativeCompiler.compileIRToExecutable(irFilePath, outputExePath);

        if (!compileSuccess) {
            return 1;
        }

        if (mode == Mode::BUILD) {
            std::cout << "\033[32m✓\033[0m Built \033[1m" << outputExePath << "\033[0m successfully.\n";
        }

        // 4. If mode is RUN (or `vit run`), execute binary immediately
        if (mode == Mode::RUN) {
            nativeCompiler.runExecutable(outputExePath);
            return 0;
        }

    } catch (const ParseError& e) {
        std::cerr << "\n\033[31m[Parse Error]\033[0m " << e.what() << "\n";
        return 1;
    } catch (const std::exception& e) {
        std::cerr << "\n\033[31m[Error]\033[0m " << e.what() << "\n";
        return 1;
    }

    return 0;
}
