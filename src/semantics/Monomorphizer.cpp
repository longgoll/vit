#include "semantics/Monomorphizer.h"
#include <algorithm>
#include <iostream>

namespace vit {

std::string Monomorphizer::mangleName(const std::string& baseName, const std::vector<std::string>& typeArgs) {
    std::string result = baseName;
    for (const auto& arg : typeArgs) {
        result += "_";
        for (char c : arg) {
            if (std::isalnum(c)) result += c;
            else result += '_';
        }
    }
    return result;
}

std::string Monomorphizer::substituteType(const std::string& typeSpec, const std::unordered_map<std::string, std::string>& typeMap) {
    if (typeSpec.empty()) return typeSpec;

    // Check direct match
    auto it = typeMap.find(typeSpec);
    if (it != typeMap.end()) {
        return it->second;
    }

    // Check array suffix e.g., T[]
    if (typeSpec.length() > 2 && typeSpec.substr(typeSpec.length() - 2) == "[]") {
        std::string elemType = typeSpec.substr(0, typeSpec.length() - 2);
        return substituteType(elemType, typeMap) + "[]";
    }

    // Check generic container e.g., Stack<T>
    size_t openBracket = typeSpec.find('<');
    size_t closeBracket = typeSpec.rfind('>');
    if (openBracket != std::string::npos && closeBracket != std::string::npos && closeBracket > openBracket) {
        std::string base = typeSpec.substr(0, openBracket);
        std::string inner = typeSpec.substr(openBracket + 1, closeBracket - openBracket - 1);
        std::string substitutedInner = substituteType(inner, typeMap);
        return mangleName(base, {substitutedInner});
    }

    return typeSpec;
}

void Monomorphizer::instantiateFunction(const FunctionDeclASTNode* templateFunc, const std::vector<std::string>& typeArgs, const std::string& mangledName) {
    if (instantiatedSignatures.count(mangledName)) return;
    instantiatedSignatures.insert(mangledName);

    std::unordered_map<std::string, std::string> typeMap;
    const auto& genParams = templateFunc->getGenericParams();
    for (size_t i = 0; i < genParams.size() && i < typeArgs.size(); ++i) {
        typeMap[genParams[i]] = typeArgs[i];
    }

    std::vector<Parameter> newParams;
    for (const auto& p : templateFunc->getParams()) {
        newParams.push_back({p.name, substituteType(p.typeName, typeMap)});
    }
    std::string newRetType = substituteType(templateFunc->getReturnType(), typeMap);

    // Deep copy body statements with substituted types
    std::unique_ptr<BlockASTNode> newBody = nullptr;
    if (templateFunc->getBody()) {
        std::vector<std::unique_ptr<StatementNode>> bodyStmts;
        for (const auto& stmt : templateFunc->getBody()->getStatements()) {
            if (stmt->getType() == NodeType::Return) {
                auto retStmt = static_cast<ReturnASTNode*>(stmt.get());
                std::unique_ptr<ExpressionNode> retVal = nullptr;
                if (retStmt->getValue()) {
                    if (retStmt->getValue()->getType() == NodeType::VariableExpr) {
                        auto v = static_cast<VariableExprASTNode*>(retStmt->getValue());
                        retVal = std::make_unique<VariableExprASTNode>(v->getName());
                    } else if (retStmt->getValue()->getType() == NodeType::NumberLiteral) {
                        auto num = static_cast<NumberLiteralASTNode*>(retStmt->getValue());
                        retVal = std::make_unique<NumberLiteralASTNode>(num->getValue());
                    } else if (retStmt->getValue()->getType() == NodeType::StringLiteral) {
                        auto str = static_cast<StringLiteralASTNode*>(retStmt->getValue());
                        retVal = std::make_unique<StringLiteralASTNode>(str->getValue());
                    } else if (retStmt->getValue()->getType() == NodeType::BooleanLiteral) {
                        auto b = static_cast<BooleanLiteralASTNode*>(retStmt->getValue());
                        retVal = std::make_unique<BooleanLiteralASTNode>(b->getValue());
                    }
                }
                bodyStmts.push_back(std::make_unique<ReturnASTNode>(std::move(retVal)));
            }
        }
        newBody = std::make_unique<BlockASTNode>(std::move(bodyStmts));
    }

    auto instFunc = std::make_unique<FunctionDeclASTNode>(
        mangledName, std::move(newParams), newRetType, std::move(newBody), templateFunc->getIsExtern()
    );
    newFunctions.push_back(std::move(instFunc));
}

void Monomorphizer::instantiateStruct(const StructDeclASTNode* templateStruct, const std::vector<std::string>& typeArgs, const std::string& mangledName) {
    if (instantiatedSignatures.count(mangledName)) return;
    instantiatedSignatures.insert(mangledName);

    std::unordered_map<std::string, std::string> typeMap;
    const auto& genParams = templateStruct->getGenericParams();
    for (size_t i = 0; i < genParams.size() && i < typeArgs.size(); ++i) {
        typeMap[genParams[i]] = typeArgs[i];
    }

    std::vector<std::pair<std::string, std::string>> newFields;
    for (const auto& field : templateStruct->getFields()) {
        newFields.push_back({field.first, substituteType(field.second, typeMap)});
    }

    auto instStruct = std::make_unique<StructDeclASTNode>(mangledName, std::move(newFields));
    newTopLevelStmts.push_back(std::move(instStruct));
}

void Monomorphizer::instantiateEnum(const EnumDeclASTNode* templateEnum, const std::vector<std::string>& typeArgs, const std::string& mangledName) {
    if (instantiatedSignatures.count(mangledName)) return;
    instantiatedSignatures.insert(mangledName);

    std::unordered_map<std::string, std::string> typeMap;
    const auto& genParams = templateEnum->getGenericParams();
    for (size_t i = 0; i < genParams.size() && i < typeArgs.size(); ++i) {
        typeMap[genParams[i]] = typeArgs[i];
    }

    std::vector<EnumVariant> newVariants;
    for (const auto& v : templateEnum->getVariants()) {
        std::vector<std::string> newPayloads;
        for (const auto& pt : v.payloadTypes) {
            newPayloads.push_back(substituteType(pt, typeMap));
        }
        newVariants.push_back({v.name, std::move(newPayloads)});
    }

    auto instEnum = std::make_unique<EnumDeclASTNode>(mangledName, std::vector<std::string>{}, std::move(newVariants));
    newTopLevelStmts.push_back(std::move(instEnum));
}

void Monomorphizer::process(ProgramASTNode* program) {
    if (!program) return;

    // Collect generic templates
    for (const auto& func : program->getFunctions()) {
        if (!func->getGenericParams().empty()) {
            genericFunctions[func->getName()] = func.get();
        }
    }
    for (const auto& stmt : program->getTopLevelStatements()) {
        if (stmt->getType() == NodeType::StructDecl) {
            auto s = static_cast<StructDeclASTNode*>(stmt.get());
            if (!s->getGenericParams().empty()) {
                genericStructs[s->getName()] = s;
            }
        } else if (stmt->getType() == NodeType::EnumDecl) {
            auto e = static_cast<EnumDeclASTNode*>(stmt.get());
            if (!e->getGenericParams().empty()) {
                genericEnums[e->getName()] = e;
            }
        }
    }

    // Traverse AST to find generic call sites and type annotations
    program->accept(this);

    // Append newly instantiated concrete functions and types into program AST
    for (auto& fn : newFunctions) {
        program->getFunctions().push_back(std::move(fn));
    }
    for (auto& stmt : newTopLevelStmts) {
        program->getTopLevelStatements().push_back(std::move(stmt));
    }
}

void Monomorphizer::visit(ProgramASTNode* node) {
    for (const auto& func : node->getFunctions()) {
        if (func->getGenericParams().empty()) func->accept(this);
    }
    for (const auto& stmt : node->getTopLevelStatements()) {
        stmt->accept(this);
    }
}

void Monomorphizer::visit(FunctionDeclASTNode* node) {
    if (node->getBody()) node->getBody()->accept(this);
}

void Monomorphizer::visit(BlockASTNode* node) {
    for (const auto& stmt : node->getStatements()) {
        stmt->accept(this);
    }
}

void Monomorphizer::visit(VarDeclASTNode* node) {
    if (node->getInitializer()) node->getInitializer()->accept(this);
}

void Monomorphizer::visit(AssignmentASTNode* node) {
    if (node->getValue()) node->getValue()->accept(this);
}

void Monomorphizer::visit(MemberAssignmentASTNode* node) {
    if (node->getTarget()) node->getTarget()->accept(this);
    if (node->getValue()) node->getValue()->accept(this);
}

void Monomorphizer::visit(ArrayAssignmentASTNode* node) {
    if (node->getArray()) node->getArray()->accept(this);
    if (node->getIndex()) node->getIndex()->accept(this);
    if (node->getValue()) node->getValue()->accept(this);
}

void Monomorphizer::visit(IfASTNode* node) {
    if (node->getCondition()) node->getCondition()->accept(this);
    if (node->getThenBlock()) node->getThenBlock()->accept(this);
    if (node->getElseBlock()) node->getElseBlock()->accept(this);
}

void Monomorphizer::visit(WhileASTNode* node) {
    if (node->getCondition()) node->getCondition()->accept(this);
    if (node->getBody()) node->getBody()->accept(this);
}

void Monomorphizer::visit(ForASTNode* node) {
    if (node->getInit()) node->getInit()->accept(this);
    if (node->getCondition()) node->getCondition()->accept(this);
    if (node->getUpdate()) node->getUpdate()->accept(this);
    if (node->getBody()) node->getBody()->accept(this);
}

void Monomorphizer::visit(BreakASTNode* node) {}
void Monomorphizer::visit(ContinueASTNode* node) {}

void Monomorphizer::visit(ReturnASTNode* node) {
    if (node->getValue()) node->getValue()->accept(this);
}

void Monomorphizer::visit(PrintASTNode* node) {
    if (node->getExpression()) node->getExpression()->accept(this);
}

void Monomorphizer::visit(StructDeclASTNode* node) {
    for (const auto& m : node->getMethods()) m->accept(this);
}

void Monomorphizer::visit(EnumDeclASTNode* node) {}

void Monomorphizer::visit(EnumVariantExprASTNode* node) {
    for (const auto& arg : node->getArgs()) arg->accept(this);
}

void Monomorphizer::visit(MatchASTNode* node) {
    if (node->getTarget()) node->getTarget()->accept(this);
    for (auto& c : node->getCases()) {
        if (c.body) c.body->accept(this);
    }
}

void Monomorphizer::visit(ImportASTNode* node) {}
void Monomorphizer::visit(TypeAliasASTNode* node) {}
void Monomorphizer::visit(NumberLiteralASTNode* node) {}
void Monomorphizer::visit(BooleanLiteralASTNode* node) {}
void Monomorphizer::visit(StringLiteralASTNode* node) {}

void Monomorphizer::visit(ArrayLiteralASTNode* node) {
    for (const auto& elem : node->getElements()) elem->accept(this);
}

void Monomorphizer::visit(VariableExprASTNode* node) {}

void Monomorphizer::visit(MemberAccessASTNode* node) {
    if (node->getTarget()) node->getTarget()->accept(this);
}

void Monomorphizer::visit(ArrayAccessASTNode* node) {
    if (node->getArray()) node->getArray()->accept(this);
    if (node->getIndex()) node->getIndex()->accept(this);
}

void Monomorphizer::visit(UnaryOpASTNode* node) {
    if (node->getOperand()) node->getOperand()->accept(this);
}

void Monomorphizer::visit(BinaryOpASTNode* node) {
    if (node->getLeft()) node->getLeft()->accept(this);
    if (node->getRight()) node->getRight()->accept(this);
}

void Monomorphizer::visit(CallExprASTNode* node) {
    for (const auto& arg : node->getArgs()) arg->accept(this);

    if (!node->getTypeArgs().empty()) {
        auto it = genericFunctions.find(node->getCallee());
        if (it != genericFunctions.end()) {
            std::string mangled = mangleName(node->getCallee(), node->getTypeArgs());
            instantiateFunction(it->second, node->getTypeArgs(), mangled);
            node->setCallee(mangled);
        }
    }
}

void Monomorphizer::visit(MethodCallASTNode* node) {
    if (node->getTarget()) node->getTarget()->accept(this);
    for (const auto& arg : node->getArgs()) arg->accept(this);
}

void Monomorphizer::visit(LambdaASTNode* node) {
    if (node->getBody()) node->getBody()->accept(this);
}

void Monomorphizer::visit(ExpressionStmtASTNode* node) {
    if (node->getExpression()) node->getExpression()->accept(this);
}

void Monomorphizer::visit(NullLiteralASTNode* node) {}

void Monomorphizer::visit(TryExprASTNode* node) {
    if (node->getExpr()) node->getExpr()->accept(this);
}

void Monomorphizer::visit(OptionalChainASTNode* node) {
    if (node->getTarget()) node->getTarget()->accept(this);
    for (const auto& arg : node->getArgs()) arg->accept(this);
}

void Monomorphizer::visit(NullCoalesceASTNode* node) {
    if (node->getLeft()) node->getLeft()->accept(this);
    if (node->getRight()) node->getRight()->accept(this);
}

} // namespace vit
