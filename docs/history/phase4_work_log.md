# Lịch Sử Phát Triển Dự Án Phase 4 (Phase 4 Work Log)

Tài liệu này ghi lại chi tiết toàn bộ công việc thực hiện trong **Phase 4** của dự án **VIT Compiler**: Tự động quản lý bộ nhớ Scope (ARC Memory Cleanup), Hệ thống Module nhiều file (`import`), Thư viện chuẩn (`std/math.vit`), Báo lỗi trực quan dạng Rust-like, và Bộ tối ưu hóa mã máy Native (`-O1`, `-O2`, `-O3`).

---

## Các Nhiệm Vụ Đã Hoàn Thành Trong Phase 4

### Task 1: Tự Động Quản Lý Bộ Nhớ Scope (ARC Memory Cleanup Pass)
* **Cơ chế**: Thêm quản lý Scope Heap Stack vào module `LLVMCodeGen`.
* **Xử lý**: Đăng ký các địa chỉ biến mảng Heap (`Array`) hoặc `struct` được khai báo trong Scope. Khi kết thúc khối lệnh `BlockASTNode`, tự động duyệt danh sách biến Heap trong Scope và chèn các lệnh LLVM IR `@free(i8* %ptr)` trước khi nhảy ra khỏi Scope.
* **Các file đã cập nhật**: `include/codegen/LLVMCodeGen.h`, `src/codegen/LLVMCodeGen.cpp`.

---

### Task 2: Hệ Thống Module (`import`) & Standard Library (`std/math.vit`)
* **Keywords**: `import`, `from` (`TokenType::KwImport`, `TokenType::KwFrom`).
* **Node AST & Parser**: Bổ sung `ImportASTNode` và phương thức `parseImportDecl()`.
* **Recursive Module Resolver**: Viết hàm `resolveImports` trong `src/main.cpp` tự động tìm kiếm file module chỉ định (`modPath`, `modPath.vit`, `./modPath`), nạp AST đệ quy, tránh nạp trùng lặp (circular/duplicate import protection) và gộp hàm/struct vào AST chính.
* **Standard Library**: Tạo file thư viện chuẩn `std/math.vit` chứa các khai báo `extern` cho các hàm toán học C runtime (`sqrt`, `cos`, `sin`, `pow`, `abs`...).
* **Các file đã cập nhật/tạo mới**: `include/ast/Statements.h`, `include/ast/ASTVisitor.h`, `src/ast/ASTPrinter.cpp`, `include/parser/Parser.h`, `src/parser/Parser.cpp`, `src/main.cpp`, `std/math.vit`.

---

### Task 3: Bộ In Báo Lỗi Trực Quan Dạng Rust-Like (Rich Diagnostic Printer)
* **Tính năng**: Bổ sung module `DiagnosticPrinter` định dạng và in thông báo lỗi với ANSI Color, trích dẫn số dòng, số cột, hiển thị dòng mã nguồn thực tế kèm con trỏ `^` màu đỏ/vàng chỉ chính xác vị trí lỗi.
* **Cập nhật Parser & Lexer**: Lưu giữ đầy đủ thông tin `line` và `column` cho tất cả các Token và Exception `ParseError`.
* **Các file đã tạo mới/cập nhật**: `include/diagnostics/DiagnosticPrinter.h`, `src/diagnostics/DiagnosticPrinter.cpp`, `src/main.cpp`, `CMakeLists.txt`.

---

### Task 4: Tích Hợp Cờ Tối Ưu Hóa Native (`-O1`, `-O2`, `-O3`)
* **Tính năng**: Thêm hỗ trợ các tham số cờ dòng lệnh `-O0`, `-O1`, `-O2`, `-O3` vào CLI parser trong `src/main.cpp`.
* **Native Compiler**: Truyền cờ optimization tương ứng sang câu lệnh gọi compiler Clang/LLVM.
* **Các file đã cập nhật**: `include/codegen/NativeCompiler.h`, `src/codegen/NativeCompiler.cpp`, `src/main.cpp`.

---

## Kết Quả Kiểm Thử (Verification Suite)

1. **Kiểm thử Module Import (`test/Phase-4/test_import.vit`)**:
   * Chạy thành công `vit run test/Phase-4/test_import.vit`.
   * Nạp `std/math.vit`, gọi `sqrt(16.0)` ra `4.000000` và `pow(2.0, 3.0)` ra `8.000000`.

2. **Kiểm thử Cờ Tối Ưu Hóa (`-O2`)**:
   * Biên dịch thành công `vit build test/Phase-4/test_import.vit -O2 -o math_test.exe`.
   * Chạy file `math_test.exe` sinh mã máy tối ưu thành công.

3. **Kiểm thử ARC Scope Cleanup (`test/Phase-4/test_arc.vit`)**:
   * Kiểm tra LLVM IR với `--emit-llvm`.
   * Xác nhận mã LLVM IR xuất hiện lệnh `call void @free(i8* %t19)` ở cuối mỗi vòng lặp `while`.

4. **Kiểm thử Rust-like Error Reporting (`test/Phase-4/test_error.vit`)**:
   * In báo lỗi định dạng đẹp mắt với trích đoạn code và con trỏ `^`.
