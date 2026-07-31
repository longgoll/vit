# Chức Năng Mới Trong Phase 5 (Phase 5 Features & Specification)

Tài liệu này mô tả chi tiết các tính năng nâng cao vừa được tích hợp vào **VIT Compiler (v0.5.0 - Phase 5)**.

---

## 1. Struct Methods & Con Trỏ Biến Ẩn `this`

VIT hỗ trợ định nghĩa các hàm/phương thức trực tiếp bên trong `struct`:
* **Cú pháp định nghĩa phương thức**:
  ```javascript
  struct Point {
      x: number,
      y: number,
      function distance(): number {
          return sqrt(this.x * this.x + this.y * this.y);
      },
      function scale(factor: number): void {
          this.x = this.x * factor;
          this.y = this.y * factor;
      }
  }
  ```
* **Con trỏ `this`**: Khi thực thi phương thức, trình biên dịch tự động đăng ký tham số ngầm `this` (pointer trỏ tới struct hiện tại).
* **Cú pháp gọi phương thức**: `let d = p.distance();` hoặc `p.scale(2.0);`.
* **LLVM IR Mangling**: Mã hóa tên hàm dưới dạng `_StructName_methodName(%struct.StructName* %this, ...)`.

---

## 2. Thao Tác Chuỗi Nâng Cao (`+`, `==`, `!=`, `str.length`)

Ngôn ngữ VIT nâng cấp kiểu dữ liệu `string` với các toán tử chuẩn:
* **Cộng chuỗi (`+`)**: Tự động cấp phát bộ nhớ Heap (`malloc`), copy nội dung bằng `strcpy` + `strcat` và tự động đăng ký vào bộ quản lý **ARC Scope Cleanup** để giải phóng bộ nhớ (`free`) khi ra khỏi Scope.
  ```javascript
  let greeting = "Hello, " + "World!"; // "Hello, World!"
  ```
* **So sánh chuỗi (`==`, `!=`)**: Tự động gọi hàm C runtime `strcmp(s1, s2)` để so sánh nội dung văn bản thực sựThay vì so sánh con trỏ.
* **Độ dài chuỗi (`str.length`)**: Trả về độ dài chuỗi dưới dạng kiểu `number`.

---

## 3. Thuộc Tính Mảng Built-in (`arr.length`)

* VIT hỗ trợ đọc số lượng phần tử của mảng dynamic/heap qua thuộc tính `.length`:
  ```javascript
  let numbers = [10, 20, 30, 40, 50];
  print(numbers.length); // In ra 5.000000
  ```
* **Cơ chế**: Mỗi mảng Heap được cấp phát kèm Header Prefix ở offset `-1` (8 bytes phía trước con trỏ dữ liệu) để lưu độ dài mảng `count`.

---

## 4. Thư Viện Chuẩn `std/string.vit`

Bổ sung thư viện chuẩn bọc các hàm C string interop:
```javascript
extern function strlen(str: string): number;
extern function strcmp(str1: string, str2: string): number;
extern function strcpy(dest: string, src: string): string;
extern function strcat(dest: string, src: string): string;
```
