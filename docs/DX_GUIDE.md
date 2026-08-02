# Vit Developer Guide & DX Cheatsheet

Chào mừng bạn đến với tài liệu trải nghiệm nhà phát triển (DX Guide) chính thức của **Vit Language** (v2.0.0+).

---

## 🚀 Quick Start Guide

### 1. Cài đặt & Setup PATH
Thêm bộ công cụ Vit vào biến môi trường PATH hệ thống (Windows):
```powershell
vit setup
```

### 2. Khởi tạo Dự án Mới
```bash
vit init my-app
cd my-app
```
Lệnh này sẽ khởi tạo cấu trúc thư mục với file cấu hình `vit.json` và mã mẫu `src/main.vit`.

---

## 🛠️ Trình Biên Dịch & Công Cụ CLI (`vit`)

### Run & Build
- **Chạy trực tiếp (Zero residual files):**
  ```bash
  vit run src/main.vit
  # Hoặc ngắn gọn:
  vit src/main.vit
  ```
- **Biên dịch Native Binary (.exe / Linux / WASM):**
  ```bash
  # Fast debug build
  vit build src/main.vit -O0 -o build/app.exe

  # High performance Release build
  vit build src/main.vit -O3 --lto=thin -march=native -o build/app.exe

  # Giữ lại file LLVM IR intermediate
  vit build src/main.vit --save-temps
  ```

### Code Formatting & Linting
- **Tự động format code theo tiêu chuẩn Vit:**
  ```bash
  vit fmt src/main.vit
  ```
- **Kiểm tra mã nguồn & cảnh báo code smells:**
  ```bash
  vit lint src/main.vit
  ```

---

## 🎨 VS Code Extension Integration

Extension `vscode-vit` hỗ trợ trải nghiệm IDE cao cấp:
1. **Real-time Diagnostics & Syntax Highlighting:** Gạch chân đỏ báo lỗi trực tiếp khi đang nhập liệu.
2. **Auto-Completion & Hover Info:** Di chuột để xem kiểu dữ liệu, hàm và doc-comments `//`.
3. **Format on Save:** Tự động gọi `vit fmt` mỗi khi nhấn `Ctrl + S`.
4. **1-Click Run CodeLens:** Nhấn **▶ Run Vit File** ngay trên hàm `main()`.

---

## 📚 Standard Library Cheat Sheet (`std/`)

| Module | Đường dẫn import | Tính năng chính |
| :--- | :--- | :--- |
| **System** | `import "std/sys"` | `getTimestampMs()`, `sleepMs(ms)`, `system(cmd)`, `exit(code)` |
| **IO** | `import "std/io"` | `print(str)`, `println(str)` |
| **Math** | `import "std/math"` | Các phép toán hình học & số học |
| **File System** | `import "std/fs"` | Đọc / Ghi / Kiểm tra file |
| **HTTP / Net** | `import "std/net"` | Khởi tạo HTTP client / server sockets |
| **JSON** | `import "std/json"` | `json_parse()`, `json_stringify()` |

---
