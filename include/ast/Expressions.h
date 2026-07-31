#ifndef VIT_EXPRESSIONS_H
#define VIT_EXPRESSIONS_H

#include "ASTNode.h"
#include "ASTVisitor.h"

namespace vit {

// Node representing a numeric literal (e.g. 10, 3.14)
class NumberLiteralASTNode : public ExpressionNode {
private:
    double value;

public:
    explicit NumberLiteralASTNode(double val) : value(val) {}

    double getValue() const { return value; }

    NodeType getType() const override { return NodeType::NumberLiteral; }
    void accept(ASTVisitor* visitor) override { visitor->visit(this); }
};

// Node representing a boolean literal (e.g. true, false)
class BooleanLiteralASTNode : public ExpressionNode {
private:
    bool value;

public:
    explicit BooleanLiteralASTNode(bool val) : value(val) {}

    bool getValue() const { return value; }

    NodeType getType() const override { return NodeType::BooleanLiteral; }
    void accept(ASTVisitor* visitor) override { visitor->visit(this); }
};

// Node representing a string literal (e.g. "hello")
class StringLiteralASTNode : public ExpressionNode {
private:
    std::string value;

public:
    explicit StringLiteralASTNode(std::string val) : value(std::move(val)) {}

    const std::string& getValue() const { return value; }

    NodeType getType() const override { return NodeType::StringLiteral; }
    void accept(ASTVisitor* visitor) override { visitor->visit(this); }
};

// Node representing an array literal (e.g. [10, 20, 30])
class ArrayLiteralASTNode : public ExpressionNode {
private:
    std::vector<std::unique_ptr<ExpressionNode>> elements;

public:
    explicit ArrayLiteralASTNode(std::vector<std::unique_ptr<ExpressionNode>> elems)
        : elements(std::move(elems)) {}

    const std::vector<std::unique_ptr<ExpressionNode>>& getElements() const { return elements; }

    NodeType getType() const override { return NodeType::ArrayLiteral; }
    void accept(ASTVisitor* visitor) override { visitor->visit(this); }
};

// Node representing a variable expression (e.g. x, y)
class VariableExprASTNode : public ExpressionNode {
private:
    std::string name;

public:
    explicit VariableExprASTNode(std::string varName) : name(std::move(varName)) {}

    const std::string& getName() const { return name; }

    NodeType getType() const override { return NodeType::VariableExpr; }
    void accept(ASTVisitor* visitor) override { visitor->visit(this); }
};

// Node representing member access (e.g. p.x)
class MemberAccessASTNode : public ExpressionNode {
private:
    std::unique_ptr<ExpressionNode> target;
    std::string member;

public:
    MemberAccessASTNode(std::unique_ptr<ExpressionNode> targetExpr, std::string memberName)
        : target(std::move(targetExpr)), member(std::move(memberName)) {}

    ExpressionNode* getTarget() const { return target.get(); }
    std::unique_ptr<ExpressionNode> takeTarget() { return std::move(target); }
    const std::string& getMember() const { return member; }

    NodeType getType() const override { return NodeType::MemberAccess; }
    void accept(ASTVisitor* visitor) override { visitor->visit(this); }
};

// Node representing array indexing (e.g. arr[i])
class ArrayAccessASTNode : public ExpressionNode {
private:
    std::unique_ptr<ExpressionNode> array;
    std::unique_ptr<ExpressionNode> index;

public:
    ArrayAccessASTNode(std::unique_ptr<ExpressionNode> arrExpr, std::unique_ptr<ExpressionNode> indexExpr)
        : array(std::move(arrExpr)), index(std::move(indexExpr)) {}

    ExpressionNode* getArray() const { return array.get(); }
    ExpressionNode* getIndex() const { return index.get(); }
    std::unique_ptr<ExpressionNode> takeArray() { return std::move(array); }
    std::unique_ptr<ExpressionNode> takeIndex() { return std::move(index); }

    NodeType getType() const override { return NodeType::ArrayAccess; }
    void accept(ASTVisitor* visitor) override { visitor->visit(this); }
};

// Node representing a unary operation (e.g. !flag, -x)
class UnaryOpASTNode : public ExpressionNode {
private:
    std::string op; // "!", "-"
    std::unique_ptr<ExpressionNode> operand;

public:
    UnaryOpASTNode(std::string opName, std::unique_ptr<ExpressionNode> expr)
        : op(std::move(opName)), operand(std::move(expr)) {}

    const std::string& getOp() const { return op; }
    ExpressionNode* getOperand() const { return operand.get(); }

    NodeType getType() const override { return NodeType::UnaryOp; }
    void accept(ASTVisitor* visitor) override { visitor->visit(this); }
};

// Node representing a binary operation (e.g. a + b, x > 0)
class BinaryOpASTNode : public ExpressionNode {
private:
    std::string op; // "+", "-", "*", "/", "<", ">", "==", "!="
    std::unique_ptr<ExpressionNode> left;
    std::unique_ptr<ExpressionNode> right;

public:
    BinaryOpASTNode(std::string opName,
                    std::unique_ptr<ExpressionNode> lhs,
                    std::unique_ptr<ExpressionNode> rhs)
        : op(std::move(opName)), left(std::move(lhs)), right(std::move(rhs)) {}

    const std::string& getOp() const { return op; }
    ExpressionNode* getLeft() const { return left.get(); }
    ExpressionNode* getRight() const { return right.get(); }

    NodeType getType() const override { return NodeType::BinaryOp; }
    void accept(ASTVisitor* visitor) override { visitor->visit(this); }
};

// Node representing a function call expression (e.g. add(x, y))
class CallExprASTNode : public ExpressionNode {
private:
    std::string callee;
    std::vector<std::unique_ptr<ExpressionNode>> args;

public:
    CallExprASTNode(std::string functionName,
                    std::vector<std::unique_ptr<ExpressionNode>> arguments)
        : callee(std::move(functionName)), args(std::move(arguments)) {}

    const std::string& getCallee() const { return callee; }
    const std::vector<std::unique_ptr<ExpressionNode>>& getArgs() const { return args; }

    NodeType getType() const override { return NodeType::CallExpr; }
    void accept(ASTVisitor* visitor) override { visitor->visit(this); }
};

} // namespace vit

#endif // VIT_EXPRESSIONS_H
