# 🚀 VIT Programming Language & Compiler Engine

[English](#english) | [Tiếng Việt](#tiếng-việt)

---

<a name="english"></a>
## 🇬🇧 English Documentation

### Overview
**VIT** is a modern, statically-typed programming language compiled directly to **LLVM IR** and native executable binaries (`.exe`). The compiler core is built completely from scratch using **C++20**.

### ✨ Key Features Matrix
- **Phase 1 — Language Core (MVP)**: `let`, `const`, basic math, conditionals `if/else`, function definitions, and built-in `print()`.
- **Phase 2 — Control Flow & Semantics**: `while`, `for`, `break`, `continue`, logic operators `&&`, `||`, `!`, and semantic validation pass.
- **Phase 3 — FFI, Structs, Arrays & Type Inference**: Foreign Function Interface (`extern function`), custom `struct` definitions, static arrays `[...]`, and implicit type inference.
- **Phase 4 — ARC Memory Management & Stdlib Modules**: Automatic Reference Counting (Scope-based ARC) without GC pauses, `import` module system (`std/math`, `std/string`), and rich Rust-style diagnostics.
- **Phase 5 — Struct Methods & String Metadata**: Object-oriented `this` methods, heap string concatenation with ARC, and `.length` metadata properties.
- **Phase 6 — Functional Programming & Lambdas**: First-class functions, closures, Arrow functions `(x: number) => x * 2.0`, `.map()`, `.filter()`, `.forEach()`.
- **Phase 7 — Generics & Pattern Matching**: Type monomorphization (`struct Stack<T>`), `enum` tagged unions, pattern matching (`match`), and system I/O (`std/fs.vit`).
- **Phase 8 — Advanced Error Handling & Safety**: Try Operator (`?`) for `Result`/`Option` types, strict null safety (`T?`, `??`, `?.`), and runtime array bounds panic checks.
- **Phase 9 — Built-in Collections & Stdlib**: Native C-runtime `HashMap<K, V>`, `Set<T>`, CLI arguments (`getArgCount()`, `getArg()`), and `std/json.vit`.
- **Phase 10 — Self-Hosting Compiler**: Bootstrap pipeline where the VIT compiler compiles its own source code (`src_vit/`).
- **Phase 11 — Async Engine & Concurrency**: First-class `async` / `await` runtime backed by `Promise<T>`, multi-threading (`std/thread`), and lock-free thread-safe channels (`std/channel`).

---

### 🛠 Project Architecture

```text
vit/
├── CMakeLists.txt         # C++20 CMake Build Configuration
├── README.md              # Project Documentation
├── std/                   # VIT Standard Library (math, string, array, sys, fs, io, thread)
├── include/               # C++ Header Files
│   ├── ast/               # Abstract Syntax Tree definitions
│   ├── diagnostics/       # Rich Rust-like ANSI diagnostic logger
│   ├── lexer/             # Tokenizer & Lexical Analyzer
│   ├── parser/            # Recursive Descent Parser
│   ├── semantics/         # Semantic Analyzer, Type Checker & Monomorphizer
│   └── codegen/           # LLVM IR CodeGenerator & Clang Linker Driver
├── src/                   # C++ Implementation Files (.cpp)
├── src_vit/               # Self-hosted compiler implementation in VIT language
├── docs/                  # Architectural documentation & milestone logs
└── test/                  # Test suite covering Phases 1 to 13
```

---

### 🚀 CLI Usage & Quick Start

```bash
# 1. Build Compiler (Release Mode)
cmake -B build -S .
cmake --build build --config Release

# 2. Run VIT Source Code Directly
vit run test/Phase-11/test_async_basic.vit

# 3. Compile to Native Optimized Executable (-O2)
vit build main.vit -O2 -o app.exe

# 4. Start Language Server Protocol (LSP) Engine
vit --lsp
```

---

<a name="tiếng-việt"></a>
## 🇻🇳 Tài Liệu Tiếng Việt

### 🌟 Tổng Quan
**VIT** là một ngôn ngữ lập trình định kiểu tĩnh (Statically-typed language) hiện đại, được biên dịch trực tiếp ra mã trung gian **LLVM IR** và đóng gói thành tệp thực thi Native (`.exe`). Compiler được thiết kế và cài đặt hoàn toàn từ đầu bằng **C++20**.

### ✨ Tính Năng Nổi Bật Qua Các Giai Đoạn (Phases)
* **Phase 1 — Cốt Lõi Ngôn Ngữ**: Khai báo `let`, `const`, phép toán cơ bản, cấu trúc rẽ nhánh `if/else`, khai báo hàm và hàm in `print()`.
* **Phase 2 — Luồng Điều Khiển & Phân Tích Ngữ Nghĩa**: Vòng lặp `while`, `for`, `break`, `continue`, toán tử logic `&&`, `||`, `!`, và bộ kiểm tra lỗi Semantic.
* **Phase 3 — C FFI, Struct, Mảng & Type Inference**: Tương thích C Runtime (`extern function`), định nghĩa `struct`, mảng `[...]`, và tự động suy luận kiểu dữ liệu.
* **Phase 4 — Quản Lý Bộ Nhớ ARC & Hệ Thống Module**: Tự động giải phóng vùng nhớ Scope (ARC Cleanup) không cần Garbage Collector, hệ thống module `import`, và báo lỗi phong cách Rust-like.
* **Phase 5 — Phương Thức Struct & Thao Tác Chuỗi**: Định nghĩa method với con trỏ `this`, nối chuỗi `+` tự động dọn dẹp bộ nhớ heap và thuộc tính `.length`.
* **Phase 6 — Lập Trình Hàm & Lambdas**: Hàm hạng nhất (First-class functions), hàm ẩn danh / Arrow Functions `(x: number) => x * 2.0`, `.map()`, `.filter()`, `.forEach()`.
* **Phase 7 — Generics & Pattern Matching**: Kỹ thuật Monomorphization (`struct Stack<T>`), kiểu liệt kê `enum`, rẽ nhánh khớp mẫu `match`, và giao tiếp file hệ thống `std/fs.vit`.
* **Phase 8 — Xử Lý Lỗi Nâng Cao & Safe Execution**: Toán tử giải nén lỗi ngắn gọn `?` cho `Result`/`Option`, null safety (`T?`, `??`, `?.`), và kiểm tra tràn mảng Runtime.
* **Phase 9 — Tập Hợp Cốt Lõi (`HashMap`, `Set`) & Stdlib**: Tích hợp cấu trúc dữ liệu `HashMap<K, V>`, `Set<T>`, đọc cờ tham số CLI và xử lý chuỗi JSON `std/json.vit`.
* **Phase 10 — Trình Biên Dịch Tự Biên Dịch (Self-Hosting)**: Khả năng biên dịch chính bộ mã nguồn trình biên dịch VIT được viết bằng 100% ngôn ngữ VIT (`src_vit/`).
* **Phase 11 — Động Cơ Bất Đồng Bộ (`async`/`await`) & Concurrency**: Hỗ trợ từ khóa `async` / `await` trực tiếp ở mức bytecode LLVM, khởi tạo OS Thread (`std/thread`), và truyền dữ liệu qua kênh an toàn luồng (`std/channel`).

---

### 💡 Ví Dụ Mã Nguồn VIT (`.vit`)

```javascript
import { createChannel, spawnThread } from "std/thread";

async function computeAsync(val: number): number {
    print("Executing async calculation...");
    return val * 2.0;
}

function worker(chHandle: string): number {
    print("Worker thread processing data...");
    let sum = 0.0;
    for (let i = 1.0; i <= 10.0; i = i + 1.0) {
        sum = sum + i;
    }
    vit_channel_send(chHandle, sum);
    return 0.0;
}

function main(): number {
    print("=== VIT Language v1.1.0 ===");

    // 1. Async / Await Engine
    let promiseVal = await computeAsync(21.0);
    print("Async Result: " + promiseVal);

    // 2. Multi-threading & Synchronization Channels
    let ch = createChannel<number>();
    let t = spawnThread(worker, ch.handle);

    let channelVal = ch.receive();
    print("Main thread received channel result: " + channelVal);

    t.join();
    ch.close();

    return 0;
}
```

---

## 📜 Giấy Phép & Phát Triển (License)
Dự án nguồn mở được phát triển hoàn toàn độc lập cho hệ sinh thái **VIT Language**. MIT License.
