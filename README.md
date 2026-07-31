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

---

## 🛠 Cấu Trúc Dự Án (Project Structure)

```text
vit/
├── CMakeLists.txt         # Cấu hình biên dịch CMake (C++20)
├── README.md              # Tài liệu hướng dẫn dự án
├── std/                   # Thư viện chuẩn VIT
│   └── math.vit
├── include/               # Header files (.h)
│   ├── ast/               # Cấu trúc Cây cú pháp trừu tượng (AST)
│   ├── diagnostics/       # Bộ in báo lỗi Rust-like rich diagnostics
│   ├── lexer/             # Bộ phân tích từ vựng (Lexer)
│   ├── parser/            # Bộ phân tích cú pháp (Parser)
│   ├── semantics/         # Bộ phân tích ngữ nghĩa (Semantic Analyzer)
│   └── codegen/           # Bộ sinh mã LLVM IR & Native Compiler
├── src/                   # Source code C++ (.cpp)
│   ├── ast/
│   ├── diagnostics/
│   ├── lexer/
│   ├── parser/
│   ├── semantics/
│   ├── codegen/
│   └── main.cpp           # Điểm vào chính của VIT Compiler CLI
├── docs/                  # Tài liệu chi tiết các Phase và Work Logs
│   ├── features/          # Đặc tả các tính năng
│   └── history/           # Lịch sử phát triển dự án
├── test/                  # Các file mã nguồn test (.vit) theo từng Phase
│   ├── Phase-1/
│   ├── Phase-2/
│   ├── Phase-3/
│   └── Phase-4/
└── scripts/               # Script tiện ích & cấu hình môi trường
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
cmake --build build --config Debug
```

---

### 2️⃣ Chạy Thử Chương Trình VIT (`.vit`)

```bash
# 1. Chạy file ví dụ Phase 4 (Module Import & Stdlib)
vit run test/Phase-4/test_import.vit

# 2. Biên dịch Native tối ưu hóa với -O2
vit build test/Phase-4/test_import.vit -O2 -o math_test.exe

# 3. Xem mã LLVM IR với tự động chèn free() của ARC
vit run test/Phase-4/test_arc.vit --emit-llvm
```

---

## 💡 Ví Dụ Mã Nguồn VIT (`.vit`)

```javascript
// Module Import & Standard Library
import { sqrt, pow } from "std/math";

struct Point {
    x: number,
    y: number
}

function main(): number {
    print("=== VIT Language v0.4.0 ===");

    let p: Point;
    p.x = 3.0;
    p.y = 4.0;

    let dist = sqrt(pow(p.x, 2.0) + pow(p.y, 2.0));
    print("Distance from origin:");
    print(dist); // In ra 5.000000

    return 0;
}
```

---

## 📜 Giấy Phép & Tác Giả (License)

Dự án được phát triển như một ngôn ngữ lập trình độc lập dựa trên hạ tầng LLVM Compiler Infrastructure.
