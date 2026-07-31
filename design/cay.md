┌─────────────────────────────────────────┐
               │    FILE MÃ NGUỒN NGÔN NGỮ MỚI (.jslik)  │
               │   let x = 10; function add(a, b) ...    │
               └────────────────────┬────────────────────┘
                                    │
                                    ▼
┌─────────────────────────────────────────────────────────────────────────┐
│                      PHẦN 1: FRONTEND (Do BẠN viết)                     │
│               (Viết bằng C++ hoặc Rust để xử lý Cú pháp)                │
├─────────────────────────────────────────────────────────────────────────┤
│                                                                         │
│  [1. LEXER / SCANNER] (Phân tích từ vựng)                               │
│   ├── Input:  Chuỗi văn bản mã nguồn                                    │
│   └── Output: Danh sách các Token [LET, IDENT("x"), ASSIGN, INT(10)]    │
│                                   │                                     │
│                                   ▼                                     │
│  [2. PARSER] (Phân tích cú pháp)                                        │
│   ├── Input:  Danh sách Token                                           │
│   └── Output: Cây Cú Pháp Trừu Tượng (AST - Abstract Syntax Tree)       │
│               Ví dụ:  VarDeclStmt                                       │
│                       ├── Name: "x"                                     │
│                       └── Value: IntLiteral(10)                         │
│                                   │                                     │
│                                   ▼                                     │
│  [3. SEMANTIC ANALYZER] (Phân tích ngữ nghĩa & Kiểm tra kiểu)           │
│   ├── Kiểm tra: Biến 'x' đã khai báo chưa? Cộng chuỗi với số có được k? │
│   └── Output: AST đã được kiểm tra tính hợp lệ (Typed AST)              │
│                                                                         │
└───────────────────────────────────┬─────────────────────────────────────┘
                                    │
                                    ▼ (Truyền Cây AST hợp lệ qua)
┌─────────────────────────────────────────────────────────────────────────┐
│              PHẦN 2: CODE GENERATOR (Cầu nối giữa BẠN & LLVM)           │
├─────────────────────────────────────────────────────────────────────────┤
│                                                                         │
│  [4. LLVM IR BUILDER] (Duyệt cây AST để tạo mã trung gian)              │
│   ├── Dùng API: llvm::IRBuilder (C++) hoặc inkwell (Rust)                │
│   └── Action: Duyệt từng Node trên AST → Gọi hàm sinh mã LLVM tương ứng │
│                                   │                                     │
│                                   ▼                                     │
│   └── Output: Mã trung gian LLVM IR (File text .ll hoặc Memory Module)  │
│               Ví dụ: %x = alloca i32, align 4                           │
│                      store i32 10, i32* %x, align 4                     │
│                                                                         │
└───────────────────────────────────┬─────────────────────────────────────┘
                                    │
                                    ▼ (Giao toàn bộ LLVM IR cho Cỗ máy LLVM)
┌─────────────────────────────────────────────────────────────────────────┐
│                       PHẦN 3: BACKEND (Do LLVM lo)                      │
│                  (Cỗ máy LLVM C++ xử lý Tối ưu & Mã máy)                │
├─────────────────────────────────────────────────────────────────────────┤
│                                                                         │
│  [5. LLVM OPTIMIZER (Pass Manager)]                                     │
│   └── Tối ưu hóa code: Xóa code thừa, gộp tính toán trước (O2, O3)      │
│                                   │                                     │
│                                   ▼                                     │
│  [6. TARGET MACHINE & CODE EMISSION]                                    │
│   ├── Chuyển LLVM IR thành Mã Assembler cho Chip (x86_64, ARM64...)     │
│   └── Gọi Linker (lld/gcc) để gộp mã máy với C-Runtime / Hệ điều hành   │
│                                                                         │
└───────────────────────────────────┬─────────────────────────────────────┘
                                    │
                                    ▼
               ┌─────────────────────────────────────────┐
               │    FILE THỰC THI NATIVE (.exe / Binary) │
               │      Chạy trực tiếp không cần VM/JS     │
               └─────────────────────────────────────────┘