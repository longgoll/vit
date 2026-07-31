#include "tools/REPL.h"
#include "lexer/Lexer.h"
#include "parser/Parser.h"
#include "semantics/SemanticAnalyzer.h"
#include "ast/ASTPrinter.h"
#include "diagnostics/DiagnosticPrinter.h"

#include <iostream>
#include <sstream>

namespace vit {

REPLEngine::REPLEngine() : m_showAST(false) {}

void REPLEngine::printHeader() {
    std::cout << "\033[36m=====================================================\033[0m\n";
    std::cout << "\033[1;32mVIT Interactive REPL Shell v1.3.0\033[0m\n";
    std::cout << "Type '.help' for REPL commands or '.exit' to quit.\n";
    std::cout << "\033[36m=====================================================\033[0m\n\n";
}

void REPLEngine::run() {
    printHeader();

    std::string line;
    while (true) {
        std::cout << "\033[1;33mvit>\033[0m ";
        if (!std::getline(std::cin, line)) {
            break;
        }

        if (line.empty()) continue;

        if (line == ".exit" || line == ".quit") {
            std::cout << "Goodbye!\n";
            break;
        } else if (line == ".help") {
            std::cout << "REPL Commands:\n";
            std::cout << "  .exit, .quit  Exit the REPL\n";
            std::cout << "  .help         Show this help message\n";
            std::cout << "  .ast          Toggle displaying AST after parsing\n";
            std::cout << "  .vars         Show evaluated statement history\n";
            std::cout << "  .clear        Clear REPL history\n\n";
            continue;
        } else if (line == ".ast") {
            m_showAST = !m_showAST;
            std::cout << "AST output mode: " << (m_showAST ? "\033[32mON\033[0m" : "\033[31mOFF\033[0m") << "\n";
            continue;
        } else if (line == ".vars") {
            std::cout << "REPL Session History (" << m_history.size() << " statements):\n";
            for (size_t i = 0; i < m_history.size(); ++i) {
                std::cout << " [" << (i + 1) << "] " << m_history[i] << "\n";
            }
            continue;
        } else if (line == ".clear") {
            m_history.clear();
            std::cout << "Session history cleared.\n";
            continue;
        }

        evalLine(line);
    }
}

void REPLEngine::evalLine(const std::string& line) {
    m_history.push_back(line);

    // Build code snippet
    std::stringstream code;
    code << "function main(): void {\n";
    for (const auto& pastStmt : m_history) {
        code << "    " << pastStmt << "\n";
    }
    code << "}\n";

    std::string snippet = code.str();

    try {
        Lexer lexer(snippet);
        Parser parser(std::move(lexer));
        auto programAST = parser.parseProgram();

        if (m_showAST) {
            std::cout << "\033[34m--- AST ---\033[0m\n";
            ASTPrinter printer(std::cout);
            programAST->accept(&printer);
            std::cout << "\033[34m-----------\033[0m\n";
        }

        SemanticAnalyzer analyzer;
        if (!analyzer.analyze(programAST.get())) {
            std::cout << "\033[31m[Semantic Error]\033[0m Errors in line:\n";
            for (const auto& err : analyzer.getErrors()) {
                std::cout << "  - " << err << "\n";
            }
            m_history.pop_back();
            return;
        }

        std::cout << "\033[32m=> OK\033[0m\n";

    } catch (const ParseError& e) {
        std::cout << "\033[31m[Parse Error]\033[0m " << e.what() << "\n";
        m_history.pop_back();
    } catch (const std::exception& e) {
        std::cout << "\033[31m[Error]\033[0m " << e.what() << "\n";
        m_history.pop_back();
    }
}

} // namespace vit
