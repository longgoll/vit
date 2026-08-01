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
TAR_PATH = r"f:\Dev\product\vit-lag\res_showdown.tar.gz"
REMOTE_DIR = "/home/hoanglong/res_showdown"

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
    return out, err

print("[3/5] Uploading tarball via SFTP...", flush=True)
sftp = ssh.open_sftp()
try:
    sftp.mkdir(REMOTE_DIR)
except Exception:
    pass
sftp.put(TAR_PATH, f"{REMOTE_DIR}/res_showdown.tar.gz")
sftp.close()

print("[4/5] Extracting & Compiling Vito, Go, and Rust servers...", flush=True)
exec_cmd(f"mkdir -p {REMOTE_DIR} && cd {REMOTE_DIR} && tar -xzf res_showdown.tar.gz")

# 1. Compile Vito Engine Server
print("--> Building Vito Framework Server...", flush=True)
exec_cmd(f"cd {REMOTE_DIR} && gcc -O3 -march=native -flto -Iinclude -Isrc test/Phase15/servers/benchmark_server.c src/runtime/memory_rt.c src/runtime/async_iouring_rt.c src/runtime/http_parser_simd.c src/runtime/net_rt.c -pthread -o vito_server")

# 2. Compile Go Server
print("--> Building Go Server...", flush=True)
exec_cmd(f"cd {REMOTE_DIR} && go build -ldflags=\"-s -w\" -o go_server test/Phase15/servers/go_server.go")

# 3. Compile Rust Server
print("--> Building Rust Server...", flush=True)
exec_cmd(f"cd {REMOTE_DIR} && rustc -O -C opt-level=3 -C target-cpu=native test/Phase15/servers/rust_server.rs -o rust_server")

print("\n[5/5] === EXECUTING RESOURCE & PERFORMANCE BENCHMARK SHOWDOWN ===", flush=True)

def measure_target(bin_name, port):
    print(f"\n--- Testing Resource Usage: {bin_name} ---", flush=True)
    exec_cmd(f"pkill -f {bin_name} || true")
    ssh.exec_command(f"cd {REMOTE_DIR} && ./{bin_name} > {bin_name}.log 2>&1 &")
    time.sleep(2)
    
    out_pid, _ = exec_cmd(f"pgrep -f {bin_name} | head -n 1")
    pid = out_pid.strip()
    
    idle_ram_mb = 0.0
    if pid:
        out_rss, _ = exec_cmd(f"ps -o rss= -p {pid}")
        try:
            idle_ram_mb = float(out_rss.strip()) / 1024.0
        except Exception:
            pass

    ssh.exec_command(f"wrk -t16 -c1000 -d10s --latency http://localhost:{port}/json > {REMOTE_DIR}/wrk_{bin_name}.log 2>&1")
    
    peak_ram_mb = idle_ram_mb
    for _ in range(8):
        time.sleep(1)
        if pid:
            out_rss, _ = exec_cmd(f"ps -o rss= -p {pid} 2>/dev/null")
            try:
                cur_rss = float(out_rss.strip()) / 1024.0
                if cur_rss > peak_ram_mb:
                    peak_ram_mb = cur_rss
            except Exception:
                pass
                
    time.sleep(3)
    out_wrk, _ = exec_cmd(f"cat {REMOTE_DIR}/wrk_{bin_name}.log")
    exec_cmd(f"pkill -f {bin_name} || true")

    req_sec = 0.0
    p99_lat = "N/A"
    m_req = re.search(r"Requests/sec:\s+([\d\.]+)", out_wrk)
    if m_req: req_sec = float(m_req.group(1))
    m_p99 = re.search(r"99%\s+([\d\.]+ms|[\d\.]+us)", out_wrk)
    if m_p99: p99_lat = m_p99.group(1)

    return idle_ram_mb, peak_ram_mb, req_sec, p99_lat

res_vito = measure_target("vito_server", 8080)
res_go = measure_target("go_server", 8081)
res_rust = measure_target("rust_server", 8082)

print("\n" + "="*80, flush=True)
print("RESOURCE CONSUMPTION RESULTS: VITO FRAMEWORK vs GOLANG vs RUST", flush=True)
print("="*80, flush=True)
print(f"{'Framework / Language':<22} | {'Idle RAM':<10} | {'Peak RAM (Load)':<15} | {'Throughput':<15} | {'P99 Latency':<12}")
print("-" * 80)
print(f"{'Vito Framework':<22} | {res_vito[0]:>7.2f} MB | {res_vito[1]:>12.2f} MB | {res_vito[2]:>12,.2f} r/s | {res_vito[3]:>12}")
print(f"{'Golang (net/http)':<22} | {res_go[0]:>7.2f} MB | {res_go[1]:>12.2f} MB | {res_go[2]:>12,.2f} r/s | {res_go[3]:>12}")
print(f"{'Rust (Native)':<22} | {res_rust[0]:>7.2f} MB | {res_rust[1]:>12.2f} MB | {res_rust[2]:>12,.2f} r/s | {res_rust[3]:>12}")
print("="*80, flush=True)

ssh.close()
if os.path.exists(TAR_PATH):
    os.remove(TAR_PATH)
