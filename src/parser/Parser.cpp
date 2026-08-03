#include "parser/Parser.h"
#include <iostream>

namespace vit {

// Core Parser infrastructure and top-level program parsing.
// Statement parsing → Parser_Stmt.cpp
// Expression parsing → Parser_Expr.cpp

Parser::Parser(Lexer lex) : lexer(std::move(lex)), curToken(TokenType::TokUnknown, "", 0, 0) {
    advance();
}

void Parser::advance() {
    curToken = lexer.nextToken();
}

bool Parser::check(TokenType type) const {
    return curToken.type == type;
}

bool Parser::match(TokenType type) {
    if (check(type)) {
        advance();
        return true;
    }
    return false;
}

Token Parser::consume(TokenType type, const std::string& errorMessage) {
    if (check(type)) {
        Token tok = curToken;
        advance();
        return tok;
    }
    std::string msg = "Syntax Error at line " + std::to_string(curToken.line) +
                      ", column " + std::to_string(curToken.column) +
                      ": Expected '" + std::string(tokenTypeToString(type)) +
                      "' but got '" + curToken.lexeme + "'. " + errorMessage;
    throw ParseError(msg, curToken.line, curToken.column);
}

std::unique_ptr<ProgramASTNode> Parser::parseProgram() {
    std::vector<std::unique_ptr<FunctionDeclASTNode>> functions;
    std::vector<std::unique_ptr<StatementNode>> topLevelStatements;

    while (!check(TokenType::TokEof)) {
        if (check(TokenType::KwExtern)) {
            advance(); // Consume 'extern'
            functions.push_back(parseFunctionDecl(true));
        } else if (check(TokenType::KwAsync)) {
            advance(); // Consume 'async'
            functions.push_back(parseFunctionDecl(false, true));
        } else if (check(TokenType::KwFunction) || check(TokenType::KwFn)) {
            functions.push_back(parseFunctionDecl(false));
        } else if (check(TokenType::KwStruct)) {
            topLevelStatements.push_back(parseStructDecl());
        } else if (check(TokenType::KwEnum)) {
            topLevelStatements.push_back(parseEnumDecl());
        } else if (check(TokenType::KwImport)) {
            topLevelStatements.push_back(parseImportDecl());
        } else if (check(TokenType::KwType)) {
            topLevelStatements.push_back(parseTypeAlias());
        } else {
            auto stmt = parseStatement();
            if (stmt) topLevelStatements.push_back(std::move(stmt));
        }
    }

    return std::make_unique<ProgramASTNode>(std::move(functions), std::move(topLevelStatements));
}

std::string Parser::parseTypeSpec() {
    std::string result;
    if (check(TokenType::LParen)) {
        advance();
        result += "(";
        bool first = true;
        while (!check(TokenType::RParen) && !check(TokenType::TokEof)) {
            if (!first) {
                consume(TokenType::Comma, "Expected ',' in function type parameter list.");
                result += ", ";
            }
            first = false;

            Token t1 = curToken;
            if (check(TokenType::Identifier) || check(TokenType::KwBoolean) || check(TokenType::KwString) || check(TokenType::KwVoid)) {
                advance();
                if (match(TokenType::Colon)) {
                    std::string paramType = parseTypeSpec();
                    result += paramType;
                } else {
                    std::string paramType = t1.lexeme;
                    if (match(TokenType::LBracket)) {
                        consume(TokenType::RBracket, "Expected ']' after '['.");
                        paramType += "[]";
                    }
                    result += paramType;
                }
            } else if (check(TokenType::LParen)) {
                std::string paramType = parseTypeSpec();
                result += paramType;
            } else {
                throw ParseError("Expected type in function parameter list.", curToken.line, curToken.column);
            }
        }
        consume(TokenType::RParen, "Expected ')' in function type.");
        result += ")";

        consume(TokenType::Arrow, "Expected '=>' in function type.");
        result += " => ";
        std::string retType = parseTypeSpec();
        result += retType;
    } else if (check(TokenType::Identifier) || check(TokenType::KwBoolean) || check(TokenType::KwString) || check(TokenType::KwVoid)
              || check(TokenType::KwInt) || check(TokenType::KwFloat)) {
        Token typeTok = curToken;
        advance();
        result = typeTok.lexeme;
        if (match(TokenType::Less)) {
            result += "<";
            bool first = true;
            while (!check(TokenType::Greater) && !check(TokenType::TokEof)) {
                if (!first) {
                    consume(TokenType::Comma, "Expected ',' between generic type arguments.");
                    result += ", ";
                }
                first = false;
                result += parseTypeSpec();
            }
            consume(TokenType::Greater, "Expected '>' after generic type arguments.");
            result += ">";
        }
    } else {
        throw ParseError("Expected type name.", curToken.line, curToken.column);
    }

    while (true) {
        if (match(TokenType::Question)) {
            result += "?";
        } else if (match(TokenType::LBracket)) {
            consume(TokenType::RBracket, "Expected ']' after '['.");
            result += "[]";
        } else {
            break;
        }
    }

    return result;
}

std::unique_ptr<TypeAliasASTNode> Parser::parseTypeAlias() {
    consume(TokenType::KwType, "Expected 'type' keyword.");
    Token aliasTok = consume(TokenType::Identifier, "Expected type alias name.");
    consume(TokenType::Equal, "Expected '=' in type alias declaration.");
    std::string targetType = parseTypeSpec();
    match(TokenType::Semicolon);
    return std::make_unique<TypeAliasASTNode>(aliasTok.lexeme, targetType);
}

std::vector<std::string> Parser::parseGenericParams() {
    std::vector<std::string> params;
    if (match(TokenType::Less)) {
        do {
            Token t = consume(TokenType::Identifier, "Expected generic parameter identifier.");
            params.push_back(t.lexeme);
        } while (match(TokenType::Comma));
        consume(TokenType::Greater, "Expected '>' after generic parameter list.");
    }
    return params;
}

} // namespace vit
