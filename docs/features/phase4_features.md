# Chức Năng Mới Trong Phase 4 (Phase 4 Features & Specification)

Tài liệu này mô tả chi tiết các tính năng nâng cao vừa được tích hợp vào **VIT Compiler (v0.4.0 - Phase 4)**.

---

## 1. Tự Động Quản Lý Bộ Nhớ Scope (ARC Scope Memory Cleanup)

VIT hỗ trợ cơ chế giải phóng bộ nhớ Heap tự động theo Scope (Scope-based Automatic Reference Cleanup):
* Trình biên dịch theo dõi các đối tượng cấp phát trên Heap (`Array` `[...]` hoặc `struct`).
* Khi một biến Heap thoát khỏi Scope (kết thúc khối lệnh `Block`, vòng lặp `while`/`for`, hoặc kết thúc hàm), trình biên dịch tự động phát sinh lệnh `@free(i8* %ptr)` trên tầng LLVM IR.
* **Lợi ích**: Giúp VIT đạt tốc độ tối đa của C/Native mà không bị Memory Leak, đồng thời không bị overhead giật lag do Garbage Collector (như JS/Python) hay độ phức tạp của Borrow Checker (như Rust).

```javascript
function main(): number {
    let i: number = 0;
    while (i < 1000) {
        let arr = [10, 20, 30]; // Cấp phát Heap qua malloc
        print(arr[0]);
        i = i + 1;
        // Tự động phát sinh call void @free(i8* %arr) tại đây!
    }
    return 0;
}
```

---

## 2. Hệ Thống Module (`import`) & Standard Library (`std/math.vit`)

VIT hỗ trợ chia nhỏ dự án thành nhiều file mã nguồn và nạp module thông qua từ khóa `import` & `from`:
* **Cú pháp nạp hàm/symbol chỉ định**:
  ```javascript
  import { sqrt, cos, pow } from "std/math";
  ```
* **Cú pháp nạp toàn bộ module**:
  ```javascript
  import "std/math";
  ```
* **Thư viện chuẩn toán học (`std/math.vit`)**: Tích hợp sẵn các hàm `sqrt`, `cos`, `sin`, `tan`, `abs`, `pow`, `floor`, `ceil` bọc các hàm C Runtime chuẩn.

---

## 3. Báo Lỗi Chuyên Nghiệp Dạng Rust-Like (Rich Error Diagnostics)

Khi phát sinh lỗi cú pháp (Lexer / Parser Error) hoặc lỗi ngữ nghĩa (Semantic Error), trình biên dịch in ra giao diện trực quan rực rỡ sắc màu (ANSI Colors), trích dẫn vị trí file, số dòng, số cột và gạch chân con trỏ `^` chính xác ký tự bị lỗi:

```text
[Parse Error] Unexpected expression token '+'
  --> test\Phase-4\test_error.vit:3:13
     |
 3 |     let y = + * 5;
     |             ^
     |
```

---

## 4. Tối Ưu Hóa Biên Dịch Native (`-O1`, `-O2`, `-O3`)

Trình biên dịch dòng lệnh `vit` hỗ trợ truyền các cờ tối ưu hóa Native Compiler của hạ tầng LLVM / Clang:
* `vit build main.vit -O2 -o output.exe`: Kích hoạt bộ tối ưu hóa O2 (Dead code elimination, loop unrolling, constant folding, function inlining).
* `vit build main.vit -O3 -o output.exe`: Kích hoạt mức tối ưu hóa tối đa cho các tác vụ tính toán nặng.
