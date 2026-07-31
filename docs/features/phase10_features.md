# Đặc Tả Kế Hoạch Phase 10: Concurrency & Async Engine (v0.10.0)

Tài liệu này là thiết kế chi tiết và kế hoạch triển khai cho **Phase 10** của trình biên dịch **VIT Compiler**.

---

## 1. Mục Tiêu Phase 10

Xây dựng mô hình lập trình bất đồng bộ và đa luồng hiệu năng cao cho **VIT Compiler**:
1. **`async` / `await` Syntax**: Cú pháp khai báo `async function` và từ khóa `await` cho phép viết mã bất đồng bộ trông giống mã đồng bộ.
2. **LLVM IR State Machine Transformation (Coroutines)**: Chuyển đổi hàm `async` thành một State Machine phân bổ trên Stack/Heap.
3. **Multi-threading & Channel Communication (`std/thread`, `std/channel`)**: Tạo Native Worker Threads và giao tiếp truyền tin giữa các luồng an toàn (Message Passing).
4. **Lightweight Event Loop**: Tích hợp Event Loop không chặn (Non-blocking I/O).

---

## 2. Thiết Kế Cú Pháp & Ví Dụ Mã Nguồn

```javascript
import { fetchHttp } from "std/net";
import { Thread, Channel } from "std/thread";

// 1. Async Function Definition
async function fetchUserData(userId: number): Promise<string> {
    print("Fetching user data asynchronously...");
    let url = "https://api.example.com/users/" + userId;
    let response = await fetchHttp(url); // Non-blocking await
    return response;
}

// 2. Multi-threading & Channels
function workerTask(ch: Channel<number>): void {
    let sum = 0.0;
    for (let i = 0; i < 1000000; i = i + 1) {
        sum = sum + i;
    }
    ch.send(sum); // Gửi kết quả qua channel
}

async function main(): number {
    // Gọi hàm Async
    let data = await fetchUserData(42.0);
    print("Received Data: " + data);

    // Giao tiếp Đa Luồng (Worker Thread & Channel)
    let ch = new Channel<number>();
    let t = Thread.spawn(() => workerTask(ch));

    let result = ch.receive(); // Nhận dữ liệu từ worker thread
    print("Thread Result: " + result);

    t.join();
    return 0;
}
```

---

## 3. Kiến Trúc Chi Tiết Cần Thay Đổi Trong Codebase

### 3.1. Lexer & Parser
* **Token mới**: `TokenType::KwAsync` (`async`), `TokenType::KwAwait` (`await`).
* **Parser**: Parse `async function` thành `FunctionDeclASTNode` có cờ `isAsync = true`. Parse `await expr` thành `AwaitExprASTNode(expr)`.

### 3.2. LLVM IR CodeGen: State Machine Transformation
* Khi biên dịch một hàm `async`, `LLVMCodeGen` hạ cấp hàm đó thành một cấu trúc dữ liệu State Machine chứa:
  - Trạng thái hiện tại (`i32 state`).
  - Các biến cục bộ sống qua lời gọi `await`.
* Mỗi điểm `await` tạo ra một điểm rẽ nhánh (yield point) trả quyền điều khiển lại cho Event Loop.

### 3.3. Runtime Event Loop & Thread Pool (`src/runtime/event_loop.cpp`)
* Tích hợp Runtime Event Loop nền móng (sử dụng libuv hoặc OS Native I/O epoll/kqueue/IOCP).

---

## 4. Danh Sách File Cần Cập Nhật Phân Chia Theo Task

1. **Task 1: Lexer & Parser for `async`/`await`**
   - `include/lexer/Token.h`, `src/lexer/Lexer.cpp`, `src/parser/Parser.cpp`.

2. **Task 2: LLVM IR Generator for Coroutines & State Machine**
   - `src/codegen/LLVMCodeGen.cpp`: Hạ cấp `async function` xuống Coroutine State Machine.

3. **Task 3: Threading & Channel Standard Library (`std/thread.vit`)**
   - Triển khai `Thread.spawn()` và `Channel<T>`.

4. **Task 4: Non-blocking I/O Event Loop Integration**
   - `src/runtime/event_loop.cpp` & `test/Phase-10/test_async.vit`.
