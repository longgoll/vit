#ifndef VIT_TOKEN_H
#define VIT_TOKEN_H

#include <string>
#include <string_view>

namespace vit {

enum class TokenType {
    // Keywords
    KwFunction, // function
    KwLet,      // let
    KwConst,    // const
    KwIf,       // if
    KwElse,     // else
    KwReturn,   // return
    KwPrint,    // print
    KwWhile,    // while
    KwFor,      // for
    KwBreak,    // break
    KwContinue, // continue
    KwTrue,     // true
    KwFalse,    // false
    KwBoolean,  // boolean
    KwString,   // string
    KwVoid,     // void

    // Identifiers & Literals
    Identifier,    // e.g. add, main, x, number
    NumberLiteral, // e.g. 10, 20.5
    StringLiteral, // e.g. "hello world"

    // Operators
    Plus,         // +
    Minus,        // -
    Star,         // *
    Slash,        // /
    Equal,        // =
    EqualEqual,   // ==
    NotEqual,     // !=
    Less,         // <
    Greater,      // >
    LessEqual,    // <=
    GreaterEqual, // >=
    AndAnd,       // &&
    PipePipe,     // ||
    Exclamation,  // !

    // Delimiters
    LParen,    // (
    RParen,    // )
    LBrace,    // {
    RBrace,    // }
    Colon,     // :
    Comma,     // ,
    Semicolon, // ;

    // Special
    TokEof,
    TokUnknown
};

struct Token {
    TokenType type;
    std::string lexeme;
    size_t line;
    size_t column;

    Token(TokenType t, std::string lex, size_t l, size_t c)
        : type(t), lexeme(std::move(lex)), line(l), column(c) {}
};

inline std::string_view tokenTypeToString(TokenType type) {
    switch (type) {
        case TokenType::KwFunction: return "function";
        case TokenType::KwLet: return "let";
        case TokenType::KwConst: return "const";
        case TokenType::KwIf: return "if";
        case TokenType::KwElse: return "else";
        case TokenType::KwReturn: return "return";
        case TokenType::KwPrint: return "print";
        case TokenType::KwWhile: return "while";
        case TokenType::KwFor: return "for";
        case TokenType::KwBreak: return "break";
        case TokenType::KwContinue: return "continue";
        case TokenType::KwTrue: return "true";
        case TokenType::KwFalse: return "false";
        case TokenType::KwBoolean: return "boolean";
        case TokenType::KwString: return "string";
        case TokenType::KwVoid: return "void";
        case TokenType::Identifier: return "Identifier";
        case TokenType::NumberLiteral: return "NumberLiteral";
        case TokenType::StringLiteral: return "StringLiteral";
        case TokenType::Plus: return "+";
        case TokenType::Minus: return "-";
        case TokenType::Star: return "*";
        case TokenType::Slash: return "/";
        case TokenType::Equal: return "=";
        case TokenType::EqualEqual: return "==";
        case TokenType::NotEqual: return "!=";
        case TokenType::Less: return "<";
        case TokenType::Greater: return ">";
        case TokenType::LessEqual: return "<=";
        case TokenType::GreaterEqual: return ">=";
        case TokenType::AndAnd: return "&&";
        case TokenType::PipePipe: return "||";
        case TokenType::Exclamation: return "!";
        case TokenType::LParen: return "(";
        case TokenType::RParen: return ")";
        case TokenType::LBrace: return "{";
        case TokenType::RBrace: return "}";
        case TokenType::Colon: return ":";
        case TokenType::Comma: return ",";
        case TokenType::Semicolon: return ";";
        case TokenType::TokEof: return "<EOF>";
        case TokenType::TokUnknown: return "<Unknown>";
    }
    return "<Unknown>";
}

} // namespace vit

#endif // VIT_TOKEN_H
