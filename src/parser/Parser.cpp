#include "parser/Parser.h"
#include <iostream>

namespace vit {

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
        if (check(TokenType::KwFunction)) {
            functions.push_back(parseFunctionDecl());
        } else {
            topLevelStatements.push_back(parseStatement());
        }
    }

    return std::make_unique<ProgramASTNode>(std::move(functions), std::move(topLevelStatements));
}

std::unique_ptr<FunctionDeclASTNode> Parser::parseFunctionDecl() {
    consume(TokenType::KwFunction, "Expected 'function' keyword.");
    Token nameTok = consume(TokenType::Identifier, "Expected function name.");

    consume(TokenType::LParen, "Expected '(' after function name.");
    std::vector<Parameter> params = parseParameterList();
    consume(TokenType::RParen, "Expected ')' after parameter list.");

    std::string returnType = "void";
    if (match(TokenType::Colon)) {
        Token typeTok = consume(TokenType::Identifier, "Expected return type name after ':'.");
        returnType = typeTok.lexeme;
    }

    std::unique_ptr<BlockASTNode> body = parseBlock();

    return std::make_unique<FunctionDeclASTNode>(
        nameTok.lexeme, std::move(params), returnType, std::move(body)
    );
}

std::vector<Parameter> Parser::parseParameterList() {
    std::vector<Parameter> params;
    if (check(TokenType::RParen)) {
        return params;
    }

    do {
        Token paramName = consume(TokenType::Identifier, "Expected parameter name.");
        consume(TokenType::Colon, "Expected ':' after parameter name.");
        Token paramType = consume(TokenType::Identifier, "Expected parameter type.");
        params.push_back({paramName.lexeme, paramType.lexeme});
    } while (match(TokenType::Comma));

    return params;
}

std::unique_ptr<BlockASTNode> Parser::parseBlock() {
    consume(TokenType::LBrace, "Expected '{' to start block.");
    std::vector<std::unique_ptr<StatementNode>> statements;

    while (!check(TokenType::RBrace) && !check(TokenType::TokEof)) {
        statements.push_back(parseStatement());
    }

    consume(TokenType::RBrace, "Expected '}' to end block.");
    return std::make_unique<BlockASTNode>(std::move(statements));
}

std::unique_ptr<StatementNode> Parser::parseStatement() {
    if (check(TokenType::KwLet) || check(TokenType::KwConst)) {
        return parseVarDecl();
    }
    if (check(TokenType::KwIf)) {
        return parseIf();
    }
    if (check(TokenType::KwReturn)) {
        return parseReturn();
    }
    if (check(TokenType::KwPrint)) {
        return parsePrint();
    }
    if (check(TokenType::LBrace)) {
        return parseBlock();
    }
    if (check(TokenType::Identifier)) {
        return parseIdentifierStatement();
    }

    std::string msg = "Unexpected statement token '" + curToken.lexeme + "'";
    throw ParseError(msg, curToken.line, curToken.column);
}

std::unique_ptr<VarDeclASTNode> Parser::parseVarDecl() {
    bool isConst = false;
    if (match(TokenType::KwConst)) {
        isConst = true;
    } else {
        consume(TokenType::KwLet, "Expected 'let' or 'const'.");
    }

    Token nameTok = consume(TokenType::Identifier, "Expected variable name.");

    std::string typeName = "number"; // Default primitive
    if (match(TokenType::Colon)) {
        Token typeTok = consume(TokenType::Identifier, "Expected type name.");
        typeName = typeTok.lexeme;
    }

    consume(TokenType::Equal, "Expected '=' in variable declaration.");
    std::unique_ptr<ExpressionNode> initExpr = parseExpression();
    consume(TokenType::Semicolon, "Expected ';' after variable declaration.");

    return std::make_unique<VarDeclASTNode>(
        isConst, nameTok.lexeme, typeName, std::move(initExpr)
    );
}

std::unique_ptr<StatementNode> Parser::parseIdentifierStatement() {
    // Check for print statement formatted as function call: print(val);
    if (curToken.lexeme == "print") {
        return parsePrint();
    }

    // Otherwise it's variable assignment: x = expr;
    Token nameTok = consume(TokenType::Identifier, "Expected variable name.");
    consume(TokenType::Equal, "Expected '=' after variable name in assignment.");
    std::unique_ptr<ExpressionNode> valExpr = parseExpression();
    consume(TokenType::Semicolon, "Expected ';' after assignment.");

    return std::make_unique<AssignmentASTNode>(nameTok.lexeme, std::move(valExpr));
}

std::unique_ptr<IfASTNode> Parser::parseIf() {
    consume(TokenType::KwIf, "Expected 'if'.");
    consume(TokenType::LParen, "Expected '(' after 'if'.");
    std::unique_ptr<ExpressionNode> cond = parseExpression();
    consume(TokenType::RParen, "Expected ')' after if condition.");

    std::unique_ptr<BlockASTNode> thenBlock = parseBlock();
    std::unique_ptr<BlockASTNode> elseBlock = nullptr;

    if (match(TokenType::KwElse)) {
        elseBlock = parseBlock();
    }

    return std::make_unique<IfASTNode>(
        std::move(cond), std::move(thenBlock), std::move(elseBlock)
    );
}

std::unique_ptr<ReturnASTNode> Parser::parseReturn() {
    consume(TokenType::KwReturn, "Expected 'return'.");
    std::unique_ptr<ExpressionNode> retVal = nullptr;

    if (!check(TokenType::Semicolon)) {
        retVal = parseExpression();
    }

    consume(TokenType::Semicolon, "Expected ';' after return statement.");
    return std::make_unique<ReturnASTNode>(std::move(retVal));
}

std::unique_ptr<PrintASTNode> Parser::parsePrint() {
    if (check(TokenType::KwPrint)) {
        advance();
    } else if (check(TokenType::Identifier) && curToken.lexeme == "print") {
        advance();
    } else {
        throw ParseError("Expected 'print'.", curToken.line, curToken.column);
    }

    consume(TokenType::LParen, "Expected '(' after 'print'.");
    std::unique_ptr<ExpressionNode> expr = parseExpression();
    consume(TokenType::RParen, "Expected ')' after print expression.");
    consume(TokenType::Semicolon, "Expected ';' after print statement.");

    return std::make_unique<PrintASTNode>(std::move(expr));
}

std::unique_ptr<ExpressionNode> Parser::parseExpression() {
    return parseEquality();
}

std::unique_ptr<ExpressionNode> Parser::parseEquality() {
    auto left = parseRelational();

    while (check(TokenType::EqualEqual) || check(TokenType::NotEqual)) {
        Token opTok = curToken;
        advance();
        auto right = parseRelational();
        left = std::make_unique<BinaryOpASTNode>(opTok.lexeme, std::move(left), std::move(right));
    }

    return left;
}

std::unique_ptr<ExpressionNode> Parser::parseRelational() {
    auto left = parseAdditive();

    while (check(TokenType::Less) || check(TokenType::Greater) ||
           check(TokenType::LessEqual) || check(TokenType::GreaterEqual)) {
        Token opTok = curToken;
        advance();
        auto right = parseAdditive();
        left = std::make_unique<BinaryOpASTNode>(opTok.lexeme, std::move(left), std::move(right));
    }

    return left;
}

std::unique_ptr<ExpressionNode> Parser::parseAdditive() {
    auto left = parseMultiplicative();

    while (check(TokenType::Plus) || check(TokenType::Minus)) {
        Token opTok = curToken;
        advance();
        auto right = parseMultiplicative();
        left = std::make_unique<BinaryOpASTNode>(opTok.lexeme, std::move(left), std::move(right));
    }

    return left;
}

std::unique_ptr<ExpressionNode> Parser::parseMultiplicative() {
    auto left = parsePrimary();

    while (check(TokenType::Star) || check(TokenType::Slash)) {
        Token opTok = curToken;
        advance();
        auto right = parsePrimary();
        left = std::make_unique<BinaryOpASTNode>(opTok.lexeme, std::move(left), std::move(right));
    }

    return left;
}

std::unique_ptr<ExpressionNode> Parser::parsePrimary() {
    if (check(TokenType::NumberLiteral)) {
        Token numTok = curToken;
        advance();
        double val = std::stod(numTok.lexeme);
        return std::make_unique<NumberLiteralASTNode>(val);
    }

    if (check(TokenType::Identifier)) {
        Token idTok = curToken;
        advance();

        // Function call: add(x, y)
        if (match(TokenType::LParen)) {
            std::vector<std::unique_ptr<ExpressionNode>> args;
            if (!check(TokenType::RParen)) {
                do {
                    args.push_back(parseExpression());
                } while (match(TokenType::Comma));
            }
            consume(TokenType::RParen, "Expected ')' after function call arguments.");
            return std::make_unique<CallExprASTNode>(idTok.lexeme, std::move(args));
        }

        // Identifier variable reference: x
        return std::make_unique<VariableExprASTNode>(idTok.lexeme);
    }

    if (match(TokenType::LParen)) {
        auto expr = parseExpression();
        consume(TokenType::RParen, "Expected ')' after expression.");
        return expr;
    }

    std::string msg = "Unexpected expression token '" + curToken.lexeme + "'";
    throw ParseError(msg, curToken.line, curToken.column);
}

} // namespace vit
