# Đặc Tả Kế Hoạch Phase 11: Concurrency & Async Engine (v1.1.0)

Tài liệu này là thiết kế chi tiết và kế hoạch triển khai cho **Phase 11** của trình biên dịch **VIT Compiler**.

---

## 1. Mục Tiêu Phase 11

Xây dựng mô hình lập trình bất đồng bộ và đa luồng hiệu năng cao cho **VIT Compiler**:
1. **`async` / `await` Syntax**: Cú pháp khai báo `async function` và từ khóa `await`.
2. **LLVM IR State Machine Transformation (Coroutines)**: Chuyển đổi hàm `async` thành một State Machine phân bổ trên Stack/Heap.
3. **Multi-threading & Channel Communication (`std/thread`, `std/channel`)**: Tạo Native Worker Threads và giao tiếp truyền tin giữa các luồng an toàn (Message Passing).
4. **Lightweight Event Loop**: Tích hợp Event Loop không chặn (Non-blocking I/O).

---

## 2. Thiết Kế Cú Pháp & Ví Dụ Mã Nguồn

```javascript
import { fetchHttp } from "std/net";
import { Thread, Channel } from "std/thread";

async function fetchUserData(userId: number): Promise<string> {
    let url = "https://api.example.com/users/" + userId;
    let response = await fetchHttp(url);
    return response;
}

function workerTask(ch: Channel<number>): void {
    let sum = 0.0;
    for (let i = 0; i < 1000000; i = i + 1) {
        sum = sum + i;
    }
    ch.send(sum);
}

async function main(): number {
    let data = await fetchUserData(42.0);
    print("Received Data: " + data);

    let ch = new Channel<number>();
    let t = Thread.spawn(() => workerTask(ch));

    let result = ch.receive();
    print("Thread Result: " + result);

    t.join();
    return 0;
}
```

---

## 3. Kiến Trúc Chi Tiết Cần Thay Đổi Trong Codebase

### 3.1. Lexer & Parser
* Token mới: `async`, `await`.
* Parse `async function` và `await expr`.

### 3.2. LLVM IR CodeGen: State Machine Transformation
* Hạ cấp hàm `async` thành State Machine phân bổ trên Stack/Heap với rẽ nhánh `yield`.

### 3.3. Runtime Event Loop & Thread Pool
* Tích hợp Runtime Event Loop nền móng (epoll/kqueue/IOCP).
