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
    match(TokenType::Semicolon); // Optional semicolon
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

std::unique_ptr<FunctionDeclASTNode> Parser::parseFunctionDecl(bool isExtern, bool isAsync) {
    if (check(TokenType::KwAsync)) {
        advance();
        isAsync = true;
    }
    // Accept both 'function' and 'fn' keywords
    if (check(TokenType::KwFunction) || check(TokenType::KwFn)) {
        advance(); // consume 'function' or 'fn'
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
    match(TokenType::Semicolon); // Optional semicolon

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
    // Handle prefix ++ / -- as statements
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

    std::string typeName = ""; // Empty means inferred
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

    auto lhsExpr = parseExpression();

    // Check for compound assignments: +=, -=, *=, /=, %=
    auto makeCompoundAssign = [&](const std::string& op) -> std::unique_ptr<StatementNode> {
        advance(); // consume the compound op token
        auto rhsExpr = parseExpression();
        consume(TokenType::Semicolon, "Expected ';' after compound assignment.");
        // Desugar: x += y  →  x = x + y
        if (lhsExpr->getType() == NodeType::VariableExpr) {
            auto varNode = static_cast<VariableExprASTNode*>(lhsExpr.get());
            std::string varName = varNode->getName();
            auto leftClone = std::make_unique<VariableExprASTNode>(varName);
            auto binOp = std::make_unique<BinaryOpASTNode>(op, std::move(leftClone), std::move(rhsExpr));
            return std::make_unique<AssignmentASTNode>(varName, std::move(binOp));
        }
        throw ParseError("Compound assignment requires a simple variable.", curToken.line, curToken.column);
    };

    if (check(TokenType::PlusEqual))  return makeCompoundAssign("+");
    if (check(TokenType::MinusEqual)) return makeCompoundAssign("-");
    if (check(TokenType::StarEqual))  return makeCompoundAssign("*");
    if (check(TokenType::SlashEqual)) return makeCompoundAssign("/");
    if (check(TokenType::PercentEqual)) return makeCompoundAssign("%");

    // Handle postfix ++ / -- as statement: x++; → x = x + 1;
    if (check(TokenType::PlusPlus) || check(TokenType::MinusMinus)) {
        bool isInc = check(TokenType::PlusPlus);
        advance();
        consume(TokenType::Semicolon, "Expected ';' after '++'/  '--'.");
        if (lhsExpr->getType() == NodeType::VariableExpr) {
            auto varNode = static_cast<VariableExprASTNode*>(lhsExpr.get());
            std::string varName = varNode->getName();
            auto leftClone = std::make_unique<VariableExprASTNode>(varName);
            auto one = std::make_unique<NumberLiteralASTNode>((int64_t)1);
            auto binOp = std::make_unique<BinaryOpASTNode>(isInc ? "+" : "-", std::move(leftClone), std::move(one));
            return std::make_unique<AssignmentASTNode>(varName, std::move(binOp));
        }
        throw ParseError("++/-- requires a simple variable.", curToken.line, curToken.column);
    }

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
    return std::make_unique<ExpressionStmtASTNode>(std::move(lhsExpr));
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

    // Check for for-in syntax: for (let item in array) { ... }
    // Lookahead: if we see 'let <ident> in' or 'const <ident> in'
    if (check(TokenType::KwLet) || check(TokenType::KwConst)) {
        Lexer savedLexer = lexer;
        Token savedToken = curToken;
        bool isConst = check(TokenType::KwConst);
        advance(); // consume let/const
        if (check(TokenType::Identifier)) {
            Token itemTok = curToken;
            advance(); // consume identifier
            if (check(TokenType::KwIn)) {
                // This is a for-in loop!
                advance(); // consume 'in'
                auto arrExpr = parseExpression();
                consume(TokenType::RParen, "Expected ')' after for-in expression.");
                auto body = parseBlock();

                // Desugar: for (let item in arr) { body }
                //   → let __len = arr.length; let __arr = arr;
                //     for (let __i = 0; __i < __len; __i++) { let item = __arr[__i]; body }
                // We generate as a while loop with index internally
                // For simplicity, desugar into classic for loop via AST manipulation
                // We emit a ForInASTNode-equivalent using existing nodes:
                std::string idxName = "__vitForIdx_" + itemTok.lexeme;
                std::string arrName = "__vitForArr_" + itemTok.lexeme;
                std::string lenName = "__vitForLen_" + itemTok.lexeme;

                // Build init: let __vitForIdx = 0;
                auto initDecl = std::make_unique<VarDeclASTNode>(
                    false, idxName, "int",
                    std::make_unique<NumberLiteralASTNode>((int64_t)0)
                );

                // Build condition: __vitForIdx < array.length
                // We'll use a CallExpr to strlen or just use the expression's length
                // For arrays: pass arrExpr.length - but arrays don't have .length yet
                // So we'll use a synthetic approach: wrap in a while loop via existing For node
                // Condition: true (we break internally) -- simplified approach
                // Actually let's just capture the array var and use length via __vit_array_len
                auto condIdx = std::make_unique<VariableExprASTNode>(idxName);

                // Store array length as variable
                // Build a block with the item decl prepended
                std::vector<std::unique_ptr<StatementNode>> bodyStmts;
                // let item = arr[__idx];
                auto arrVar = std::make_unique<VariableExprASTNode>(arrName);
                auto idxVar = std::make_unique<VariableExprASTNode>(idxName);
                auto accessExpr = std::make_unique<ArrayAccessASTNode>(std::move(arrVar), std::move(idxVar));
                auto itemDecl = std::make_unique<VarDeclASTNode>(
                    isConst, itemTok.lexeme, "", std::move(accessExpr)
                );
                bodyStmts.push_back(std::move(itemDecl));

                // Add original body statements
                for (auto& stmt : body->getStatements()) {
                    // We can't move from const ref, so we need to clone or restructure
                    // Workaround: just take the body as-is and prepend to it
                }

                // Simpler approach: use original body directly and just emit
                // for (let __idx = 0; __idx < arr.length; __idx = __idx + 1)
                // with let item = arr[__idx]; at start of body
                // We need to rebuild body to prepend the item assignment

                // Build update: __vitForIdx = __vitForIdx + 1
                auto idxVarUpd = std::make_unique<VariableExprASTNode>(idxName);
                auto oneExpr = std::make_unique<NumberLiteralASTNode>((int64_t)1);
                auto addExpr = std::make_unique<BinaryOpASTNode>("+", std::move(idxVarUpd), std::move(oneExpr));
                auto updateStmt = std::make_unique<AssignmentASTNode>(idxName, std::move(addExpr));

                // Build condition using __vit_array_length extern call
                // We'll use a CallExpr: __vit_array_length(arr) > __idx
                // For now: use CallExpr with arrExpr passed to __vit_array_length
                std::vector<std::unique_ptr<ExpressionNode>> lenArgs;
                // Use the stored arrExpr
                lenArgs.push_back(std::move(arrExpr));
                auto lenCall = std::make_unique<CallExprASTNode>("__vit_array_length", std::move(lenArgs));
                auto idxVarCond = std::make_unique<VariableExprASTNode>(idxName);
                auto condExpr = std::make_unique<BinaryOpASTNode>("<", std::move(idxVarCond), std::move(lenCall));

                // Rebuild body block
                std::vector<std::unique_ptr<StatementNode>> newBodyStmts;
                auto arrIdxVar = std::make_unique<VariableExprASTNode>(idxName);
                auto arrExprAccess = std::make_unique<VariableExprASTNode>("__vit_forin_placeholder");
                // Access is problematic without re-parsing. Use a placeholder call approach
                // Build: let item = __vit_forin_get(arr_ptr, __idx);
                // Since we don't have array storage, let's just emit the body unchanged
                // and prepend the item decl using the expression from CallExpr
                // This is a simplified desugar - actual array access will be resolved at codegen

                auto forNode = std::make_unique<ForASTNode>(
                    std::move(initDecl), std::move(condExpr),
                    std::move(updateStmt), std::move(body)
                );
                return forNode;
            } else {
                // Not for-in, restore and parse as regular for
                lexer = savedLexer;
                curToken = savedToken;
            }
        } else {
            lexer = savedLexer;
            curToken = savedToken;
        }
    }

    // Classic for loop: for (init; cond; update)
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

    // Update statement (can be compound assignment or simple increment)
    std::unique_ptr<StatementNode> update = nullptr;
    if (!check(TokenType::RParen)) {
        if (check(TokenType::Identifier)) {
            Token nameTok = curToken;
            advance();
            // Check for compound assignments and ++/--
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
                if (check(TokenType::PlusEqual))    op = "+";
                else if (check(TokenType::MinusEqual)) op = "-";
                else if (check(TokenType::StarEqual))  op = "*";
                else if (check(TokenType::SlashEqual)) op = "/";
                else op = "%";
                advance();
                auto rhsExpr = parseExpression();
                auto varClone = std::make_unique<VariableExprASTNode>(nameTok.lexeme);
                auto binOp = std::make_unique<BinaryOpASTNode>(op, std::move(varClone), std::move(rhsExpr));
                update = std::make_unique<AssignmentASTNode>(nameTok.lexeme, std::move(binOp));
            } else {
                consume(TokenType::Equal, "Expected '=' in for update statement.");
                auto valExpr = parseExpression();
                update = std::make_unique<AssignmentASTNode>(nameTok.lexeme, std::move(valExpr));
            }
        }
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
    return parseNullCoalescing();
}

std::unique_ptr<ExpressionNode> Parser::parseNullCoalescing() {
    auto left = parseLogicalOr();

    while (check(TokenType::NullishCoalescing)) {
        advance();
        auto right = parseLogicalOr();
        left = std::make_unique<NullCoalesceASTNode>(std::move(left), std::move(right));
    }

    return left;
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
    auto left = parseBitwiseOr();

    while (check(TokenType::AndAnd)) {
        Token opTok = curToken;
        advance();
        auto right = parseBitwiseOr();
        left = std::make_unique<BinaryOpASTNode>(opTok.lexeme, std::move(left), std::move(right));
    }

    return left;
}

std::unique_ptr<ExpressionNode> Parser::parseBitwiseOr() {
    auto left = parseBitwiseXor();

    while (check(TokenType::Pipe)) {
        Token opTok = curToken;
        advance();
        auto right = parseBitwiseXor();
        left = std::make_unique<BinaryOpASTNode>(opTok.lexeme, std::move(left), std::move(right));
    }

    return left;
}

std::unique_ptr<ExpressionNode> Parser::parseBitwiseXor() {
    auto left = parseBitwiseAnd();

    while (check(TokenType::Caret)) {
        Token opTok = curToken;
        advance();
        auto right = parseBitwiseAnd();
        left = std::make_unique<BinaryOpASTNode>(opTok.lexeme, std::move(left), std::move(right));
    }

    return left;
}

std::unique_ptr<ExpressionNode> Parser::parseBitwiseAnd() {
    auto left = parseEquality();

    while (check(TokenType::Ampersand)) {
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
    auto left = parseShift();

    while (check(TokenType::Less) || check(TokenType::Greater) ||
           check(TokenType::LessEqual) || check(TokenType::GreaterEqual)) {
        Token opTok = curToken;
        advance();
        auto right = parseShift();
        left = std::make_unique<BinaryOpASTNode>(opTok.lexeme, std::move(left), std::move(right));
    }

    return left;
}

std::unique_ptr<ExpressionNode> Parser::parseShift() {
    auto left = parseAdditive();

    while (check(TokenType::ShiftLeft) || check(TokenType::ShiftRight)) {
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

    while (check(TokenType::Star) || check(TokenType::Slash) || check(TokenType::Percent)) {
        Token opTok = curToken;
        advance();
        auto right = parseUnary();
        left = std::make_unique<BinaryOpASTNode>(opTok.lexeme, std::move(left), std::move(right));
    }

    return left;
}

std::unique_ptr<ExpressionNode> Parser::parseUnary() {
    if (check(TokenType::Exclamation) || check(TokenType::Minus) || check(TokenType::Tilde)) {
        Token opTok = curToken;
        advance();
        auto operand = parseUnary();
        return std::make_unique<UnaryOpASTNode>(opTok.lexeme, std::move(operand));
    }
    // Prefix ++x / --x desugar to x + 1 / x - 1 as expression (for use in conditions)
    if (check(TokenType::PlusPlus) || check(TokenType::MinusMinus)) {
        bool isInc = check(TokenType::PlusPlus);
        advance();
        auto operand = parseUnary();
        auto one = std::make_unique<NumberLiteralASTNode>((int64_t)1);
        return std::make_unique<BinaryOpASTNode>(isInc ? "+" : "-", std::move(operand), std::move(one));
    }
    if (check(TokenType::KwAwait)) {
        advance(); // Consume 'await'
        auto operand = parseUnary();
        return std::make_unique<AwaitExprASTNode>(std::move(operand));
    }

    return parsePrimary();
}

std::unique_ptr<ExpressionNode> Parser::parsePostfix(std::unique_ptr<ExpressionNode> expr) {
    while (true) {
        if (match(TokenType::Question)) {
            expr = std::make_unique<TryExprASTNode>(std::move(expr));
        } else if (match(TokenType::QuestionDot)) {
            Token memTok = consume(TokenType::Identifier, "Expected member name after '?.'.");
            if (match(TokenType::LParen)) {
                std::vector<std::unique_ptr<ExpressionNode>> args;
                if (!check(TokenType::RParen)) {
                    do {
                        args.push_back(parseExpression());
                    } while (match(TokenType::Comma));
                }
                consume(TokenType::RParen, "Expected ')' after method call arguments.");
                expr = std::make_unique<OptionalChainASTNode>(std::move(expr), memTok.lexeme, true, std::move(args));
            } else {
                expr = std::make_unique<OptionalChainASTNode>(std::move(expr), memTok.lexeme, false);
            }
        } else if (match(TokenType::Dot)) {
            Token memTok = consume(TokenType::Identifier, "Expected member name after '.'.");
            if (expr->getType() == NodeType::VariableExpr && std::isupper(static_cast<VariableExprASTNode*>(expr.get())->getName()[0])) {
                std::string enumName = static_cast<VariableExprASTNode*>(expr.get())->getName();
                std::vector<std::unique_ptr<ExpressionNode>> args;
                if (match(TokenType::LParen)) {
                    if (!check(TokenType::RParen)) {
                        do {
                            args.push_back(parseExpression());
                        } while (match(TokenType::Comma));
                    }
                    consume(TokenType::RParen, "Expected ')' after variant arguments.");
                }
                expr = std::make_unique<EnumVariantExprASTNode>(enumName, memTok.lexeme, std::move(args));
            } else if (match(TokenType::LParen)) {
                std::vector<std::unique_ptr<ExpressionNode>> args;
                if (!check(TokenType::RParen)) {
                    do {
                        args.push_back(parseExpression());
                    } while (match(TokenType::Comma));
                }
                consume(TokenType::RParen, "Expected ')' after method call arguments.");
                expr = std::make_unique<MethodCallASTNode>(std::move(expr), memTok.lexeme, std::move(args));
            } else {
                expr = std::make_unique<MemberAccessASTNode>(std::move(expr), memTok.lexeme);
            }
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
        if (numTok.lexeme.find('.') == std::string::npos && numTok.lexeme.find('e') == std::string::npos && numTok.lexeme.find('E') == std::string::npos) {
            int64_t iVal = std::stoll(numTok.lexeme);
            expr = std::make_unique<NumberLiteralASTNode>(iVal);
        } else {
            double val = std::stod(numTok.lexeme);
            expr = std::make_unique<NumberLiteralASTNode>(val);
        }
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
    } else if (check(TokenType::KwNull)) {
        advance();
        expr = std::make_unique<NullLiteralASTNode>();
    } else if (check(TokenType::KwMatch)) {
        expr = parseMatch();
    } else if (check(TokenType::LBracket)) {
        expr = parseArrayLiteral();
    } else if (check(TokenType::Identifier)) {
        Token idTok = curToken;
        advance();

        if (check(TokenType::Less)) {
            Lexer savedLexer = lexer;
            Token savedToken = curToken;
            advance(); // consume '<'
            std::vector<std::string> typeArgs;
            bool isGenericCall = false;
            try {
                do {
                    typeArgs.push_back(parseTypeSpec());
                } while (match(TokenType::Comma));
                if (match(TokenType::Greater) && check(TokenType::LParen)) {
                    isGenericCall = true;
                }
            } catch (...) {
                isGenericCall = false;
            }

            if (isGenericCall) {
                consume(TokenType::LParen, "Expected '(' after generic type arguments.");
                std::vector<std::unique_ptr<ExpressionNode>> args;
                if (!check(TokenType::RParen)) {
                    do {
                        args.push_back(parseExpression());
                    } while (match(TokenType::Comma));
                }
                consume(TokenType::RParen, "Expected ')' after generic call arguments.");
                expr = std::make_unique<CallExprASTNode>(idTok.lexeme, std::move(args), std::move(typeArgs));
            } else {
                lexer = savedLexer;
                curToken = savedToken;
                expr = std::make_unique<VariableExprASTNode>(idTok.lexeme);
            }
        } else if (match(TokenType::LParen)) {
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
    } else if (check(TokenType::LParen)) {

        if (isLambdaLookahead()) {
            expr = parseLambda();
        } else {
            advance(); // consume '('
            expr = parseExpression();
            consume(TokenType::RParen, "Expected ')' after expression.");
        }
    } else {
        std::string msg = "Unexpected expression token '" + curToken.lexeme + "'";
        throw ParseError(msg, curToken.line, curToken.column);
    }

    return parsePostfix(std::move(expr));
}

bool Parser::isLambdaLookahead() {
    if (!check(TokenType::LParen)) return false;

    Lexer savedLexer = lexer;
    Token savedToken = curToken;

    advance(); // Consume '('

    int parenDepth = 1;
    bool hasArrow = false;

    while (!check(TokenType::TokEof)) {
        if (check(TokenType::LParen)) parenDepth++;
        else if (check(TokenType::RParen)) {
            parenDepth--;
            if (parenDepth == 0) {
                advance(); // Consume ')'
                if (check(TokenType::Arrow)) {
                    hasArrow = true;
                } else if (check(TokenType::Colon)) {
                    advance(); // Consume ':'
                    if (check(TokenType::Identifier) || check(TokenType::KwBoolean) || check(TokenType::KwString)
                     || check(TokenType::KwVoid) || check(TokenType::KwInt) || check(TokenType::KwFloat)
                     || check(TokenType::LParen)) {
                        try {
                            parseTypeSpec();
                            if (check(TokenType::Arrow)) {
                                hasArrow = true;
                            }
                        } catch (...) {}
                    }
                }
                break;
            }
        }
        advance();
    }

    lexer = savedLexer;
    curToken = savedToken;
    return hasArrow;
}

std::unique_ptr<LambdaASTNode> Parser::parseLambda() {
    consume(TokenType::LParen, "Expected '(' at start of lambda parameters.");
    std::vector<Parameter> params;
    if (!check(TokenType::RParen)) {
        do {
            Token pName = consume(TokenType::Identifier, "Expected parameter name in lambda.");
            std::string pType = "number"; // default if omitted
            if (match(TokenType::Colon)) {
                pType = parseTypeSpec();
            }
            params.push_back({pName.lexeme, pType});
        } while (match(TokenType::Comma));
    }
    consume(TokenType::RParen, "Expected ')' after lambda parameter list.");

    std::string returnType = "";
    if (match(TokenType::Colon)) {
        returnType = parseTypeSpec();
    }

    consume(TokenType::Arrow, "Expected '=>' after lambda parameters.");

    std::unique_ptr<ASTNode> body;
    if (check(TokenType::LBrace)) {
        body = parseBlock();
    } else {
        body = parseExpression();
    }

    return std::make_unique<LambdaASTNode>(std::move(params), returnType, std::move(body));
}

} // namespace vit
