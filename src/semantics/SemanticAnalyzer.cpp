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
    structTable.clear();
    errorMessages.clear();
    hasError = false;
    loopDepth = 0;

    visit(program);
    return !hasError;
}

void SemanticAnalyzer::visit(ProgramASTNode* node) {
    enterScope();

    // First pass: Register structs and function signatures
    for (const auto& stmt : node->getTopLevelStatements()) {
        if (stmt->getType() == NodeType::StructDecl) {
            auto structDecl = static_cast<StructDeclASTNode*>(stmt.get());
            structTable[structDecl->getName()] = structDecl->getFields();
        }
    }

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

void SemanticAnalyzer::visit(StructDeclASTNode* node) {
    structTable[node->getName()] = node->getFields();
}

void SemanticAnalyzer::visit(ImportASTNode* node) {
    // Module importing resolved at parser/module pass level
}

void SemanticAnalyzer::visit(VarDeclASTNode* node) {
    std::string typeName = node->getTypeName();

    if (node->getInitializer()) {
        node->getInitializer()->accept(this);
        if (typeName.empty()) {
            typeName = lastInferredType; // Type Inference
        }
    }

    if (typeName.empty()) {
        typeName = "number"; // Fallback default
    }

    declareVariable(node->getName(), typeName, node->getIsConst());
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

void SemanticAnalyzer::visit(MemberAssignmentASTNode* node) {
    if (node->getTarget()) {
        node->getTarget()->accept(this);
    }
    if (node->getValue()) {
        node->getValue()->accept(this);
    }
}

void SemanticAnalyzer::visit(ArrayAssignmentASTNode* node) {
    if (node->getArray()) {
        node->getArray()->accept(this);
    }
    if (node->getIndex()) {
        node->getIndex()->accept(this);
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

void SemanticAnalyzer::visit(ArrayLiteralASTNode* node) {
    std::string elemType = "number";
    if (!node->getElements().empty()) {
        node->getElements()[0]->accept(this);
        elemType = lastInferredType;
        for (size_t i = 1; i < node->getElements().size(); ++i) {
            node->getElements()[i]->accept(this);
        }
    }
    lastInferredType = elemType + "[]";
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

void SemanticAnalyzer::visit(MemberAccessASTNode* node) {
    if (node->getTarget()) {
        node->getTarget()->accept(this);
    }
    std::string structType = lastInferredType;
    auto it = structTable.find(structType);
    if (it != structTable.end()) {
        bool foundField = false;
        for (const auto& field : it->second) {
            if (field.first == node->getMember()) {
                lastInferredType = field.second;
                foundField = true;
                break;
            }
        }
        if (!foundField) {
            reportError("Field '" + node->getMember() + "' does not exist on struct '" + structType + "'.");
            lastInferredType = "unknown";
        }
    } else {
        lastInferredType = "unknown";
    }
}

void SemanticAnalyzer::visit(ArrayAccessASTNode* node) {
    if (node->getArray()) {
        node->getArray()->accept(this);
    }
    std::string arrType = lastInferredType;
    if (node->getIndex()) {
        node->getIndex()->accept(this);
    }
    if (arrType.size() > 2 && arrType.substr(arrType.size() - 2) == "[]") {
        lastInferredType = arrType.substr(0, arrType.size() - 2);
    } else {
        lastInferredType = "number";
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
