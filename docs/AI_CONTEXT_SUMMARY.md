# VIT Compiler - Complete Architecture & Phase 1-4 Summary (AI Context Document)

> **Purpose**: Tài liệu tóm tắt toàn bộ kiến trúc, cú pháp, tính năng và lịch sử 4 Phase đã hoàn thành (v0.4.0) của trình biên dịch **VIT Compiler**. File này được tối ưu hóa cho AI Coding Assistant đọc nhanh, giúp nắm bắt 100% ngữ cảnh dự án với lượng token tối thiểu.

---

## 1. Quick Facts & Tech Stack
* **Project**: VIT Compiler (`v0.4.0`) - Ngôn ngữ biên dịch Native có cú pháp giống JS/TS.
* **Implementation**: C++20, LLVM C++ API (IRBuilder, LLVM IR CodeGen), Clang Native Linker.
* **Compilation Pipeline**: `Source (.vit/.jslik)` ➔ `Lexer` ➔ `Parser (Recursive Descent + Precedence Climbing)` ➔ `SemanticAnalyzer (Type Check)` ➔ `LLVMCodeGen (IR + ARC Pass)` ➔ `NativeCompiler (Clang -O1/-O2/-O3)` ➔ `Executable (.exe)`.

---

## 2. Summary Timeline: Phase 1 -> Phase 4

| Phase | Milestone Name | Features & Scope | Key Files Updated |
|---|---|---|---|
| **Phase 1** | **MVP Foundation** | - Type: `number` (IEEE 754 f64 / `double`).<br>- Variables: `let`, `const`.<br>- AST, Hand-written Lexer/Parser, LLVM IR CodeGen, Clang Native link.<br>- CLI subcommands: `vit run`, `vit build`, `vit version`, `vit help`. | [Lexer.cpp](file:///d:/HoangLong/Dev/vit/src/lexer/Lexer.cpp), [Parser.cpp](file:///d:/HoangLong/Dev/vit/src/parser/Parser.cpp), [LLVMCodeGen.cpp](file:///d:/HoangLong/Dev/vit/src/codegen/LLVMCodeGen.cpp), [NativeCompiler.cpp](file:///d:/HoangLong/Dev/vit/src/codegen/NativeCompiler.cpp), [main.cpp](file:///d:/HoangLong/Dev/vit/src/main.cpp) |
| **Phase 2** | **Control Flow & Semantics** | - Control flow: `if/else`, `while`, `for`, `break`, `continue`.<br>- Types: `boolean` (`true`/`false`), `string` (`i8*`), `void`.<br>- Short-circuit logic (`&&`, `||`), unary `!`.<br>- Semantic Analyzer Pass: Undefined var check, duplicate decl, const reassignment, loop context check. | [SemanticAnalyzer.cpp](file:///d:/HoangLong/Dev/vit/src/semantics/SemanticAnalyzer.cpp), [Expressions.h](file:///d:/HoangLong/Dev/vit/include/ast/Expressions.h), [Statements.h](file:///d:/HoangLong/Dev/vit/include/ast/Statements.h) |
| **Phase 3** | **Structs, Arrays & FFI** | - `struct` definition & field access (`p.x`).<br>- Heap Array allocation (`let arr = [10, 20, 30]`) & indexing (`arr[i]`).<br>- Type Inference: `let x = 10` (no explicit type annotation needed).<br>- C FFI: `extern function sqrt(x: number): number;`. | [Statements.h](file:///d:/HoangLong/Dev/vit/include/ast/Statements.h), [Expressions.h](file:///d:/HoangLong/Dev/vit/include/ast/Expressions.h), [SemanticAnalyzer.cpp](file:///d:/HoangLong/Dev/vit/src/semantics/SemanticAnalyzer.cpp) |
| **Phase 4** | **ARC, Modules & Tooling** | - ARC Scope Cleanup Pass: Auto insertion of `@free()` for heap arrays/structs at scope exit.<br>- Module System: `import { a, b } from "std/math";` & `import "mod";`. Recursive resolver protection.<br>- Standard Library: `std/math.vit` (`sqrt`, `cos`, `sin`, `pow`, `abs`...).<br>- Rich Rust-like Diagnostics: Colored ANSI terminal error reporting with `^` caret snippet pointing.<br>- Native Compiler Optimizations: `-O1`, `-O2`, `-O3` passed to Clang backend. | [DiagnosticPrinter.cpp](file:///d:/HoangLong/Dev/vit/src/diagnostics/DiagnosticPrinter.cpp), [LLVMCodeGen.cpp](file:///d:/HoangLong/Dev/vit/src/codegen/LLVMCodeGen.cpp), [main.cpp](file:///d:/HoangLong/Dev/vit/src/main.cpp), [std/math.vit](file:///d:/HoangLong/Dev/vit/std/math.vit) |

---

## 3. Language Features & Code Syntax Example (v0.4.0)

```javascript
// 1. Module System & Standard Library Import
import { sqrt, pow } from "std/math";

// 2. Struct Definition
struct Point {
    x: number,
    y: number
}

// 3. Foreign Function Interface (C FFI)
extern function abs(x: number): number;

// 4. Function Definition & Return Types
function calculateDistance(p: Point): number {
    return sqrt(pow(p.x, 2.0) + pow(p.y, 2.0));
}

function main(): number {
    // 5. Variables & Type Inference
    let p: Point;
    p.x = 3.0;
    p.y = 4.0;
    
    let dist = calculateDistance(p);
    print(dist); // Output: 5.000000

    // 6. Heap Array & Automatic Reference Cleanup (ARC)
    let numbers = [10, 20, 30]; // Heap allocated
    let i = 0;
    while (i < 3) {
        print(numbers[i]);
        i = i + 1;
    }
    // Auto insert call void @free(i8* %numbers) upon exiting main scope

    return 0;
}
```

---

## 4. Architectural Map & Codebase Layout

```text
vit/
├── include/
│   ├── ast/           # AST Nodes (ASTNode.h, Expressions.h, Statements.h, Functions.h, ASTVisitor.h)
│   ├── lexer/         # Token.h (TokenType), Lexer.h
│   ├── parser/        # Parser.h (Recursive Descent + Precedence Climbing)
│   ├── semantics/     # SemanticAnalyzer.h (Symbol tables & type checking)
│   ├── diagnostics/   # DiagnosticPrinter.h (Rust-like colored diagnostic output)
│   └── codegen/       # LLVMCodeGen.h (LLVM IR Generation & ARC Pass), NativeCompiler.h (Clang wrapper)
├── src/               # Implementation files (.cpp) matching include/
│   └── main.cpp       # CLI Command Handler (run, build, flags: -O1/-O2/-O3, --emit-ast, --emit-llvm)
├── std/
│   └── math.vit       # Standard Library module containing extern C math declarations
├── test/              # Test suites organized by Phase
│   ├── Phase-1/       # Basic expressions & functions
│   ├── Phase-2/       # Control flows & boolean/string types
│   ├── Phase-3/       # Structs, arrays, FFI, type inference
│   └── Phase-4/       # Imports, ARC cleanup, Rust-like errors, -O2 optimization
└── docs/              # Detailed specifications and logs
    ├── AI_CONTEXT_SUMMARY.md  # [THIS FILE] Compact context summary for AI
    ├── features/      # Detailed feature specs (current_features.md, phase2/3/4_features.md)
    └── history/       # Phase work logs (work_log.md, phase2/3/4_work_log.md)
```

---

## 5. Key Internal System Mechanics

1. **Memory Management (ARC Scope Cleanup)**:
   - Module `LLVMCodeGen` maintains a stack of heap-allocated pointers (`scopeHeapAllocations`).
   - When exiting any block (`BlockASTNode`), `LLVMCodeGen` generates `@free(i8* %ptr)` calls for all heap variables declared in that scope.
2. **Type Inference**:
   - `SemanticAnalyzer` & `LLVMCodeGen` evaluate `initializer` expressions on `VarDeclASTNode` when no explicit type annotation `: Type` is provided.
3. **Module Import Resolver**:
   - `resolveImports()` in `src/main.cpp` recursively resolves relative imports (`std/math`, `./mod.vit`), prevents duplicate/circular loading, and merges AST declarations into the main `ProgramASTNode`.
4. **Rich Diagnostics**:
   - `DiagnosticPrinter` catches `ParseError` or semantic exceptions, extracts source line code from file, and prints colored snippet pointers `^`.

---

## 6. How to Build & Run Compiler

```cmd
# 1. Build Compiler Binary (vit.exe)
cmake -B build -S .
cmake --build build --config Debug

# 2. Run source script directly
vit run test/Phase-4/test_import.vit

# 3. Build optimized Native Binary (.exe)
vit build test/Phase-4/test_import.vit -O2 -o app.exe

# 4. Debug LLVM IR / AST Output
vit run script.vit --emit-ast --emit-llvm
```
