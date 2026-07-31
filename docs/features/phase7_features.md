# Đặc Tả Kế Hoạch Phase 7: Generics, Enums, Pattern Matching & System FFI (v0.7.0)

Tài liệu này là thiết kế chi tiết và kế hoạch triển khai cho **Phase 7** của trình biên dịch **VIT Compiler**. File này chuẩn bị sẵn toàn bộ kiến trúc để AI Coding Assistant có thể đọc và tự động triển khai.

---

## 1. Mục Tiêu Phase 7

Đưa VIT trở thành một ngôn ngữ hoàn chỉnh cho **System Programming & Dynamic Web Application Backend**:
1. **Generics / Parametric Polymorphism**: Hỗ trợ hàm và struct tổng quát `stack<T>`, `identity<T>(val: T)`.
2. **Enum & Tagged Unions**: Định nghĩa kiểu liệt kê và Enum mang dữ liệu `enum Result { Success(val: number), Error(msg: string) }`.
3. **Pattern Matching (`match` / `switch`)**: Cú pháp khớp mẫu kiểm tra loại dữ liệu an toàn.
4. **Hệ Thống Thư Viện File I/O (`std/fs.vit` & `std/io.vit`)**: Đọc/ghi file hệ thống, nhập/xuất bàn phím qua C FFI.

---

## 2. Thiết Kế Cú Pháp & Ví Dụ Mã Nguồn

```javascript
import { readFile, writeFile } from "std/fs";

// 1. Generic Struct Definition
struct Stack<T> {
    items: T[],
    count: number
}

// 2. Generic Function Definition
function identity<T>(item: T): T {
    return item;
}

// 3. Enum & Tagged Union
enum Option<T> {
    Some(val: T),
    None
}

function main(): number {
    // 4. Khởi tạo Generic Struct
    let s: Stack<number>;
    let name = identity<string>("VIT Compiler");

    // 5. Pattern Matching (match expression)
    let opt = Option.Some(42.0);
    
    match (opt) {
        Option.Some(val) => {
            print("Found value:");
            print(val);
        },
        Option.None => {
            print("No value found.");
        }
    }

    // 6. File I/O System Interop
    let content = readFile("data.txt");
    print(content);

    return 0;
}
```

---

## 3. Kiến Trúc Chi Tiết Cần Thay Đổi Trong Codebase

### 3.1. Monomorphization Pass (Generics Compiler Pass)
* Khi gặp khai báo Generic `function identity<T>(item: T): T` và lời gọi `identity<number>(10)`:
  * Trình biên dịch thực hiện **Monomorphization** (nhân bản ASTNode): Thay thế tham số kiểu `T` bằng kiểu thực tế (`number`), tạo ra một hàm concrete `identity_number` trong AST trước khi chuyển sang Semantic Check và LLVM CodeGen.
  * Giúp giữ nguyên hiệu năng Native của C/C++ mà không cần vtable/boxing.

### 3.2. Tagged Unions & Structural Enums Trong LLVM IR
* Mỗi Enum được biểu diễn dưới dạng struct LLVM: `%struct.Enum = type { i32, i8* }` (với `i32` là Tag định danh variant, và `i8*` là Payload chứa dữ liệu).
* Biểu thức `match` được hạ cấp (lower) xuống dạng chuỗi câu lệnh kiểm tra điều kiện rẽ nhánh `switch i32 %tag` hoặc `br label` trong LLVM IR.

### 3.3. Standard File System Library (`std/fs.vit`)
* Bổ sung FFI liên kết với thư viện C chuẩn `fopen`, `fread`, `fwrite`, `fclose`:
  ```javascript
  extern function fopen(filename: string, mode: string): i8*;
  extern function fclose(file: i8*): number;
  ```

---

## 4. Danh Sách File Cần Cập Nhật Phân Chia Theo Task

1. **Task 1: Generic Syntax Parsing & AST Nodes**
   - `include/ast/ASTNode.h` & `Statements.h`: Thêm tham số kiểu `<T>` cho `FunctionDeclASTNode` và `StructDeclASTNode`.
   - `src/parser/Parser.cpp`: Parse cú pháp góc `<T>`.

2. **Task 2: Monomorphization Pass**
   - Tạo mới `include/semantics/Monomorphizer.h` & `src/semantics/Monomorphizer.cpp`: Nhân bản và thế kiểu cụ thể cho AST.

3. **Task 3: Enums & Pattern Matching (`match`)**
   - `include/ast/Expressions.h` & `Statements.h`: Thêm `EnumDeclASTNode`, `MatchASTNode`.
   - `src/parser/Parser.cpp`: Parse `enum` và `match`.
   - `src/codegen/LLVMCodeGen.cpp`: Emitting Tagged Union layout & `switch` IR instructions.

4. **Task 4: File I/O & Console Input (`std/fs.vit`, `std/io.vit`)**
   - `std/fs.vit`: File interop (`readFile`, `writeFile`).
   - `std/io.vit`: Console input (`readLine`).
   - `test/Phase-7/test_generics.vit`, `test/Phase-7/test_enum_match.vit`.
