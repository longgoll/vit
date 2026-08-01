# Quy Trình Kiểm Thử Benchmark SSH Trực Tiếp (SSH Remote Benchmark Protocol)

> **Dành cho AI Agents & Developers**: Tài liệu này quy định quy trình chuẩn hóa để thực hiện kiểm thử benchmark hiệu năng bare-metal qua SSH cho Vit Engine & Vito Framework. Tất cả thông tin nhạy cảm (IP, Password, Credentials) **tuyệt đối phải sử dụng biến môi trường hoặc placeholder** để đảm bảo an toàn khi công khai repository.

---

## 1. Nguyên Tắc Bảo Mật (Security Guidelines)

> [!CAUTION]
> **TUYỆT ĐỐI KHÔNG** hardcode IP nội bộ, Username, Password hoặc SSH Keys trong các file source code (`.py`, `.sh`, `.bat`, `.md`).

- **Dùng Biến Môi Trường (Environment Variables)**:
  ```python
  import os

  HOST = os.getenv("BENCHMARK_SSH_HOST", "127.0.0.1")
  PORT = int(os.getenv("BENCHMARK_SSH_PORT", "22"))
  USER = os.getenv("BENCHMARK_SSH_USER", "remote_user")
  PASS = os.getenv("BENCHMARK_SSH_PASS", "")
  ```
- **Tài liệu & Báo cáo công khai**: Sử dụng địa chỉ IP giả định / ví dụ như `192.168.x.x` hoặc `bench-node.internal`.

---

## 2. Quy Hoạch Thư Mục Test Phase 15 (`vit/test/Phase15`)

Tất cả các thành phần kiểm thử Phase 15 phải tuân theo cấu trúc phân cấp chuẩn sau:

```text
vit/test/Phase15/
├── scripts/       # Kịch bản chạy SSH tự động (remote_showdown.py, remote_benchmark.py, ...)
├── servers/       # Source code server của các ngôn ngữ (benchmark_server.c, go_server.go, rust_server.rs)
├── bin/           # File thực thi binary đã biên dịch (.exe, .o, native ELF)
└── reports/       # Báo cáo kết quả đo đạc (JSON, log output từ wrk/k6)
```

---

## 3. Quy Trình Chạy Test SSH Tự Động (Automation Protocol)

Khi AI Agent thực hiện chạy test SSH remote:

### Bước 1: Nén & Tải Source Code Lên Remote Server
1. Nén toàn bộ repository local (loại bỏ `.git`, `node_modules`, `build/`).
2. Mở kết nối SFTP tới Remote Host và đưa bản nén lên thư mục tạm (ví dụ `/tmp/vit_showdown`).

### Bước 2: Biên Dịch Server Native Trên Remote Host
Chạy các lệnh build tối ưu hóa tối đa cho CPU của máy chủ:
- **Vito Framework (C Engine)**:
  ```bash
  gcc -O3 -march=native -flto -Iinclude -Isrc test/Phase15/servers/benchmark_server.c \
      src/runtime/memory_rt.c src/runtime/async_iouring_rt.c \
      src/runtime/http_parser_simd.c src/runtime/net_rt.c -pthread -o vito_server
  ```
- **Go Server**:
  ```bash
  go build -ldflags="-s -w" -o go_server test/Phase15/servers/go_server.go
  ```
- **Rust Server**:
  ```bash
  rustc -O -C opt-level=3 -C target-cpu=native test/Phase15/servers/rust_server.rs -o rust_server
  ```

### Bước 3: Đo Đạc Hiệu Năng Bằng `wrk`
- Sử dụng lệnh `wrk` với thông số tiêu chuẩn:
  ```bash
  wrk -t16 -c1000 -d10s --latency http://localhost:8080/json
  ```
- Lần lượt khởi chạy từng HTTP server trên các port riêng biệt (`8080`, `8081`, `8082`), chạy `wrk`, ghi nhận kết quả và `pkill` ngay sau khi đo xong.

---

## 4. Xử Lý Lỗi Unicode Trên Windows Console

> [!TIP]
> Khi xuất báo cáo từ Python script chạy trên môi trường Windows Terminal (mã hóa `CP1252`), không được dùng các ký tự Unicode Emoji không tương thích trực tiếp trong lệnh `print()` để tránh lỗi `UnicodeEncodeError`.

---

## 5. Đồng Bộ Kết Quả Lên Website

Sau khi chạy xong test SSH:
1. Tổng hợp số liệu **Throughput (req/s)**, **Avg Latency**, **P99 Latency**.
2. Cập nhật vào component [BenchmarkVisualizer.vue](file:///f:/Dev/product/vit-lag/vito/website/.vitepress/theme/components/BenchmarkVisualizer.vue) và file báo cáo [BENCHMARK_WHITE_PAPER.md](file:///f:/Dev/product/vit-lag/vito/website/public/BENCHMARK_WHITE_PAPER.md).
