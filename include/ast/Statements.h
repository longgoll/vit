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

// Node representing struct member assignment (e.g. p.x = 10;)
class MemberAssignmentASTNode : public StatementNode {
private:
    std::unique_ptr<ExpressionNode> target;
    std::string member;
    std::unique_ptr<ExpressionNode> value;

public:
    MemberAssignmentASTNode(std::unique_ptr<ExpressionNode> targetExpr,
                            std::string memberName,
                            std::unique_ptr<ExpressionNode> valExpr)
        : target(std::move(targetExpr)), member(std::move(memberName)), value(std::move(valExpr)) {}

    ExpressionNode* getTarget() const { return target.get(); }
    const std::string& getMember() const { return member; }
    ExpressionNode* getValue() const { return value.get(); }

    NodeType getType() const override { return NodeType::MemberAssignment; }
    void accept(ASTVisitor* visitor) override { visitor->visit(this); }
};

// Node representing array element assignment (e.g. arr[i] = 5;)
class ArrayAssignmentASTNode : public StatementNode {
private:
    std::unique_ptr<ExpressionNode> array;
    std::unique_ptr<ExpressionNode> index;
    std::unique_ptr<ExpressionNode> value;

public:
    ArrayAssignmentASTNode(std::unique_ptr<ExpressionNode> arrExpr,
                           std::unique_ptr<ExpressionNode> indexExpr,
                           std::unique_ptr<ExpressionNode> valExpr)
        : array(std::move(arrExpr)), index(std::move(indexExpr)), value(std::move(valExpr)) {}

    ExpressionNode* getArray() const { return array.get(); }
    ExpressionNode* getIndex() const { return index.get(); }
    ExpressionNode* getValue() const { return value.get(); }

    NodeType getType() const override { return NodeType::ArrayAssignment; }
    void accept(ASTVisitor* visitor) override { visitor->visit(this); }
};

class FunctionDeclASTNode;

// Node representing a struct declaration (e.g. struct Point { x: number, y: number })
class StructDeclASTNode : public StatementNode {
private:
    std::string name;
    std::vector<std::pair<std::string, std::string>> fields; // name -> typeName
    std::vector<std::unique_ptr<FunctionDeclASTNode>> methods;
    std::vector<std::string> genericParams;

public:
    StructDeclASTNode(std::string structName,
                      std::vector<std::pair<std::string, std::string>> structFields,
                      std::vector<std::unique_ptr<FunctionDeclASTNode>> structMethods = {},
                      std::vector<std::string> genParams = {})
        : name(std::move(structName)),
          fields(std::move(structFields)),
          methods(std::move(structMethods)),
          genericParams(std::move(genParams)) {}

    const std::string& getName() const { return name; }
    const std::vector<std::pair<std::string, std::string>>& getFields() const { return fields; }
    const std::vector<std::unique_ptr<FunctionDeclASTNode>>& getMethods() const { return methods; }
    const std::vector<std::string>& getGenericParams() const { return genericParams; }

    NodeType getType() const override { return NodeType::StructDecl; }
    void accept(ASTVisitor* visitor) override { visitor->visit(this); }
};

struct EnumVariant {
    std::string name;
    std::vector<std::string> payloadTypes; // Empty if no payload (e.g., Option.None)
};

// Node representing an enum declaration (e.g. enum Option<T> { Some(val: T), None })
class EnumDeclASTNode : public StatementNode {
private:
    std::string name;
    std::vector<std::string> genericParams;
    std::vector<EnumVariant> variants;

public:
    EnumDeclASTNode(std::string enumName,
                    std::vector<std::string> genParams,
                    std::vector<EnumVariant> enumVariants)
        : name(std::move(enumName)),
          genericParams(std::move(genParams)),
          variants(std::move(enumVariants)) {}

    const std::string& getName() const { return name; }
    const std::vector<std::string>& getGenericParams() const { return genericParams; }
    const std::vector<EnumVariant>& getVariants() const { return variants; }

    NodeType getType() const override { return NodeType::EnumDecl; }
    void accept(ASTVisitor* visitor) override { visitor->visit(this); }
};


// Node representing an import statement (e.g. import { sqrt, cos } from "math"; or import "math";)
class ImportASTNode : public StatementNode {
private:
    std::vector<std::string> symbols;
    std::string modulePath;

public:
    ImportASTNode(std::vector<std::string> importedSymbols, std::string path)
        : symbols(std::move(importedSymbols)), modulePath(std::move(path)) {}

    const std::vector<std::string>& getSymbols() const { return symbols; }
    const std::string& getModulePath() const { return modulePath; }

    NodeType getType() const override { return NodeType::ImportDecl; }
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

// Node representing a while loop (e.g. while (cond) { ... })
class WhileASTNode : public StatementNode {
private:
    std::unique_ptr<ExpressionNode> condition;
    std::unique_ptr<BlockASTNode> body;

public:
    WhileASTNode(std::unique_ptr<ExpressionNode> condExpr,
                 std::unique_ptr<BlockASTNode> bodyBlock)
        : condition(std::move(condExpr)), body(std::move(bodyBlock)) {}

    ExpressionNode* getCondition() const { return condition.get(); }
    BlockASTNode* getBody() const { return body.get(); }

    NodeType getType() const override { return NodeType::While; }
    void accept(ASTVisitor* visitor) override { visitor->visit(this); }
};

// Node representing a for loop (e.g. for (let i = 0; i < 10; i = i + 1) { ... })
class ForASTNode : public StatementNode {
private:
    std::unique_ptr<StatementNode> init;       // VarDecl or Assignment or nullptr
    std::unique_ptr<ExpressionNode> condition; // Expression or nullptr
    std::unique_ptr<StatementNode> update;     // Assignment or nullptr
    std::unique_ptr<BlockASTNode> body;

public:
    ForASTNode(std::unique_ptr<StatementNode> initStmt,
               std::unique_ptr<ExpressionNode> condExpr,
               std::unique_ptr<StatementNode> updateStmt,
               std::unique_ptr<BlockASTNode> bodyBlock)
        : init(std::move(initStmt)),
          condition(std::move(condExpr)),
          update(std::move(updateStmt)),
          body(std::move(bodyBlock)) {}

    StatementNode* getInit() const { return init.get(); }
    ExpressionNode* getCondition() const { return condition.get(); }
    StatementNode* getUpdate() const { return update.get(); }
    BlockASTNode* getBody() const { return body.get(); }

    NodeType getType() const override { return NodeType::For; }
    void accept(ASTVisitor* visitor) override { visitor->visit(this); }
};

// Node representing a break statement (break;)
class BreakASTNode : public StatementNode {
public:
    BreakASTNode() = default;

    NodeType getType() const override { return NodeType::Break; }
    void accept(ASTVisitor* visitor) override { visitor->visit(this); }
};

// Node representing a continue statement (continue;)
class ContinueASTNode : public StatementNode {
public:
    ContinueASTNode() = default;

    NodeType getType() const override { return NodeType::Continue; }
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

// Node representing an expression statement (e.g. p.scale(2.0); or doSomething();)
class ExpressionStmtASTNode : public StatementNode {
private:
    std::unique_ptr<ExpressionNode> expression;

public:
    explicit ExpressionStmtASTNode(std::unique_ptr<ExpressionNode> expr)
        : expression(std::move(expr)) {}

    ExpressionNode* getExpression() const { return expression.get(); }

    NodeType getType() const override { return NodeType::ExpressionStmt; }
    void accept(ASTVisitor* visitor) override { visitor->visit(this); }
};

// Node representing a type alias (e.g. type Mapper = (x: number) => number;)
class TypeAliasASTNode : public StatementNode {
private:
    std::string aliasName;
    std::string typeSpec;

public:
    TypeAliasASTNode(std::string alias, std::string targetType)
        : aliasName(std::move(alias)), typeSpec(std::move(targetType)) {}

    const std::string& getAliasName() const { return aliasName; }
    const std::string& getTypeSpec() const { return typeSpec; }

    NodeType getType() const override { return NodeType::TypeAlias; }
    void accept(ASTVisitor* visitor) override { visitor->visit(this); }
};

} // namespace vit

#endif // VIT_STATEMENTS_H
