#include "ast/ASTPrinter.h"
#include "ast/AST.h"

namespace vit {

void ASTPrinter::printIndent() const {
    for (int i = 0; i < indentLevel; ++i) {
        out << "  ";
    }
}

void ASTPrinter::visit(ProgramASTNode* node) {
    printIndent();
    out << "[ProgramASTNode]\n";
    indentLevel++;
    for (const auto& func : node->getFunctions()) {
        func->accept(this);
    }
    for (const auto& stmt : node->getTopLevelStatements()) {
        stmt->accept(this);
    }
    indentLevel--;
}

void ASTPrinter::visit(FunctionDeclASTNode* node) {
    printIndent();
    if (node->getIsExtern()) {
        out << "[ExternFunctionDeclASTNode] " << node->getName() << "(";
    } else {
        out << "[FunctionDeclASTNode] " << node->getName() << "(";
    }
    const auto& params = node->getParams();
    for (size_t i = 0; i < params.size(); ++i) {
        out << params[i].name << ": " << params[i].typeName;
        if (i + 1 < params.size()) out << ", ";
    }
    out << "): " << node->getReturnType() << "\n";

    indentLevel++;
    if (node->getBody()) {
        node->getBody()->accept(this);
    }
    indentLevel--;
}

void ASTPrinter::visit(BlockASTNode* node) {
    printIndent();
    out << "[BlockASTNode]\n";
    indentLevel++;
    for (const auto& stmt : node->getStatements()) {
        stmt->accept(this);
    }
    indentLevel--;
}

void ASTPrinter::visit(VarDeclASTNode* node) {
    printIndent();
    out << "[VarDeclASTNode] " << (node->getIsConst() ? "const " : "let ")
        << node->getName() << ": " << node->getTypeName() << "\n";
    if (node->getInitializer()) {
        indentLevel++;
        printIndent();
        out << "Initializer:\n";
        indentLevel++;
        node->getInitializer()->accept(this);
        indentLevel -= 2;
    }
}

void ASTPrinter::visit(AssignmentASTNode* node) {
    printIndent();
    out << "[AssignmentASTNode] " << node->getName() << " =\n";
    indentLevel++;
    if (node->getValue()) {
        node->getValue()->accept(this);
    }
    indentLevel--;
}

void ASTPrinter::visit(MemberAssignmentASTNode* node) {
    printIndent();
    out << "[MemberAssignmentASTNode] member: ." << node->getMember() << " =\n";
    indentLevel++;
    if (node->getTarget()) node->getTarget()->accept(this);
    if (node->getValue()) node->getValue()->accept(this);
    indentLevel--;
}

void ASTPrinter::visit(ArrayAssignmentASTNode* node) {
    printIndent();
    out << "[ArrayAssignmentASTNode] [] =\n";
    indentLevel++;
    if (node->getArray()) node->getArray()->accept(this);
    if (node->getIndex()) node->getIndex()->accept(this);
    if (node->getValue()) node->getValue()->accept(this);
    indentLevel--;
}

void ASTPrinter::visit(StructDeclASTNode* node) {
    printIndent();
    out << "[StructDeclASTNode] struct " << node->getName() << " {\n";
    indentLevel++;
    for (const auto& field : node->getFields()) {
        printIndent();
        out << field.first << ": " << field.second << "\n";
    }
    indentLevel--;
}

void ASTPrinter::visit(ImportASTNode* node) {
    printIndent();
    out << "[ImportASTNode] import from \"" << node->getModulePath() << "\"\n";
}

void ASTPrinter::visit(IfASTNode* node) {
    printIndent();
    out << "[IfASTNode]\n";
    indentLevel++;

    printIndent();
    out << "Condition:\n";
    indentLevel++;
    if (node->getCondition()) {
        node->getCondition()->accept(this);
    }
    indentLevel--;

    printIndent();
    out << "Then:\n";
    indentLevel++;
    if (node->getThenBlock()) {
        node->getThenBlock()->accept(this);
    }
    indentLevel--;

    if (node->getElseBlock()) {
        printIndent();
        out << "Else:\n";
        indentLevel++;
        node->getElseBlock()->accept(this);
        indentLevel--;
    }

    indentLevel--;
}

void ASTPrinter::visit(WhileASTNode* node) {
    printIndent();
    out << "[WhileASTNode]\n";
    indentLevel++;
    printIndent();
    out << "Condition:\n";
    indentLevel++;
    if (node->getCondition()) node->getCondition()->accept(this);
    indentLevel--;
    printIndent();
    out << "Body:\n";
    indentLevel++;
    if (node->getBody()) node->getBody()->accept(this);
    indentLevel -= 2;
}

void ASTPrinter::visit(ForASTNode* node) {
    printIndent();
    out << "[ForASTNode]\n";
    indentLevel++;
    if (node->getInit()) {
        printIndent();
        out << "Init:\n";
        indentLevel++;
        node->getInit()->accept(this);
        indentLevel--;
    }
    if (node->getCondition()) {
        printIndent();
        out << "Condition:\n";
        indentLevel++;
        node->getCondition()->accept(this);
        indentLevel--;
    }
    if (node->getUpdate()) {
        printIndent();
        out << "Update:\n";
        indentLevel++;
        node->getUpdate()->accept(this);
        indentLevel--;
    }
    printIndent();
    out << "Body:\n";
    indentLevel++;
    if (node->getBody()) node->getBody()->accept(this);
    indentLevel -= 2;
}

void ASTPrinter::visit(BreakASTNode* node) {
    printIndent();
    out << "[BreakASTNode]\n";
}

void ASTPrinter::visit(ContinueASTNode* node) {
    printIndent();
    out << "[ContinueASTNode]\n";
}

void ASTPrinter::visit(ReturnASTNode* node) {
    printIndent();
    out << "[ReturnASTNode]\n";
    if (node->getValue()) {
        indentLevel++;
        node->getValue()->accept(this);
        indentLevel--;
    }
}

void ASTPrinter::visit(PrintASTNode* node) {
    printIndent();
    out << "[PrintASTNode]\n";
    if (node->getExpression()) {
        indentLevel++;
        node->getExpression()->accept(this);
        indentLevel--;
    }
}

void ASTPrinter::visit(NumberLiteralASTNode* node) {
    printIndent();
    out << "[NumberLiteralASTNode] " << node->getValue() << "\n";
}

void ASTPrinter::visit(BooleanLiteralASTNode* node) {
    printIndent();
    out << "[BooleanLiteralASTNode] " << (node->getValue() ? "true" : "false") << "\n";
}

void ASTPrinter::visit(StringLiteralASTNode* node) {
    printIndent();
    out << "[StringLiteralASTNode] \"" << node->getValue() << "\"\n";
}

void ASTPrinter::visit(ArrayLiteralASTNode* node) {
    printIndent();
    out << "[ArrayLiteralASTNode]\n";
    indentLevel++;
    for (const auto& elem : node->getElements()) {
        elem->accept(this);
    }
    indentLevel--;
}

void ASTPrinter::visit(VariableExprASTNode* node) {
    printIndent();
    out << "[VariableExprASTNode] " << node->getName() << "\n";
}

void ASTPrinter::visit(MemberAccessASTNode* node) {
    printIndent();
    out << "[MemberAccessASTNode] member: ." << node->getMember() << "\n";
    indentLevel++;
    if (node->getTarget()) node->getTarget()->accept(this);
    indentLevel--;
}

void ASTPrinter::visit(ArrayAccessASTNode* node) {
    printIndent();
    out << "[ArrayAccessASTNode]\n";
    indentLevel++;
    if (node->getArray()) node->getArray()->accept(this);
    if (node->getIndex()) node->getIndex()->accept(this);
    indentLevel--;
}

void ASTPrinter::visit(UnaryOpASTNode* node) {
    printIndent();
    out << "[UnaryOpASTNode] op: '" << node->getOp() << "'\n";
    indentLevel++;
    if (node->getOperand()) {
        node->getOperand()->accept(this);
    }
    indentLevel--;
}

void ASTPrinter::visit(BinaryOpASTNode* node) {
    printIndent();
    out << "[BinaryOpASTNode] op: '" << node->getOp() << "'\n";
    indentLevel++;
    if (node->getLeft()) {
        node->getLeft()->accept(this);
    }
    if (node->getRight()) {
        node->getRight()->accept(this);
    }
    indentLevel--;
}

void ASTPrinter::visit(CallExprASTNode* node) {
    printIndent();
    out << "[CallExprASTNode] callee: " << node->getCallee() << "\n";
    indentLevel++;
    for (const auto& arg : node->getArgs()) {
        arg->accept(this);
    }
    indentLevel--;
}

} // namespace vit
