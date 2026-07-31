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
# 1. Chạy file ví dụ Phase 8 (Try Operator & Null Safety)
vit run test/Phase-8/test_null_safety.vit

# 2. Biên dịch Native tối ưu hóa với -O2
vit build test/Phase-8/test_try_operator.vit -O2 -o try_test.exe

# 3. Xem mã LLVM IR với bounds check & ARC cleanup
vit run test/Phase-8/test_bounds_check.vit --emit-llvm
```

---

## 💡 Ví Dụ Mã Nguồn VIT (`.vit`)

```javascript
import { readFile } from "std/fs";

struct User {
    id: number,
    name: string,
    email: string? // Nullable type
}

function parseUser(raw: string): Option<User> {
    if (raw.length == 0) {
        return Option.None;
    }
    let u: User;
    u.id = 1.0;
    u.name = "Hoang Long";
    u.email = null;
    return Option.Some(u);
}

function main(): number {
    print("=== VIT Language v0.8.0 ===");

    // 1. Try Operator (?) & Null Safety (?. / ??)
    let raw = readFile("user.json")?;
    let user = parseUser(raw)?;

    let email = user?.email ?? "no-email@domain.com";
    print(email);

    // 2. Runtime Array Bounds Check & Panic Safety
    let numbers = [10.0, 20.0, 30.0];
    assert(numbers.length == 3.0, "Array length must be 3");

    print(numbers[0]);

    return 0;
}
```

---

## 📜 Giấy Phép & Tác Giả (License)

Dự án được phát triển như một ngôn ngữ lập trình độc lập dựa trên hạ tầng LLVM Compiler Infrastructure.
