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
class WhileASTNode;
class ForASTNode;
class BreakASTNode;
class ContinueASTNode;
class ReturnASTNode;
class PrintASTNode;
class NumberLiteralASTNode;
class BooleanLiteralASTNode;
class StringLiteralASTNode;
class VariableExprASTNode;
class UnaryOpASTNode;
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
    virtual void visit(WhileASTNode* node) = 0;
    virtual void visit(ForASTNode* node) = 0;
    virtual void visit(BreakASTNode* node) = 0;
    virtual void visit(ContinueASTNode* node) = 0;
    virtual void visit(ReturnASTNode* node) = 0;
    virtual void visit(PrintASTNode* node) = 0;
    virtual void visit(NumberLiteralASTNode* node) = 0;
    virtual void visit(BooleanLiteralASTNode* node) = 0;
    virtual void visit(StringLiteralASTNode* node) = 0;
    virtual void visit(VariableExprASTNode* node) = 0;
    virtual void visit(UnaryOpASTNode* node) = 0;
    virtual void visit(BinaryOpASTNode* node) = 0;
    virtual void visit(CallExprASTNode* node) = 0;
};

} // namespace vit

#endif // VIT_AST_VISITOR_H
