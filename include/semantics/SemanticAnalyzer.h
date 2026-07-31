#ifndef VIT_SEMANTIC_ANALYZER_H
#define VIT_SEMANTIC_ANALYZER_H

#include "ast/ASTVisitor.h"
#include "ast/AST.h"
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

namespace vit {

class SemanticError : public std::runtime_error {
public:
    explicit SemanticError(const std::string& msg) : std::runtime_error(msg) {}
};

struct SymbolInfo {
    std::string name;
    std::string typeName; // "number", "boolean", "string", "void"
    bool isConst;
};

class SemanticAnalyzer : public ASTVisitor {
private:
    std::vector<std::unordered_map<std::string, SymbolInfo>> scopeStack;
    std::unordered_map<std::string, std::pair<std::string, std::vector<std::string>>> functionTable; // funcName -> (returnType, paramTypes)
    std::unordered_map<std::string, std::vector<std::pair<std::string, std::string>>> structTable; // structName -> fields (name, type)
    std::unordered_map<std::string, std::unordered_map<std::string, std::pair<std::string, std::vector<std::string>>>> structMethodsTable; // structName -> (methodName -> (retType, paramTypes))
    std::unordered_map<std::string, std::string> typeAliasTable; // alias -> concreteType
    std::unordered_map<std::string, std::vector<EnumVariant>> enumTable; // enumName -> variants

    std::string currentReturnType;
    std::string lastInferredType;
    int loopDepth = 0;
    bool inAsyncScope = false;
    bool hasError = false;
    std::vector<std::string> errorMessages;

    void enterScope();
    void exitScope();
    bool declareVariable(const std::string& name, const std::string& typeName, bool isConst);
    const SymbolInfo* lookupVariable(const std::string& name) const;
    std::string resolveType(const std::string& typeName) const;
    void reportError(const std::string& msg);

public:
    SemanticAnalyzer() = default;

    bool analyze(ProgramASTNode* program);
    const std::vector<std::string>& getErrors() const { return errorMessages; }

    void visit(ProgramASTNode* node) override;
    void visit(FunctionDeclASTNode* node) override;
    void visit(BlockASTNode* node) override;
    void visit(VarDeclASTNode* node) override;
    void visit(AssignmentASTNode* node) override;
    void visit(MemberAssignmentASTNode* node) override;
    void visit(ArrayAssignmentASTNode* node) override;
    void visit(IfASTNode* node) override;
    void visit(WhileASTNode* node) override;
    void visit(ForASTNode* node) override;
    void visit(BreakASTNode* node) override;
    void visit(ContinueASTNode* node) override;
    void visit(ReturnASTNode* node) override;
    void visit(PrintASTNode* node) override;
    void visit(StructDeclASTNode* node) override;
    void visit(ImportASTNode* node) override;
    void visit(TypeAliasASTNode* node) override;
    void visit(NumberLiteralASTNode* node) override;
    void visit(BooleanLiteralASTNode* node) override;
    void visit(StringLiteralASTNode* node) override;
    void visit(ArrayLiteralASTNode* node) override;
    void visit(VariableExprASTNode* node) override;
    void visit(MemberAccessASTNode* node) override;
    void visit(ArrayAccessASTNode* node) override;
    void visit(UnaryOpASTNode* node) override;
    void visit(BinaryOpASTNode* node) override;
    void visit(CallExprASTNode* node) override;
    void visit(MethodCallASTNode* node) override;
    void visit(LambdaASTNode* node) override;
    void visit(EnumDeclASTNode* node) override;
    void visit(EnumVariantExprASTNode* node) override;
    void visit(MatchASTNode* node) override;
    void visit(ExpressionStmtASTNode* node) override;
    void visit(NullLiteralASTNode* node) override;
    void visit(TryExprASTNode* node) override;
    void visit(OptionalChainASTNode* node) override;
    void visit(NullCoalesceASTNode* node) override;
    void visit(AwaitExprASTNode* node) override;
};


} // namespace vit

#endif // VIT_SEMANTIC_ANALYZER_H
