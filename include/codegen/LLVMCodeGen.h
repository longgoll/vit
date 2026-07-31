#ifndef VIT_LLVM_CODEGEN_H
#define VIT_LLVM_CODEGEN_H

#include "ast/ASTVisitor.h"
#include "ast/AST.h"
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

namespace vit {

class LLVMCodeGen : public ASTVisitor {
private:
    std::stringstream irStream;
    std::string lastResultReg;
    int regCounter = 0;
    int labelCounter = 0;
    bool blockHasTerminator = false;

    // Symbol table mapping source variable name to LLVM memory allocation register (e.g., "x" -> "%x.addr")
    std::unordered_map<std::string, std::string> symbolTable;

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
    void visit(ReturnASTNode* node) override;
    void visit(PrintASTNode* node) override;
    void visit(NumberLiteralASTNode* node) override;
    void visit(VariableExprASTNode* node) override;
    void visit(BinaryOpASTNode* node) override;
    void visit(CallExprASTNode* node) override;
};

} // namespace vit

#endif // VIT_LLVM_CODEGEN_H
