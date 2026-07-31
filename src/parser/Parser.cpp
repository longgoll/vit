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
        Token typeTok = curToken;
        if (check(TokenType::Identifier) || check(TokenType::KwBoolean) || check(TokenType::KwString) || check(TokenType::KwVoid)) {
            advance();
            returnType = typeTok.lexeme;
        } else {
            throw ParseError("Expected return type name after ':'.", curToken.line, curToken.column);
        }
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
        Token paramType = curToken;
        if (check(TokenType::Identifier) || check(TokenType::KwBoolean) || check(TokenType::KwString) || check(TokenType::KwVoid)) {
            advance();
            params.push_back({paramName.lexeme, paramType.lexeme});
        } else {
            throw ParseError("Expected parameter type name.", curToken.line, curToken.column);
        }
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
    if (check(TokenType::KwWhile)) {
        return parseWhile();
    }
    if (check(TokenType::KwFor)) {
        return parseFor();
    }
    if (check(TokenType::KwBreak)) {
        return parseBreak();
    }
    if (check(TokenType::KwContinue)) {
        return parseContinue();
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
        Token typeTok = curToken;
        if (check(TokenType::Identifier) || check(TokenType::KwBoolean) || check(TokenType::KwString) || check(TokenType::KwVoid)) {
            advance();
            typeName = typeTok.lexeme;
        } else {
            throw ParseError("Expected type name after ':'.", curToken.line, curToken.column);
        }
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

std::unique_ptr<WhileASTNode> Parser::parseWhile() {
    consume(TokenType::KwWhile, "Expected 'while'.");
    consume(TokenType::LParen, "Expected '(' after 'while'.");
    std::unique_ptr<ExpressionNode> cond = parseExpression();
    consume(TokenType::RParen, "Expected ')' after while condition.");

    std::unique_ptr<BlockASTNode> body = parseBlock();

    return std::make_unique<WhileASTNode>(std::move(cond), std::move(body));
}

std::unique_ptr<ForASTNode> Parser::parseFor() {
    consume(TokenType::KwFor, "Expected 'for'.");
    consume(TokenType::LParen, "Expected '(' after 'for'.");

    // Init statement (VarDecl or Assignment or nullptr)
    std::unique_ptr<StatementNode> init = nullptr;
    if (!check(TokenType::Semicolon)) {
        if (check(TokenType::KwLet) || check(TokenType::KwConst)) {
            init = parseVarDecl(); // parseVarDecl consumes trailing ';'
        } else if (check(TokenType::Identifier)) {
            Token nameTok = consume(TokenType::Identifier, "Expected variable name in for init.");
            consume(TokenType::Equal, "Expected '=' after variable name.");
            auto valExpr = parseExpression();
            consume(TokenType::Semicolon, "Expected ';' after for init statement.");
            init = std::make_unique<AssignmentASTNode>(nameTok.lexeme, std::move(valExpr));
        }
    } else {
        consume(TokenType::Semicolon, "Expected ';'.");
    }

    // Condition expression (or nullptr)
    std::unique_ptr<ExpressionNode> cond = nullptr;
    if (!check(TokenType::Semicolon)) {
        cond = parseExpression();
    }
    consume(TokenType::Semicolon, "Expected ';' after for condition.");

    // Update statement (Assignment without trailing ';', or nullptr)
    std::unique_ptr<StatementNode> update = nullptr;
    if (!check(TokenType::RParen)) {
        Token nameTok = consume(TokenType::Identifier, "Expected variable name in for update.");
        consume(TokenType::Equal, "Expected '=' in for update statement.");
        auto valExpr = parseExpression();
        update = std::make_unique<AssignmentASTNode>(nameTok.lexeme, std::move(valExpr));
    }
    consume(TokenType::RParen, "Expected ')' after for clause.");

    std::unique_ptr<BlockASTNode> body = parseBlock();

    return std::make_unique<ForASTNode>(
        std::move(init), std::move(cond), std::move(update), std::move(body)
    );
}

std::unique_ptr<BreakASTNode> Parser::parseBreak() {
    consume(TokenType::KwBreak, "Expected 'break'.");
    consume(TokenType::Semicolon, "Expected ';' after break statement.");
    return std::make_unique<BreakASTNode>();
}

std::unique_ptr<ContinueASTNode> Parser::parseContinue() {
    consume(TokenType::KwContinue, "Expected 'continue'.");
    consume(TokenType::Semicolon, "Expected ';' after continue statement.");
    return std::make_unique<ContinueASTNode>();
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
    return parseLogicalOr();
}

std::unique_ptr<ExpressionNode> Parser::parseLogicalOr() {
    auto left = parseLogicalAnd();

    while (check(TokenType::PipePipe)) {
        Token opTok = curToken;
        advance();
        auto right = parseLogicalAnd();
        left = std::make_unique<BinaryOpASTNode>(opTok.lexeme, std::move(left), std::move(right));
    }

    return left;
}

std::unique_ptr<ExpressionNode> Parser::parseLogicalAnd() {
    auto left = parseEquality();

    while (check(TokenType::AndAnd)) {
        Token opTok = curToken;
        advance();
        auto right = parseEquality();
        left = std::make_unique<BinaryOpASTNode>(opTok.lexeme, std::move(left), std::move(right));
    }

    return left;
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
    auto left = parseUnary();

    while (check(TokenType::Star) || check(TokenType::Slash)) {
        Token opTok = curToken;
        advance();
        auto right = parseUnary();
        left = std::make_unique<BinaryOpASTNode>(opTok.lexeme, std::move(left), std::move(right));
    }

    return left;
}

std::unique_ptr<ExpressionNode> Parser::parseUnary() {
    if (check(TokenType::Exclamation) || check(TokenType::Minus)) {
        Token opTok = curToken;
        advance();
        auto operand = parseUnary();
        return std::make_unique<UnaryOpASTNode>(opTok.lexeme, std::move(operand));
    }

    return parsePrimary();
}

std::unique_ptr<ExpressionNode> Parser::parsePrimary() {
    if (check(TokenType::NumberLiteral)) {
        Token numTok = curToken;
        advance();
        double val = std::stod(numTok.lexeme);
        return std::make_unique<NumberLiteralASTNode>(val);
    }

    if (check(TokenType::StringLiteral)) {
        Token strTok = curToken;
        advance();
        return std::make_unique<StringLiteralASTNode>(strTok.lexeme);
    }

    if (check(TokenType::KwTrue)) {
        advance();
        return std::make_unique<BooleanLiteralASTNode>(true);
    }

    if (check(TokenType::KwFalse)) {
        advance();
        return std::make_unique<BooleanLiteralASTNode>(false);
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
