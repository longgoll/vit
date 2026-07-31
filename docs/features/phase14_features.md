# Đặc Tả Kế Hoạch Phase 14: Cross-Compilation, WASM & Optimizations (v2.0.0 Release)

Tài liệu này là thiết kế chi tiết cho **Phase 14** của trình biên dịch **VIT Compiler** - cột mốc phát hành **v2.0.0 Release**.

---

## 1. Mục Tiêu Phase 14

Mở rộng môi trường biên dịch đa nền tảng và tối ưu hóa bộ nhớ chuyên sâu:
1. **Multi-target Cross-Compilation**: Biên dịch từ một OS (ví dụ Windows) ra file thực thi cho Linux (`x86_64-linux-gnu`), macOS (`aarch64-apple-darwin`), hoặc WebAssembly (`wasm32-wasi`).
2. **WebAssembly Target (`wasm32`)**: Biên dịch thẳng mã Vit thành file `.wasm` chạy được trên Browser và Serverless runtime.
3. **Custom LLVM ARC Escape Analysis Pass**: Pass tối ưu hóa chuyên sâu tự động loại bỏ các lệnh `retain`/`release` thừa và chuyển phân bổ bộ nhớ từ Heap sang Stack.

---

## 2. Thiết Kế Cú Pháp & CLI Flags

```cmd
# 1. Biên dịch Cross-platform ra Linux x86_64
vit build app.vit --target x86_64-unknown-linux-gnu -O3 -o app_linux

# 2. Biên dịch Cross-platform ra macOS Apple Silicon (ARM64)
vit build app.vit --target aarch64-apple-darwin -O3 -o app_mac

# 3. Biên dịch ra WebAssembly
vit build app.vit --target wasm32-wasi -o app.wasm

# 4. Tối ưu bộ nhớ với Escape Analysis
vit build app.vit -O3 --enable-escape-analysis
```

---

## 3. Kiến Trúc Chi Tiết Cần Thay Đổi Trong Codebase

### 3.1. Target Triple & LLVM Target Machine (`src/codegen/NativeCompiler.cpp`)
* Cấu hình LLVM Target Machine dựa trên cờ `--target <triple>`.

### 3.2. WebAssembly Backend Codegen
* Tích hợp LLVM WebAssembly Backend Target (`LLVMInitializeWebAssemblyTarget()`).

### 3.3. LLVM Custom Pass: ARC Escape Analysis Pass
* Tự động phân tích đồ thị luồng dữ liệu (Dataflow Analysis) để chuyển heap allocations thành stack allocations (`alloca`).
