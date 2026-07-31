# Lịch Sử Phát Triển Dự Án Phase 3 (Phase 3 Work Log)

Tài liệu này ghi lại chi tiết toàn bộ công việc thực hiện trong **Phase 3** của dự án **VIT Compiler**: Mở rộng ngôn ngữ hỗ trợ Cấu trúc dữ liệu người dùng (`struct`), Mảng (`Array`), Giao tiếp thư viện C (`FFI / C Interop`), Suy luận kiểu tự động (`Type Inference`), và Nền tảng quản lý bộ nhớ Heap tự động (ARC Foundation).

---

## Các Nhiệm Vụ Đã Hoàn Thành Trong Phase 3

### Task 1: Giao Tiếp Thư Viện C (`FFI / C Interop`)
* **Cú pháp**: `extern function functionName(param: type): returnType;`.
* **Lexer & Parser**: Bổ sung keyword `extern` (`TokenType::KwExtern`). Parser nhận diện các khai báo hàm không có body và kết thúc bằng dấu `;`.
* **Semantic Analyzer**: Đăng ký các hàm `extern` vào bảng ký hiệu hàm `functionTable` toàn cục để cho phép gọi từ bất kỳ phạm vi nào trong mã VIT.
* **LLVM CodeGen**: Sinh lệnh khai báo hàm ngoại vi `declare <retType> @<funcName>(<paramTypes>)` trong module LLVM IR.

---

### Task 2: Cấu Trúc Dữ Liệu Phức Hợp (`struct` & Member Access `.`)
* **Cú pháp**:
  ```javascript
  struct Point {
      x: number,
      y: number
  }

  let p: Point;
  p.x = 15;
  print(p.x);
  ```
* **Node AST**: `StructDeclASTNode`, `MemberAccessASTNode`, `MemberAssignmentASTNode`.
* **Semantic Analyzer**: Lưu trữ sơ đồ cấu trúc trường của Struct vào `structTable`, kiểm tra sự tồn tại của trường và kiểu trả về khi truy xuất `p.field`.
* **LLVM CodeGen**: Sinh kiểu cấu trúc LLVM `%struct.Point = type { double, double }`, tính toán chỉ số offset trường và dùng `getelementptr inbounds` để đọc/ghi giá trị.

---

### Task 3: Mảng Dữ Liệu (`Array`) & Truy Xuất Chỉ Số (`arr[i]`)
* **Cú pháp**:
  ```javascript
  let arr: number[] = [10, 20, 30];
  print(arr[0]);
  arr[1] = 50;
  ```
* **Node AST**: `ArrayLiteralASTNode`, `ArrayAccessASTNode`, `ArrayAssignmentASTNode`.
* **LLVM CodeGen**: Cấp phát bộ nhớ mảng Heap thông qua hàm `malloc`, chuyển đổi kiểu con trỏ `bitcast i8* to double*`, và truy xuất phần tử theo offset chỉ số động (`getelementptr inbounds double, double*`).

---

### Task 4: Suy Luận Kiểu Tự Động (`Type Inference`) & Khai Báo Biến Tùy Chọn Kiểu
* **Tính năng**: Cho phép khai báo biến bỏ qua phần chỉ định kiểu `: type` (như TypeScript). Trình biên dịch tự suy luận kiểu dữ liệu dựa trên vế biểu thức khởi tạo `initializer`.
* **Ví dụ**:
  ```javascript
  let arr = [10, 20, 30]; // Tự động suy luận kiểu number[]
  let sum = 0;           // Tự động suy luận kiểu number
  ```
* **Xử lý**: `SemanticAnalyzer` & `LLVMCodeGen` đánh giá kiểu biểu thức `initializer` trước để xác định kiểu `typeName` và sinh lệnh `alloca` chính xác.

---

## Kết Quả Kiểm Thử (Verification Suite)
1. **Kiểm thử FFI (`test/Phase-3/ffi.vit`)**: Gọi thành công hàm `sqrt(25.0)` từ C stdlib và in kết quả `5.000000`.
2. **Kiểm thử Struct (`test/Phase-3/struct.vit`)**: Khai báo struct `Point`, gán `p.x = 15`, `p.y = 30` và in ra chính xác.
3. **Kiểm thử Mảng & Suy Luận Kiểu (`test/Phase-3/array_infer.vit`)**: Tạo mảng suy luận `[10, 20, 30]`, lặp qua vòng `for` cộng dồn tổng bằng `60.000000`.
4. **Tính Tương Thích Ngược**: Tất cả các test Phase 1 & Phase 2 đều chạy mượt mà 100%.
