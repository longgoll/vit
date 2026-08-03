#include "semantics/SemanticAnalyzer.h"
#include <iostream>

namespace vit {

// ─── Expression / literal visitors ───────────────────────────────────────────

void SemanticAnalyzer::visit(NumberLiteralASTNode* node) {
    lastInferredType = "number";
}

void SemanticAnalyzer::visit(BooleanLiteralASTNode* node) {
    lastInferredType = "boolean";
}

void SemanticAnalyzer::visit(StringLiteralASTNode* node) {
    lastInferredType = "string";
}

void SemanticAnalyzer::visit(NullLiteralASTNode* node) {
    lastInferredType = "null";
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
        if (enumTable.find(node->getName()) != enumTable.end()) {
            lastInferredType = node->getName();
            return;
        }
        if (functionTable.find(node->getName()) != functionTable.end()) {
            lastInferredType = "function";
            return;
        }
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

    if (node->getMember() == "length") {
        if (structType == "string" || (structType.size() > 2 && structType.substr(structType.size() - 2) == "[]")) {
            lastInferredType = "number";
            return;
        }
    }

    auto enumIt = enumTable.find(structType);
    if (enumIt != enumTable.end()) {
        for (const auto& var : enumIt->second) {
            if (var.name == node->getMember()) {
                lastInferredType = structType;
                return;
            }
        }
    }

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
    if (op == "+") {
        if (leftType == "string" || rightType == "string") {
            lastInferredType = "string";
        } else {
            lastInferredType = "number";
        }
    } else if (op == "&&" || op == "||" || op == ">" || op == "<" || op == "==" || op == "!=" || op == ">=" || op == "<=") {
        lastInferredType = "boolean";
    } else {
        lastInferredType = "number";
    }
}

void SemanticAnalyzer::visit(TypeAliasASTNode* node) {
    typeAliasTable[node->getAliasName()] = node->getTypeSpec();
}

void SemanticAnalyzer::visit(LambdaASTNode* node) {
    enterScope();
    for (const auto& param : node->getParams()) {
        declareVariable(param.name, resolveType(param.typeName), false);
    }

    std::string bodyType = "void";
    if (node->getBody()) {
        std::string oldRet = currentReturnType;
        node->getBody()->accept(this);
        bodyType = lastInferredType;
        currentReturnType = oldRet;
    }

    exitScope();

    if (node->getReturnType().empty()) {
        node->setReturnType(bodyType);
    }

    std::string fnType = "(";
    for (size_t i = 0; i < node->getParams().size(); ++i) {
        fnType += resolveType(node->getParams()[i].typeName);
        if (i + 1 < node->getParams().size()) fnType += ", ";
    }
    fnType += ") => " + resolveType(node->getReturnType());

    lastInferredType = fnType;
}

void SemanticAnalyzer::visit(CallExprASTNode* node) {
    for (const auto& arg : node->getArgs()) {
        arg->accept(this);
    }

    auto it = functionTable.find(node->getCallee());
    if (it != functionTable.end()) {
        const auto& expectedParamTypes = it->second.second;
        if (!expectedParamTypes.empty() && node->getArgs().size() != expectedParamTypes.size()) {
            reportError("Function '" + node->getCallee() + "' expects " +
                        std::to_string(expectedParamTypes.size()) + " argument(s), but got " +
                        std::to_string(node->getArgs().size()) + ".");
        }
        lastInferredType = resolveType(it->second.first);
    } else {
        const SymbolInfo* sym = lookupVariable(node->getCallee());
        if (sym) {
            std::string resolved = resolveType(sym->typeName);
            size_t arrowPos = resolved.rfind("=> ");
            if (arrowPos != std::string::npos) {
                lastInferredType = resolved.substr(arrowPos + 3);
            } else {
                lastInferredType = "unknown";
            }
        } else {
            reportError("Call to undefined function or variable '" + node->getCallee() + "'.");
            lastInferredType = "unknown";
        }
    }
}

void SemanticAnalyzer::visit(MethodCallASTNode* node) {
    if (node->getTarget()) {
        node->getTarget()->accept(this);
    }
    std::string targetType = resolveType(lastInferredType);

    if (targetType.size() > 2 && targetType.substr(targetType.size() - 2) == "[]") {
        std::string elemType = targetType.substr(0, targetType.size() - 2);
        const std::string& method = node->getMethod();

        if (method == "map" || method == "filter" || method == "forEach") {
            if (!node->getArgs().empty()) {
                node->getArgs()[0]->accept(this);
                std::string lambdaType = resolveType(lastInferredType);

                if (method == "map") {
                    size_t arrowPos = lambdaType.rfind("=> ");
                    std::string retType = (arrowPos != std::string::npos) ? lambdaType.substr(arrowPos + 3) : elemType;
                    lastInferredType = retType + "[]";
                } else if (method == "filter") {
                    lastInferredType = targetType;
                } else if (method == "forEach") {
                    lastInferredType = "void";
                }
                return;
            }
        }
    }

    for (const auto& arg : node->getArgs()) {
        arg->accept(this);
    }

    auto structIt = structMethodsTable.find(targetType);
    if (structIt != structMethodsTable.end()) {
        auto methodIt = structIt->second.find(node->getMethod());
        if (methodIt != structIt->second.end()) {
            lastInferredType = resolveType(methodIt->second.first);
            return;
        }
    }

    reportError("Call to undefined method '" + node->getMethod() + "' on type '" + targetType + "'.");
    lastInferredType = "unknown";
}

void SemanticAnalyzer::visit(EnumDeclASTNode* node) {
    enumTable[node->getName()] = node->getVariants();
}

void SemanticAnalyzer::visit(EnumVariantExprASTNode* node) {
    auto it = enumTable.find(node->getEnumName());
    if (it != enumTable.end()) {
        for (const auto& arg : node->getArgs()) {
            arg->accept(this);
        }
        lastInferredType = node->getEnumName();
        return;
    }
    reportError("Unknown enum '" + node->getEnumName() + "'.");
    lastInferredType = "unknown";
}

void SemanticAnalyzer::visit(MatchASTNode* node) {
    if (node->getTarget()) {
        node->getTarget()->accept(this);
    }

    for (auto& c : node->getCases()) {
        enterScope();
        if (!c.bindings.empty()) {
            for (const auto& b : c.bindings) {
                declareVariable(b, "number", false);
            }
        }
        if (c.body) {
            c.body->accept(this);
        }
        exitScope();
    }
    lastInferredType = "void";
}

void SemanticAnalyzer::visit(ExpressionStmtASTNode* node) {
    if (node->getExpression()) {
        node->getExpression()->accept(this);
    }
}

void SemanticAnalyzer::visit(TryExprASTNode* node) {
    if (node->getExpr()) {
        node->getExpr()->accept(this);
    }
    std::string type = lastInferredType;
    if (type.rfind("Result<", 0) == 0) {
        size_t commaPos = type.find(',');
        if (commaPos != std::string::npos) {
            type = type.substr(7, commaPos - 7);
        } else {
            type = type.substr(7, type.length() - 8);
        }
    } else if (type.rfind("Option<", 0) == 0) {
        type = type.substr(7, type.length() - 8);
    } else if (!type.empty() && type.back() == '?') {
        type.pop_back();
    }
    lastInferredType = type;
}

void SemanticAnalyzer::visit(OptionalChainASTNode* node) {
    if (node->getTarget()) {
        node->getTarget()->accept(this);
    }
    std::string targetType = lastInferredType;
    if (!targetType.empty() && targetType.back() == '?') {
        targetType.pop_back();
    }

    std::string resType = "unknown";
    auto sIt = structTable.find(targetType);
    if (sIt != structTable.end()) {
        for (const auto& field : sIt->second) {
            if (field.first == node->getMember()) {
                resType = field.second;
                break;
            }
        }
    }
    auto mIt = structMethodsTable.find(targetType);
    if (mIt != structMethodsTable.end()) {
        auto methodIt = mIt->second.find(node->getMember());
        if (methodIt != mIt->second.end()) {
            resType = methodIt->second.first;
        }
    }
    for (const auto& arg : node->getArgs()) {
        arg->accept(this);
    }
    if (!resType.empty() && resType.back() != '?') {
        resType += "?";
    }
    lastInferredType = resType;
}

void SemanticAnalyzer::visit(NullCoalesceASTNode* node) {
    std::string leftType;
    if (node->getLeft()) {
        node->getLeft()->accept(this);
        leftType = lastInferredType;
    }
    if (node->getRight()) {
        node->getRight()->accept(this);
    }
    if (!leftType.empty() && leftType.back() == '?') {
        leftType.pop_back();
    }
    lastInferredType = leftType.empty() ? lastInferredType : leftType;
}

void SemanticAnalyzer::visit(AwaitExprASTNode* node) {
    if (!inAsyncScope) {
        reportError("'await' expression is only allowed inside an 'async' function.");
    }
    if (node->getExpr()) {
        node->getExpr()->accept(this);
    }
    // Unwrap Promise<T> if present
    std::string type = lastInferredType;
    if (type.rfind("Promise<", 0) == 0 && type.back() == '>') {
        type = type.substr(8, type.size() - 9);
    }
    lastInferredType = type;
}

} // namespace vit
