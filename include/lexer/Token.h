#ifndef VIT_TOKEN_H
#define VIT_TOKEN_H

#include <string>
#include <string_view>

namespace vit {

enum class TokenType {
    // Keywords
    KwFunction, // function
    KwFn,       // fn (alias for function)
    KwLet,      // let
    KwConst,    // const
    KwIf,       // if
    KwElse,     // else
    KwReturn,   // return
    KwPrint,    // print
    KwWhile,    // while
    KwFor,      // for
    KwIn,       // in (for-in loop)
    KwBreak,    // break
    KwContinue, // continue
    KwTrue,     // true
    KwFalse,    // false
    KwBoolean,  // boolean
    KwInt,      // int (integer type)
    KwFloat,    // float (float type)
    KwString,   // string
    KwVoid,     // void
    KwStruct,   // struct
    KwExtern,   // extern
    KwImport,   // import
    KwFrom,     // from
    KwType,     // type
    KwEnum,     // enum
    KwMatch,    // match
    KwNull,     // null
    KwAsync,    // async
    KwAwait,    // await

    // Identifiers & Literals
    Identifier,    // e.g. add, main, x, number
    NumberLiteral, // e.g. 10, 20.5
    StringLiteral, // e.g. "hello world"

    // Operators
    Plus,               // +
    Minus,              // -
    Star,               // *
    Slash,              // /
    Percent,            // %
    Equal,              // =
    EqualEqual,         // ==
    NotEqual,           // !=
    Less,               // <
    Greater,            // >
    LessEqual,          // <=
    GreaterEqual,       // >=
    AndAnd,             // &&
    PipePipe,           // ||
    Ampersand,          // & (bitwise AND)
    Pipe,               // | (bitwise OR)
    Caret,              // ^ (bitwise XOR)
    Tilde,              // ~ (bitwise NOT)
    ShiftLeft,          // <<
    ShiftRight,         // >>
    PlusPlus,           // ++
    MinusMinus,         // --
    PlusEqual,          // +=
    MinusEqual,         // -=
    StarEqual,          // *=
    SlashEqual,         // /=
    PercentEqual,       // %=
    Exclamation,        // !
    Dot,                // .
    Arrow,              // =>
    Question,           // ?
    QuestionDot,        // ?.
    NullishCoalescing,  // ??
    Backtick,           // ` (template string start/end)
    DollarLBrace,       // ${ (template interpolation start)

    // Delimiters
    LParen,    // (
    RParen,    // )
    LBrace,    // {
    RBrace,    // }
    LBracket,  // [
    RBracket,  // ]
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
        case TokenType::KwFn: return "fn";
        case TokenType::KwLet: return "let";
        case TokenType::KwConst: return "const";
        case TokenType::KwIf: return "if";
        case TokenType::KwElse: return "else";
        case TokenType::KwReturn: return "return";
        case TokenType::KwPrint: return "print";
        case TokenType::KwWhile: return "while";
        case TokenType::KwFor: return "for";
        case TokenType::KwIn: return "in";
        case TokenType::KwBreak: return "break";
        case TokenType::KwContinue: return "continue";
        case TokenType::KwTrue: return "true";
        case TokenType::KwFalse: return "false";
        case TokenType::KwBoolean: return "boolean";
        case TokenType::KwInt: return "int";
        case TokenType::KwFloat: return "float";
        case TokenType::KwString: return "string";
        case TokenType::KwVoid: return "void";
        case TokenType::KwStruct: return "struct";
        case TokenType::KwExtern: return "extern";
        case TokenType::KwImport: return "import";
        case TokenType::KwFrom: return "from";
        case TokenType::KwType: return "type";
        case TokenType::KwEnum: return "enum";
        case TokenType::KwMatch: return "match";
        case TokenType::KwNull: return "null";
        case TokenType::KwAsync: return "async";
        case TokenType::KwAwait: return "await";
        case TokenType::Identifier: return "Identifier";
        case TokenType::NumberLiteral: return "NumberLiteral";
        case TokenType::StringLiteral: return "StringLiteral";
        case TokenType::Plus: return "+";
        case TokenType::Minus: return "-";
        case TokenType::Star: return "*";
        case TokenType::Slash: return "/";
        case TokenType::Percent: return "%";
        case TokenType::Equal: return "=";
        case TokenType::EqualEqual: return "==";
        case TokenType::NotEqual: return "!=";
        case TokenType::Less: return "<";
        case TokenType::Greater: return ">";
        case TokenType::LessEqual: return "<=";
        case TokenType::GreaterEqual: return ">=";
        case TokenType::AndAnd: return "&&";
        case TokenType::PipePipe: return "||";
        case TokenType::Ampersand: return "&";
        case TokenType::Pipe: return "|";
        case TokenType::Caret: return "^";
        case TokenType::Tilde: return "~";
        case TokenType::ShiftLeft: return "<<";
        case TokenType::ShiftRight: return ">>";
        case TokenType::PlusPlus: return "++";
        case TokenType::MinusMinus: return "--";
        case TokenType::PlusEqual: return "+=";
        case TokenType::MinusEqual: return "-=";
        case TokenType::StarEqual: return "*=";
        case TokenType::SlashEqual: return "/=";
        case TokenType::PercentEqual: return "%=";
        case TokenType::Exclamation: return "!";
        case TokenType::Dot: return ".";
        case TokenType::Arrow: return "=>";
        case TokenType::Question: return "?";
        case TokenType::QuestionDot: return "?.";
        case TokenType::NullishCoalescing: return "??";
        case TokenType::Backtick: return "`";
        case TokenType::DollarLBrace: return "${";
        case TokenType::LParen: return "(";
        case TokenType::RParen: return ")";
        case TokenType::LBrace: return "{";
        case TokenType::RBrace: return "}";
        case TokenType::LBracket: return "[";
        case TokenType::RBracket: return "]";
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
