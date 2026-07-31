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

struct StructInfo {
    std::string name;
    std::vector<std::pair<std::string, std::string>> fields;
    std::unordered_map<std::string, int> fieldIndices;
};

class LLVMCodeGen : public ASTVisitor {
private:
    std::stringstream irStream;
    std::stringstream globalDefsStream;
    std::string lastResultReg;
    std::string lastResultType; // "number", "boolean", "string", struct name, array type
    std::string currentFunctionReturnType;
    std::string currentBlockLabel;
    std::unordered_map<std::string, std::string> functionReturnTypes;
    std::unordered_map<std::string, StructInfo> structs;
    int regCounter = 0;
    int labelCounter = 0;
    int stringCounter = 0;
    bool blockHasTerminator = false;

    std::vector<LoopTarget> loopStack;
    std::unordered_map<std::string, VarSymbol> symbolTable;

    std::string newReg();
    std::string newLabel(const std::string& prefix);
    std::string getLLVMType(const std::string& vitType);
    void emitIndent();

public:
    LLVMCodeGen() = default;

    std::string generateIR(ProgramASTNode* program);

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
};

} // namespace vit

#endif // VIT_LLVM_CODEGEN_H
