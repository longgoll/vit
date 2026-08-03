#include "codegen/LLVMCodeGen.h"
#include "lexer/Lexer.h"
#include "parser/Parser.h"
#include <iomanip>
#include <iostream>

namespace vit {

// Statement and Declaration AST Visitors for LLVMCodeGen

void LLVMCodeGen::visit(ProgramASTNode* node) {
    currentProgram = node;
    // Pass 0: Register type aliases
    for (const auto& stmt : node->getTopLevelStatements()) {
        if (stmt->getType() == NodeType::TypeAlias) {
            auto alias = static_cast<TypeAliasASTNode*>(stmt.get());
            typeAliases[alias->getAliasName()] = alias->getTypeSpec();
        }
    }

    // Pass 0.5: Register global variables for top-level VarDecls
    for (const auto& stmt : node->getTopLevelStatements()) {
        if (stmt->getType() == NodeType::VarDecl) {
            auto varDecl = static_cast<VarDeclASTNode*>(stmt.get());
            std::string typeName = varDecl->getTypeName();
            if (typeName.empty() && varDecl->getInitializer()) {
                if (varDecl->getInitializer()->getType() == NodeType::StringLiteral) typeName = "string";
                else if (varDecl->getInitializer()->getType() == NodeType::BooleanLiteral) typeName = "boolean";
                else if (varDecl->getInitializer()->getType() == NodeType::NumberLiteral) typeName = "int";
            }
            if (typeName.empty()) typeName = "string";
            std::string llvmType = getLLVMType(typeName);
            std::string gName = "@" + varDecl->getName() + ".addr";

            globalDefsStream << gName << " = global " << llvmType << " ";
            if (llvmType == "i8*" || llvmType.back() == '*') globalDefsStream << "null\n";
            else if (llvmType == "i64") globalDefsStream << "0\n";
            else if (llvmType == "i1") globalDefsStream << "false\n";
            else globalDefsStream << "0.000000e+00\n";

            globalSymbolTable[varDecl->getName()] = {gName, typeName};
        }
    }

    // Pass 1: Register struct definitions and method return types
    for (const auto& stmt : node->getTopLevelStatements()) {
        if (stmt->getType() == NodeType::StructDecl) {
            auto structDecl = static_cast<StructDeclASTNode*>(stmt.get());
            StructInfo info;
            info.name = structDecl->getName();
            info.fields = structDecl->getFields();
            for (int i = 0; i < (int)structDecl->getFields().size(); ++i) {
                info.fieldIndices[structDecl->getFields()[i].first] = i;
            }
            structs[structDecl->getName()] = info;

            globalDefsStream << "%struct." << structDecl->getName() << " = type { ";
            for (size_t i = 0; i < structDecl->getFields().size(); ++i) {
                globalDefsStream << getLLVMType(structDecl->getFields()[i].second);
                if (i + 1 < structDecl->getFields().size()) globalDefsStream << ", ";
            }
            globalDefsStream << " }\n";

            for (const auto& method : structDecl->getMethods()) {
                std::string mangledName = "_" + structDecl->getName() + "_" + method->getName();
                functionReturnTypes[mangledName] = method->getReturnType();
            }
        } else if (stmt->getType() == NodeType::EnumDecl) {
            auto enumDecl = static_cast<EnumDeclASTNode*>(stmt.get());
            enums.insert(enumDecl->getName());
            globalDefsStream << "%struct." << enumDecl->getName() << " = type { i32, [8 x i8] }\n";
        }
    }

    // Pass 2: Register top-level function return types and param types
    for (const auto& func : node->getFunctions()) {
        if (func->getIsAsync()) {
            functionReturnTypes[func->getName()] = "Promise";
        } else {
            functionReturnTypes[func->getName()] = func->getReturnType();
        }
        std::vector<std::string> pTypes;
        for (const auto& p : func->getParams()) {
            pTypes.push_back(p.typeName);
        }
        functionParamTypes[func->getName()] = pTypes;
    }

    // Pass 3: Emit IR for struct methods and functions
    for (const auto& stmt : node->getTopLevelStatements()) {
        if (stmt->getType() == NodeType::StructDecl) {
            stmt->accept(this);
        }
    }

    for (const auto& func : node->getFunctions()) {
        func->accept(this);
    }
}

void LLVMCodeGen::visit(StructDeclASTNode* node) {
    for (const auto& method : node->getMethods()) {
        std::string mangledName = "_" + node->getName() + "_" + method->getName();
        functionReturnTypes[mangledName] = method->getReturnType();

        std::vector<Parameter> methodParams;
        methodParams.push_back(Parameter{"this", node->getName()});
        for (const auto& p : method->getParams()) {
            methodParams.push_back(p);
        }

        symbolTable.clear();
        heapScopeStack.clear();
        regCounter = 0;
        blockHasTerminator = false;

        currentFunctionReturnType = method->getReturnType();
        std::string llvmRetType = getLLVMType(currentFunctionReturnType);

        irStream << "define " << llvmRetType << " @" << mangledName << "(";
        for (size_t i = 0; i < methodParams.size(); ++i) {
            irStream << getLLVMType(methodParams[i].typeName) << " %" << methodParams[i].name;
            if (i + 1 < methodParams.size()) irStream << ", ";
        }
        irStream << ") {\n";

        currentBlockLabel = "entry";
        irStream << "entry:\n";

        for (const auto& param : methodParams) {
            std::string paramLLVMType = getLLVMType(param.typeName);
            std::string addrReg = newReg();
            emitIndent();
            irStream << addrReg << " = alloca " << paramLLVMType << ", align 8\n";
            emitIndent();
            irStream << "store " << paramLLVMType << " %" << param.name << ", " << paramLLVMType << "* " << addrReg << ", align 8\n";
            symbolTable[param.name] = {addrReg, param.typeName};
        }

        if (method->getBody()) {
            method->getBody()->accept(this);
        }

        if (!blockHasTerminator) {
            emitIndent();
            if (llvmRetType == "void") {
                irStream << "ret void\n";
            } else if (llvmRetType == "i1") {
                irStream << "ret i1 false\n";
            } else if (llvmRetType == "i8*") {
                irStream << "ret i8* null\n";
            } else {
                irStream << "ret double 0.000000e+00\n";
            }
        }

        irStream << "}\n\n";
    }
}

void LLVMCodeGen::cleanupCurrentScope() {
    if (heapScopeStack.empty()) return;
    auto scopeVars = heapScopeStack.back();
    heapScopeStack.pop_back();

    for (const auto& var : scopeVars) {
        if (var.typeName.size() > 2 && var.typeName.substr(var.typeName.size() - 2) == "[]") {
            std::string llvmType = getLLVMType(var.typeName);
            std::string ptrReg = newReg();
            emitIndent();
            irStream << ptrReg << " = load " << llvmType << ", " << llvmType << "* " << var.addrReg << ", align 8\n";
            
            std::string baseDoublePtr = newReg();
            emitIndent();
            irStream << baseDoublePtr << " = getelementptr inbounds double, double* " << ptrReg << ", i64 -1\n";
            std::string i8Ptr = newReg();
            emitIndent();
            irStream << i8Ptr << " = bitcast double* " << baseDoublePtr << " to i8*\n";
            emitIndent();
            irStream << "call void @free(i8* " << i8Ptr << ")\n";
        }
    }
}

void LLVMCodeGen::visit(ImportASTNode* node) {
    // Module importing resolved at parser/module pass level
}

void LLVMCodeGen::visit(FunctionDeclASTNode* node) {
    symbolTable.clear();
    heapScopeStack.clear();
    regCounter = 0;
    blockHasTerminator = false;

    currentFunctionName = node->getName();
    currentFunctionReturnType = node->getReturnType();
    currentFunctionIsAsync = (currentFunctionName == "main") ? false : node->getIsAsync();
    std::string llvmRetType = (currentFunctionName == "main") ? "i32" : (currentFunctionIsAsync ? "i8*" : getLLVMType(currentFunctionReturnType));

    if (node->getIsExtern()) {
        if (declaredFunctions.count(node->getName())) {
            return;
        }
        declaredFunctions.insert(node->getName());
        irStream << "declare " << llvmRetType << " @" << node->getName() << "(";
        const auto& params = node->getParams();
        for (size_t i = 0; i < params.size(); ++i) {
            irStream << getLLVMType(params[i].typeName);
            if (i + 1 < params.size()) irStream << ", ";
        }
        irStream << ")\n\n";
        return;
    }

    irStream << "define " << llvmRetType << " @" << node->getName() << "(";
    const auto& params = node->getParams();
    if (currentFunctionName == "main") {
        irStream << "i32 %argc, i8** %argv";
    } else {
        for (size_t i = 0; i < params.size(); ++i) {
            std::string pType = getLLVMType(params[i].typeName);
            irStream << pType << " %" << params[i].name;
            if (i + 1 < params.size()) irStream << ", ";
        }
    }
    irStream << ") {\n";
    irStream << "entry:\n";
    currentBlockLabel = "entry";

    if (currentFunctionName == "main") {
        emitIndent();
        irStream << "call void @__vit_init_args(i32 %argc, i8** %argv)\n";
        if (currentProgram) {
            for (const auto& stmt : currentProgram->getTopLevelStatements()) {
                if (stmt->getType() == NodeType::VarDecl || stmt->getType() == NodeType::Assignment || stmt->getType() == NodeType::ExpressionStmt) {
                    stmt->accept(this);
                }
            }
        }
    }

    if (currentFunctionIsAsync) {
        std::string promReg = newReg();
        emitIndent();
        irStream << promReg << " = call i8* @vit_promise_create()\n";
        currentPromiseReg = promReg;
    }

    for (const auto& param : params) {
        std::string addrReg = "%" + param.name + ".addr";
        std::string pType = getLLVMType(param.typeName);

        emitIndent();
        irStream << addrReg << " = alloca " << pType << ", align 8\n";
        emitIndent();
        irStream << "store " << pType << " %" << param.name << ", " << pType << "* " << addrReg << ", align 8\n";

        symbolTable[param.name] = {addrReg, param.typeName};
    }

    if (node->getBody()) {
        node->getBody()->accept(this);
    }

    if (!blockHasTerminator) {
        emitIndent();
        if (currentFunctionName == "main") {
            irStream << "call i32 @fflush(i8* null)\n";
            emitIndent();
            irStream << "ret i32 0\n";
        } else if (currentFunctionIsAsync) {
            emitIndent();
            irStream << "call void @vit_promise_resolve(i8* " << currentPromiseReg << ", double 0.000000e+00)\n";
            emitIndent();
            irStream << "ret i8* " << currentPromiseReg << "\n";
        } else if (llvmRetType == "void") {
            irStream << "ret void\n";
        } else if (llvmRetType == "i1") {
            irStream << "ret i1 false\n";
        } else if (llvmRetType == "i8*" || (!llvmRetType.empty() && llvmRetType.back() == '*')) {
            irStream << "ret " << llvmRetType << " null\n";
        } else {
            irStream << "ret double 0.000000e+00\n";
        }
    }
    irStream << "}\n\n";
}

void LLVMCodeGen::visit(BlockASTNode* node) {
    heapScopeStack.push_back({});
    for (const auto& stmt : node->getStatements()) {
        if (blockHasTerminator) break;
        stmt->accept(this);
    }
    if (!blockHasTerminator) {
        cleanupCurrentScope();
    } else if (!heapScopeStack.empty()) {
        heapScopeStack.pop_back();
    }
}

void LLVMCodeGen::visit(VarDeclASTNode* node) {
    const VarSymbol* sym = findSymbol(node->getName());
    bool isGlobal = (sym && !sym->addrReg.empty() && sym->addrReg[0] == '@');

    std::string addrReg = isGlobal ? sym->addrReg : ("%" + node->getName() + ".addr");
    std::string typeName = node->getTypeName();

    std::string valReg = "";
    if (node->getInitializer()) {
        node->getInitializer()->accept(this);
        valReg = lastResultReg;
        if (typeName.empty()) {
            typeName = lastResultType;
        }
    }

    if (typeName.empty()) typeName = "number";
    std::string llvmType = getLLVMType(typeName);

    if (!isGlobal) {
        emitIndent();
        irStream << addrReg << " = alloca " << llvmType << ", align 8\n";
        symbolTable[node->getName()] = {addrReg, typeName};
    }

    if (!heapScopeStack.empty() && typeName.size() > 2 && typeName.substr(typeName.size() - 2) == "[]") {
        heapScopeStack.back().push_back({addrReg, typeName});
    }

    if (auto it = structs.find(typeName); it != structs.end()) {
        // Allocate struct memory on stack
        std::string allocReg = newReg();
        emitIndent();
        irStream << allocReg << " = alloca %struct." << typeName << ", align 8\n";
        emitIndent();
        irStream << "store %struct." << typeName << "* " << allocReg << ", %struct." << typeName << "** " << addrReg << ", align 8\n";
    }

    if (!valReg.empty()) {
        if (llvmType == "double" && lastResultType == "int") {
            std::string dReg = newReg();
            emitIndent();
            irStream << dReg << " = sitofp i64 " << valReg << " to double\n";
            valReg = dReg;
        } else if (llvmType == "i64" && lastResultType == "number") {
            std::string iReg = newReg();
            emitIndent();
            irStream << iReg << " = fptosi double " << valReg << " to i64\n";
            valReg = iReg;
        }
        emitIndent();
        irStream << "store " << llvmType << " " << valReg << ", " << llvmType << "* " << addrReg << ", align 8\n";
    }
}

void LLVMCodeGen::visit(AssignmentASTNode* node) {
    const VarSymbol* sym = findSymbol(node->getName());
    std::string addrReg = sym ? sym->addrReg : ("%" + node->getName() + ".addr");
    std::string typeName = sym ? sym->typeName : "number";

    std::string llvmType = getLLVMType(typeName);

    if (node->getValue()) {
        node->getValue()->accept(this);
        std::string valReg = lastResultReg;

        if (llvmType == "double" && lastResultType == "int") {
            std::string dReg = newReg();
            emitIndent();
            irStream << dReg << " = sitofp i64 " << valReg << " to double\n";
            valReg = dReg;
        } else if (llvmType == "i64" && lastResultType == "number") {
            std::string iReg = newReg();
            emitIndent();
            irStream << iReg << " = fptosi double " << valReg << " to i64\n";
            valReg = iReg;
        }

        emitIndent();
        irStream << "store " << llvmType << " " << valReg << ", " << llvmType << "* " << addrReg << ", align 8\n";
    }
}

void LLVMCodeGen::visit(MemberAssignmentASTNode* node) {
    node->getTarget()->accept(this);
    std::string structPtrReg = lastResultReg;
    std::string structType = lastResultType;

    node->getValue()->accept(this);
    std::string valReg = lastResultReg;

    auto it = structs.find(structType);
    if (it != structs.end()) {
        int idx = it->second.fieldIndices[node->getMember()];
        std::string fieldType = it->second.fields[idx].second;
        std::string llvmFieldType = getLLVMType(fieldType);

        std::string gepReg = newReg();
        emitIndent();
        irStream << gepReg << " = getelementptr inbounds %struct." << structType << ", %struct." << structType << "* " << structPtrReg << ", i32 0, i32 " << idx << "\n";
        emitIndent();
        irStream << "store " << llvmFieldType << " " << valReg << ", " << llvmFieldType << "* " << gepReg << ", align 8\n";
    }
}

void LLVMCodeGen::visit(ArrayAssignmentASTNode* node) {
    node->getArray()->accept(this);
    std::string arrPtrReg = lastResultReg;

    node->getIndex()->accept(this);
    std::string idxReg = lastResultReg;
    std::string idxI64 = idxReg;
    if (lastResultType == "number") {
        idxI64 = newReg();
        emitIndent();
        irStream << idxI64 << " = fptosi double " << idxReg << " to i64\n";
    }

    std::string gepReg = newReg();
    emitIndent();
    irStream << gepReg << " = getelementptr inbounds double, double* " << arrPtrReg << ", i64 " << idxI64 << "\n";

    node->getValue()->accept(this);
    std::string valReg = lastResultReg;

    emitIndent();
    irStream << "store double " << valReg << ", double* " << gepReg << ", align 8\n";
}

void LLVMCodeGen::visit(IfASTNode* node) {
    std::string thenLabel = newLabel("if.then");
    std::string elseLabel = newLabel("if.else");
    std::string mergeLabel = newLabel("if.end");

    node->getCondition()->accept(this);
    std::string condVal = lastResultReg;

    std::string condI1 = condVal;
    if (lastResultType == "number") {
        condI1 = newReg();
        emitIndent();
        irStream << condI1 << " = fcmp one double " << condVal << ", 0.000000e+00\n";
    }

    emitIndent();
    irStream << "br i1 " << condI1 << ", label %" << thenLabel << ", label %" << (node->getElseBlock() ? elseLabel : mergeLabel) << "\n\n";

    irStream << thenLabel << ":\n";
    currentBlockLabel = thenLabel;
    blockHasTerminator = false;
    if (node->getThenBlock()) {
        node->getThenBlock()->accept(this);
    }
    bool thenTerminated = blockHasTerminator;
    if (!thenTerminated) {
        emitIndent();
        irStream << "br label %" << mergeLabel << "\n\n";
    }

    bool elseTerminated = false;
    if (node->getElseBlock()) {
        irStream << elseLabel << ":\n";
        currentBlockLabel = elseLabel;
        blockHasTerminator = false;
        node->getElseBlock()->accept(this);
        elseTerminated = blockHasTerminator;
        if (!elseTerminated) {
            emitIndent();
            irStream << "br label %" << mergeLabel << "\n\n";
        }
    }

    if (!thenTerminated || !elseTerminated || !node->getElseBlock()) {
        irStream << mergeLabel << ":\n";
        currentBlockLabel = mergeLabel;
        blockHasTerminator = false;
    } else {
        blockHasTerminator = true;
    }
}

void LLVMCodeGen::visit(WhileASTNode* node) {
    std::string condLabel = newLabel("while.cond");
    std::string bodyLabel = newLabel("while.body");
    std::string endLabel = newLabel("while.end");

    loopStack.push_back({endLabel, condLabel});

    emitIndent();
    irStream << "br label %" << condLabel << "\n\n";

    irStream << condLabel << ":\n";
    currentBlockLabel = condLabel;
    blockHasTerminator = false;

    node->getCondition()->accept(this);
    std::string condVal = lastResultReg;

    std::string condI1 = condVal;
    if (lastResultType == "number") {
        condI1 = newReg();
        emitIndent();
        irStream << condI1 << " = fcmp one double " << condVal << ", 0.000000e+00\n";
    }

    emitIndent();
    irStream << "br i1 " << condI1 << ", label %" << bodyLabel << ", label %" << endLabel << "\n\n";

    irStream << bodyLabel << ":\n";
    currentBlockLabel = bodyLabel;
    blockHasTerminator = false;

    if (node->getBody()) {
        node->getBody()->accept(this);
    }

    if (!blockHasTerminator) {
        emitIndent();
        irStream << "br label %" << condLabel << "\n\n";
    }

    irStream << endLabel << ":\n";
    currentBlockLabel = endLabel;
    blockHasTerminator = false;

    loopStack.pop_back();
}

void LLVMCodeGen::visit(ForASTNode* node) {
    std::string condLabel = newLabel("for.cond");
    std::string bodyLabel = newLabel("for.body");
    std::string stepLabel = newLabel("for.step");
    std::string endLabel = newLabel("for.end");

    loopStack.push_back({endLabel, stepLabel});

    if (node->getInit()) {
        node->getInit()->accept(this);
    }

    emitIndent();
    irStream << "br label %" << condLabel << "\n\n";

    irStream << condLabel << ":\n";
    currentBlockLabel = condLabel;
    blockHasTerminator = false;

    if (node->getCondition()) {
        node->getCondition()->accept(this);
        std::string condVal = lastResultReg;
        std::string condI1 = condVal;
        if (lastResultType == "number") {
            condI1 = newReg();
            emitIndent();
            irStream << condI1 << " = fcmp one double " << condVal << ", 0.000000e+00\n";
        }
        emitIndent();
        irStream << "br i1 " << condI1 << ", label %" << bodyLabel << ", label %" << endLabel << "\n\n";
    } else {
        emitIndent();
        irStream << "br label %" << bodyLabel << "\n\n";
    }

    irStream << bodyLabel << ":\n";
    currentBlockLabel = bodyLabel;
    blockHasTerminator = false;

    if (node->getBody()) {
        node->getBody()->accept(this);
    }

    if (!blockHasTerminator) {
        emitIndent();
        irStream << "br label %" << stepLabel << "\n\n";
    }

    irStream << stepLabel << ":\n";
    currentBlockLabel = stepLabel;
    blockHasTerminator = false;

    if (node->getUpdate()) {
        node->getUpdate()->accept(this);
    }

    emitIndent();
    irStream << "br label %" << condLabel << "\n\n";

    irStream << endLabel << ":\n";
    currentBlockLabel = endLabel;
    blockHasTerminator = false;

    loopStack.pop_back();
}

void LLVMCodeGen::visit(BreakASTNode* node) {
    if (!loopStack.empty()) {
        emitIndent();
        irStream << "br label %" << loopStack.back().breakLabel << "\n";
        blockHasTerminator = true;
    }
}

void LLVMCodeGen::visit(ContinueASTNode* node) {
    if (!loopStack.empty()) {
        emitIndent();
        irStream << "br label %" << loopStack.back().continueLabel << "\n";
        blockHasTerminator = true;
    }
}

void LLVMCodeGen::visit(ReturnASTNode* node) {
    if (currentFunctionIsAsync) {
        if (node->getValue()) {
            node->getValue()->accept(this);
            std::string retVal = lastResultReg;
            emitIndent();
            irStream << "call void @vit_promise_resolve(i8* " << currentPromiseReg << ", double " << retVal << ")\n";
        }
        emitIndent();
        irStream << "ret i8* " << currentPromiseReg << "\n";
        blockHasTerminator = true;
        return;
    }

    std::string llvmRetType = (currentFunctionName == "main") ? "i32" : getLLVMType(currentFunctionReturnType);
    if (node->getValue()) {
        node->getValue()->accept(this);
        std::string retVal = lastResultReg;
        emitIndent();
        irStream << "ret " << llvmRetType << " " << retVal << "\n";
    } else {
        emitIndent();
        if (llvmRetType == "void") {
            irStream << "ret void\n";
        } else if (llvmRetType == "i1") {
            irStream << "ret i1 false\n";
        } else if (llvmRetType == "i8*" || (!llvmRetType.empty() && llvmRetType.back() == '*')) {
            irStream << "ret " << llvmRetType << " null\n";
        } else {
            irStream << "ret double 0.000000e+00\n";
        }
    }
    blockHasTerminator = true;
}

void LLVMCodeGen::visit(PrintASTNode* node) {
    if (node->getExpression()) {
        node->getExpression()->accept(this);
    }
    std::string valReg = lastResultReg;
    std::string valType = lastResultType;

    std::string fmtGlobal;
    if (valType == "string") {
        fmtGlobal = "@.fmt_str";
        std::string fmtPtr = newReg();
        emitIndent();
        irStream << fmtPtr << " = getelementptr inbounds [4 x i8], [4 x i8]* " << fmtGlobal << ", i64 0, i64 0\n";
        emitIndent();
        irStream << "call i32 (i8*, ...) @printf(i8* " << fmtPtr << ", i8* " << valReg << ")\n";
    } else if (valType == "boolean") {
        std::string trueLabel = newLabel("print.true");
        std::string falseLabel = newLabel("print.false");
        std::string endLabel = newLabel("print.end");

        emitIndent();
        irStream << "br i1 " << valReg << ", label %" << trueLabel << ", label %" << falseLabel << "\n";

        irStream << trueLabel << ":\n";
        currentBlockLabel = trueLabel;
        std::string trueFmt = newReg();
        emitIndent();
        irStream << trueFmt << " = getelementptr inbounds [6 x i8], [6 x i8]* @.fmt_bool_true, i64 0, i64 0\n";
        emitIndent();
        irStream << "call i32 (i8*, ...) @printf(i8* " << trueFmt << ")\n";
        emitIndent();
        irStream << "br label %" << endLabel << "\n";

        irStream << falseLabel << ":\n";
        currentBlockLabel = falseLabel;
        std::string falseFmt = newReg();
        emitIndent();
        irStream << falseFmt << " = getelementptr inbounds [7 x i8], [7 x i8]* @.fmt_bool_false, i64 0, i64 0\n";
        emitIndent();
        irStream << "call i32 (i8*, ...) @printf(i8* " << falseFmt << ")\n";
        emitIndent();
        irStream << "br label %" << endLabel << "\n";

        irStream << endLabel << ":\n";
        currentBlockLabel = endLabel;
    } else if (valType == "int") {
        fmtGlobal = "@.fmt_int";
        std::string fmtPtr = newReg();
        emitIndent();
        irStream << fmtPtr << " = getelementptr inbounds [6 x i8], [6 x i8]* " << fmtGlobal << ", i64 0, i64 0\n";
        emitIndent();
        irStream << "call i32 (i8*, ...) @printf(i8* " << fmtPtr << ", i64 " << valReg << ")\n";
    } else {
        fmtGlobal = "@.fmt_num";
        std::string fmtPtr = newReg();
        emitIndent();
        irStream << fmtPtr << " = getelementptr inbounds [4 x i8], [4 x i8]* " << fmtGlobal << ", i64 0, i64 0\n";
        emitIndent();
        irStream << "call i32 (i8*, ...) @printf(i8* " << fmtPtr << ", double " << valReg << ")\n";
    }

    emitIndent();
    irStream << "call i32 @fflush(i8* null)\n";
}

void LLVMCodeGen::visit(TypeAliasASTNode* node) {
    typeAliases[node->getAliasName()] = node->getTypeSpec();
}

void LLVMCodeGen::visit(EnumDeclASTNode* node) {
    globalDefsStream << "%struct." << node->getName() << " = type { i32, [8 x i8] }\n";
}

void LLVMCodeGen::visit(MatchASTNode* node) {
    if (node->getTarget()) {
        node->getTarget()->accept(this);
    }
    std::string targetReg = lastResultReg;

    std::string endLabel = newLabel("match.end");
    std::string defaultLabel = newLabel("match.default");

    std::vector<std::string> caseLabels;
    for (size_t i = 0; i < node->getCases().size(); ++i) {
        caseLabels.push_back(newLabel("match.case." + std::to_string(i)));
    }

    std::string targetType = lastResultType;

    std::string tagPtr = newReg();
    emitIndent();
    irStream << tagPtr << " = getelementptr inbounds %struct." << targetType << ", %struct." << targetType << "* " << targetReg << ", i32 0, i32 0\n";

    std::string tagVal = newReg();
    emitIndent();
    irStream << tagVal << " = load i32, i32* " << tagPtr << "\n";

    emitIndent();
    irStream << "switch i32 " << tagVal << ", label %" << defaultLabel << " [\n";
    for (size_t i = 0; i < node->getCases().size(); ++i) {
        emitIndent();
        irStream << "  i32 " << i << ", label %" << caseLabels[i] << "\n";
    }
    emitIndent();
    irStream << "]\n";

    for (size_t i = 0; i < node->getCases().size(); ++i) {
        irStream << caseLabels[i] << ":\n";
        currentBlockLabel = caseLabels[i];
        blockHasTerminator = false;

        const auto& c = node->getCases()[i];
        if (c.body) {
            c.body->accept(this);
        }

        if (!blockHasTerminator) {
            emitIndent();
            irStream << "br label %" << endLabel << "\n";
        }
    }

    irStream << defaultLabel << ":\n";
    currentBlockLabel = defaultLabel;
    blockHasTerminator = false;
    emitIndent();
    irStream << "br label %" << endLabel << "\n";

    irStream << endLabel << ":\n";
    currentBlockLabel = endLabel;
    blockHasTerminator = false;
}

void LLVMCodeGen::visit(ExpressionStmtASTNode* node) {
    if (node->getExpression()) {
        node->getExpression()->accept(this);
    }
}

} // namespace vit
