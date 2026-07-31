Đây là bản Specification (Đặc tả dự án) được viết chuẩn hóa theo dạng Prompt / System Architecture Document. Bạn chỉ cần sao chép toàn bộ đoạn dưới đây và dán cho AI lập trình (như Claude 3.5 Sonnet, GPT-4o, hay Cursor/Windsurf) là nó sẽ hiểu chính xác 100% nhiệm vụ và bắt đầu viết code cho bạn ngay!

Markdown
# PROJECT SPECIFICATION: A JS-Like Syntax Programming Language Compiled via LLVM

## 1. Executive Summary (Tổng quan Ý tưởng)
Tạo một ngôn ngữ lập trình mới kết hợp giữa:
* **Syntax:** Đơn giản, tự nhiên, dễ đọc giống JavaScript / TypeScript (dùng `function`, `let`, `const`, `return`).
* **Performance:** Hiệu năng cao, an toàn, biên dịch trực tiếp ra mã máy (Native Binary / Machine Code) thông qua cỗ máy biên dịch **LLVM IR**.
* **Memory Management:** Quản lý bộ nhớ tự động, nhẹ nhàng (ở các bản sau sẽ dùng Automatic Reference Counting - ARC giống Swift) để không bị giật lag vì Garbage Collector (như JS/Python) và không bị khó như Borrow Checker (của Rust).

---

## 2. Technical Stack (Công nghệ đề xuất)
* **Language to write Compiler:** C++ 
* **Parser Generator:** ANTLR4 hoặc Tree-sitter (hoặc viết Hand-written Recursive Descent Parser đơn giản).
* **Backend:** LLVM (LLVM API / LLVM IR Generator).

---

## 3. Scope & Roadmap (Lộ trình phát triển từng bước - MVP First)

### Phase 1: MVP - The Foundation (Mục tiêu tối thượng ban đầu)
Chỉ tập trung vào 5% tính năng cốt lõi nhất để đảm bảo biên dịch được 1 file mã máy chạy được:
1. **Types:** Primitive type `number` (float64 hoặc int32).
2. **Statements:** Khai báo biến (`const`, `let`), gán giá trị, câu điều kiện `if/else`, câu lệnh `return`.
3. **Functions:** Khai báo hàm đơn giản `function add(a: number, b: number): number`.
4. **I/O:** Hàm tích hợp sẵn `print(val)` để in số ra màn hình console.

**Mẫu code ví dụ cần chạy được ở Phase 1:**
```javascript
function add(a: number, b: number): number {
    return a + b;
}

function main(): number {
    let x = 10;
    let y = 20;
    let result = add(x, y);
    print(result);
    return 0;
}
4. Architectural Pipeline (Kiến trúc trình biên dịch)
Lexer & Parser:

Input: Source code file (.js-like text).

Process: Phân tích từ vựng và cú pháp, tạo ra Abstract Syntax Tree (AST).

Semantic Analyzer:

Kiểm tra kiểu dữ liệu (Type checking) cơ bản trên cây AST.

Code Generator (LLVM IR Module):

Duyệt cây AST.

Sử dụng llvm::IRBuilder để chuyển từng node AST thành các lệnh LLVM IR tương ứng (ví dụ: CreateAdd, CreateFCmp, CreateCall).

Native Code Emission:

Dùng LLVM Target Machine để biên dịch LLVM IR thành file thực thi .exe (Windows) hoặc Binary executable (Linux/macOS).

5. Instructions for AI Coding Assistant (Nhiệm vụ cho AI Code)
Dear AI Assistant, hãy giúp tôi bắt đầu phát triển Trình biên dịch cho ngôn ngữ này theo thứ tự:

Task 1: Thiết kế cấu trúc các Data Structure cho cây AST (ProgramNode, FunctionNode, BinaryOpNode, VariableNode, PrintNode...).

Task 2: Viết một bộ Parser đơn giản để đọc đoạn mã mẫu ở Phase 1 và trả về cây AST.

Task 3: Viết module CodeGen kết nối với LLVM C++ API (hoặc LLVM bindings tương ứng) để chuyển đổi AST thành LLVM IR.

Task 4: Viết hàm main cho Compiler để đọc file source, biên dịch ra LLVM IR và xuất ra file chạy binary.

Hãy bắt đầu bằng Task 1 trước. Hỏi lại tôi nếu cần làm rõ thêm chi tiết kỹ thuật!

