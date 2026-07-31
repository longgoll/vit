#include "semantics/SemanticAnalyzer.h"
#include <iostream>

namespace vit {

void SemanticAnalyzer::enterScope() {
    scopeStack.push_back({});
}

void SemanticAnalyzer::exitScope() {
    if (!scopeStack.empty()) {
        scopeStack.pop_back();
    }
}

bool SemanticAnalyzer::declareVariable(const std::string& name, const std::string& typeName, bool isConst) {
    if (scopeStack.empty()) return false;
    auto& currentScope = scopeStack.back();
    if (currentScope.find(name) != currentScope.end()) {
        reportError("Variable '" + name + "' is already declared in the current scope.");
        return false;
    }
    currentScope[name] = {name, typeName, isConst};
    return true;
}

const SymbolInfo* SemanticAnalyzer::lookupVariable(const std::string& name) const {
    for (auto it = scopeStack.rbegin(); it != scopeStack.rend(); ++it) {
        auto found = it->find(name);
        if (found != it->end()) {
            return &(found->second);
        }
    }
    return nullptr;
}

void SemanticAnalyzer::reportError(const std::string& msg) {
    hasError = true;
    errorMessages.push_back(msg);
}

bool SemanticAnalyzer::analyze(ProgramASTNode* program) {
    scopeStack.clear();
    functionTable.clear();
    errorMessages.clear();
    hasError = false;
    loopDepth = 0;

    visit(program);
    return !hasError;
}

void SemanticAnalyzer::visit(ProgramASTNode* node) {
    enterScope();

    // First pass: Register all functions in global table
    for (const auto& func : node->getFunctions()) {
        std::vector<std::string> paramTypes;
        for (const auto& p : func->getParams()) {
            paramTypes.push_back(p.typeName);
        }
        functionTable[func->getName()] = {func->getReturnType(), paramTypes};
    }

    // Second pass: Validate function bodies & top level statements
    for (const auto& func : node->getFunctions()) {
        func->accept(this);
    }
    for (const auto& stmt : node->getTopLevelStatements()) {
        stmt->accept(this);
    }

    exitScope();
}

void SemanticAnalyzer::visit(FunctionDeclASTNode* node) {
    currentReturnType = node->getReturnType();
    enterScope();

    for (const auto& param : node->getParams()) {
        declareVariable(param.name, param.typeName, false);
    }

    if (node->getBody()) {
        node->getBody()->accept(this);
    }

    exitScope();
}

void SemanticAnalyzer::visit(BlockASTNode* node) {
    enterScope();
    for (const auto& stmt : node->getStatements()) {
        stmt->accept(this);
    }
    exitScope();
}

void SemanticAnalyzer::visit(VarDeclASTNode* node) {
    if (node->getInitializer()) {
        node->getInitializer()->accept(this);
    }
    declareVariable(node->getName(), node->getTypeName(), node->getIsConst());
}

void SemanticAnalyzer::visit(AssignmentASTNode* node) {
    const SymbolInfo* sym = lookupVariable(node->getName());
    if (!sym) {
        reportError("Cannot assign to undeclared variable '" + node->getName() + "'.");
    } else if (sym->isConst) {
        reportError("Cannot re-assign to const variable '" + node->getName() + "'.");
    }

    if (node->getValue()) {
        node->getValue()->accept(this);
    }
}

void SemanticAnalyzer::visit(IfASTNode* node) {
    if (node->getCondition()) {
        node->getCondition()->accept(this);
    }
    if (node->getThenBlock()) {
        node->getThenBlock()->accept(this);
    }
    if (node->getElseBlock()) {
        node->getElseBlock()->accept(this);
    }
}

void SemanticAnalyzer::visit(WhileASTNode* node) {
    if (node->getCondition()) {
        node->getCondition()->accept(this);
    }
    loopDepth++;
    if (node->getBody()) {
        node->getBody()->accept(this);
    }
    loopDepth--;
}

void SemanticAnalyzer::visit(ForASTNode* node) {
    enterScope(); // Loop scope for init variable
    if (node->getInit()) {
        node->getInit()->accept(this);
    }
    if (node->getCondition()) {
        node->getCondition()->accept(this);
    }
    if (node->getUpdate()) {
        node->getUpdate()->accept(this);
    }
    loopDepth++;
    if (node->getBody()) {
        node->getBody()->accept(this);
    }
    loopDepth--;
    exitScope();
}

void SemanticAnalyzer::visit(BreakASTNode* node) {
    if (loopDepth <= 0) {
        reportError("'break' statement can only be used inside a loop.");
    }
}

void SemanticAnalyzer::visit(ContinueASTNode* node) {
    if (loopDepth <= 0) {
        reportError("'continue' statement can only be used inside a loop.");
    }
}

void SemanticAnalyzer::visit(ReturnASTNode* node) {
    if (node->getValue()) {
        node->getValue()->accept(this);
    } else {
        lastInferredType = "void";
    }
}

void SemanticAnalyzer::visit(PrintASTNode* node) {
    if (node->getExpression()) {
        node->getExpression()->accept(this);
    }
}

void SemanticAnalyzer::visit(NumberLiteralASTNode* node) {
    lastInferredType = "number";
}

void SemanticAnalyzer::visit(BooleanLiteralASTNode* node) {
    lastInferredType = "boolean";
}

void SemanticAnalyzer::visit(StringLiteralASTNode* node) {
    lastInferredType = "string";
}

void SemanticAnalyzer::visit(VariableExprASTNode* node) {
    const SymbolInfo* sym = lookupVariable(node->getName());
    if (!sym) {
        reportError("Use of undeclared variable '" + node->getName() + "'.");
        lastInferredType = "unknown";
    } else {
        lastInferredType = sym->typeName;
    }
}

void SemanticAnalyzer::visit(UnaryOpASTNode* node) {
    if (node->getOperand()) {
        node->getOperand()->accept(this);
    }
    if (node->getOp() == "!") {
        lastInferredType = "boolean";
    } else {
        lastInferredType = "number";
    }
}

void SemanticAnalyzer::visit(BinaryOpASTNode* node) {
    if (node->getLeft()) node->getLeft()->accept(this);
    std::string leftType = lastInferredType;

    if (node->getRight()) node->getRight()->accept(this);
    std::string rightType = lastInferredType;

    const std::string& op = node->getOp();
    if (op == "&&" || op == "||" || op == ">" || op == "<" || op == "==" || op == "!=" || op == ">=" || op == "<=") {
        lastInferredType = "boolean";
    } else {
        lastInferredType = "number";
    }
}

void SemanticAnalyzer::visit(CallExprASTNode* node) {
    auto it = functionTable.find(node->getCallee());
    if (it == functionTable.end()) {
        reportError("Call to undefined function '" + node->getCallee() + "'.");
        lastInferredType = "unknown";
    } else {
        lastInferredType = it->second.first;
    }

    for (const auto& arg : node->getArgs()) {
        arg->accept(this);
    }
}

} // namespace vit
