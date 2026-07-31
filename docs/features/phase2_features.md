# Chức Năng Mới Trong Phase 2 (Phase 2 Features & Specification)

Tài liệu này mô tả chi tiết các tính năng ngôn ngữ và kiến trúc vừa được bổ sung vào **VIT Compiler (Phase 2)**.

---

## 1. Các Cấu Trúc Điều Khiển Nâng Cao (Control Flow)

### 1.1. Vòng Lặp `while`
Cho phép thực thi khối lệnh đệ quy khi điều kiện còn đúng.
```javascript
let count: number = 0;
while (count < 5) {
    print(count);
    count = count + 1;
}
```

### 1.2. Vòng Lặp `for`
Hỗ trợ cú pháp 3 thành phần standard: `for (init; condition; update) { ... }`.
```javascript
for (let i = 0; i < 10; i = i + 1) {
    if (i == 3) continue; // Nhảy tới bước update
    if (i == 7) break;    // Thoát khỏi vòng lặp
    print(i);
}
```

### 1.3. Lệnh Chuyển Hướng `break` & `continue`
* `break;`: Lập tức thoát khỏi vòng lặp gần nhất.
* `continue;`: Bỏ qua các lệnh còn lại trong block và nhảy tới bước lặp tiếp theo.

---

## 2. Kiểu Dữ Liệu & Toán Tử Mới

### 2.1. Kiểu `boolean`
* Biểu diễn giá trị logic `true` hoặc `false`.
* Được ánh xạ trực tiếp thành `i1` trong mã LLVM IR.
```javascript
let isReady: boolean = true;
let isFinished: boolean = false;
```

### 2.2. Kiểu `string`
* Biểu diễn chuỗi ký tự được bọc trong dấu ngoặc kép `"..."`.
* Tự động sinh ra các hằng số chuỗi toàn cục (Global string constants) trong LLVM IR.
```javascript
let msg: string = "Hello VIT Compiler!";
print(msg);
```

### 2.3. Kiểu `void`
* Sử dụng làm kiểu trả về cho các hàm không trả về giá trị.
```javascript
function logMessage(msg: string): void {
    print(msg);
}
```

### 2.4. Toán Tử Logic (`&&`, `||`, `!`)
* **Logic NOT (`!`)**: Đảo ngược giá trị boolean.
* **Logic AND (`&&`)**: Đánh giá short-circuit, trả về `true` khi cả 2 vế đều đúng.
* **Logic OR (`||`)**: Đánh giá short-circuit, trả về `true` khi ít nhất 1 vế đúng.
```javascript
let isValid: boolean = isReady && !isFinished;
```

---

## 3. Hàm `print` Tự Động Nhận Diện Kiểu (Polymorphic Print)
Hàm `print(expr)` trong Phase 2 tự động phát hiện kiểu của biểu thức và in ra định dạng phù hợp:
* **Kiểu `number`**: In ra dạng số thực decimal (ví dụ: `50.000000`).
* **Kiểu `string`**: In ra chuỗi ký tự nguyên bản.
* **Kiểu `boolean`**: In ra chuỗi `"true"` hoặc `"false"`.

---

## 4. Bộ Phân Tích Ngữ Nghĩa (Semantic Analyzer Pass)
Trình biên dịch tự động chạy pass kiểm tra ngữ nghĩa trước khi sinh mã máy. Nếu phát hiện các lỗi sau, chương trình sẽ ngắt biên dịch và hiển thị thông báo lỗi chi tiết:
1. `Undeclared variable`: Dùng biến chưa khai báo.
2. `Duplicate declaration`: Khai báo 2 biến trùng tên trong cùng scope.
3. `Const reassignment`: Thay đổi giá trị biến `const`.
4. `Invalid loop statement`: Gọi `break` hoặc `continue` bên ngoài vòng lặp.
