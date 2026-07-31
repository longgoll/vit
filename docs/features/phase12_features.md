# Đặc Tả Kế Hoạch Phase 12: Developer Experience & Ecosystem (v1.2.0)

Tài liệu này là thiết kế chi tiết và kế hoạch triển khai cho **Phase 12** của trình biên dịch **VIT Compiler**.

---

## 1. Mục Tiêu Phase 12

Tạo dựng hệ sinh thái công cụ hỗ trợ lập trình viên (Developer Experience - DX) đỉnh cao:
1. **Language Server Protocol (`vit-lsp`)**: Server LSP cho IDEs (VS Code) hỗ trợ Autocomplete, Go-to-definition, Hover, và Diagnostics.
2. **Package Manager (`vit pm`)**: Quản lý thư viện và dự án via `vit.toml` / `vit.json`.
3. **Interactive REPL (`vit repl`)**: Môi trường tương tác gõ mã chạy trực tiếp nhờ LLVM ORC JIT Engine.
4. **Formatter & Linter (`vit fmt`, `vit lint`)**: Công cụ tự động định dạng mã nguồn và kiểm tra mùi mã.

---

## 2. Thiết Kế Công Cụ & Cú Pháp Dòng Lệnh

```cmd
# 1. Khởi tạo dự án Vit mới
vit init my-app

# 2. Cài đặt thư viện phụ thuộc
vit add github.com/user/vit-http

# 3. Môi trường REPL JIT
vit repl

# 4. Định dạng lại mã nguồn
vit fmt src/
```

---

## 3. Kiến Trúc Chi Tiết Cần Thay Đổi Trong Codebase

### 3.1. Language Server Protocol (`src/tools/lsp/`)
* Binary `vit-lsp` giao tiếp JSON-RPC 2.0 trên STDIN/STDOUT.

### 3.2. LLVM ORC JIT Engine cho `vit repl` (`src/tools/repl/`)
* Sử dụng LLVM `orc::LLJIT` để biên dịch tức thì từng câu lệnh Vit.

### 3.3. Package Manager (`src/tools/pm/`)
* Lệnh `vit init`, `vit add`, `vit install`, parse `vit.json`.
