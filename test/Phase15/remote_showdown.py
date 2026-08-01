import sys
import os
import tarfile
import paramiko
import time
import re

HOST = "192.168.1.150"
PORT = 22
USER = "hoanglong"
PASS = "258456"

LOCAL_DIR = r"f:\Dev\product\vit-lag\vit"
TAR_PATH = r"f:\Dev\product\vit-lag\vit_showdown.tar.gz"
REMOTE_DIR = "/home/hoanglong/vit_showdown"

print("[1/6] Archiving local Vito, Go, and Rust source files...", flush=True)
with tarfile.open(TAR_PATH, "w:gz") as tar:
    for root, dirs, files in os.walk(LOCAL_DIR):
        rel_path = os.path.relpath(os.path.join(root), LOCAL_DIR)
        if any(part in rel_path for part in ["build", ".git", "node_modules", "tools"]):
            continue
        for file in files:
            full_path = os.path.join(root, file)
            file_rel = os.path.relpath(full_path, LOCAL_DIR)
            tar.add(full_path, arcname=file_rel)

print(f"Archive size: {os.path.getsize(TAR_PATH) / 1024 / 1024:.2f} MB", flush=True)

print("[2/6] Connecting to SSH server 192.168.1.150...", flush=True)
ssh = paramiko.SSHClient()
ssh.set_missing_host_key_policy(paramiko.AutoAddPolicy())
ssh.connect(HOST, port=PORT, username=USER, password=PASS, timeout=10)

def exec_sudo(cmd):
    print(f"--> Sudo Executing: {cmd}", flush=True)
    stdin, stdout, stderr = ssh.exec_command(f"echo '{PASS}' | sudo -S env DEBIAN_FRONTEND=noninteractive {cmd}")
    out = stdout.read().decode('utf-8', errors='ignore')
    err = stderr.read().decode('utf-8', errors='ignore')
    if out: print(out, flush=True)
    if err: print(err, flush=True)

def exec_cmd(cmd):
    print(f"--> Executing: {cmd}", flush=True)
    stdin, stdout, stderr = ssh.exec_command(cmd)
    out = stdout.read().decode('utf-8', errors='ignore')
    err = stderr.read().decode('utf-8', errors='ignore')
    return out, err

print("[3/6] Uploading tarball via SFTP...", flush=True)
sftp = ssh.open_sftp()
try:
    sftp.mkdir(REMOTE_DIR)
except Exception:
    pass
sftp.put(TAR_PATH, f"{REMOTE_DIR}/vit_showdown.tar.gz")
sftp.close()

print("[4/6] Extracting source code & Installing Go and Rust toolchains...", flush=True)
exec_cmd(f"mkdir -p {REMOTE_DIR} && cd {REMOTE_DIR} && tar -xzf vit_showdown.tar.gz")
exec_sudo("apt-get update -y")
exec_sudo("apt-get install -y --no-install-recommends build-essential gcc wrk liburing-dev golang-go rustc")

print("[5/6] Compiling Vito, Go, and Rust Native Servers...", flush=True)

# 1. Compile Vito Engine Server
print("--> Building Vito Framework Server...", flush=True)
exec_cmd(f"cd {REMOTE_DIR} && gcc -O3 -march=native -flto -Iinclude -Isrc test/Phase15/benchmark_server.c src/runtime/memory_rt.c src/runtime/async_iouring_rt.c src/runtime/http_parser_simd.c src/runtime/net_rt.c -pthread -o vito_server")

# 2. Compile Go Server
print("--> Building Go Server...", flush=True)
exec_cmd(f"cd {REMOTE_DIR} && go build -ldflags=\"-s -w\" -o go_server test/Phase15/go_server.go")

# 3. Compile Rust Server
print("--> Building Rust Server...", flush=True)
exec_cmd(f"cd {REMOTE_DIR} && rustc -O -C opt-level=3 -C target-cpu=native test/Phase15/rust_server.rs -o rust_server")

print("\n[6/6] === EXECUTING HEAD-TO-HEAD BENCHMARK SHOWDOWN ===", flush=True)

results = {}

def parse_wrk(out):
    req_sec = 0.0
    p99_lat = "N/A"
    avg_lat = "N/A"
    
    m_req = re.search(r"Requests/sec:\s+([\d\.]+)", out)
    if m_req: req_sec = float(m_req.group(1))
    
    m_avg = re.search(r"Latency\s+([\d\.]+ms|[\d\.]+us)", out)
    if m_avg: avg_lat = m_avg.group(1)

    m_p99 = re.search(r"99%\s+([\d\.]+ms|[\d\.]+us)", out)
    if m_p99: p99_lat = m_p99.group(1)
    
    return req_sec, avg_lat, p99_lat

# A. Vito Benchmark
print("\n--- Testing 1/3: Vito Framework (:8080) ---", flush=True)
exec_cmd("pkill -f vito_server || true")
ssh.exec_command(f"cd {REMOTE_DIR} && ./vito_server 8080 16 > vito.log 2>&1 &")
time.sleep(2)
out_vito, _ = exec_cmd("wrk -t16 -c1000 -d10s --latency http://localhost:8080/json")
print(out_vito, flush=True)
exec_cmd("pkill -f vito_server || true")
results["Vito Framework"] = parse_wrk(out_vito)

# B. Go Benchmark
print("\n--- Testing 2/3: Golang Server (:8081) ---", flush=True)
exec_cmd("pkill -f go_server || true")
ssh.exec_command(f"cd {REMOTE_DIR} && ./go_server > go.log 2>&1 &")
time.sleep(2)
out_go, _ = exec_cmd("wrk -t16 -c1000 -d10s --latency http://localhost:8081/json")
print(out_go, flush=True)
exec_cmd("pkill -f go_server || true")
results["Golang (net/http)"] = parse_wrk(out_go)

# C. Rust Benchmark
print("\n--- Testing 3/3: Rust Server (:8082) ---", flush=True)
exec_cmd("pkill -f rust_server || true")
ssh.exec_command(f"cd {REMOTE_DIR} && ./rust_server > rust.log 2>&1 &")
time.sleep(2)
out_rust, _ = exec_cmd("wrk -t16 -c1000 -d10s --latency http://localhost:8082/json")
print(out_rust, flush=True)
exec_cmd("pkill -f rust_server || true")
results["Rust (Native)"] = parse_wrk(out_rust)

print("\n" + "="*65, flush=True)
print("🏆 SHOWDOWN RESULTS: VITO FRAMEWORK vs GOLANG vs RUST", flush=True)
print("="*65, flush=True)
print(f"{'Framework / Language':<22} | {'Throughput (req/s)':<18} | {'Avg Latency':<12} | {'P99 Latency':<12}")
print("-" * 72)
for name, data in results.items():
    print(f"{name:<22} | {data[0]:>16,.2f} | {data[1]:>12} | {data[2]:>12}")
print("="*65, flush=True)

ssh.close()
if os.path.exists(TAR_PATH):
    os.remove(TAR_PATH)
