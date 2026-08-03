#include "parser/Parser.h"
#include <iostream>

namespace vit {

// Statement and Declaration parsing implementation for Parser

std::vector<Parameter> Parser::parseParameterList() {
    std::vector<Parameter> params;
    if (check(TokenType::RParen)) {
        return params;
    }

    do {
        Token paramName = consume(TokenType::Identifier, "Expected parameter name.");
        consume(TokenType::Colon, "Expected ':' after parameter name.");
        std::string pType = parseTypeSpec();
        params.push_back({paramName.lexeme, pType});
    } while (match(TokenType::Comma));

    return params;
}

std::unique_ptr<FunctionDeclASTNode> Parser::parseFunctionDecl(bool isExtern, bool isAsync) {
    if (check(TokenType::KwAsync)) {
        advance();
        isAsync = true;
    }
    if (check(TokenType::KwFunction) || check(TokenType::KwFn)) {
        advance();
    } else {
        throw ParseError("Expected 'function' or 'fn' keyword.", curToken.line, curToken.column);
    }
    Token nameTok = consume(TokenType::Identifier, "Expected function name.");

    auto genParams = parseGenericParams();

    consume(TokenType::LParen, "Expected '(' after function name.");
    std::vector<Parameter> params = parseParameterList();
    consume(TokenType::RParen, "Expected ')' after parameter list.");

    std::string returnType = "void";
    if (match(TokenType::Colon)) {
        returnType = parseTypeSpec();
    }

    std::unique_ptr<BlockASTNode> body = nullptr;
    if (isExtern) {
        consume(TokenType::Semicolon, "Expected ';' after extern function declaration.");
    } else {
        body = parseBlock();
    }

    return std::make_unique<FunctionDeclASTNode>(
        nameTok.lexeme, std::move(params), returnType, std::move(body), isExtern, std::move(genParams), isAsync
    );
}

std::unique_ptr<StructDeclASTNode> Parser::parseStructDecl() {
    consume(TokenType::KwStruct, "Expected 'struct' keyword.");
    Token nameTok = consume(TokenType::Identifier, "Expected struct name.");
    auto genParams = parseGenericParams();

    consume(TokenType::LBrace, "Expected '{' to start struct body.");

    std::vector<std::pair<std::string, std::string>> fields;
    std::vector<std::unique_ptr<FunctionDeclASTNode>> methods;

    while (!check(TokenType::RBrace) && !check(TokenType::TokEof)) {
        if (check(TokenType::KwAsync) || check(TokenType::KwFunction) || check(TokenType::KwFn)) {
            methods.push_back(parseFunctionDecl(false));
        } else {
            Token fieldName = consume(TokenType::Identifier, "Expected field name or method declaration.");
            consume(TokenType::Colon, "Expected ':' after field name.");
            std::string typeStr = parseTypeSpec();
            fields.push_back({fieldName.lexeme, typeStr});
        }
        if (!match(TokenType::Comma)) {
            if (check(TokenType::RBrace)) break;
        }
    }
    consume(TokenType::RBrace, "Expected '}' after struct body.");
    match(TokenType::Semicolon);

    return std::make_unique<StructDeclASTNode>(nameTok.lexeme, std::move(fields), std::move(methods), std::move(genParams));
}

std::unique_ptr<EnumDeclASTNode> Parser::parseEnumDecl() {
    consume(TokenType::KwEnum, "Expected 'enum' keyword.");
    Token nameTok = consume(TokenType::Identifier, "Expected enum name.");
    auto genParams = parseGenericParams();
    consume(TokenType::LBrace, "Expected '{' to start enum body.");

    std::vector<EnumVariant> variants;
    while (!check(TokenType::RBrace) && !check(TokenType::TokEof)) {
        Token varTok = consume(TokenType::Identifier, "Expected enum variant name.");
        std::vector<std::string> payloadTypes;
        if (match(TokenType::LParen)) {
            if (!check(TokenType::RParen)) {
                do {
                    payloadTypes.push_back(parseTypeSpec());
                } while (match(TokenType::Comma));
            }
            consume(TokenType::RParen, "Expected ')' after variant payload types.");
        }
        variants.push_back({varTok.lexeme, std::move(payloadTypes)});
        if (!match(TokenType::Comma)) {
            if (check(TokenType::RBrace)) break;
        }
    }
    consume(TokenType::RBrace, "Expected '}' after enum body.");
    match(TokenType::Semicolon);

    return std::make_unique<EnumDeclASTNode>(nameTok.lexeme, std::move(genParams), std::move(variants));
}

std::unique_ptr<MatchASTNode> Parser::parseMatch() {
    consume(TokenType::KwMatch, "Expected 'match' keyword.");
    consume(TokenType::LParen, "Expected '(' after 'match'.");
    auto targetExpr = parseExpression();
    consume(TokenType::RParen, "Expected ')' after match target expression.");
    consume(TokenType::LBrace, "Expected '{' to start match cases.");

    std::vector<MatchCase> cases;
    while (!check(TokenType::RBrace) && !check(TokenType::TokEof)) {
        Token firstTok = consume(TokenType::Identifier, "Expected variant pattern identifier in match case.");
        std::string pattern = firstTok.lexeme;
        if (match(TokenType::Dot)) {
            Token secondTok = consume(TokenType::Identifier, "Expected variant name after '.' in match pattern.");
            pattern += "." + secondTok.lexeme;
        }

        std::vector<std::string> bindings;
        if (match(TokenType::LParen)) {
            if (!check(TokenType::RParen)) {
                do {
                    Token b = consume(TokenType::Identifier, "Expected binding variable name in match pattern.");
                    bindings.push_back(b.lexeme);
                } while (match(TokenType::Comma));
            }
            consume(TokenType::RParen, "Expected ')' after pattern bindings.");
        }

        consume(TokenType::Arrow, "Expected '=>' in match case.");

        std::unique_ptr<StatementNode> body;
        if (check(TokenType::LBrace)) {
            body = parseBlock();
        } else {
            auto expr = parseExpression();
            match(TokenType::Semicolon);
            body = std::make_unique<ExpressionStmtASTNode>(std::move(expr));
        }

        cases.push_back({pattern, std::move(bindings), std::move(body)});
        match(TokenType::Comma);
    }
    consume(TokenType::RBrace, "Expected '}' after match cases.");

    return std::make_unique<MatchASTNode>(std::move(targetExpr), std::move(cases));
}

std::unique_ptr<BlockASTNode> Parser::parseBlock() {
    consume(TokenType::LBrace, "Expected '{' to start block.");
    std::vector<std::unique_ptr<StatementNode>> statements;

    while (!check(TokenType::RBrace) && !check(TokenType::TokEof)) {
        auto stmt = parseStatement();
        if (stmt) statements.push_back(std::move(stmt));
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
    match(TokenType::Semicolon);

    return std::make_unique<ImportASTNode>(std::move(symbols), pathTok.lexeme);
}

std::unique_ptr<StatementNode> Parser::parseStatement() {
    if (check(TokenType::KwImport)) {
        return parseImportDecl();
    }
    if (check(TokenType::KwStruct)) {
        return parseStructDecl();
    }
    if (check(TokenType::KwEnum)) {
        return parseEnumDecl();
    }
    if (check(TokenType::KwMatch)) {
        auto matchNode = parseMatch();
        match(TokenType::Semicolon);
        return std::make_unique<ExpressionStmtASTNode>(std::move(matchNode));
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
    if (check(TokenType::PlusPlus) || check(TokenType::MinusMinus)) {
        bool isInc = check(TokenType::PlusPlus);
        advance();
        Token nameTok = consume(TokenType::Identifier, "Expected variable name after '++' or '--'.");
        consume(TokenType::Semicolon, "Expected ';' after increment/decrement.");
        auto varExpr = std::make_unique<VariableExprASTNode>(nameTok.lexeme);
        auto one = std::make_unique<NumberLiteralASTNode>((int64_t)1);
        auto binOp = std::make_unique<BinaryOpASTNode>(isInc ? "+" : "-", std::move(varExpr), std::move(one));
        return std::make_unique<AssignmentASTNode>(nameTok.lexeme, std::move(binOp));
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

    std::string typeName = "";
    if (match(TokenType::Colon)) {
        typeName = parseTypeSpec();
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

    Token identTok = consume(TokenType::Identifier, "Expected identifier statement.");

    if (match(TokenType::Equal)) {
        auto valExpr = parseExpression();
        consume(TokenType::Semicolon, "Expected ';' after assignment.");
        return std::make_unique<AssignmentASTNode>(identTok.lexeme, std::move(valExpr));
    }

    if (check(TokenType::PlusPlus) || check(TokenType::MinusMinus)) {
        bool isInc = check(TokenType::PlusPlus);
        advance();
        consume(TokenType::Semicolon, "Expected ';' after increment/decrement.");
        auto varExpr = std::make_unique<VariableExprASTNode>(identTok.lexeme);
        auto one = std::make_unique<NumberLiteralASTNode>((int64_t)1);
        auto binOp = std::make_unique<BinaryOpASTNode>(isInc ? "+" : "-", std::move(varExpr), std::move(one));
        return std::make_unique<AssignmentASTNode>(identTok.lexeme, std::move(binOp));
    }

    if (check(TokenType::PlusEqual) || check(TokenType::MinusEqual) ||
        check(TokenType::StarEqual) || check(TokenType::SlashEqual) ||
        check(TokenType::PercentEqual)) {
        std::string op;
        if (match(TokenType::PlusEqual)) op = "+";
        else if (match(TokenType::MinusEqual)) op = "-";
        else if (match(TokenType::StarEqual)) op = "*";
        else if (match(TokenType::SlashEqual)) op = "/";
        else if (match(TokenType::PercentEqual)) op = "%";

        auto rhsExpr = parseExpression();
        consume(TokenType::Semicolon, "Expected ';' after compound assignment.");
        auto lhsExpr = std::make_unique<VariableExprASTNode>(identTok.lexeme);
        auto binOp = std::make_unique<BinaryOpASTNode>(op, std::move(lhsExpr), std::move(rhsExpr));
        return std::make_unique<AssignmentASTNode>(identTok.lexeme, std::move(binOp));
    }

    std::unique_ptr<ExpressionNode> expr = std::make_unique<VariableExprASTNode>(identTok.lexeme);

    while (check(TokenType::Dot) || check(TokenType::LBracket) || check(TokenType::LParen) || check(TokenType::QuestionDot)) {
        if (match(TokenType::Dot)) {
            Token memberTok = consume(TokenType::Identifier, "Expected member name after '.'.");
            if (check(TokenType::LParen)) {
                consume(TokenType::LParen, "Expected '('.");
                std::vector<std::unique_ptr<ExpressionNode>> args;
                if (!check(TokenType::RParen)) {
                    do {
                        args.push_back(parseExpression());
                    } while (match(TokenType::Comma));
                }
                consume(TokenType::RParen, "Expected ')'.");
                expr = std::make_unique<MethodCallASTNode>(std::move(expr), memberTok.lexeme, std::move(args));
            } else if (match(TokenType::Equal)) {
                auto valExpr = parseExpression();
                consume(TokenType::Semicolon, "Expected ';' after member assignment.");
                return std::make_unique<MemberAssignmentASTNode>(std::move(expr), memberTok.lexeme, std::move(valExpr));
            } else {
                expr = std::make_unique<MemberAccessASTNode>(std::move(expr), memberTok.lexeme);
            }
        } else if (match(TokenType::LBracket)) {
            auto indexExpr = parseExpression();
            consume(TokenType::RBracket, "Expected ']' after index expression.");
            if (match(TokenType::Equal)) {
                auto valExpr = parseExpression();
                consume(TokenType::Semicolon, "Expected ';' after array element assignment.");
                return std::make_unique<ArrayAssignmentASTNode>(std::move(expr), std::move(indexExpr), std::move(valExpr));
            } else {
                expr = std::make_unique<ArrayAccessASTNode>(std::move(expr), std::move(indexExpr));
            }
        } else if (match(TokenType::LParen)) {
            std::vector<std::unique_ptr<ExpressionNode>> args;
            if (!check(TokenType::RParen)) {
                do {
                    args.push_back(parseExpression());
                } while (match(TokenType::Comma));
            }
            consume(TokenType::RParen, "Expected ')' after function arguments.");
            if (expr->getType() == NodeType::VariableExpr) {
                auto varNode = static_cast<VariableExprASTNode*>(expr.get());
                expr = std::make_unique<CallExprASTNode>(varNode->getName(), std::move(args));
            } else {
                throw ParseError("Complex function calls not supported yet.", curToken.line, curToken.column);
            }
        } else if (match(TokenType::QuestionDot)) {
            Token memberTok = consume(TokenType::Identifier, "Expected member name after '?.'.");
            expr = std::make_unique<OptionalChainASTNode>(std::move(expr), memberTok.lexeme);
        }
    }

    match(TokenType::Semicolon);
    return std::make_unique<ExpressionStmtASTNode>(std::move(expr));
}

std::unique_ptr<IfASTNode> Parser::parseIf() {
    consume(TokenType::KwIf, "Expected 'if'.");
    consume(TokenType::LParen, "Expected '(' after 'if'.");
    std::unique_ptr<ExpressionNode> condition = parseExpression();
    consume(TokenType::RParen, "Expected ')' after condition.");

    std::unique_ptr<BlockASTNode> thenBlock = parseBlock();
    std::unique_ptr<BlockASTNode> elseBlock = nullptr;

    if (match(TokenType::KwElse)) {
        if (check(TokenType::KwIf)) {
            auto nestedIf = parseIf();
            std::vector<std::unique_ptr<StatementNode>> stmts;
            stmts.push_back(std::move(nestedIf));
            elseBlock = std::make_unique<BlockASTNode>(std::move(stmts));
        } else {
            elseBlock = parseBlock();
        }
    }

    return std::make_unique<IfASTNode>(std::move(condition), std::move(thenBlock), std::move(elseBlock));
}

std::unique_ptr<WhileASTNode> Parser::parseWhile() {
    consume(TokenType::KwWhile, "Expected 'while'.");
    consume(TokenType::LParen, "Expected '(' after 'while'.");
    std::unique_ptr<ExpressionNode> condition = parseExpression();
    consume(TokenType::RParen, "Expected ')' after condition.");

    std::unique_ptr<BlockASTNode> body = parseBlock();
    return std::make_unique<WhileASTNode>(std::move(condition), std::move(body));
}

std::unique_ptr<ForASTNode> Parser::parseFor() {
    consume(TokenType::KwFor, "Expected 'for'.");
    consume(TokenType::LParen, "Expected '(' after 'for'.");

    if (check(TokenType::KwLet) || check(TokenType::KwConst) || check(TokenType::Identifier)) {
        Lexer savedLexer = lexer;
        Token savedToken = curToken;

        bool isConst = match(TokenType::KwConst);
        if (!isConst) match(TokenType::KwLet);

        if (check(TokenType::Identifier)) {
            Token itemTok = curToken;
            advance();

            if (check(TokenType::KwIn)) {
                advance();
                auto arrExpr = parseExpression();
                consume(TokenType::RParen, "Expected ')' after for-in expression.");
                auto body = parseBlock();

                std::string idxName = "__vit_idx_" + itemTok.lexeme;
                auto zeroVal = std::make_unique<NumberLiteralASTNode>((int64_t)0);
                auto initDecl = std::make_unique<VarDeclASTNode>(false, idxName, "int", std::move(zeroVal));

                auto idxVarUpd = std::make_unique<VariableExprASTNode>(idxName);
                auto oneExpr = std::make_unique<NumberLiteralASTNode>((int64_t)1);
                auto addExpr = std::make_unique<BinaryOpASTNode>("+", std::move(idxVarUpd), std::move(oneExpr));
                auto updateStmt = std::make_unique<AssignmentASTNode>(idxName, std::move(addExpr));

                std::vector<std::unique_ptr<ExpressionNode>> lenArgs;
                lenArgs.push_back(std::move(arrExpr));
                auto lenCall = std::make_unique<CallExprASTNode>("__vit_array_length", std::move(lenArgs));
                auto idxVarCond = std::make_unique<VariableExprASTNode>(idxName);
                auto condExpr = std::make_unique<BinaryOpASTNode>("<", std::move(idxVarCond), std::move(lenCall));

                auto forNode = std::make_unique<ForASTNode>(
                    std::move(initDecl), std::move(condExpr),
                    std::move(updateStmt), std::move(body)
                );
                return forNode;
            } else {
                lexer = savedLexer;
                curToken = savedToken;
            }
        } else {
            lexer = savedLexer;
            curToken = savedToken;
        }
    }

    std::unique_ptr<StatementNode> init = nullptr;
    if (!check(TokenType::Semicolon)) {
        if (check(TokenType::KwLet) || check(TokenType::KwConst)) {
            init = parseVarDecl();
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

    std::unique_ptr<ExpressionNode> cond = nullptr;
    if (!check(TokenType::Semicolon)) {
        cond = parseExpression();
    }
    consume(TokenType::Semicolon, "Expected ';' after for condition.");

    std::unique_ptr<StatementNode> update = nullptr;
    if (!check(TokenType::RParen)) {
        if (check(TokenType::Identifier)) {
            Token nameTok = curToken;
            advance();
            if (check(TokenType::PlusPlus)) {
                advance();
                auto varClone = std::make_unique<VariableExprASTNode>(nameTok.lexeme);
                auto one = std::make_unique<NumberLiteralASTNode>((int64_t)1);
                auto addExpr = std::make_unique<BinaryOpASTNode>("+", std::move(varClone), std::move(one));
                update = std::make_unique<AssignmentASTNode>(nameTok.lexeme, std::move(addExpr));
            } else if (check(TokenType::MinusMinus)) {
                advance();
                auto varClone = std::make_unique<VariableExprASTNode>(nameTok.lexeme);
                auto one = std::make_unique<NumberLiteralASTNode>((int64_t)1);
                auto subExpr = std::make_unique<BinaryOpASTNode>("-", std::move(varClone), std::move(one));
                update = std::make_unique<AssignmentASTNode>(nameTok.lexeme, std::move(subExpr));
            } else if (check(TokenType::PlusEqual) || check(TokenType::MinusEqual) ||
                       check(TokenType::StarEqual) || check(TokenType::SlashEqual) ||
                       check(TokenType::PercentEqual)) {
                std::string op;
                if (match(TokenType::PlusEqual)) op = "+";
                else if (match(TokenType::MinusEqual)) op = "-";
                else if (match(TokenType::StarEqual)) op = "*";
                else if (match(TokenType::SlashEqual)) op = "/";
                else if (match(TokenType::PercentEqual)) op = "%";

                auto rhsExpr = parseExpression();
                auto lhsExpr = std::make_unique<VariableExprASTNode>(nameTok.lexeme);
                auto binOp = std::make_unique<BinaryOpASTNode>(op, std::move(lhsExpr), std::move(rhsExpr));
                update = std::make_unique<AssignmentASTNode>(nameTok.lexeme, std::move(binOp));
            } else {
                consume(TokenType::Equal, "Expected '=' after variable name in for update.");
                auto valExpr = parseExpression();
                update = std::make_unique<AssignmentASTNode>(nameTok.lexeme, std::move(valExpr));
            }
        }
    }
    consume(TokenType::RParen, "Expected ')' to end for header.");

    std::unique_ptr<BlockASTNode> body = parseBlock();
    return std::make_unique<ForASTNode>(std::move(init), std::move(cond), std::move(update), std::move(body));
}

std::unique_ptr<BreakASTNode> Parser::parseBreak() {
    consume(TokenType::KwBreak, "Expected 'break'.");
    consume(TokenType::Semicolon, "Expected ';' after break.");
    return std::make_unique<BreakASTNode>();
}

std::unique_ptr<ContinueASTNode> Parser::parseContinue() {
    consume(TokenType::KwContinue, "Expected 'continue'.");
    consume(TokenType::Semicolon, "Expected ';' after continue.");
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

} // namespace vit
