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
        std::string mangled = mangleName(base, {substitutedInner});
        auto sIt = genericStructs.find(base);
        if (sIt != genericStructs.end()) {
            instantiateStruct(sIt->second, {substitutedInner}, mangled);
        }
        return mangled;
    }

    return typeSpec;
}

static std::unique_ptr<ExpressionNode> cloneExpr(const ExpressionNode* expr) {
    if (!expr) return nullptr;
    switch (expr->getType()) {
        case NodeType::NumberLiteral: {
            auto n = static_cast<const NumberLiteralASTNode*>(expr);
            return std::make_unique<NumberLiteralASTNode>(n->getValue());
        }
        case NodeType::BooleanLiteral: {
            auto b = static_cast<const BooleanLiteralASTNode*>(expr);
            return std::make_unique<BooleanLiteralASTNode>(b->getValue());
        }
        case NodeType::StringLiteral: {
            auto s = static_cast<const StringLiteralASTNode*>(expr);
            return std::make_unique<StringLiteralASTNode>(s->getValue());
        }
        case NodeType::VariableExpr: {
            auto v = static_cast<const VariableExprASTNode*>(expr);
            return std::make_unique<VariableExprASTNode>(v->getName());
        }
        case NodeType::MemberAccess: {
            auto m = static_cast<const MemberAccessASTNode*>(expr);
            return std::make_unique<MemberAccessASTNode>(cloneExpr(m->getTarget()), m->getMember());
        }
        case NodeType::ArrayAccess: {
            auto a = static_cast<const ArrayAccessASTNode*>(expr);
            return std::make_unique<ArrayAccessASTNode>(cloneExpr(a->getArray()), cloneExpr(a->getIndex()));
        }
        case NodeType::UnaryOp: {
            auto u = static_cast<const UnaryOpASTNode*>(expr);
            return std::make_unique<UnaryOpASTNode>(u->getOp(), cloneExpr(u->getOperand()));
        }
        case NodeType::BinaryOp: {
            auto b = static_cast<const BinaryOpASTNode*>(expr);
            return std::make_unique<BinaryOpASTNode>(b->getOp(), cloneExpr(b->getLeft()), cloneExpr(b->getRight()));
        }
        case NodeType::CallExpr: {
            auto c = static_cast<const CallExprASTNode*>(expr);
            std::vector<std::unique_ptr<ExpressionNode>> args;
            for (const auto& arg : c->getArgs()) {
                args.push_back(cloneExpr(arg.get()));
            }
            return std::make_unique<CallExprASTNode>(c->getCallee(), std::move(args));
        }
        case NodeType::MethodCall: {
            auto m = static_cast<const MethodCallASTNode*>(expr);
            std::vector<std::unique_ptr<ExpressionNode>> args;
            for (const auto& arg : m->getArgs()) {
                args.push_back(cloneExpr(arg.get()));
            }
            return std::make_unique<MethodCallASTNode>(cloneExpr(m->getTarget()), m->getMethod(), std::move(args));
        }
        case NodeType::NullLiteral: {
            return std::make_unique<NullLiteralASTNode>();
        }
        case NodeType::ArrayLiteral: {
            auto a = static_cast<const ArrayLiteralASTNode*>(expr);
            std::vector<std::unique_ptr<ExpressionNode>> elems;
            for (const auto& elem : a->getElements()) {
                elems.push_back(cloneExpr(elem.get()));
            }
            return std::make_unique<ArrayLiteralASTNode>(std::move(elems));
        }
        case NodeType::TryExpr: {
            auto t = static_cast<const TryExprASTNode*>(expr);
            return std::make_unique<TryExprASTNode>(cloneExpr(t->getExpr()));
        }
        case NodeType::OptionalChain: {
            auto o = static_cast<const OptionalChainASTNode*>(expr);
            std::vector<std::unique_ptr<ExpressionNode>> args;
            for (const auto& arg : o->getArgs()) {
                args.push_back(cloneExpr(arg.get()));
            }
            return std::make_unique<OptionalChainASTNode>(cloneExpr(o->getTarget()), o->getMember(), o->getIsMethodCall(), std::move(args));
        }
        case NodeType::NullCoalesce: {
            auto n = static_cast<const NullCoalesceASTNode*>(expr);
            return std::make_unique<NullCoalesceASTNode>(cloneExpr(n->getLeft()), cloneExpr(n->getRight()));
        }
        case NodeType::AwaitExpr: {
            auto a = static_cast<const AwaitExprASTNode*>(expr);
            return std::make_unique<AwaitExprASTNode>(cloneExpr(a->getExpr()));
        }
        case NodeType::EnumVariantExpr: {
            auto ev = static_cast<const EnumVariantExprASTNode*>(expr);
            std::vector<std::unique_ptr<ExpressionNode>> args;
            for (const auto& arg : ev->getArgs()) {
                args.push_back(cloneExpr(arg.get()));
            }
            return std::make_unique<EnumVariantExprASTNode>(ev->getEnumName(), ev->getVariantName(), std::move(args));
        }
        default:
            return nullptr;
    }
}

static std::unique_ptr<StatementNode> cloneStmt(const StatementNode* stmt, const std::unordered_map<std::string, std::string>& typeMap, Monomorphizer* mono);

static std::unique_ptr<BlockASTNode> cloneBlock(const BlockASTNode* block, const std::unordered_map<std::string, std::string>& typeMap, Monomorphizer* mono) {
    if (!block) return nullptr;
    std::vector<std::unique_ptr<StatementNode>> stmts;
    for (const auto& stmt : block->getStatements()) {
        auto cloned = cloneStmt(stmt.get(), typeMap, mono);
        if (cloned) stmts.push_back(std::move(cloned));
    }
    return std::make_unique<BlockASTNode>(std::move(stmts));
}

static std::unique_ptr<StatementNode> cloneStmt(const StatementNode* stmt, const std::unordered_map<std::string, std::string>& typeMap, Monomorphizer* mono) {
    if (!stmt) return nullptr;
    switch (stmt->getType()) {
        case NodeType::Block: {
            auto b = static_cast<const BlockASTNode*>(stmt);
            return cloneBlock(b, typeMap, mono);
        }
        case NodeType::VarDecl: {
            auto v = static_cast<const VarDeclASTNode*>(stmt);
            std::string substituted = mono->substituteType(v->getTypeName(), typeMap);
            return std::make_unique<VarDeclASTNode>(v->getIsConst(), v->getName(), substituted, cloneExpr(v->getInitializer()));
        }
        case NodeType::Assignment: {
            auto a = static_cast<const AssignmentASTNode*>(stmt);
            return std::make_unique<AssignmentASTNode>(a->getName(), cloneExpr(a->getValue()));
        }
        case NodeType::MemberAssignment: {
            auto m = static_cast<const MemberAssignmentASTNode*>(stmt);
            return std::make_unique<MemberAssignmentASTNode>(cloneExpr(m->getTarget()), m->getMember(), cloneExpr(m->getValue()));
        }
        case NodeType::ArrayAssignment: {
            auto a = static_cast<const ArrayAssignmentASTNode*>(stmt);
            return std::make_unique<ArrayAssignmentASTNode>(cloneExpr(a->getArray()), cloneExpr(a->getIndex()), cloneExpr(a->getValue()));
        }
        case NodeType::If: {
            auto i = static_cast<const IfASTNode*>(stmt);
            return std::make_unique<IfASTNode>(cloneExpr(i->getCondition()), cloneBlock(i->getThenBlock(), typeMap, mono), cloneBlock(i->getElseBlock(), typeMap, mono));
        }
        case NodeType::While: {
            auto w = static_cast<const WhileASTNode*>(stmt);
            return std::make_unique<WhileASTNode>(cloneExpr(w->getCondition()), cloneBlock(w->getBody(), typeMap, mono));
        }
        case NodeType::For: {
            auto f = static_cast<const ForASTNode*>(stmt);
            return std::make_unique<ForASTNode>(cloneStmt(f->getInit(), typeMap, mono), cloneExpr(f->getCondition()), cloneStmt(f->getUpdate(), typeMap, mono), cloneBlock(f->getBody(), typeMap, mono));
        }
        case NodeType::Break: {
            return std::make_unique<BreakASTNode>();
        }
        case NodeType::Continue: {
            return std::make_unique<ContinueASTNode>();
        }
        case NodeType::ExpressionStmt: {
            auto e = static_cast<const ExpressionStmtASTNode*>(stmt);
            return std::make_unique<ExpressionStmtASTNode>(cloneExpr(e->getExpression()));
        }
        case NodeType::Return: {
            auto r = static_cast<const ReturnASTNode*>(stmt);
            return std::make_unique<ReturnASTNode>(cloneExpr(r->getValue()));
        }
        case NodeType::Print: {
            auto p = static_cast<const PrintASTNode*>(stmt);
            return std::make_unique<PrintASTNode>(cloneExpr(p->getExpression()));
        }
        default:
            return nullptr;
    }
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
            auto cloned = cloneStmt(stmt.get(), typeMap, this);
            if (cloned) {
                bodyStmts.push_back(std::move(cloned));
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

    std::vector<std::unique_ptr<FunctionDeclASTNode>> newMethods;
    for (const auto& method : templateStruct->getMethods()) {
        std::vector<Parameter> newParams;
        for (const auto& p : method->getParams()) {
            newParams.push_back({p.name, substituteType(p.typeName, typeMap)});
        }
        std::string newRetType = substituteType(method->getReturnType(), typeMap);
        std::unique_ptr<BlockASTNode> methodBody = nullptr;
        if (method->getBody()) {
            std::vector<std::unique_ptr<StatementNode>> bodyStmts;
            for (const auto& stmt : method->getBody()->getStatements()) {
                auto cloned = cloneStmt(stmt.get(), typeMap, this);
                if (cloned) bodyStmts.push_back(std::move(cloned));
            }
            methodBody = std::make_unique<BlockASTNode>(std::move(bodyStmts));
        }
        newMethods.push_back(std::make_unique<FunctionDeclASTNode>(
            method->getName(), std::move(newParams), newRetType, std::move(methodBody), method->getIsExtern()
        ));
    }

    auto instStruct = std::make_unique<StructDeclASTNode>(mangledName, std::move(newFields), std::move(newMethods));
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

void Monomorphizer::visit(AwaitExprASTNode* node) {
    if (node->getExpr()) node->getExpr()->accept(this);
}

} // namespace vit
