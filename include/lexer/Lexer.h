#ifndef VIT_LEXER_H
#define VIT_LEXER_H

#include "Token.h"
#include <string>
#include <vector>

namespace vit {

class Lexer {
private:
    std::string source;
    size_t cursor = 0;
    size_t line = 1;
    size_t column = 1;

    char peek() const;
    char peekNext() const;
    char advance();
    bool isAtEnd() const;
    void skipWhitespaceAndComments();

    Token number();
    Token stringLiteral();
    Token templateStringLiteral();
    Token identifierOrKeyword();

public:
    explicit Lexer(std::string sourceCode);

    Token nextToken();
    std::vector<Token> tokenizeAll();
};

} // namespace vit

#endif // VIT_LEXER_H
