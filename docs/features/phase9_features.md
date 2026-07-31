# Đặc Tả Kế Hoạch Phase 9: Built-in Collections & Advanced Standard Library (v0.9.0)

Tài liệu này là thiết kế chi tiết và kế hoạch triển khai cho **Phase 9** của trình biên dịch **VIT Compiler**.

---

## 1. Mục Tiêu Phase 9

Cung cấp cho **VIT Compiler** bộ cấu trúc dữ liệu phong phú và thư viện chuẩn cao cấp:
1. **`std/collections`**: Các cấu trúc dữ liệu tối ưu: `HashMap<K, V>`, `Set<T>`, `Queue<T>`, `LinkedList<T>` tận dụng Generics của Phase 7.
2. **`std/json`**: Library Parse và Serialize JSON linh hoạt cho web backend / trao đổi dữ liệu.
3. **`std/env` & `std/sys` Extension**: Đọc tham số dòng lệnh CLI (`process.argv`), biến môi trường hệ thống (`getEnv`, `setEnv`).

---

## 2. Thiết Kế Cú Pháp & Ví Dụ Mã Nguồn

```javascript
import { HashMap, Set } from "std/collections";
import { parseJSON, stringifyJSON } from "std/json";
import { getEnv, getArgs } from "std/env";

struct Product {
    id: number,
    name: string,
    price: number
}

function main(): number {
    // 1. Đọc tham số dòng lệnh & biến môi trường
    let args = getArgs();
    let port = getEnv("PORT") ?? "8080";
    print("Server running on port: " + port);

    // 2. HashMap<K, V> với Generics
    let map = new HashMap<string, Product>();
    let p1: Product;
    p1.id = 101.0;
    p1.name = "VIT Compiler Pro";
    p1.price = 99.0;

    map.set("prod_101", p1);
    
    if (map.has("prod_101")) {
        let item = map.get("prod_101");
        print("Found product: " + item.name);
    }

    // 3. Set<T> loại bỏ phần tử trùng lặp
    let uniqueTags = new Set<string>();
    uniqueTags.add("compiler");
    uniqueTags.add("llvm");
    uniqueTags.add("compiler"); // Bị loại bỏ

    print(uniqueTags.size); // Output: 2

    // 4. JSON Serialization / Deserialization
    let jsonStr = stringifyJSON(p1);
    print("Serialized JSON: " + jsonStr); // {"id": 101, "name": "VIT Compiler Pro", "price": 99}

    return 0;
}
```

---

## 3. Kiến Trúc Chi Tiết Cần Thay Đổi Trong Codebase

### 3.1. Thư Viện Cấu Trúc Dữ Liệu (`std/collections.vit`)
* Xây dựng `HashMap<K, V>` bằng kỹ thuật Open Addressing hoặc Separate Chaining với FFI helper viết bằng C++ runtime helper (`vit_rt_hashmap`).
* Tận dụng `Monomorphizer` của Phase 7 để tự động sinh mã Native cho từng cặp kiểu `<K, V>`.

### 3.2. JSON Parser Engine (`std/json.vit`)
* Viết JSON Lexer & Parser thuần bằng Vit / C Interop cho hiệu năng biên dịch cao nhất.
* Chuyển đổi JSON AST sang Struct / Generic Object của Vit.

### 3.3. Standard Environment Access (`std/env.vit`)
* Tích hợp C FFI `getenv()`, `argc`, `argv` từ điểm khởi chạy main trong C Runtime Wrapper (`vit_runtime_init`).

---

## 4. Danh Sách File Cần Cập Nhật Phân Chia Theo Task

1. **Task 1: C Runtime Utility Helpers (`src/runtime/collections_rt.cpp`)**
   - Triển khai Hash Table và Dynamic Storage helpers cho HashMap/Set.

2. **Task 2: Standard Module `std/collections.vit`**
   - Viết các Generic Struct Wrappers cho `HashMap<K, V>`, `Set<T>`, `Queue<T>`.

3. **Task 3: Standard Module `std/json.vit`**
   - Viết Parser và Stringifier cho dữ liệu JSON.

4. **Task 4: Standard Module `std/env.vit` & CLI Integration**
   - Tích hợp `getArgs()`, `getEnv()`.
   - `test/Phase-9/test_hashmap.vit`, `test/Phase-9/test_json.vit`.
