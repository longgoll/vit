# Đặc Tả Kế Hoạch Phase 10: Self-Hosting Compiler (v1.0.0 Milestone)

Tài liệu này là thiết kế chi tiết cho cột mốc quan trọng nhất của **VIT Compiler**: **Self-Hosting (Trình biên dịch Tự biên dịch)**.

---

## 1. Mục Tiêu Phase 10

Biến VIT thành một ngôn ngữ hoàn toàn độc lập bằng cách viết lại chính trình biên dịch **VIT Compiler bằng chính ngôn ngữ VIT (`vitc.vit`)**:
1. **Bootstrap Phase (Thủ tục Tự khởi sinh)**:
   - Dùng trình biên dịch C++ (`build/vit.exe`) hiện tại để biên dịch mã nguồn trình biên dịch viết bằng Vit (`src_vit/main.vit`) thành file thực thi `vitc_stage1.exe`.
   - Dùng `vitc_stage1.exe` tự biên dịch lại mã nguồn `src_vit/main.vit` thành `vitc_stage2.exe`.
2. **Kiểm tra tính nhất quán (Bootstrapping Verification)**: So sánh đầu ra giữa `stage1` và `stage2`.

---

## 2. Tiền Đề Tính Năng Đã Có (Từ Phase 1 -> Phase 9)

Sau khi hoàn thành Phase 8 và Phase 9, Vit đã sở hữu 100% công cụ nòng nốt để tự viết chính trình biên dịch của mình:
- **Phase 5**: Struct Methods & String Ops ➔ Xử lý chuỗi mã nguồn & AST Nodes.
- **Phase 7**: Generics, Enums & Pattern Matching ➔ Biểu diễn Token & AST Node an toàn.
- **Phase 7**: File I/O (`std/fs.vit`) ➔ Đọc file `.vit` & xuất file `.ll` (LLVM IR).
- **Phase 8**: Result<T, E> & Try Operator (`?`) ➔ Quản lý lỗi Lexer/Parser không gây crash.
- **Phase 9**: HashMap<K, V> ➔ Xây dựng Bảng ký hiệu (Symbol Table) & Scope Resolver.

---

## 3. Kiến Trúc Của Self-Hosted Compiler (`src_vit/`)

```text
src_vit/
├── lexer/
│   ├── token.vit       # Enum Token, TokenType
│   └── lexer.vit       # Lexer class phân tích từ vựng
├── parser/
│   ├── ast.vit         # Struct Node cho AST
│   └── parser.vit      # Recursive Descent Parser
├── semantics/
│   ├── symbol_table.vit # HashMap quản lý biến và hàm
│   └── type_checker.vit # Kiểm tra kiểu dữ liệu
├── codegen/
│   └── llvm_emitter.vit # LLVM IR Text Generation
└── main.vit            # CLI Entry Point chính của vitc
```

---

## 4. Quy Trình Bootstrapping 3 Giai Đoạn

```cmd
# Stage 0: Trình biên dịch C++ gốc (vit_cpp.exe) biên dịch mã nguồn Vit
vit_cpp.exe build src_vit/main.vit -o bin/vitc_stage1.exe

# Stage 1: Trình biên dịch bằng Vit (vitc_stage1.exe) tự biên dịch chính nó
bin/vitc_stage1.exe build src_vit/main.vit -o bin/vitc_stage2.exe

# Stage 2: Kiểm tra tính đúng đắn
bin/vitc_stage2.exe run test/Phase-1/test_hello.vit
```
