#ifndef VIT_AST_VISITOR_H
#define VIT_AST_VISITOR_H

namespace vit {

// Forward declarations
class ProgramASTNode;
class FunctionDeclASTNode;
class BlockASTNode;
class VarDeclASTNode;
class AssignmentASTNode;
class IfASTNode;
class ReturnASTNode;
class PrintASTNode;
class NumberLiteralASTNode;
class VariableExprASTNode;
class BinaryOpASTNode;
class CallExprASTNode;

class ASTVisitor {
public:
    virtual ~ASTVisitor() = default;

    virtual void visit(ProgramASTNode* node) = 0;
    virtual void visit(FunctionDeclASTNode* node) = 0;
    virtual void visit(BlockASTNode* node) = 0;
    virtual void visit(VarDeclASTNode* node) = 0;
    virtual void visit(AssignmentASTNode* node) = 0;
    virtual void visit(IfASTNode* node) = 0;
    virtual void visit(ReturnASTNode* node) = 0;
    virtual void visit(PrintASTNode* node) = 0;
    virtual void visit(NumberLiteralASTNode* node) = 0;
    virtual void visit(VariableExprASTNode* node) = 0;
    virtual void visit(BinaryOpASTNode* node) = 0;
    virtual void visit(CallExprASTNode* node) = 0;
};

} // namespace vit

#endif // VIT_AST_VISITOR_H
