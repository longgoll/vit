#include "parser/Parser.h"
#include <iostream>

namespace vit {

// Expression parsing implementation for Parser

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
    // Prefix ++x / --x desugar to x + 1 / x - 1 as expression
    if (check(TokenType::PlusPlus) || check(TokenType::MinusMinus)) {
        bool isInc = check(TokenType::PlusPlus);
        advance();
        auto operand = parseUnary();
        auto one = std::make_unique<NumberLiteralASTNode>((int64_t)1);
        return std::make_unique<BinaryOpASTNode>(isInc ? "+" : "-", std::move(operand), std::move(one));
    }
    if (check(TokenType::KwAwait)) {
        advance();
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
        if (numTok.lexeme.find('.') == std::string::npos &&
            numTok.lexeme.find('e') == std::string::npos &&
            numTok.lexeme.find('E') == std::string::npos) {
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
