#ifndef VIT_EXPRESSIONS_H
#define VIT_EXPRESSIONS_H

#include <cstdint>
#include "ASTNode.h"
#include "ASTVisitor.h"

namespace vit {

// Node representing a numeric literal (e.g. 10, 3.14)
class NumberLiteralASTNode : public ExpressionNode {
private:
    double value;
    int64_t intVal = 0;
    bool isInt = false;

public:
    explicit NumberLiteralASTNode(double val) : value(val), isInt(false) {}
    explicit NumberLiteralASTNode(int64_t val) : value((double)val), intVal(val), isInt(true) {}
    NumberLiteralASTNode(double val, int64_t iVal, bool isInteger)
        : value(val), intVal(iVal), isInt(isInteger) {}

    double getValue() const { return value; }
    int64_t getIntValue() const { return isInt ? intVal : (int64_t)value; }
    bool isInteger() const { return isInt; }

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

// Node representing a function call expression (e.g. add(x, y) or identity<string>("VIT"))
class CallExprASTNode : public ExpressionNode {
private:
    std::string callee;
    std::vector<std::unique_ptr<ExpressionNode>> args;
    std::vector<std::string> typeArgs;

public:
    CallExprASTNode(std::string functionName,
                    std::vector<std::unique_ptr<ExpressionNode>> arguments,
                    std::vector<std::string> typeArguments = {})
        : callee(std::move(functionName)),
          args(std::move(arguments)),
          typeArgs(std::move(typeArguments)) {}

    const std::string& getCallee() const { return callee; }
    void setCallee(const std::string& newCallee) { callee = newCallee; }
    const std::vector<std::unique_ptr<ExpressionNode>>& getArgs() const { return args; }
    const std::vector<std::string>& getTypeArgs() const { return typeArgs; }

    NodeType getType() const override { return NodeType::CallExpr; }
    void accept(ASTVisitor* visitor) override { visitor->visit(this); }
};


// Node representing a method call expression (e.g. obj.distance())
class MethodCallASTNode : public ExpressionNode {
private:
    std::unique_ptr<ExpressionNode> target;
    std::string method;
    std::vector<std::unique_ptr<ExpressionNode>> args;

public:
    MethodCallASTNode(std::unique_ptr<ExpressionNode> targetExpr,
                      std::string methodName,
                      std::vector<std::unique_ptr<ExpressionNode>> arguments)
        : target(std::move(targetExpr)),
          method(std::move(methodName)),
          args(std::move(arguments)) {}

    ExpressionNode* getTarget() const { return target.get(); }
    const std::string& getMethod() const { return method; }
    const std::vector<std::unique_ptr<ExpressionNode>>& getArgs() const { return args; }

    NodeType getType() const override { return NodeType::MethodCall; }
    void accept(ASTVisitor* visitor) override { visitor->visit(this); }
};

// Node representing a lambda / arrow function expression (e.g. (x: number): number => x * 2)
class LambdaASTNode : public ExpressionNode {
private:
    std::vector<Parameter> params;
    std::string returnType;
    std::unique_ptr<ASTNode> body;

public:
    LambdaASTNode(std::vector<Parameter> parameters,
                  std::string retType,
                  std::unique_ptr<ASTNode> funcBody)
        : params(std::move(parameters)),
          returnType(std::move(retType)),
          body(std::move(funcBody)) {}

    const std::vector<Parameter>& getParams() const { return params; }
    const std::string& getReturnType() const { return returnType; }
    void setReturnType(const std::string& type) { returnType = std::move(type); }
    ASTNode* getBody() const { return body.get(); }

    NodeType getType() const override { return NodeType::Lambda; }
    void accept(ASTVisitor* visitor) override { visitor->visit(this); }
};

// Node representing an enum variant construction expression (e.g. Option.Some(42.0) or Option.None)
class EnumVariantExprASTNode : public ExpressionNode {
private:
    std::string enumName;
    std::string variantName;
    std::vector<std::unique_ptr<ExpressionNode>> args;

public:
    EnumVariantExprASTNode(std::string enumIdent,
                           std::string variantIdent,
                           std::vector<std::unique_ptr<ExpressionNode>> arguments = {})
        : enumName(std::move(enumIdent)),
          variantName(std::move(variantIdent)),
          args(std::move(arguments)) {}

    const std::string& getEnumName() const { return enumName; }
    const std::string& getVariantName() const { return variantName; }
    const std::vector<std::unique_ptr<ExpressionNode>>& getArgs() const { return args; }

    NodeType getType() const override { return NodeType::EnumVariantExpr; }
    void accept(ASTVisitor* visitor) override { visitor->visit(this); }
};

struct MatchCase {
    std::string variantPattern; // e.g. "Option.Some" or "Some"
    std::vector<std::string> bindings; // e.g. ["val"]
    std::unique_ptr<StatementNode> body;
};

// Node representing pattern matching (e.g. match (expr) { Option.Some(val) => { ... }, Option.None => { ... } })
class MatchASTNode : public ExpressionNode {
private:
    std::unique_ptr<ExpressionNode> target;
    std::vector<MatchCase> cases;

public:
    MatchASTNode(std::unique_ptr<ExpressionNode> targetExpr, std::vector<MatchCase> matchCases)
        : target(std::move(targetExpr)), cases(std::move(matchCases)) {}

    ExpressionNode* getTarget() const { return target.get(); }
    const std::vector<MatchCase>& getCases() const { return cases; }
    std::vector<MatchCase>& getCases() { return cases; }

    NodeType getType() const override { return NodeType::Match; }
    void accept(ASTVisitor* visitor) override { visitor->visit(this); }
};

// Node representing a null literal (e.g. null)
class NullLiteralASTNode : public ExpressionNode {
public:
    NullLiteralASTNode() = default;

    NodeType getType() const override { return NodeType::NullLiteral; }
    void accept(ASTVisitor* visitor) override { visitor->visit(this); }
};

// Node representing try operator (e.g. expr?)
class TryExprASTNode : public ExpressionNode {
private:
    std::unique_ptr<ExpressionNode> expr;

public:
    explicit TryExprASTNode(std::unique_ptr<ExpressionNode> e)
        : expr(std::move(e)) {}

    ExpressionNode* getExpr() const { return expr.get(); }

    NodeType getType() const override { return NodeType::TryExpr; }
    void accept(ASTVisitor* visitor) override { visitor->visit(this); }
};

// Node representing optional chaining (e.g. target?.member or target?.method(args))
class OptionalChainASTNode : public ExpressionNode {
private:
    std::unique_ptr<ExpressionNode> target;
    std::string member;
    bool isMethodCall;
    std::vector<std::unique_ptr<ExpressionNode>> args;

public:
    OptionalChainASTNode(std::unique_ptr<ExpressionNode> targetExpr, std::string memberName, bool isCall = false, std::vector<std::unique_ptr<ExpressionNode>> arguments = {})
        : target(std::move(targetExpr)), member(std::move(memberName)), isMethodCall(isCall), args(std::move(arguments)) {}

    ExpressionNode* getTarget() const { return target.get(); }
    const std::string& getMember() const { return member; }
    bool getIsMethodCall() const { return isMethodCall; }
    const std::vector<std::unique_ptr<ExpressionNode>>& getArgs() const { return args; }

    NodeType getType() const override { return NodeType::OptionalChain; }
    void accept(ASTVisitor* visitor) override { visitor->visit(this); }
};

// Node representing nullish coalescing (e.g. left ?? right)
class NullCoalesceASTNode : public ExpressionNode {
private:
    std::unique_ptr<ExpressionNode> left;
    std::unique_ptr<ExpressionNode> right;

public:
    NullCoalesceASTNode(std::unique_ptr<ExpressionNode> lhs, std::unique_ptr<ExpressionNode> rhs)
        : left(std::move(lhs)), right(std::move(rhs)) {}

    ExpressionNode* getLeft() const { return left.get(); }
    ExpressionNode* getRight() const { return right.get(); }

    NodeType getType() const override { return NodeType::NullCoalesce; }
    void accept(ASTVisitor* visitor) override { visitor->visit(this); }
};

// Node representing an await expression (e.g. await fetchUserData(42))
class AwaitExprASTNode : public ExpressionNode {
private:
    std::unique_ptr<ExpressionNode> expr;

public:
    explicit AwaitExprASTNode(std::unique_ptr<ExpressionNode> targetExpr)
        : expr(std::move(targetExpr)) {}

    ExpressionNode* getExpr() const { return expr.get(); }

    NodeType getType() const override { return NodeType::AwaitExpr; }
    void accept(ASTVisitor* visitor) override { visitor->visit(this); }
};

} // namespace vit

#endif // VIT_EXPRESSIONS_H

