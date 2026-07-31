#ifndef VIT_LLVM_CODEGEN_H
#define VIT_LLVM_CODEGEN_H

#include "ast/ASTVisitor.h"
#include "ast/AST.h"
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

namespace vit {

struct VarSymbol {
    std::string addrReg;
    std::string typeName; // "number", "boolean", "string"
};

struct LoopTarget {
    std::string breakLabel;
    std::string continueLabel;
};

class LLVMCodeGen : public ASTVisitor {
private:
    std::stringstream irStream;
    std::stringstream globalDefsStream;
    std::string lastResultReg;
    std::string lastResultType; // "number", "boolean", "string"
    std::string currentFunctionReturnType;
    std::string currentBlockLabel;
    std::unordered_map<std::string, std::string> functionReturnTypes;
    int regCounter = 0;
    int labelCounter = 0;
    int stringCounter = 0;
    bool blockHasTerminator = false;

    std::vector<LoopTarget> loopStack;
    std::unordered_map<std::string, VarSymbol> symbolTable;

    std::string newReg();
    std::string newLabel(const std::string& prefix);
    void emitIndent();

public:
    LLVMCodeGen() = default;

    std::string generateIR(ProgramASTNode* program);

    void visit(ProgramASTNode* node) override;
    void visit(FunctionDeclASTNode* node) override;
    void visit(BlockASTNode* node) override;
    void visit(VarDeclASTNode* node) override;
    void visit(AssignmentASTNode* node) override;
    void visit(IfASTNode* node) override;
    void visit(WhileASTNode* node) override;
    void visit(ForASTNode* node) override;
    void visit(BreakASTNode* node) override;
    void visit(ContinueASTNode* node) override;
    void visit(ReturnASTNode* node) override;
    void visit(PrintASTNode* node) override;
    void visit(NumberLiteralASTNode* node) override;
    void visit(BooleanLiteralASTNode* node) override;
    void visit(StringLiteralASTNode* node) override;
    void visit(VariableExprASTNode* node) override;
    void visit(UnaryOpASTNode* node) override;
    void visit(BinaryOpASTNode* node) override;
    void visit(CallExprASTNode* node) override;
};

} // namespace vit

#endif // VIT_LLVM_CODEGEN_H
