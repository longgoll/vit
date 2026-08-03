#include "codegen/LLVMCodeGen.h"
#include "lexer/Lexer.h"
#include "parser/Parser.h"
#include <iomanip>
#include <iostream>
#include <sstream>

namespace vit {

// Expression AST Visitors for LLVMCodeGen

void LLVMCodeGen::visit(NumberLiteralASTNode* node) {
    if (node->isInteger()) {
        lastResultReg = std::to_string(node->getIntValue());
        lastResultType = "int";
    } else {
        std::ostringstream simpleSS;
        simpleSS << node->getValue();
        std::string str = simpleSS.str();
        if (str.find('.') == std::string::npos && str.find('e') == std::string::npos) {
            str += ".0";
        }
        lastResultReg = str;
        lastResultType = "number";
    }
}

void LLVMCodeGen::visit(BooleanLiteralASTNode* node) {
    lastResultReg = node->getValue() ? "true" : "false";
    lastResultType = "boolean";
}

void LLVMCodeGen::visit(StringLiteralASTNode* node) {
    std::string rawVal = node->getValue();

    // Template string literal: starts with \x01
    // Format: \x01 text ${expr} more text ${expr2} ...
    bool isTemplate = (!rawVal.empty() && rawVal[0] == '\x01');
    std::string strVal = isTemplate ? rawVal.substr(1) : rawVal;

    if (!isTemplate) {
        // Regular string literal
        std::string globalName = "@.str." + std::to_string(stringCounter++);
        std::string irStr;
        size_t byteCount = 0;
        for (char c : strVal) {
            if (c == '\n') { irStr += "\\0A"; byteCount++; }
            else if (c == '\t') { irStr += "\\09"; byteCount++; }
            else if (c == '\r') { irStr += "\\0D"; byteCount++; }
            else if (c == '"')  { irStr += "\\22"; byteCount++; }
            else if (c == '\\') { irStr += "\\5C"; byteCount++; }
            else { irStr += c; byteCount++; }
        }
        irStr += "\\00";
        byteCount += 1;

        globalDefsStream << globalName << " = private unnamed_addr constant [" << byteCount << " x i8] c\"" << irStr << "\", align 1\n";
        std::string ptrReg = newReg();
        emitIndent();
        irStream << ptrReg << " = getelementptr inbounds [" << byteCount << " x i8], [" << byteCount << " x i8]* " << globalName << ", i64 0, i64 0\n";
        lastResultReg = ptrReg;
        lastResultType = "string";
        return;
    }

    // Template string: parse segments
    std::string outBuf = newReg();
    emitIndent();
    irStream << outBuf << " = call i8* @malloc(i64 4096)\n";

    // Initialize buffer to empty string
    std::string zeroStr = "@.str.tpl_empty_" + std::to_string(stringCounter++);
    globalDefsStream << zeroStr << " = private unnamed_addr constant [1 x i8] c\"\\00\", align 1\n";
    std::string emptyPtr = newReg();
    emitIndent();
    irStream << emptyPtr << " = getelementptr inbounds [1 x i8], [1 x i8]* " << zeroStr << ", i64 0, i64 0\n";
    std::string cpyRes = newReg();
    emitIndent();
    irStream << cpyRes << " = call i8* @strcpy(i8* " << outBuf << ", i8* " << emptyPtr << ")\n";

    // Parse the template string into segments
    size_t pos = 0;
    while (pos < strVal.size()) {
        size_t dollarPos = strVal.find("${", pos);
        std::string literal;
        if (dollarPos == std::string::npos) {
            literal = strVal.substr(pos);
            pos = strVal.size();
        } else {
            literal = strVal.substr(pos, dollarPos - pos);
            pos = dollarPos + 2;
        }

        if (!literal.empty()) {
            std::string globalName = "@.str.tpl_" + std::to_string(stringCounter++);
            std::string irStr;
            size_t byteCount = 0;
            for (char c : literal) {
                if (c == '\n') { irStr += "\\0A"; byteCount++; }
                else if (c == '\t') { irStr += "\\09"; byteCount++; }
                else if (c == '"') { irStr += "\\22"; byteCount++; }
                else if (c == '\\') { irStr += "\\5C"; byteCount++; }
                else { irStr += c; byteCount++; }
            }
            irStr += "\\00"; byteCount++;
            globalDefsStream << globalName << " = private unnamed_addr constant [" << byteCount << " x i8] c\"" << irStr << "\", align 1\n";
            std::string litPtr = newReg();
            emitIndent();
            irStream << litPtr << " = getelementptr inbounds [" << byteCount << " x i8], [" << byteCount << " x i8]* " << globalName << ", i64 0, i64 0\n";
            std::string catRes = newReg();
            emitIndent();
            irStream << catRes << " = call i8* @strcat(i8* " << outBuf << ", i8* " << litPtr << ")\n";
        }

        if (dollarPos != std::string::npos) {
            size_t closePos = strVal.find('}', pos);
            if (closePos == std::string::npos) break;
            std::string exprText = strVal.substr(pos, closePos - pos);
            pos = closePos + 1;

            try {
                Lexer exprLexer(exprText + ";");
                Parser exprParser(std::move(exprLexer));
                std::string wrappedCode = "function __tpl_expr(): void {\n    print(" + exprText + ");\n}\n";
                Lexer wl(wrappedCode);
                Parser wp(std::move(wl));
                auto wast = wp.parseProgram();
                if (!wast->getFunctions().empty()) {
                    auto* fn = wast->getFunctions()[0].get();
                    if (fn->getBody() && !fn->getBody()->getStatements().empty()) {
                        auto* pstmt = static_cast<PrintASTNode*>(fn->getBody()->getStatements()[0].get());
                        if (pstmt) {
                            pstmt->getExpression()->accept(this);
                            std::string exprReg = lastResultReg;
                            std::string exprType = lastResultType;

                            std::string exprStr;
                            if (exprType == "string") {
                                exprStr = exprReg;
                            } else {
                                std::string numBuf = newReg();
                                emitIndent();
                                irStream << numBuf << " = call i8* @malloc(i64 64)\n";
                                std::string fmtPtr = newReg();
                                if (exprType == "int") {
                                    std::string fmtGlobal = "@.fmt_int_tpl_" + std::to_string(stringCounter++);
                                    globalDefsStream << fmtGlobal << " = private unnamed_addr constant [4 x i8] c\"%ld\\00\", align 1\n";
                                    emitIndent();
                                    irStream << fmtPtr << " = getelementptr inbounds [4 x i8], [4 x i8]* " << fmtGlobal << ", i64 0, i64 0\n";
                                    emitIndent();
                                    irStream << "call i32 (i8*, i8*, ...) @sprintf(i8* " << numBuf << ", i8* " << fmtPtr << ", i64 " << exprReg << ")\n";
                                } else if (exprType == "boolean") {
                                    std::string fmtGlobal = "@.fmt_bool_tpl_" + std::to_string(stringCounter++);
                                    globalDefsStream << fmtGlobal << " = private unnamed_addr constant [3 x i8] c\"%d\\00\", align 1\n";
                                    emitIndent();
                                    irStream << fmtPtr << " = getelementptr inbounds [3 x i8], [3 x i8]* " << fmtGlobal << ", i64 0, i64 0\n";
                                    emitIndent();
                                    irStream << "call i32 (i8*, i8*, ...) @sprintf(i8* " << numBuf << ", i8* " << fmtPtr << ", i1 " << exprReg << ")\n";
                                } else {
                                    std::string fmtGlobal = "@.fmt_num_tpl_" + std::to_string(stringCounter++);
                                    globalDefsStream << fmtGlobal << " = private unnamed_addr constant [4 x i8] c\"%g\\00\", align 1\n";
                                    emitIndent();
                                    irStream << fmtPtr << " = getelementptr inbounds [4 x i8], [4 x i8]* " << fmtGlobal << ", i64 0, i64 0\n";
                                    emitIndent();
                                    irStream << "call i32 (i8*, i8*, ...) @sprintf(i8* " << numBuf << ", i8* " << fmtPtr << ", double " << exprReg << ")\n";
                                }
                                exprStr = numBuf;
                            }
                            std::string cat2 = newReg();
                            emitIndent();
                            irStream << cat2 << " = call i8* @strcat(i8* " << outBuf << ", i8* " << exprStr << ")\n";
                        }
                    }
                }
            } catch (...) {
            }
        }
    }

    lastResultReg = outBuf;
    lastResultType = "string";
}

void LLVMCodeGen::visit(ArrayLiteralASTNode* node) {
    size_t count = node->getElements().size();

    std::string mallocCall = newReg();
    emitIndent();
    irStream << mallocCall << " = call i8* @malloc(i64 " << ((count + 1) * 8) << ")\n";

    std::string basePtr = newReg();
    emitIndent();
    irStream << basePtr << " = bitcast i8* " << mallocCall << " to double*\n";
    emitIndent();
    irStream << "store double " << (double)count << ".0, double* " << basePtr << ", align 8\n";

    std::string elemPtr = newReg();
    emitIndent();
    irStream << elemPtr << " = getelementptr inbounds double, double* " << basePtr << ", i64 1\n";

    for (size_t i = 0; i < count; ++i) {
        node->getElements()[i]->accept(this);
        std::string elemVal = lastResultReg;

        std::string gepReg = newReg();
        emitIndent();
        irStream << gepReg << " = getelementptr inbounds double, double* " << elemPtr << ", i64 " << i << "\n";
        emitIndent();
        irStream << "store double " << elemVal << ", double* " << gepReg << ", align 8\n";
    }

    lastResultReg = elemPtr;
    lastResultType = "number[]";
}

void LLVMCodeGen::visit(VariableExprASTNode* node) {
    const VarSymbol* sym = findSymbol(node->getName());
    if (!sym && functionReturnTypes.count(node->getName())) {
        std::string fnPtr = newReg();
        emitIndent();
        irStream << fnPtr << " = bitcast i8* @" << node->getName() << " to i8*\n";
        lastResultReg = fnPtr;
        lastResultType = "function";
        return;
    }
    std::string addrReg = sym ? sym->addrReg : ("%" + node->getName() + ".addr");
    std::string typeName = sym ? sym->typeName : "number";

    std::string llvmType = getLLVMType(typeName);

    std::string loadReg = newReg();
    emitIndent();
    irStream << loadReg << " = load " << llvmType << ", " << llvmType << "* " << addrReg << ", align 8\n";
    lastResultReg = loadReg;
    lastResultType = typeName;
}

void LLVMCodeGen::visit(MemberAccessASTNode* node) {
    if (node->getTarget() && node->getTarget()->getType() == NodeType::VariableExpr) {
        auto varNode = static_cast<VariableExprASTNode*>(node->getTarget());
        if (enums.count(varNode->getName())) {
            std::string targetName = varNode->getName();
            std::string mallocCall = newReg();
            emitIndent();
            irStream << mallocCall << " = call i8* @malloc(i64 16)\n";
            std::string enumPtr = newReg();
            emitIndent();
            irStream << enumPtr << " = bitcast i8* " << mallocCall << " to %struct." << targetName << "*\n";

            std::string tagPtr = newReg();
            emitIndent();
            irStream << tagPtr << " = getelementptr inbounds %struct." << targetName << ", %struct." << targetName << "* " << enumPtr << ", i32 0, i32 0\n";
            emitIndent();
            irStream << "store i32 0, i32* " << tagPtr << "\n";

            lastResultReg = enumPtr;
            lastResultType = targetName;
            return;
        }
    }

    node->getTarget()->accept(this);
    std::string structPtrReg = lastResultReg;
    std::string structType = lastResultType;

    if (node->getMember() == "length") {
        if (structType == "string") {
            std::string lenI64 = newReg();
            emitIndent();
            irStream << lenI64 << " = call i64 @strlen(i8* " << structPtrReg << ")\n";
            std::string lenDbl = newReg();
            emitIndent();
            irStream << lenDbl << " = uitofp i64 " << lenI64 << " to double\n";
            lastResultReg = lenDbl;
            lastResultType = "number";
            return;
        }
        if (structType.size() > 2 && structType.substr(structType.size() - 2) == "[]") {
            std::string headerPtr = newReg();
            emitIndent();
            irStream << headerPtr << " = getelementptr inbounds double, double* " << structPtrReg << ", i64 -1\n";
            std::string lenDbl = newReg();
            emitIndent();
            irStream << lenDbl << " = load double, double* " << headerPtr << ", align 8\n";
            lastResultReg = lenDbl;
            lastResultType = "number";
            return;
        }
    }

    auto it = structs.find(structType);
    if (it != structs.end()) {
        int idx = it->second.fieldIndices[node->getMember()];
        std::string fieldType = it->second.fields[idx].second;
        std::string llvmFieldType = getLLVMType(fieldType);

        std::string gepReg = newReg();
        emitIndent();
        irStream << gepReg << " = getelementptr inbounds %struct." << structType << ", %struct." << structType << "* " << structPtrReg << ", i32 0, i32 " << idx << "\n";

        std::string valReg = newReg();
        emitIndent();
        irStream << valReg << " = load " << llvmFieldType << ", " << llvmFieldType << "* " << gepReg << ", align 8\n";

        lastResultReg = valReg;
        lastResultType = fieldType;
    }
}

void LLVMCodeGen::visit(ArrayAccessASTNode* node) {
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

    std::string headerPtr = newReg();
    emitIndent();
    irStream << headerPtr << " = getelementptr inbounds double, double* " << arrPtrReg << ", i64 -1\n";
    std::string lenDbl = newReg();
    emitIndent();
    irStream << lenDbl << " = load double, double* " << headerPtr << ", align 8\n";
    std::string lenI64 = newReg();
    emitIndent();
    irStream << lenI64 << " = fptosi double " << lenDbl << " to i64\n";

    std::string isOutOfBounds = newReg();
    emitIndent();
    irStream << isOutOfBounds << " = icmp uge i64 " << idxI64 << ", " << lenI64 << "\n";

    std::string okLabel = newLabel("bounds.ok");
    std::string panicLabel = newLabel("bounds.panic");

    emitIndent();
    irStream << "br i1 " << isOutOfBounds << ", label %" << panicLabel << ", label %" << okLabel << "\n\n";

    irStream << panicLabel << ":\n";
    emitIndent();
    irStream << "call void @__vit_panic(i8* getelementptr inbounds ([21 x i8], [21 x i8]* @.str.bounds_panic, i64 0, i64 0))\n";
    emitIndent();
    irStream << "unreachable\n\n";

    irStream << okLabel << ":\n";
    currentBlockLabel = okLabel;

    std::string gepReg = newReg();
    emitIndent();
    irStream << gepReg << " = getelementptr inbounds double, double* " << arrPtrReg << ", i64 " << idxI64 << "\n";

    std::string valReg = newReg();
    emitIndent();
    irStream << valReg << " = load double, double* " << gepReg << ", align 8\n";

    lastResultReg = valReg;
    lastResultType = "number";
}

void LLVMCodeGen::visit(UnaryOpASTNode* node) {
    if (node->getOperand()) {
        node->getOperand()->accept(this);
    }
    std::string operandReg = lastResultReg;
    std::string operandType = lastResultType;
    std::string resultReg = newReg();

    if (node->getOp() == "!") {
        std::string condI1 = operandReg;
        if (operandType == "number") {
            condI1 = newReg();
            emitIndent();
            irStream << condI1 << " = fcmp one double " << operandReg << ", 0.000000e+00\n";
        }
        emitIndent();
        irStream << resultReg << " = xor i1 " << condI1 << ", true\n";
        lastResultReg = resultReg;
        lastResultType = "boolean";
    } else if (node->getOp() == "-") {
        if (operandType == "int") {
            emitIndent();
            irStream << resultReg << " = sub i64 0, " << operandReg << "\n";
            lastResultReg = resultReg;
            lastResultType = "int";
        } else {
            emitIndent();
            irStream << resultReg << " = fneg double " << operandReg << "\n";
            lastResultReg = resultReg;
            lastResultType = "number";
        }
    }
}

void LLVMCodeGen::visit(BinaryOpASTNode* node) {
    const std::string& op = node->getOp();

    if (op == "&&" || op == "||") {
        std::string rhsLabel = newLabel("log.rhs");
        std::string mergeLabel = newLabel("log.merge");

        node->getLeft()->accept(this);
        std::string lhsVal = lastResultReg;
        std::string lhsI1 = lhsVal;
        if (lastResultType == "number") {
            lhsI1 = newReg();
            emitIndent();
            irStream << lhsI1 << " = fcmp one double " << lhsVal << ", 0.000000e+00\n";
        }

        std::string lhsBlock = currentBlockLabel;
        std::string resReg = newReg();

        if (op == "&&") {
            emitIndent();
            irStream << "br i1 " << lhsI1 << ", label %" << rhsLabel << ", label %" << mergeLabel << "\n";
            irStream << rhsLabel << ":\n";
            currentBlockLabel = rhsLabel;

            node->getRight()->accept(this);
            std::string rhsVal = lastResultReg;
            std::string rhsI1 = rhsVal;
            if (lastResultType == "number") {
                rhsI1 = newReg();
                emitIndent();
                irStream << rhsI1 << " = fcmp one double " << rhsVal << ", 0.000000e+00\n";
            }
            std::string rhsBlock = currentBlockLabel;
            emitIndent();
            irStream << "br label %" << mergeLabel << "\n";

            irStream << mergeLabel << ":\n";
            currentBlockLabel = mergeLabel;
            emitIndent();
            irStream << resReg << " = phi i1 [ false, %" << lhsBlock << " ], [ " << rhsI1 << ", %" << rhsBlock << " ]\n";
        } else { // ||
            emitIndent();
            irStream << "br i1 " << lhsI1 << ", label %" << mergeLabel << ", label %" << rhsLabel << "\n";
            irStream << rhsLabel << ":\n";
            currentBlockLabel = rhsLabel;

            node->getRight()->accept(this);
            std::string rhsVal = lastResultReg;
            std::string rhsI1 = rhsVal;
            if (lastResultType == "number") {
                rhsI1 = newReg();
                emitIndent();
                irStream << rhsI1 << " = fcmp one double " << rhsVal << ", 0.000000e+00\n";
            }
            std::string rhsBlock = currentBlockLabel;
            emitIndent();
            irStream << "br label %" << mergeLabel << "\n";

            irStream << mergeLabel << ":\n";
            currentBlockLabel = mergeLabel;
            emitIndent();
            irStream << resReg << " = phi i1 [ true, %" << lhsBlock << " ], [ " << rhsI1 << ", %" << rhsBlock << " ]\n";
        }

        lastResultReg = resReg;
        lastResultType = "boolean";
        return;
    }

    node->getLeft()->accept(this);
    std::string lhs = lastResultReg;
    std::string lhsType = lastResultType;

    node->getRight()->accept(this);
    std::string rhs = lastResultReg;
    std::string rhsType = lastResultType;

    std::string resultReg = newReg();

    if (op == "+") {
        if (lhsType == "string" || rhsType == "string") {
            std::string strLhs = lhs;
            if (lhsType == "number") {
                std::string numBuf = newReg();
                emitIndent();
                irStream << numBuf << " = call i8* @malloc(i64 64)\n";
                std::string fmtPtr = newReg();
                emitIndent();
                irStream << fmtPtr << " = getelementptr inbounds [4 x i8], [4 x i8]* @.fmt_num, i64 0, i64 0\n";
                emitIndent();
                irStream << "call i32 (i8*, i8*, ...) @sprintf(i8* " << numBuf << ", i8* " << fmtPtr << ", double " << lhs << ")\n";
                strLhs = numBuf;
            }
            std::string strRhs = rhs;
            if (rhsType == "number") {
                std::string numBuf = newReg();
                emitIndent();
                irStream << numBuf << " = call i8* @malloc(i64 64)\n";
                std::string fmtPtr = newReg();
                emitIndent();
                irStream << fmtPtr << " = getelementptr inbounds [4 x i8], [4 x i8]* @.fmt_num, i64 0, i64 0\n";
                emitIndent();
                irStream << "call i32 (i8*, i8*, ...) @sprintf(i8* " << numBuf << ", i8* " << fmtPtr << ", double " << rhs << ")\n";
                strRhs = numBuf;
            }

            std::string len1 = newReg();
            emitIndent();
            irStream << len1 << " = call i64 @strlen(i8* " << strLhs << ")\n";

            std::string len2 = newReg();
            emitIndent();
            irStream << len2 << " = call i64 @strlen(i8* " << strRhs << ")\n";

            std::string totalLen = newReg();
            emitIndent();
            irStream << totalLen << " = add i64 " << len1 << ", " << len2 << "\n";

            std::string allocLen = newReg();
            emitIndent();
            irStream << allocLen << " = add i64 " << totalLen << ", 1\n";

            std::string bufReg = newReg();
            emitIndent();
            irStream << bufReg << " = call i8* @malloc(i64 " << allocLen << ")\n";

            std::string dummy1 = newReg();
            emitIndent();
            irStream << dummy1 << " = call i8* @strcpy(i8* " << bufReg << ", i8* " << strLhs << ")\n";

            std::string dummy2 = newReg();
            emitIndent();
            irStream << dummy2 << " = call i8* @strcat(i8* " << bufReg << ", i8* " << strRhs << ")\n";

            lastResultReg = bufReg;
            lastResultType = "string";
            return;
        }
        if (lhsType == "int" && rhsType == "int") {
            emitIndent();
            irStream << resultReg << " = add i64 " << lhs << ", " << rhs << "\n";
            lastResultType = "int";
        } else {
            std::string dLhs = (lhsType == "int") ? newReg() : lhs;
            if (lhsType == "int") { emitIndent(); irStream << dLhs << " = sitofp i64 " << lhs << " to double\n"; }
            std::string dRhs = (rhsType == "int") ? newReg() : rhs;
            if (rhsType == "int") { emitIndent(); irStream << dRhs << " = sitofp i64 " << rhs << " to double\n"; }
            emitIndent();
            irStream << resultReg << " = fadd double " << dLhs << ", " << dRhs << "\n";
            lastResultType = "number";
        }
    } else if (op == "-") {
        if (lhsType == "int" && rhsType == "int") {
            emitIndent();
            irStream << resultReg << " = sub i64 " << lhs << ", " << rhs << "\n";
            lastResultType = "int";
        } else {
            std::string dLhs = (lhsType == "int") ? newReg() : lhs;
            if (lhsType == "int") { emitIndent(); irStream << dLhs << " = sitofp i64 " << lhs << " to double\n"; }
            std::string dRhs = (rhsType == "int") ? newReg() : rhs;
            if (rhsType == "int") { emitIndent(); irStream << dRhs << " = sitofp i64 " << rhs << " to double\n"; }
            emitIndent();
            irStream << resultReg << " = fsub double " << dLhs << ", " << dRhs << "\n";
            lastResultType = "number";
        }
    } else if (op == "*") {
        if (lhsType == "int" && rhsType == "int") {
            emitIndent();
            irStream << resultReg << " = mul i64 " << lhs << ", " << rhs << "\n";
            lastResultType = "int";
        } else {
            std::string dLhs = (lhsType == "int") ? newReg() : lhs;
            if (lhsType == "int") { emitIndent(); irStream << dLhs << " = sitofp i64 " << lhs << " to double\n"; }
            std::string dRhs = (rhsType == "int") ? newReg() : rhs;
            if (rhsType == "int") { emitIndent(); irStream << dRhs << " = sitofp i64 " << rhs << " to double\n"; }
            emitIndent();
            irStream << resultReg << " = fmul double " << dLhs << ", " << dRhs << "\n";
            lastResultType = "number";
        }
    } else if (op == "/") {
        if (lhsType == "int" && rhsType == "int") {
            emitIndent();
            irStream << resultReg << " = sdiv i64 " << lhs << ", " << rhs << "\n";
            lastResultType = "int";
        } else {
            std::string dLhs = (lhsType == "int") ? newReg() : lhs;
            if (lhsType == "int") { emitIndent(); irStream << dLhs << " = sitofp i64 " << lhs << " to double\n"; }
            std::string dRhs = (rhsType == "int") ? newReg() : rhs;
            if (rhsType == "int") { emitIndent(); irStream << dRhs << " = sitofp i64 " << rhs << " to double\n"; }
            emitIndent();
            irStream << resultReg << " = fdiv double " << dLhs << ", " << dRhs << "\n";
            lastResultType = "number";
        }
    } else if (op == "%") {
        if (lhsType == "int" && rhsType == "int") {
            emitIndent();
            irStream << resultReg << " = srem i64 " << lhs << ", " << rhs << "\n";
            lastResultType = "int";
        } else {
            std::string dLhs = (lhsType == "int") ? newReg() : lhs;
            if (lhsType == "int") { emitIndent(); irStream << dLhs << " = sitofp i64 " << lhs << " to double\n"; }
            std::string dRhs = (rhsType == "int") ? newReg() : rhs;
            if (rhsType == "int") { emitIndent(); irStream << dRhs << " = sitofp i64 " << rhs << " to double\n"; }
            emitIndent();
            irStream << resultReg << " = frem double " << dLhs << ", " << dRhs << "\n";
            lastResultType = "number";
        }
    } else if (op == "&") {
        std::string iLhs = lhs, iRhs = rhs;
        if (lhsType != "int") { iLhs = newReg(); emitIndent(); irStream << iLhs << " = fptosi double " << lhs << " to i64\n"; }
        if (rhsType != "int") { iRhs = newReg(); emitIndent(); irStream << iRhs << " = fptosi double " << rhs << " to i64\n"; }
        emitIndent();
        irStream << resultReg << " = and i64 " << iLhs << ", " << iRhs << "\n";
        lastResultType = "int";
    } else if (op == "|") {
        std::string iLhs = lhs, iRhs = rhs;
        if (lhsType != "int") { iLhs = newReg(); emitIndent(); irStream << iLhs << " = fptosi double " << lhs << " to i64\n"; }
        if (rhsType != "int") { iRhs = newReg(); emitIndent(); irStream << iRhs << " = fptosi double " << rhs << " to i64\n"; }
        emitIndent();
        irStream << resultReg << " = or i64 " << iLhs << ", " << iRhs << "\n";
        lastResultType = "int";
    } else if (op == "^") {
        std::string iLhs = lhs, iRhs = rhs;
        if (lhsType != "int") { iLhs = newReg(); emitIndent(); irStream << iLhs << " = fptosi double " << lhs << " to i64\n"; }
        if (rhsType != "int") { iRhs = newReg(); emitIndent(); irStream << iRhs << " = fptosi double " << rhs << " to i64\n"; }
        emitIndent();
        irStream << resultReg << " = xor i64 " << iLhs << ", " << iRhs << "\n";
        lastResultType = "int";
    } else if (op == "<<") {
        std::string iLhs = lhs, iRhs = rhs;
        if (lhsType != "int") { iLhs = newReg(); emitIndent(); irStream << iLhs << " = fptosi double " << lhs << " to i64\n"; }
        if (rhsType != "int") { iRhs = newReg(); emitIndent(); irStream << iRhs << " = fptosi double " << rhs << " to i64\n"; }
        emitIndent();
        irStream << resultReg << " = shl i64 " << iLhs << ", " << iRhs << "\n";
        lastResultType = "int";
    } else if (op == ">>") {
        std::string iLhs = lhs, iRhs = rhs;
        if (lhsType != "int") { iLhs = newReg(); emitIndent(); irStream << iLhs << " = fptosi double " << lhs << " to i64\n"; }
        if (rhsType != "int") { iRhs = newReg(); emitIndent(); irStream << iRhs << " = fptosi double " << rhs << " to i64\n"; }
        emitIndent();
        irStream << resultReg << " = ashr i64 " << iLhs << ", " << iRhs << "\n";
        lastResultType = "int";
    } else if (op == ">" || op == "<" || op == "==" || op == "!=" || op == ">=" || op == "<=") {
        if (lhsType == "null" || rhsType == "null" || (lhsType == "string" && rhsType == "string")) {
            if (lhsType == "string" && rhsType == "string") {
                std::string cmpReg = newReg();
                emitIndent();
                irStream << cmpReg << " = call i32 @strcmp(i8* " << lhs << ", i8* " << rhs << ")\n";
                std::string condCode = (op == "==") ? "eq" : (op == "!=") ? "ne" : "eq";
                emitIndent();
                irStream << resultReg << " = icmp " << condCode << " i32 " << cmpReg << ", 0\n";
                lastResultType = "boolean";
                lastResultReg = resultReg;
                return;
            }
            std::string condCode = (op == "==") ? "eq" : "ne";
            std::string lhsPtr = (lhs == "null") ? "null" : lhs;
            std::string rhsPtr = (rhs == "null") ? "null" : rhs;
            emitIndent();
            irStream << resultReg << " = icmp " << condCode << " i8* " << lhsPtr << ", " << rhsPtr << "\n";
            lastResultType = "boolean";
            lastResultReg = resultReg;
            return;
        }

        bool lhsIsInt = (lhsType == "int" || lhsType == "i32" || lhsType == "i64");
        bool rhsIsInt = (rhsType == "int" || rhsType == "i32" || rhsType == "i64");

        if (lhsIsInt && rhsIsInt) {
            std::string iLhs = lhs;
            if (lhsType == "i32") { iLhs = newReg(); emitIndent(); irStream << iLhs << " = sext i32 " << lhs << " to i64\n"; }
            std::string iRhs = rhs;
            if (rhsType == "i32") { iRhs = newReg(); emitIndent(); irStream << iRhs << " = sext i32 " << rhs << " to i64\n"; }

            std::string condCode = "eq";
            if (op == ">") condCode = "sgt";
            else if (op == "<") condCode = "slt";
            else if (op == "==") condCode = "eq";
            else if (op == "!=") condCode = "ne";
            else if (op == ">=") condCode = "sge";
            else if (op == "<=") condCode = "sle";

            emitIndent();
            irStream << resultReg << " = icmp " << condCode << " i64 " << iLhs << ", " << iRhs << "\n";
            lastResultType = "boolean";
        } else {
            std::string condCode = "oeq";
            if (op == ">") condCode = "ogt";
            else if (op == "<") condCode = "olt";
            else if (op == "==") condCode = "oeq";
            else if (op == "!=") condCode = "one";
            else if (op == ">=") condCode = "oge";
            else if (op == "<=") condCode = "ole";

            std::string dLhs = lhs;
            if (lhsType == "i32") { dLhs = newReg(); emitIndent(); irStream << dLhs << " = sitofp i32 " << lhs << " to double\n"; }
            else if (lhsType == "int" || lhsType == "i64") { dLhs = newReg(); emitIndent(); irStream << dLhs << " = sitofp i64 " << lhs << " to double\n"; }

            std::string dRhs = rhs;
            if (rhsType == "i32") { dRhs = newReg(); emitIndent(); irStream << dRhs << " = sitofp i32 " << rhs << " to double\n"; }
            else if (rhsType == "int" || rhsType == "i64") { dRhs = newReg(); emitIndent(); irStream << dRhs << " = sitofp i64 " << rhs << " to double\n"; }

            emitIndent();
            irStream << resultReg << " = fcmp " << condCode << " double " << dLhs << ", " << dRhs << "\n";
            lastResultType = "boolean";
        }
    }

    lastResultReg = resultReg;
}

void LLVMCodeGen::visit(CallExprASTNode* node) {
    std::string calleeName = node->getCallee();

    if (calleeName == "spawn") {
        if (!node->getArgs().empty()) {
            node->getArgs()[0]->accept(this);
            std::string fnPtrRaw = lastResultReg;
            emitIndent();
            irStream << "call void @vit_task_spawn(i8* " << fnPtrRaw << ", i8* null)\n";
        }
        lastResultReg = "";
        lastResultType = "void";
        return;
    }

    std::vector<std::string> argRegs;
    std::vector<std::string> argTypes;

    for (const auto& arg : node->getArgs()) {
        arg->accept(this);
        argRegs.push_back(lastResultReg);
        argTypes.push_back(lastResultType);
    }

    if (node->getCallee() == "panic" && !argRegs.empty()) {
        emitIndent();
        irStream << "call void @__vit_panic(i8* " << argRegs[0] << ")\n";
        lastResultReg = "";
        lastResultType = "void";
        return;
    }

    if (node->getCallee() == "assert" && argRegs.size() >= 2) {
        std::string passLabel = newLabel("assert.pass");
        std::string failLabel = newLabel("assert.fail");

        std::string condI1 = argRegs[0];
        if (argTypes[0] == "number") {
            condI1 = newReg();
            emitIndent();
            irStream << condI1 << " = fcmp one double " << argRegs[0] << ", 0.000000e+00\n";
        }

        emitIndent();
        irStream << "br i1 " << condI1 << ", label %" << passLabel << ", label %" << failLabel << "\n\n";

        irStream << failLabel << ":\n";
        emitIndent();
        irStream << "call void @__vit_panic(i8* " << argRegs[1] << ")\n";
        emitIndent();
        irStream << "unreachable\n\n";

        irStream << passLabel << ":\n";
        currentBlockLabel = passLabel;
        lastResultReg = "";
        lastResultType = "void";
        return;
    }

    auto it = functionReturnTypes.find(node->getCallee());
    if (it != functionReturnTypes.end()) {
        std::string retType = it->second;
        std::string llvmRetType = getLLVMType(retType);

        auto paramIt = functionParamTypes.find(node->getCallee());

        std::vector<std::string> castedArgRegs;
        std::vector<std::string> castedArgTypes;

        for (size_t i = 0; i < argRegs.size(); ++i) {
            std::string expectedType = (paramIt != functionParamTypes.end() && i < paramIt->second.size()) ? paramIt->second[i] : argTypes[i];
            std::string expectedLLVM = getLLVMType(expectedType);
            std::string currentLLVM = getLLVMType(argTypes[i]);

            if (expectedLLVM == "double" && currentLLVM == "i64") {
                std::string convReg = newReg();
                emitIndent();
                irStream << convReg << " = sitofp i64 " << argRegs[i] << " to double\n";
                castedArgRegs.push_back(convReg);
                castedArgTypes.push_back("double");
            } else if (expectedLLVM == "i64" && currentLLVM == "double") {
                std::string convReg = newReg();
                emitIndent();
                irStream << convReg << " = fptosi double " << argRegs[i] << " to i64\n";
                castedArgRegs.push_back(convReg);
                castedArgTypes.push_back("i64");
            } else {
                castedArgRegs.push_back(argRegs[i]);
                castedArgTypes.push_back(expectedLLVM);
            }
        }

        std::string callReg = "";
        emitIndent();
        if (llvmRetType != "void") {
            callReg = newReg();
            irStream << callReg << " = ";
        }
        irStream << "call " << llvmRetType << " @" << node->getCallee() << "(";
        for (size_t i = 0; i < castedArgRegs.size(); ++i) {
            irStream << castedArgTypes[i] << " " << castedArgRegs[i];
            if (i + 1 < castedArgRegs.size()) irStream << ", ";
        }
        irStream << ")\n";

        lastResultReg = callReg;
        lastResultType = retType;
        return;
    }

    auto symIt = symbolTable.find(node->getCallee());
    if (symIt != symbolTable.end()) {
        std::string varType = symIt->second.typeName;
        std::string llvmVarType = getLLVMType(varType);

        std::string rawPtr = newReg();
        emitIndent();
        irStream << rawPtr << " = load " << llvmVarType << ", " << llvmVarType << "* " << symIt->second.addrReg << ", align 8\n";

        std::string retType = "number";
        size_t arrowPos = varType.rfind("=> ");
        if (arrowPos != std::string::npos) {
            retType = varType.substr(arrowPos + 3);
        }
        std::string llvmRetType = getLLVMType(retType);

        std::string castPtr = newReg();
        emitIndent();
        irStream << castPtr << " = bitcast i8* " << rawPtr << " to " << llvmRetType << " (";
        for (size_t i = 0; i < argTypes.size(); ++i) {
            irStream << getLLVMType(argTypes[i]);
            if (i + 1 < argTypes.size()) irStream << ", ";
        }
        irStream << ")*\n";

        std::string callReg = "";
        emitIndent();
        if (llvmRetType != "void") {
            callReg = newReg();
            irStream << callReg << " = ";
        }
        irStream << "call " << llvmRetType << " " << castPtr << "(";
        for (size_t i = 0; i < argRegs.size(); ++i) {
            irStream << getLLVMType(argTypes[i]) << " " << argRegs[i];
            if (i + 1 < argRegs.size()) irStream << ", ";
        }
        irStream << ")\n";

        lastResultReg = callReg;
        lastResultType = retType;
        return;
    }

    std::string callReg = newReg();
    emitIndent();
    irStream << callReg << " = call double @" << node->getCallee() << "()\n";
    lastResultReg = callReg;
    lastResultType = "number";
}

void LLVMCodeGen::visit(MethodCallASTNode* node) {
    node->getTarget()->accept(this);
    std::string targetReg = lastResultReg;
    std::string targetType = lastResultType;

    std::string method = node->getMethod();

    // Functional higher-order array methods (map, filter, forEach)
    if (targetType.size() > 2 && targetType.substr(targetType.size() - 2) == "[]") {
        if (!node->getArgs().empty()) {
            node->getArgs()[0]->accept(this);
            std::string fnPtrRaw = lastResultReg;

            std::string headerPtr = newReg();
            emitIndent();
            irStream << headerPtr << " = getelementptr inbounds double, double* " << targetReg << ", i64 -1\n";
            std::string lenDbl = newReg();
            emitIndent();
            irStream << lenDbl << " = load double, double* " << headerPtr << ", align 8\n";
            std::string nI64 = newReg();
            emitIndent();
            irStream << nI64 << " = fptosi double " << lenDbl << " to i64\n";

            std::string loopCondLabel = newLabel("hoc.cond");
            std::string loopBodyLabel = newLabel("hoc.body");
            std::string loopEndLabel = newLabel("hoc.end");

            if (method == "map") {
                std::string allocBytes = newReg();
                std::string nPlus1 = newReg();
                emitIndent();
                irStream << nPlus1 << " = add i64 " << nI64 << ", 1\n";
                emitIndent();
                irStream << allocBytes << " = mul i64 " << nPlus1 << ", 8\n";

                std::string mallocCall = newReg();
                emitIndent();
                irStream << mallocCall << " = call i8* @malloc(i64 " << allocBytes << ")\n";

                std::string newBase = newReg();
                emitIndent();
                irStream << newBase << " = bitcast i8* " << mallocCall << " to double*\n";
                emitIndent();
                irStream << "store double " << lenDbl << ", double* " << newBase << ", align 8\n";

                std::string newElemPtr = newReg();
                emitIndent();
                irStream << newElemPtr << " = getelementptr inbounds double, double* " << newBase << ", i64 1\n";

                std::string idxVar = newReg();
                emitIndent();
                irStream << idxVar << " = alloca i64, align 8\n";
                emitIndent();
                irStream << "store i64 0, i64* " << idxVar << ", align 8\n";

                emitIndent();
                irStream << "br label %" << loopCondLabel << "\n";

                irStream << loopCondLabel << ":\n";
                currentBlockLabel = loopCondLabel;
                std::string kVal = newReg();
                emitIndent();
                irStream << kVal << " = load i64, i64* " << idxVar << ", align 8\n";
                std::string cond = newReg();
                emitIndent();
                irStream << cond << " = icmp slt i64 " << kVal << ", " << nI64 << "\n";
                emitIndent();
                irStream << "br i1 " << cond << ", label %" << loopBodyLabel << ", label %" << loopEndLabel << "\n";

                irStream << loopBodyLabel << ":\n";
                currentBlockLabel = loopBodyLabel;

                std::string srcGep = newReg();
                emitIndent();
                irStream << srcGep << " = getelementptr inbounds double, double* " << targetReg << ", i64 " << kVal << "\n";
                std::string srcElem = newReg();
                emitIndent();
                irStream << srcElem << " = load double, double* " << srcGep << ", align 8\n";

                std::string castFn = newReg();
                emitIndent();
                irStream << castFn << " = bitcast i8* " << fnPtrRaw << " to double (double)*\n";

                std::string mappedElem = newReg();
                emitIndent();
                irStream << mappedElem << " = call double " << castFn << "(double " << srcElem << ")\n";

                std::string dstGep = newReg();
                emitIndent();
                irStream << dstGep << " = getelementptr inbounds double, double* " << newElemPtr << ", i64 " << kVal << "\n";
                emitIndent();
                irStream << "store double " << mappedElem << ", double* " << dstGep << ", align 8\n";

                std::string nextK = newReg();
                emitIndent();
                irStream << nextK << " = add i64 " << kVal << ", 1\n";
                emitIndent();
                irStream << "store i64 " << nextK << ", i64* " << idxVar << ", align 8\n";
                emitIndent();
                irStream << "br label %" << loopCondLabel << "\n";

                irStream << loopEndLabel << ":\n";
                currentBlockLabel = loopEndLabel;

                lastResultReg = newElemPtr;
                lastResultType = "number[]";
                return;
            } else if (method == "filter") {
                std::string allocBytes = newReg();
                std::string nPlus1 = newReg();
                emitIndent();
                irStream << nPlus1 << " = add i64 " << nI64 << ", 1\n";
                emitIndent();
                irStream << allocBytes << " = mul i64 " << nPlus1 << ", 8\n";

                std::string mallocCall = newReg();
                emitIndent();
                irStream << mallocCall << " = call i8* @malloc(i64 " << allocBytes << ")\n";

                std::string newBase = newReg();
                emitIndent();
                irStream << newBase << " = bitcast i8* " << mallocCall << " to double*\n";
                std::string newElemPtr = newReg();
                emitIndent();
                irStream << newElemPtr << " = getelementptr inbounds double, double* " << newBase << ", i64 1\n";

                std::string idxVar = newReg();
                std::string countVar = newReg();
                emitIndent();
                irStream << idxVar << " = alloca i64, align 8\n";
                emitIndent();
                irStream << countVar << " = alloca i64, align 8\n";
                emitIndent();
                irStream << "store i64 0, i64* " << idxVar << ", align 8\n";
                emitIndent();
                irStream << "store i64 0, i64* " << countVar << ", align 8\n";

                emitIndent();
                irStream << "br label %" << loopCondLabel << "\n";

                irStream << loopCondLabel << ":\n";
                currentBlockLabel = loopCondLabel;
                std::string kVal = newReg();
                emitIndent();
                irStream << kVal << " = load i64, i64* " << idxVar << ", align 8\n";
                std::string cond = newReg();
                emitIndent();
                irStream << cond << " = icmp slt i64 " << kVal << ", " << nI64 << "\n";
                emitIndent();
                irStream << "br i1 " << cond << ", label %" << loopBodyLabel << ", label %" << loopEndLabel << "\n";

                irStream << loopBodyLabel << ":\n";
                currentBlockLabel = loopBodyLabel;

                std::string srcGep = newReg();
                emitIndent();
                irStream << srcGep << " = getelementptr inbounds double, double* " << targetReg << ", i64 " << kVal << "\n";
                std::string srcElem = newReg();
                emitIndent();
                irStream << srcElem << " = load double, double* " << srcGep << ", align 8\n";

                std::string castFn = newReg();
                emitIndent();
                irStream << castFn << " = bitcast i8* " << fnPtrRaw << " to i1 (double)*\n";

                std::string matchBool = newReg();
                emitIndent();
                irStream << matchBool << " = call i1 " << castFn << "(double " << srcElem << ")\n";

                std::string ifTrueLabel = newLabel("filter.true");
                std::string ifNextLabel = newLabel("filter.next");

                emitIndent();
                irStream << "br i1 " << matchBool << ", label %" << ifTrueLabel << ", label %" << ifNextLabel << "\n";

                irStream << ifTrueLabel << ":\n";
                currentBlockLabel = ifTrueLabel;
                std::string curCount = newReg();
                emitIndent();
                irStream << curCount << " = load i64, i64* " << countVar << ", align 8\n";

                std::string dstGep = newReg();
                emitIndent();
                irStream << dstGep << " = getelementptr inbounds double, double* " << newElemPtr << ", i64 " << curCount << "\n";
                emitIndent();
                irStream << "store double " << srcElem << ", double* " << dstGep << ", align 8\n";

                std::string nextCount = newReg();
                emitIndent();
                irStream << nextCount << " = add i64 " << curCount << ", 1\n";
                emitIndent();
                irStream << "store i64 " << nextCount << ", i64* " << countVar << ", align 8\n";
                emitIndent();
                irStream << "br label %" << ifNextLabel << "\n";

                irStream << ifNextLabel << ":\n";
                currentBlockLabel = ifNextLabel;

                std::string nextK = newReg();
                emitIndent();
                irStream << nextK << " = add i64 " << kVal << ", 1\n";
                emitIndent();
                irStream << "store i64 " << nextK << ", i64* " << idxVar << ", align 8\n";
                emitIndent();
                irStream << "br label %" << loopCondLabel << "\n";

                irStream << loopEndLabel << ":\n";
                currentBlockLabel = loopEndLabel;

                std::string finalCount = newReg();
                emitIndent();
                irStream << finalCount << " = load i64, i64* " << countVar << ", align 8\n";
                std::string finalDbl = newReg();
                emitIndent();
                irStream << finalDbl << " = uitofp i64 " << finalCount << " to double\n";
                emitIndent();
                irStream << "store double " << finalDbl << ", double* " << newBase << ", align 8\n";

                lastResultReg = newElemPtr;
                lastResultType = targetType;
                return;
            } else if (method == "forEach") {
                std::string idxVar = newReg();
                emitIndent();
                irStream << idxVar << " = alloca i64, align 8\n";
                emitIndent();
                irStream << "store i64 0, i64* " << idxVar << ", align 8\n";

                emitIndent();
                irStream << "br label %" << loopCondLabel << "\n";

                irStream << loopCondLabel << ":\n";
                currentBlockLabel = loopCondLabel;
                std::string kVal = newReg();
                emitIndent();
                irStream << kVal << " = load i64, i64* " << idxVar << ", align 8\n";
                std::string cond = newReg();
                emitIndent();
                irStream << cond << " = icmp slt i64 " << kVal << ", " << nI64 << "\n";
                emitIndent();
                irStream << "br i1 " << cond << ", label %" << loopBodyLabel << ", label %" << loopEndLabel << "\n";

                irStream << loopBodyLabel << ":\n";
                currentBlockLabel = loopBodyLabel;

                std::string srcGep = newReg();
                emitIndent();
                irStream << srcGep << " = getelementptr inbounds double, double* " << targetReg << ", i64 " << kVal << "\n";
                std::string srcElem = newReg();
                emitIndent();
                irStream << srcElem << " = load double, double* " << srcGep << ", align 8\n";

                std::string castFn = newReg();
                emitIndent();
                irStream << castFn << " = bitcast i8* " << fnPtrRaw << " to void (double)*\n";
                emitIndent();
                irStream << "call void " << castFn << "(double " << srcElem << ")\n";

                std::string nextK = newReg();
                emitIndent();
                irStream << nextK << " = add i64 " << kVal << ", 1\n";
                emitIndent();
                irStream << "store i64 " << nextK << ", i64* " << idxVar << ", align 8\n";
                emitIndent();
                irStream << "br label %" << loopCondLabel << "\n";

                irStream << loopEndLabel << ":\n";
                currentBlockLabel = loopEndLabel;

                lastResultReg = "";
                lastResultType = "void";
                return;
            }
        }
    }

    std::vector<std::string> argRegs;
    std::vector<std::string> argTypes;

    argRegs.push_back(targetReg);
    argTypes.push_back(targetType);

    for (const auto& arg : node->getArgs()) {
        arg->accept(this);
        argRegs.push_back(lastResultReg);
        argTypes.push_back(lastResultType);
    }

    std::string mangledName = "_" + targetType + "_" + node->getMethod();
    std::string retType = "number";
    auto it = functionReturnTypes.find(mangledName);
    if (it != functionReturnTypes.end()) {
        retType = it->second;
    }

    std::string llvmRetType = getLLVMType(retType);
    std::string callReg = "";
    emitIndent();
    if (llvmRetType != "void") {
        callReg = newReg();
        irStream << callReg << " = ";
    }
    irStream << "call " << llvmRetType << " @" << mangledName << "(";
    for (size_t i = 0; i < argRegs.size(); ++i) {
        std::string pType = getLLVMType(argTypes[i]);
        irStream << pType << " " << argRegs[i];
        if (i + 1 < argRegs.size()) irStream << ", ";
    }
    irStream << ")\n";

    lastResultReg = callReg;
    lastResultType = retType;
}

void LLVMCodeGen::visit(LambdaASTNode* node) {
    std::string lambdaName = "__lambda_" + std::to_string(lambdaCounter++);

    std::stringstream oldStream;
    oldStream << irStream.rdbuf();
    irStream.str("");
    irStream.clear();

    std::string oldFnName = currentFunctionName;
    std::string oldRetType = currentFunctionReturnType;
    auto oldSymbolTable = symbolTable;

    currentFunctionName = lambdaName;
    currentFunctionReturnType = "number";
    symbolTable.clear();

    std::string llvmRetType = "double";
    irStream << "define " << llvmRetType << " @" << lambdaName << "(";
    const auto& params = node->getParams();
    for (size_t i = 0; i < params.size(); ++i) {
        irStream << "double %" << params[i].name;
        if (i + 1 < params.size()) irStream << ", ";
    }
    irStream << ") {\n";
    irStream << "entry:\n";
    currentBlockLabel = "entry";

    for (size_t i = 0; i < params.size(); ++i) {
        std::string addrReg = "%" + params[i].name + ".addr";
        emitIndent();
        irStream << addrReg << " = alloca double, align 8\n";
        emitIndent();
        irStream << "store double %" << params[i].name << ", double* " << addrReg << ", align 8\n";
        symbolTable[params[i].name] = {addrReg, "number"};
    }

    if (node->getBody()) {
        node->getBody()->accept(this);
    }

    if (!blockHasTerminator) {
        emitIndent();
        irStream << "ret double 0.000000e+00\n";
    }

    irStream << "}\n\n";

    std::string lambdaIR = irStream.str();
    irStream.str("");
    irStream.clear();
    irStream << oldStream.str() << lambdaIR;

    currentFunctionName = oldFnName;
    currentFunctionReturnType = oldRetType;
    symbolTable = oldSymbolTable;

    std::string fnPtr = newReg();
    emitIndent();
    irStream << fnPtr << " = bitcast double (";
    for (size_t i = 0; i < params.size(); ++i) {
        irStream << "double";
        if (i + 1 < params.size()) irStream << ", ";
    }
    irStream << ")* @" << lambdaName << " to i8*\n";

    lastResultReg = fnPtr;
    lastResultType = "function";
}

void LLVMCodeGen::visit(EnumVariantExprASTNode* node) {
    std::string mallocCall = newReg();
    emitIndent();
    irStream << mallocCall << " = call i8* @malloc(i64 16)\n";
    std::string enumPtr = newReg();
    emitIndent();
    irStream << enumPtr << " = bitcast i8* " << mallocCall << " to %struct." << node->getEnumName() << "*\n";

    int tagIndex = (node->getVariantName() == "Some" || node->getVariantName() == "Ok") ? 0 : 1;

    std::string tagPtr = newReg();
    emitIndent();
    irStream << tagPtr << " = getelementptr inbounds %struct." << node->getEnumName() << ", %struct." << node->getEnumName() << "* " << enumPtr << ", i32 0, i32 0\n";
    emitIndent();
    irStream << "store i32 " << tagIndex << ", i32* " << tagPtr << "\n";

    if (!node->getArgs().empty()) {
        node->getArgs()[0]->accept(this);
        std::string argReg = lastResultReg;

        std::string payloadPtr = newReg();
        emitIndent();
        irStream << payloadPtr << " = getelementptr inbounds %struct." << node->getEnumName() << ", %struct." << node->getEnumName() << "* " << enumPtr << ", i32 0, i32 1\n";
        std::string payloadDblPtr = newReg();
        emitIndent();
        irStream << payloadDblPtr << " = bitcast [8 x i8]* " << payloadPtr << " to double*\n";
        emitIndent();
        irStream << "store double " << argReg << ", double* " << payloadDblPtr << ", align 8\n";
    }

    lastResultReg = enumPtr;
    lastResultType = node->getEnumName();
}

void LLVMCodeGen::visit(NullLiteralASTNode* node) {
    lastResultReg = "null";
    lastResultType = "null";
}

void LLVMCodeGen::visit(TryExprASTNode* node) {
    if (node->getExpr()) {
        node->getExpr()->accept(this);
    }
    std::string innerReg = lastResultReg;
    std::string innerType = lastResultType;

    std::string cleanType = innerType;
    if (!cleanType.empty() && cleanType.back() == '?') cleanType.pop_back();
    size_t anglePos = cleanType.find('<');
    if (anglePos != std::string::npos) cleanType = cleanType.substr(0, anglePos);

    std::string okLabel = newLabel("try.ok");
    std::string errLabel = newLabel("try.err");

    if (cleanType.rfind("Option", 0) == 0 || cleanType.rfind("Result", 0) == 0 || enums.count(cleanType) || structs.count(cleanType)) {
        std::string tagPtr = newReg();
        emitIndent();
        irStream << tagPtr << " = getelementptr inbounds %struct." << cleanType << ", %struct." << cleanType << "* " << innerReg << ", i32 0, i32 0\n";
        std::string tagVal = newReg();
        emitIndent();
        irStream << tagVal << " = load i32, i32* " << tagPtr << ", align 4\n";

        std::string isErr = newReg();
        emitIndent();
        irStream << isErr << " = icmp ne i32 " << tagVal << ", 0\n";

        emitIndent();
        irStream << "br i1 " << isErr << ", label %" << errLabel << ", label %" << okLabel << "\n\n";

        irStream << errLabel << ":\n";
        currentBlockLabel = errLabel;
        emitIndent();
        std::string llvmRetType = getLLVMType(currentFunctionReturnType);
        if (llvmRetType == "void") {
            irStream << "ret void\n";
        } else if (llvmRetType == "i1") {
            irStream << "ret i1 false\n";
        } else if (llvmRetType == "i8*" || (!llvmRetType.empty() && llvmRetType.back() == '*')) {
            irStream << "ret " << llvmRetType << " null\n";
        } else {
            irStream << "ret double 0.000000e+00\n";
        }

        irStream << okLabel << ":\n";
        currentBlockLabel = okLabel;

        std::string payloadPtr = newReg();
        emitIndent();
        irStream << payloadPtr << " = getelementptr inbounds %struct." << cleanType << ", %struct." << cleanType << "* " << innerReg << ", i32 0, i32 1\n";
        std::string payloadDblPtr = newReg();
        emitIndent();
        irStream << payloadDblPtr << " = bitcast [8 x i8]* " << payloadPtr << " to double*\n";
        std::string payloadVal = newReg();
        emitIndent();
        irStream << payloadVal << " = load double, double* " << payloadDblPtr << ", align 8\n";

        lastResultReg = payloadVal;
        lastResultType = "number";
    } else {
        std::string isNull = newReg();
        emitIndent();
        irStream << isNull << " = icmp eq i8* " << innerReg << ", null\n";
        emitIndent();
        irStream << "br i1 " << isNull << ", label %" << errLabel << ", label %" << okLabel << "\n\n";

        irStream << errLabel << ":\n";
        currentBlockLabel = errLabel;
        emitIndent();
        std::string llvmRetType = getLLVMType(currentFunctionReturnType);
        if (llvmRetType == "void") {
            irStream << "ret void\n";
        } else if (llvmRetType == "i8*" || (!llvmRetType.empty() && llvmRetType.back() == '*')) {
            irStream << "ret " << llvmRetType << " null\n";
        } else {
            irStream << "ret double 0.000000e+00\n";
        }

        irStream << okLabel << ":\n";
        currentBlockLabel = okLabel;
        lastResultReg = innerReg;
        lastResultType = innerType;
    }
}

void LLVMCodeGen::visit(OptionalChainASTNode* node) {
    if (node->getTarget()) {
        node->getTarget()->accept(this);
    }
    std::string targetReg = lastResultReg;
    std::string targetType = lastResultType;

    std::string cleanType = targetType;
    if (!cleanType.empty() && cleanType.back() == '?') cleanType.pop_back();
    size_t anglePos = cleanType.find('<');
    if (anglePos != std::string::npos) cleanType = cleanType.substr(0, anglePos);

    std::string nullLabel = newLabel("opt.null");
    std::string validLabel = newLabel("opt.valid");
    std::string mergeLabel = newLabel("opt.merge");

    std::string isNull = newReg();
    emitIndent();
    std::string llvmTargetType = getLLVMType(targetType);
    if (llvmTargetType == "i8*" || (!llvmTargetType.empty() && llvmTargetType.back() == '*')) {
        irStream << isNull << " = icmp eq " << llvmTargetType << " " << targetReg << ", null\n";
    } else {
        irStream << isNull << " = fcmp oeq double " << targetReg << ", 0.000000e+00\n";
    }

    emitIndent();
    irStream << "br i1 " << isNull << ", label %" << nullLabel << ", label %" << validLabel << "\n\n";

    irStream << nullLabel << ":\n";
    currentBlockLabel = nullLabel;
    emitIndent();
    irStream << "br label %" << mergeLabel << "\n\n";

    irStream << validLabel << ":\n";
    currentBlockLabel = validLabel;

    std::string validValReg = "0.0";
    std::string validValType = "number";
    auto it = structs.find(cleanType);
    if (it != structs.end()) {
        int idx = it->second.fieldIndices[node->getMember()];
        std::string fieldType = it->second.fields[idx].second;
        std::string llvmFieldType = getLLVMType(fieldType);

        std::string gepReg = newReg();
        emitIndent();
        irStream << gepReg << " = getelementptr inbounds %struct." << cleanType << ", %struct." << cleanType << "* " << targetReg << ", i32 0, i32 " << idx << "\n";

        validValReg = newReg();
        emitIndent();
        irStream << validValReg << " = load " << llvmFieldType << ", " << llvmFieldType << "* " << gepReg << ", align 8\n";
        validValType = fieldType;
    }

    std::string validBlock = currentBlockLabel;
    emitIndent();
    irStream << "br label %" << mergeLabel << "\n\n";

    irStream << mergeLabel << ":\n";
    currentBlockLabel = mergeLabel;
    std::string llvmResultType = getLLVMType(validValType);
    std::string phiReg = newReg();
    emitIndent();
    std::string nullValStr = (llvmResultType == "i8*" || (!llvmResultType.empty() && llvmResultType.back() == '*')) ? "null" : "0.000000e+00";
    irStream << phiReg << " = phi " << llvmResultType << " [ " << nullValStr << ", %" << nullLabel << " ], [ " << validValReg << ", %" << validBlock << " ]\n";

    lastResultReg = phiReg;
    lastResultType = validValType;
}

void LLVMCodeGen::visit(NullCoalesceASTNode* node) {
    if (node->getLeft()) {
        node->getLeft()->accept(this);
    }
    std::string leftReg = lastResultReg;
    std::string leftType = lastResultType;

    std::string nullLabel = newLabel("nc.null");
    std::string validLabel = newLabel("nc.valid");
    std::string mergeLabel = newLabel("nc.merge");

    std::string isNull = newReg();
    emitIndent();
    std::string llvmLeftType = getLLVMType(leftType);
    if (llvmLeftType == "i8*" || (!llvmLeftType.empty() && llvmLeftType.back() == '*')) {
        irStream << isNull << " = icmp eq " << llvmLeftType << " " << leftReg << ", null\n";
    } else {
        irStream << isNull << " = fcmp oeq double " << leftReg << ", 0.000000e+00\n";
    }

    emitIndent();
    irStream << "br i1 " << isNull << ", label %" << nullLabel << ", label %" << validLabel << "\n\n";

    irStream << nullLabel << ":\n";
    currentBlockLabel = nullLabel;
    if (node->getRight()) {
        node->getRight()->accept(this);
    }
    std::string rightReg = lastResultReg;
    std::string rightType = lastResultType;
    std::string rhsBlock = currentBlockLabel;
    emitIndent();
    irStream << "br label %" << mergeLabel << "\n\n";

    irStream << validLabel << ":\n";
    currentBlockLabel = validLabel;
    std::string lhsBlock = validLabel;
    emitIndent();
    irStream << "br label %" << mergeLabel << "\n\n";

    irStream << mergeLabel << ":\n";
    currentBlockLabel = mergeLabel;

    std::string llvmResType = getLLVMType(rightType);
    std::string phiReg = newReg();
    emitIndent();
    irStream << phiReg << " = phi " << llvmResType << " [ " << leftReg << ", %" << lhsBlock << " ], [ " << rightReg << ", %" << rhsBlock << " ]\n";

    lastResultReg = phiReg;
    lastResultType = rightType;
}

void LLVMCodeGen::visit(AwaitExprASTNode* node) {
    if (node->getExpr()) {
        node->getExpr()->accept(this);
    }
    std::string promiseReg = lastResultReg;

    std::string resVal = newReg();
    emitIndent();
    irStream << resVal << " = call double @vit_promise_await(i8* " << promiseReg << ")\n";

    lastResultReg = resVal;
    lastResultType = "number";
}

} // namespace vit
