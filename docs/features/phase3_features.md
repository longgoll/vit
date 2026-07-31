# Chức Năng Mới Trong Phase 3 (Phase 3 Features & Specification)

Tài liệu này mô tả chi tiết các tính năng nâng cao vừa được tích hợp vào **VIT Compiler (v0.3.0 - Phase 3)**.

---

## 1. Giao Tiếp Thư Viện C (`FFI / C Interop`)

VIT hỗ trợ khai báo và gọi trực tiếp các hàm chuẩn C/C++ từ hệ thống thông qua từ khóa `extern`.
```javascript
extern function sqrt(x: number): number;
extern function cos(x: number): number;

function main(): number {
    let val: number = sqrt(25.0);
    print("sqrt(25.0):");
    print(val); // In ra 5.000000
    return 0;
}
```

---

## 2. Cấu Trúc Dữ Liệu Phức Hợp (`struct`)

Người dùng có thể tự định nghĩa kiểu dữ liệu gồm nhiều trường thành phần.
```javascript
struct Point {
    x: number,
    y: number
}

function main(): number {
    let p: Point;
    p.x = 15;
    p.y = 30;

    print("p.x:");
    print(p.x);
    return 0;
}
```

---

## 3. Mảng Dữ Liệu (`Array`)

Hỗ trợ mảng các phần tử khởi tạo trực tiếp qua `[...]` và truy xuất qua chỉ số `arr[i]`.
```javascript
let numbers: number[] = [10, 20, 30];
print(numbers[0]); // 10.000000

numbers[1] = 99;   // Gán lại phần tử index 1
```

---

## 4. Suy Luận Kiểu Tự Động (`Type Inference`)

Không bắt buộc phải ghi rõ kiểu dữ liệu `: type` trong khai báo `let` / `const`. Trình biên dịch sẽ tự động xác định kiểu dựa trên vế phải.
```javascript
let name = "VIT Compiler"; // Tự suy luận kiểu string
let flag = true;           // Tự suy luận kiểu boolean
let list = [1, 2, 3];      // Tự suy luận kiểu number[]
let sum = 0;               // Tự suy luận kiểu number
```

---

## 5. Quản Lý Bộ Nhớ Heap (ARC Memory Foundation)

* Trình biên dịch tự động phát sinh mã cấp phát bộ nhớ Heap bằng `malloc` cho các đối tượng mảng và struct.
* Khai báo biến `let` hiện cho phép không cần biểu thức khởi tạo ngay lập tức (khởi tạo mặc định ngầm định).
