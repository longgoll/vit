# Đặc Tả Kế Hoạch Phase 8: Advanced Error Handling & Safety (v0.8.0)

Tài liệu này là thiết kế chi tiết và kế hoạch triển khai cho **Phase 8** của trình biên dịch **VIT Compiler**. File này chuẩn bị đầy đủ kiến trúc để AI Coding Assistant có thể đọc và tự động triển khai.

---

## 1. Mục Tiêu Phase 8

Nâng cao độ an toàn và khả năng quản lý lỗi của **VIT Compiler**:
1. **Result & Option Patterns & Try Operator (`?`)**: Quản lý lỗi không cần đến exception cồng kềnh, sử dụng cú pháp unwrapping ngắn gọn `let val = func()?;`.
2. **Strict Null / Undefined Safety**: Hỗ trợ Nullable Types (`T?`), Optional Chaining (`obj?.prop`), và Nullish Coalescing (`a ?? b`).
3. **Runtime Bounds & Overflow Checking**: Tự động chèn câu lệnh kiểm tra chỉ số mảng (Array bounds checking) để tránh segfault.
4. **Panic & Assertions**: Hàm hệ thống `panic(msg)` và `assert(condition, message)`.

---

## 2. Thiết Kế Cú Pháp & Ví Dụ Mã Nguồn

```javascript
import { readFile } from "std/fs";

struct User {
    id: number,
    name: string,
    email: string? // Nullable type
}

function parseUserData(raw: string): Result<User, string> {
    if (raw.length == 0) {
        return Result.Error("Empty raw data");
    }
    let u: User;
    u.id = 1.0;
    u.name = "Hoang Long";
    u.email = null;
    return Result.Ok(u);
}

function fetchUserEmail(path: string): string {
    // 1. Try operator (?) tự động unwrap Result.Ok hoặc return sớm nếu gặp Result.Error
    let raw = readFile(path)?; 
    let user = parseUserData(raw)?;

    // 2. Optional chaining (?. ) và Nullish Coalescing (??)
    let email = user.email ?? "no-email@domain.com";

    return email;
}

function main(): number {
    let numbers = [10.0, 20.0, 30.0];

    // 3. Array bounds checking (Runtime error nếu index >= numbers.length)
    let first = numbers[0]; // Ok
    // let invalid = numbers[5]; // Panics with: "Index out of bounds: index 5, length 3"

    let email = fetchUserEmail("user.json");
    print(email);

    return 0;
}
```

---

## 3. Kiến Trúc Chi Tiết Cần Thay Đổi Trong Codebase

### 3.1. Lexer & Parser
* **Token mới**:
  * `TokenType::Question` (`?`).
  * `TokenType::QuestionDot` (`?.`).
  * `TokenType::NullishCoalescing` (`??`).
  * `TokenType::KwNull` (`null`).
* **Try Operator (`expr?`)**:
  * Parse dưới dạng biểu thức postfix `TryExprASTNode(expr)`.
  * Đơn giản hóa về mặt ngữ nghĩa thành một câu lệnh `match (expr)`: Nếu là `Result.Error(err)` hoặc `Option.None`, hàm hiện tại sẽ `return Result.Error(err)` hoặc `Option.None` ngay lập tức.

### 3.2. Type Checker & Semantic Analyzer
* **Nullable Types (`T?`)**:
  * Biểu diễn kiểu dưới dạng `NullableType(BaseType)`. Biến kiểu `T?` có thể nhận giá trị `null`.
  * Yêu cầu unwrapping (thông qua `if (x != null)` hoặc `?.` / `??`) trước khi truy cập thuộc tính của `T`.

### 3.3. LLVM CodeGen for Bounds Check & Panic
* **Array Bounds Check**:
  * Khi phát sinh mã cho `ArrayAccessASTNode`, chèn 1 câu lệnh so sánh `icmp uge %index, %array_length`.
  * Nếu true, chuyển hướng sang khối `@__vit_panic("Index out of bounds")` dừng chương trình an toàn và in thông tin dòng code.

---

## 4. Danh Sách File Cần Cập Nhật Phân Chia Theo Task

1. **Task 1: Lexer & Parser for `?`, `?.`, `??`, `null`**
   - `include/lexer/Token.h`: Thêm các TokenType mới.
   - `src/lexer/Lexer.cpp`: Nhận diện `?`, `?.`, `??`.
   - `include/ast/Expressions.h`: Thêm `TryExprASTNode`, `OptionalChainASTNode`, `NullCoalesceASTNode`.
   - `src/parser/Parser.cpp`: Parse cú pháp try, optional chain và nullish coalescing.

2. **Task 2: Semantic Analyzer & Type Checking**
   - `src/semantics/SemanticAnalyzer.cpp`: Thêm Type Checking cho Nullable Types và Try Operator lowering.

3. **Task 3: LLVM IR CodeGen for Bounds Check & Panic Handler**
   - `src/codegen/LLVMCodeGen.cpp`: Sinh lệnh so sánh chỉ số mảng trước khi load/store.

4. **Task 4: Standard Library & Test Suite**
   - `test/Phase-8/test_try_operator.vit`
   - `test/Phase-8/test_null_safety.vit`
   - `test/Phase-8/test_bounds_check.vit`
