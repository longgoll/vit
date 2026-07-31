# Đặc Tả Kế Hoạch Phase 12: Network Engine & Web Server Framework (v1.2.0)

Tài liệu này là thiết kế chi tiết và kế hoạch triển khai cho **Phase 12** của trình biên dịch **VIT Compiler**.

---

## 1. Mục Tiêu Phase 12

Tận dụng **Async Engine, Multithreading & Channels** của Phase 11 để phát triển hệ thống Thư viện Mạng (Networking) và Web Framework native cho ngôn ngữ Vit:
1. **Low-level Network Sockets (`std/net`)**: Hỗ trợ TCP/UDP Client & Server Sockets non-blocking (tương thích Windows WinSock2 & POSIX Sockets).
2. **HTTP Protocol Parser & Client (`std/http`)**: Xử lý parse HTTP Request/Response (HTTP/1.1), đồng thời cung cấp HTTP Client (`httpGet`, `httpPost`).
3. **Async Web Server & Routing System**: Xây dựng HTTP Server chạy trên mô hình Coroutine/Task Async, hỗ trợ router (`GET`, `POST`, `PUT`, `DELETE`), middleware và JSON responses.

---

## 2. Kiến Trúc Cấu Trúc File & Components

```text
vit/
├── src/
│   └── runtime/
│       └── net_rt.c        # C runtime native socket abstraction (WinSock / POSIX)
├── std/
│   ├── net.vit             # Low-level Socket API (TcpListener, TcpStream, UdpSocket)
│   └── http.vit            # HTTP Request, Response, HttpClient, HttpServer & Router
└── test/
    └── Phase12/            # Integration & benchmark test cases cho HTTP Server/Client
```

---

## 3. Cú Pháp & Code Syntax Ví Dụ

### 3.1. Low-level TCP Echo Server (`std/net`)
```javascript
import { TcpListener, TcpStream } from "std/net";

async function main(): number {
    let listener = TcpListener.bind("127.0.0.1", 8080);
    print("TCP Server running on 127.0.0.1:8080");

    while (true) {
        let stream = await listener.accept();
        async (): void => {
            let buffer = stream.read();
            stream.write("Echo: " + buffer);
            stream.close();
        };
    }
    return 0;
}
```

### 3.2. Web Server Framework (`std/http`)
```javascript
import { HttpServer, Request, Response } from "std/http";

async function main(): number {
    let app = new HttpServer();

    // 1. Route GET với JSON response
    app.get("/api/health", async (req: Request, res: Response) => {
        res.json({ status: "ok", version: "1.2.0" });
    });

    // 2. Route POST xử lý dữ liệu
    app.post("/api/echo", async (req: Request, res: Response) => {
        let body = req.getBody();
        res.send("Received: " + body);
    });

    // 3. Khởi chạy HTTP Server
    print("HTTP Server ready at http://localhost:3000");
    await app.listen(3000);
    return 0;
}
```

---

## 4. Chi Tiết Triển Khai Triển Khai Trong Codebase

### 4.1. Runtime Native Layer (`src/runtime/net_rt.c`)
* Khởi tạo `WSAStartup` trên Windows hoặc bỏ qua trên POSIX/Linux.
* Hàm c wrapper:
  - `vit_net_socket_create(type: int)`
  - `vit_net_socket_bind(fd: int, ip: char*, port: int)`
  - `vit_net_socket_listen(fd: int, backlog: int)`
  - `vit_net_socket_accept(fd: int)`
  - `vit_net_socket_connect(fd: int, ip: char*, port: int)`
  - `vit_net_socket_send(fd: int, buf: char*, len: int)`
  - `vit_net_socket_recv(fd: int, buf: char*, max_len: int)`
  - `vit_net_socket_close(fd: int)`

### 4.2. Standard Library (`std/net.vit` & `std/http.vit`)
* Bọc C FFI functions thành các struct đẹp mắt, tích hợp cơ chế `async/await` của Phase 11.
* Impl HTTP Parser: Tách header `Content-Type`, `Content-Length`, path, query parameters.
* Impl HTTP Client: Hỗ trợ gửi HTTP GET/POST request và trả về `HttpResponse` struct.

---

## 5. Kế Hoạch Kiểm Thử (Verification Plan)
1. **TCP Unit Tests**: Test kết nối socket gửi/nhận chuỗi nhị phân và text.
2. **HTTP Parser Tests**: Test parse request hợp lệ và không hợp lệ.
3. **High-Concurrency Benchmark**: Test HTTP Server chịu tải hàng nghìn request/giây với `wrk` hoặc `ab`.
