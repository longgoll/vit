#ifndef VIT_AST_PRINTER_H
#define VIT_AST_PRINTER_H

#include "ASTVisitor.h"
#include <iostream>

namespace vit {

class ASTPrinter : public ASTVisitor {
private:
    std::ostream& out;
    int indentLevel = 0;

    void printIndent() const;

public:
    explicit ASTPrinter(std::ostream& os = std::cout) : out(os) {}

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

#endif // VIT_AST_PRINTER_H
