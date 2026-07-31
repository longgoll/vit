# Đặc Tả Kế Hoạch Phase 6: First-Class Functions, Lambdas & Array Methods (v0.6.0)

Tài liệu này là thiết kế chi tiết và kế hoạch triển khai cho **Phase 6** của trình biên dịch **VIT Compiler**. File này được biên soạn đầy đủ để AI Coding Assistant có thể đọc và tự động triển khai chính xác các tính năng.

---

## 1. Mục Tiêu Phase 6

Đưa VIT trở thành một ngôn ngữ hỗ trợ **Functional Programming (Lập trình chức năng)** cấp độ đầu tiên bằng cách cung cấp:
1. **First-Class Functions**: Hàm là giá trị loại một, có thể gán vào biến, truyền vào tham số và trả về từ hàm.
2. **Lambda / Arrow Functions**: Cú pháp hàm ẩn danh ngắn gọn `(x: number): number => x * 2`.
3. **Kiểu Hàm (Function Types)**: Cú pháp khai báo kiểu hàm `(a: number) => number`.
4. **Higher-Order Array Methods**: Thuộc tính mảng `.map()`, `.filter()`, `.forEach()`.
5. **Thư Viện Chuẩn `std/array.vit` & `std/sys.vit`**: Bổ sung FFI thao tác mảng động (`push`, `pop`), hệ thống (`clock`, `exit`).

---

## 2. Thiết Kế Cú Pháp & Ví Dụ Mã Nguồn

```javascript
import { clock } from "std/sys";

// 1. Khai báo Kiểu Hàm (Function Types)
type Mapper = (x: number) => number;

// 2. Hàm nhận tham số là một Hàm khác (Higher-Order Function)
function applyTwice(fn: Mapper, val: number): number {
    return fn(fn(val));
}

function main(): number {
    // 3. Lambda / Arrow Function Expression
    let double = (x: number): number => x * 2.0;

    let res = applyTwice(double, 5.0); // Output: 20.0

    // 4. Higher-Order Array Methods (.map, .filter, .forEach)
    let numbers = [1.0, 2.0, 3.0, 4.0, 5.0];
    
    let doubledNumbers = numbers.map((x: number): number => x * 2.0);
    // doubledNumbers -> [2.0, 4.0, 6.0, 8.0, 10.0]

    let evens = numbers.filter((x: number): boolean => x > 2.0);
    // evens -> [3.0, 4.0, 5.0]

    numbers.forEach((x: number): void => {
        print(x);
    });

    return 0;
}
```

---

## 3. Kiến Trúc Chi Tiết Cần Thay Đổi Trong Codebase

### 3.1. Từ Vựng & Cú Pháp (Lexer & Parser)
* **Token mới**: `TokenType::Arrow` (`=>`).
* **Parser (`parsePrimary` / `parseExpression`)**:
  * Khi gặp dấu mở ngoặc `(` trong ngữ cảnh biểu thức, kiểm tra xem có phải Lambda `(param: Type): RetType => body` hay không.
  * Sinh node `LambdaASTNode(params, returnType, bodyExprOrBlock)`.
* **Kiểu dữ liệu Hàm (`FunctionType`)**:
  * Chuỗi đại diện kiểu: `"(number, number) => number"`.

### 3.2. Kiểm Tra Ngữ Nghĩa (Semantic Analyzer)
* Đăng ký tham số hàm lambda vào bảng ký hiệu cục bộ `symbolTable`.
* Kiểm tra tính tương thích giữa kiểu hàm được truyền vào và kiểu tham số yêu cầu.
* Suy luận kiểu tự động cho các hàm nhận mảng `.map()` và `.filter()`.

### 3.3. Sinh Mã LLVM IR (LLVM CodeGen)
* **Function Pointers**:
  * Mỗi Lambda không bắt biến (non-capturing) được sinh dưới dạng một hàm ẩn danh LLVM IR `@__lambda_0(...)`.
  * Giá trị trả về của biểu thức Lambda là một Con trỏ Hàm (`i8*` hoặc `double(double)*`).
* **Gọi con trỏ hàm (`CallExprASTNode`)**:
  * Nếu mục tiêu gọi là một biến con trỏ hàm, sinh lệnh `call` gián tiếp thông qua con trỏ hàm LLVM: `call double %funcPtr(double %arg)`.
* **Array Methods (`.map`, `.filter`, `.forEach`)**:
  * `.map(fn)`: Cấp phát mảng mới trên Heap, lập qua từng phần tử, gọi `fn(elem)` và lưu kết quả vào mảng mới.
  * `.filter(fn)`: Đếm số phần tử thỏa mãn `fn(elem) == true`, cấp phát mảng mới và chèn các phần tử thỏa mãn.

---

## 4. Danh Sách File Cần Cập Nhật Phân Chia Theo Task

1. **Task 1: Lexer & Parser for Lambdas & Function Types**
   - `include/lexer/Token.h`: Thêm `TokenType::Arrow`.
   - `src/lexer/Lexer.cpp`: Nhận diện `=>`.
   - `include/ast/Expressions.h`: Thêm `LambdaASTNode`.
   - `src/parser/Parser.cpp`: Parse cú pháp `(x: type) => expr` và `function(x: type) { ... }`.

2. **Task 2: Semantic Checking & Function Pointer Types**
   - `src/semantics/SemanticAnalyzer.cpp`: Thêm kiểm tra kiểu cho `LambdaASTNode` và lời gọi con trỏ hàm gián tiếp.

3. **Task 3: LLVM IR CodeGen for Function Pointers & Indirect Calls**
   - `src/codegen/LLVMCodeGen.cpp`: Emitting LLVM function pointers and indirect `call` instructions.

4. **Task 4: Array Built-in Higher-Order Methods (`.map`, `.filter`, `.forEach`)**
   - `src/codegen/LLVMCodeGen.cpp`: Built-in code generation cho `.map`, `.filter`, `.forEach`.

5. **Task 5: Standard Library (`std/array.vit` & `std/sys.vit`) & Test Suite**
   - `std/array.vit`: C interop utility helpers.
   - `std/sys.vit`: `clock()`, `exit()`.
   - `test/Phase-6/test_lambda.vit`, `test/Phase-6/test_array_methods.vit`.
