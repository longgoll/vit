# 🚀 VS Code Extension for VIT & Vito Language Support

[English](#english) | [Tiếng Việt](#tiếng-việt)

---

<a name="english"></a>
## 🇬🇧 English Documentation

### Overview
Official Visual Studio Code extension for the **VIT Programming Language** and **Vito Web Framework**. Provides a rich IDE experience powered by native Language Server Protocol (LSP).

### ✨ Features
- 🎨 **Syntax Highlighting**: Rich tokenization for `.vit` source files, keywords, strings, types, and annotations.
- ⚡ **CodeLens 1-Click Execution**: Quick `Run file` action right above `function main()`.
- 🧠 **Intellisense & Autocomplete**: Context-aware completion powered by `vit-lsp`.
- 💡 **Hover Documentation & Signature Help**: View docstrings, function signatures, and param types on hover.
- 🏷 **Inlay Hints**: Automatic variable type hints and function return type displays.
- 🛠 **Rename Symbol & Formatting**: Rename variables/functions across scope safely; format code cleanly.
- 🛑 **Rich Error Diagnostics**: Inline syntax error and type checking error highlights.

### 📥 Installation

#### Option 1: Direct `.vsix` Package Installation
1. Open VS Code.
2. Press `Ctrl+Shift+P` (or `Cmd+Shift+P` on macOS) to open the Command Palette.
3. Select **Extensions: Install from VSIX...**.
4. Choose the latest `.vsix` bundle inside `vit/editors/vscode-vit/` (e.g. `vscode-vit-2.3.0.vsix`).

#### Option 2: Developer / Manual Setup
```bash
cd vit/editors/vscode-vit
npm install
# Open in VS Code and press F5 to start extension debugging host
```

---

<a name="tiếng-việt"></a>
## 🇻🇳 Tài Liệu Tiếng Việt

### Tổng Quan
Extension chính thức dành cho Visual Studio Code hỗ trợ ngôn ngữ lập trình **VIT** và framework **Vito**. Cung cấp trải nghiệm lập trình IDE hiện đại được tích hợp trực tiếp với Language Server Protocol (LSP native).

### ✨ Tính Năng Nổi Bật
- 🎨 **Tô Màu Cú Pháp (Syntax Highlighting)**: Đầy đủ màu sắc nhận diện từ khóa, kiểu dữ liệu, chuỗi, hàm và struct cho các tệp `.vit`.
- ⚡ **Chạy Code 1-Click (CodeLens)**: Nút `Run` tiện lợi xuất hiện ngay trên hàm `function main()`.
- 🧠 **Gợi Ý Code Tự Động (Intellisense & Autocomplete)**: Tự động hoàn thành code theo ngữ cảnh nhờ `vit-lsp`.
- 💡 **Xem Document & Chú Thích (Hover & Signature Help)**: Rà chuột để xem mô tả hàm, kiểu dữ liệu tham số và giá trị trả về.
- 🏷 **Gợi Ý Kiểu Dữ Liệu ngầm (Inlay Hints)**: Hiển thị tự động kiểu dữ liệu biến và kiểu trả về mà không cần khai báo tường minh.
- 🛠 **Đổi Tên Biến Hàng Loạt (Rename Symbol) & Định Dạng (Formatting)**: Đổi tên an toàn theo scope và căn chỉnh code chuẩn hóa.
- 🛑 **Báo Lỗi Trực Tiếp (Inline Diagnostics)**: Hiển thị gạch chân đỏ thông báo lỗi cú pháp hoặc lỗi ngữ nghĩa ngay trong khi gõ.

### 📥 Hướng Dẫn Cài Đặt

#### Cách 1: Cài đặt qua file `.vsix`
1. Mở VS Code.
2. Nhấn tổ hợp phím `Ctrl+Shift+P` (hoặc `Cmd+Shift+P` trên macOS).
3. Chọn **Extensions: Install from VSIX...**.
4. Trỏ tới file `.vsix` mới nhất trong thư mục `vit/editors/vscode-vit/` (ví dụ `vscode-vit-2.3.0.vsix`).

#### Cách 2: Dành cho Developer phát triển Extension
```bash
cd vit/editors/vscode-vit
npm install
# Mở thư mục này bằng VS Code và nhấn phím F5 để khởi chạy môi trường Debug Extension
```
