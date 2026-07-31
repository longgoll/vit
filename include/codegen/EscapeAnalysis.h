#ifndef VIT_ESCAPE_ANALYSIS_H
#define VIT_ESCAPE_ANALYSIS_H

#include "ast/AST.h"
#include "ast/ASTVisitor.h"
#include <string>
#include <unordered_set>
#include <unordered_map>
#include <vector>

namespace vit {

struct EscapeAnalysisResult {
    int stackAllocatedCount = 0;
    int eliminatedARCCount = 0;
    std::vector<std::string> optimizedVars;
    std::string report;
};

class EscapeAnalyzer : public ASTVisitor {
private:
    std::string currentFunctionName;
    std::unordered_set<std::string> localAllocations;
    std::unordered_set<std::string> escapingVars;
    std::unordered_set<std::string> globalVars;

    int totalStackAllocated = 0;
    int totalARCEliminated = 0;
    std::vector<std::string> reportLines;

    void markEscaping(const std::string& varName);
    void checkExprEscape(ExpressionNode* expr);

public:
    EscapeAnalyzer() = default;

    EscapeAnalysisResult analyze(ProgramASTNode* program);

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

#endif // VIT_ESCAPE_ANALYSIS_H
