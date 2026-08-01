import sys
import os
import tarfile
import paramiko
import time
import re

HOST = os.getenv("BENCHMARK_SSH_HOST", "192.168.1.150")
PORT = int(os.getenv("BENCHMARK_SSH_PORT", "22"))
USER = os.getenv("BENCHMARK_SSH_USER", "hoanglong")
PASS = os.getenv("BENCHMARK_SSH_PASS", "")

LOCAL_DIR = r"f:\Dev\product\vit-lag\vit"
TAR_PATH = r"f:\Dev\product\vit-lag\pure_showdown.tar.gz"
REMOTE_DIR = "/home/hoanglong/pure_showdown"

print("[1/5] Archiving source files...", flush=True)
with tarfile.open(TAR_PATH, "w:gz") as tar:
    for root, dirs, files in os.walk(LOCAL_DIR):
        rel_path = os.path.relpath(os.path.join(root), LOCAL_DIR)
        if any(part in rel_path for part in ["build", ".git", "node_modules", "tools"]):
            continue
        for file in files:
            full_path = os.path.join(root, file)
            file_rel = os.path.relpath(full_path, LOCAL_DIR)
            tar.add(full_path, arcname=file_rel)

print("[2/5] Connecting to SSH server 192.168.1.150...", flush=True)
ssh = paramiko.SSHClient()
ssh.set_missing_host_key_policy(paramiko.AutoAddPolicy())
ssh.connect(HOST, port=PORT, username=USER, password=PASS, timeout=10)

def exec_cmd(cmd):
    print(f"--> Executing: {cmd}", flush=True)
    stdin, stdout, stderr = ssh.exec_command(cmd)
    out = stdout.read().decode('utf-8', errors='ignore')
    err = stderr.read().decode('utf-8', errors='ignore')
    if out: print(out, flush=True)
    if err: print(err, flush=True)
    return out, err

print("[3/5] Uploading tarball via SFTP...", flush=True)
sftp = ssh.open_sftp()
try:
    sftp.mkdir(REMOTE_DIR)
except Exception:
    pass
sftp.put(TAR_PATH, f"{REMOTE_DIR}/pure_showdown.tar.gz")
sftp.close()

print("[4/5] Extracting & Compiling Network Benchmarks...", flush=True)
exec_cmd(f"mkdir -p {REMOTE_DIR} && cd {REMOTE_DIR} && tar -xzf pure_showdown.tar.gz")

# 1. Compile Vito Engine Server
print("--> Building Vito Framework Server...", flush=True)
exec_cmd(f"cd {REMOTE_DIR} && gcc -O3 -march=native -flto -Iinclude -Isrc test/Phase15/servers/benchmark_server.c src/runtime/memory_rt.c src/runtime/async_iouring_rt.c src/runtime/http_parser_simd.c src/runtime/net_rt.c -pthread -o vito_server")

# 2. Compile C++ Server
print("--> Building C++ uWebSockets-style Server...", flush=True)
exec_cmd(f"cd {REMOTE_DIR} && g++ -O3 -march=native -flto test/Phase15/servers/bench_cpp.cpp -pthread -o cpp_server")

# 3. Compile Go Server
print("--> Building Go Server...", flush=True)
exec_cmd(f"cd {REMOTE_DIR} && go build -ldflags=\"-s -w\" -o go_server test/Phase15/servers/go_server.go")

# 4. Compile Rust Server
print("--> Building Rust Server...", flush=True)
exec_cmd(f"cd {REMOTE_DIR} && rustc -O -C opt-level=3 -C target-cpu=native test/Phase15/servers/rust_server.rs -o rust_server")

print("\n[5/5] === EXECUTING PURE LANGUAGE CPU BENCHMARK SHOWDOWN ===", flush=True)

results = {}

def parse_timing(out):
    fib_ms = 0.0
    mat_ms = 0.0
    m_fib = re.search(r"Fibonacci\(42\): \d+ in ([\d\.]+) ms", out)
    if m_fib: fib_ms = float(m_fib.group(1))
    
    m_mat = re.search(r"Matrix 500x500: in ([\d\.]+) ms", out)
    if m_mat: mat_ms = float(m_mat.group(1))
    
    return fib_ms, mat_ms

# Run Vit
print("\n--- 1. Vit Native (LLVM Pipeline) ---", flush=True)
out_vit, _ = exec_cmd(f"cd {REMOTE_DIR} && ./bench_vit")
results["Vit Native (LLVM)"] = parse_timing(out_vit)

# Run C++
print("\n--- 2. C++20 (GCC -O3 Native) ---", flush=True)
out_cpp, _ = exec_cmd(f"cd {REMOTE_DIR} && ./bench_cpp")
results["C++20 (GCC -O3)"] = parse_timing(out_cpp)

# Run Rust
print("\n--- 3. Rust (rustc -O Native) ---", flush=True)
out_rust, _ = exec_cmd(f"cd {REMOTE_DIR} && ./bench_rust")
results["Rust (rustc -O)"] = parse_timing(out_rust)

# Run Go
print("\n--- 4. Golang (gc 1.22) ---", flush=True)
out_go, _ = exec_cmd(f"cd {REMOTE_DIR} && ./bench_go")
results["Golang (gc compiler)"] = parse_timing(out_go)

print("\n" + "="*70, flush=True)
print("PURE LANGUAGE CPU BENCHMARK RESULTS: VIT vs C++ vs RUST vs GO", flush=True)
print("="*70, flush=True)
print(f"{'Language / Compiler':<24} | {'Fibonacci(42) Time':<20} | {'Matrix 500x500 Time':<20}")
print("-" * 70)
for lang, timing in results.items():
    print(f"{lang:<24} | {timing[0]:>17.2f} ms | {timing[1]:>17.2f} ms")
print("="*70, flush=True)

ssh.close()
if os.path.exists(TAR_PATH):
    os.remove(TAR_PATH)
