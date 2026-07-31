# Tổng Quan Chức Năng Đang Có (Current Features & Architecture v0.5.0)

Tài liệu này dành cho lập trình viên làm việc trên codebase của dự án **VIT Compiler (v0.5.0)**. Tài liệu mô tả các tính năng ngôn ngữ đã hỗ trợ, kiến trúc codebase, cấu trúc thư mục, và hướng dẫn biên dịch/sử dụng.

---

## 1. Các Chức Năng Ngôn Ngữ Hiện Có (Supported Features)

### 1.1. Kiểu dữ liệu & Biến
* **Primitive Types**: `number` (float64 IEEE 754 `double`), `boolean` (`true`/`false`), `string`, `void`.
* **Composite Types**:
  * **Mảng dữ liệu (`Array`)**: `let arr: number[] = [10, 20, 30];`
  * **Cấu trúc dữ liệu (`struct`)**: `struct Point { x: number, y: number }`
* **Phương thức của Struct (`Struct Methods`)**: `struct Point { function distance(): number { ... } }` với con trỏ `this`.
* **Khai báo biến & Suy luận kiểu (`Type Inference`)**:
  * `let x = 10;` (Tự suy luận kiểu `number`)
  * `let name = "VIT";` (Tự suy luận kiểu `string`)
  * `const MAX = 100;`
* **Phép gán**: `x = y + 5;`, `p.x = 10;`, `arr[0] = 99;`, `this.x = 10;`

### 1.2. Biểu thức, Toán tử & Thao Tác Chuỗi
* **Toán tử số học**: `+`, `-`, `*`, `/`.
* **Toán tử so sánh**: `==`, `!=`, `<`, `>`, `<=`, `>=`.
* **Toán tử logic**: `&&`, `||`, `!`.
* **Thao tác chuỗi**:
  * Nối chuỗi với toán tử `+`: `let name = "VIT " + "Compiler";` (Heap allocated & ARC managed)
  * So sánh chuỗi với `==` và `!=` (dựa trên C `strcmp`)
  * Độ dài chuỗi & mảng: `str.length`, `arr.length`

### 1.3. Câu lệnh điều khiển (Control Flow)
* **Khối lệnh (Block)**: `{ stmt1; stmt2; }`
* **Câu điều kiện `if/else`**: `if (cond) { ... } else { ... }`
* **Vòng lặp `while` & `for`**: `while (cond) { ... }`, `for (init; cond; update) { ... }`
* **Lệnh rẽ nhánh**: `break;`, `continue;`

### 1.4. Hàm, C Interop & Module System
* **Định nghĩa hàm**: `function add(a: number, b: number): number { return a + b; }`
* **Giao tiếp C FFI**: `extern function sqrt(x: number): number;`
* **Hệ thống Module (`import`)**: `import { sqrt, cos } from "std/math";` hoặc `import "std/math";`
* **Thư viện chuẩn**: Standard Library `std/math.vit` & `std/string.vit`.

### 1.5. Tự Động Quản Lý Bộ Nhớ (ARC Scope Memory Cleanup)
* Tự động giải phóng các vùng nhớ Heap (`malloc` mảng/struct/string) thông qua lệnh `call void @free(i8*)` khi biến thoát khỏi scope.

### 1.6. Báo Lỗi Trực Quan (Rust-Like Diagnostics)
* In thông báo lỗi với ANSI Color, trích đoạn code thực tế, chỉ số dòng/cột và con trỏ `^` đánh dấu vị trí lỗi.

---

## 2. Kiến Trúc Mã Nguồn (Codebase Architecture)

```text
vit/
├── CMakeLists.txt              # Cấu hình biên dịch CMake C++20
├── docs/                       # Tài liệu dự án
│   ├── history/
│   │   ├── work_log.md         # Phase 1 log
│   │   ├── phase2_work_log.md  # Phase 2 log
│   │   ├── phase3_work_log.md  # Phase 3 log
│   │   ├── phase4_work_log.md  # Phase 4 log
│   │   └── phase5_work_log.md  # Phase 5 log
│   └── features/
│       ├── current_features.md # [File này]
│       ├── phase2_features.md
│       ├── phase3_features.md
│       ├── phase4_features.md
│       └── phase5_features.md
├── std/                        # Thư viện chuẩn VIT
│   ├── math.vit
│   └── string.vit
├── include/                    # Header files (.h)
│   ├── ast/                    # Định nghĩa Cây cú pháp AST & Visitor
│   ├── diagnostics/            # Bộ báo lỗi Rust-like rich diagnostics
│   ├── lexer/                  # Bộ phân tích từ vựng (Lexer & Token)
│   ├── parser/                 # Bộ phân tích cú pháp (Parser)
│   ├── semantics/              # Bộ phân tích ngữ nghĩa (Semantic Analyzer)
│   └── codegen/                # Bộ sinh mã LLVM IR & Native Compiler
├── src/                        # Implementation files (.cpp)
│   ├── ast/
│   ├── diagnostics/
│   ├── lexer/
│   ├── parser/
│   ├── semantics/
│   ├── codegen/
│   └── main.cpp                # CLI entry point
└── test/                       # Bộ kiểm thử theo từng phase
    ├── Phase-1/
    ├── Phase-2/
    ├── Phase-3/
    ├── Phase-4/
    └── Phase-5/
```

---

## 3. Cách Biên Dịch & Chạy Dự Án

### Biên dịch Trình biên dịch `vit` từ mã nguồn C++:
```cmd
cmake -B build -S .
cmake --build build --config Debug
```

### Sử dụng Trình biên dịch `vit`:
```cmd
# 1. Chạy trực tiếp file code (Biên dịch + Chạy ngay)
vit run main.vit

# 2. Biên dịch ra file thực thi .exe với bộ tối ưu hóa -O2
vit build main.vit -O2 -o output.exe

# 3. In cây cú pháp AST & mã LLVM IR khi biên dịch
vit run main.vit --emit-ast --emit-llvm

# 4. Thêm vit vào PATH để dùng ở bất cứ đâu trên máy:
vit setup
```
