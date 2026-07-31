#include "lexer/Lexer.h"
#include <cctype>
#include <unordered_map>

namespace vit {

static const std::unordered_map<std::string, TokenType> keywords = {
    {"function", TokenType::KwFunction},
    {"let", TokenType::KwLet},
    {"const", TokenType::KwConst},
    {"if", TokenType::KwIf},
    {"else", TokenType::KwElse},
    {"return", TokenType::KwReturn},
    {"print", TokenType::KwPrint},
    {"while", TokenType::KwWhile},
    {"for", TokenType::KwFor},
    {"break", TokenType::KwBreak},
    {"continue", TokenType::KwContinue},
    {"true", TokenType::KwTrue},
    {"false", TokenType::KwFalse},
    {"boolean", TokenType::KwBoolean},
    {"string", TokenType::KwString},
    {"void", TokenType::KwVoid},
    {"struct", TokenType::KwStruct},
    {"extern", TokenType::KwExtern},
    {"import", TokenType::KwImport},
    {"from", TokenType::KwFrom},
    {"type", TokenType::KwType}
};

Lexer::Lexer(std::string sourceCode) : source(std::move(sourceCode)) {}

char Lexer::peek() const {
    if (isAtEnd()) return '\0';
    return source[cursor];
}

char Lexer::peekNext() const {
    if (cursor + 1 >= source.size()) return '\0';
    return source[cursor + 1];
}

char Lexer::advance() {
    if (isAtEnd()) return '\0';
    char c = source[cursor++];
    if (c == '\n') {
        line++;
        column = 1;
    } else {
        column++;
    }
    return c;
}

bool Lexer::isAtEnd() const {
    return cursor >= source.size();
}

void Lexer::skipWhitespaceAndComments() {
    while (!isAtEnd()) {
        char c = peek();
        if (c == ' ' || c == '\t' || c == '\r' || c == '\n') {
            advance();
        } else if (c == '/' && peekNext() == '/') {
            // Single-line comment: // ...
            while (!isAtEnd() && peek() != '\n') {
                advance();
            }
        } else if (c == '/' && peekNext() == '*') {
            // Multi-line comment: /* ... */
            advance(); // '/'
            advance(); // '*'
            while (!isAtEnd()) {
                if (peek() == '*' && peekNext() == '/') {
                    advance(); // '*'
                    advance(); // '/'
                    break;
                }
                advance();
            }
        } else {
            break;
        }
    }
}

Token Lexer::number() {
    size_t startLine = line;
    size_t startColumn = column;
    std::string numStr;

    while (!isAtEnd() && (std::isdigit(peek()) || peek() == '.')) {
        numStr += advance();
    }

    return Token(TokenType::NumberLiteral, numStr, startLine, startColumn);
}

Token Lexer::stringLiteral() {
    size_t startLine = line;
    size_t startColumn = column;
    advance(); // Consume opening '"'
    std::string value;

    while (!isAtEnd() && peek() != '"') {
        if (peek() == '\\' && peekNext() != '\0') {
            advance(); // Consume '\'
            char escaped = advance();
            if (escaped == 'n') value += '\n';
            else if (escaped == 't') value += '\t';
            else if (escaped == 'r') value += '\r';
            else if (escaped == '"') value += '"';
            else if (escaped == '\\') value += '\\';
            else value += escaped;
        } else {
            value += advance();
        }
    }

    if (isAtEnd()) {
        return Token(TokenType::TokUnknown, value, startLine, startColumn);
    }

    advance(); // Consume closing '"'
    return Token(TokenType::StringLiteral, value, startLine, startColumn);
}

Token Lexer::identifierOrKeyword() {
    size_t startLine = line;
    size_t startColumn = column;
    std::string text;

    while (!isAtEnd() && (std::isalnum(peek()) || peek() == '_')) {
        text += advance();
    }

    auto it = keywords.find(text);
    if (it != keywords.end()) {
        return Token(it->second, text, startLine, startColumn);
    }
    return Token(TokenType::Identifier, text, startLine, startColumn);
}

Token Lexer::nextToken() {
    skipWhitespaceAndComments();

    if (isAtEnd()) {
        return Token(TokenType::TokEof, "", line, column);
    }

    size_t startLine = line;
    size_t startColumn = column;
    char c = peek();

    if (c == '"') {
        return stringLiteral();
    }

    if (std::isdigit(c)) {
        return number();
    }

    if (std::isalpha(c) || c == '_') {
        return identifierOrKeyword();
    }

    advance(); // Consume the character

    switch (c) {
        case '+': return Token(TokenType::Plus, "+", startLine, startColumn);
        case '-': return Token(TokenType::Minus, "-", startLine, startColumn);
        case '*': return Token(TokenType::Star, "*", startLine, startColumn);
        case '/': return Token(TokenType::Slash, "/", startLine, startColumn);
        case '.': return Token(TokenType::Dot, ".", startLine, startColumn);
        case '(': return Token(TokenType::LParen, "(", startLine, startColumn);
        case ')': return Token(TokenType::RParen, ")", startLine, startColumn);
        case '{': return Token(TokenType::LBrace, "{", startLine, startColumn);
        case '}': return Token(TokenType::RBrace, "}", startLine, startColumn);
        case '[': return Token(TokenType::LBracket, "[", startLine, startColumn);
        case ']': return Token(TokenType::RBracket, "]", startLine, startColumn);
        case ':': return Token(TokenType::Colon, ":", startLine, startColumn);
        case ',': return Token(TokenType::Comma, ",", startLine, startColumn);
        case ';': return Token(TokenType::Semicolon, ";", startLine, startColumn);

        case '=':
            if (peek() == '=') {
                advance();
                return Token(TokenType::EqualEqual, "==", startLine, startColumn);
            }
            if (peek() == '>') {
                advance();
                return Token(TokenType::Arrow, "=>", startLine, startColumn);
            }
            return Token(TokenType::Equal, "=", startLine, startColumn);

        case '!':
            if (peek() == '=') {
                advance();
                return Token(TokenType::NotEqual, "!=", startLine, startColumn);
            }
            return Token(TokenType::Exclamation, "!", startLine, startColumn);

        case '&':
            if (peek() == '&') {
                advance();
                return Token(TokenType::AndAnd, "&&", startLine, startColumn);
            }
            return Token(TokenType::TokUnknown, "&", startLine, startColumn);

        case '|':
            if (peek() == '|') {
                advance();
                return Token(TokenType::PipePipe, "||", startLine, startColumn);
            }
            return Token(TokenType::TokUnknown, "|", startLine, startColumn);

        case '<':
            if (peek() == '=') {
                advance();
                return Token(TokenType::LessEqual, "<=", startLine, startColumn);
            }
            return Token(TokenType::Less, "<", startLine, startColumn);

        case '>':
            if (peek() == '=') {
                advance();
                return Token(TokenType::GreaterEqual, ">=", startLine, startColumn);
            }
            return Token(TokenType::Greater, ">", startLine, startColumn);

        default:
            return Token(TokenType::TokUnknown, std::string(1, c), startLine, startColumn);
    }
}

std::vector<Token> Lexer::tokenizeAll() {
    std::vector<Token> tokens;
    while (true) {
        Token tok = nextToken();
        tokens.push_back(tok);
        if (tok.type == TokenType::TokEof) break;
    }
    return tokens;
}

} // namespace vit
