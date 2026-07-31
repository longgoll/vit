# Đặc Tả Kế Hoạch Phase 12: Cross-Compilation, WASM & Optimizations (v1.0.0)

Tài liệu này là thiết kế chi tiết và kế hoạch triển khai cho **Phase 12** của trình biên dịch **VIT Compiler** - cột mốc **Release v1.0.0**.

---

## 1. Mục Tiêu Phase 12

Đạt cột mốc chính thức **VIT Compiler v1.0.0 Release**:
1. **Multi-target Cross-Compilation**: Biên dịch từ một OS (ví dụ Windows) ra file thực thi cho Linux (`x86_64-linux-gnu`), macOS (`aarch64-apple-darwin`), hoặc WebAssembly (`wasm32-wasi`).
2. **WebAssembly Target (`wasm32`)**: Biên dịch thẳng mã Vit thành file `.wasm` chạy được trên Browser và các môi trường Serverless (Cloudflare Workers, WASI).
3. **Custom LLVM ARC Escape Analysis Pass**: Pass tối ưu hóa chuyên sâu tự động loại bỏ các lệnh `retain`/`release` thừa và chuyển phân bổ bộ nhớ từ Heap sang Stack nếu đối tượng không thoát ra khỏi scope hiện tại.

---

## 2. Thiết Kế Cú Pháp & Ví Dụ Sử Dụng CLI

```cmd
# 1. Biên dịch Cross-platform ra Linux x86_64
vit build app.vit --target x86_64-unknown-linux-gnu -O3 -o app_linux

# 2. Biên dịch Cross-platform ra macOS Apple Silicon (ARM64)
vit build app.vit --target aarch64-apple-darwin -O3 -o app_mac

# 3. Biên dịch ra WebAssembly
vit build app.vit --target wasm32-wasi -o app.wasm

# 4. Bật Custom ARC Escape Analysis Pass để tối ưu bộ nhớ tối đa
vit build app.vit -O3 --enable-escape-analysis
```

---

## 3. Kiến Trúc Chi Tiết Cần Thay Đổi Trong Codebase

### 3.1. Target Triple & LLVM Target Machine (`src/codegen/NativeCompiler.cpp`)
* Cấu hình LLVM Target Machine dựa trên cờ `--target <triple>`.
* Tích hợp Clang Cross-linker flags (`-target <triple> --sysroot=<path>`).

### 3.2. WebAssembly Backend Codegen
* Tích hợp LLVM WebAssembly Backend Target (`LLVMInitializeWebAssemblyTarget()`, `LLVMInitializeWebAssemblyTargetInfo()`).
* Chuyển đổi FFI từ C standard library sang WASI (WebAssembly System Interface) imports.

### 3.3. LLVM Custom Pass: ARC Escape Analysis Pass (`src/codegen/ARCEscapeAnalysis.cpp`)
* Xây dựng một custom LLVM IR Function Pass.
* Phân tích đồ thị luồng dữ liệu (Dataflow Analysis): Nếu một struct hoặc mảng khởi tạo trong khối không bị leak ra bên ngoài (không gán vào biến toàn cục, không return, không truyền vào hàm external), chuyển lệnh phân bổ từ `malloc` (heap) sang `alloca` (stack) và xóa bỏ các lệnh `@free()` tương ứng.

---

## 4. Danh Sách File Cần Cập Nhật Phân Chia Theo Task

1. **Task 1: Multi-target CLI & LLVM Target Machine Handling**
   - `src/codegen/NativeCompiler.cpp` & `src/main.cpp`.

2. **Task 2: WASM Target Emitter & WASI Standard Library Integration**
   - `src/codegen/WasmCodeGen.cpp` & `std/wasi.vit`.

3. **Task 3: ARC Custom Escape Analysis Pass**
   - `include/codegen/ARCEscapeAnalysis.h` & `src/codegen/ARCEscapeAnalysis.cpp`.

4. **Task 4: v1.0.0 Release Verification & Benchmarks Suite**
   - `test/benchmarks/`: Benchmark tốc độ thực thi so sánh với Node.js, C++, và Rust.
