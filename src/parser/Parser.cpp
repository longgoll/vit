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
        if (check(TokenType::KwExtern)) {
            advance(); // Consume 'extern'
            functions.push_back(parseFunctionDecl(true));
        } else if (check(TokenType::KwFunction)) {
            functions.push_back(parseFunctionDecl(false));
        } else if (check(TokenType::KwStruct)) {
            topLevelStatements.push_back(parseStructDecl());
        } else if (check(TokenType::KwImport)) {
            topLevelStatements.push_back(parseImportDecl());
        } else {
            topLevelStatements.push_back(parseStatement());
        }
    }

    return std::make_unique<ProgramASTNode>(std::move(functions), std::move(topLevelStatements));
}

std::unique_ptr<FunctionDeclASTNode> Parser::parseFunctionDecl(bool isExtern) {
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
            if (match(TokenType::LBracket)) {
                consume(TokenType::RBracket, "Expected ']' after '['.");
                returnType += "[]";
            }
        } else {
            throw ParseError("Expected return type name after ':'.", curToken.line, curToken.column);
        }
    }

    std::unique_ptr<BlockASTNode> body = nullptr;
    if (isExtern) {
        consume(TokenType::Semicolon, "Expected ';' after extern function declaration.");
    } else {
        body = parseBlock();
    }

    return std::make_unique<FunctionDeclASTNode>(
        nameTok.lexeme, std::move(params), returnType, std::move(body), isExtern
    );
}

std::unique_ptr<StructDeclASTNode> Parser::parseStructDecl() {
    consume(TokenType::KwStruct, "Expected 'struct' keyword.");
    Token nameTok = consume(TokenType::Identifier, "Expected struct name.");
    consume(TokenType::LBrace, "Expected '{' to start struct body.");

    std::vector<std::pair<std::string, std::string>> fields;
    while (!check(TokenType::RBrace) && !check(TokenType::TokEof)) {
        Token fieldName = consume(TokenType::Identifier, "Expected field name.");
        consume(TokenType::Colon, "Expected ':' after field name.");
        Token fieldType = curToken;
        if (check(TokenType::Identifier) || check(TokenType::KwBoolean) || check(TokenType::KwString) || check(TokenType::KwVoid)) {
            advance();
            std::string typeStr = fieldType.lexeme;
            if (match(TokenType::LBracket)) {
                consume(TokenType::RBracket, "Expected ']' after '['.");
                typeStr += "[]";
            }
            fields.push_back({fieldName.lexeme, typeStr});
        } else {
            throw ParseError("Expected field type.", curToken.line, curToken.column);
        }
        if (!match(TokenType::Comma)) {
            if (check(TokenType::RBrace)) break;
        }
    }
    consume(TokenType::RBrace, "Expected '}' after struct body.");
    match(TokenType::Semicolon); // Optional semicolon

    return std::make_unique<StructDeclASTNode>(nameTok.lexeme, std::move(fields));
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
            std::string pType = paramType.lexeme;
            if (match(TokenType::LBracket)) {
                consume(TokenType::RBracket, "Expected ']' after '['.");
                pType += "[]";
            }
            params.push_back({paramName.lexeme, pType});
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

std::unique_ptr<ImportASTNode> Parser::parseImportDecl() {
    consume(TokenType::KwImport, "Expected 'import' keyword.");
    std::vector<std::string> symbols;

    if (match(TokenType::LBrace)) {
        if (!check(TokenType::RBrace)) {
            do {
                Token sym = consume(TokenType::Identifier, "Expected imported symbol name.");
                symbols.push_back(sym.lexeme);
            } while (match(TokenType::Comma));
        }
        consume(TokenType::RBrace, "Expected '}' after imported symbol list.");
        consume(TokenType::KwFrom, "Expected 'from' keyword after import symbol list.");
    }

    Token pathTok = consume(TokenType::StringLiteral, "Expected module path string literal.");
    match(TokenType::Semicolon); // Optional semicolon

    return std::make_unique<ImportASTNode>(std::move(symbols), pathTok.lexeme);
}

std::unique_ptr<StatementNode> Parser::parseStatement() {
    if (check(TokenType::KwImport)) {
        return parseImportDecl();
    }
    if (check(TokenType::KwStruct)) {
        return parseStructDecl();
    }
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

    std::string typeName = ""; // Empty means inferred
    if (match(TokenType::Colon)) {
        Token typeTok = curToken;
        if (check(TokenType::Identifier) || check(TokenType::KwBoolean) || check(TokenType::KwString) || check(TokenType::KwVoid)) {
            advance();
            typeName = typeTok.lexeme;
            if (match(TokenType::LBracket)) {
                consume(TokenType::RBracket, "Expected ']' after '['.");
                typeName += "[]";
            }
        } else {
            throw ParseError("Expected type name after ':'.", curToken.line, curToken.column);
        }
    }

    std::unique_ptr<ExpressionNode> initExpr = nullptr;
    if (match(TokenType::Equal)) {
        initExpr = parseExpression();
    }
    consume(TokenType::Semicolon, "Expected ';' after variable declaration.");

    return std::make_unique<VarDeclASTNode>(
        isConst, nameTok.lexeme, typeName, std::move(initExpr)
    );
}

std::unique_ptr<StatementNode> Parser::parseIdentifierStatement() {
    if (curToken.lexeme == "print") {
        return parsePrint();
    }

    auto lhsExpr = parseExpression();

    if (match(TokenType::Equal)) {
        auto rhsExpr = parseExpression();
        consume(TokenType::Semicolon, "Expected ';' after assignment.");

        if (lhsExpr->getType() == NodeType::VariableExpr) {
            auto varNode = static_cast<VariableExprASTNode*>(lhsExpr.get());
            return std::make_unique<AssignmentASTNode>(varNode->getName(), std::move(rhsExpr));
        } else if (lhsExpr->getType() == NodeType::MemberAccess) {
            auto memNode = static_cast<MemberAccessASTNode*>(lhsExpr.get());
            return std::make_unique<MemberAssignmentASTNode>(memNode->takeTarget(), memNode->getMember(), std::move(rhsExpr));
        } else if (lhsExpr->getType() == NodeType::ArrayAccess) {
            auto arrNode = static_cast<ArrayAccessASTNode*>(lhsExpr.get());
            return std::make_unique<ArrayAssignmentASTNode>(arrNode->takeArray(), arrNode->takeIndex(), std::move(rhsExpr));
        } else {
            throw ParseError("Invalid lvalue in assignment.", curToken.line, curToken.column);
        }
    }

    consume(TokenType::Semicolon, "Expected ';' after expression statement.");
    // Function call statement wrapper or dummy expression statement
    return nullptr;
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

std::unique_ptr<ExpressionNode> Parser::parsePostfix(std::unique_ptr<ExpressionNode> expr) {
    while (true) {
        if (match(TokenType::Dot)) {
            Token memTok = consume(TokenType::Identifier, "Expected member name after '.'.");
            expr = std::make_unique<MemberAccessASTNode>(std::move(expr), memTok.lexeme);
        } else if (match(TokenType::LBracket)) {
            auto indexExpr = parseExpression();
            consume(TokenType::RBracket, "Expected ']' after array index.");
            expr = std::make_unique<ArrayAccessASTNode>(std::move(expr), std::move(indexExpr));
        } else {
            break;
        }
    }
    return expr;
}

std::unique_ptr<ExpressionNode> Parser::parseArrayLiteral() {
    consume(TokenType::LBracket, "Expected '['.");
    std::vector<std::unique_ptr<ExpressionNode>> elements;
    if (!check(TokenType::RBracket)) {
        do {
            elements.push_back(parseExpression());
        } while (match(TokenType::Comma));
    }
    consume(TokenType::RBracket, "Expected ']' after array elements.");
    return std::make_unique<ArrayLiteralASTNode>(std::move(elements));
}

std::unique_ptr<ExpressionNode> Parser::parsePrimary() {
    std::unique_ptr<ExpressionNode> expr = nullptr;

    if (check(TokenType::NumberLiteral)) {
        Token numTok = curToken;
        advance();
        double val = std::stod(numTok.lexeme);
        expr = std::make_unique<NumberLiteralASTNode>(val);
    } else if (check(TokenType::StringLiteral)) {
        Token strTok = curToken;
        advance();
        expr = std::make_unique<StringLiteralASTNode>(strTok.lexeme);
    } else if (check(TokenType::KwTrue)) {
        advance();
        expr = std::make_unique<BooleanLiteralASTNode>(true);
    } else if (check(TokenType::KwFalse)) {
        advance();
        expr = std::make_unique<BooleanLiteralASTNode>(false);
    } else if (check(TokenType::LBracket)) {
        expr = parseArrayLiteral();
    } else if (check(TokenType::Identifier)) {
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
            expr = std::make_unique<CallExprASTNode>(idTok.lexeme, std::move(args));
        } else {
            // Identifier variable reference: x
            expr = std::make_unique<VariableExprASTNode>(idTok.lexeme);
        }
    } else if (match(TokenType::LParen)) {
        expr = parseExpression();
        consume(TokenType::RParen, "Expected ')' after expression.");
    } else {
        std::string msg = "Unexpected expression token '" + curToken.lexeme + "'";
        throw ParseError(msg, curToken.line, curToken.column);
    }

    return parsePostfix(std::move(expr));
}

} // namespace vit
