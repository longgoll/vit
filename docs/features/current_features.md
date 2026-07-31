# Tổng Quan Chức Năng Đang Có (Current Features & Architecture)

Tài liệu này dành cho lập trình viên mới bắt tay vào codebase của dự án **VIT Compiler**. Tài liệu mô tả các tính năng ngôn ngữ đã hỗ trợ, kiến trúc codebase, cấu trúc thư mục, và hướng dẫn tiếp tục phát triển.

---

## 1. Các Chức Năng Ngôn Ngữ Hiện Có (Supported Features)

### 1.1. Kiểu dữ liệu & Biến
* **Primitive Type**: `number` (biểu diễn dưới dạng 64-bit float IEEE 754 `double` trong LLVM IR).
* **Khai báo biến**:
  * `let x = 10;`
  * `const y = 20;`
  * Khai báo có chỉ định kiểu: `let result: number = 0;`
* **Phép gán**: `x = y + 5;`

### 1.2. Biểu thức & Toán tử
* **Toán tử số học**: `+` (cộng), `-` (trừ), `*` (nhân), `/` (chia).
* **Toán tử so sánh**: `==`, `!=`, `<`, `>`, `<=`, `>=`.
* **Độ ưu tiên toán tử**: Đã hỗ trợ ưu tiên toán tử chuẩn (nhân chia trước, cộng trừ sau, nhóm ngoặc `(...)`).

### 1.3. Câu lệnh điều khiển (Control Flow)
* **Khối lệnh (Block)**: `{ stmt1; stmt2; }`
* **Câu điều kiện `if/else`**:
  ```javascript
  if (total > 100) {
      print(total);
  } else {
      print(0);
  }
  ```

### 1.4. Hàm (Functions)
* **Định nghĩa hàm**: `function name(param1: type, param2: type): returnType { ... }`
* **Gọi hàm**: `let res = add(x, y);`
* **Trả về giá trị**: `return expr;` hoặc `return;`

### 1.5. I/O Tích hợp
* **Hàm in giá trị**: `print(expr);` (tự động link tới `printf` của C Runtime).

---

## 2. Kiến Trúc Mã Nguồn (Codebase Architecture)

```text
vit/
├── CMakeLists.txt              # Cấu hình biên dịch CMake C++20
├── design/                     # File thiết kế đặc tả ban đầu
│   ├── main.md
│   └── cay.md
├── docs/                       # Tài liệu dự án
│   ├── history/
│   │   └── work_log.md         # Lịch sử các việc đã làm
│   └── features/
│       └── current_features.md # [File này] Chi tiết chức năng hiện có
├── include/                    # Header files (.h)
│   ├── ast/                    # Định nghĩa Cây cú pháp AST & Visitor
│   │   ├── AST.h
│   │   ├── ASTNode.h
│   │   ├── ASTPrinter.h
│   │   ├── ASTVisitor.h
│   │   ├── Expressions.h
│   │   ├── Functions.h
│   │   └── Statements.h
│   ├── lexer/                  # Bộ phân tích từ vựng (Lexer & Token)
│   │   ├── Lexer.h
│   │   └── Token.h
│   ├── parser/                 # Bộ phân tích cú pháp (Parser)
│   │   └── Parser.h
│   └── codegen/                # Bộ phát sinh mã LLVM IR & Compiler Native
│       ├── LLVMCodeGen.h
│       └── NativeCompiler.h
├── src/                        # Implementation files (.cpp)
│   ├── ast/
│   │   └── ASTPrinter.cpp
│   ├── lexer/
│   │   └── Lexer.cpp
│   ├── parser/
│   │   └── Parser.cpp
│   ├── codegen/
│   │   ├── LLVMCodeGen.cpp
│   │   └── NativeCompiler.cpp
│   └── main.cpp                # CLI entry point
└── examples/                   # File mã nguồn mẫu kiểm thử (.jslik)
    └── sample.jslik
```

---

## 3. Cách Biên Dịch & Chạy Dự Án

### Biên dịch Trình biên dịch `vit` từ mã nguồn C++:
```cmd
cmake -B build -S .
cmake --build build --config Debug
```

### Sử dụng Trình biên dịch `vit` (Cú pháp chuẩn Golang/Rust):
```cmd
# 1. Chạy trực tiếp file code (Biên dịch + Chạy ngay)
vit run main.vit

# 2. Biên dịch ra file thực thi .exe
vit build main.vit -o output.exe

# 3. Cú pháp viết tắt (tự động run)
vit main.vit

# 4. In cây cú pháp AST & mã LLVM IR khi biên dịch
vit run main.vit --emit-ast --emit-llvm

# 5. Thêm vit vào PATH để dùng ở bất cứ đâu trên máy:
.\scripts\setup_path.ps1

# 6. Tự động đóng gói bộ Toolchain Clang Portable đi kèm (Giải pháp Zero Dependency):
.\scripts\bundle_tools.ps1
```

---

## 4. Lộ Trình Phát Triển Tiếp Theo (Roadmap cho Developer tương lai)

Nếu bạn là lập trình viên tiếp theo phát triển dự án này, dưới đây là các tính năng được đề xuất cho **Phase 2**:

1. **Kiểu dữ liệu mở rộng**:
   * Thêm kiểu `string` (chuỗi ký tự).
   * Thêm kiểu `boolean` (`true`/`false`).
   * Thêm kiểu `array` / `vector`.
2. **Cấu trúc điều khiển nâng cao**:
   * Vòng lặp `while (cond) { ... }`
   * Vòng lặp `for (let i = 0; i < n; i = i + 1) { ... }`
3. **Quản lý bộ nhớ tự động**:
   * Triển khai bộ quản lý bộ nhớ ARC (Automatic Reference Counting - giống Swift) để tự động giải phóng vùng nhớ heap khi biến thoát khỏi scope.
4. **Phân tích ngữ nghĩa nâng cao (Semantic Analyzer Pass)**:
   * Kiểm tra ép kiểu ngầm định / cảnh báo mismatched types trước khi qua giai đoạn CodeGen.
