#ifndef VIT_FUNCTIONS_H
#define VIT_FUNCTIONS_H

#include "ASTNode.h"
#include "ASTVisitor.h"
#include "Statements.h"

namespace vit {

// Node representing a function declaration (e.g. function add(a: number, b: number): number { ... })
class FunctionDeclASTNode : public ASTNode {
private:
    std::string name;
    std::vector<Parameter> params;
    std::string returnType;
    std::unique_ptr<BlockASTNode> body;
    bool isExtern = false;
    bool isAsync = false;
    std::vector<std::string> genericParams;

public:
    FunctionDeclASTNode(std::string funcName,
                        std::vector<Parameter> parameters,
                        std::string retType,
                        std::unique_ptr<BlockASTNode> funcBody,
                        bool externFlag = false,
                        std::vector<std::string> genParams = {},
                        bool asyncFlag = false)
        : name(std::move(funcName)),
          params(std::move(parameters)),
          returnType(std::move(retType)),
          body(std::move(funcBody)),
          isExtern(externFlag),
          isAsync(asyncFlag),
          genericParams(std::move(genParams)) {}

    const std::string& getName() const { return name; }
    const std::vector<Parameter>& getParams() const { return params; }
    const std::string& getReturnType() const { return returnType; }
    BlockASTNode* getBody() const { return body.get(); }
    bool getIsExtern() const { return isExtern; }
    bool getIsAsync() const { return isAsync; }
    const std::vector<std::string>& getGenericParams() const { return genericParams; }

    NodeType getType() const override { return NodeType::FunctionDecl; }
    void accept(ASTVisitor* visitor) override { visitor->visit(this); }
};

// Root node for an entire source program file
class ProgramASTNode : public ASTNode {
private:
    std::vector<std::unique_ptr<FunctionDeclASTNode>> functions;
    std::vector<std::unique_ptr<StatementNode>> topLevelStatements;

public:
    ProgramASTNode(std::vector<std::unique_ptr<FunctionDeclASTNode>> funcs,
                   std::vector<std::unique_ptr<StatementNode>> topStmts = {})
        : functions(std::move(funcs)), topLevelStatements(std::move(topStmts)) {}

    std::vector<std::unique_ptr<FunctionDeclASTNode>>& getFunctions() {
        return functions;
    }
    const std::vector<std::unique_ptr<FunctionDeclASTNode>>& getFunctions() const {
        return functions;
    }
    std::vector<std::unique_ptr<StatementNode>>& getTopLevelStatements() {
        return topLevelStatements;
    }
    const std::vector<std::unique_ptr<StatementNode>>& getTopLevelStatements() const {
        return topLevelStatements;
    }

    NodeType getType() const override { return NodeType::Program; }
    void accept(ASTVisitor* visitor) override { visitor->visit(this); }
};

} // namespace vit

#endif // VIT_FUNCTIONS_H
