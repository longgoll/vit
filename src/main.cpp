// main.cpp — VIT compiler entry point (CLI argument dispatch)
// resolveImports + setupPath → main_commands.cpp

#include "main_shared.h"

#include "ast/ASTPrinter.h"
#include "codegen/LLVMCodeGen.h"
#include "codegen/NativeCompiler.h"
#include "codegen/JITEngine.h"
#include "codegen/EscapeAnalysis.h"
#include "diagnostics/DiagnosticPrinter.h"
#include "lexer/Lexer.h"
#include "parser/Parser.h"
#include "semantics/SemanticAnalyzer.h"
#include "semantics/Monomorphizer.h"
#include "tools/LSP.h"
#include "tools/PackageManager.h"
#include "tools/REPL.h"
#include "tools/Formatter.h"
#include "tools/Linter.h"
#include "tools/DevServer.h"
#include "utils/Platform.h"

#include <algorithm>
#include <chrono>
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

const std::string VIT_VERSION = "2.0.0 (Phase 14 - Cross-Compilation, WASM & Optimizations)";

void printUsage(const char* progName) {
    std::cout << "VIT Language Compiler & Ecosystem v" << VIT_VERSION << "\n\n";
    std::cout << "Usage:\n";
    std::cout << "  vit <command> [file|project] [options]\n";
    std::cout << "  vit <file> [options]         (Shortcut for 'vit run <file>')\n\n";
    std::cout << "Commands:\n";
    std::cout << "  run <file>      Compile and execute a VIT source file immediately\n";
    std::cout << "  dev [file]      Start dev server with live-reload watching source changes\n";
    std::cout << "  build <file>    Compile a VIT source file into a native executable (.exe)\n";
    std::cout << "  test [path]     Run all *.test.vit test files in directory (default: tests/)\n";
    std::cout << "  init [name]     Initialize a new Vit project directory\n";
    std::cout << "  add <package>   Add a package dependency to vit.json\n";
    std::cout << "  install         Install project dependencies defined in vit.json\n";
    std::cout << "  repl            Start interactive REPL shell\n";
    std::cout << "  fmt [path]      Format VIT code file or directory\n";
    std::cout << "  lint [path]     Lint VIT code file for warnings and code smells\n";
    std::cout << "  lsp             Run Language Server Protocol (JSON-RPC) over stdin/stdout\n";
    std::cout << "  setup           Automatically add 'vit' directory to Windows User PATH\n";
    std::cout << "  version         Display compiler version info\n";
    std::cout << "  help            Show this help message\n\n";
    std::cout << "Options:\n";
    std::cout << "  -o <output.exe>        Specify output executable file path\n";
    std::cout << "  -O0, -O1, -O2, -O3     Native compiler optimization levels\n";
    std::cout << "  --target <triple>      Cross-compilation target triple (e.g., x86_64-unknown-linux-gnu, wasm32-wasi)\n";
    std::cout << "  --enable-escape-analysis Enable LLVM ARC Escape Analysis optimization pass\n";
    std::cout << "  --lto=thin|full        Link-Time Optimization (LTO) pass mode\n";
    std::cout << "  --pgo-gen=<prof.raw>   Profile-Guided Optimization generation phase\n";
    std::cout << "  --pgo-use=<prof.data>  Profile-Guided Optimization consumption phase\n";
    std::cout << "  -march=native          Tune code generation for host CPU vectorization\n";
    std::cout << "  --emit-ast             Print Abstract Syntax Tree (AST) to console\n";
    std::cout << "  --emit-llvm            Print LLVM IR intermediate representation to console\n";
    std::cout << "  -v, --verbose          Display compilation phase breakdown and performance timing\n";
    std::cout << "  -h, --help             Display this help message\n\n";
    std::cout << "Examples:\n";
    std::cout << "  vit build app.vit --target x86_64-unknown-linux-gnu -O3 -o app_linux\n";
    std::cout << "  vit build app.vit --lto=thin -march=native -O3 -o app_extreme\n";
    std::cout << "  vit build app.vit --target wasm32-wasi -o app.wasm\n";
    std::cout << "  vit build app.vit -O3 --enable-escape-analysis\n";
    std::cout << "  vit repl\n";
}

void printVersion() {
    std::cout << "VIT Compiler v" << VIT_VERSION << "\n";
}

// Declared in main_commands.cpp
void setupPath();

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

    Mode mode = Mode::RUN;
    std::string sourceFilePath;
    std::string outputExePath;
    std::string irFilePath = "output.ll";
    std::string optLevel = "-O0";
    bool emitAST = false;
    bool emitLLVM = false;
    bool customOutput = false;
    bool useJIT = true;

    std::string firstArg = argv[1];

    if (firstArg == "init") {
        std::string projName = (argc >= 3) ? argv[2] : "vit-app";
        return PackageManager::initProject(projName) ? 0 : 1;
    } else if (firstArg == "add") {
        if (argc < 3) {
            std::cerr << "\033[31m[VIT Error]\033[0m Usage: vit add <package-url-or-name>\n";
            return 1;
        }
        return PackageManager::addPackage(argv[2]) ? 0 : 1;
    } else if (firstArg == "install") {
        return PackageManager::installDependencies() ? 0 : 1;
    } else if (firstArg == "repl") {
        REPLEngine repl;
        repl.run();
        return 0;
    } else if (firstArg == "fmt") {
        std::string target = (argc >= 3) ? argv[2] : ".";
        return Formatter::formatFile(target) ? 0 : 1;
    } else if (firstArg == "lint") {
        std::string target = (argc >= 3) ? argv[2] : "src/main.vit";
        std::vector<LintWarning> warnings;
        Linter::lintFile(target, warnings);
        if (warnings.empty()) {
            std::cout << "\033[32m[VIT Lint]\033[0m " << target << " — No issues found. ✓\n";
        } else {
            std::cout << "\033[36m[VIT Lint]\033[0m Scanned " << target << " — Found "
                      << warnings.size() << " issue(s):\n";
            for (const auto& w : warnings) {
                std::cout << "  \033[33m" << w.filePath << ":" << w.line << ":" << w.column
                          << " [" << w.rule << "]\033[0m " << w.message << "\n";
            }
        }
        return 0;
    } else if (firstArg == "test") {
        std::string testDir = (argc >= 3) ? argv[2] : "tests";
        std::vector<std::string> testFiles;
        try {
            namespace fs = std::filesystem;
            if (fs::exists(testDir) && fs::is_directory(testDir)) {
                for (const auto& entry : fs::recursive_directory_iterator(testDir)) {
                    if (entry.is_regular_file()) {
                        std::string fname = entry.path().string();
                        if ((fname.size() >= 9 && fname.substr(fname.size() - 9) == ".test.vit") ||
                            (fname.size() >= 9 && fname.substr(fname.size() - 9) == "_test.vit")) {
                            testFiles.push_back(fname);
                        }
                    }
                }
            } else {
                testFiles.push_back(testDir);
            }
        } catch (...) {
            testFiles.push_back(testDir);
        }

        if (testFiles.empty()) {
            std::cout << "\033[33m[VIT Test]\033[0m No test files found in '" << testDir << "'.\n";
            std::cout << "  Create files named *.test.vit or *_test.vit in the tests/ directory.\n";
            return 0;
        }

        std::cout << "\033[36m╔══════════════════════════════════╗\033[0m\n";
        std::cout << "\033[36m║     VIT Test Runner v2.0         ║\033[0m\n";
        std::cout << "\033[36m╚══════════════════════════════════╝\033[0m\n";
        std::cout << "\033[2mRunning " << testFiles.size() << " test file(s)...\033[0m\n\n";

        int totalPass = 0, totalFail = 0;
        auto testStart = std::chrono::high_resolution_clock::now();

        for (const auto& testFile : testFiles) {
            std::cout << "\033[1m▶ " << testFile << "\033[0m\n";

            std::string tempExe = testFile + ".test_runner.exe";
            std::string tempLL  = testFile + ".ll";

            std::ifstream tf(testFile);
            if (!tf.is_open()) {
                std::cout << "  \033[31m✗ Could not open: " << testFile << "\033[0m\n";
                totalFail++;
                continue;
            }
            std::stringstream tbuf;
            tbuf << tf.rdbuf();
            tf.close();

            try {
                Lexer lexer(tbuf.str());
                Parser parser(std::move(lexer));
                auto ast = parser.parseProgram();

                std::unordered_set<std::string> visited;
                std::error_code ecCanon;
                visited.insert(std::filesystem::weakly_canonical(testFile, ecCanon).string());
                resolveImports(ast.get(), testFile, visited);

                LLVMCodeGen codegen;
                std::string ir = codegen.generateIR(ast.get());

                { std::ofstream f(tempLL); f << ir; }

                NativeCompiler nc;
                NativeCompileOptions opts;
                opts.optLevel = "-O0";
                bool ok = nc.compileIRWithOptions(tempLL, tempExe, opts);
                if (!ok) {
                    std::cout << "  \033[31m✗ Compile error\033[0m\n";
                    totalFail++;
                    std::remove(tempLL.c_str());
                    continue;
                }

                std::string absExe = std::filesystem::absolute(tempExe).string();
                std::string absTmp = std::filesystem::absolute(testFile + ".run.tmp").string();
#ifdef _WIN32
                std::string runCmd = "cmd.exe /S /C \"\"" + absExe + "\" > \"" + absTmp + "\" 2>&1\"";
#else
                std::string runCmd = "\"" + absExe + "\" > \"" + absTmp + "\" 2>&1";
#endif
                int sysRes = std::system(runCmd.c_str());

                std::ifstream outF(absTmp);
                std::string output((std::istreambuf_iterator<char>(outF)), std::istreambuf_iterator<char>());
                outF.close();
                std::remove(absTmp.c_str());

                std::istringstream outStream(output);
                std::string outLine;
                int filePasses = 0, fileFails = 0;
                while (std::getline(outStream, outLine)) {
                    size_t passPos = outLine.find("[PASS]");
                    size_t failPos = outLine.find("[FAIL]");
                    if (passPos != std::string::npos) {
                        std::cout << "  \033[32m✓\033[0m " << outLine.substr(passPos + 7) << "\n";
                        filePasses++;
                    } else if (failPos != std::string::npos) {
                        std::cout << "  \033[31m✗\033[0m " << outLine.substr(failPos + 7) << "\n";
                        fileFails++;
                    }
                }
                totalPass += filePasses;
                totalFail += fileFails;

                std::remove(tempLL.c_str());
                std::remove(tempExe.c_str());

            } catch (const std::exception& ex) {
                std::cout << "  \033[31m✗ Error: " << ex.what() << "\033[0m\n";
                totalFail++;
            }
            std::cout << "\n";
        }

        auto testEnd = std::chrono::high_resolution_clock::now();
        double ms = std::chrono::duration<double, std::milli>(testEnd - testStart).count();

        std::cout << "\033[36m─────────────────────────────────────\033[0m\n";
        if (totalFail == 0) {
            std::cout << "\033[1;32m✓ All " << totalPass << " tests passed\033[0m";
        } else {
            std::cout << "\033[1;32m" << totalPass << " passed\033[0m, "
                      << "\033[1;31m" << totalFail << " failed\033[0m";
        }
        std::cout << "  \033[2m(" << ms << " ms)\033[0m\n";
        return (totalFail > 0) ? 1 : 0;
    } else if (firstArg == "lsp") {
        LSPServer lsp;
        lsp.run();
        return 0;
    } else if (firstArg == "dev") {
        std::string target = (argc >= 3) ? argv[2] : "";
        return DevServer::run(target, argc, argv);
    }

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

    NativeCompileOptions compileOpts;

    for (int i = startIndex; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "-h" || arg == "--help") {
            printUsage(argv[0]);
            return 0;
        } else if (arg == "-o" && i + 1 < argc) {
            outputExePath = argv[++i];
            customOutput = true;
        } else if (arg == "--target" && i + 1 < argc) {
            compileOpts.targetTriple = argv[++i];
        } else if (arg == "--enable-escape-analysis") {
            compileOpts.enableEscapeAnalysis = true;
        } else if (arg == "--lto=thin") {
            compileOpts.ltoMode = "thin";
        } else if (arg == "--lto=full" || arg == "--lto") {
            compileOpts.ltoMode = "full";
        } else if (arg.rfind("--pgo-gen", 0) == 0 || arg.rfind("--pgo-generate", 0) == 0) {
            compileOpts.pgoMode = "generate";
            size_t eqPos = arg.find('=');
            if (eqPos != std::string::npos) compileOpts.pgoPath = arg.substr(eqPos + 1);
        } else if (arg.rfind("--pgo-use", 0) == 0) {
            compileOpts.pgoMode = "use";
            size_t eqPos = arg.find('=');
            if (eqPos != std::string::npos) compileOpts.pgoPath = arg.substr(eqPos + 1);
        } else if (arg == "-march=native" || arg == "--march-native") {
            compileOpts.marchNative = true;
        } else if (arg == "-O0" || arg == "-O1" || arg == "-O2" || arg == "-O3") {
            compileOpts.optLevel = arg;
            optLevel = arg;
        } else if (arg == "--emit-ast") {
            emitAST = true;
        } else if (arg == "--emit-llvm") {
            emitLLVM = true;
        } else if (arg == "--verbose" || arg == "-v") {
            compileOpts.verbose = true;
        } else if (arg == "--jit") {
            useJIT = true;
        } else if (arg == "--no-jit") {
            useJIT = false;
        } else if (arg == "--save-temps") {
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

    struct TempDirCleanup {
        std::filesystem::path path;
        bool active = false;
        ~TempDirCleanup() {
            if (active && !path.empty()) {
                std::error_code ec;
                std::filesystem::remove_all(path, ec);
            }
        }
    } tempCleanup;

    if (mode == Mode::RUN) {
        auto now = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
        tempCleanup.path = std::filesystem::temp_directory_path() / ("vit_run_" + std::to_string(now));
        std::error_code ec;
        std::filesystem::create_directories(tempCleanup.path, ec);
        tempCleanup.active = true;

        irFilePath = (tempCleanup.path / "output.ll").string();
        outputExePath = (tempCleanup.path / "app.exe").string();
    } else {
        std::filesystem::path srcP(sourceFilePath);
        std::string stem = srcP.stem().string();
        if (!customOutput) {
            std::error_code ec;
            std::filesystem::create_directories("build", ec);
            if (compileOpts.targetTriple.find("wasm32") != std::string::npos) {
                outputExePath = (std::filesystem::path("build") / (stem + ".wasm")).string();
            } else {
                outputExePath = (std::filesystem::path("build") / (stem + ".exe")).string();
            }
        }
        std::filesystem::path parentDir = std::filesystem::path(outputExePath).parent_path();
        if (!parentDir.empty()) {
            std::error_code ec;
            std::filesystem::create_directories(parentDir, ec);
        }
        irFilePath = (parentDir.empty() ? std::filesystem::path("output.ll") : (parentDir / "output.ll")).string();
    }

    std::ifstream file(sourceFilePath);
    if (!file.is_open()) {
        std::cerr << "\033[31m[VIT Error]\033[0m Could not open source file '" << sourceFilePath << "'\n";
        return 1;
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string sourceCode = buffer.str();

    if (mode == Mode::BUILD) {
        std::cout << "\033[36m[VIT]\033[0m Compiling " << sourceFilePath;
        if (!compileOpts.targetTriple.empty()) {
            std::cout << " (Target: " << compileOpts.targetTriple << ")";
        }
        if (!compileOpts.ltoMode.empty()) {
            std::cout << " [LTO: " << compileOpts.ltoMode << "]";
        }
        if (compileOpts.marchNative) {
            std::cout << " [Native CPU]";
        }
        std::cout << " ...\n";
    }

    try {
        auto totalStart = std::chrono::high_resolution_clock::now();

        // 1. Lexical & Syntax Analysis
        auto t0 = std::chrono::high_resolution_clock::now();
        Lexer lexer(sourceCode);
        Parser parser(std::move(lexer));
        auto programAST = parser.parseProgram();
        auto t1 = std::chrono::high_resolution_clock::now();
        if (compileOpts.verbose) {
            std::cout << "\033[36m[VIT Verbose]\033[0m Lexing & Parsing: "
                      << std::chrono::duration<double, std::milli>(t1 - t0).count() << " ms\n";
        }

        // Resolve imports
        t0 = std::chrono::high_resolution_clock::now();
        std::unordered_set<std::string> visitedFiles;
        std::error_code ecCanon;
        visitedFiles.insert(std::filesystem::weakly_canonical(sourceFilePath, ecCanon).string());
        resolveImports(programAST.get(), sourceFilePath, visitedFiles);
        t1 = std::chrono::high_resolution_clock::now();
        if (compileOpts.verbose) {
            std::cout << "\033[36m[VIT Verbose]\033[0m Import Resolution: "
                      << std::chrono::duration<double, std::milli>(t1 - t0).count() << " ms\n";
        }

        // 1.5 Monomorphization Pass
        t0 = std::chrono::high_resolution_clock::now();
        Monomorphizer monomorphizer;
        monomorphizer.process(programAST.get());
        t1 = std::chrono::high_resolution_clock::now();
        if (compileOpts.verbose) {
            std::cout << "\033[36m[VIT Verbose]\033[0m Monomorphization: "
                      << std::chrono::duration<double, std::milli>(t1 - t0).count() << " ms\n";
        }

        if (emitAST) {
            std::cout << "\n--- Abstract Syntax Tree (AST) ---\n";
            ASTPrinter printer(std::cout);
            programAST->accept(&printer);
            std::cout << "-----------------------------------\n";
        }

        // 2. Semantic Analysis
        t0 = std::chrono::high_resolution_clock::now();
        SemanticAnalyzer semanticAnalyzer;
        if (!semanticAnalyzer.analyze(programAST.get())) {
            std::cerr << "\n\033[31m[Semantic Error]\033[0m Found "
                      << semanticAnalyzer.getErrors().size() << " error(s):\n";
            for (const auto& err : semanticAnalyzer.getErrors()) {
                DiagnosticPrinter::printError("Semantic Error", err, sourceFilePath, sourceCode, 0, 0);
            }
            return 1;
        }
        t1 = std::chrono::high_resolution_clock::now();
        if (compileOpts.verbose) {
            std::cout << "\033[36m[VIT Verbose]\033[0m Semantic Analysis: "
                      << std::chrono::duration<double, std::milli>(t1 - t0).count() << " ms\n";
        }

        // 2.5 Escape Analysis (optional)
        if (compileOpts.enableEscapeAnalysis) {
            EscapeAnalyzer escapeAnalyzer;
            auto escapeResult = escapeAnalyzer.analyze(programAST.get());
            std::cout << escapeResult.report << "\n";
        }

        // 3. Code Generation (LLVM IR)
        t0 = std::chrono::high_resolution_clock::now();
        LLVMCodeGen codeGen;
        std::string llvmIR = codeGen.generateIR(programAST.get(), compileOpts.targetTriple);
        t1 = std::chrono::high_resolution_clock::now();
        if (compileOpts.verbose) {
            std::cout << "\033[36m[VIT Verbose]\033[0m LLVM IR CodeGen: "
                      << std::chrono::duration<double, std::milli>(t1 - t0).count() << " ms\n";
        }

        if (emitLLVM) {
            std::cout << "\n--- Generated LLVM IR Code ---\n";
            std::cout << llvmIR;
            std::cout << "-------------------------------\n";
        }

        // 4. JIT execution (vit run)
        if (mode == Mode::RUN) {
            if (compileOpts.targetTriple.find("wasm32") != std::string::npos ||
                compileOpts.targetTriple.find("linux") != std::string::npos ||
                compileOpts.targetTriple.find("darwin") != std::string::npos) {
                std::cout << "\033[33m[VIT Notice]\033[0m Binary built for cross-target '" << compileOpts.targetTriple << "'. Skipping direct execution.\n";
                return 0;
            }

            if (useJIT && !customOutput && compileOpts.targetTriple.empty()) {
                JITEngine jitEngine;
                return jitEngine.executeIR(llvmIR, sourceFilePath, compileOpts);
            }
        }

        // Save LLVM IR
        std::ofstream outFile(irFilePath);
        if (outFile.is_open()) {
            outFile << llvmIR;
            outFile.close();
        }

        // 5. Native Binary Compilation
        NativeCompiler nativeCompiler;
        bool compileSuccess = nativeCompiler.compileIRWithOptions(irFilePath, outputExePath, compileOpts);

        if (!compileSuccess) {
            return 1;
        }

        if (mode == Mode::BUILD) {
            if (!emitLLVM) {
                std::error_code ec;
                std::filesystem::remove(irFilePath, ec);
            }
            std::cout << "\033[32m✓\033[0m Built \033[1m" << outputExePath << "\033[0m successfully (" << optLevel;
            if (!compileOpts.targetTriple.empty()) {
                std::cout << ", target: " << compileOpts.targetTriple;
            }
            std::cout << ").\n";
        }

        if (mode == Mode::RUN) {
            nativeCompiler.runExecutable(outputExePath);
            return 0;
        }

    } catch (const ParseError& e) {
        DiagnosticPrinter::printError("Parse Error", e.what(), sourceFilePath, sourceCode, e.line, e.column);
        return 1;
    } catch (const std::exception& e) {
        DiagnosticPrinter::printError("Error", e.what(), sourceFilePath, sourceCode, 0, 0);
        return 1;
    }

    return 0;
}
