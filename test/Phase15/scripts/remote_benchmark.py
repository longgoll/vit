import sys
import os
import tarfile
import paramiko
import time

HOST = os.getenv("BENCHMARK_SSH_HOST", "192.168.1.150")
PORT = int(os.getenv("BENCHMARK_SSH_PORT", "22"))
USER = os.getenv("BENCHMARK_SSH_USER", "hoanglong")
PASS = os.getenv("BENCHMARK_SSH_PASS", "")

LOCAL_DIR = r"f:\Dev\product\vit-lag\vit"
TAR_PATH = r"f:\Dev\product\vit-lag\vit_code.tar.gz"
REMOTE_DIR = "/home/hoanglong/vit_test"

print("[1/5] Archiving local Vit source files...", flush=True)
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

print("[2/5] Connecting to SSH server 192.168.1.150...", flush=True)
ssh = paramiko.SSHClient()
ssh.set_missing_host_key_policy(paramiko.AutoAddPolicy())
ssh.connect(HOST, port=PORT, username=USER, password=PASS, timeout=10)

def exec_sudo(cmd):
    print(f"--> Sudo Executing: {cmd}", flush=True)
    stdin, stdout, stderr = ssh.exec_command(f"echo '{PASS}' | sudo -S {cmd}")
    out = stdout.read().decode('utf-8', errors='ignore')
    err = stderr.read().decode('utf-8', errors='ignore')
    if out: print(out, flush=True)
    if err: print(err, flush=True)

def exec_cmd(cmd):
    print(f"--> Executing: {cmd}", flush=True)
    stdin, stdout, stderr = ssh.exec_command(cmd)
    out = stdout.read().decode('utf-8', errors='ignore')
    err = stderr.read().decode('utf-8', errors='ignore')
    if out: print(out, flush=True)
    if err: print(err, flush=True)
    return out, err

print("[3/5] Uploading source tarball via SFTP...", flush=True)
sftp = ssh.open_sftp()
try:
    sftp.mkdir(REMOTE_DIR)
except Exception:
    pass
sftp.put(TAR_PATH, f"{REMOTE_DIR}/vit_code.tar.gz")
sftp.close()

print("[4/5] Extracting source code...", flush=True)
exec_cmd(f"mkdir -p {REMOTE_DIR} && cd {REMOTE_DIR} && tar -xzf vit_code.tar.gz")

print("[4b] Installing build-essential & gcc & wrk...", flush=True)
exec_sudo("apt-get update -y")
exec_sudo("apt-get install -y build-essential gcc wrk liburing-dev")

print("[4c] Compiling Native Benchmark Server...", flush=True)
exec_cmd(f"cd {REMOTE_DIR} && gcc -O3 -march=native -flto -Iinclude -Isrc test/Phase15/servers/benchmark_server.c src/runtime/memory_rt.c src/runtime/async_iouring_rt.c src/runtime/http_parser_simd.c src/runtime/net_rt.c -pthread -o linux_benchmark_server")

print("[5/5] Running High-Throughput Bare-Metal Linux Benchmark (wrk)...", flush=True)
exec_cmd(f"pkill -f linux_benchmark_server || true")
ssh.exec_command(f"cd {REMOTE_DIR} && ./linux_benchmark_server 8080 16 > server.log 2>&1 &")

time.sleep(2)

print("\n=== RUNNING WRK STRESS BENCHMARK ON LINUX SERVER ===", flush=True)
out, err = exec_cmd("wrk -t16 -c1000 -d10s --latency http://localhost:8080/json")

exec_cmd(f"pkill -f linux_benchmark_server || true")
ssh.close()
if os.path.exists(TAR_PATH):
    os.remove(TAR_PATH)
print("=== BENCHMARK COMPLETED SUCCESSFULLY ===", flush=True)
