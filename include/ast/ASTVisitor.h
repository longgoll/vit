#ifndef VIT_AST_VISITOR_H
#define VIT_AST_VISITOR_H

namespace vit {

// Forward declarations
class ProgramASTNode;
class FunctionDeclASTNode;
class BlockASTNode;
class VarDeclASTNode;
class AssignmentASTNode;
class MemberAssignmentASTNode;
class ArrayAssignmentASTNode;
class IfASTNode;
class WhileASTNode;
class ForASTNode;
class BreakASTNode;
class ContinueASTNode;
class ReturnASTNode;
class PrintASTNode;
class StructDeclASTNode;
class ImportASTNode;
class NumberLiteralASTNode;
class BooleanLiteralASTNode;
class StringLiteralASTNode;
class ArrayLiteralASTNode;
class VariableExprASTNode;
class MemberAccessASTNode;
class ArrayAccessASTNode;
class UnaryOpASTNode;
class BinaryOpASTNode;
class CallExprASTNode;
class MethodCallASTNode;
class LambdaASTNode;
class TypeAliasASTNode;
class EnumDeclASTNode;
class EnumVariantExprASTNode;
class MatchASTNode;
class ExpressionStmtASTNode;
class NullLiteralASTNode;
class TryExprASTNode;
class OptionalChainASTNode;
class NullCoalesceASTNode;
class AwaitExprASTNode;

class ASTVisitor {
public:
    virtual ~ASTVisitor() = default;

    virtual void visit(ProgramASTNode* node) = 0;
    virtual void visit(FunctionDeclASTNode* node) = 0;
    virtual void visit(BlockASTNode* node) = 0;
    virtual void visit(VarDeclASTNode* node) = 0;
    virtual void visit(AssignmentASTNode* node) = 0;
    virtual void visit(MemberAssignmentASTNode* node) = 0;
    virtual void visit(ArrayAssignmentASTNode* node) = 0;
    virtual void visit(IfASTNode* node) = 0;
    virtual void visit(WhileASTNode* node) = 0;
    virtual void visit(ForASTNode* node) = 0;
    virtual void visit(BreakASTNode* node) = 0;
    virtual void visit(ContinueASTNode* node) = 0;
    virtual void visit(ReturnASTNode* node) = 0;
    virtual void visit(PrintASTNode* node) = 0;
    virtual void visit(StructDeclASTNode* node) = 0;
    virtual void visit(ImportASTNode* node) = 0;
    virtual void visit(TypeAliasASTNode* node) = 0;
    virtual void visit(NumberLiteralASTNode* node) = 0;
    virtual void visit(BooleanLiteralASTNode* node) = 0;
    virtual void visit(StringLiteralASTNode* node) = 0;
    virtual void visit(ArrayLiteralASTNode* node) = 0;
    virtual void visit(VariableExprASTNode* node) = 0;
    virtual void visit(MemberAccessASTNode* node) = 0;
    virtual void visit(ArrayAccessASTNode* node) = 0;
    virtual void visit(UnaryOpASTNode* node) = 0;
    virtual void visit(BinaryOpASTNode* node) = 0;
    virtual void visit(CallExprASTNode* node) = 0;
    virtual void visit(MethodCallASTNode* node) = 0;
    virtual void visit(LambdaASTNode* node) = 0;
    virtual void visit(EnumDeclASTNode* node) = 0;
    virtual void visit(EnumVariantExprASTNode* node) = 0;
    virtual void visit(MatchASTNode* node) = 0;
    virtual void visit(ExpressionStmtASTNode* node) = 0;
    virtual void visit(NullLiteralASTNode* node) = 0;
    virtual void visit(TryExprASTNode* node) = 0;
    virtual void visit(OptionalChainASTNode* node) = 0;
    virtual void visit(NullCoalesceASTNode* node) = 0;
    virtual void visit(AwaitExprASTNode* node) = 0;
};

} // namespace vit

#endif // VIT_AST_VISITOR_H
