# Lịch Sử Phát Triển Dự Án (Work Log)

Tài liệu này ghi lại chi tiết toàn bộ các bước đã thực hiện trong quá trình xây dựng Trình biên dịch **VIT Compiler (Phase 1 MVP)** từ giai đoạn khởi tạo đến hoàn thiện.

---

## Giai đoạn: Phase 1 MVP (Hoàn thành)

### Task 1: Thiết kế cấu trúc Cây cú pháp trừu tượng (AST Data Structures)
* **Mục tiêu**: Thiết kế các lớp dữ liệu đại diện cho tất cả các node trong cây AST của ngôn ngữ (hỗ trợ kiểu `number`, biến `let`/`const`, hàm, toán tử số học/so sánh, câu lệnh `if/else`, `return`, `print`).
* **Giải pháp**:
  * Sử dụng C++20 smart pointers (`std::unique_ptr`) để quản lý bộ nhớ tự động và an toàn.
  * Áp dụng **Visitor Pattern** (`ASTVisitor`) để tách biệt cấu trúc dữ liệu AST với các thao tác xử lý sau này (Semantic Check, CodeGen, Print AST).
* **Các file đã tạo**:
  * `include/ast/ASTNode.h`: Lớp cơ sở abstract `ASTNode`, `ExpressionNode`, `StatementNode`.
  * `include/ast/ASTVisitor.h`: Lớp giao diện Visitor.
  * `include/ast/Expressions.h`: Node biểu thức (`NumberLiteral`, `VariableExpr`, `BinaryOp`, `CallExpr`).
  * `include/ast/Statements.h`: Node câu lệnh (`VarDecl`, `Assignment`, `Block`, `If`, `Return`, `Print`).
  * `include/ast/Functions.h`: Node định nghĩa hàm (`FunctionDecl`) và node gốc của chương trình (`Program`).
  * `include/ast/AST.h`: Header tổng hợp.
  * `include/ast/ASTPrinter.h` & `src/ast/ASTPrinter.cpp`: Lớp Visitor in cây AST dạng phân cấp để kiểm thử.

---

### Task 2: Bộ Phân tích Từ vựng & Cú pháp (Hand-written Lexer & Recursive Descent Parser)
* **Mục tiêu**: Đọc chuỗi mã nguồn dạng văn bản `.jslik`, phân tích thành danh sách Token và dựng thành cây `ProgramASTNode`.
* **Giải pháp**:
  * **Lexer**: Quét mã nguồn từng ký tự, tự động bỏ qua khoảng trắng, comment đơn dòng (`//`) và đa dòng (`/* */`), sinh Token kèm dòng/cột để báo lỗi.
  * **Parser**: Phân tích cú pháp theo phương pháp đi xuống đệ quy (Recursive Descent) kết hợp với thuật toán **Precedence Climbing** để xử lý độ ưu tiên toán tử số học (`* /` trước `+ -`) và so sánh (`== != < > <= >=`).
* **Các file đã tạo**:
  * `include/lexer/Token.h`: Enum `TokenType` và struct `Token`.
  * `include/lexer/Lexer.h` & `src/lexer/Lexer.cpp`: Bộ quét từ vựng.
  * `include/parser/Parser.h` & `src/parser/Parser.cpp`: Bộ phân tích cú pháp đệ quy.

---

### Task 3: Module Sinh mã trung gian LLVM IR (LLVM IR CodeGenerator)
* **Mục tiêu**: Chuyển đổi cây AST thu được từ Task 2 thành mã trung gian LLVM IR (`.ll`) chuẩn quốc tế.
* **Giải pháp**:
  * Tạo lớp `LLVMCodeGen` kế thừa từ `ASTVisitor`.
  * Quản lý biến local và tham số hàm thông qua stack allocations (`alloca double`, `store`, `load`).
  * Sinh các lệnh toán tử số học (`fadd`, `fsub`, `fmul`, `fdiv`) và so sánh (`fcmp`).
  * Xử lý luồng rẽ nhánh điều kiện `if/else` bằng Basic Blocks (`label %then`, `%else`, `%merge`) và lệnh nhảy `br i1`.
  * Khai báo và link với hàm C Runtime `@printf` để in giá trị ra màn hình.
* **Các file đã tạo**:
  * `include/codegen/LLVMCodeGen.h` & `src/codegen/LLVMCodeGen.cpp`: Module phát sinh mã LLVM IR.

---

### Task 4: Trình điều khiển Compiler CLI & Biên dịch ra File thực thi Native (.exe)
* **Mục tiêu**: Đóng gói compiler thành một công cụ dòng lệnh (CLI) hoàn chỉnh, có khả năng tự động gọi `clang` để biên dịch file `.ll` ra file binary `.exe`.
* **Giải pháp**:
  * Tạo lớp `NativeCompiler` tự động phát hiện `clang` trong hệ thống và thực thi lệnh biên dịch mã máy.
  * Nâng cấp `main.cpp` nhận các cờ dòng lệnh: `-o`, `--emit-ast`, `--emit-llvm`, `-run`.
* **Các file đã tạo/chỉnh sửa**:
  * `include/codegen/NativeCompiler.h` & `src/codegen/NativeCompiler.cpp`: Module đóng gói và gọi Clang.
  * `src/main.cpp`: Điểm vào chính của Trình biên dịch CLI.
  * `CMakeLists.txt`: Cấu hình biên dịch CMake C++20.
  * `examples/sample.jslik`: File mã nguồn mẫu kiểm thử.

---

### Task 5: Nâng cấp Giao diện CLI chuẩn Golang & Tiện ích PATH (`vit run`, `vit build`)
* **Mục tiêu**: Tối ưu Trải nghiệm Lập trình viên (DX), biến lệnh biên dịch dài dòng thành lệnh ngắn gọn hệt như Golang/Rust (`vit run`, `vit build`, `vit <file>`), đồng thời hỗ trợ cài đặt PATH để chia sẻ cho bạn bè dễ dàng.
* **Giải pháp**:
  * Đổi tên binary từ `vit_compiler` thành `vit` trong `CMakeLists.txt`.
  * Viết lại `src/main.cpp` hỗ trợ các subcommand `run`, `build`, `version`, `help` và mặc định chạy ngay khi truyền file path.
  * Tạo script `scripts/setup_path.ps1` hỗ trợ tự động thêm `vit` vào `PATH` trên Windows.
* **Các file đã chỉnh sửa/tạo mới**:
  * `CMakeLists.txt`: Cập nhật target name `vit`.
  * `src/main.cpp`: Tái cấu trúc parser lệnh dòng lệnh.
  * `scripts/setup_path.ps1`: Script PowerShell thêm `vit` vào User PATH.
  * `docs/features/current_features.md`: Cập nhật hướng dẫn CLI.

---

## Giai đoạn: Phase 13 Developer Experience & Ecosystem (v1.3.0 Milestone - Hoàn thành)

### Task: Hệ sinh thái công cụ hỗ trợ Lập trình viên (DX Tooling)
* **Mục tiêu**: Xây dựng Language Server Protocol (`vit-lsp`), Package Manager (`vit pm`), Interactive REPL (`vit repl`), Formatter & Linter (`vit fmt`, `vit lint`).
* **Các file đã tạo/chỉnh sửa**:
  * `include/tools/LSP.h` & `src/tools/LSP.cpp`: Server JSON-RPC 2.0 cho Autocomplete, Hover, Go-to-definition, Diagnostics.
  * `src/lsp_main.cpp`: Binary độc lập `vit-lsp.exe`.
  * `include/tools/PackageManager.h` & `src/tools/PackageManager.cpp`: Lệnh `vit init`, `vit add`, `vit install` quản lý `vit.json`.
  * `include/tools/REPL.h` & `src/tools/REPL.cpp`: Shell REPL tương tác `vit repl`.
  * `include/tools/Formatter.h` & `src/tools/Formatter.cpp`: Định dạng mã nguồn `vit fmt`.
  * `include/tools/Linter.h` & `src/tools/Linter.cpp`: Kiểm tra mùi mã và quy tắc đặt tên `vit lint`.
  * `src/main.cpp`: Điều hướng subcommand CLI cho Phase 13.
  * `CMakeLists.txt`: Cập nhật build targets `vit` và `vit-lsp`.
  * `test/Phase13/run_phase13_tests.bat`: Bộ test tự động kiểm thử toàn bộ tính năng Phase 13.

---

## Tổng kết Kiểm thử (Verification Log)
1. **Biên dịch dự án C++**: Dự án biên dịch thành công 100% bằng CMake & Ninja C++20 sinh ra `vit.exe` và `vit-lsp.exe`.
2. **Chạy thử chương trình CLI mới**:
   * Kiểm thử `vit init`, `vit add`, `vit install` quản lý gói mượt mà.
   * Kiểm thử `vit fmt` căn chỉnh lề và định dạng code chuẩn xác.
   * Kiểm thử `vit lint` bắt lỗi đặt tên và unreachable code thành công.
   * Kiểm thử `vit-lsp` giao tiếp JSON-RPC 2.0 và trả về kết quả diagnostic.

