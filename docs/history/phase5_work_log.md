# Lịch Sử Phát Triển Dự Án Phase 5 (Phase 5 Work Log)

Tài liệu này ghi lại chi tiết toàn bộ công việc thực hiện trong **Phase 5** của dự án **VIT Compiler**: Phương thức cho Struct (`this`), Thao tác chuỗi (`+`, `==`, `!=`), Thuộc tính `.length` cho Chuỗi & Mảng, và Thư viện chuẩn `std/string.vit`.

---

## Các Nhiệm Vụ Đã Hoàn Thành Trong Phase 5

### Task 1: Struct Methods & `this` (Phương Thức Cho Struct)
* **AST & Parser**:
  * Thêm `MethodCallASTNode` và `ExpressionStmtASTNode` vào AST.
  * Cập nhật `StructDeclASTNode` lưu trữ danh sách các phương thức `methods`.
  * Cập nhật `Parser::parseStructDecl()` đọc từ khóa `function` bên trong `struct`.
  * Cập nhật `Parser::parsePostfix()` nhận diện gọi phương thức `obj.method(args)`.
* **Semantic Analyzer**:
  * Bổ sung `structMethodsTable` lưu trữ chữ ký các phương thức.
  * Tự động khai báo tham số ngầm `this` kiểu struct khi kiểm tra ngữ nghĩa thân phương thức.
* **LLVM CodeGen**:
  * Mã hóa tên hàm theo quy tắc mangling: `_StructName_methodName`.
  * Thêm tham số đầu tiên `%this` kiểu con trỏ `%struct.StructName*`.
  * Hỗ trợ gán và đọc trường struct qua `this.x = ...` và `this.x`.

---

### Task 2: Thao Tác Chuỗi Nâng Cao (`+`, `==`, `!=`, `str.length`)
* **Nối chuỗi (`+`)**:
  * Sinh mã gọi `strlen` tính độ dài hai chuỗi, `malloc` vùng nhớ mới, copy với `strcpy` và `strcat`.
  * Tự động đăng ký vùng nhớ chuỗi vừa nối vào ARC Scope Cleanup để giải phóng (`free`) khi hết scope.
* **So sánh chuỗi (`==`, `!=`)**:
  * Sinh lệnh gọi `strcmp(s1, s2) == 0` (hoặc `!= 0`) trả về giá trị kiểu `boolean`.
* **Chuỗi `.length`**:
  * Đọc thuộc tính `.length` của chuỗi bằng lệnh `strlen` và ép kiểu sang `double`.

---

### Task 3: Thuộc Tính Mảng (`arr.length`) & Sửa Lỗi Output Execution
* **Array Header Prefix**:
  * Khi khởi tạo mảng `ArrayLiteralASTNode`, cấp phát thêm 8 bytes phía trước (`(N+1)*8`), lưu độ dài `N` ở chỉ số 0 và trả về con trỏ ở chỉ số 1.
  * Thuộc tính `arr.length` đọc trực tiếp giá trị tại offset `-1` (`gep i64 -1`).
  * ARC Scope Cleanup giải phóng vùng nhớ tại `arr - 1`.
* **Sửa Lỗi Hàm `main()` Return Exit Code**:
  * Chuyển kiểu trả về của hàm `main()` tầng LLVM IR sang `i32` (`ret i32 0`) thay vì `double`, giúp file thực thi `.exe` trả về exit code 0 chuẩn hệ điều hành.

---

### Task 4: Thư Viện Chuẩn `std/string.vit` & Bộ Kiểm Thử
* **Thư viện chuẩn**: Tạo file `std/string.vit` chứa khai báo `extern function` cho C string interop.
* **Kiểm thử**:
  * `test/Phase-5/test_methods.vit`: Kiểm thử struct methods, `this`, scale, distance.
  * `test/Phase-5/test_string.vit`: Kiểm thử cộng chuỗi, so sánh chuỗi, `str.length`, `arr.length`.

---

## Kết Quả Kiểm Thử (Verification Suite)

1. **Biên dịch C++ Compiler**: Biên dịch thành công 100% bằng CMake & MSVC/Clang C++20.
2. **Chạy Trực Tiếp (`vit run`)**:
   * `vit run test/Phase-5/test_methods.vit`: Chạy thành công, in đúng kết quả `5.000000`, `6.000000`, `10.000000`.
   * `vit run test/Phase-5/test_string.vit`: Chạy thành công, in đúng `"Hello, World!"`, length `13`, `s1 == s2: TRUE`, array length `5`.
3. **Biên Dịch Native (`vit build -O2`)**:
   * Biên dịch `method_test.exe` và `string_test.exe` thành công với cờ tối ưu hóa `-O2`. Chạy thực thi độc lập kết quả chính xác 100%.
