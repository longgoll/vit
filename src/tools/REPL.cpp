#include "tools/REPL.h"
#include "lexer/Lexer.h"
#include "parser/Parser.h"
#include "codegen/LLVMCodeGen.h"
#include "codegen/NativeCompiler.h"
#include "semantics/SemanticAnalyzer.h"
#include "ast/ASTPrinter.h"
#include "diagnostics/DiagnosticPrinter.h"

#include <iostream>
#include <sstream>
#include <fstream>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#ifdef _WIN32
#include <windows.h>
#define POPEN  _popen
#define PCLOSE _pclose
#else
#define POPEN  popen
#define PCLOSE pclose
#endif

namespace vit {

REPLEngine::REPLEngine() : m_showAST(false) {}

void REPLEngine::printHeader() {
    std::cout << "\033[36m╔═══════════════════════════════════════════════╗\033[0m\n";
    std::cout << "\033[36m║\033[0m  \033[1;32mVIT Interactive REPL Shell v2.0\033[0m            \033[36m║\033[0m\n";
    std::cout << "\033[36m║\033[0m  Type \033[33m.help\033[0m for commands, \033[33m.exit\033[0m to quit       \033[36m║\033[0m\n";
    std::cout << "\033[36m╚═══════════════════════════════════════════════╝\033[0m\n\n";
}

void REPLEngine::run() {
    printHeader();

    std::string line;
    while (true) {
        std::cout << "\033[1;32mvit\033[0m\033[1;33m›\033[0m ";
        std::cout.flush();
        if (!std::getline(std::cin, line)) {
            std::cout << "\n";
            break;
        }

        if (line.empty()) continue;

        if (line == ".exit" || line == ".quit" || line == "exit" || line == "quit") {
            std::cout << "\033[36mGoodbye! 👋\033[0m\n";
            break;
        } else if (line == ".help") {
            std::cout << "\033[1mREPL Commands:\033[0m\n";
            std::cout << "  \033[33m.exit\033[0m, \033[33m.quit\033[0m   Exit the REPL\n";
            std::cout << "  \033[33m.help\033[0m           Show this help message\n";
            std::cout << "  \033[33m.ast\033[0m            Toggle AST output mode\n";
            std::cout << "  \033[33m.clear\033[0m          Clear REPL history\n";
            std::cout << "  \033[33m.reset\033[0m          Reset all declared variables/functions\n";
            std::cout << "  \033[33m.history\033[0m        Show input history\n\n";
            std::cout << "\033[2mTips: End statements with ';'. Use 'print(expr);' to see results.\033[0m\n\n";
            continue;
        } else if (line == ".ast") {
            m_showAST = !m_showAST;
            std::cout << "AST mode: " << (m_showAST ? "\033[32mON\033[0m" : "\033[31mOFF\033[0m") << "\n";
            continue;
        } else if (line == ".clear") {
            m_history.clear();
            std::cout << "\033[32mHistory cleared.\033[0m\n";
            continue;
        } else if (line == ".reset") {
            m_history.clear();
            std::cout << "\033[32mREPL state reset.\033[0m\n";
            continue;
        } else if (line == ".history") {
            if (m_history.empty()) {
                std::cout << "\033[2m(no history)\033[0m\n";
            } else {
                for (size_t i = 0; i < m_history.size(); ++i) {
                    std::cout << "  \033[2m[" << (i + 1) << "]\033[0m " << m_history[i] << "\n";
                }
            }
            continue;
        }

        evalLine(line);
    }
}

void REPLEngine::evalLine(const std::string& line) {
    // Accumulate the new line into history
    m_history.push_back(line);

    // Build a complete program with all history statements inside main
    std::stringstream code;

    // Check if any line looks like a function declaration to lift out of main
    std::vector<std::string> topLevel;
    std::vector<std::string> bodyStmts;
    for (const auto& stmt : m_history) {
        std::string trimmed = stmt;
        // strip leading whitespace
        size_t start = trimmed.find_first_not_of(" \t");
        if (start != std::string::npos) trimmed = trimmed.substr(start);

        if (trimmed.rfind("function ", 0) == 0 || trimmed.rfind("fn ", 0) == 0
            || trimmed.rfind("struct ", 0) == 0 || trimmed.rfind("enum ", 0) == 0
            || trimmed.rfind("extern ", 0) == 0 || trimmed.rfind("type ", 0) == 0
            || trimmed.rfind("import ", 0) == 0) {
            topLevel.push_back(stmt);
        } else {
            bodyStmts.push_back(stmt);
        }
    }

    for (const auto& tl : topLevel) {
        code << tl << "\n";
    }
    code << "function main(): void {\n";
    for (const auto& s : bodyStmts) {
        code << "    " << s << "\n";
    }
    code << "}\n";

    std::string snippet = code.str();

    try {
        // Parse
        Lexer lexer(snippet);
        Parser parser(std::move(lexer));
        auto programAST = parser.parseProgram();

        if (m_showAST) {
            std::cout << "\033[34m── AST ──────────────────────────────────\033[0m\n";
            ASTPrinter printer(std::cout);
            programAST->accept(&printer);
            std::cout << "\033[34m────────────────────────────────────────\033[0m\n";
        }

        // Codegen → LLVM IR
        LLVMCodeGen codegen;
        std::string ir = codegen.generateIR(programAST.get());

        // Write temp IR file
        std::string tempLL  = "__vit_repl_tmp.ll";
        std::string tempExe = "__vit_repl_tmp.exe";
        { std::ofstream f(tempLL); f << ir; }

        // Compile to native
        NativeCompiler nc;
        NativeCompileOptions opts;
        opts.optLevel = "-O0";
        bool ok = nc.compileIRWithOptions(tempLL, tempExe, opts);
        std::remove(tempLL.c_str());

        if (!ok) {
            // Compilation failed — pop the bad line
            m_history.pop_back();
            std::cout << "\033[31m[Compile Error]\033[0m Could not compile.\n";
            return;
        }

        // Run and capture output
        std::string runCmd = "\"" + tempExe + "\" 2>&1";
        FILE* pipe = POPEN(runCmd.c_str(), "r");
        std::string output;
        if (pipe) {
            char buf[512];
            while (fgets(buf, sizeof(buf), pipe)) output += buf;
            PCLOSE(pipe);
        }
        std::remove(tempExe.c_str());

        // Print output
        if (!output.empty()) {
            // Remove trailing newline for cleaner display
            if (output.back() == '\n') output.pop_back();
            std::cout << "\033[2m" << output << "\033[0m\n";
        }

    } catch (const ParseError& e) {
        m_history.pop_back();
        std::cout << "\033[31m[Parse Error]\033[0m Line " << e.line
                  << ":" << e.column << " — " << e.what() << "\n";
    } catch (const std::exception& e) {
        m_history.pop_back();
        std::cout << "\033[31m[Error]\033[0m " << e.what() << "\n";
    }
}

} // namespace vit
