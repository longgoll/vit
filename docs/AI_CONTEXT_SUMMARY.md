# VIT Compiler - Complete Architecture & Phase 1-14 Summary (AI Context Document)

> **Purpose**: Tài liệu tóm tắt toàn bộ kiến trúc, cú pháp, tính năng và lịch sử 14 Phase đã hoàn thành (v2.0.0 Release) của trình biên dịch **VIT Compiler**. File này được tối ưu hóa cho AI Coding Assistant đọc nhanh, giúp nắm bắt 100% ngữ cảnh dự án với lượng token tối thiểu.

---

## 1. Quick Facts & Tech Stack
* **Project**: VIT Compiler (`v2.0.0 Release`) - Ngôn ngữ biên dịch Native có cú pháp giống JS/TS.
* **Implementation**: C++20 / Self-Hosted Vit (`src_vit/`), LLVM C++ API (IRBuilder, LLVM IR CodeGen), Monomorphizer Pass, ARC Escape Analysis Pass (`EscapeAnalysis.cpp`), Win32 Concurrency Runtime (`concurrency_rt.c`), Socket C Runtime (`net_rt.c`), Clang Native & Cross Linker, LSP JSON-RPC Server (`vit-lsp`), Package Manager, REPL, Formatter & Linter.
* **Compilation Pipeline**: `Source (.vit)` ➔ `Lexer` ➔ `Parser` ➔ `Monomorphizer (Generics Pass)` ➔ `SemanticAnalyzer (Type Check)` ➔ `EscapeAnalyzer (ARC Pass)` ➔ `LLVMCodeGen (IR)` ➔ `NativeCompiler (Clang --target=<triple> -O1/-O2/-O3)` ➔ `Executable (.exe/.wasm)`.

---

## 2. Summary Timeline: Phase 1 -> Phase 14

| Phase | Milestone Name | Features & Scope | Key Files Updated |
|---|---|---|---|
| **Phase 1** | **MVP Foundation** | - Type: `number` (IEEE 754 f64 / `double`).<br>- Variables: `let`, `const`.<br>- AST, Hand-written Lexer/Parser, LLVM IR CodeGen, Clang Native link.<br>- CLI subcommands: `vit run`, `vit build`, `vit version`, `vit help`. | [Lexer.cpp](file:///f:/Dev/product/vit/src/lexer/Lexer.cpp), [Parser.cpp](file:///f:/Dev/product/vit/src/parser/Parser.cpp), [LLVMCodeGen.cpp](file:///f:/Dev/product/vit/src/codegen/LLVMCodeGen.cpp), [NativeCompiler.cpp](file:///f:/Dev/product/vit/src/codegen/NativeCompiler.cpp), [main.cpp](file:///f:/Dev/product/vit/src/main.cpp) |
| **Phase 2** | **Control Flow & Semantics** | - Control flow: `if/else`, `while`, `for`, `break`, `continue`.<br>- Types: `boolean` (`true`/`false`), `string` (`i8*`), `void`.<br>- Short-circuit logic (`&&`, `||`), unary `!`.<br>- Semantic Analyzer Pass: Undefined var check, duplicate decl, const reassignment, loop context check. | [SemanticAnalyzer.cpp](file:///f:/Dev/product/vit/src/semantics/SemanticAnalyzer.cpp), [Expressions.h](file:///f:/Dev/product/vit/include/ast/Expressions.h), [Statements.h](file:///f:/Dev/product/vit/include/ast/Statements.h) |
| **Phase 3** | **Structs, Arrays & FFI** | - `struct` definition & field access (`p.x`).<br>- Heap Array allocation (`let arr = [10, 20, 30]`) & indexing (`arr[i]`).<br>- Type Inference: `let x = 10` (no explicit type annotation needed).<br>- C FFI: `extern function sqrt(x: number): number;`. | [Statements.h](file:///f:/Dev/product/vit/include/ast/Statements.h), [Expressions.h](file:///f:/Dev/product/vit/include/ast/Expressions.h), [SemanticAnalyzer.cpp](file:///f:/Dev/product/vit/src/semantics/SemanticAnalyzer.cpp) |
| **Phase 4** | **ARC, Modules & Tooling** | - ARC Scope Cleanup Pass: Auto insertion of `@free()` for heap arrays/structs at scope exit.<br>- Module System: `import { a, b } from "std/math";` & `import "mod";`. Recursive resolver protection.<br>- Standard Library: `std/math.vit` (`sqrt`, `cos`, `sin`, `pow`, `abs`...).<br>- Rich Rust-like Diagnostics: Colored ANSI terminal error reporting with `^` caret snippet pointing.<br>- Native Compiler Optimizations: `-O1`, `-O2`, `-O3` passed to Clang backend. | [DiagnosticPrinter.cpp](file:///f:/Dev/product/vit/src/diagnostics/DiagnosticPrinter.cpp), [LLVMCodeGen.cpp](file:///f:/Dev/product/vit/src/codegen/LLVMCodeGen.cpp), [main.cpp](file:///f:/Dev/product/vit/src/main.cpp), [std/math.vit](file:///f:/Dev/product/vit/std/math.vit) |
| **Phase 5** | **Struct Methods & String Operations** | - Struct Methods (`this`): Declare methods inside `struct`, call `obj.method()`.<br>- String Ops: Concatenation `+` (with ARC Heap cleanup), equality `==`/`!=` (`strcmp`), `.length`.<br>- Array `.length`: Header prefix length metadata.<br>- Stdlib: `std/string.vit` C interop.<br>- Main return type `i32 0` exit code fix. | [Parser.cpp](file:///f:/Dev/product/vit/src/parser/Parser.cpp), [SemanticAnalyzer.cpp](file:///f:/Dev/product/vit/src/semantics/SemanticAnalyzer.cpp), [LLVMCodeGen.cpp](file:///f:/Dev/product/vit/src/codegen/LLVMCodeGen.cpp), [std/string.vit](file:///f:/Dev/product/vit/std/string.vit) |
| **Phase 6** | **Functional Programming & Lambdas** | - First-class functions, Lambdas / Arrow Functions (`(x: number) => x * 2`).<br>- Function types (`(a: number) => number`).<br>- Higher-order array methods: `.map()`, `.filter()`, `.forEach()`.<br>- Stdlib: `std/array.vit`, `std/sys.vit`. | [Parser.cpp](file:///f:/Dev/product/vit/src/parser/Parser.cpp), [SemanticAnalyzer.cpp](file:///f:/Dev/product/vit/src/semantics/SemanticAnalyzer.cpp), [LLVMCodeGen.cpp](file:///f:/Dev/product/vit/src/codegen/LLVMCodeGen.cpp), [std/array.vit](file:///f:/Dev/product/vit/std/array.vit) |
| **Phase 7** | **Generics, Enums & System FFI** | - Generics (`struct Stack<T>`, `fn identity<T>`) via Monomorphizer.<br>- Enums / Tagged Unions (`enum Option<T> { Some(val: T), None }`).<br>- Pattern Matching (`match (expr) { Option.Some(v) => ... }`).<br>- File System FFI & Standard Console (`std/fs.vit`, `std/io.vit`). | [Monomorphizer.cpp](file:///f:/Dev/product/vit/src/semantics/Monomorphizer.cpp), [Parser.cpp](file:///f:/Dev/product/vit/src/parser/Parser.cpp), [LLVMCodeGen.cpp](file:///f:/Dev/product/vit/src/codegen/LLVMCodeGen.cpp), [std/fs.vit](file:///f:/Dev/product/vit/std/fs.vit) |
| **Phase 8** | **Advanced Error Handling & Safety** | - Try Operator (`?`) for `Result`/`Option` short-circuit unwrapping.<br>- Strict Null Safety: `null`, Nullable Types (`T?`), Optional Chaining (`?.`), Nullish Coalescing (`??`).<br>- Runtime Array Bounds Checking: `icmp uge` checks with clean `@__vit_panic`.<br>- Assertions & Panic system primitives: `assert()`, `panic()`. | [Lexer.cpp](file:///f:/Dev/product/vit/src/lexer/Lexer.cpp), [Parser.cpp](file:///f:/Dev/product/vit/src/parser/Parser.cpp), [SemanticAnalyzer.cpp](file:///f:/Dev/product/vit/src/semantics/SemanticAnalyzer.cpp), [LLVMCodeGen.cpp](file:///f:/Dev/product/vit/src/codegen/LLVMCodeGen.cpp) |
| **Phase 9** | **Built-in Collections & Advanced Stdlib** | - `HashMap<K, V>` & `Set<T>` backed by C runtime (`collections_rt.c`).<br>- CLI args (`getArgCount()`, `getArg()`) & System environment (`getEnv()`).<br>- JSON stringify & escape utilities (`std/json.vit`). | [collections_rt.c](file:///f:/Dev/product/vit/src/runtime/collections_rt.c), [collections.vit](file:///f:/Dev/product/vit/std/collections.vit), [env.vit](file:///f:/Dev/product/vit/std/env.vit), [LLVMCodeGen.cpp](file:///f:/Dev/product/vit/src/codegen/LLVMCodeGen.cpp) |
| **Phase 10** | **Self-Hosting Compiler** | - 100% Native Self-Hosting Compiler written in Vit (`src_vit/`).<br>- 3-stage bootstrapping workflow (`vitc_stage1.exe`, `vitc_stage2.exe`). | [src_vit/main.vit](file:///f:/Dev/product/vit/src_vit/main.vit), [src_vit/lexer/lexer.vit](file:///f:/Dev/product/vit/src_vit/lexer/lexer.vit), [src_vit/parser/parser.vit](file:///f:/Dev/product/vit/src_vit/parser/parser.vit), [src_vit/codegen/llvm_emitter.vit](file:///f:/Dev/product/vit/src_vit/codegen/llvm_emitter.vit) |
| **Phase 11** | **Concurrency & Async Engine** | - `async`/`await` language keywords & LLVM lowering.<br>- `Promise<T>` handling.<br>- Native multi-threading & thread-safe message-passing channels (`std/thread`, `std/channel`).<br>- Full multi-statement generic AST cloning in Monomorphizer. | [concurrency_rt.c](file:///f:/Dev/product/vit/src/runtime/concurrency_rt.c), [std/thread.vit](file:///f:/Dev/product/vit/std/thread.vit), [std/async.vit](file:///f:/Dev/product/vit/std/async.vit), [Monomorphizer.cpp](file:///f:/Dev/product/vit/src/semantics/Monomorphizer.cpp), [LLVMCodeGen.cpp](file:///f:/Dev/product/vit/src/codegen/LLVMCodeGen.cpp) |
| **Phase 12** | **Network Engine & Web Server Framework** | - Non-blocking TCP/UDP sockets (`std/net`).<br>- Native C socket runtime (`net_rt.c`, `-lws2_32`).<br>- HTTP/1.1 Protocol Parser, Async HTTP Server (`HttpServer`) & Client (`httpGet`). | [net_rt.c](file:///f:/Dev/product/vit/src/runtime/net_rt.c), [net.vit](file:///f:/Dev/product/vit/std/net.vit), [http.vit](file:///f:/Dev/product/vit/std/http.vit), [NativeCompiler.cpp](file:///f:/Dev/product/vit/src/codegen/NativeCompiler.cpp) |
| **Phase 13** | **Developer Experience & Ecosystem** | - Language Server Protocol (`vit-lsp` JSON-RPC over stdin/stdout).<br>- Package Manager (`vit init`, `vit add`, `vit install`, `vit.json`).<br>- Interactive REPL shell (`vit repl`).<br>- Code Formatter (`vit fmt`) & Linter (`vit lint`). | [LSP.cpp](file:///f:/Dev/product/vit/src/tools/LSP.cpp), [PackageManager.cpp](file:///f:/Dev/product/vit/src/tools/PackageManager.cpp), [REPL.cpp](file:///f:/Dev/product/vit/src/tools/REPL.cpp), [Formatter.cpp](file:///f:/Dev/product/vit/src/tools/Formatter.cpp), [Linter.cpp](file:///f:/Dev/product/vit/src/tools/Linter.cpp), [main.cpp](file:///f:/Dev/product/vit/src/main.cpp) |
| **Phase 14** | **Cross-Compilation, WASM & ARC Optimization** | - Multi-target Cross-Compilation (`--target <triple>`).<br>- WebAssembly target module support (`wasm32-wasi` / `.wasm`).<br>- LLVM Custom ARC Escape Analysis Pass (`--enable-escape-analysis`) stack-allocates non-escaping objects. | [EscapeAnalysis.cpp](file:///f:/Dev/product/vit/src/codegen/EscapeAnalysis.cpp), [LLVMCodeGen.cpp](file:///f:/Dev/product/vit/src/codegen/LLVMCodeGen.cpp), [NativeCompiler.cpp](file:///f:/Dev/product/vit/src/codegen/NativeCompiler.cpp), [main.cpp](file:///f:/Dev/product/vit/src/main.cpp) |

---

---

## 3. Language Features & Code Syntax Example (v0.7.0)

```javascript
import { readFile, writeFile } from "std/fs";
import { sqrt } from "std/math";

// 1. Generic Struct & Enums
struct Stack<T> {
    items: T[],
    count: number
}

enum Option<T> {
    Some(val: T),
    None
}

// 2. Struct with Methods & 'this'
struct Point {
    x: number,
    y: number,
    function distance(): number {
        return sqrt(this.x * this.x + this.y * this.y);
    }
}

function main(): number {
    // 3. Lambda & Array Methods (.map)
    let numbers = [1.0, 2.0, 3.0, 4.0];
    let doubled = numbers.map((x: number): number => x * 2.0);

    // 4. Generics & Pattern Matching
    let opt = Option.Some(42.0);
    match (opt) {
        Option.Some(val) => {
            print(val);
        },
        Option.None => {
            print("No value");
        }
    }

    // 5. System File I/O
    let content = readFile("input.txt");
    print(content);

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
│   ├── semantics/     # SemanticAnalyzer.h, Monomorphizer.h (Generics monomorphization pass)
│   ├── diagnostics/   # DiagnosticPrinter.h (Rust-like colored diagnostic output)
│   └── codegen/       # LLVMCodeGen.h (LLVM IR Generation & ARC Pass), NativeCompiler.h (Clang wrapper)
├── src/               # Implementation files (.cpp) matching include/
│   └── main.cpp       # CLI Command Handler (run, build, flags: -O1/-O2/-O3, --emit-ast, --emit-llvm)
├── std/
│   ├── math.vit       # Standard Library module for math
│   ├── string.vit     # Standard Library module for string ops
│   ├── array.vit      # Dynamic array operations
│   ├── sys.vit        # System clock and exit helpers
│   ├── fs.vit         # File System I/O (readFile, writeFile)
│   └── io.vit         # Console I/O
├── test/              # Test suites organized by Phase (Phase-1 to Phase-7)
└── docs/              # Specifications and work logs
    ├── AI_CONTEXT_SUMMARY.md  # [THIS FILE] Compact context summary for AI
    ├── features/      # Feature specs (phase6_features.md, phase7_features.md, phase8-12_features.md)
    └── history/       # Phase work logs (phase6_work_log.md, phase7_work_log.md)
```

---

## 5. Completed Roadmap (Phase 1 -> Phase 14 Completed - v2.0.0 Release)

All 14 planned phases of the VIT Compiler ecosystem have been successfully implemented and verified:

### 🔥 **Phase 14: Cross-Compilation, WASM & ARC Optimization (`v2.0.0 Release Milestone`)**
* **Spec File**: [phase14_features.md](file:///f:/Dev/product/vit/docs/features/phase14_features.md)
* **Scope**: Target triples (`--target x86_64-linux`, `aarch64-darwin`, `wasm32-wasi`), WebAssembly backend, Custom LLVM ARC Escape Analysis Pass (`--enable-escape-analysis`).
* **Work Log**: [phase14_work_log.md](file:///f:/Dev/product/vit/docs/history/phase14_work_log.md)



