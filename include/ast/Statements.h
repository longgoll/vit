#ifndef VIT_STATEMENTS_H
#define VIT_STATEMENTS_H

#include "ASTNode.h"
#include "ASTVisitor.h"
#include "Expressions.h"

namespace vit {

// Node representing a variable declaration (e.g. let x = 10; or const y = 20;)
class VarDeclASTNode : public StatementNode {
private:
    bool isConst;
    std::string name;
    std::string typeName; // e.g. "number"
    std::unique_ptr<ExpressionNode> initializer;

public:
    VarDeclASTNode(bool isConstant,
                   std::string varName,
                   std::string type,
                   std::unique_ptr<ExpressionNode> initExpr)
        : isConst(isConstant),
          name(std::move(varName)),
          typeName(std::move(type)),
          initializer(std::move(initExpr)) {}

    bool getIsConst() const { return isConst; }
    const std::string& getName() const { return name; }
    const std::string& getTypeName() const { return typeName; }
    ExpressionNode* getInitializer() const { return initializer.get(); }

    NodeType getType() const override { return NodeType::VarDecl; }
    void accept(ASTVisitor* visitor) override { visitor->visit(this); }
};

// Node representing variable assignment (e.g. x = y + 5;)
class AssignmentASTNode : public StatementNode {
private:
    std::string name;
    std::unique_ptr<ExpressionNode> value;

public:
    AssignmentASTNode(std::string varName, std::unique_ptr<ExpressionNode> valExpr)
        : name(std::move(varName)), value(std::move(valExpr)) {}

    const std::string& getName() const { return name; }
    ExpressionNode* getValue() const { return value.get(); }

    NodeType getType() const override { return NodeType::Assignment; }
    void accept(ASTVisitor* visitor) override { visitor->visit(this); }
};

// Node representing a block of statements (e.g. { stmt1; stmt2; })
class BlockASTNode : public StatementNode {
private:
    std::vector<std::unique_ptr<StatementNode>> statements;

public:
    explicit BlockASTNode(std::vector<std::unique_ptr<StatementNode>> stmts)
        : statements(std::move(stmts)) {}

    const std::vector<std::unique_ptr<StatementNode>>& getStatements() const {
        return statements;
    }

    NodeType getType() const override { return NodeType::Block; }
    void accept(ASTVisitor* visitor) override { visitor->visit(this); }
};

// Node representing an if/else conditional (e.g. if (cond) { ... } else { ... })
class IfASTNode : public StatementNode {
private:
    std::unique_ptr<ExpressionNode> condition;
    std::unique_ptr<BlockASTNode> thenBlock;
    std::unique_ptr<BlockASTNode> elseBlock; // Can be nullptr

public:
    IfASTNode(std::unique_ptr<ExpressionNode> condExpr,
              std::unique_ptr<BlockASTNode> thenStmtBlock,
              std::unique_ptr<BlockASTNode> elseStmtBlock = nullptr)
        : condition(std::move(condExpr)),
          thenBlock(std::move(thenStmtBlock)),
          elseBlock(std::move(elseStmtBlock)) {}

    ExpressionNode* getCondition() const { return condition.get(); }
    BlockASTNode* getThenBlock() const { return thenBlock.get(); }
    BlockASTNode* getElseBlock() const { return elseBlock.get(); }

    NodeType getType() const override { return NodeType::If; }
    void accept(ASTVisitor* visitor) override { visitor->visit(this); }
};

// Node representing a return statement (e.g. return a + b;)
class ReturnASTNode : public StatementNode {
private:
    std::unique_ptr<ExpressionNode> value; // Can be nullptr for void return

public:
    explicit ReturnASTNode(std::unique_ptr<ExpressionNode> returnVal = nullptr)
        : value(std::move(returnVal)) {}

    ExpressionNode* getValue() const { return value.get(); }

    NodeType getType() const override { return NodeType::Return; }
    void accept(ASTVisitor* visitor) override { visitor->visit(this); }
};

// Node representing built-in print statement (e.g. print(result);)
class PrintASTNode : public StatementNode {
private:
    std::unique_ptr<ExpressionNode> expression;

public:
    explicit PrintASTNode(std::unique_ptr<ExpressionNode> expr)
        : expression(std::move(expr)) {}

    ExpressionNode* getExpression() const { return expression.get(); }

    NodeType getType() const override { return NodeType::Print; }
    void accept(ASTVisitor* visitor) override { visitor->visit(this); }
};

} // namespace vit

#endif // VIT_STATEMENTS_H
