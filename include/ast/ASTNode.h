#ifndef VIT_AST_NODE_H
#define VIT_AST_NODE_H

#include <memory>
#include <string>
#include <vector>

namespace vit {

// Forward declaration of Visitor
class ASTVisitor;

struct Parameter {
    std::string name;
    std::string typeName;
};

enum class NodeType {
    Program,
    FunctionDecl,
    Block,
    VarDecl,
    Assignment,
    MemberAssignment,
    ArrayAssignment,
    If,
    While,
    For,
    Break,
    Continue,
    Return,
    Print,
    StructDecl,
    ImportDecl,
    NumberLiteral,
    BooleanLiteral,
    StringLiteral,
    ArrayLiteral,
    VariableExpr,
    MemberAccess,
    ArrayAccess,
    UnaryOp,
    BinaryOp,
    CallExpr,
    MethodCall,
    Lambda,
    TypeAlias,
    EnumDecl,
    EnumVariantExpr,
    Match,
    ExpressionStmt,
    NullLiteral,
    TryExpr,
    OptionalChain,
    NullCoalesce,
    AwaitExpr
};

// Base class for all AST nodes
class ASTNode {
public:
    virtual ~ASTNode() = default;
    virtual NodeType getType() const = 0;
    virtual void accept(ASTVisitor* visitor) = 0;
};

// Base class for Expression nodes (produces a value)
class ExpressionNode : public ASTNode {
public:
    virtual ~ExpressionNode() = default;
};

// Base class for Statement nodes (performs an action)
class StatementNode : public ASTNode {
public:
    virtual ~StatementNode() = default;
};

} // namespace vit

#endif // VIT_AST_NODE_H
