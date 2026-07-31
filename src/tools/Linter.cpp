#include "tools/Linter.h"
#include "lexer/Lexer.h"
#include "parser/Parser.h"
#include "ast/ASTVisitor.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <cctype>

namespace vit {

class LintVisitor : public ASTVisitor {
public:
    std::string filePath;
    std::vector<LintWarning>& warnings;

    LintVisitor(const std::string& path, std::vector<LintWarning>& warnList)
        : filePath(path), warnings(warnList) {}

    void visit(ProgramASTNode* node) override {
        for (const auto& func : node->getFunctions()) {
            func->accept(this);
        }
        for (const auto& stmt : node->getTopLevelStatements()) {
            stmt->accept(this);
        }
    }

    void visit(FunctionDeclASTNode* node) override {
        std::string name = node->getName();
        if (!name.empty() && std::isupper(name[0]) && name != "Main") {
            warnings.push_back({
                filePath, 1, 1, "naming-convention",
                "Function '" + name + "' should start with a lowercase letter (camelCase)."
            });
        }
        if (node->getBody()) {
            node->getBody()->accept(this);
        }
    }

    void visit(BlockASTNode* node) override {
        bool hasReturned = false;
        for (const auto& stmt : node->getStatements()) {
            if (hasReturned) {
                warnings.push_back({
                    filePath, 1, 1, "unreachable-code",
                    "Unreachable statement detected after return."
                });
                break;
            }
            if (stmt->getType() == NodeType::Return) {
                hasReturned = true;
            }
            stmt->accept(this);
        }
    }

    void visit(VarDeclASTNode* node) override {
        std::string varName = node->getName();
        if (!varName.empty() && std::isupper(varName[0])) {
            warnings.push_back({
                filePath, 1, 1, "naming-convention",
                "Variable '" + varName + "' should start with a lowercase letter (camelCase)."
            });
        }
        if (node->getInitializer()) {
            node->getInitializer()->accept(this);
        }
    }

    void visit(StructDeclASTNode* node) override {
        std::string structName = node->getName();
        if (!structName.empty() && std::islower(structName[0])) {
            warnings.push_back({
                filePath, 1, 1, "naming-convention",
                "Struct '" + structName + "' should start with an uppercase letter (PascalCase)."
            });
        }
        for (const auto& method : node->getMethods()) {
            method->accept(this);
        }
    }

    void visit(AssignmentASTNode* node) override {
        if (node->getValue()) node->getValue()->accept(this);
    }
    void visit(MemberAssignmentASTNode* node) override {
        if (node->getTarget()) node->getTarget()->accept(this);
        if (node->getValue()) node->getValue()->accept(this);
    }
    void visit(ArrayAssignmentASTNode* node) override {
        if (node->getArray()) node->getArray()->accept(this);
        if (node->getIndex()) node->getIndex()->accept(this);
        if (node->getValue()) node->getValue()->accept(this);
    }
    void visit(IfASTNode* node) override {
        if (node->getCondition()) node->getCondition()->accept(this);
        if (node->getThenBlock()) node->getThenBlock()->accept(this);
        if (node->getElseBlock()) node->getElseBlock()->accept(this);
    }
    void visit(WhileASTNode* node) override {
        if (node->getCondition()) node->getCondition()->accept(this);
        if (node->getBody()) node->getBody()->accept(this);
    }
    void visit(ForASTNode* node) override {
        if (node->getInit()) node->getInit()->accept(this);
        if (node->getCondition()) node->getCondition()->accept(this);
        if (node->getUpdate()) node->getUpdate()->accept(this);
        if (node->getBody()) node->getBody()->accept(this);
    }
    void visit(BreakASTNode*) override {}
    void visit(ContinueASTNode*) override {}
    void visit(ReturnASTNode* node) override {
        if (node->getValue()) node->getValue()->accept(this);
    }
    void visit(PrintASTNode* node) override {
        if (node->getExpression()) node->getExpression()->accept(this);
    }
    void visit(ImportASTNode*) override {}
    void visit(TypeAliasASTNode*) override {}
    void visit(NumberLiteralASTNode*) override {}
    void visit(BooleanLiteralASTNode*) override {}
    void visit(StringLiteralASTNode*) override {}
    void visit(NullLiteralASTNode*) override {}
    void visit(ArrayLiteralASTNode* node) override {
        for (const auto& elem : node->getElements()) {
            elem->accept(this);
        }
    }
    void visit(VariableExprASTNode*) override {}
    void visit(MemberAccessASTNode* node) override {
        if (node->getTarget()) node->getTarget()->accept(this);
    }
    void visit(ArrayAccessASTNode* node) override {
        if (node->getArray()) node->getArray()->accept(this);
        if (node->getIndex()) node->getIndex()->accept(this);
    }
    void visit(UnaryOpASTNode* node) override {
        if (node->getOperand()) node->getOperand()->accept(this);
    }
    void visit(BinaryOpASTNode* node) override {
        if (node->getLeft()) node->getLeft()->accept(this);
        if (node->getRight()) node->getRight()->accept(this);
    }
    void visit(CallExprASTNode* node) override {
        for (const auto& arg : node->getArgs()) {
            arg->accept(this);
        }
    }
    void visit(MethodCallASTNode* node) override {
        if (node->getTarget()) node->getTarget()->accept(this);
        for (const auto& arg : node->getArgs()) {
            arg->accept(this);
        }
    }
    void visit(LambdaASTNode* node) override {
        if (node->getBody()) node->getBody()->accept(this);
    }
    void visit(EnumDeclASTNode* node) override {
        std::string enumName = node->getName();
        if (!enumName.empty() && std::islower(enumName[0])) {
            warnings.push_back({
                filePath, 1, 1, "naming-convention",
                "Enum '" + enumName + "' should start with an uppercase letter (PascalCase)."
            });
        }
    }
    void visit(EnumVariantExprASTNode*) override {}
    void visit(MatchASTNode*) override {}
    void visit(ExpressionStmtASTNode* node) override {
        if (node->getExpression()) node->getExpression()->accept(this);
    }
    void visit(TryExprASTNode* node) override {
        if (node->getExpr()) node->getExpr()->accept(this);
    }
    void visit(OptionalChainASTNode* node) override {
        if (node->getTarget()) node->getTarget()->accept(this);
    }
    void visit(NullCoalesceASTNode* node) override {
        if (node->getLeft()) node->getLeft()->accept(this);
        if (node->getRight()) node->getRight()->accept(this);
    }
    void visit(AwaitExprASTNode* node) override {
        if (node->getExpr()) node->getExpr()->accept(this);
    }
};

bool Linter::lintCode(const std::string& code, const std::string& filePath, std::vector<LintWarning>& warnings) {
    try {
        Lexer lexer(code);
        Parser parser(std::move(lexer));
        auto programAST = parser.parseProgram();

        LintVisitor visitor(filePath, warnings);
        programAST->accept(&visitor);
        return true;
    } catch (const std::exception& e) {
        warnings.push_back({
            filePath, 1, 1, "syntax-error",
            std::string("Syntax error during linting: ") + e.what()
        });
        return false;
    }
}

bool Linter::lintFile(const std::string& filePath, std::vector<LintWarning>& warnings) {
    std::ifstream file(filePath);
    if (!file.is_open()) {
        std::cerr << "\033[31m[VIT Error]\033[0m Could not open file for linting: '" << filePath << "'\n";
        return false;
    }

    std::stringstream ss;
    ss << file.rdbuf();
    file.close();

    return lintCode(ss.str(), filePath, warnings);
}

} // namespace vit
