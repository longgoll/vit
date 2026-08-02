#include "tools/Linter.h"
#include "lexer/Lexer.h"
#include "parser/Parser.h"
#include "ast/ASTVisitor.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <cctype>
#include <set>
#include <map>

namespace vit {

// Track declarations with their source location using token positions
// We extend ASTNode to carry line info by rebuilding with position-aware visit
struct VarUsage {
    std::string name;
    int declLine = 0;
    int declCol = 0;
    bool isUsed = false;
    bool isConst = false;
};

class LintVisitor : public ASTVisitor {
public:
    std::string filePath;
    std::vector<LintWarning>& warnings;
    std::vector<std::map<std::string, VarUsage>> scopeStack; // scope stack for unused var tracking
    std::set<std::string> calledFunctions;
    std::set<std::string> declaredFunctions;

    LintVisitor(const std::string& path, std::vector<LintWarning>& warnList)
        : filePath(path), warnings(warnList) {
        scopeStack.push_back({});
    }

    void addWarning(int line, int col, const std::string& rule, const std::string& msg) {
        warnings.push_back({filePath, line, col, rule, msg});
    }

    void declareVar(const std::string& name, int line, int col, bool isConst) {
        if (!scopeStack.empty()) {
            scopeStack.back()[name] = {name, line, col, false, isConst};
        }
    }

    void markUsed(const std::string& name) {
        for (auto it = scopeStack.rbegin(); it != scopeStack.rend(); ++it) {
            auto found = it->find(name);
            if (found != it->end()) {
                found->second.isUsed = true;
                return;
            }
        }
    }

    void checkUnusedInScope() {
        if (scopeStack.empty()) return;
        for (auto& [name, usage] : scopeStack.back()) {
            // Skip loop vars like i, j, k and internal vars
            if (!usage.isUsed && name.size() > 1 && name.substr(0, 7) != "__vitFor") {
                addWarning(usage.declLine, usage.declCol, "unused-variable",
                    "Variable '" + name + "' is declared but never used.");
            }
        }
    }

    void enterScope() { scopeStack.push_back({}); }
    void exitScope() {
        checkUnusedInScope();
        if (!scopeStack.empty()) scopeStack.pop_back();
    }

    void visit(ProgramASTNode* node) override {
        for (const auto& func : node->getFunctions()) {
            declaredFunctions.insert(func->getName());
        }
        for (const auto& func : node->getFunctions()) {
            func->accept(this);
        }
        for (const auto& stmt : node->getTopLevelStatements()) {
            stmt->accept(this);
        }
        // Check for unused functions (excluding main and extern)
        for (const auto& func : node->getFunctions()) {
            if (!func->getIsExtern() && func->getName() != "main"
                && calledFunctions.find(func->getName()) == calledFunctions.end()) {
                addWarning(1, 1, "dead-code",
                    "Function '" + func->getName() + "' is declared but never called.");
            }
        }
    }

    void visit(FunctionDeclASTNode* node) override {
        std::string name = node->getName();
        // Naming: functions should be camelCase
        if (!name.empty() && std::isupper(name[0]) && name != "Main" && name != "main") {
            addWarning(1, 1, "naming-convention",
                "Function '" + name + "' should start with a lowercase letter (camelCase).");
        }
        if (node->getIsExtern()) return;
        if (node->getBody()) {
            enterScope();
            // Register parameters as used (they're declared externally)
            for (const auto& param : node->getParams()) {
                declareVar(param.name, 1, 1, false);
                markUsed(param.name); // params are considered "used" by being part of signature
            }
            node->getBody()->accept(this);
            // Check if non-void function has return
            if (node->getReturnType() != "void" && node->getReturnType() != "") {
                checkFunctionHasReturn(node->getBody(), node->getName(), node->getReturnType());
            }
            exitScope();
        }
    }

    void checkFunctionHasReturn(BlockASTNode* block, const std::string& funcName, const std::string& retType) {
        if (!block) return;
        bool hasReturn = false;
        for (const auto& stmt : block->getStatements()) {
            if (stmt->getType() == NodeType::Return) {
                hasReturn = true;
                break;
            }
        }
        if (!hasReturn) {
            addWarning(1, 1, "missing-return",
                "Function '" + funcName + "' with return type '" + retType + "' may be missing a return statement.");
        }
    }

    void visit(BlockASTNode* node) override {
        bool hasReturned = false;
        for (const auto& stmt : node->getStatements()) {
            if (hasReturned) {
                addWarning(1, 1, "unreachable-code",
                    "Unreachable statement detected after return.");
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
        // Naming: variables should be camelCase
        if (!varName.empty() && std::isupper(varName[0])) {
            addWarning(1, 1, "naming-convention",
                "Variable '" + varName + "' should start with a lowercase letter (camelCase).");
        }
        // Track for unused variable detection
        declareVar(varName, 1, 1, node->getIsConst());
        if (node->getInitializer()) {
            node->getInitializer()->accept(this);
        }
    }

    void visit(StructDeclASTNode* node) override {
        std::string structName = node->getName();
        if (!structName.empty() && std::islower(structName[0])) {
            addWarning(1, 1, "naming-convention",
                "Struct '" + structName + "' should start with an uppercase letter (PascalCase).");
        }
        for (const auto& method : node->getMethods()) {
            method->accept(this);
        }
    }

    void visit(AssignmentASTNode* node) override {
        markUsed(node->getName());
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
        if (node->getThenBlock()) {
            enterScope();
            node->getThenBlock()->accept(this);
            exitScope();
        }
        if (node->getElseBlock()) {
            enterScope();
            node->getElseBlock()->accept(this);
            exitScope();
        }
    }
    void visit(WhileASTNode* node) override {
        if (node->getCondition()) node->getCondition()->accept(this);
        if (node->getBody()) {
            enterScope();
            node->getBody()->accept(this);
            exitScope();
        }
    }
    void visit(ForASTNode* node) override {
        enterScope();
        if (node->getInit()) node->getInit()->accept(this);
        if (node->getCondition()) node->getCondition()->accept(this);
        if (node->getUpdate()) node->getUpdate()->accept(this);
        if (node->getBody()) node->getBody()->accept(this);
        exitScope();
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
    void visit(VariableExprASTNode* node) override {
        markUsed(node->getName());
    }
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
        calledFunctions.insert(node->getCallee());
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
            addWarning(1, 1, "naming-convention",
                "Enum '" + enumName + "' should start with an uppercase letter (PascalCase).");
        }
    }
    void visit(EnumVariantExprASTNode*) override {}
    void visit(MatchASTNode* node) override {
        if (node->getTarget()) node->getTarget()->accept(this);
    }
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
