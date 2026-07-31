#include "tools/Formatter.h"
#include "lexer/Lexer.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>

namespace vit {

std::string Formatter::formatCode(const std::string& code) {
    Lexer lexer(code);
    std::vector<Token> tokens;
    while (true) {
        Token tok = lexer.nextToken();
        tokens.push_back(tok);
        if (tok.type == TokenType::TokEof) break;
    }

    std::stringstream result;
    int indentLevel = 0;
    bool newLine = true;

    auto printIndent = [&](int level) {
        for (int i = 0; i < level * 4; ++i) {
            result << ' ';
        }
    };

    for (size_t i = 0; i < tokens.size(); ++i) {
        const auto& tok = tokens[i];
        if (tok.type == TokenType::TokEof) break;

        if (tok.type == TokenType::RBrace) {
            if (indentLevel > 0) indentLevel--;
            if (!newLine) result << "\n";
            printIndent(indentLevel);
            result << "}";
            result << "\n";
            newLine = true;
            continue;
        }

        if (newLine) {
            printIndent(indentLevel);
            newLine = false;
        }

        if (tok.type == TokenType::LBrace) {
            result << " {\n";
            indentLevel++;
            newLine = true;
            continue;
        }

        if (tok.type == TokenType::Semicolon) {
            result << ";\n";
            newLine = true;
            continue;
        }

        // Spacing rules for operators
        if (tok.type == TokenType::Equal || tok.type == TokenType::Plus ||
            tok.type == TokenType::Minus || tok.type == TokenType::Star ||
            tok.type == TokenType::Slash || tok.type == TokenType::EqualEqual ||
            tok.type == TokenType::NotEqual || tok.type == TokenType::Less ||
            tok.type == TokenType::Greater || tok.type == TokenType::LessEqual ||
            tok.type == TokenType::GreaterEqual) {
            result << " " << tok.lexeme << " ";
        } else if (tok.type == TokenType::Comma) {
            result << ", ";
        } else if (tok.type == TokenType::Colon) {
            result << ": ";
        } else {
            // Add space between tokens if needed
            if (i > 0) {
                const auto& prev = tokens[i - 1];
                if ((prev.type == TokenType::Identifier || (int)prev.type <= (int)TokenType::KwAwait) &&
                    (tok.type == TokenType::Identifier || (int)tok.type <= (int)TokenType::KwAwait || tok.type == TokenType::NumberLiteral || tok.type == TokenType::StringLiteral)) {
                    result << " ";
                }
            }
            result << tok.lexeme;
        }
    }

    return result.str();
}

bool Formatter::formatFile(const std::string& filePath) {
    std::ifstream file(filePath);
    if (!file.is_open()) {
        std::cerr << "\033[31m[VIT Error]\033[0m Could not open file to format: '" << filePath << "'\n";
        return false;
    }

    std::stringstream ss;
    ss << file.rdbuf();
    file.close();

    std::string original = ss.str();
    std::string formatted = formatCode(original);

    std::ofstream outFile(filePath);
    if (!outFile.is_open()) {
        std::cerr << "\033[31m[VIT Error]\033[0m Could not write formatted code to: '" << filePath << "'\n";
        return false;
    }

    outFile << formatted;
    outFile.close();

    std::cout << "\033[32m✓\033[0m Formatted file: " << filePath << "\n";
    return true;
}

} // namespace vit
