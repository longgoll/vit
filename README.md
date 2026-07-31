# 🚀 VIT Programming Language & Compiler

**VIT** là một ngôn ngữ lập trình định kiểu tĩnh (Statically-typed language) hiện đại, được biên dịch trực tiếp ra mã trung gian **LLVM IR** và đóng gói thành file thực thi Native (`.exe`). Compiler được thiết kế và cài đặt hoàn toàn bằng **C++20**.

---

## ✨ Tính Năng Nổi Bật (Key Features)

### 🔹 Phase 1 — Cốt Lõi Ngôn Ngữ (Language MVP)
* **Khai báo biến & hằng số**: Hỗ trợ `let` và `const`.
* **Biểu thức số học & So sánh**: `+`, `-`, `*`, `/`, `==`, `!=`, `<`, `>`, `<=`, `>=`.
* **Cấu trúc rẽ nhánh**: `if`, `else`.
* **Định nghĩa hàm**: Hỗ trợ khai báo hàm, truyền tham số, giá trị trả về (`return`).
* **Hàm in built-in**: `print(...)`.

### 🔹 Phase 2 — Mở Rộng Luồng Điều Khiển & Phân Tích Ngữ Nghĩa
* **Vòng lặp & Điều hướng**: `while`, `for`, `break`, `continue`.
* **Mở rộng hệ thống kiểu**: `boolean` (`true`/`false`), `string` (`"..."`), `void`.
* **Toán tử Logic Short-Circuit**: `&&`, `||`, `!`.
* **Bộ Phân Tích Ngữ Nghĩa (Semantic Analyzer)**: Báo lỗi biến chưa khai báo, trùng tên, gán lại hằng `const`, sai vị trí `break`/`continue`.

### 🔹 Phase 3 — FFI, Struct, Mảng & Type Inference
* **C Interop (FFI)**: Gọi trực tiếp các hàm từ C Runtime thông qua từ khóa `extern function`.
* **Cấu trúc dữ liệu (`struct`)**: Định nghĩa kiểu phức hợp tùy chỉnh.
* **Mảng dữ liệu (`Array`)**: Khởi tạo mảng `[...]` và truy xuất theo chỉ số `arr[i]`.
* **Suy luận kiểu tự động (`Type Inference`)**: Tự động xác định kiểu dữ liệu của biến khai báo với `let`.

### 🔹 Phase 4 — ARC Memory Cleanup, Module System, Rich Diagnostics & Native Optimizations
* **Tự Động Quản Lý Bộ Nhớ Scope (ARC Cleanup)**: Tự động phát sinh lệnh `@free(i8*)` giải phóng mảng/struct Heap khi thoát khỏi Scope mà không cần Garbage Collector giật lag.
* **Hệ Thống Module (`import`) & Thư Viện Chuẩn**: Chia nhỏ dự án thành nhiều file `import { sqrt, cos } from "std/math";`. Tích hợp sẵn `std/math.vit`.
* **Báo Lỗi Dạng Rust-Like (Rich Error Diagnostics)**: In dòng lỗi màu sắc ANSI, trích đoạn code thực tế và con trỏ `^` chỉ vị trí lỗi.
* **Cờ Tối Ưu Hóa Native (`-O1`, `-O2`, `-O3`)**: Kích hoạt bộ tối ưu hóa LLVM/Clang cho file `.exe`.

### 🔹 Phase 5 — Struct Methods, String Operations & Length Metadata
* **Phương Thức Struct (`this`)**: Định nghĩa method trong `struct`, gọi `obj.method()`.
* **Xử Lý Chuỗi (`string`)**: Phép cộng chuỗi `+` (với ARC cleanup), so sánh `==`/`!=`, truy cập độ dài `.length`.
* **Array Length Metadata**: Header prefixmetadata lưu độ dài mảng cho `.length`.

### 🔹 Phase 6 — Functional Programming & Lambdas
* **Hàm Hạng Nhất (First-class functions) & Lambdas**: Hàm ẩn danh / Arrow Functions `(x: number) => x * 2.0`.
* **Phương Thức Mảng Hạng Cao**: Tích hợp sẵn `.map()`, `.filter()`, `.forEach()`.

### 🔹 Phase 7 — Generics, Enums, Pattern Matching & System FFI
* **Generics (Monomorphization)**: Định nghĩa `struct Stack<T>`, `function identity<T>()`.
* **Enums / Tagged Unions**: Định nghĩa kiểu liệt kê `enum Option<T> { Some(T), None }`.
* **Pattern Matching (`match`)**: Cấu trúc rẽ nhánh khớp mẫu `match (expr) { Option.Some(v) => ... }`.
* **System File I/O FFI**: Thư viện `std/fs.vit` (`readFile`, `writeFile`), `std/io.vit`.

### 🔹 Phase 8 — Advanced Error Handling & Safety (v0.8.0)
* **Try Operator (`?`)**: Unwrapping ngắn gọn cho `Result<T, E>` / `Option<T>` với cơ chế early-return khi gặp lỗi.
* **Strict Null Safety**: Kiểu dữ liệu Nullable (`T?`), Optional Chaining (`obj?.prop`), Nullish Coalescing (`a ?? b`), và giá trị `null`.
* **Runtime Bounds Checking**: Tự động so sánh chỉ số mảng và gọi handler `@__vit_panic("Index out of bounds")` khi truy cập vượt giới hạn.
* **Panic System & Assertions**: Hàm built-in `panic(msg)` và `assert(condition, msg)`.

### 🔹 Phase 9 — Built-in Collections & Advanced Stdlib (v0.9.0)
* **Tập Hợp Cốt Lõi (`HashMap`, `Set`)**: Cấu trúc dữ liệu `HashMap<K, V>` và `Set<T>` được liên kết với C Runtime (`collections_rt.c`).
* **Tiện Ích Hệ Thống & CLI**: Đọc cờ tham số dòng lệnh (`getArgCount()`, `getArg()`) và biến môi trường (`getEnv()`).
* **Chuẩn Hóa JSON**: Module `std/json.vit` hỗ trợ encode/escape văn bản JSON.

### 🔹 Phase 10 — Self-Hosting Compiler (v1.0.0 Milestone)
* **Compiler Tự Biên Dịch 100%**: Mã nguồn trình biên dịch VIT được viết hoàn toàn bằng chính ngôn ngữ VIT (`src_vit/`).
* **Quy Trình Bootstrapping 3 Giai Đoạn**: Khởi chạy từ `vit.exe` (Stage 0) ➔ `vitc_stage1.exe` ➔ `vitc_stage2.exe` kiểm thử độc lập.

### 🔹 Phase 11 — Concurrency & Async Engine (v1.1.0)
* **Bất Đồng Bộ (`async` / `await`)**: Từ khóa `async` và `await` tích hợp trực tiếp với hạ tầng `Promise<T>` và mã trung gian LLVM IR.
* **Đa Luồng & Channel**: Spawning OS Thread và truyền dữ liệu qua kênh đồng bộ an toàn luồng (`std/thread`, `std/channel`) trên hạ tầng Win32 API.
* **Monormorphizer Multi-statement Pass**: Tự động nhân bản và thay thế kiểu cho phương thức mảng/struct phức hợp.

---

## 🛠 Cấu Trúc Dự Án (Project Structure)

```text
vit/
├── CMakeLists.txt         # Cấu hình biên dịch CMake (C++20)
├── README.md              # Tài liệu hướng dẫn dự án
├── std/                   # Thư viện chuẩn VIT (math.vit, string.vit, array.vit, sys.vit, fs.vit, io.vit)
├── include/               # Header files (.h)
│   ├── ast/               # Cấu trúc Cây cú pháp trừu tượng (AST)
│   ├── diagnostics/       # Bộ in báo lỗi Rust-like rich diagnostics
│   ├── lexer/             # Bộ phân tích từ vựng (Lexer)
│   ├── parser/            # Bộ phân tích cú pháp (Parser)
│   ├── semantics/         # Semantic Analyzer & Monomorphizer Pass (Generics)
│   └── codegen/           # Bộ sinh mã LLVM IR & Native Compiler Wrapper
├── src/                   # Source code C++ (.cpp)
│   ├── ast/
│   ├── diagnostics/
│   ├── lexer/
│   ├── parser/
│   ├── semantics/
│   ├── codegen/
│   └── main.cpp           # Điểm vào chính của VIT Compiler CLI
├── docs/                  # Tài liệu chi tiết các Phase và Work Logs
│   ├── AI_CONTEXT_SUMMARY.md # Context tóm tắt Phase 1-8 dành cho AI Code Assistant
│   ├── features/          # Đặc tả tính năng từ Phase 1 tới Phase 13
│   └── history/           # Lịch sử phát triển & Work logs (Phase 1-8)
└── test/                  # Các file test (.vit) phân theo từng Phase (Phase-1 -> Phase-8)
```

---

## 🚀 Hướng Dẫn Biên Dịch & Chạy (Getting Started)

### 📌 Yêu Cầu Tiền Trạm (Prerequisites)
* **C++20 Compiler**: GCC / Clang / MSVC.
* **CMake**: Phiên bản `>= 3.16`.
* **LLVM & Clang**: (Để dịch mã IR thành `.exe`).

---

### 1️⃣ Biên Dịch VIT Compiler

```bash
# Tạo thư mục build và cấu hình CMake
cmake -B build -S .

# Biên dịch chương trình
cmake --build build --config Release
```

---

### 2️⃣ Chạy Thử Chương Trình VIT (`.vit`)

```bash
# 1. Chạy file ví dụ Phase 11 (Async & Promise Engine)
vit run test/Phase-11/test_async_basic.vit

# 2. Chạy file ví dụ Multi-threading & Channels
vit run test/Phase-11/test_threads_channels.vit

# 3. Biên dịch Native tối ưu hóa với -O2
vit build test/Phase-11/test_threads_channels.vit -O2 -o thread_test.exe
```

---

## 💡 Ví Dụ Mã Nguồn VIT (`.vit`)

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

## 📜 Giấy Phép & Tác Giả (License)

Dự án được phát triển như một ngôn ngữ lập trình độc lập dựa trên hạ tầng LLVM Compiler Infrastructure.
