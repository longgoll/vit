#ifndef VIT_PARSER_H
#define VIT_PARSER_H

#include "ast/AST.h"
#include "lexer/Lexer.h"
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

namespace vit {

class ParseError : public std::runtime_error {
public:
    size_t line;
    size_t column;

    ParseError(const std::string& msg, size_t l, size_t c)
        : std::runtime_error(msg), line(l), column(c) {}
};

class Parser {
private:
    Lexer lexer;
    Token curToken;

    void advance();
    bool check(TokenType type) const;
    bool match(TokenType type);
    Token consume(TokenType type, const std::string& errorMessage);

    // Grammars
    std::unique_ptr<FunctionDeclASTNode> parseFunctionDecl();
    std::vector<Parameter> parseParameterList();
    std::unique_ptr<BlockASTNode> parseBlock();
    std::unique_ptr<StatementNode> parseStatement();
    std::unique_ptr<VarDeclASTNode> parseVarDecl();
    std::unique_ptr<StatementNode> parseIdentifierStatement(); // Assignment or Print or Call
    std::unique_ptr<IfASTNode> parseIf();
    std::unique_ptr<WhileASTNode> parseWhile();
    std::unique_ptr<ForASTNode> parseFor();
    std::unique_ptr<BreakASTNode> parseBreak();
    std::unique_ptr<ContinueASTNode> parseContinue();
    std::unique_ptr<ReturnASTNode> parseReturn();
    std::unique_ptr<PrintASTNode> parsePrint();

    // Expression parsing (Precedence Climbing)
    std::unique_ptr<ExpressionNode> parseExpression();
    std::unique_ptr<ExpressionNode> parseLogicalOr();
    std::unique_ptr<ExpressionNode> parseLogicalAnd();
    std::unique_ptr<ExpressionNode> parseEquality();
    std::unique_ptr<ExpressionNode> parseRelational();
    std::unique_ptr<ExpressionNode> parseAdditive();
    std::unique_ptr<ExpressionNode> parseMultiplicative();
    std::unique_ptr<ExpressionNode> parseUnary();
    std::unique_ptr<ExpressionNode> parsePrimary();

public:
    explicit Parser(Lexer lex);

    std::unique_ptr<ProgramASTNode> parseProgram();
};

} // namespace vit

#endif // VIT_PARSER_H
