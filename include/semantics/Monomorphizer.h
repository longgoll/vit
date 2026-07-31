#ifndef VIT_MONOMORPHIZER_H
#define VIT_MONOMORPHIZER_H

#include "ast/AST.h"
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <memory>

namespace vit {

class Monomorphizer : public ASTVisitor {
private:
    std::unordered_map<std::string, const FunctionDeclASTNode*> genericFunctions;
    std::unordered_map<std::string, const StructDeclASTNode*> genericStructs;
    std::unordered_map<std::string, const EnumDeclASTNode*> genericEnums;

    std::unordered_set<std::string> instantiatedSignatures;

    std::vector<std::unique_ptr<FunctionDeclASTNode>> newFunctions;
    std::vector<std::unique_ptr<StatementNode>> newTopLevelStmts;

    std::string mangleName(const std::string& baseName, const std::vector<std::string>& typeArgs);
    void instantiateFunction(const FunctionDeclASTNode* templateFunc, const std::vector<std::string>& typeArgs, const std::string& mangledName);
    void instantiateStruct(const StructDeclASTNode* templateStruct, const std::vector<std::string>& typeArgs, const std::string& mangledName);
    void instantiateEnum(const EnumDeclASTNode* templateEnum, const std::vector<std::string>& typeArgs, const std::string& mangledName);

public:
    Monomorphizer() = default;

    std::string substituteType(const std::string& typeSpec, const std::unordered_map<std::string, std::string>& typeMap);

    void process(ProgramASTNode* program);

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
    void visit(EnumDeclASTNode* node) override;
    void visit(EnumVariantExprASTNode* node) override;
    void visit(MatchASTNode* node) override;
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
    void visit(ExpressionStmtASTNode* node) override;
    void visit(NullLiteralASTNode* node) override;
    void visit(TryExprASTNode* node) override;
    void visit(OptionalChainASTNode* node) override;
    void visit(NullCoalesceASTNode* node) override;
    void visit(AwaitExprASTNode* node) override;
};

} // namespace vit

#endif // VIT_MONOMORPHIZER_H
