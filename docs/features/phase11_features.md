# Đặc Tả Kế Hoạch Phase 11: Developer Experience & Ecosystem (v0.11.0)

Tài liệu này là thiết kế chi tiết và kế hoạch triển khai cho **Phase 11** của trình biên dịch **VIT Compiler**.

---

## 1. Mục Tiêu Phase 11

Tạo dựng hệ sinh thái công cụ hỗ trợ lập trình viên (Developer Experience - DX) đỉnh cao:
1. **Language Server Protocol (`vit-lsp`)**: Xây dựng server LSP cho IDEs (VS Code) hỗ trợ Gợi ý mã (Autocomplete), Chuyển đến định nghĩa (Go-to-definition), Xem tài liệu (Hover), và Báo lỗi trực tiếp (Diagnostics).
2. **Package Manager (`vit pm`)**: Công cụ quản lý thư viện và dự án qua file `vit.json` / `vit.toml` (Cài đặt, cập nhật, tải package từ Git).
3. **Interactive REPL (`vit repl`)**: Môi trường chạy mã trực tiếp tương tác nhờ LLVM ORC JIT Engine.
4. **Formatter & Linter (`vit fmt`, `vit lint`)**: Công cụ tự động định dạng mã nguồn và phát hiện mùi mã xấu.

---

## 2. Thiết Kế Công Cụ & Cú Pháp Dòng Lệnh

```cmd
# 1. Khởi tạo dự án Vit mới
vit init my-app

# 2. Cài đặt một thư viện phụ thuộc
vit add github.com/user/vit-http

# 3. Chạy Môi Trường REPL Tương Tác (Read-Eval-Print Loop)
vit repl
> let x = 10;
> let y = 20;
> print(x + y);
30

# 4. Định dạng lại toàn bộ file trong dự án
vit fmt src/

# 5. Kiểm tra mã nguồn (Linter)
vit lint src/
```

---

## 3. Kiến Trúc Chi Tiết Cần Thay Đổi Trong Codebase

### 3.1. Language Server Protocol (`src/tools/lsp/`)
* Xây dựng binary `vit-lsp` giao tiếp qua chuẩn JSON-RPC 2.0 trên STDIN/STDOUT.
* Tận dụng `Parser` và `SemanticAnalyzer` của Vit để tạo ra Abstract Syntax Tree & Symbol Index cho từng file trong project.

### 3.2. LLVM ORC JIT Engine cho `vit repl` (`src/tools/repl/`)
* Sử dụng LLVM `orc::LLJIT` để biên dịch tức thì từng dòng lệnh Vit gõ từ bàn phím thành mã máy x86_64/ARM64 và thực thi ngay lập tức mà không cần tạo file binary tạm.

### 3.3. Package Manager Subcommands (`src/cli/pm.cpp`)
* Bổ sung lệnh `vit init`, `vit add`, `vit install`, `vit fmt`.
* Phân tích cú pháp file `vit.json`:
  ```json
  {
    "name": "my-app",
    "version": "1.0.0",
    "dependencies": {
      "vit-http": "v1.2.0"
    }
  }
  ```

---

## 4. Danh Sách File Cần Cập Nhật Phân Chia Theo Task

1. **Task 1: LLVM ORC JIT Engine & REPL Subcommand (`vit repl`)**
   - `include/codegen/JITEngine.h`, `src/codegen/JITEngine.cpp`, `src/tools/repl/REPL.cpp`.

2. **Task 2: Package Manager Subcommands (`vit init`, `vit add`)**
   - `src/tools/pm/PackageManager.cpp`.

3. **Task 3: Code Formatter & Linter (`vit fmt`, `vit lint`)**
   - `src/tools/fmt/Formatter.cpp`.

4. **Task 4: Language Server Protocol Executable (`vit-lsp`)**
   - `src/tools/lsp/main.cpp` & VS Code Extension manifest.
