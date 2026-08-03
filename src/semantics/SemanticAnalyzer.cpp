#include "semantics/SemanticAnalyzer.h"
#include <iostream>
#include <unordered_set>

namespace vit {

// ─── Scope management ─────────────────────────────────────────────────────────

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

std::string SemanticAnalyzer::resolveType(const std::string& typeName) const {
    std::unordered_set<std::string> visited;
    std::string current = typeName;
    while (visited.find(current) == visited.end()) {
        visited.insert(current);
        auto it = typeAliasTable.find(current);
        if (it != typeAliasTable.end()) {
            current = it->second;
        } else {
            break;
        }
    }
    return current;
}

bool SemanticAnalyzer::analyze(ProgramASTNode* program) {
    scopeStack.clear();
    functionTable.clear();
    structTable.clear();
    structMethodsTable.clear();
    typeAliasTable.clear();
    errorMessages.clear();
    hasError = false;
    loopDepth = 0;

    visit(program);
    return !hasError;
}

// ─── Statement visitors ───────────────────────────────────────────────────────

void SemanticAnalyzer::visit(ProgramASTNode* node) {
    enterScope();

    // Register built-in safety and runtime C functions
    functionTable["panic"]   = {"void", {"string"}};
    functionTable["assert"]  = {"void", {"boolean", "string"}};
    functionTable["printf"]  = {"int", {}};
    functionTable["sprintf"] = {"int", {}};
    functionTable["fflush"]  = {"int", {}};
    functionTable["malloc"]  = {"void*", {"int"}};
    functionTable["free"]    = {"void", {"void*"}};
    functionTable["strlen"]  = {"int", {"string"}};
    functionTable["strcmp"]  = {"int", {"string", "string"}};
    functionTable["strcpy"]  = {"string", {"string", "string"}};
    functionTable["strcat"]  = {"string", {"string", "string"}};
    functionTable["exit"]    = {"void", {"int"}};

    // First pass: Register structs and function signatures
    for (const auto& stmt : node->getTopLevelStatements()) {
        if (stmt->getType() == NodeType::StructDecl) {
            auto structDecl = static_cast<StructDeclASTNode*>(stmt.get());
            structTable[structDecl->getName()] = structDecl->getFields();
            for (const auto& method : structDecl->getMethods()) {
                std::vector<std::string> paramTypes;
                for (const auto& p : method->getParams()) {
                    paramTypes.push_back(p.typeName);
                }
                structMethodsTable[structDecl->getName()][method->getName()] = {method->getReturnType(), paramTypes};
            }
        } else if (stmt->getType() == NodeType::EnumDecl) {
            auto enumDecl = static_cast<EnumDeclASTNode*>(stmt.get());
            enumTable[enumDecl->getName()] = enumDecl->getVariants();
        }
    }

    for (const auto& func : node->getFunctions()) {
        std::vector<std::string> paramTypes;
        for (const auto& p : func->getParams()) {
            paramTypes.push_back(p.typeName);
        }
        functionTable[func->getName()] = {func->getReturnType(), paramTypes};
    }

    // Pass 1.5: Register top-level variable declarations in global scope
    for (const auto& stmt : node->getTopLevelStatements()) {
        if (stmt->getType() == NodeType::VarDecl) {
            auto varDecl = static_cast<VarDeclASTNode*>(stmt.get());
            declareVariable(varDecl->getName(), varDecl->getTypeName(), varDecl->getIsConst());
        }
    }

    // Second pass: Validate function bodies & top level statements
    for (const auto& func : node->getFunctions()) {
        func->accept(this);
    }
    for (const auto& stmt : node->getTopLevelStatements()) {
        if (stmt->getType() == NodeType::VarDecl) {
            auto varDecl = static_cast<VarDeclASTNode*>(stmt.get());
            if (varDecl->getInitializer()) {
                varDecl->getInitializer()->accept(this);
            }
        } else {
            stmt->accept(this);
        }
    }

    exitScope();
}

void SemanticAnalyzer::visit(FunctionDeclASTNode* node) {
    currentReturnType = node->getReturnType();
    bool oldAsync = inAsyncScope;
    inAsyncScope = node->getIsAsync();
    enterScope();

    for (const auto& param : node->getParams()) {
        declareVariable(param.name, param.typeName, false);
    }

    if (node->getBody()) {
        node->getBody()->accept(this);
    }

    exitScope();
    inAsyncScope = oldAsync;
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
    for (const auto& method : node->getMethods()) {
        currentReturnType = method->getReturnType();
        enterScope();
        declareVariable("this", node->getName(), false); // Implicit 'this' pointer
        for (const auto& param : method->getParams()) {
            declareVariable(param.name, param.typeName, false);
        }
        if (method->getBody()) {
            method->getBody()->accept(this);
        }
        exitScope();
    }
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

} // namespace vit
