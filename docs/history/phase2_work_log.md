# Lịch Sử Phát Triển Dự Án Phase 2 (Phase 2 Work Log)

Tài liệu này ghi lại chi tiết toàn bộ công việc thực hiện trong **Phase 2** của dự án **VIT Compiler**: Nâng cấp cấu trúc điều khiển vòng lặp, mở rộng hệ thống kiểu dữ liệu, xây dựng bộ phân tích ngữ nghĩa (`SemanticAnalyzer Pass`), và hỗ trợ in ấn đa dạng kiểu dữ liệu.

---

## Các Nhiệm Vụ Đã Hoàn Thành Trong Phase 2

### Task 1: Bộ Phân Tích Từ Vựng & Cú Pháp Cho Phase 2 (Lexer & Parser Updates)
* **Từ khóa mới**: Bổ sung `while`, `for`, `break`, `continue`, `true`, `false`, `boolean`, `string`, `void`.
* **Toán tử logic & Chuỗi**: Thêm token `&&` (AndAnd), `||` (PipePipe), `!` (Exclamation), và chuỗi ký tự dạng `"..."` (`StringLiteral`).
* **Cấu trúc ngữ pháp đệ quy**:
  * Hàm `parseWhile()` và `parseFor()` xử lý các cấu trúc vòng lặp.
  * Hàm `parseBreak()` và `parseContinue()` xử lý câu lệnh chuyển hướng luồng.
  * Cập nhật thứ tự ưu tiên biểu thức (Precedence Climbing): `LogicalOr` -> `LogicalAnd` -> `Equality` -> `Relational` -> `Additive` -> `Multiplicative` -> `Unary` -> `Primary`.
* **File cập nhật**:
  * `include/lexer/Token.h` & `src/lexer/Lexer.cpp`
  * `include/parser/Parser.h` & `src/parser/Parser.cpp`

---

### Task 2: Mở Rộng Cây Cú Pháp Trừu Tượng (AST & Visitor Updates)
* **Node mới**:
  * Node câu lệnh: `WhileASTNode`, `ForASTNode`, `BreakASTNode`, `ContinueASTNode`.
  * Node biểu thức: `BooleanLiteralASTNode`, `StringLiteralASTNode`, `UnaryOpASTNode`.
* **Cập nhật Visitor & Printer**: Cập nhật `ASTVisitor` interface và `ASTPrinter` để hỗ trợ hiển thị đầy đủ cây AST mở rộng.
* **File cập nhật**:
  * `include/ast/ASTNode.h`
  * `include/ast/ASTVisitor.h`
  * `include/ast/Expressions.h`
  * `include/ast/Statements.h`
  * `include/ast/ASTPrinter.h` & `src/ast/ASTPrinter.cpp`

---

### Task 3: Bộ Phân Tích Ngữ Nghĩa (Semantic Analyzer Pass)
* **Mục tiêu**: Kiểm tra tính hợp lệ của chương trình trước khi sinh mã IR, ngăn ngừa các lỗi hổng bộ nhớ hoặc lỗi thời gian chạy.
* **Quy tắc kiểm tra**:
  * Báo lỗi sử dụng biến chưa khai báo (`Undeclared Variable`).
  * Báo lỗi khai báo trùng biến trong cùng scope (`Duplicate Declaration`).
  * Báo lỗi gán lại giá trị cho biến `const` (`Const Re-assignment`).
  * Báo lỗi câu lệnh `break` / `continue` nằm ngoài vòng lặp.
* **File tạo mới**:
  * `include/semantics/SemanticAnalyzer.h`
  * `src/semantics/SemanticAnalyzer.cpp`

---

### Task 4: Sinh Mã LLVM IR Cho Phase 2 (LLVM IR CodeGenerator)
* **Xử lý Vòng lặp**:
  * Khởi tạo các Basic Blocks (`while.cond`, `while.body`, `while.end`, `for.cond`, `for.body`, `for.step`, `for.end`).
  * Quản lý stack `loopStack` để ánh xạ chính xác đích đến của `break` và `continue`.
* **Xử lý Short-Circuit Logic & Kiểu dữ liệu**:
  * Toán tử logic `&&` và `||` sinh mã rẽ nhánh short-circuit chuẩn xác với câu lệnh `phi i1`.
  * Chuỗi ký tự sinh mã hằng số toàn cục `@.str.N` và truy xuất qua `getelementptr`.
  * Hàm `print` tự động kiểm tra kiểu dữ liệu (`number` -> `%f`, `string` -> `%s`, `boolean` -> `"true"` / `"false"`).
* **File cập nhật**:
  * `include/codegen/LLVMCodeGen.h`
  * `src/codegen/LLVMCodeGen.cpp`
  * `src/main.cpp`
  * `CMakeLists.txt`

---

## Kết Quả Kiểm Thử (Verification)
1. **Biên dịch**: Dự án biên dịch thành công 100% bằng CMake & Clang/MSVC target sinh ra `vit.exe`.
2. **Kiểm thử Vòng lặp (`test/Phase-2/loop.vit`)**: Chạy thành công các vòng lặp `while`, `for`, `break`, và `continue`.
3. **Kiểm thử Kiểu dữ liệu (`test/Phase-2/types.vit`)**: Chạy thành công các hàm xử lý `boolean`, `string`, `!`, `&&`, `||`, và hàm `print` đa kiểu.
4. **Kiểm thử Ngữ nghĩa (`test/Phase-2/semantic_err.vit`)**: Bộ `SemanticAnalyzer` phát hiện đúng 3/3 lỗi ngữ nghĩa cố ý.
5. **Tính Tương Thích Ngược (`test/Phase-1/main.vit`)**: Các chương trình Phase 1 tiếp tục chạy chính xác 100%.
